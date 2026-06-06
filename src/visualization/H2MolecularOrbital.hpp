#pragma once

#include "visualization/MolecularOrbital.hpp"
#include "visualization/BondingWaveFunction.hpp"
#include "visualization/HydrogenicOrbital.hpp"
#include <memory>
#include <string>

namespace Orbital {

class H2MolecularOrbital : public MolecularOrbital {
public:
    enum class Type {
        Sigma1s = 0,
        Sigma1sStar = 1
    };

    H2MolecularOrbital(Type type, float separation)
        : m_Type(type), m_Separation(separation)
    {
        // 1s orbital is defined by n=1, l=0, m=0
        m_OrbitalA = std::make_shared<HydrogenicOrbital>(1, 0, 0, 0.5, false);
        m_OrbitalB = std::make_shared<HydrogenicOrbital>(1, 0, 0, 0.5, false);

        BondingWaveFunction::StateType stateType = (type == Type::Sigma1s)
            ? BondingWaveFunction::StateType::Bonding
            : BondingWaveFunction::StateType::Antibonding;

        m_Underlying = std::make_unique<BondingWaveFunction>(m_OrbitalA, m_OrbitalB, m_Separation, stateType);
    }

    virtual ~H2MolecularOrbital() override = default;

    // From DataSet
    [[nodiscard]] const DataSetMetadata& GetMetadata() const noexcept override {
        return m_Underlying->GetMetadata();
    }

    [[nodiscard]] std::type_index GetTypeIndex() const noexcept override {
        return typeid(H2MolecularOrbital);
    }

    // From WaveFunction
    [[nodiscard]] std::complex<double> Evaluate(const glm::vec3& position) const override {
        return m_Underlying->Evaluate(position);
    }

    [[nodiscard]] double ProbabilityDensity(const glm::vec3& position) const override {
        return m_Underlying->ProbabilityDensity(position);
    }

    // From MolecularOrbital
    [[nodiscard]] int GetEnergyOrdering() const override {
        return (m_Type == Type::Sigma1s) ? 1 : 2;
    }

    [[nodiscard]] std::string GetLabel() const override {
        return (m_Type == Type::Sigma1s) ? "σ(1s)" : "σ*(1s)";
    }

    [[nodiscard]] std::string GetDescription() const override {
        if (m_Type == Type::Sigma1s) {
            return "Bonding σ(1s) molecular orbital formed by the in-phase (constructive) combination of two hydrogen 1s atomic orbitals.";
        } else {
            return "Antibonding σ*(1s) molecular orbital formed by the out-of-phase (destructive) combination of two hydrogen 1s atomic orbitals.";
        }
    }

    [[nodiscard]] double GetMaxDensity() const {
        return m_Underlying->GetMaxDensity();
    }

    [[nodiscard]] float GetSeparation() const noexcept {
        return m_Separation;
    }

    [[nodiscard]] Type GetType() const noexcept {
        return m_Type;
    }

private:
    Type m_Type;
    float m_Separation;
    std::shared_ptr<HydrogenicOrbital> m_OrbitalA;
    std::shared_ptr<HydrogenicOrbital> m_OrbitalB;
    std::unique_ptr<BondingWaveFunction> m_Underlying;
};

} // namespace Orbital
