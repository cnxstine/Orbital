#pragma once

/**
 * @file visualization/SamplePoint.hpp
 * @brief Representation of a single sampled point in a quantum probability cloud.
 */

#include <glm/glm.hpp>

namespace Orbital {

struct SamplePoint {
    glm::vec3 Position; // 3D position in physical space (atomic units / Bohr radii)
    float Density;      // Probability density |psi|^2 at this point
    float Phase;        // Wavefunction phase in radians [-pi, pi]
};

} // namespace Orbital
