#pragma once

/**
 * @file visualization/MonteCarloSamplingPipeline.hpp
 * @brief Sampling implementation using direct analytical methods and Metropolis-Hastings MCMC.
 */

#include "visualization/SamplingPipeline.hpp"
#include "visualization/SamplePoint.hpp"
#include <random>
#include <vector>

namespace Orbital {

class MonteCarloSamplingPipeline : public SamplingPipeline {
public:
    MonteCarloSamplingPipeline();
    virtual ~MonteCarloSamplingPipeline() = default;

    // SamplingPipeline overrides
    virtual Result<void> SampleToField(const WaveFunction& source, 
                                       std::shared_ptr<DensityField> targetField) override;

    virtual Result<void> SampleToCloud(const WaveFunction& source, 
                                       std::shared_ptr<ProbabilityCloud> targetCloud, 
                                       uint32_t numSamples) override;

    virtual void SetResolution(VoxelResolution resolution) override;
    [[nodiscard]] virtual VoxelResolution GetResolution() const noexcept override;
    virtual void RegisterFrameTime(float frameTimeMs) override;

    /**
     * @brief Populate a vector with sampled points from the given WaveFunction.
     *        Chooses direct sampling for Hydrogen 1s, 2s, 2p and MCMC for general wavefunctions.
     */
    Result<void> SampleToVector(const WaveFunction& source, std::vector<SamplePoint>& outPoints, uint32_t numSamples);

private:
    Result<void> SampleDirect(const WaveFunction& source, std::vector<SamplePoint>& outPoints, uint32_t numSamples);
    Result<void> SampleMCMC(const WaveFunction& source, std::vector<SamplePoint>& outPoints, uint32_t numSamples);

    VoxelResolution m_Resolution = VoxelResolution::Low_128;
    float m_LastFrameTimeMs = 16.6f;
    
    mutable std::mt19937 m_Rng;
};

} // namespace Orbital
