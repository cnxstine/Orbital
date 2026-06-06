#include "visualization/EnergySweep.hpp"
#include "visualization/OverlapIntegral.hpp"
#include "visualization/MolecularEnergyModel.hpp"
#include "core/Log.hpp"
#include <fstream>
#include <filesystem>
#include <limits>
#include <iostream>

namespace Orbital {

SweepResult EnergySweep::RunSweep(float startSep, float endSep, float step) {
    SweepResult result;
    result.minimumBondingEnergy = std::numeric_limits<double>::max();
    result.equilibriumSeparation = 0.0f;

    // 1s orbitals are used for the standard H2 LCAO sweep
    HydrogenicOrbital orbitalA(1, 0, 0, 0.5, false);
    HydrogenicOrbital orbitalB(1, 0, 0, 0.5, false);

    for (float sep = startSep; sep <= endSep; sep += step) {
        double S = OverlapIntegral::ComputeAnalytical1s(sep);
        EnergyResult energies = MolecularEnergyModel::ComputeEnergies(sep, S);

        SweepPoint pt;
        pt.separation = sep;
        pt.overlap = S;
        pt.bondingEnergy = energies.bondingEnergy;
        pt.antibondingEnergy = energies.antibondingEnergy;

        result.points.push_back(pt);

        if (pt.bondingEnergy < result.minimumBondingEnergy) {
            result.minimumBondingEnergy = pt.bondingEnergy;
            result.equilibriumSeparation = pt.separation;
        }
    }

    return result;
}

bool EnergySweep::ExportToCSV(const SweepResult& sweep, const std::string& filepath) {
    try {
        std::filesystem::path path(filepath);
        if (path.has_parent_path()) {
            if (!path.parent_path().empty()) {
                std::filesystem::create_directories(path.parent_path());
            }
        }
        std::ofstream file(filepath);
        if (!file) {
            ORB_CORE_ERROR("Failed to open file for exporting sweep: {}", filepath);
            return false;
        }

        // Export with units in headers as requested:
        file << "Separation_Bohr,Overlap,BondingEnergy_eV,AntibondingEnergy_eV\n";
        for (const auto& pt : sweep.points) {
            file << pt.separation << ","
                 << pt.overlap << ","
                 << pt.bondingEnergy << ","
                 << pt.antibondingEnergy << "\n";
        }

        ORB_CORE_INFO("Exported energy sweep results to: {}", filepath);
        return true;
    } catch (const std::exception& e) {
        ORB_CORE_ERROR("Exception occurred during CSV export: {}", e.what());
        return false;
    }
}

} // namespace Orbital
