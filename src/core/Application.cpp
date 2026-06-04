#include "core/Application.hpp"
#include "core/Log.hpp"

namespace Orbital {

Application::Application(const ApplicationSpec& spec)
{
    Log::Init();
    ORB_CORE_INFO("──────────────────────────────────────────");
    ORB_CORE_INFO("  Orbital v0.1.0");
    ORB_CORE_INFO("  Application: {}", spec.Name);
    ORB_CORE_INFO("──────────────────────────────────────────");

    EngineSpec engineSpec;
    engineSpec.Window.Title  = std::string(spec.Name);
    engineSpec.Window.Width  = spec.WindowWidth;
    engineSpec.Window.Height = spec.WindowHeight;
    engineSpec.Window.VSync  = spec.VSync;

    m_Engine = std::make_unique<Engine>(engineSpec);
}

Application::~Application()
{
    OnShutdown();
    m_Engine.reset();
    ORB_CORE_INFO("Application destroyed");
}

void Application::Run()
{
    OnInit();
    m_Engine->Run();
}

} // namespace Orbital
