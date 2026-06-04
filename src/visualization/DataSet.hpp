#pragma once

/**
 * @file visualization/DataSet.hpp
 * @brief Base scientific data abstraction for the Orbital framework.
 */

#include <string>
#include <typeindex>
#include <unordered_map>

namespace Orbital {

enum class DataSetType {
    Discrete,   // e.g. Atomic coordinates, crystal lattices
    Continuous, // e.g. Volumetric fields, continuous wavefunctions
    Spectral    // e.g. Energy bands, phonon dispersion curves
};

struct DataSetMetadata {
    std::string Name;
    std::string Description;
    DataSetType Type;
    std::unordered_map<std::string, double> ScientificConstants;
};

class DataSet {
public:
    virtual ~DataSet() = default;

    [[nodiscard]] virtual const DataSetMetadata& GetMetadata() const noexcept = 0;
    [[nodiscard]] virtual std::type_index GetTypeIndex() const noexcept = 0;
};

} // namespace Orbital
