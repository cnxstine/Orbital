#include <gtest/gtest.h>
#include "visualization/HydrogenicOrbital.hpp"
#include "visualization/BondingWaveFunction.hpp"
#include "visualization/OverlapIntegral.hpp"
#include "visualization/MolecularEnergyModel.hpp"
#include "visualization/EnergySweep.hpp"
#include "visualization/MolecularOrbitalFactory.hpp"
#include "visualization/MolecularOrbitalType.hpp"
#include "camera/CameraManager.hpp"
#include "camera/CameraController.hpp"
#include "events/events/InputEvents.hpp"
#include "core/Log.hpp"
#include <imgui.h>
#include <cmath>
#include <numbers>

using namespace Orbital;

class LogEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        Log::Init();
    }
};

::testing::Environment* const logEnv = ::testing::AddGlobalTestEnvironment(new LogEnvironment);

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

TEST(HydrogenOrbitalTests, ValidateLCAOBondingExplorer)
{
    // Create atomic 1s orbitals
    auto orbitalA = std::make_shared<HydrogenicOrbital>(1, 0, 0, 0.5, false);
    auto orbitalB = std::make_shared<HydrogenicOrbital>(1, 0, 0, 0.5, false);

    double separations[] = { 10.0, 6.0, 3.0, 1.5 };

    for (double separation : separations) {
        // 1. Bonding State Verification: ψ+ = ψA + ψB
        BondingWaveFunction psiPlus(orbitalA, orbitalB, static_cast<float>(separation), BondingWaveFunction::StateType::Bonding);

        // Midpoint at x = 0
        glm::vec3 midpoint(0.0f, 0.0f, 0.0f);
        glm::vec3 nucleusA(-separation * 0.5f, 0.0f, 0.0f);
        glm::vec3 nucleusB(separation * 0.5f, 0.0f, 0.0f);

        double denA_bonding = psiPlus.ProbabilityDensity(nucleusA);
        double denB_bonding = psiPlus.ProbabilityDensity(nucleusB);
        double denMid_bonding = psiPlus.ProbabilityDensity(midpoint);

        // Symmetry check
        EXPECT_NEAR(denA_bonding, denB_bonding, EPSILON);

        // Constructive interference check: midpoint density compared to isolated atoms
        // An isolated atom centered at A evaluated at midpoint:
        double denMid_isolatedA = std::norm(orbitalA->Evaluate(midpoint - nucleusA));
        // Theoretical density with zero overlap would be: 2 * denMid_isolatedA (for 2 independent probability densities)
        // With LCAO wavefunction, it is (psi_A + psi_B)^2 = 4 * psi_A(midpoint)^2 = 4 * denMid_isolatedA.
        // Therefore, denMid_bonding must be exactly 4 * denMid_isolatedA.
        EXPECT_NEAR(denMid_bonding, 4.0 * denMid_isolatedA, EPSILON);

        // Qualitative behavior as separation decreases:
        // Midpoint density should increase significantly.
        if (separation == 10.0) {
            EXPECT_LT(denMid_bonding, 1e-4);
        } else if (separation == 1.5) {
            EXPECT_GT(denMid_bonding, 0.1);
        }

        // 2. Antibonding State Verification: ψ- = ψA - ψB
        BondingWaveFunction psiMinus(orbitalA, orbitalB, static_cast<float>(separation), BondingWaveFunction::StateType::Antibonding);

        double denA_anti = psiMinus.ProbabilityDensity(nucleusA);
        double denB_anti = psiMinus.ProbabilityDensity(nucleusB);
        double denMid_anti = psiMinus.ProbabilityDensity(midpoint);

        // Midpoint density MUST be EXACTLY zero due to perfect destructive interference (nodal plane)
        EXPECT_NEAR(denMid_anti, 0.0, EPSILON);

        // Entire yz-plane at x = 0 must be a nodal plane. Let's test a few offset points on x = 0 plane.
        EXPECT_NEAR(psiMinus.ProbabilityDensity({0.0f, 1.0f, 0.0f}), 0.0, EPSILON);
        EXPECT_NEAR(psiMinus.ProbabilityDensity({0.0f, 0.0f, 2.0f}), 0.0, EPSILON);
        EXPECT_NEAR(psiMinus.ProbabilityDensity({0.0f, 1.5f, -1.5f}), 0.0, EPSILON);

        // Symmetry check for nuclei density in antibonding
        EXPECT_NEAR(denA_anti, denB_anti, EPSILON);

        // 3. Isolated core states verification
        BondingWaveFunction psiA_only(orbitalA, orbitalB, static_cast<float>(separation), BondingWaveFunction::StateType::OrbitalA);
        BondingWaveFunction psiB_only(orbitalA, orbitalB, static_cast<float>(separation), BondingWaveFunction::StateType::OrbitalB);

        // ψA only should have peak at nucleus A and decay towards nucleus B
        double peakA = psiA_only.ProbabilityDensity(nucleusA);
        double peakB_for_A = psiA_only.ProbabilityDensity(nucleusB);
        EXPECT_GT(peakA, 0.1);
        double expectedRatio = std::exp(2.0 * separation);
        EXPECT_NEAR(peakA / peakB_for_A, expectedRatio, expectedRatio * 1e-5);

        // ψB only should have peak at nucleus B and decay towards nucleus A
        double peakB = psiB_only.ProbabilityDensity(nucleusB);
        double peakA_for_B = psiB_only.ProbabilityDensity(nucleusA);
        EXPECT_GT(peakB, 0.1);
        EXPECT_NEAR(peakB / peakA_for_B, expectedRatio, expectedRatio * 1e-5);

        // Output results for the validation report
        std::cout << "[LCAO VALIDATION] Separation: " << separation << " Bohr" << std::endl;
        std::cout << "  - Isolated (psi_A) at A (nucleus): " << peakA << ", at B (remote): " << peakB_for_A << std::endl;
        std::cout << "  - Bonding (psi_A + psi_B) max: " << psiPlus.GetMaxDensity() << ", at midpoint (overlap): " << denMid_bonding << std::endl;
        std::cout << "  - Antibonding (psi_A - psi_B) max: " << psiMinus.GetMaxDensity() << ", at midpoint: " << denMid_anti << std::endl;
        std::cout << "  - Nodal plane at x=0 check: " << (denMid_anti < 1e-15 ? "Present (Density = 0.0)" : "Absent") << std::endl;
    }
}

