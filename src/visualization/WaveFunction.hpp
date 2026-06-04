#pragma once

/**
 * @file visualization/WaveFunction.hpp
 * @brief Base mathematical interface representing a quantum state wavefunction.
 */

#include "visualization/DataSet.hpp"
#include <glm/glm.hpp>
#include <complex>

namespace Orbital {

/**
 * @brief Base mathematical interface representing a quantum state wavefunction.
 *        Inherited by hydrogenic states, molecular LCAO states, and crystal Bloch states.
 */
class WaveFunction : public DataSet {
public:
    virtual ~WaveFunction() = default;

    /**
     * @brief Evaluate the wavefunction amplitude psi(r) at a coordinate in space.
     * @param position Coordinate in spatial coordinates (physical units).
     * @return Complex amplitude: Re = real part, Im = imaginary part.
     */
    [[nodiscard]] virtual std::complex<double> Evaluate(const glm::vec3& position) const = 0;

    /**
     * @brief Evaluate the probability density |psi(r)|^2 at a coordinate.
     * @param position Coordinate in spatial coordinates (physical units).
     */
    [[nodiscard]] virtual double ProbabilityDensity(const glm::vec3& position) const = 0;
};

} // namespace Orbital
