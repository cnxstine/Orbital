#pragma once

/**
 * @file events/EventBus.hpp
 * @brief Typed, synchronous event bus with RAII subscription tokens.
 *
 * Architecture overview:
 *   - Producers call EventBus::Post<E>(event)  — stores event in a queue.
 *   - EventBus::Dispatch()  — drains ALL queues and calls subscribers, once per frame.
 *   - Subscribers receive events via std::function<bool(const E&)> callbacks.
 *   - Returning true from a handler marks the event as Handled and stops propagation.
 *
 * Thread safety:
 *   - Post() is thread-safe (mutex-protected enqueue).
 *   - Dispatch() must be called from the main thread only.
 *   - Subscribe() / Unsubscribe() must be called from the main thread.
 *
 * RAII tokens:
 *   auto token = EventBus::Subscribe<KeyPressedEvent>([](const auto& e){ ... });
 *   // token destructor calls Unsubscribe automatically.
 */

#include "events/Event.hpp"
#include "core/Log.hpp"

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <any>
#include <cstdint>

namespace Orbital {

/// @cond INTERNAL
namespace detail {

/// Type-erased handler wrapper
struct HandlerEntry {
    uint64_t                        id;
    std::function<bool(Event&)>     invoke; ///< Returns true if event consumed
};

} // namespace detail
/// @endcond

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief RAII handle for an EventBus subscription.
 *
 * Destroys itself by unsubscribing when it goes out of scope.
 * Move-only (cannot be copied to avoid double-unsubscribe).
 */
class SubscriptionToken {
public:
    SubscriptionToken() = default;
    SubscriptionToken(std::type_index type, uint64_t id, class EventBus* bus);
    ~SubscriptionToken();

    SubscriptionToken(SubscriptionToken&&) noexcept;
    SubscriptionToken& operator=(SubscriptionToken&&) noexcept;

    SubscriptionToken(const SubscriptionToken&)            = delete;
    SubscriptionToken& operator=(const SubscriptionToken&) = delete;

    /// Manually release the subscription before token destruction.
    void Release();

    [[nodiscard]] bool Valid() const noexcept { return m_Bus != nullptr; }

private:
    std::type_index  m_Type = std::type_index(typeid(void));
    uint64_t         m_Id   = 0;
    EventBus*        m_Bus  = nullptr;
};

// ─────────────────────────────────────────────────────────────────────────────

class EventBus {
public:
    EventBus()  = default;
    ~EventBus() = default;

    EventBus(const EventBus&)            = delete;
    EventBus& operator=(const EventBus&) = delete;

    // ── Subscription ──────────────────────────────────────────────────────────

    /**
     * @brief Subscribe to events of type E.
     * @param handler Callable receiving const E&; returning true consumes the event.
     * @return RAII token — keep alive for the duration of the subscription.
     */
    template <typename E>
    [[nodiscard]] SubscriptionToken Subscribe(std::function<bool(const E&)> handler)
    {
        static_assert(std::is_base_of_v<Event, E>,
                      "EventBus::Subscribe<E>: E must inherit from Event");

        const uint64_t id = m_NextId++;
        detail::HandlerEntry entry{
            id,
            [h = std::move(handler)](Event& e) -> bool {
                return h(static_cast<const E&>(e));
            }
        };

        m_Handlers[std::type_index(typeid(E))].push_back(std::move(entry));
        return SubscriptionToken(std::type_index(typeid(E)), id, this);
    }

    // ── Posting ───────────────────────────────────────────────────────────────

    /**
     * @brief Enqueue an event for dispatch at the next Dispatch() call.
     *        Thread-safe: may be called from any thread.
     */
    template <typename E>
    void Post(E event)
    {
        static_assert(std::is_base_of_v<Event, E>, "Post<E>: E must inherit from Event");

        std::lock_guard lock(m_QueueMutex);
        // Store as heap-allocated Event* (type-erased) in the pending queue
        m_PendingQueue.push(std::make_unique<E>(std::move(event)));
    }

    // ── Dispatch ──────────────────────────────────────────────────────────────

    /**
     * @brief Drain the pending queue and call all matching subscribers.
     *        Must be called from the main thread once per frame.
     */
    void Dispatch();

    // ── Internal (called by SubscriptionToken) ────────────────────────────────
    void Unsubscribe(std::type_index type, uint64_t id);

private:
    std::unordered_map<std::type_index, std::vector<detail::HandlerEntry>> m_Handlers;

    std::queue<std::unique_ptr<Event>> m_PendingQueue;
    mutable std::mutex                 m_QueueMutex;

    uint64_t m_NextId = 1;
};

} // namespace Orbital
