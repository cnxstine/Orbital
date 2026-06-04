#include "visualization/ModuleLayer.hpp"

namespace Orbital {

ModuleLayer::ModuleLayer()
    : Layer("ModuleLayer")
{
}

ModuleLayer::~ModuleLayer()
{
    if (m_ActiveModule) {
        m_ActiveModule->OnExit();
    }
}

void ModuleLayer::OnAttach()
{
    if (m_ActiveModule) {
        m_ActiveModule->OnEnter();
    }
}

void ModuleLayer::OnDetach()
{
    if (m_ActiveModule) {
        m_ActiveModule->OnExit();
    }
}

void ModuleLayer::OnUpdate(float dt)
{
    if (m_ActiveModule) {
        m_ActiveModule->Update(dt);
    }
}

void ModuleLayer::OnRender()
{
    if (m_ActiveModule) {
        m_ActiveModule->Render();
    }
}

void ModuleLayer::OnImGui()
{
    if (m_ActiveModule) {
        m_ActiveModule->OnParameterPanel();
    }
}

bool ModuleLayer::OnEvent(Event& event)
{
    return false;
}

void ModuleLayer::SetActiveModule(std::unique_ptr<VisualizationModule> module)
{
    if (m_ActiveModule) {
        m_ActiveModule->OnExit();
    }
    m_ActiveModule = std::move(module);
    if (m_ActiveModule) {
        m_ActiveModule->OnEnter();
    }
}

} // namespace Orbital
