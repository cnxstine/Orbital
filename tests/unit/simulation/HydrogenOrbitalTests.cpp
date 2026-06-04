#include <gtest/gtest.h>
#include "visualization/HydrogenicOrbital.hpp"
#include <cmath>
#include <numbers>

using namespace Orbital;

// Tolerance for floating point comparisons
constexpr double EPSILON = 1e-7;

TEST(HydrogenOrbitalTests, Validate1sOrbital)
{
    // Hydrogen 1s orbital (n=1, l=0, m=0)
    HydrogenicOrbital orbital(1, 0, 0);

    // 1. Energy verification: E_1 = -13.605693 eV
    EXPECT_NEAR(orbital.GetEnergy(), -13.605693, EPSILON);

    // 2. Wavefunction at nucleus (r = 0): psi(0) = 1/sqrt(pi)
    const double expectedAtNucleus = 1.0 / std::sqrt(std::numbers::pi);
    auto valAtOrigin = orbital.Evaluate({0.0f, 0.0f, 0.0f});
    EXPECT_NEAR(valAtOrigin.real(), expectedAtNucleus, EPSILON);
    EXPECT_NEAR(valAtOrigin.imag(), 0.0, EPSILON);

    // 3. Spherical symmetry checks (r = 1 Bohr radius along different axes)
    const float r = 1.0f;
    auto valX = orbital.Evaluate({r, 0.0f, 0.0f});
    auto valY = orbital.Evaluate({0.0f, r, 0.0f});
    auto valZ = orbital.Evaluate({0.0f, 0.0f, r});
    auto valDiag = orbital.Evaluate({r / std::sqrt(3.0f), r / std::sqrt(3.0f), r / std::sqrt(3.0f)});

    // All should be equal to 1/sqrt(pi) * e^-1
    const double expectedAtR1 = expectedAtNucleus * std::exp(-1.0);
    EXPECT_NEAR(valX.real(), expectedAtR1, EPSILON);
    EXPECT_NEAR(valY.real(), expectedAtR1, EPSILON);
    EXPECT_NEAR(valZ.real(), expectedAtR1, EPSILON);
    EXPECT_NEAR(valDiag.real(), expectedAtR1, EPSILON);

    // Imaginary parts must be 0 for m=0
    EXPECT_NEAR(valX.imag(), 0.0, EPSILON);
    EXPECT_NEAR(valY.imag(), 0.0, EPSILON);
    EXPECT_NEAR(valZ.imag(), 0.0, EPSILON);
    EXPECT_NEAR(valDiag.imag(), 0.0, EPSILON);

    // 4. Probability density checks
    EXPECT_NEAR(orbital.ProbabilityDensity({r, 0.0f, 0.0f}), expectedAtR1 * expectedAtR1, EPSILON);
}

TEST(HydrogenOrbitalTests, Validate2sOrbital)
{
    // Hydrogen 2s orbital (n=2, l=0, m=0)
    HydrogenicOrbital orbital(2, 0, 0);

    // 1. Energy verification: E_2 = -13.605693 / 4 = -3.40142325 eV
    EXPECT_NEAR(orbital.GetEnergy(), -3.40142325, EPSILON);

    // 2. Wavefunction at nucleus (r = 0): psi(0) = 1/(4*sqrt(2*pi)) * 2 = 1/(2*sqrt(2*pi))
    const double expectedAtNucleus = 1.0 / (2.0 * std::sqrt(2.0 * std::numbers::pi));
    auto valAtOrigin = orbital.Evaluate({0.0f, 0.0f, 0.0f});
    EXPECT_NEAR(valAtOrigin.real(), expectedAtNucleus, EPSILON);
    EXPECT_NEAR(valAtOrigin.imag(), 0.0, EPSILON);

    // 3. Radial Node: psi(r=2) must be 0 (changes sign at r=2 Bohr radii)
    auto nodeX = orbital.Evaluate({2.0f, 0.0f, 0.0f});
    auto nodeY = orbital.Evaluate({0.0f, 2.0f, 0.0f});
    auto nodeZ = orbital.Evaluate({0.0f, 0.0f, 2.0f});

    EXPECT_NEAR(nodeX.real(), 0.0, EPSILON);
    EXPECT_NEAR(nodeY.real(), 0.0, EPSILON);
    EXPECT_NEAR(nodeZ.real(), 0.0, EPSILON);

    // 4. Sign inversion check
    auto insideNode = orbital.Evaluate({1.0f, 0.0f, 0.0f});
    auto outsideNode = orbital.Evaluate({3.0f, 0.0f, 0.0f});

    EXPECT_GT(insideNode.real(), 0.0);
    EXPECT_LT(outsideNode.real(), 0.0);
}

