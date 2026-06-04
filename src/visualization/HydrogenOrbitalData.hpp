#pragma once

/**
 * @file visualization/HydrogenOrbitalData.hpp
 * @brief Hydrogen-like single-electron atomic orbital wavefunction descriptor.
 */

#include "visualization/WaveFunction.hpp"

namespace Orbital {

/**
 * @brief Hydrogen-like single-electron atomic orbital wavefunction.
 */
class HydrogenOrbitalData : public WaveFunction {
public:
    struct QuantumNumbers {
        int n; // Principal quantum number
        int l; // Azimuthal quantum number
        int m; // Magnetic quantum number
        double Spin; // Spin (+0.5 or -0.5)
    };

    virtual ~HydrogenOrbitalData() = default;

    /**
     * @brief Retrieve quantum numbers defining this hydrogenic state.
     */
    [[nodiscard]] virtual const QuantumNumbers& GetQuantumNumbers() const noexcept = 0;

    /**
     * @brief Retrieve the computed analytic energy of this state (in ElectronVolts).
     */
    [[nodiscard]] virtual double GetEnergy() const noexcept = 0;
};

} // namespace Orbital
