#include "platform/Window.hpp"
#include "platform/GLContext.hpp"
#include "core/Assert.hpp"
#include "core/Log.hpp"
#include "events/events/WindowEvents.hpp"
#include "events/events/InputEvents.hpp"

#include <GLFW/glfw3.h>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ─────────────────────────────────────────────────────────────────────────────

Window::Window(const WindowSpec& spec, EventBus& bus)
    : m_Spec(spec), m_Bus(bus)
{
    ORB_CORE_INFO("Creating window '{}' ({}x{})", spec.Title, spec.Width, spec.Height);

    const int ok = glfwInit();
    ORB_ASSERT(ok == GLFW_TRUE, "GLFW initialisation failed");

    // Request OpenGL 4.6 Core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE,        GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    // Multi-sample anti-aliasing: 4× MSAA
    glfwWindowHint(GLFW_SAMPLES, 4);

    // 32-bit depth buffer (reversed-Z requires maximum precision)
    glfwWindowHint(GLFW_DEPTH_BITS, 32);

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    m_Handle = glfwCreateWindow(
        static_cast<int>(spec.Width),
        static_cast<int>(spec.Height),
        spec.Title.c_str(),
        nullptr, nullptr);

    ORB_ASSERT(m_Handle != nullptr, "GLFW window creation failed");

    // Store 'this' so static callbacks can reach the Window
    glfwSetWindowUserPointer(m_Handle, this);

    glfwMakeContextCurrent(m_Handle);
    SetVSync(spec.VSync);

    // Initialise OpenGL function pointers (GLAD)
    GLContext::Init();

    InstallCallbacks();

    ORB_CORE_INFO("Window created successfully");
}

Window::~Window()
{
    if (m_Handle) {
        glfwDestroyWindow(m_Handle);
        m_Handle = nullptr;
    }
    glfwTerminate();
    ORB_CORE_INFO("Window destroyed");
}

// ─────────────────────────────────────────────────────────────────────────────
// Frame interface
// ─────────────────────────────────────────────────────────────────────────────

void Window::PollEvents() const
{
    glfwPollEvents();
}

void Window::SwapBuffers() const
{
    glfwSwapBuffers(m_Handle);
}

bool Window::ShouldClose() const
{
    return glfwWindowShouldClose(m_Handle) != 0;
}

void Window::SetVSync(bool enabled)
{
    glfwSwapInterval(enabled ? 1 : 0);
    m_Spec.VSync = enabled;
}

// ─────────────────────────────────────────────────────────────────────────────
// Callback installation
// ─────────────────────────────────────────────────────────────────────────────

void Window::InstallCallbacks()
{
    glfwSetWindowCloseCallback  (m_Handle, CB_WindowClose);
    glfwSetWindowSizeCallback   (m_Handle, CB_WindowResize);
    glfwSetWindowFocusCallback  (m_Handle, CB_WindowFocus);
    glfwSetKeyCallback          (m_Handle, CB_KeyCallback);
    glfwSetCharCallback         (m_Handle, CB_CharCallback);
    glfwSetMouseButtonCallback  (m_Handle, CB_MouseButton);
    glfwSetCursorPosCallback    (m_Handle, CB_CursorPos);
    glfwSetScrollCallback       (m_Handle, CB_Scroll);
}

// ─────────────────────────────────────────────────────────────────────────────
// Static GLFW callbacks
// ─────────────────────────────────────────────────────────────────────────────

static Window& GetWindow(GLFWwindow* handle)
{
    return *static_cast<Window*>(glfwGetWindowUserPointer(handle));
}

void Window::CB_WindowClose(GLFWwindow* handle)
{
    auto& w = GetWindow(handle);
    w.m_Bus.Post(WindowCloseEvent{});
}

void Window::CB_WindowResize(GLFWwindow* handle, int width, int height)
{
    auto& w = GetWindow(handle);
    w.m_Spec.Width  = static_cast<uint32_t>(width);
    w.m_Spec.Height = static_cast<uint32_t>(height);
    w.m_Bus.Post(WindowResizeEvent{
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    });
}

void Window::CB_WindowFocus(GLFWwindow* handle, int focused)
{
    GetWindow(handle).m_Bus.Post(WindowFocusEvent{focused != 0});
}

void Window::CB_KeyCallback(GLFWwindow* handle, int key, int /*scancode*/, int action, int mods)
{
    auto& w = GetWindow(handle);
    switch (action) {
        case GLFW_PRESS:
            w.m_Bus.Post(KeyPressedEvent{key, mods, false});
            break;
        case GLFW_REPEAT:
            w.m_Bus.Post(KeyPressedEvent{key, mods, true});
            break;
        case GLFW_RELEASE:
            w.m_Bus.Post(KeyReleasedEvent{key, mods});
            break;
        default: break;
    }
}

void Window::CB_CharCallback(GLFWwindow* handle, unsigned int codepoint)
{
    GetWindow(handle).m_Bus.Post(KeyTypedEvent{codepoint});
}

void Window::CB_MouseButton(GLFWwindow* handle, int button, int action, int mods)
{
    auto& w = GetWindow(handle);
    if (action == GLFW_PRESS)
        w.m_Bus.Post(MouseButtonPressedEvent{button, mods});
    else
        w.m_Bus.Post(MouseButtonReleasedEvent{button, mods});
}

void Window::CB_CursorPos(GLFWwindow* handle, double x, double y)
{
    auto& w = GetWindow(handle);
    w.m_Bus.Post(MouseMovedEvent{
        static_cast<float>(x),
        static_cast<float>(y)
    });
}

void Window::CB_Scroll(GLFWwindow* handle, double ox, double oy)
{
    auto& w = GetWindow(handle);
    w.m_Bus.Post(MouseScrolledEvent{
        static_cast<float>(ox),
        static_cast<float>(oy)
    });
}

} // namespace Orbital
