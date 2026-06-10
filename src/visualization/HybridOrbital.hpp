#pragma once

#include "visualization/WaveFunction.hpp"
#include "visualization/HydrogenicOrbital.hpp"
#include "visualization/HybridOrbitalType.hpp"
#include <memory>
#include <string>

namespace Orbital {

class HybridOrbital : public WaveFunction {
public:
    HybridOrbital(HybridOrbitalType type, int orbitalIndex);
    virtual ~HybridOrbital() override = default;

    // DataSet overrides
    [[nodiscard]] const DataSetMetadata& GetMetadata() const noexcept override { return m_Metadata; }
    [[nodiscard]] std::type_index GetTypeIndex() const noexcept override { return typeid(HybridOrbital); }

    // WaveFunction overrides
    [[nodiscard]] std::complex<double> Evaluate(const glm::vec3& position) const override;
    [[nodiscard]] double ProbabilityDensity(const glm::vec3& position) const override;

    // Hybrid-specific accessors
    [[nodiscard]] HybridOrbitalType GetType() const noexcept { return m_Type; }
    [[nodiscard]] int GetOrbitalIndex() const noexcept { return m_OrbitalIndex; }
    [[nodiscard]] double GetMaxDensity() const noexcept { return m_MaxDensity; }

    [[nodiscard]] double GetCoeff2s() const noexcept { return m_c2s; }
    [[nodiscard]] double GetCoeff2px() const noexcept { return m_c2px; }
    [[nodiscard]] double GetCoeff2py() const noexcept { return m_c2py; }
    [[nodiscard]] double GetCoeff2pz() const noexcept { return m_c2pz; }

private:
    void SetupCoefficients();
    void CalculateMaxDensity();

    HybridOrbitalType m_Type;
    int m_OrbitalIndex;
    DataSetMetadata m_Metadata;
    double m_MaxDensity = 0.0;

    // Pre-allocated constituent orbitals
    std::shared_ptr<HydrogenicOrbital> m_2s;
    std::shared_ptr<HydrogenicOrbital> m_2px;
    std::shared_ptr<HydrogenicOrbital> m_2py;
    std::shared_ptr<HydrogenicOrbital> m_2pz;

    // Coefficients
    double m_c2s = 0.0;
    double m_c2px = 0.0;
    double m_c2py = 0.0;
    double m_c2pz = 0.0;
};

} // namespace Orbital
