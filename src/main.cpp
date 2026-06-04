/**
 * @file main.cpp
 * @brief Orbital application entry point.
 *
 * Constructs the application, pushes the initial rendering layer, and runs.
 * As the project grows, this file stays minimal — all configuration belongs
 * in the Application subclass.
 */

#include "core/Application.hpp"
#include "core/Layer.hpp"
#include "core/Log.hpp"
#include "visualization/ModuleLayer.hpp"
#include "visualization/OrbitalViewerModule.hpp"

#include <memory>

namespace Orbital {

/**
 * @brief Concrete Orbital application.
 *
 * Pushes the ModuleLayer containing the OrbitalViewerModule.
 */
class OrbitalApplication final : public Application {
public:
    OrbitalApplication()
        : Application(ApplicationSpec{
            .Name        = "Orbital — Quantum Mechanics Visualization",
            .WindowWidth  = 1600,
            .WindowHeight = 900,
            .VSync        = true
        })
    {}

protected:
    void OnInit() override
    {
        ORB_CORE_INFO("OrbitalApplication::OnInit() — pushing initial layers");
        
        auto viewerModule = std::make_unique<OrbitalViewerModule>(GetEngine());
        auto moduleLayer = std::make_unique<ModuleLayer>();
        moduleLayer->SetActiveModule(std::move(viewerModule));
        
        GetEngine().PushLayer(std::move(moduleLayer));
    }

    void OnShutdown() override
    {
        ORB_CORE_INFO("OrbitalApplication::OnShutdown()");
    }
};

} // namespace Orbital

// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    try {
        Orbital::OrbitalApplication app;
        app.Run();
    }
    catch (const std::exception& ex) {
        // Log::Init() may not have run if Application constructor threw early.
        // Fall back to stderr.
        if (Orbital::Log::GetCoreLogger()) {
            ORB_CORE_CRITICAL("Unhandled exception: {}", ex.what());
        } else {
            fprintf(stderr, "[FATAL] Unhandled exception: %s\n", ex.what());
        }
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
