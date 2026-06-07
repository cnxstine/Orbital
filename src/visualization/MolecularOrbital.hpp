#pragma once

#include "visualization/WaveFunction.hpp"
#include <string>

namespace Orbital {

/**
 * @brief Abstract interface representing a molecular orbital.
 *        Extends WaveFunction to add chemical/physical properties.
 */
class MolecularOrbital : public WaveFunction {
public:
    virtual ~MolecularOrbital() override = default;

    /**
     * @brief Get the energy ordering (lower is more stable, e.g., 1 for bonding, 2 for antibonding).
     */
    [[nodiscard]] virtual int GetEnergyOrdering() const = 0;

    /**
     * @brief Get the label (e.g., "σ(1s)").
     */
    [[nodiscard]] virtual std::string GetLabel() const = 0;

    /**
     * @brief Get the description.
     */
    [[nodiscard]] virtual std::string GetDescription() const = 0;

    /**
     * @brief Get the maximum probability density of this molecular orbital.
     */
    [[nodiscard]] virtual double GetMaxDensity() const = 0;
};

} // namespace Orbital
