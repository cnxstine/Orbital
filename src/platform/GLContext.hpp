#pragma once

/**
 * @file platform/GLContext.hpp
 * @brief OpenGL 4.6 context initialisation and debug callback setup.
 *
 * Responsibilities:
 *  - Load all GL function pointers via GLAD after context creation.
 *  - Install GL_KHR_debug callback in Debug builds.
 *  - Query and log driver/GPU information.
 *  - Provide utility for checking GL errors programmatically.
 *
 * Only this file and renderer/backend/*.cpp may include glad headers.
 * All other translation units see only GLM, GLuint typedefs via
 * renderer/backend/*.hpp (which forward-declare GL types where possible).
 */

#include <cstdint>
#include <string_view>

#ifndef APIENTRY
    #if defined(_WIN32)
        #define APIENTRY __stdcall
    #else
        #define APIENTRY
    #endif
    #define ORB_APIENTRY_DEFINED_LOCALLY
#endif

namespace Orbital {

class GLContext {
public:
    GLContext() = delete;

    /**
     * @brief Load GL function pointers and install debug callback.
     *        Must be called after glfwMakeContextCurrent().
     */
    static void Init();

    /**
     * @brief Check for pending OpenGL errors and log them.
     * @param location  Human-readable location string for the log message.
     * @return Number of errors found (0 on success).
     *
     * In Release builds (NDEBUG) this is a no-op that always returns 0.
     */
    static int CheckErrors(std::string_view location);

    // ── Queried at Init() ──────────────────────────────────────────────────────
    [[nodiscard]] static std::string_view GetVendor()   noexcept;
    [[nodiscard]] static std::string_view GetRenderer() noexcept;
    [[nodiscard]] static std::string_view GetVersion()  noexcept;

private:
    static void APIENTRY DebugCallback(
        uint32_t source, uint32_t type, uint32_t id,
        uint32_t severity, int32_t length,
        const char* message, const void* userParam);

    static std::string s_Vendor;
    static std::string s_Renderer;
    static std::string s_Version;
};

} // namespace Orbital

// ── Convenience macro (strips in Release) ─────────────────────────────────────
#ifndef NDEBUG
    #define ORB_GL_CHECK(loc) ::Orbital::GLContext::CheckErrors(loc)
#else
    #define ORB_GL_CHECK(loc) 0
#endif

#ifdef ORB_APIENTRY_DEFINED_LOCALLY
    #undef APIENTRY
    #undef ORB_APIENTRY_DEFINED_LOCALLY
#endif
