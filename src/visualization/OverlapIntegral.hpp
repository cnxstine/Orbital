#pragma once

#include "visualization/HydrogenicOrbital.hpp"

namespace Orbital {

class OverlapIntegral {
public:
    /**
     * @brief Compute the overlap integral S numerically using a 3D Cartesian grid.
     * S = ∫ ψA*(r) ψB(r) dV
     */
    static double ComputeNumerical(
        const HydrogenicOrbital& orbitalA,
        const HydrogenicOrbital& orbitalB,
        float separation,
        int gridPointsPerDim = 60
    );

    /**
     * @brief Compute the analytical overlap integral for two 1s hydrogen orbitals.
     * S(R) = (1 + R + R^2/3) * e^-R
     */
    static double ComputeAnalytical1s(float separation);
};

} // namespace Orbital
