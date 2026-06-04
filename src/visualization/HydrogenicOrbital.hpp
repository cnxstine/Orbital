#pragma once

/**
 * @file visualization/HydrogenicOrbital.hpp
 * @brief Concrete implementation of hydrogen atomic wavefunctions.
 */

#include "visualization/HydrogenOrbitalData.hpp"

namespace Orbital {

class HydrogenicOrbital : public HydrogenOrbitalData {
public:
    HydrogenicOrbital(int n, int l, int m, double spin = 0.5, bool isRealCombo = false);
    virtual ~HydrogenicOrbital() = default;

    // DataSet overrides
    [[nodiscard]] const DataSetMetadata& GetMetadata() const noexcept override { return m_Metadata; }
    [[nodiscard]] std::type_index GetTypeIndex() const noexcept override { return typeid(HydrogenicOrbital); }

    // WaveFunction overrides
    [[nodiscard]] std::complex<double> Evaluate(const glm::vec3& position) const override;
    [[nodiscard]] double ProbabilityDensity(const glm::vec3& position) const override;

    // HydrogenOrbitalData overrides
    [[nodiscard]] const QuantumNumbers& GetQuantumNumbers() const noexcept override { return m_QN; }
    [[nodiscard]] double GetEnergy() const noexcept override { return m_Energy; }

    [[nodiscard]] bool IsRealCombo() const noexcept { return m_IsRealCombo; }

private:
    QuantumNumbers m_QN;
    double m_Energy;
    bool m_IsRealCombo = false;
    DataSetMetadata m_Metadata;
};

} // namespace Orbital
