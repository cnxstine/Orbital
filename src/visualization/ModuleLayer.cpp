#include "visualization/ModuleLayer.hpp"
#include "visualization/OrbitalViewerModule.hpp"
#include "visualization/MolecularOrbitalExplorerModule.hpp"
#include "visualization/HybridOrbitalExplorerModule.hpp"
#include "core/Log.hpp"
#include <imgui.h>

namespace Orbital {

ModuleLayer::ModuleLayer(Engine& engine)
    : Layer("ModuleLayer"),
      m_Engine(engine)
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
    ORB_CORE_INFO("ModuleLayer::OnImGui called");
    ImGui::Begin("Module Navigator");
    const char* moduleNames[] = { "Hydrogen Orbital Explorer", "Bonding Explorer", "Hybrid Orbital Explorer" };
    int activeIdx = 0;
    if (dynamic_cast<MolecularOrbitalExplorerModule*>(m_ActiveModule.get())) {
        activeIdx = 1;
    } else if (dynamic_cast<HybridOrbitalExplorerModule*>(m_ActiveModule.get())) {
        activeIdx = 2;
    }

    if (ImGui::Combo("Active Module", &activeIdx, moduleNames, 3)) {
        if (activeIdx == 0) {
            SetActiveModule(std::make_unique<OrbitalViewerModule>(m_Engine));
        } else if (activeIdx == 1) {
            SetActiveModule(std::make_unique<MolecularOrbitalExplorerModule>(m_Engine));
        } else if (activeIdx == 2) {
            SetActiveModule(std::make_unique<HybridOrbitalExplorerModule>(m_Engine));
        }
    }
    ImGui::End();

    if (m_ActiveModule) {
        m_ActiveModule->OnParameterPanel();
    }
}

bool ModuleLayer::OnEvent(Event& event)
{
    if (m_ActiveModule) {
        return m_ActiveModule->OnEvent(event);
    }
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