TEST(HydrogenOrbitalTests, ValidateOverlapIntegral)
{
    // 1. Specific check: S_analytical(2.0) ≈ 0.586453
    double s_anal_2 = OverlapIntegral::ComputeAnalytical1s(2.0f);
    EXPECT_NEAR(s_anal_2, 0.586453, 1e-5);

    // 2. Overlap approaches zero at large separation
    double s_anal_10 = OverlapIntegral::ComputeAnalytical1s(10.0f);
    EXPECT_LT(s_anal_10, 3e-3);

    HydrogenicOrbital orbitalA(1, 0, 0, 0.5, false);
    HydrogenicOrbital orbitalB(1, 0, 0, 0.5, false);
    double s_num_10 = OverlapIntegral::ComputeNumerical(orbitalA, orbitalB, 10.0f, 60);
    EXPECT_LT(s_num_10, 3e-3);

    // 3. Numerical vs Analytical accuracy at R = 2.0 Bohr
    double s_num_2 = OverlapIntegral::ComputeNumerical(orbitalA, orbitalB, 2.0f, 60);
    EXPECT_NEAR(s_num_2, s_anal_2, 1e-3);
}

TEST(HydrogenOrbitalTests, ValidateMolecularEnergyModel)
{
    double separations[] = { 1.0, 1.5, 2.0, 3.0, 6.0, 10.0 };

    for (double sep : separations) {
        double S = OverlapIntegral::ComputeAnalytical1s(static_cast<float>(sep));
        EnergyResult energy = MolecularEnergyModel::ComputeEnergies(static_cast<float>(sep), S);

        // Bonding energy must be strictly lower than antibonding energy
        EXPECT_LT(energy.bondingEnergy, energy.antibondingEnergy);

        // At large separations, both energies should approach -13.605693 eV
        if (sep == 10.0) {
            EXPECT_NEAR(energy.bondingEnergy, -13.605693, 0.02);
            EXPECT_NEAR(energy.antibondingEnergy, -13.605693, 0.02);
        }
    }
}

