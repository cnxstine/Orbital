#pragma once

/**
 * @file visualization/SamplingPipeline.hpp
 * @brief Pipeline interface to discretize mathematical wavefunctions into fields and clouds.
 */

#include "core/Error.hpp"
#include <memory>
#include <cstdint>

namespace Orbital {

class WaveFunction;
class DensityField;
class ProbabilityCloud;

enum class VoxelResolution {
    Low_128   = 128,
    Medium_192 = 192,
    High_256  = 256
};

class SamplingPipeline {
public:
    virtual ~SamplingPipeline() = default;

    /**
     * @brief Sample the wavefunction into a 3D grid texture.
     *        Supports GPU Compute Shader execution where possible.
     */
    virtual Result<void> SampleToField(const WaveFunction& source, 
                                       std::shared_ptr<DensityField> targetField) = 0;

    /**
     * @brief Perform Monte Carlo sampling to generate probability point clouds.
     */
    virtual Result<void> SampleToCloud(const WaveFunction& source, 
                                       std::shared_ptr<ProbabilityCloud> targetCloud, 
                                       uint32_t numSamples) = 0;

    /**
     * @brief Explicitly override and set voxel sampling resolution.
     */
    virtual void SetResolution(VoxelResolution resolution) = 0;

    /// @brief Retrieve active voxel sampling resolution.
    [[nodiscard]] virtual VoxelResolution GetResolution() const noexcept = 0;

    /**
     * @brief Hook to submit frame time stats for dynamic profiling.
     *        Allows future implementation of auto-downscaling to maintain 60 FPS.
     */
    virtual void RegisterFrameTime(float frameTimeMs) = 0;
};

} // namespace Orbital
