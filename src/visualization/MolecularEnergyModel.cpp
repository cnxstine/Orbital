#include "visualization/MolecularEnergyModel.hpp"
#include <cmath>

namespace Orbital {

EnergyResult MolecularEnergyModel::ComputeEnergies(float separation, double overlapS) {
    // Exact LCAO model for H2+
    // E1s = -13.605693 eV (or -0.5 Hartree)
    const double E1s = -13.605693;
    const double HartreeToEV = 27.211386;

    double R = static_cast<double>(separation);
    if (R < 0.1) {
        R = 0.1; // Prevent division by zero
    }

    // J' = 1/R - (1 + 1/R) * e^(-2R) (Hartree)
    double J_prime = 1.0 / R - (1.0 + 1.0 / R) * std::exp(-2.0 * R);

    // Kc' = (1 + R) * e^(-R) (Hartree)
    double Kc_prime = (1.0 + R) * std::exp(-R);

    // Define HAA and HAB in eV to match:
    // E+ = (HAA + HAB) / (1 + S)
    // E- = (HAA - HAB) / (1 - S)
    // where E+ and E- include both the electronic energies and the nuclear-nuclear repulsion 1/R.
    //
    // E+_total = E1s + 1/R - (J' + Kc') / (1 + S)
    //          = ( (E1s + 1/R)(1 + S) - J' - Kc' ) / (1 + S)
    //          = ( [E1s + 1/R - J'] + [(E1s + 1/R)*S - Kc'] ) / (1 + S)
    // Therefore:
    // HAA = E1s + 1/R - J'
    // HAB = (E1s + 1/R)*S - Kc'
    double HAA_hartree = E1s / HartreeToEV + 1.0 / R - J_prime;
    double HAB_hartree = (E1s / HartreeToEV + 1.0 / R) * overlapS - Kc_prime;

    EnergyResult result;
    result.HAA = HAA_hartree * HartreeToEV;
    result.HAB = HAB_hartree * HartreeToEV;

    // Apply denominator bounds to prevent singularity near S = 1
    double denBonding = 1.0 + overlapS;
    double denAntibonding = 1.0 - overlapS;
    if (denAntibonding < 1e-6) {
        denAntibonding = 1e-6;
    }

    result.bondingEnergy = (result.HAA + result.HAB) / denBonding;
    result.antibondingEnergy = (result.HAA - result.HAB) / denAntibonding;

    return result;
}

} // namespace Orbital
