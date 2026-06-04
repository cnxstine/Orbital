#pragma once

/**
 * @file visualization/ScientificRenderer.hpp
 * @brief Subsystem extending the base Renderer for scientific visualization primitives.
 */

#include "renderer/Renderer.hpp"
#include "visualization/VisualizationComponents.hpp"

namespace Orbital {

class ScientificRenderer {
public:
    explicit ScientificRenderer(Renderer& baseRenderer);
    ~ScientificRenderer() = default;

    /**
     * @brief Submit a volumetric ray-marching draw command.
     */
    void SubmitVolume(const VolumeComponent& volume, const glm::mat4& transform);

    /**
     * @brief Submit a stochastic point cloud particle drawing pass.
     */
    void SubmitStochasticCloud(const StochasticCloudComponent& cloud);

    /**
     * @brief Submit vector glyph fields.
     */
    void SubmitGlyphs(const VectorFieldComponent& field);

    /**
     * @brief Submit a wireframe bounding/lattice cell box.
     */
    void SubmitLatticeBox(const glm::vec3& minBounds, const glm::vec3& maxBounds, const glm::vec4& color);

private:
    Renderer& m_BaseRenderer;
};

} // namespace Orbital
