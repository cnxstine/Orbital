#pragma once

/**
 * @file visualization/ModuleLayer.hpp
 * @brief Layer subclass for driving VisualizationModules.
 */

#include "core/Layer.hpp"
#include "visualization/VisualizationModule.hpp"
#include <memory>

namespace Orbital {
    class Engine;
}

namespace Orbital {

class ModuleLayer : public Layer {
public:
    explicit ModuleLayer(Engine& engine);
    virtual ~ModuleLayer() override;

    // Layer overrides
    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float dt) override;
    virtual void OnRender() override;
    virtual void OnImGui() override;
    virtual bool OnEvent(Event& event) override;

    void SetActiveModule(std::unique_ptr<VisualizationModule> module);
    [[nodiscard]] VisualizationModule* GetActiveModule() const noexcept { return m_ActiveModule.get(); }
    [[nodiscard]] Engine& GetEngine() const noexcept { return m_Engine; }

private:
    Engine& m_Engine;
    std::unique_ptr<VisualizationModule> m_ActiveModule;
};

} // namespace Orbital