TEST(HydrogenOrbitalTests, Validate2pOrbital)
{
    // Hydrogen 2p_z orbital (n=2, l=1, m=0)
    HydrogenicOrbital orbital2pz(2, 1, 0);

    // 1. Angular node check: xy-plane (z = 0) must have zero amplitude
    auto nodeXY = orbital2pz.Evaluate({1.0f, 1.0f, 0.0f});
    EXPECT_NEAR(nodeXY.real(), 0.0, EPSILON);
    EXPECT_NEAR(nodeXY.imag(), 0.0, EPSILON);

    // 2. Asymmetry (dumbbell peaks along z-axis)
    auto positiveZ = orbital2pz.Evaluate({0.0f, 0.0f, 1.0f});
    auto negativeZ = orbital2pz.Evaluate({0.0f, 0.0f, -1.0f});

    EXPECT_GT(positiveZ.real(), 0.0);
    EXPECT_LT(negativeZ.real(), 0.0);
    EXPECT_NEAR(std::abs(positiveZ.real()), std::abs(negativeZ.real()), EPSILON);

    // Hydrogen 2p_+1 orbital (n=2, l=1, m=1)
    HydrogenicOrbital orbital2pPlus1(2, 1, 1);

    // 3. Node along z-axis (theta = 0 -> sin(theta) = 0)
    auto nodeZAxis = orbital2pPlus1.Evaluate({0.0f, 0.0f, 2.0f});
    EXPECT_NEAR(std::norm(nodeZAxis), 0.0, EPSILON);

    // 4. Phase check: rotating in xy-plane (phi = 0 vs. phi = pi/2)
    auto posPhi0 = orbital2pPlus1.Evaluate({1.0f, 0.0f, 0.0f}); // phi = 0 -> cos(0) + i sin(0) = 1 (real part only)
    auto posPhiHalfPi = orbital2pPlus1.Evaluate({0.0f, 1.0f, 0.0f}); // phi = pi/2 -> cos(pi/2) + i sin(pi/2) = i (imag part only)

    EXPECT_NEAR(posPhi0.imag(), 0.0, EPSILON);
    EXPECT_LT(posPhi0.real(), 0.0); // Note the negative sign in the spherical harmonic Y_1^1 coefficient

    EXPECT_NEAR(posPhiHalfPi.real(), 0.0, EPSILON);
    EXPECT_LT(posPhiHalfPi.imag(), 0.0); // i * negative coefficient -> negative imaginary part

    // Probability density must be identical at both points (rotationally symmetric magnitude)
    EXPECT_NEAR(orbital2pPlus1.ProbabilityDensity({1.0f, 0.0f, 0.0f}),
                orbital2pPlus1.ProbabilityDensity({0.0f, 1.0f, 0.0f}), EPSILON);
}

