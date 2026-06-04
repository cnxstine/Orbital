#include "events/EventBus.hpp"
#include "core/Assert.hpp"

#include <algorithm>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// SubscriptionToken
// ─────────────────────────────────────────────────────────────────────────────

SubscriptionToken::SubscriptionToken(std::type_index type, uint64_t id, EventBus* bus)
    : m_Type(type), m_Id(id), m_Bus(bus)
{}

SubscriptionToken::~SubscriptionToken()
{
    Release();
}

SubscriptionToken::SubscriptionToken(SubscriptionToken&& other) noexcept
    : m_Type(other.m_Type), m_Id(other.m_Id), m_Bus(other.m_Bus)
{
    other.m_Bus = nullptr;
}

SubscriptionToken& SubscriptionToken::operator=(SubscriptionToken&& other) noexcept
{
    if (this != &other) {
        Release();
        m_Type      = other.m_Type;
        m_Id        = other.m_Id;
        m_Bus       = other.m_Bus;
        other.m_Bus = nullptr;
    }
    return *this;
}

void SubscriptionToken::Release()
{
    if (m_Bus) {
        m_Bus->Unsubscribe(m_Type, m_Id);
        m_Bus = nullptr;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// EventBus
// ─────────────────────────────────────────────────────────────────────────────

void EventBus::Dispatch()
{
    // Swap queue to a local copy under the lock, then process lock-free
    std::queue<std::unique_ptr<Event>> local;
    {
        std::lock_guard lock(m_QueueMutex);
        std::swap(local, m_PendingQueue);
    }

    while (!local.empty()) {
        auto& event = *local.front();

        const auto typeKey = std::type_index(typeid(event));
        auto it = m_Handlers.find(typeKey);
        if (it != m_Handlers.end()) {
            for (auto& entry : it->second) {
                if (event.Handled) break;
                const bool consumed = entry.invoke(event);
                if (consumed) event.Handled = true;
            }
        }

        local.pop();
    }
}

void EventBus::Unsubscribe(std::type_index type, uint64_t id)
{
    auto it = m_Handlers.find(type);
    if (it == m_Handlers.end()) return;

    auto& vec = it->second;
    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
            [id](const detail::HandlerEntry& e) { return e.id == id; }),
        vec.end());
}

} // namespace Orbital
