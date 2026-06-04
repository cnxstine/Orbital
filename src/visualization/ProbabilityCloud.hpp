#pragma once

/**
 * @file visualization/ProbabilityCloud.hpp
 * @brief Abstract representation of a sampled quantum probability cloud.
 */

#include "resources/Handle.hpp"
#include <memory>
#include <span>

namespace Orbital {

class DensityField;
class GLBuffer;
struct SamplePoint;

class ProbabilityCloud {
public:
    virtual ~ProbabilityCloud() = default;

    /**
     * @brief Retrieve the continuous volumetric representation of the cloud (if available).
     */
    [[nodiscard]] virtual std::shared_ptr<DensityField> GetDensityField() const noexcept = 0;

    /**
     * @brief Retrieve the Monte Carlo sampled point buffer on the CPU.
     */
    [[nodiscard]] virtual std::span<const SamplePoint> GetSampledPoints() const noexcept = 0;

    /**
     * @brief Return the handle to the GPU vertex buffer containing the points.
     */
    [[nodiscard]] virtual Handle<GLBuffer> GetGPUPointBuffer() const noexcept = 0;
    
    /**
     * @brief Force resampling of the points using the active sampling pipeline.
     */
    virtual void Resample(uint32_t numSamples) = 0;
};

} // namespace Orbital
