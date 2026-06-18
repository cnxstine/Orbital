#pragma once

#include "visualization/VisualizationModule.hpp"
#include "visualization/MolecularOrbital.hpp"
#include "visualization/MolecularOrbitalType.hpp"
#include "visualization/MonteCarloSamplingPipeline.hpp"
#include "visualization/StochasticProbabilityCloud.hpp"
#include "visualization/EnergySweep.hpp"
#include "renderer/backend/GLFramebuffer.hpp"
#include "renderer/backend/GLVertexArray.hpp"
#include "resources/Handle.hpp"
#include "renderer/backend/GLShader.hpp"

#include <memory>

namespace Orbital {

class Engine;

class MolecularOrbitalExplorerModule : public VisualizationModule {
public:
    explicit MolecularOrbitalExplorerModule(Engine& engine);
    virtual ~MolecularOrbitalExplorerModule() override = default;

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
    void UpdateMOState();

    Engine& m_Engine;

    // Active LCAO simulation state
    std::shared_ptr<MolecularOrbital> m_ActiveWaveFunction;
    std::shared_ptr<MonteCarloSamplingPipeline> m_SamplingPipeline;
    std::shared_ptr<StochasticProbabilityCloud> m_ProbabilityCloud;

    // Rendering assets
    std::unique_ptr<GLFramebuffer> m_Framebuffer;
    std::unique_ptr<GLVertexArray> m_PointsVAO;
    std::unique_ptr<GLVertexArray> m_DummyVAO; // fullscreen quad resolve

    Handle<GLShader> m_PointDensityShader = Handle<GLShader>::Null();
    Handle<GLShader> m_ResolveShader = Handle<GLShader>::Null();

    // User LCAO parameters
    float m_Separation = 2.0f; // Bohr radii
    MolecularOrbitalType m_MOType = MolecularOrbitalType::Sigma1s;

    enum class ExposureMode { Manual, PerOrbital };
    ExposureMode m_ExposureMode = ExposureMode::PerOrbital;

    int m_SelectedSampleCountIdx = 1; // Default to 250k
    uint32_t m_ActiveSampleCount = 250000;

    float m_ParticleSize = 4.0f;
    float m_IntensityScale = 0.5f;
    float m_Exposure = 2.5f;
    float m_Contrast = 1.0f;
    float m_Gamma = 2.2f;

    bool m_PendingResample = false;
    bool m_GenerateVerificationPackage = false;
    bool m_AutoExitAfterGeneration = false;

    // Performance instrumentation statistics
    float m_FrameTimeMs = 0.0f;
    float m_FPS = 0.0f;
    float m_FpsTimer = 0.0f;
    uint32_t m_FpsFrameCount = 0;

    // Sweep results cache
    bool m_HasSweepResults = false;
    float m_SweepEquilibriumSep = 0.0f;
    double m_SweepMinBondingEnergy = 0.0;
    std::string m_SweepExportPath = "";

    // Cached sweep data for the Energy Curve Explorer graph
    SweepResult m_CachedSweepResult;
};

} // namespace Orbital
