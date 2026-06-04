#pragma once

/**
 * @file visualization/DensityField.hpp
 * @brief Volumetric scalar field abstraction (electron charge density, potential fields).
 */

#include "visualization/DataSet.hpp"
#include "resources/Handle.hpp"
#include <glm/glm.hpp>

namespace Orbital {

class GLTexture;

/**
 * @brief DensityField represents a general 3D scalar field sampled on a regular grid.
 */
class DensityField : public DataSet {
public:
    virtual ~DensityField() = default;

    /// @brief Get the discrete dimensions of the voxel grid (e.g. 128x128x128).
    [[nodiscard]] virtual glm::ivec3 GetResolution() const noexcept = 0;

    /// @brief Get the physical size of the bounding box (in Ångströms).
    [[nodiscard]] virtual glm::vec3 GetPhysicalDimensions() const noexcept = 0;

    /**
     * @brief Sample the field at a normalized coordinate [0, 1].
     */
    [[nodiscard]] virtual float SampleLocal(const glm::vec3& normalizedCoord) const = 0;

    /**
     * @brief Compute the spatial gradient (partial derivatives) at a coordinate.
     *        Used for volumetric surface lighting.
     */
    [[nodiscard]] virtual glm::vec3 ComputeGradient(const glm::vec3& normalizedCoord) const = 0;

    /**
     * @brief Retrieve the GPU texture handle mapping this 3D field (R32F format).
     *        Allows direct binding to ray-marching shaders.
     */
    [[nodiscard]] virtual Handle<GLTexture> GetTextureHandle() const noexcept = 0;
};

} // namespace Orbital
