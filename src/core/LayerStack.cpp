#include "core/LayerStack.hpp"
#include "core/Assert.hpp"
#include "core/Log.hpp"

#include <algorithm>
#include <ranges>

namespace Orbital {

Layer* LayerStack::PushLayer(std::unique_ptr<Layer> layer)
{
    ORB_ASSERT(layer != nullptr, "Cannot push a null layer");
    Layer* raw = layer.get();

    // Insert before the overlay region
    auto it = m_Layers.begin() + static_cast<std::ptrdiff_t>(m_LayerInsert);
    m_Layers.insert(it, std::move(layer));
    ++m_LayerInsert;

    raw->OnAttach();
    ORB_CORE_INFO("Layer '{}' pushed  (stack size: {})", raw->GetName(), m_Layers.size());
    return raw;
}

Layer* LayerStack::PushOverlay(std::unique_ptr<Layer> overlay)
{
    ORB_ASSERT(overlay != nullptr, "Cannot push a null overlay");
    Layer* raw = overlay.get();

    // Overlays always go at the end
    m_Layers.push_back(std::move(overlay));

    raw->OnAttach();
    ORB_CORE_INFO("Overlay '{}' pushed (stack size: {})", raw->GetName(), m_Layers.size());
    return raw;
}

void LayerStack::PopLayer(std::string_view name)
{
    auto it = std::find_if(
        m_Layers.begin(),
        m_Layers.begin() + static_cast<std::ptrdiff_t>(m_LayerInsert),
        [name](const auto& l) { return l->GetName() == name; });

    if (it != m_Layers.begin() + static_cast<std::ptrdiff_t>(m_LayerInsert)) {
        (*it)->OnDetach();
        m_Layers.erase(it);
        --m_LayerInsert;
        ORB_CORE_INFO("Layer '{}' popped  (stack size: {})", name, m_Layers.size());
    } else {
        ORB_CORE_WARN("PopLayer: layer '{}' not found", name);
    }
}

void LayerStack::PopOverlay(std::string_view name)
{
    auto it = std::find_if(
        m_Layers.begin() + static_cast<std::ptrdiff_t>(m_LayerInsert),
        m_Layers.end(),
        [name](const auto& l) { return l->GetName() == name; });

    if (it != m_Layers.end()) {
        (*it)->OnDetach();
        m_Layers.erase(it);
        ORB_CORE_INFO("Overlay '{}' popped (stack size: {})", name, m_Layers.size());
    } else {
        ORB_CORE_WARN("PopOverlay: overlay '{}' not found", name);
    }
}

void LayerStack::Clear()
{
    // Detach in reverse order (top-most first)
    for (auto it = m_Layers.rbegin(); it != m_Layers.rend(); ++it) {
        (*it)->OnDetach();
    }
    m_Layers.clear();
    m_LayerInsert = 0;
    ORB_CORE_INFO("LayerStack cleared");
}

} // namespace Orbital
