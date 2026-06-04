#include "resources/loaders/ShaderLoader.hpp"
#include "renderer/backend/GLShader.hpp"
#include "resources/ResourceManager.hpp"
#include "core/Log.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <filesystem>

namespace Orbital {

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

std::optional<std::string> ShaderLoader::ReadFile(std::string_view path)
{
    std::filesystem::path filePath(path);
    if (filePath.is_relative()) {
#ifdef ORBITAL_ASSET_DIR
        filePath = std::filesystem::path(ORBITAL_ASSET_DIR) / filePath;
#endif
    }

    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        ORB_CORE_ERROR("ShaderLoader: cannot open '{}' (resolved: '{}')", path, filePath.string());
        return std::nullopt;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

Result<std::unique_ptr<GLShader>>
ShaderLoader::Load(std::string_view vertPath, std::string_view fragPath)
{
    auto vertSrc = ReadFile(vertPath);
    if (!vertSrc)
        return OrbitalError::IO(std::string("Cannot read vertex shader: ") + std::string(vertPath));

    auto fragSrc = ReadFile(fragPath);
    if (!fragSrc)
        return OrbitalError::IO(std::string("Cannot read fragment shader: ") + std::string(fragPath));

    // Derive a display name from the vertex shader file basename
    std::string name(vertPath);
    if (auto pos = name.find_last_of("/\\"); pos != std::string::npos)
        name = name.substr(pos + 1);
    if (auto pos = name.rfind('.'); pos != std::string::npos)
        name = name.substr(0, pos);

    auto shader = std::make_unique<GLShader>(name, *vertSrc, *fragSrc);
    if (!shader->IsValid())
        return OrbitalError::GL("Shader compilation/link failed: " + std::string(name));

    return shader;
}

Result<std::unique_ptr<GLShader>>
ShaderLoader::Load(std::string_view vertPath,
                   std::string_view fragPath,
                   std::string_view geomPath)
{
    auto vertSrc = ReadFile(vertPath);
    if (!vertSrc)
        return OrbitalError::IO(std::string("Cannot read vertex shader: ") + std::string(vertPath));

    auto fragSrc = ReadFile(fragPath);
    if (!fragSrc)
        return OrbitalError::IO(std::string("Cannot read fragment shader: ") + std::string(fragPath));

    auto geomSrc = ReadFile(geomPath);
    if (!geomSrc)
        return OrbitalError::IO(std::string("Cannot read geometry shader: ") + std::string(geomPath));

    std::string name(vertPath);
    if (auto pos = name.find_last_of("/\\"); pos != std::string::npos)
        name = name.substr(pos + 1);
    if (auto pos = name.rfind('.'); pos != std::string::npos)
        name = name.substr(0, pos);

    auto shader = std::make_unique<GLShader>(name, *vertSrc, *fragSrc, *geomSrc);
    if (!shader->IsValid())
        return OrbitalError::GL("Shader compilation/link failed: " + std::string(name));

    return shader;
}

std::unique_ptr<GLShader> ShaderLoader::LoadFromBase(std::string_view basePath)
{
    std::string base(basePath);
    const std::string vertPath = base + ".vert";
    const std::string fragPath = base + ".frag";

    auto result = Load(vertPath, fragPath);
    if (!result) {
        ORB_CORE_ERROR("ShaderLoader::LoadFromBase: {}", result.error().message);
        return nullptr;
    }
    return std::move(result.value());
}

std::unique_ptr<GLShader> Loader<GLShader>::Load(std::string_view path) {
    return ShaderLoader::LoadFromBase(path);
}

} // namespace Orbital
