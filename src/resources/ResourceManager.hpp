#pragma once

/**
 * @file resources/ResourceManager.hpp
 * @brief Central resource cache with typed Handle<T> access.
 *
 * Design:
 *  - Each resource type T gets its own cache slot: unordered_map<Handle<T>, unique_ptr<T>>.
 *  - Deduplication: path → Handle<T> map avoids loading the same asset twice.
 *  - Loaders are plugged in via the Loader<T> struct specialization pattern.
 *    Each specialization provides:
 *       static std::unique_ptr<T> Load(std::string_view path)
 *  - Thread safety: loading/caching is protected by a shared_mutex.
 *    Reads (Get) use a shared lock; writes (Load/Unload) use exclusive locks.
 *
 * Lifetime tiers are managed by the caller: a module calls Clear() on its
 * dedicated ResourceManager instance, or the Engine calls Clear() on shutdown.
 *
 * Usage:
 *   auto& rm = Engine::GetResourceManager();
 *   Handle<GLShader> h = rm.Load<GLShader>("shaders/geometry/mesh");
 *   GLShader* s = rm.Get(h);    // nullptr only if manually unloaded
 */

#include "resources/Handle.hpp"
#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <any>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <memory>
#include <shared_mutex>
#include <atomic>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Loader<T> — specialise to teach ResourceManager how to load each type T.
// ─────────────────────────────────────────────────────────────────────────────
template <typename T>
struct Loader {
    // Specializations must implement:
    //   static std::unique_ptr<T> Load(std::string_view path);
    // A static_assert in ResourceManager::Load() will fire if missing.
};

// ─────────────────────────────────────────────────────────────────────────────

class ResourceManager {
public:
    ResourceManager()  = default;
    ~ResourceManager() = default;

    ResourceManager(const ResourceManager&)            = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // ── Loading ───────────────────────────────────────────────────────────────

    /**
     * @brief Load (or retrieve from cache) a resource of type T.
     * @param path  Asset path relative to ORBITAL_ASSET_DIR, without extension.
     *              Loader<T>::Load receives the resolved absolute path.
     * @return A valid Handle<T> on success; Handle<T>::Null() on failure.
     */
    template <typename T>
    [[nodiscard]] Handle<T> Load(std::string_view path)
    {
        const std::string key(path);

        // ── Shared read lock: check cache ──────────────────────────────────
        {
            std::shared_lock lock(m_Mutex);
            auto& pathMap = GetPathMap<T>();
            if (auto it = pathMap.find(key); it != pathMap.end()) {
                return Handle<T>::FromId(it->second);
            }
        }

        // ── Exclusive write lock: load and insert ──────────────────────────
        std::unique_lock lock(m_Mutex);

        // Double-checked locking: another thread may have inserted between locks
        auto& pathMap = GetPathMap<T>();
        if (auto it = pathMap.find(key); it != pathMap.end()) {
            return Handle<T>::FromId(it->second);
        }

        // Invoke the Loader specialization
        auto resource = Loader<T>::Load(path);
        if (!resource) {
            ORB_CORE_ERROR("ResourceManager: failed to load '{}'", path);
            return Handle<T>::Null();
        }

        const uint64_t id  = m_NextId.fetch_add(1, std::memory_order_relaxed);
        const Handle<T> handle = Handle<T>::FromId(id);

        GetCache<T>().emplace(handle, std::move(resource));
        pathMap.emplace(key, id);

        ORB_CORE_TRACE("ResourceManager: loaded '{}' (id={})", path, id);
        return handle;
    }

    /**
     * @brief Register an externally created resource under a new Handle<T>.
     * @return A valid Handle<T> representing the registered resource.
     */
    template <typename T>
    [[nodiscard]] Handle<T> Register(std::unique_ptr<T> resource)
    {
        std::unique_lock lock(m_Mutex);
        const uint64_t id = m_NextId.fetch_add(1, std::memory_order_relaxed);
        const Handle<T> handle = Handle<T>::FromId(id);
        GetCache<T>().emplace(handle, std::move(resource));
        return handle;
    }

    // ── Access ────────────────────────────────────────────────────────────────

    /// @return Raw pointer to the resource, or nullptr if handle is invalid.
    template <typename T>
    [[nodiscard]] T* Get(Handle<T> handle)
    {
        if (!handle.IsValid()) return nullptr;
        std::shared_lock lock(m_Mutex);
        auto& cache = GetCache<T>();
        if (auto it = cache.find(handle); it != cache.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    // ── Removal ───────────────────────────────────────────────────────────────

    template <typename T>
    void Unload(Handle<T> handle)
    {
        if (!handle.IsValid()) return;
        std::unique_lock lock(m_Mutex);
        GetCache<T>().erase(handle);
        // Note: path map entry is left (load will re-insert if path is reused)
    }

    /// Destroy all resources of every type.
    void Clear()
    {
        std::unique_lock lock(m_Mutex);
        m_Caches.clear();
        m_PathMaps.clear();
        ORB_CORE_INFO("ResourceManager cleared");
    }

    // ── Statistics ────────────────────────────────────────────────────────────

    template <typename T>
    [[nodiscard]] std::size_t Count()
    {
        std::shared_lock lock(m_Mutex);
        auto it = m_Caches.find(std::type_index(typeid(T)));
        return (it != m_Caches.end())
            ? std::any_cast<std::shared_ptr<Cache<T>>>(it->second)->size()
            : 0;
    }

private:
    // ── Internal type-indexed caches ──────────────────────────────────────────

    template <typename T>
    using Cache = std::unordered_map<Handle<T>, std::unique_ptr<T>>;

    template <typename T>
    Cache<T>& GetCache()
    {
        auto key = std::type_index(typeid(T));
        auto it  = m_Caches.find(key);
        if (it == m_Caches.end()) {
            m_Caches.emplace(key, std::make_shared<Cache<T>>());
            it = m_Caches.find(key);
        }
        return *std::any_cast<std::shared_ptr<Cache<T>>>(it->second);
    }

    template <typename T>
    std::unordered_map<std::string, uint64_t>& GetPathMap()
    {
        auto key = std::type_index(typeid(T));
        return m_PathMaps[key];
    }

    mutable std::shared_mutex  m_Mutex;
    std::unordered_map<std::type_index, std::any> m_Caches;
    std::unordered_map<std::type_index, std::unordered_map<std::string, uint64_t>> m_PathMaps;
    std::atomic<uint64_t> m_NextId { 1 };
};

} // namespace Orbital
