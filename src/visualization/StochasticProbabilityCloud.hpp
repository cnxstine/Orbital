#pragma once

/**
 * @file visualization/StochasticProbabilityCloud.hpp
 * @brief Representation of a Monte Carlo sampled quantum probability cloud.
 */

#include "visualization/ProbabilityCloud.hpp"
#include "visualization/SamplePoint.hpp"
#include "renderer/backend/GLBuffer.hpp"
#include "resources/Handle.hpp"
#include "resources/ResourceManager.hpp"
#include "visualization/WaveFunction.hpp"
#include "visualization/SamplingPipeline.hpp"

#include <vector>
#include <memory>
#include <span>

namespace Orbital {

class StochasticProbabilityCloud : public ProbabilityCloud, public std::enable_shared_from_this<StochasticProbabilityCloud> {
public:
    StochasticProbabilityCloud(ResourceManager& resourceManager, 
                               std::shared_ptr<WaveFunction> waveFunction,
                               std::shared_ptr<SamplingPipeline> samplingPipeline);
    virtual ~StochasticProbabilityCloud() override = default;

    // ProbabilityCloud overrides
    [[nodiscard]] virtual std::shared_ptr<DensityField> GetDensityField() const noexcept override { return nullptr; }
    [[nodiscard]] virtual std::span<const SamplePoint> GetSampledPoints() const noexcept override { return m_Points; }
    [[nodiscard]] virtual Handle<GLBuffer> GetGPUPointBuffer() const noexcept override { return m_BufferHandle; }
    virtual void Resample(uint32_t numSamples) override;

    // Set points directly and perform GPU upload
    void SetPoints(std::vector<SamplePoint> points);

    // Set active wavefunction or sampling pipeline (for swapping)
    void SetWaveFunction(std::shared_ptr<WaveFunction> waveFunction) { m_WaveFunction = waveFunction; }
    void SetSamplingPipeline(std::shared_ptr<SamplingPipeline> pipeline) { m_SamplingPipeline = pipeline; }

    // Instrumentation metrics
    [[nodiscard]] double GetSampleGenTimeMs() const noexcept { return m_SampleGenTimeMs; }
    [[nodiscard]] double GetGpuUploadTimeMs() const noexcept { return m_GpuUploadTimeMs; }
    [[nodiscard]] uint32_t GetPointCount() const noexcept { return static_cast<uint32_t>(m_Points.size()); }

private:
    ResourceManager& m_ResourceManager;
    std::shared_ptr<WaveFunction> m_WaveFunction;
    std::shared_ptr<SamplingPipeline> m_SamplingPipeline;

    std::vector<SamplePoint> m_Points;
    Handle<GLBuffer> m_BufferHandle = Handle<GLBuffer>::Null();

    double m_SampleGenTimeMs = 0.0;
    double m_GpuUploadTimeMs = 0.0;
};

} // namespace Orbital
