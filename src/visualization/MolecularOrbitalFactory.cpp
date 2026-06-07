#include "visualization/MolecularOrbitalFactory.hpp"
#include "visualization/H2MolecularOrbital.hpp"
#include "visualization/LCAOMolecularOrbital.hpp"

namespace Orbital {

std::shared_ptr<MolecularOrbital> MolecularOrbitalFactory::Create(MolecularOrbitalType type, float separation) {
    if (type == MolecularOrbitalType::Sigma1s) {
        return std::make_shared<H2MolecularOrbital>(H2MolecularOrbital::Type::Sigma1s, separation);
    } else if (type == MolecularOrbitalType::Sigma1sStar) {
        return std::make_shared<H2MolecularOrbital>(H2MolecularOrbital::Type::Sigma1sStar, separation);
    } else {
        return std::make_shared<LCAOMolecularOrbital>(type, separation);
    }
}

} // namespace Orbital
