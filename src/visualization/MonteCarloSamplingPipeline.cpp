#include "visualization/MonteCarloSamplingPipeline.hpp"
#include "visualization/HydrogenOrbitalData.hpp"
#include "visualization/HydrogenicOrbital.hpp"
#include "visualization/BondingWaveFunction.hpp"
#include "visualization/StochasticProbabilityCloud.hpp"
#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <cmath>
#include <chrono>

namespace Orbital {

MonteCarloSamplingPipeline::MonteCarloSamplingPipeline()
{
    // Seed with current time
    m_Rng.seed(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
}

Result<void> MonteCarloSamplingPipeline::SampleToField(const WaveFunction& source, 
                                                       std::shared_ptr<DensityField> targetField)
{
    // Volume density sampling (future implementation)
    return {};
}

Result<void> MonteCarloSamplingPipeline::SampleToCloud(const WaveFunction& source, 
                                                       std::shared_ptr<ProbabilityCloud> targetCloud, 
                                                       uint32_t numSamples)
{
    auto stochasticCloud = std::dynamic_pointer_cast<StochasticProbabilityCloud>(targetCloud);
    if (!stochasticCloud) {
        return OrbitalError::Logic("MonteCarloSamplingPipeline: targetCloud is not a StochasticProbabilityCloud.");
    }

    std::vector<SamplePoint> points;
    auto result = SampleToVector(source, points, numSamples);
    if (!result) return result.error();

    // Set points in cloud (which handles GPU upload)
    stochasticCloud->SetPoints(std::move(points));
    return {};
}

void MonteCarloSamplingPipeline::SetResolution(VoxelResolution resolution)
{
    m_Resolution = resolution;
}

VoxelResolution MonteCarloSamplingPipeline::GetResolution() const noexcept
{
    return m_Resolution;
}

void MonteCarloSamplingPipeline::RegisterFrameTime(float frameTimeMs)
{
    m_LastFrameTimeMs = frameTimeMs;
}

Result<void> MonteCarloSamplingPipeline::SampleToVector(const WaveFunction& source, 
                                                         std::vector<SamplePoint>& outPoints, 
                                                         uint32_t numSamples)
{
    outPoints.clear();
    outPoints.reserve(numSamples);

    // Try to downcast to HydrogenOrbitalData for direct analytical sampling
    const auto* hydrogenData = dynamic_cast<const HydrogenOrbitalData*>(&source);
    if (hydrogenData) {
        const auto& qn = hydrogenData->GetQuantumNumbers();
        // Currently support direct analytical sampling for n <= 2
        if (qn.n <= 2) {
            ORB_CORE_TRACE("MonteCarloSamplingPipeline: Prioritizing Direct Analytical Sampling for Hydrogen (n={}, l={}, m={})", 
                           qn.n, qn.l, qn.m);
            return SampleDirect(source, outPoints, numSamples);
        }
    }

    // General fallback: Metropolis-Hastings MCMC
    ORB_CORE_TRACE("MonteCarloSamplingPipeline: Falling back to Metropolis-Hastings MCMC Sampling");
    return SampleMCMC(source, outPoints, numSamples);
}

Result<void> MonteCarloSamplingPipeline::SampleDirect(const WaveFunction& source, 
                                                       std::vector<SamplePoint>& outPoints, 
                                                       uint32_t numSamples)
{
    const auto* hydrogenData = dynamic_cast<const HydrogenOrbitalData*>(&source);
    ORB_ASSERT(hydrogenData != nullptr, "SampleDirect called on non-hydrogenic source");
    const auto& qn = hydrogenData->GetQuantumNumbers();

    const auto* hydrogenic = dynamic_cast<const HydrogenicOrbital*>(&source);
    bool isRealCombo = hydrogenic && hydrogenic->IsRealCombo();

    double maxDensity = 1.0;
    if (qn.n == 1 && qn.l == 0) {
        maxDensity = 1.0 / std::numbers::pi;
    } else if (qn.n == 2 && qn.l == 0) {
        maxDensity = 1.0 / (8.0 * std::numbers::pi);
    } else if (qn.n == 2 && qn.l == 1) {
        maxDensity = std::exp(-2.0) / (8.0 * std::numbers::pi);
    }

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::uniform_real_distribution<double> distNeg11(-1.0, 1.0);
    const double doublePi = 2.0 * std::numbers::pi;

    for (uint32_t i = 0; i < numSamples; ++i) {
        double r = 0.0;
        double cosTheta = 0.0;

        // ── 1. Radial Sampling ───────────────────────────────────────────────
        if (qn.n == 1) {
            // 1s radial PDF: P(r) = 4 * r^2 * e^(-2r)
            // This is Gamma(3, 2). Rate lambda = 2.0.
            // Sample as sum of 3 exponential random variables.
            double u1 = dist01(m_Rng);
            double u2 = dist01(m_Rng);
            double u3 = dist01(m_Rng);
            // Avoid log(0)
            u1 = std::max(u1, 1e-15);
            u2 = std::max(u2, 1e-15);
            u3 = std::max(u3, 1e-15);
            r = -0.5 * std::log(u1 * u2 * u3);
        }
        else if (qn.n == 2) {
            if (qn.l == 0) {
                // 2s radial PDF: P(r) ~ (2-r)^2 * r^2 * e^(-r)
                // Use Rejection Sampling. Peak is at r approx 5.236 (max height approx 1.52).
                // Bounding box: r in [0, 24], y in [0, 1.6]
                while (true) {
                    double testR = dist01(m_Rng) * 24.0;
                    double testY = dist01(m_Rng) * 1.6;
                    
                    // Evaluate radial distribution function (unnormalized PDF part)
                    double diff = 2.0 - testR;
                    double density = (1.0 / 8.0) * diff * diff * testR * testR * std::exp(-testR);
                    if (testY < density) {
                        r = testR;
                        break;
                    }
                }
            }
            else if (qn.l == 1) {
                // 2p radial PDF: P(r) = 1/24 * r^4 * e^(-r)
                // This is Gamma(5, 1). Rate lambda = 1.0.
                // Sample as sum of 5 exponential random variables.
                double product = 1.0;
                for (int k = 0; k < 5; ++k) {
                    double u = dist01(m_Rng);
                    product *= std::max(u, 1e-15);
                }
                r = -std::log(product);
            }
        }

        // ── 2. Angular Sampling ──────────────────────────────────────────────
        if (qn.l == 0) {
            // Spherically symmetric: cos(theta) is uniform in [-1, 1]
            cosTheta = distNeg11(m_Rng);
        }
        else if (qn.l == 1) {
            bool useDumbbellDist = (qn.m == 0) || (isRealCombo && (qn.m == 1 || qn.m == -1));
            if (useDumbbellDist) {
                // 2p_z (or real 2p_x / 2p_y before coordinate swapping) angular PDF ~ cos^2(theta) * sin(theta)
                // PDF in mu = cos(theta) is 1.5 * mu^2 on [-1, 1].
                // CDF is 0.5 * (mu^3 + 1). Inverting gives: mu = (2*u - 1)^(1/3)
                double u = dist01(m_Rng);
                double val = 2.0 * u - 1.0;
                cosTheta = (val >= 0.0) ? std::pow(val, 1.0 / 3.0) : -std::pow(-val, 1.0 / 3.0);
            }
            else {
                // Complex 2p_+/-1 angular PDF ~ sin^2(theta) * sin(theta) = (1 - mu^2) * sin(theta)
                // PDF in mu is 0.75 * (1 - mu^2) on [-1, 1].
                // Rejection sample mu in [-1, 1], y in [0, 0.75]
                while (true) {
                    double mu = distNeg11(m_Rng);
                    double y = dist01(m_Rng) * 0.75;
                    if (y < 0.75 * (1.0 - mu * mu)) {
                        cosTheta = mu;
                        break;
                    }
                }
            }
        }

        // ── 3. Phi Sampling ──────────────────────────────────────────────────
        double phi = dist01(m_Rng) * doublePi;

        // ── 4. Coordinate Conversion ─────────────────────────────────────────
        double sinTheta = std::sqrt(std::max(0.0, 1.0 - cosTheta * cosTheta));
        glm::vec3 pos;
        if (qn.n == 2 && qn.l == 1 && isRealCombo) {
            if (qn.m == 1) { // 2p_x: swap z and x
                pos.x = static_cast<float>(r * cosTheta);
                pos.y = static_cast<float>(r * sinTheta * std::sin(phi));
                pos.z = static_cast<float>(r * sinTheta * std::cos(phi));
            } else if (qn.m == -1) { // 2p_y: swap z and y
                pos.x = static_cast<float>(r * sinTheta * std::cos(phi));
                pos.y = static_cast<float>(r * cosTheta);
                pos.z = static_cast<float>(r * sinTheta * std::sin(phi));
            } else { // 2p_z
                pos.x = static_cast<float>(r * sinTheta * std::cos(phi));
                pos.y = static_cast<float>(r * sinTheta * std::sin(phi));
                pos.z = static_cast<float>(r * cosTheta);
            }
        } else {
            pos.x = static_cast<float>(r * sinTheta * std::cos(phi));
            pos.y = static_cast<float>(r * sinTheta * std::sin(phi));
            pos.z = static_cast<float>(r * cosTheta);
        }

        // Evaluate actual physical wavefunction values for phase & density
        auto val = source.Evaluate(pos);
        
        SamplePoint pt;
        pt.Position = pos;
        pt.Density  = static_cast<float>(std::norm(val) / maxDensity);
        pt.Phase    = static_cast<float>(std::arg(val));

        outPoints.push_back(pt);
    }

    return {};
}

Result<void> MonteCarloSamplingPipeline::SampleMCMC(const WaveFunction& source, 
                                                     std::vector<SamplePoint>& outPoints, 
                                                     uint32_t numSamples)
{
    double maxDensity = 1.0;
    const auto* hydrogenData = dynamic_cast<const HydrogenOrbitalData*>(&source);
    if (hydrogenData) {
        const auto& qn = hydrogenData->GetQuantumNumbers();
        if (qn.n == 1 && qn.l == 0) {
            maxDensity = 1.0 / std::numbers::pi;
        } else if (qn.n == 2 && qn.l == 0) {
            maxDensity = 1.0 / (8.0 * std::numbers::pi);
        } else if (qn.n == 2 && qn.l == 1) {
            maxDensity = std::exp(-2.0) / (8.0 * std::numbers::pi);
        }
    } else {
        const auto* bondingWF = dynamic_cast<const BondingWaveFunction*>(&source);
        if (bondingWF) {
            maxDensity = bondingWF->GetMaxDensity();
        }
    }

    std::uniform_real_distribution<double> dist01(0.0, 1.0);
    std::uniform_real_distribution<double> stepDist(-1.5, 1.5); // step size parameter width

    // Initial state: start at (1, 0, 0)
    glm::vec3 currentPos(1.0f, 0.0f, 0.0f);
    double currentDensity = source.ProbabilityDensity(currentPos);

    // Warm-up / Burn-in phase to reach stationary distribution
    for (int step = 0; step < 1000; ++step) {
        glm::vec3 proposedPos = currentPos + glm::vec3(stepDist(m_Rng), stepDist(m_Rng), stepDist(m_Rng));
        double proposedDensity = source.ProbabilityDensity(proposedPos);
        
        double acceptanceRatio = (currentDensity > 0.0) ? (proposedDensity / currentDensity) : 1.0;
        if (dist01(m_Rng) < acceptanceRatio) {
            currentPos = proposedPos;
            currentDensity = proposedDensity;
        }
    }

    // Sampling loop
    for (uint32_t i = 0; i < numSamples; ++i) {
        // Run MCMC chain for 3 step proposals to reduce autocorrelation
        for (int k = 0; k < 3; ++k) {
            glm::vec3 proposedPos = currentPos + glm::vec3(stepDist(m_Rng), stepDist(m_Rng), stepDist(m_Rng));
            double proposedDensity = source.ProbabilityDensity(proposedPos);
            
            double acceptanceRatio = (currentDensity > 0.0) ? (proposedDensity / currentDensity) : 1.0;
            if (dist01(m_Rng) < acceptanceRatio) {
                currentPos = proposedPos;
                currentDensity = proposedDensity;
            }
        }

        SamplePoint pt;
        pt.Position = currentPos;
        pt.Density  = static_cast<float>(currentDensity / maxDensity);
        
        auto val = source.Evaluate(currentPos);
        pt.Phase = static_cast<float>(std::arg(val));

        outPoints.push_back(pt);
    }

    return {};
}

} // namespace Orbital
