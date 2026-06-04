#pragma once

/**
 * @file core/Application.hpp
 * @brief Entry point abstraction. Users subclass Application to configure the Engine.
 *
 * Application is the user-facing API that sits between main() and Engine.
 * It creates the Engine and gives derived classes hooks to push their initial layers.
 *
 * Usage (in user code):
 *
 *   class OrbitalApp : public Application {
 *   public:
 *       OrbitalApp() {
 *           GetEngine().PushLayer(std::make_unique<OrbitalViewerLayer>());
 *       }
 *   };
 *
 *   // In main.cpp:
 *   int main() {
 *       OrbitalApp app;
 *       app.Run();
 *   }
 *
 * The Application owns the Engine. The Engine drives the main loop.
 * Log::Init() is called here (before Engine construction) so all subsequent
 * Engine/platform code can use the ORB_CORE_* macros.
 */

#include "core/Engine.hpp"
#include <string_view>

namespace Orbital {

struct ApplicationSpec {
    std::string_view Name        = "Orbital Application";
    uint32_t         WindowWidth  = 1600;
    uint32_t         WindowHeight = 900;
    bool             VSync        = true;
};

class Application {
public:
    explicit Application(const ApplicationSpec& spec = {});
    virtual ~Application();

    Application(const Application&)            = delete;
    Application& operator=(const Application&) = delete;

    /// Blocks until the window is closed or RequestShutdown() is called.
    void Run();

    [[nodiscard]] Engine& GetEngine() noexcept { return *m_Engine; }

protected:
    /// Override to push initial layers before Run() starts the main loop.
    virtual void OnInit() {}

    /// Override to perform cleanup after the main loop exits.
    virtual void OnShutdown() {}

private:
    std::unique_ptr<Engine> m_Engine;
};

} // namespace Orbital
