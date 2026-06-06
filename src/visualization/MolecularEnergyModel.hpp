#pragma once

namespace Orbital {

struct EnergyResult {
    double bondingEnergy;      // E+ in eV
    double antibondingEnergy;  // E- in eV
    double HAA;                // H_AA in eV
    double HAB;                // H_AB in eV
};

class MolecularEnergyModel {
public:
    /**
     * @brief Compute the molecular orbital energies for H2+ / H2 LCAO model.
     * E+ = (HAA + HAB) / (1 + S)
     * E- = (HAA - HAB) / (1 - S)
     */
    static EnergyResult ComputeEnergies(float separation, double overlapS);
};

} // namespace Orbital
