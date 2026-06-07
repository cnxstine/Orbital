#pragma once

#include "visualization/MolecularOrbital.hpp"
#include "visualization/BondingWaveFunction.hpp"
#include "visualization/HydrogenicOrbital.hpp"
#include "visualization/MolecularOrbitalType.hpp"
#include <memory>
#include <string>

namespace Orbital {

class LCAOMolecularOrbital : public MolecularOrbital {
public:
    LCAOMolecularOrbital(MolecularOrbitalType type, float separation);
    virtual ~LCAOMolecularOrbital() override = default;

    // DataSet overrides
    [[nodiscard]] const DataSetMetadata& GetMetadata() const noexcept override {
        return m_Underlying->GetMetadata();
    }
    [[nodiscard]] std::type_index GetTypeIndex() const noexcept override {
        return typeid(LCAOMolecularOrbital);
    }

    // WaveFunction overrides
    [[nodiscard]] std::complex<double> Evaluate(const glm::vec3& position) const override {
        return m_Underlying->Evaluate(position);
    }
    [[nodiscard]] double ProbabilityDensity(const glm::vec3& position) const override {
        return m_Underlying->ProbabilityDensity(position);
    }

    // MolecularOrbital overrides
    [[nodiscard]] virtual int GetEnergyOrdering() const override;
    [[nodiscard]] virtual std::string GetLabel() const override;
    [[nodiscard]] virtual std::string GetDescription() const override;
    [[nodiscard]] virtual double GetMaxDensity() const override {
        return m_Underlying->GetMaxDensity();
    }

    // Helper utilities
    [[nodiscard]] float GetSeparation() const noexcept {
        return m_Separation;
    }
    [[nodiscard]] MolecularOrbitalType GetType() const noexcept {
        return m_Type;
    }

private:
    MolecularOrbitalType m_Type;
    float m_Separation;
    std::shared_ptr<HydrogenicOrbital> m_OrbitalA;
    std::shared_ptr<HydrogenicOrbital> m_OrbitalB;
    std::unique_ptr<BondingWaveFunction> m_Underlying;
};

} // namespace Orbital
