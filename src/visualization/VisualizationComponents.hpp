#pragma once

/**
 * @file visualization/VisualizationComponents.hpp
 * @brief ECS component definitions for material and quantum visualizations.
 */

#include "resources/Handle.hpp"
#include <glm/glm.hpp>

namespace Orbital {

class GLTexture;
class GLBuffer;

/**
 * @brief Volumetric Ray-Marcher settings.
 *        Marks an entity as a volumetric density cloud (e.g. Orbitals, charge density).
 */
struct VolumeComponent {
    Handle<GLTexture> VolumeTexture;      // 3D Texture (R32F)
    Handle<GLTexture> TransferFunction;   // 1D Transfer Function (RGBA)
    float ValueScale          = 1.0f;     // Density scaling
    float SampleStepSize      = 0.005f;   // Ray step size
    bool SelfShading          = true;     // Evaluate local shadows
    glm::vec3 BoxExtent       = glm::vec3(1.0f); // Bounding box bounds
};

/**
 * @brief Isosurface visualizer settings.
 *        Renders a polygonized surface representing a constant value threshold.
 *        Marching Cubes is executed strictly on GPU Compute Shaders (OpenGL 4.6).
 *        No CPU fallback is supported.
 */
struct IsosurfaceComponent {
    Handle<GLTexture> VolumeTexture;
    float IsoValue            = 0.05f;    // Threshold boundary
    glm::vec4 SurfaceColor    = glm::vec4(0.2f, 0.7f, 1.0f, 0.8f);
    bool Wireframe            = false;
    bool Dirty                = true;     // Flags reconstruction via Compute Shaders
    
    // GPU-only buffer handles for zero-copy mesh generation
    Handle<GLBuffer> VertexBuffer;        // Append buffer populated by compute shader
    Handle<GLBuffer> IndirectDrawBuffer;  // Command argument buffer for glDrawArraysIndirect
};

/**
 * @brief Point cloud particle descriptor (for Stochastic Probability Clouds).
 */
struct StochasticCloudComponent {
    Handle<GLBuffer> PointBuffer;         // GPU buffer containing vec4 points (xyz = position, w = phase/color)
    uint32_t PointCount       = 0;
    float ParticleSize        = 2.0f;     // Pixel size
    bool UsePhaseColoring     = true;     // Maps wave phase to colors
};

/**
 * @brief Vector field glyph component.
 *        Marks an entity as a lattice vector or magnetic dipole field.
 */
struct VectorFieldComponent {
    Handle<GLBuffer> FieldBuffer;         // Structured VBO containing {Position, Vector}
    uint32_t FieldSize        = 0;
    float ArrowScale          = 0.2f;
    glm::vec4 ArrowColor      = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
};

/**
 * @brief Atomic visualizer descriptor.
 */
struct AtomComponent {
    uint32_t AtomicNumber     = 1;        // Periodic table index
    float AtomicRadius        = 1.2f;     // Visual sphere scaling
    glm::vec4 ElementColor    = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f); // CPK Coloring
};

/**
 * @brief Chemical bond descriptor.
 */
struct BondComponent {
    glm::vec3 StartPosition;
    glm::vec3 EndPosition;
    float Thickness           = 0.15f;
    glm::vec4 BondColor       = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
};

} // namespace Orbital
