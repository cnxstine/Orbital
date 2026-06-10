#include "visualization/HybridOrbital.hpp"
#include "core/Assert.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace Orbital {

HybridOrbital::HybridOrbital(HybridOrbitalType type, int orbitalIndex)
    : m_Type(type), m_OrbitalIndex(orbitalIndex)
{
    // Pre-allocate constituent orbitals
    m_2s  = std::make_shared<HydrogenicOrbital>(2, 0, 0, 0.5, false);
    m_2px = std::make_shared<HydrogenicOrbital>(2, 1, 1, 0.5, true);
    m_2py = std::make_shared<HydrogenicOrbital>(2, 1, -1, 0.5, true);
    m_2pz = std::make_shared<HydrogenicOrbital>(2, 1, 0, 0.5, true);

    SetupCoefficients();

    // Setup metadata
    std::string typeStr = "";
    std::string geomStr = "";
    std::string desc = "";

    if (m_Type == HybridOrbitalType::sp) {
        ORB_ASSERT(orbitalIndex >= 0 && orbitalIndex < 2, "sp orbital index must be 0 or 1");
        typeStr = "sp";
        geomStr = "Linear (180°)";
        desc = "sp hybridized orbital " + std::to_string(orbitalIndex + 1) + 
               " formed by mixing 2s and 2pz orbitals. Directional lobe points along the " + 
               (orbitalIndex == 0 ? "+Z" : "-Z") + " axis.";
    } else if (m_Type == HybridOrbitalType::sp2) {
        ORB_ASSERT(orbitalIndex >= 0 && orbitalIndex < 3, "sp2 orbital index must be 0, 1, or 2");
        typeStr = "sp2";
        geomStr = "Trigonal Planar (120°)";
        desc = "sp² hybridized orbital " + std::to_string(orbitalIndex + 1) + 
               " formed by mixing 2s, 2px, and 2py orbitals in the XY plane. Directional lobe points at " + 
               (orbitalIndex == 0 ? "0°" : orbitalIndex == 1 ? "120°" : "240°") + " in the XY plane.";
    } else if (m_Type == HybridOrbitalType::sp3) {
        ORB_ASSERT(orbitalIndex >= 0 && orbitalIndex < 4, "sp3 orbital index must be between 0 and 3");
        typeStr = "sp3";
        geomStr = "Tetrahedral (109.47°)";
        desc = "sp³ hybridized orbital " + std::to_string(orbitalIndex + 1) + 
               " formed by mixing 2s, 2px, 2py, and 2pz orbitals. Directional lobe points towards a tetrahedral vertex: " + 
               (orbitalIndex == 0 ? "(1, 1, 1)" : orbitalIndex == 1 ? "(1, -1, -1)" : orbitalIndex == 2 ? "(-1, 1, -1)" : "(-1, -1, 1)");
    }

    m_Metadata.Name = typeStr + " Hybrid Orbital " + std::to_string(orbitalIndex + 1);
    m_Metadata.Description = desc + " Part of " + geomStr + " geometry.";
    m_Metadata.Type = DataSetType::Continuous;

    m_Metadata.ScientificConstants["hybridization_type"] = static_cast<double>(m_Type);
    m_Metadata.ScientificConstants["orbital_index"]      = orbitalIndex;
    m_Metadata.ScientificConstants["energy_ev"]          = -3.40142325; // n=2 energy level (-13.6/4)

    CalculateMaxDensity();
}

void HybridOrbital::SetupCoefficients()
{
    if (m_Type == HybridOrbitalType::sp) {
        // Linear combinations of 2s and 2pz
        double invSqrt2 = 1.0 / std::sqrt(2.0);
        m_c2s  = invSqrt2;
        m_c2px = 0.0;
        m_c2py = 0.0;
        m_c2pz = (m_OrbitalIndex == 0) ? invSqrt2 : -invSqrt2;
    } else if (m_Type == HybridOrbitalType::sp2) {
        // Trigonal planar combinations of 2s, 2px, 2py in XY plane
        double invSqrt3 = 1.0 / std::sqrt(3.0);
        double sqrt2_3  = std::sqrt(2.0 / 3.0);
        double invSqrt6 = 1.0 / std::sqrt(6.0);
        double invSqrt2 = 1.0 / std::sqrt(2.0);

        if (m_OrbitalIndex == 0) {
            m_c2s  = invSqrt3;
            m_c2px = sqrt2_3;
            m_c2py = 0.0;
            m_c2pz = 0.0;
        } else if (m_OrbitalIndex == 1) {
            m_c2s  = invSqrt3;
            m_c2px = -invSqrt6;
            m_c2py = invSqrt2;
            m_c2pz = 0.0;
        } else if (m_OrbitalIndex == 2) {
            m_c2s  = invSqrt3;
            m_c2px = -invSqrt6;
            m_c2py = -invSqrt2;
            m_c2pz = 0.0;
        }
    } else if (m_Type == HybridOrbitalType::sp3) {
        // Tetrahedral combinations of 2s, 2px, 2py, 2pz
        m_c2s = 0.5;
        if (m_OrbitalIndex == 0) {
            m_c2px = 0.5;  m_c2py = 0.5;  m_c2pz = 0.5;
        } else if (m_OrbitalIndex == 1) {
            m_c2px = 0.5;  m_c2py = -0.5; m_c2pz = -0.5;
        } else if (m_OrbitalIndex == 2) {
            m_c2px = -0.5; m_c2py = 0.5;  m_c2pz = -0.5;
        } else if (m_OrbitalIndex == 3) {
            m_c2px = -0.5; m_c2py = -0.5; m_c2pz = 0.5;
        }
    }
}

std::complex<double> HybridOrbital::Evaluate(const glm::vec3& position) const
{
    std::complex<double> val = 0.0;
    val += m_c2s * m_2s->Evaluate(position);
    val += m_c2px * m_2px->Evaluate(position);
    val += m_c2py * m_2py->Evaluate(position);
    val += m_c2pz * m_2pz->Evaluate(position);
    return val;
}

double HybridOrbital::ProbabilityDensity(const glm::vec3& position) const
{
    return std::norm(Evaluate(position));
}

void HybridOrbital::CalculateMaxDensity()
{
    double maxD = 0.0;
    // Perform a coarse radial-angular grid search to find the peak probability density of this hybrid orbital.
    // Radial range: 0.0 to 12.0 Bohr.
    // Angular range: theta [0, pi], phi [0, 2pi].
    for (double r = 0.0; r <= 12.0; r += 0.1) {
        for (double theta = 0.0; theta <= std::numbers::pi; theta += 0.08) {
            for (double phi = 0.0; phi <= 2.0 * std::numbers::pi; phi += 0.16) {
                float x = static_cast<float>(r * std::sin(theta) * std::cos(phi));
                float y = static_cast<float>(r * std::sin(theta) * std::sin(phi));
                float z = static_cast<float>(r * std::cos(theta));
                double density = ProbabilityDensity({x, y, z});
                if (density > maxD) {
                    maxD = density;
                }
            }
        }
    }
    m_MaxDensity = maxD;
}

} // namespace Orbital
