#pragma once

/**
 * @file resources/loaders/ShaderLoader.hpp
 * @brief Utility for loading GLSL shader programs from the filesystem.
 *
 * ShaderLoader reads vertex and fragment (and optionally geometry) shader
 * source files from disk and constructs a GLShader object.
 *
 * It also provides the Loader<GLShader> specialization so that
 * ResourceManager::Load<GLShader>(path) works transparently —
 * the convention is that `path` is a base path (e.g. "shaders/geometry/mesh")
 * and the loader appends ".vert" / ".frag" automatically.
 *
 * Usage (direct):
 *   auto result = ShaderLoader::Load("shaders/mesh.vert", "shaders/mesh.frag");
 *   if (!result) { ORB_CORE_ERROR("{}", result.error().message); }
 *   auto& shader = result.value();
 *
 * Usage (via ResourceManager):
 *   Handle<GLShader> h = rm.Load<GLShader>("shaders/geometry/mesh");
 */

#include "core/Error.hpp"

#include <memory>
#include <string_view>

namespace Orbital {

// Forward declaration — no GL header needed here.
class GLShader;

// ─────────────────────────────────────────────────────────────────────────────

class ShaderLoader {
public:
    ShaderLoader() = delete;

    /**
     * @brief Load a shader program from explicit .vert/.frag paths.
     * @param vertPath  Absolute or ORBITAL_ASSET_DIR-relative path to vertex shader.
     * @param fragPath  Absolute or ORBITAL_ASSET_DIR-relative path to fragment shader.
     * @return Result containing a heap-allocated GLShader, or an IO/GL error.
     */
    [[nodiscard]] static Result<std::unique_ptr<GLShader>>
    Load(std::string_view vertPath, std::string_view fragPath);

    /**
     * @brief Load a shader program with an optional geometry stage.
     * @param vertPath  Path to vertex shader source.
     * @param fragPath  Path to fragment shader source.
     * @param geomPath  Path to geometry shader source.
     * @return Result containing a heap-allocated GLShader, or an IO/GL error.
     */
    [[nodiscard]] static Result<std::unique_ptr<GLShader>>
    Load(std::string_view vertPath, std::string_view fragPath, std::string_view geomPath);

    /**
     * @brief Load a shader by base path (appends .vert / .frag automatically).
     *        Used by ResourceManager::Load<GLShader>(basePath).
     * @param basePath  e.g. "shaders/geometry/mesh" → loads mesh.vert + mesh.frag
     * @return Heap-allocated GLShader, or nullptr on failure (ResourceManager convention).
     */
    [[nodiscard]] static std::unique_ptr<GLShader>
    LoadFromBase(std::string_view basePath);

private:
    /// Read entire file to string. Returns nullopt on failure.
    [[nodiscard]] static std::optional<std::string>
    ReadFile(std::string_view path);
};

// Forward declare Loader template to allow specialization
template <typename T>
struct Loader;

template <>
struct Loader<GLShader> {
    static std::unique_ptr<GLShader> Load(std::string_view path);
};

} // namespace Orbital
