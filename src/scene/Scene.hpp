#pragma once

/**
 * @file scene/Scene.hpp
 * @brief Scene container: entity registry and component storage.
 *
 * Implementation uses a flat ECS design:
 *   - Entities are uint64_t IDs.
 *   - Components are stored as std::any in a nested unordered_map:
 *       { EntityID → { type_index → any(ComponentT) } }
 *
 * This is intentionally simple for the rendering foundation milestone.
 * A future iteration will replace this with a sparse-set ECS (EnTT-compatible)
 * for cache-efficient iteration over large entity sets.
 *
 * All template methods are defined here (cannot be split to .cpp).
 */

#include "scene/Entity.hpp"

#include <any>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <functional>

namespace Orbital {

class Scene {
public:
    explicit Scene(std::string_view name = "Scene");
    ~Scene();

    Scene(const Scene&)            = delete;
    Scene& operator=(const Scene&) = delete;

    // ── Entity lifecycle ──────────────────────────────────────────────────────

    /// Create a new entity and optionally assign a debug name.
    [[nodiscard]] Entity CreateEntity(std::string_view name = "Entity");

    /// Destroy an entity and all its components.
    void DestroyEntity(Entity e);

    // ── Component access ──────────────────────────────────────────────────────

    /**
     * @brief Add a component to an entity (copy or move).
     * @return Reference to the stored component.
     */
    template <typename T>
    T& AddComponent(Entity e, T component)
    {
        m_Components[e.id][std::type_index(typeid(T))] = std::move(component);
        return std::any_cast<T&>(m_Components[e.id][std::type_index(typeid(T))]);
    }

    /**
     * @brief Retrieve a mutable pointer to a component.
     * @return nullptr if the entity doesn't have the component.
     */
    template <typename T>
    [[nodiscard]] T* GetComponent(Entity e)
    {
        auto it = m_Components.find(e.id);
        if (it == m_Components.end()) return nullptr;

        auto cit = it->second.find(std::type_index(typeid(T)));
        if (cit == it->second.end()) return nullptr;

        return std::any_cast<T>(&cit->second);
    }

    template <typename T>
    [[nodiscard]] const T* GetComponent(Entity e) const
    {
        auto it = m_Components.find(e.id);
        if (it == m_Components.end()) return nullptr;

        auto cit = it->second.find(std::type_index(typeid(T)));
        if (cit == it->second.end()) return nullptr;

        return std::any_cast<T>(&cit->second);
    }

    /// Returns true if the entity has a component of type T.
    template <typename T>
    [[nodiscard]] bool HasComponent(Entity e) const
    {
        return GetComponent<T>(e) != nullptr;
    }

    /// Remove a component from an entity (no-op if not present).
    template <typename T>
    void RemoveComponent(Entity e)
    {
        auto it = m_Components.find(e.id);
        if (it != m_Components.end())
            it->second.erase(std::type_index(typeid(T)));
    }

    /**
     * @brief Iterate all entities that have component T, calling func(Entity, T&).
     *
     * Example:
     *   scene.Each<TransformComponent>([](Entity e, TransformComponent& t) {
     *       t.Position.y += 0.1f;
     *   });
     */
    template <typename T>
    void Each(std::function<void(Entity, T&)> func)
    {
        for (auto& [id, comps] : m_Components) {
            auto it = comps.find(std::type_index(typeid(T)));
            if (it != comps.end()) {
                func(Entity{id}, std::any_cast<T&>(it->second));
            }
        }
    }

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    void OnUpdate(float dt);
    void Clear();

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] std::string_view  GetName()        const noexcept { return m_Name; }
    [[nodiscard]] std::size_t       GetEntityCount()  const noexcept { return m_Components.size(); }
    [[nodiscard]] const std::vector<std::string>& GetEntityNames() const noexcept { return m_EntityNames; }

private:
    std::string  m_Name;
    EntityID     m_NextId     = 1;

    /// Component storage: entity → type → any
    std::unordered_map<EntityID, std::unordered_map<std::type_index, std::any>> m_Components;

    /// Debug names (indexed parallel to entity IDs; not performance-critical)
    std::unordered_map<EntityID, std::string> m_DebugNames;
    std::vector<std::string> m_EntityNames; ///< Ordered for UI display
};

} // namespace Orbital
