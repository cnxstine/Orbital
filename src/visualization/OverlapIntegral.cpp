#include "visualization/OverlapIntegral.hpp"
#include <cmath>
#include <algorithm>

namespace Orbital {

double OverlapIntegral::ComputeNumerical(
    const HydrogenicOrbital& orbitalA,
    const HydrogenicOrbital& orbitalB,
    float separation,
    int gridPointsPerDim
) {
    if (separation < 1e-4f) {
        return 1.0; // Perfect overlap at zero separation
    }

    // Determine the bounding box to capture the exponential decay.
    // The wavefunctions decay as e^-r (or e^(-r/2) for n=2).
    // For n=1, a radius of 8.0 Bohr is sufficient.
    // For n=2, a radius of 16.0 Bohr is more appropriate.
    float waveRadius = 8.0f;
    
    // Safely retrieve the principal quantum number 'n' from scientific constants
    int nA = 1;
    int nB = 1;
    const auto& metaA = orbitalA.GetMetadata().ScientificConstants;
    const auto& metaB = orbitalB.GetMetadata().ScientificConstants;
    if (metaA.find("n") != metaA.end()) {
        nA = static_cast<int>(metaA.at("n"));
    }
    if (metaB.find("n") != metaB.end()) {
        nB = static_cast<int>(metaB.at("n"));
    }

    int maxN = std::max(nA, nB);
    if (maxN > 1) {
        waveRadius = 16.0f;
    }

    float xHalfSep = separation * 0.5f;
    float xMin = -xHalfSep - waveRadius;
    float xMax =  xHalfSep + waveRadius;
    float yMin = -waveRadius;
    float yMax =  waveRadius;
    float zMin = -waveRadius;
    float zMax =  waveRadius;

    double dx = (xMax - xMin) / (gridPointsPerDim - 1);
    double dy = (yMax - yMin) / (gridPointsPerDim - 1);
    double dz = (zMax - zMin) / (gridPointsPerDim - 1);
    double dV = dx * dy * dz;

    double overlapSum = 0.0;

    for (int i = 0; i < gridPointsPerDim; ++i) {
        float x = xMin + i * dx;
        for (int j = 0; j < gridPointsPerDim; ++j) {
            float y = yMin + j * dy;
            for (int k = 0; k < gridPointsPerDim; ++k) {
                float z = zMin + k * dz;
                
                glm::vec3 pos(x, y, z);
                glm::vec3 posA = pos - glm::vec3(-xHalfSep, 0.0f, 0.0f);
                glm::vec3 posB = pos - glm::vec3(xHalfSep, 0.0f, 0.0f);

                auto psiA = orbitalA.Evaluate(posA);
                auto psiB = orbitalB.Evaluate(posB);

                // Integrate real part of psi_A * conj(psi_B)
                overlapSum += (psiA.real() * psiB.real() + psiA.imag() * psiB.imag());
            }
        }
    }

    return overlapSum * dV;
}

double OverlapIntegral::ComputeAnalytical1s(float separation) {
    double R = static_cast<double>(separation);
    return (1.0 + R + (R * R) / 3.0) * std::exp(-R);
}

} // namespace Orbital