TEST(HydrogenOrbitalTests, ValidateEnergySweep)
{
    SweepResult sweep = EnergySweep::RunSweep(0.5f, 10.0f, 0.05f);

    // Export to CSV to verify the exporter code and generate the required deliverable
    bool success = EnergySweep::ExportToCSV(sweep, "exports/h2_energy_curve.csv");
    EXPECT_TRUE(success);

    // Equilibrium bond distance for LCAO H2+ is ~2.5 Bohr
    EXPECT_GT(sweep.equilibriumSeparation, 2.3f);
    EXPECT_LT(sweep.equilibriumSeparation, 2.7f);

    // Minimum bonding energy should be around -15.37 eV
    EXPECT_GT(sweep.minimumBondingEnergy, -15.5);
    EXPECT_LT(sweep.minimumBondingEnergy, -15.2);
}

TEST(HydrogenOrbitalTests, ValidateNumericalOverlapConvergence)
{
    HydrogenicOrbital orbitalA(1, 0, 0, 0.5, false);
    HydrogenicOrbital orbitalB(1, 0, 0, 0.5, false);
    
    double S_analytical = OverlapIntegral::ComputeAnalytical1s(2.0f);

    // Measure error at different grid resolutions
    int resolutions[] = { 20, 40, 60, 80 };
    double errors[4];

    for (int i = 0; i < 4; ++i) {
        double S_numerical = OverlapIntegral::ComputeNumerical(orbitalA, orbitalB, 2.0f, resolutions[i]);
        errors[i] = std::abs(S_analytical - S_numerical);
        std::cout << "[CONVERGENCE TEST] Grid: " << resolutions[i] << "^3 | Error: " << errors[i] << std::endl;
    }

    // Verify error is small at 60^3 and 80^3
    EXPECT_LT(errors[2], 1e-4); // 60^3
    EXPECT_LT(errors[3], 1e-4); // 80^3

    // Verify convergence: error decreases from 20^3 to 60^3
    EXPECT_LT(errors[2], errors[0]);
    // Verify convergence: error decreases from 40^3 to 80^3
    EXPECT_LT(errors[3], errors[1]);
}

TEST(HydrogenOrbitalTests, Validate2pMolecularOrbitals)
{
    // 1. Construct 2p molecular orbitals at R = 3.0 Bohr using the factory
    float R = 3.0f;
    auto sigma2p = MolecularOrbitalFactory::Create(MolecularOrbitalType::Sigma2p, R);
    auto sigma2p_star = MolecularOrbitalFactory::Create(MolecularOrbitalType::Sigma2pStar, R);
    auto pi2p_x = MolecularOrbitalFactory::Create(MolecularOrbitalType::Pi2pX, R);
    auto pi2p_x_star = MolecularOrbitalFactory::Create(MolecularOrbitalType::Pi2pXStar, R);
    auto pi2p_y = MolecularOrbitalFactory::Create(MolecularOrbitalType::Pi2pY, R);
    auto pi2p_y_star = MolecularOrbitalFactory::Create(MolecularOrbitalType::Pi2pYStar, R);

    // Verify types, classifications, and labels
    EXPECT_EQ(sigma2p->GetLabel(), "σ(2p)");
    EXPECT_EQ(sigma2p_star->GetLabel(), "σ*(2p)");
    EXPECT_EQ(pi2p_x->GetLabel(), "π(2p_x)");
    EXPECT_EQ(pi2p_x_star->GetLabel(), "π*(2p_x)");

    // 2. Midpoint density ordering validation at R = 3.0 Bohr
    // Midpoint is x = 0. We evaluate at a small transverse offset z = 0.5 Bohr to verify π orbital density.
    glm::vec3 midpoint_offset(0.0f, 0.0f, 0.5f);
    double den_sigma = sigma2p->ProbabilityDensity(midpoint_offset);
    double den_pi = pi2p_x->ProbabilityDensity(midpoint_offset);
    double den_pi_star = pi2p_x_star->ProbabilityDensity(midpoint_offset);
    double den_sigma_star = sigma2p_star->ProbabilityDensity(midpoint_offset);

    // σ(2p) > π(2p) > π*(2p) >= σ*(2p) at the midpoint plane (with transverse offset)
    EXPECT_GT(den_sigma, den_pi);
    EXPECT_GT(den_pi, den_pi_star);
    EXPECT_NEAR(den_pi_star, 0.0, EPSILON);
    EXPECT_NEAR(den_sigma_star, 0.0, EPSILON);

    // 3. Nodal plane validation at x = 0 (midpoint plane)
    // For all antibonding states, x = 0 must be a nodal plane (zero density)
    glm::vec3 test_points_x0[] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 2.0f},
        {0.0f, -1.5f, 1.5f}
    };
    for (const auto& pt : test_points_x0) {
        EXPECT_NEAR(sigma2p_star->ProbabilityDensity(pt), 0.0, EPSILON);
        EXPECT_NEAR(pi2p_x_star->ProbabilityDensity(pt), 0.0, EPSILON);
        EXPECT_NEAR(pi2p_y_star->ProbabilityDensity(pt), 0.0, EPSILON);
    }

    // 4. Bond axis nodal plane validation for π orbitals
    // π(2p_x) is formed by 2p_z orbitals, so z = 0 is a nodal plane containing the bond axis
    glm::vec3 test_points_z0[] = {
        {0.0f, 0.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f},
        {1.5f, -0.5f, 0.0f}
    };
    for (const auto& pt : test_points_z0) {
        EXPECT_NEAR(pi2p_x->ProbabilityDensity(pt), 0.0, EPSILON);
        EXPECT_NEAR(pi2p_x_star->ProbabilityDensity(pt), 0.0, EPSILON);
    }

    // π(2p_y) is formed by 2p_y orbitals, so y = 0 is a nodal plane containing the bond axis
    glm::vec3 test_points_y0[] = {
        {0.0f, 0.0f, 0.0f},
        {-1.0f, 0.0f, 1.0f},
        {1.5f, 0.0f, -0.5f}
    };
    for (const auto& pt : test_points_y0) {
        EXPECT_NEAR(pi2p_y->ProbabilityDensity(pt), 0.0, EPSILON);
        EXPECT_NEAR(pi2p_y_star->ProbabilityDensity(pt), 0.0, EPSILON);
    }
}

