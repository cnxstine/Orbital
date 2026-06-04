#include "renderer/backend/GLShader.hpp"

#include "core/Log.hpp"
#include "core/Assert.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <format>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string GLShader::AnnotateSource(std::string_view src)
{
    std::string result;
    result.reserve(src.size() + src.size() / 20); // ~5% overhead for line numbers

    int    lineNum = 1;
    size_t start   = 0;

    while (start < src.size()) {
        size_t end = src.find('\n', start);
        bool   last = (end == std::string_view::npos);
        size_t lineEnd = last ? src.size() : end;

        result += std::format("{:4d}: ", lineNum);
        result.append(src.data() + start, lineEnd - start);
        result += '\n';

        ++lineNum;
        start = last ? src.size() : end + 1;
    }
    return result;
}

uint32_t GLShader::CompileStage(uint32_t glType,
                                 std::string_view source,
                                 std::string_view label)
{
    const char* src = source.data();
    const GLint len = static_cast<GLint>(source.size());

    GLuint shader = glCreateShader(glType);
    glShaderSource(shader, 1, &src, &len);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        GLint logLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);

        std::vector<char> log(static_cast<size_t>(logLen));
        glGetShaderInfoLog(shader, logLen, nullptr, log.data());

        ORB_CORE_ERROR("Shader compile error [{}]:\n{}\n--- Source ---\n{}",
                       label,
                       std::string(log.data(), static_cast<size_t>(logLen > 0 ? logLen - 1 : 0)),
                       AnnotateSource(source));

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

GLShader::GLShader(std::string_view name,
                   std::string_view vertSrc,
                   std::string_view fragSrc)
    : m_Name(name)
{
    uint32_t vert = CompileStage(GL_VERTEX_SHADER,   vertSrc, std::string(name) + ".vert");
    uint32_t frag = CompileStage(GL_FRAGMENT_SHADER, fragSrc, std::string(name) + ".frag");

    if (!vert || !frag) {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        return;
    }

    m_ProgramID = glCreateProgram();
    glAttachShader(m_ProgramID, vert);
    glAttachShader(m_ProgramID, frag);
    glLinkProgram(m_ProgramID);

    GLint success = GL_FALSE;
    glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &success);

    if (!success) {
        GLint logLen = 0;
        glGetProgramiv(m_ProgramID, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(static_cast<size_t>(logLen));
        glGetProgramInfoLog(m_ProgramID, logLen, nullptr, log.data());
        ORB_CORE_ERROR("Shader link error [{}]:\n{}", name, std::string(log.data()));
        glDeleteProgram(m_ProgramID);
        m_ProgramID = 0;
    } else {
        ORB_CORE_TRACE("Shader '{}' linked (id={})", name, m_ProgramID);
    }

    glDetachShader(m_ProgramID, vert);
    glDetachShader(m_ProgramID, frag);
    glDeleteShader(vert);
    glDeleteShader(frag);
}

GLShader::GLShader(std::string_view name,
                   std::string_view vertSrc,
                   std::string_view fragSrc,
                   std::string_view geomSrc)
    : m_Name(name)
{
    uint32_t vert = CompileStage(GL_VERTEX_SHADER,   vertSrc, std::string(name) + ".vert");
    uint32_t frag = CompileStage(GL_FRAGMENT_SHADER, fragSrc, std::string(name) + ".frag");
    uint32_t geom = CompileStage(GL_GEOMETRY_SHADER, geomSrc, std::string(name) + ".geom");

    if (!vert || !frag || !geom) {
        if (vert) glDeleteShader(vert);
        if (frag) glDeleteShader(frag);
        if (geom) glDeleteShader(geom);
        return;
    }

    m_ProgramID = glCreateProgram();
    glAttachShader(m_ProgramID, vert);
    glAttachShader(m_ProgramID, frag);
    glAttachShader(m_ProgramID, geom);
    glLinkProgram(m_ProgramID);

    GLint success = GL_FALSE;
    glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &success);

    if (!success) {
        GLint logLen = 0;
        glGetProgramiv(m_ProgramID, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(static_cast<size_t>(logLen));
        glGetProgramInfoLog(m_ProgramID, logLen, nullptr, log.data());
        ORB_CORE_ERROR("Shader link error [{}]:\n{}", name, std::string(log.data()));
        glDeleteProgram(m_ProgramID);
        m_ProgramID = 0;
    } else {
        ORB_CORE_TRACE("Shader '{}' (geom) linked (id={})", name, m_ProgramID);
    }

    glDetachShader(m_ProgramID, vert);
    glDetachShader(m_ProgramID, frag);
    glDetachShader(m_ProgramID, geom);
    glDeleteShader(vert);
    glDeleteShader(frag);
    glDeleteShader(geom);
}

