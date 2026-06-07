#pragma once

#include "visualization/MolecularOrbital.hpp"
#include "visualization/MolecularOrbitalType.hpp"
#include <memory>

namespace Orbital {

class MolecularOrbitalFactory {
public:
    static std::shared_ptr<MolecularOrbital> Create(MolecularOrbitalType type, float separation);
};

} // namespace Orbital
