#include "visualization/HydrogenicOrbital.hpp"
#include "core/Assert.hpp"
#include <cmath>
#include <numbers>

namespace Orbital {

HydrogenicOrbital::HydrogenicOrbital(int n, int l, int m, double spin, bool isRealCombo)
    : m_IsRealCombo(isRealCombo)
{
    ORB_ASSERT(n >= 1, "Principal quantum number n must be >= 1");
    ORB_ASSERT(l >= 0 && l < n, "Azimuthal quantum number l must satisfy 0 <= l < n");
    ORB_ASSERT(m >= -l && m <= l, "Magnetic quantum number m must satisfy -l <= m <= l");

    m_QN.n = n;
    m_QN.l = l;
    m_QN.m = m;
    m_QN.Spin = spin;

    // Hydrogen energy levels: E_n = -13.605693 eV / n^2
    m_Energy = -13.605693 / static_cast<double>(n * n);

    m_Metadata.Name = "Hydrogen Orbital";
    
    std::string orbitalName = "";
    if (n == 1 && l == 0 && m == 0) orbitalName = "1s";
    else if (n == 2 && l == 0 && m == 0) orbitalName = "2s";
    else if (n == 2 && l == 1) {
        if (isRealCombo) {
            if (m == 1) orbitalName = "2p_x";
            else if (m == -1) orbitalName = "2p_y";
            else if (m == 0) orbitalName = "2p_z";
        } else {
            if (m == 0) orbitalName = "2p_z (complex)";
            else if (m == 1) orbitalName = "2p_+1 (complex)";
            else if (m == -1) orbitalName = "2p_-1 (complex)";
        }
    } else {
        orbitalName = std::to_string(n) + (l == 0 ? "s" : l == 1 ? "p" : l == 2 ? "d" : "f");
    }

    m_Metadata.Description = "Hydrogenic atomic wavefunction " + orbitalName +
                             " (n=" + std::to_string(n) +
                             ", l=" + std::to_string(l) + 
                             ", m=" + std::to_string(m) + 
                             (isRealCombo ? ", real combo)" : ", spherical harmonic)");
    m_Metadata.Type = DataSetType::Continuous;
    m_Metadata.ScientificConstants["n"] = n;
    m_Metadata.ScientificConstants["l"] = l;
    m_Metadata.ScientificConstants["m"] = m;
    m_Metadata.ScientificConstants["energy_ev"] = m_Energy;
}

std::complex<double> HydrogenicOrbital::Evaluate(const glm::vec3& position) const
{
    const double x = static_cast<double>(position.x);
    const double y = static_cast<double>(position.y);
    const double z = static_cast<double>(position.z);

    const double r = std::sqrt(x * x + y * y + z * z);
    const double theta = (r > 0.0) ? std::acos(z / r) : 0.0;
    const double phi = std::atan2(y, x);

    // Compute wavefunction based on QNs (supporting n <= 2 for 1s, 2s, 2p validation)
    const double pi = std::numbers::pi;

    if (m_QN.n == 1) {
        if (m_QN.l == 0 && m_QN.m == 0) {
            // 1s: psi = 1/sqrt(pi) * e^-r
            return (1.0 / std::sqrt(pi)) * std::exp(-r);
        }
    } else if (m_QN.n == 2) {
        if (m_QN.l == 0 && m_QN.m == 0) {
            // 2s: psi = 1/(4*sqrt(2*pi)) * (2 - r) * e^(-r/2)
            return (1.0 / (4.0 * std::sqrt(2.0 * pi))) * (2.0 - r) * std::exp(-r / 2.0);
        } else if (m_QN.l == 1) {
            if (m_IsRealCombo) {
                // Real combinations of 2p orbitals
                const double radialPart = (1.0 / (4.0 * std::sqrt(2.0 * pi))) * std::exp(-r / 2.0);
                if (m_QN.m == 1) {
                    // 2p_x = radialPart * x
                    return radialPart * x;
                } else if (m_QN.m == -1) {
                    // 2p_y = radialPart * y
                    return radialPart * y;
                } else if (m_QN.m == 0) {
                    // 2p_z = radialPart * z
                    return radialPart * z;
                }
            } else {
                // Complex spherical harmonics
                if (m_QN.m == 0) {
                    // 2p_z: psi = 1/(4*sqrt(2*pi)) * r * cos(theta) * e^(-r/2)
                    return (1.0 / (4.0 * std::sqrt(2.0 * pi))) * r * std::cos(theta) * std::exp(-r / 2.0);
                } else if (m_QN.m == 1) {
                    // 2p_+1: psi = -1/(8*sqrt(pi)) * r * sin(theta) * e^(i*phi) * e^(-r/2)
                    const double amplitude = -1.0 / (8.0 * std::sqrt(pi)) * r * std::sin(theta) * std::exp(-r / 2.0);
                    return std::complex<double>(amplitude * std::cos(phi), amplitude * std::sin(phi));
                } else if (m_QN.m == -1) {
                    // 2p_-1: psi = 1/(8*sqrt(pi)) * r * sin(theta) * e^(-i*phi) * e^(-r/2)
                    const double amplitude = 1.0 / (8.0 * std::sqrt(pi)) * r * std::sin(theta) * std::exp(-r / 2.0);
                    return std::complex<double>(amplitude * std::cos(-phi), amplitude * std::sin(-phi));
                }
            }
        }
    }

    // Default fallback for unhandled states (return 0)
    return 0.0;
}

double HydrogenicOrbital::ProbabilityDensity(const glm::vec3& position) const
{
    return std::norm(Evaluate(position));
}

} // namespace Orbital
