#pragma once

#include "visualization/WaveFunction.hpp"
#include "visualization/HydrogenicOrbital.hpp"
#include <memory>
#include <algorithm>
#include <cmath>

namespace Orbital {

class BondingWaveFunction : public WaveFunction {
public:
    enum class StateType {
        Bonding,
        Antibonding,
        OrbitalA,
        OrbitalB
    };

    BondingWaveFunction(std::shared_ptr<HydrogenicOrbital> orbitalA,
                        std::shared_ptr<HydrogenicOrbital> orbitalB,
                        float separation,
                        StateType stateType = StateType::Bonding)
        : m_OrbitalA(std::move(orbitalA)),
          m_OrbitalB(std::move(orbitalB)),
          m_Separation(separation),
          m_StateType(stateType)
    {
        m_Metadata.Name = "Bonding WaveFunction";
        m_Metadata.Description = "LCAO representation of two interacting hydrogenic orbitals";
        m_Metadata.Type = DataSetType::Continuous;
        m_Metadata.ScientificConstants["separation"] = separation;
        m_Metadata.ScientificConstants["state_type"] = static_cast<double>(stateType);
    }

    virtual ~BondingWaveFunction() override = default;

    [[nodiscard]] const DataSetMetadata& GetMetadata() const noexcept override { return m_Metadata; }
    [[nodiscard]] std::type_index GetTypeIndex() const noexcept override { return typeid(BondingWaveFunction); }

    [[nodiscard]] std::complex<double> Evaluate(const glm::vec3& position) const override
    {
        glm::vec3 posA = position - glm::vec3(-m_Separation * 0.5f, 0.0f, 0.0f);
        glm::vec3 posB = position - glm::vec3(m_Separation * 0.5f, 0.0f, 0.0f);

        auto psiA = m_OrbitalA ? m_OrbitalA->Evaluate(posA) : std::complex<double>(0.0);
        auto psiB = m_OrbitalB ? m_OrbitalB->Evaluate(posB) : std::complex<double>(0.0);

        switch (m_StateType) {
            case StateType::Bonding:
                return psiA + psiB;
            case StateType::Antibonding:
                return psiA - psiB;
            case StateType::OrbitalA:
                return psiA;
            case StateType::OrbitalB:
                return psiB;
        }
        return 0.0;
    }

    [[nodiscard]] double ProbabilityDensity(const glm::vec3& position) const override
    {
        return std::norm(Evaluate(position));
    }

    [[nodiscard]] double GetMaxDensity() const {
        glm::vec3 nucleusA(-m_Separation * 0.5f, 0.0f, 0.0f);
        glm::vec3 nucleusB(m_Separation * 0.5f, 0.0f, 0.0f);
        glm::vec3 midpoint(0.0f, 0.0f, 0.0f);

        double denA = ProbabilityDensity(nucleusA);
        double denB = ProbabilityDensity(nucleusB);
        double denMid = ProbabilityDensity(midpoint);

        return std::max({denA, denB, denMid, 1e-5});
    }

    [[nodiscard]] StateType GetStateType() const noexcept { return m_StateType; }
    [[nodiscard]] float GetSeparation() const noexcept { return m_Separation; }

private:
    std::shared_ptr<HydrogenicOrbital> m_OrbitalA;
    std::shared_ptr<HydrogenicOrbital> m_OrbitalB;
    float m_Separation;
    StateType m_StateType;
    DataSetMetadata m_Metadata;
};

} // namespace Orbital
