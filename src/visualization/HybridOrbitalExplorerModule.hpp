#pragma once

#include "visualization/VisualizationModule.hpp"
#include "visualization/HybridOrbital.hpp"
#include "visualization/MonteCarloSamplingPipeline.hpp"
#include "visualization/StochasticProbabilityCloud.hpp"
#include "renderer/backend/GLFramebuffer.hpp"
#include "renderer/backend/GLVertexArray.hpp"
#include "renderer/backend/GLBuffer.hpp"
#include "resources/Handle.hpp"
#include "renderer/backend/GLShader.hpp"

#include <memory>
#include <vector>

namespace Orbital {

class Engine;

class HybridOrbitalExplorerModule : public VisualizationModule {
public:
    explicit HybridOrbitalExplorerModule(Engine& engine);
    virtual ~HybridOrbitalExplorerModule() override = default;

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
    void UpdateOrbitalState();
    void UpdateGuideBuffers();
    void GenerateScreenshots();

    Engine& m_Engine;

    // Active simulation state
    std::shared_ptr<HybridOrbital> m_ActiveWaveFunction;
    std::shared_ptr<MonteCarloSamplingPipeline> m_SamplingPipeline;
    std::shared_ptr<StochasticProbabilityCloud> m_ProbabilityCloud;

    // Rendering assets
    std::unique_ptr<GLFramebuffer> m_Framebuffer;
    std::unique_ptr<GLVertexArray> m_PointsVAO;
    std::unique_ptr<GLVertexArray> m_DummyVAO; // for fullscreen quad resolve

    // Guide geometry assets
    std::unique_ptr<GLVertexArray> m_GuideVAO;
    std::unique_ptr<GLBuffer> m_GuideVBO;
    uint32_t m_BondVertexCount = 0;
    uint32_t m_OutlineVertexCount = 0;

    Handle<GLShader> m_PointDensityShader = Handle<GLShader>::Null();
    Handle<GLShader> m_ResolveShader = Handle<GLShader>::Null();
    Handle<GLShader> m_LineShader = Handle<GLShader>::Null();

    // User parameters controlled via ImGui panel
    HybridOrbitalType m_SelectedType = HybridOrbitalType::sp;
    int m_ActiveOrbitalIdx = 0;

    bool m_ShowCloud = true;
    bool m_ShowGuides = true;

    bool m_GenerateVerificationPackage = false;
    bool m_AutoExitAfterGeneration = false;

    int m_SelectedSampleCountIdx = 1; // Default to 250k
    uint32_t m_ActiveSampleCount = 250000;

    float m_ParticleSize = 4.0f;
    float m_IntensityScale = 0.5f;
    float m_Exposure = 2.0f;
    float m_Contrast = 1.0f;
    float m_Gamma = 2.2f;

    bool m_PendingResample = false;

    // Performance instrumentation statistics
    float m_FrameTimeMs = 0.0f;
    float m_FPS = 0.0f;
    float m_FpsTimer = 0.0f;
    uint32_t m_FpsFrameCount = 0;
};

} // namespace Orbital
