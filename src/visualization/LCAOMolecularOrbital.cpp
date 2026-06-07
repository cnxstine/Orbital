#include "visualization/LCAOMolecularOrbital.hpp"

namespace Orbital {

LCAOMolecularOrbital::LCAOMolecularOrbital(MolecularOrbitalType type, float separation)
    : m_Type(type), m_Separation(separation)
{
    BondingWaveFunction::StateType stateType = BondingWaveFunction::StateType::Bonding;

    switch (m_Type) {
        case MolecularOrbitalType::Sigma2p:
            // 2p_x aligned along the X-axis bond direction (m=1)
            m_OrbitalA = std::make_shared<HydrogenicOrbital>(2, 1, 1, 0.5, true);
            m_OrbitalB = std::make_shared<HydrogenicOrbital>(2, 1, 1, 0.5, true);
            // Constructive overlap (bonding): lobes point in opposite directions at midpoint.
            // Under default coordinates, psiA is positive at midpoint and psiB is negative,
            // so we subtract (psiA - psiB) to get constructive interference.
            stateType = BondingWaveFunction::StateType::Antibonding;
            break;
        case MolecularOrbitalType::Sigma2pStar:
            m_OrbitalA = std::make_shared<HydrogenicOrbital>(2, 1, 1, 0.5, true);
            m_OrbitalB = std::make_shared<HydrogenicOrbital>(2, 1, 1, 0.5, true);
            // Destructive overlap (antibonding): we add (psiA + psiB) to get cancellation at midpoint.
            stateType = BondingWaveFunction::StateType::Bonding;
            break;
        case MolecularOrbitalType::Pi2pX:
            // 2p_z aligned along Z-axis, perpendicular to X-axis separation (m=0)
            m_OrbitalA = std::make_shared<HydrogenicOrbital>(2, 1, 0, 0.5, true);
            m_OrbitalB = std::make_shared<HydrogenicOrbital>(2, 1, 0, 0.5, true);
            // Constructive overlap: parallel lobes point in same direction, so we add.
            stateType = BondingWaveFunction::StateType::Bonding;
            break;
        case MolecularOrbitalType::Pi2pXStar:
            m_OrbitalA = std::make_shared<HydrogenicOrbital>(2, 1, 0, 0.5, true);
            m_OrbitalB = std::make_shared<HydrogenicOrbital>(2, 1, 0, 0.5, true);
            // Destructive overlap: we subtract.
            stateType = BondingWaveFunction::StateType::Antibonding;
            break;
        case MolecularOrbitalType::Pi2pY:
            // 2p_y aligned along Y-axis, perpendicular to X-axis separation (m=-1)
            m_OrbitalA = std::make_shared<HydrogenicOrbital>(2, 1, -1, 0.5, true);
            m_OrbitalB = std::make_shared<HydrogenicOrbital>(2, 1, -1, 0.5, true);
            // Constructive overlap: parallel lobes point in same direction, so we add.
            stateType = BondingWaveFunction::StateType::Bonding;
            break;
        case MolecularOrbitalType::Pi2pYStar:
            m_OrbitalA = std::make_shared<HydrogenicOrbital>(2, 1, -1, 0.5, true);
            m_OrbitalB = std::make_shared<HydrogenicOrbital>(2, 1, -1, 0.5, true);
            // Destructive overlap: we subtract.
            stateType = BondingWaveFunction::StateType::Antibonding;
            break;
        default:
            // Fallback for Sigma1s and Sigma1sStar (handled by original H2MolecularOrbital class)
            m_OrbitalA = std::make_shared<HydrogenicOrbital>(1, 0, 0, 0.5, false);
            m_OrbitalB = std::make_shared<HydrogenicOrbital>(1, 0, 0, 0.5, false);
            stateType = (m_Type == MolecularOrbitalType::Sigma1s) 
                ? BondingWaveFunction::StateType::Bonding 
                : BondingWaveFunction::StateType::Antibonding;
            break;
    }

    m_Underlying = std::make_unique<BondingWaveFunction>(m_OrbitalA, m_OrbitalB, m_Separation, stateType);
}

int LCAOMolecularOrbital::GetEnergyOrdering() const {
    switch (m_Type) {
        case MolecularOrbitalType::Sigma2p:     return 3;
        case MolecularOrbitalType::Pi2pX:       return 4;
        case MolecularOrbitalType::Pi2pY:       return 4;
        case MolecularOrbitalType::Pi2pXStar:   return 5;
        case MolecularOrbitalType::Pi2pYStar:   return 5;
        case MolecularOrbitalType::Sigma2pStar: return 6;
        default:                                return 1;
    }
}

std::string LCAOMolecularOrbital::GetLabel() const {
    switch (m_Type) {
        case MolecularOrbitalType::Sigma2p:     return "σ(2p)";
        case MolecularOrbitalType::Sigma2pStar: return "σ*(2p)";
        case MolecularOrbitalType::Pi2pX:       return "π(2p_x)";
        case MolecularOrbitalType::Pi2pXStar:   return "π*(2p_x)";
        case MolecularOrbitalType::Pi2pY:       return "π(2p_y)";
        case MolecularOrbitalType::Pi2pYStar:   return "π*(2p_y)";
        default:                                return "";
    }
}

std::string LCAOMolecularOrbital::GetDescription() const {
    switch (m_Type) {
        case MolecularOrbitalType::Sigma2p:
            return "Bonding σ(2p) molecular orbital formed by head-on (constructive) overlap of atomic 2p_x orbitals along the bond axis. Features a continuous electron density bridge.";
        case MolecularOrbitalType::Sigma2pStar:
            return "Antibonding σ*(2p) molecular orbital formed by head-on (destructive) overlap of atomic 2p_x orbitals. Features an internuclear nodal plane at the midpoint x = 0.";
        case MolecularOrbitalType::Pi2pX:
            return "Bonding π(2p_x) molecular orbital formed by side-by-side (constructive) overlap of atomic 2p_z orbitals. Electron density is concentrated above and below the bond axis (Z-axis direction).";
        case MolecularOrbitalType::Pi2pXStar:
            return "Antibonding π*(2p_x) molecular orbital formed by side-by-side (destructive) overlap of atomic 2p_z orbitals. Features both the horizontal nodal plane (z = 0) and an vertical nodal plane (x = 0).";
        case MolecularOrbitalType::Pi2pY:
            return "Bonding π(2p_y) molecular orbital formed by side-by-side (constructive) overlap of atomic 2p_y orbitals. Electron density is concentrated in front of and behind the bond axis (Y-axis direction).";
        case MolecularOrbitalType::Pi2pYStar:
            return "Antibonding π*(2p_y) molecular orbital formed by side-by-side (destructive) overlap of atomic 2p_y orbitals. Features both the horizontal nodal plane (y = 0) and an vertical nodal plane (x = 0).";
        default:
            return "";
    }
}

} // namespace Orbital