TEST(HydrogenOrbitalTests, ValidateEnergyCurveExplorer)
{
    // 1. Run the sweep using EnergySweep
    SweepResult sweep = EnergySweep::RunSweep(0.5f, 10.0f, 0.05f);

    // 2. Verify sweep data is monotonic in separation
    // Separation must be strictly increasing.
    // Overlap S(R) must be strictly decreasing.
    // Antibonding energy E-(R) must be strictly decreasing (strictly monotonic).
    for (size_t i = 1; i < sweep.points.size(); ++i) {
        EXPECT_GT(sweep.points[i].separation, sweep.points[i-1].separation);
        EXPECT_LT(sweep.points[i].overlap, sweep.points[i-1].overlap);
        EXPECT_LT(sweep.points[i].antibondingEnergy, sweep.points[i-1].antibondingEnergy);
    }

    // 3. Verify bonding minimum remains near 2.5 Bohr
    EXPECT_GT(sweep.equilibriumSeparation, 2.3f);
    EXPECT_LT(sweep.equilibriumSeparation, 2.7f);
    EXPECT_GT(sweep.minimumBondingEnergy, -15.5);
    EXPECT_LT(sweep.minimumBondingEnergy, -15.2);

    // 4. Verify marker lookup returns the correct energy values
    // Find the closest sweep point to a target separation
    auto lookupClosest = [&](float targetSep) -> SweepPoint {
        float minDiff = std::numeric_limits<float>::max();
        SweepPoint closestPt{};
        for (const auto& pt : sweep.points) {
            float diff = std::abs(pt.separation - targetSep);
            if (diff < minDiff) {
                minDiff = diff;
                closestPt = pt;
            }
        }
        return closestPt;
    };

    // Test lookup at separation R = 2.0 Bohr (exactly on the grid)
    float testSep = 2.0f;
    SweepPoint marker = lookupClosest(testSep);
    EXPECT_NEAR(marker.separation, testSep, 1e-5);
    
    // Check that lookup values match direct calculations
    double S = OverlapIntegral::ComputeAnalytical1s(testSep);
    EnergyResult expected = MolecularEnergyModel::ComputeEnergies(testSep, S);
    EXPECT_NEAR(marker.bondingEnergy, expected.bondingEnergy, 1e-5);
    EXPECT_NEAR(marker.antibondingEnergy, expected.antibondingEnergy, 1e-5);
    EXPECT_NEAR(marker.overlap, S, 1e-5);

    // Test lookup at a non-grid separation R = 2.02 Bohr
    float testSepNonGrid = 2.02f;
    SweepPoint markerNonGrid = lookupClosest(testSepNonGrid);
    // The closest point should be 2.00 Bohr (diff <= 0.02 Bohr)
    EXPECT_LT(std::abs(markerNonGrid.separation - testSepNonGrid), 0.03f);
    EXPECT_NEAR(markerNonGrid.separation, 2.0f, 1e-5);
}

