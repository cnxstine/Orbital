#pragma once

/**
 * @file visualization/OrbitalViewerModule.hpp
 * @brief Concrete VisualizationModule for hydrogen orbitals rendering.
 */

#include "visualization/VisualizationModule.hpp"
#include "visualization/HydrogenicOrbital.hpp"
#include "visualization/MonteCarloSamplingPipeline.hpp"
#include "visualization/StochasticProbabilityCloud.hpp"
#include "renderer/backend/GLFramebuffer.hpp"
#include "renderer/backend/GLVertexArray.hpp"
#include "resources/Handle.hpp"
#include "renderer/backend/GLShader.hpp"

#include <memory>

namespace Orbital {

class Engine;

class OrbitalViewerModule : public VisualizationModule {
public:
    explicit OrbitalViewerModule(Engine& engine);
    virtual ~OrbitalViewerModule() override = default;

    // VisualizationModule overrides
    virtual void OnEnter() override;
    virtual void OnExit() override;
    virtual void Update(float dt) override;
    virtual void Render() override;
    virtual void OnParameterPanel() override;

private:
    void SetupProbabilityCloud();
    void ReallocateFBO();
    void RenderFrameInternal();
    void UpdateOrbitalStateFromSelection();

    Engine& m_Engine;

    // Active simulation state
    std::shared_ptr<HydrogenicOrbital> m_ActiveOrbital;
    std::shared_ptr<MonteCarloSamplingPipeline> m_SamplingPipeline;
    std::shared_ptr<StochasticProbabilityCloud> m_ProbabilityCloud;

    // Rendering assets
    std::unique_ptr<GLFramebuffer> m_Framebuffer;
    std::unique_ptr<GLVertexArray> m_PointsVAO;
    std::unique_ptr<GLVertexArray> m_DummyVAO; // for fullscreen quad resolve

    Handle<GLShader> m_PointDensityShader = Handle<GLShader>::Null();
    Handle<GLShader> m_ResolveShader = Handle<GLShader>::Null();

    // User parameters controlled via ImGui panel
    int m_QuantumN = 1;
    int m_QuantumL = 0;
    int m_QuantumM = 0;
    bool m_IsRealCombo = false;

    enum class SelectedState { H_1s, H_2s, H_2p_x, H_2p_y, H_2p_z };
    SelectedState m_SelectedState = SelectedState::H_1s;
    bool m_GenerateVerificationPackage = false;
    bool m_AutoExitAfterGeneration = false;

    int m_SelectedSampleCountIdx = 1; // Default to 250k
    uint32_t m_ActiveSampleCount = 250000;
    
    float m_ParticleSize = 4.0f;
    float m_IntensityScale = 0.5f;
    float m_Exposure = 2.0f;
    float m_Gamma = 2.2f;

    bool m_PendingResample = false;
    
    // Performance instrumentation statistics
    float m_FrameTimeMs = 0.0f;
    float m_FPS = 0.0f;
    float m_FpsTimer = 0.0f;
    uint32_t m_FpsFrameCount = 0;
};

} // namespace Orbital
