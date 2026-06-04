#pragma once

/**
 * @file renderer/backend/GLShader.hpp
 * @brief RAII wrapper around an OpenGL shader program.
 *
 * GLShader compiles GLSL sources, links a program, and caches
 * uniform locations so repeated SetUniform calls avoid repeated
 * glGetUniformLocation queries.
 *
 * All OpenGL calls are in GLShader.cpp — this header is safe to
 * include from non-GL translation units (it does not include glad/gl.h).
 *
 * Move-only: copy is disabled to enforce single ownership of the GL program object.
 *
 * Usage:
 *   auto res = GLShader::FromFiles("Mesh", "mesh.vert", "mesh.frag");
 *   if (!res) { ... }
 *   res->Bind();
 *   res->SetUniform("u_Model", model);
 */

#include "core/Error.hpp"
#include "core/Log.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────

class GLShader {
public:
    // ── Construction / destruction ─────────────────────────────────────────────

    /**
     * @brief Compile and link a vertex + fragment shader program.
     * @param name     Debug label for the program (used in log messages).
     * @param vertSrc  GLSL vertex shader source code.
     * @param fragSrc  GLSL fragment shader source code.
     */
    GLShader(std::string_view name,
             std::string_view vertSrc,
             std::string_view fragSrc);

    /**
     * @brief Compile and link a vertex + geometry + fragment shader program.
     * @param name     Debug label for the program.
     * @param vertSrc  GLSL vertex shader source code.
     * @param fragSrc  GLSL fragment shader source code.
     * @param geomSrc  GLSL geometry shader source code.
     */
    GLShader(std::string_view name,
             std::string_view vertSrc,
             std::string_view fragSrc,
             std::string_view geomSrc);

    ~GLShader();

    // Move-only
    GLShader(GLShader&& other) noexcept;
    GLShader& operator=(GLShader&& other) noexcept;

    GLShader(const GLShader&)            = delete;
    GLShader& operator=(const GLShader&) = delete;

    // ── State ─────────────────────────────────────────────────────────────────

    void Bind()   const noexcept;
    void Unbind() const noexcept;

    /// Returns true if the program compiled and linked successfully.
    [[nodiscard]] bool IsValid() const noexcept { return m_ProgramID != 0; }

    // ── Accessors ─────────────────────────────────────────────────────────────

    [[nodiscard]] std::string_view GetName()      const noexcept { return m_Name; }
    [[nodiscard]] uint32_t         GetProgramID() const noexcept { return m_ProgramID; }

    // ── Uniform setters ───────────────────────────────────────────────────────

    void SetUniform(std::string_view name, bool value);
    void SetUniform(std::string_view name, int  value);
    void SetUniform(std::string_view name, float value);
    void SetUniform(std::string_view name, const glm::vec2& value);
    void SetUniform(std::string_view name, const glm::vec3& value);
    void SetUniform(std::string_view name, const glm::vec4& value);
    void SetUniform(std::string_view name, const glm::mat3& value);
    void SetUniform(std::string_view name, const glm::mat4& value);

    /**
     * @brief Look up (and cache) a uniform location.
     * @return GL uniform location, or -1 if the uniform is not active.
     */
    [[nodiscard]] int GetUniformLocation(std::string_view name);

    // ── Factory ───────────────────────────────────────────────────────────────

    /**
     * @brief Load shader sources from disk and construct a GLShader.
     * @param name      Display name for the shader.
     * @param vertPath  Path to the .vert file.
     * @param fragPath  Path to the .frag file.
     * @return Result<GLShader> — error on IO failure or compile error.
     */
    [[nodiscard]] static Result<GLShader>
    FromFiles(std::string_view name,
              std::string_view vertPath,
              std::string_view fragPath);

private:
    /// Compile one shader stage. Returns 0 on failure (error logged).
    [[nodiscard]] static uint32_t CompileStage(uint32_t glType,
                                               std::string_view source,
                                               std::string_view label);

    /// Annotate source with line numbers for error readability.
    [[nodiscard]] static std::string AnnotateSource(std::string_view src);

    std::string  m_Name;
    uint32_t     m_ProgramID = 0;

    mutable std::unordered_map<std::string, int> m_UniformCache;
};

} // namespace Orbital