class MockCameraController : public CameraController {
public:
    int MousePressedCount = 0;
    int MouseScrolledCount = 0;

    void OnUpdate(float) override {}
    bool OnEvent(Event& e) override {
        if (e.GetType() == EventType::MouseButtonPressed) MousePressedCount++;
        if (e.GetType() == EventType::MouseScrolled) MouseScrolledCount++;
        return false; // Return false so other subscribers can run and verify propagation
    }
};

TEST(HydrogenOrbitalTests, ValidateInputEventRouting)
{
    // 1. Setup event bus, camera manager, and mock controller
    EventBus bus;
    CameraManager manager(bus);
    
    auto controller = std::make_unique<MockCameraController>();
    auto* ctrlPtr = controller.get();
    manager.SetController(std::move(controller));

    // Auxiliary variables and subscribers to detect if events propagate past CameraManager
    bool pressPropagated = false;
    bool scrollPropagated = false;

    auto pressSub = bus.Subscribe<MouseButtonPressedEvent>([&](const MouseButtonPressedEvent&) {
        pressPropagated = true;
        return false;
    });

    auto scrollSub = bus.Subscribe<MouseScrolledEvent>([&](const MouseScrolledEvent&) {
        scrollPropagated = true;
        return false;
    });

    // Verify initial count is zero
    EXPECT_EQ(ctrlPtr->MousePressedCount, 0);
    EXPECT_EQ(ctrlPtr->MouseScrolledCount, 0);

    // 2. Test event routing without ImGui context (headless mode)
    {
        pressPropagated = false;
        scrollPropagated = false;

        bus.Post(MouseButtonPressedEvent(0, 0));
        bus.Post(MouseScrolledEvent(0.0f, 1.0f));
        bus.Dispatch();

        EXPECT_EQ(ctrlPtr->MousePressedCount, 1);
        EXPECT_EQ(ctrlPtr->MouseScrolledCount, 1);
        EXPECT_TRUE(pressPropagated);
        EXPECT_TRUE(scrollPropagated);
    }

    // 3. Setup ImGui context and verify events are blocked when WantCaptureMouse is true
    ImGui::CreateContext();
    ImGui::GetIO().WantCaptureMouse = true;

    {
        pressPropagated = false;
        scrollPropagated = false;

        bus.Post(MouseButtonPressedEvent(0, 0));
        bus.Post(MouseScrolledEvent(0.0f, 1.0f));
        bus.Dispatch();

        // Counts should remain at 1 (not route to camera controller)
        EXPECT_EQ(ctrlPtr->MousePressedCount, 1);
        EXPECT_EQ(ctrlPtr->MouseScrolledCount, 1);
        
        // Propagation should be blocked (events marked handled before reaching general observers)
        EXPECT_FALSE(pressPropagated);
        EXPECT_FALSE(scrollPropagated);
    }

    // 4. Verify events route normally when WantCaptureMouse is false
    ImGui::GetIO().WantCaptureMouse = false;

    {
        pressPropagated = false;
        scrollPropagated = false;

        bus.Post(MouseButtonPressedEvent(0, 0));
        bus.Post(MouseScrolledEvent(0.0f, 1.0f));
        bus.Dispatch();

        // Counts should increment to 2
        EXPECT_EQ(ctrlPtr->MousePressedCount, 2);
        EXPECT_EQ(ctrlPtr->MouseScrolledCount, 2);
        
        // Propagation should succeed
        EXPECT_TRUE(pressPropagated);
        EXPECT_TRUE(scrollPropagated);
    }

    // Clean up
    ImGui::DestroyContext();
}