GLShader::~GLShader()
{
    if (m_ProgramID) {
        glDeleteProgram(m_ProgramID);
        m_ProgramID = 0;
    }
}

GLShader::GLShader(GLShader&& other) noexcept
    : m_Name(std::move(other.m_Name))
    , m_ProgramID(other.m_ProgramID)
    , m_UniformCache(std::move(other.m_UniformCache))
{
    other.m_ProgramID = 0;
}

GLShader& GLShader::operator=(GLShader&& other) noexcept
{
    if (this != &other) {
        if (m_ProgramID) glDeleteProgram(m_ProgramID);
        m_Name         = std::move(other.m_Name);
        m_ProgramID    = other.m_ProgramID;
        m_UniformCache = std::move(other.m_UniformCache);
        other.m_ProgramID = 0;
    }
    return *this;
}

// ─────────────────────────────────────────────────────────────────────────────
// Binding
// ─────────────────────────────────────────────────────────────────────────────

void GLShader::Bind() const noexcept
{
    glUseProgram(m_ProgramID);
}

void GLShader::Unbind() const noexcept
{
    glUseProgram(0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Uniform location cache
// ─────────────────────────────────────────────────────────────────────────────

int GLShader::GetUniformLocation(std::string_view name)
{
    auto it = m_UniformCache.find(std::string(name));
    if (it != m_UniformCache.end())
        return it->second;

    int loc = glGetUniformLocation(m_ProgramID, name.data());
    if (loc == -1) {
        ORB_CORE_WARN("Shader '{}': uniform '{}' not found (inactive or optimised out)", m_Name, name);
    }
    m_UniformCache.emplace(std::string(name), loc);
    return loc;
}

// ─────────────────────────────────────────────────────────────────────────────
// Uniform setters
// ─────────────────────────────────────────────────────────────────────────────

void GLShader::SetUniform(std::string_view name, bool value)
{
    glUniform1i(GetUniformLocation(name), static_cast<int>(value));
}

void GLShader::SetUniform(std::string_view name, int value)
{
    glUniform1i(GetUniformLocation(name), value);
}

void GLShader::SetUniform(std::string_view name, float value)
{
    glUniform1f(GetUniformLocation(name), value);
}

void GLShader::SetUniform(std::string_view name, const glm::vec2& value)
{
    glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void GLShader::SetUniform(std::string_view name, const glm::vec3& value)
{
    glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void GLShader::SetUniform(std::string_view name, const glm::vec4& value)
{
    glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
}

void GLShader::SetUniform(std::string_view name, const glm::mat3& value)
{
    glUniformMatrix3fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void GLShader::SetUniform(std::string_view name, const glm::mat4& value)
{
    glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

Result<GLShader> GLShader::FromFiles(std::string_view name,
                                      std::string_view vertPath,
                                      std::string_view fragPath)
{
    auto readFile = [](std::string_view path) -> std::optional<std::string> {
        std::ifstream file(std::string(path), std::ios::in | std::ios::binary);
        if (!file.is_open()) return std::nullopt;
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    };

    auto vertSrc = readFile(vertPath);
    if (!vertSrc)
        return OrbitalError::IO(std::string("Cannot read vertex shader: ") + std::string(vertPath));

    auto fragSrc = readFile(fragPath);
    if (!fragSrc)
        return OrbitalError::IO(std::string("Cannot read fragment shader: ") + std::string(fragPath));

    GLShader shader(name, *vertSrc, *fragSrc);
    if (!shader.IsValid())
        return OrbitalError::GL("Shader compile/link failed: " + std::string(name));

    return shader;
}

} // namespace Orbital
