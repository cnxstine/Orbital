#include "platform/GLContext.hpp"
#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>  // Required for glfwGetProcAddress

namespace Orbital {

std::string GLContext::s_Vendor;
std::string GLContext::s_Renderer;
std::string GLContext::s_Version;

// ─────────────────────────────────────────────────────────────────────────────

void GLContext::Init()
{
    // Load function pointers; gladLoadGL returns a version struct
    const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress));
    ORB_ASSERT(version != 0, "GLAD failed to load OpenGL function pointers");

    // Query GPU info
    s_Vendor   = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    s_Renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    s_Version  = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    ORB_CORE_INFO("──────────────────────────────────────────");
    ORB_CORE_INFO("OpenGL context initialised");
    ORB_CORE_INFO("  Vendor   : {}", s_Vendor);
    ORB_CORE_INFO("  Renderer : {}", s_Renderer);
    ORB_CORE_INFO("  Version  : {}", s_Version);
    ORB_CORE_INFO("──────────────────────────────────────────");

    // Validate we have at least 4.6
    int major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    ORB_ASSERT(major > 4 || (major == 4 && minor >= 6),
               "OpenGL 4.6 or higher is required");

#ifndef NDEBUG
    // Install debug message callback
    int flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Callback on the calling thread
        glDebugMessageCallback(DebugCallback, nullptr);
        // Filter out low/notification severity to reduce noise
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE,
                              GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        ORB_CORE_INFO("GL_KHR_debug installed (synchronous)");
    } else {
        ORB_CORE_WARN("GL debug context was requested but GL_CONTEXT_FLAG_DEBUG_BIT is not set");
    }
#endif

    // Sensible global GL state (render passes may override locally)
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);    // LEQUAL — ready for reversed-Z when proj is adjusted
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

int GLContext::CheckErrors(std::string_view location)
{
#ifndef NDEBUG
    int count = 0;
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        ++count;
        const char* msg = nullptr;
        switch (err) {
            case GL_INVALID_ENUM:                  msg = "GL_INVALID_ENUM";      break;
            case GL_INVALID_VALUE:                 msg = "GL_INVALID_VALUE";     break;
            case GL_INVALID_OPERATION:             msg = "GL_INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                msg = "GL_STACK_OVERFLOW";    break;
            case GL_STACK_UNDERFLOW:               msg = "GL_STACK_UNDERFLOW";   break;
            case GL_OUT_OF_MEMORY:                 msg = "GL_OUT_OF_MEMORY";     break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: msg = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            default:                               msg = "GL_UNKNOWN_ERROR";     break;
        }
        ORB_CORE_ERROR("GL error at '{}': {} (0x{:04X})", location, msg, err);
    }
    return count;
#else
    return 0;
#endif
}

std::string_view GLContext::GetVendor()   noexcept { return s_Vendor;   }
std::string_view GLContext::GetRenderer() noexcept { return s_Renderer; }
std::string_view GLContext::GetVersion()  noexcept { return s_Version;  }

// ─────────────────────────────────────────────────────────────────────────────
// Debug callback
// ─────────────────────────────────────────────────────────────────────────────

void APIENTRY GLContext::DebugCallback(
    uint32_t source, uint32_t type, uint32_t id,
    uint32_t severity, int32_t /*length*/,
    const char* message, const void* /*userParam*/)
{
    // Skip known noise
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    const char* srcStr = [source]() -> const char* {
        switch (source) {
            case GL_DEBUG_SOURCE_API:             return "API";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "Window System";
            case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
            case GL_DEBUG_SOURCE_THIRD_PARTY:     return "Third Party";
            case GL_DEBUG_SOURCE_APPLICATION:     return "Application";
            default:                              return "Other";
        }
    }();

    const char* typeStr = [type]() -> const char* {
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:               return "Error";
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "Undefined Behaviour";
            case GL_DEBUG_TYPE_PORTABILITY:         return "Portability";
            case GL_DEBUG_TYPE_PERFORMANCE:         return "Performance";
            case GL_DEBUG_TYPE_MARKER:              return "Marker";
            default:                                return "Other";
        }
    }();

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            ORB_CORE_ERROR("[GL] [{}] [{}] (id={}): {}", srcStr, typeStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            ORB_CORE_WARN ("[GL] [{}] [{}] (id={}): {}", srcStr, typeStr, id, message);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            ORB_CORE_INFO ("[GL] [{}] [{}] (id={}): {}", srcStr, typeStr, id, message);
            break;
        default:
            break; // Notifications already filtered out above
    }
}

} // namespace Orbital
