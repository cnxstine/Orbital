#pragma once

#include <vector>
#include <string>

namespace Orbital {

struct SweepPoint {
    float separation;
    double overlap;
    double bondingEnergy;
    double antibondingEnergy;
};

struct SweepResult {
    std::vector<SweepPoint> points;
    float equilibriumSeparation; // in Bohr
    double minimumBondingEnergy;  // in eV
};

class EnergySweep {
public:
    /**
     * @brief Run the energy vs separation sweep from 0.5 to 10.0 Bohr
     */
    static SweepResult RunSweep(float startSep = 0.5f, float endSep = 10.0f, float step = 0.05f);

    /**
     * @brief Export the sweep result to a CSV file
     */
    static bool ExportToCSV(const SweepResult& sweep, const std::string& filepath);
};

} // namespace Orbital