TEST(HydrogenOrbitalTests, VerifyNormalizationViaIntegration)
{
    // Helper function for 3D numerical integration of |psi|^2 over a grid
    auto integrateProbability = [](const WaveFunction& orbital, double limit, double step) -> double {
        double sum = 0.0;
        const double dV = step * step * step;
        
        for (double x = -limit; x <= limit; x += step) {
            for (double y = -limit; y <= limit; y += step) {
                for (double z = -limit; z <= limit; z += step) {
                    sum += orbital.ProbabilityDensity({static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)});
                }
            }
        }
        return sum * dV;
    };

    // 1. Validate 1s Normalization: Integrate up to 10 Bohr radii with step = 0.15
    HydrogenicOrbital orbital1s(1, 0, 0);
    double integral1s = integrateProbability(orbital1s, 10.0, 0.15);
    EXPECT_NEAR(integral1s, 1.0, 0.01); // within 1%

    // 2. Validate 2s Normalization: Integrate up to 20 Bohr radii with step = 0.25
    HydrogenicOrbital orbital2s(2, 0, 0);
    double integral2s = integrateProbability(orbital2s, 20.0, 0.25);
    EXPECT_NEAR(integral2s, 1.0, 0.015); // within 1.5%

    // 3. Validate 2p_z Normalization: Integrate up to 20 Bohr radii with step = 0.25
    HydrogenicOrbital orbital2pz(2, 1, 0);
    double integral2pz = integrateProbability(orbital2pz, 20.0, 0.25);
    EXPECT_NEAR(integral2pz, 1.0, 0.015); // within 1.5%

    // 4. Validate 2p_x Normalization (real): Integrate up to 20 Bohr radii with step = 0.25
    HydrogenicOrbital orbital2px(2, 1, 1, 0.5, true);
    double integral2px = integrateProbability(orbital2px, 20.0, 0.25);
    EXPECT_NEAR(integral2px, 1.0, 0.015); // within 1.5%

    // 5. Validate 2p_y Normalization (real): Integrate up to 20 Bohr radii with step = 0.25
    HydrogenicOrbital orbital2py(2, 1, -1, 0.5, true);
    double integral2py = integrateProbability(orbital2py, 20.0, 0.25);
    EXPECT_NEAR(integral2py, 1.0, 0.015); // within 1.5%
}

TEST(HydrogenOrbitalTests, Validate2pxOrbital)
{
    // Hydrogen 2p_x orbital (n=2, l=1, m=1, spin=0.5, isRealCombo=true)
    HydrogenicOrbital orbital2px(2, 1, 1, 0.5, true);

    // 1. Angular node check: yz-plane (x = 0) must have zero amplitude
    auto nodeYZ = orbital2px.Evaluate({0.0f, 1.0f, 1.0f});
    EXPECT_NEAR(nodeYZ.real(), 0.0, EPSILON);
    EXPECT_NEAR(nodeYZ.imag(), 0.0, EPSILON);

    // 2. Asymmetry (dumbbell peaks along x-axis)
    auto positiveX = orbital2px.Evaluate({1.0f, 0.0f, 0.0f});
    auto negativeX = orbital2px.Evaluate({-1.0f, 0.0f, 0.0f});

    EXPECT_GT(positiveX.real(), 0.0);
    EXPECT_LT(negativeX.real(), 0.0);
    EXPECT_NEAR(std::abs(positiveX.real()), std::abs(negativeX.real()), EPSILON);
}

TEST(HydrogenOrbitalTests, Validate2pyOrbital)
{
    // Hydrogen 2p_y orbital (n=2, l=1, m=-1, spin=0.5, isRealCombo=true)
    HydrogenicOrbital orbital2py(2, 1, -1, 0.5, true);

    // 1. Angular node check: xz-plane (y = 0) must have zero amplitude
    auto nodeXZ = orbital2py.Evaluate({1.0f, 0.0f, 1.0f});
    EXPECT_NEAR(nodeXZ.real(), 0.0, EPSILON);
    EXPECT_NEAR(nodeXZ.imag(), 0.0, EPSILON);

    // 2. Asymmetry (dumbbell peaks along y-axis)
    auto positiveY = orbital2py.Evaluate({0.0f, 1.0f, 0.0f});
    auto negativeY = orbital2py.Evaluate({0.0f, -1.0f, 0.0f});

    EXPECT_GT(positiveY.real(), 0.0);
    EXPECT_LT(negativeY.real(), 0.0);
    EXPECT_NEAR(std::abs(positiveY.real()), std::abs(negativeY.real()), EPSILON);
}

