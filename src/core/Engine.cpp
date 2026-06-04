#include "core/Engine.hpp"
#include "core/Log.hpp"
#include "core/Assert.hpp"
#include "platform/Input.hpp"
#include "renderer/Renderer.hpp"
#include "camera/CameraManager.hpp"
#include "camera/PerspectiveCamera.hpp"
#include "camera/controllers/ArcballController.hpp"
#include "events/events/WindowEvents.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

Engine::Engine(const EngineSpec& spec)
{
    ORB_CORE_INFO("Engine initialising...");

    // Layer 0: Window (creates GL context internally)
    m_Window = std::make_unique<Window>(spec.Window, m_EventBus);

    // Layer 2: Renderer
    m_Renderer = std::make_unique<Renderer>();
    m_Renderer->Init(spec.Window.Width, spec.Window.Height);

    // Layer 2: Camera — default Arcball targeting origin from z=5
    m_CameraManager = std::make_unique<CameraManager>(m_EventBus);

    auto cam = std::make_unique<PerspectiveCamera>(
        glm::radians(45.0f),
        m_Window->GetAspect(),
        0.01f,
        1000.0f
    );
    cam->SetPosition({0.0f, 0.0f, 5.0f});
    cam->SetTarget  ({0.0f, 0.0f, 0.0f});

    auto arcball = std::make_unique<ArcballController>();
    m_CameraManager->SetCamera(std::move(cam));
    m_CameraManager->SetController(std::move(arcball));

    // Subscribe to window events
    m_CloseToken  = m_EventBus.Subscribe<WindowCloseEvent>(
        [this](const WindowCloseEvent&) { m_Running = false; return true; });

    m_ResizeToken = m_EventBus.Subscribe<WindowResizeEvent>(
        [this](const WindowResizeEvent& e) {
            m_Renderer->OnWindowResize(e.Width, e.Height);
            return false; // don't consume — camera also needs to know
        });

    // Initialize Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_Window->GetNativeHandle(), true);
    ImGui_ImplOpenGL3_Init("#version 460");

    ORB_CORE_INFO("Engine initialised");
}

Engine::~Engine()
{
    m_LayerStack.Clear();

    // Shutdown Dear ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_Renderer->Shutdown();
    ORB_CORE_INFO("Engine shut down");
}

// ─────────────────────────────────────────────────────────────────────────────
// Layer management
// ─────────────────────────────────────────────────────────────────────────────

Layer* Engine::PushLayer(std::unique_ptr<Layer> layer)
{
    return m_LayerStack.PushLayer(std::move(layer));
}

Layer* Engine::PushOverlay(std::unique_ptr<Layer> overlay)
{
    return m_LayerStack.PushOverlay(std::move(overlay));
}

// ─────────────────────────────────────────────────────────────────────────────
// Main loop
// ─────────────────────────────────────────────────────────────────────────────

void Engine::Run()
{
    m_Running = true;
    ORB_CORE_INFO("Entering main loop");

    while (m_Running && !m_Window->ShouldClose()) {
        // ── 1. Timing ─────────────────────────────────────────────────────────
        Time::Tick();
        const float dt      = Time::Delta();
        const float fixedDt = Time::Fixed();

        // ── 2. Input snapshot ─────────────────────────────────────────────────
        Input::Update(m_Window->GetNativeHandle());

        // ── 3. Poll OS events → fills EventBus queue ──────────────────────────
        m_Window->PollEvents();

        // ── 4. Dispatch queued events (top layer → bottom, consumed on true) ──
        m_EventBus.Dispatch();

        // ── 5. Camera events (pass events to camera controller) ───────────────
        // Camera manager subscribes its own events via EventBus in its constructor.
        // Per-frame update (not event-driven):
        m_CameraManager->OnUpdate(dt);

        // ── 6. Fixed-step accumulator ─────────────────────────────────────────
        m_FixedAccumulator += dt;
        while (m_FixedAccumulator >= fixedDt) {
            for (auto& layer : m_LayerStack) {
                if (layer->IsEnabled()) layer->OnFixedUpdate(fixedDt);
            }
            m_FixedAccumulator -= fixedDt;
        }

        // ── 7. Variable-step layer update (bottom → top) ──────────────────────
        for (auto& layer : m_LayerStack) {
            if (layer->IsEnabled()) layer->OnUpdate(dt);
        }

        // ── 8. Render ─────────────────────────────────────────────────────────
        auto& cam = *m_CameraManager->GetCamera();
        m_Renderer->BeginFrame(cam);

        for (auto& layer : m_LayerStack) {
            if (layer->IsEnabled()) layer->OnRender();
        }

        m_Renderer->EndFrame();

        // ── 9. ImGui ──────────────────────────────────────────────────────────
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        for (auto& layer : m_LayerStack) {
            if (layer->IsEnabled()) layer->OnImGui();
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // ── 10. Present ───────────────────────────────────────────────────────
        m_Window->SwapBuffers();
    }

    ORB_CORE_INFO("Main loop exited | Frames: {} | Uptime: {:.2f}s",
        Time::Frame(), Time::Elapsed());
}

} // namespace Orbital
