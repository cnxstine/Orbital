#pragma once

/**
 * @file core/Error.hpp
 * @brief Lightweight result and error types for the Orbital engine.
 *
 * Avoids exceptions for performance-sensitive subsystem boundaries.
 * Uses a simple discriminated union approach compatible with C++20.
 *
 * Design rationale:
 *   - Exceptions are disabled for GPU/simulation code (non-deterministic latency)
 *   - std::expected is C++23; this provides an equivalent for C++20
 *   - Error propagates as a value, forcing call-site handling
 *
 * Usage:
 *   Result<GLShader> LoadShader(std::string_view path);
 *
 *   auto result = LoadShader("mesh.vert");
 *   if (!result) {
 *       ORB_CORE_ERROR("Shader error: {}", result.error().message);
 *       return;
 *   }
 *   result->Bind();
 */

#include <string>
#include <variant>
#include <stdexcept>
#include <utility>
#include <optional>

namespace Orbital {

/// Structured error returned by subsystem operations.
struct OrbitalError {
    enum class Category {
        None,
        IO,           ///< File not found, permission denied, etc.
        GL,           ///< OpenGL compile / link / runtime error
        Parse,        ///< JSON, GLSL preprocessor, scene file errors
        Resource,     ///< Missing asset, version mismatch
        Simulation,   ///< Numerics: divergence, eigenvalue failure
        Logic,        ///< Programmer error: invalid argument, bad state
    };

    Category    category = Category::None;
    std::string message;
    int         code     = 0; ///< Optional OS or GL error code

    OrbitalError() = default;
    OrbitalError(Category cat, std::string msg, int codeVal = 0)
        : category(cat), message(std::move(msg)), code(codeVal) {}

    /// Convenience factories
    static OrbitalError IO    (std::string msg) { return {Category::IO,         std::move(msg)}; }
    static OrbitalError GL    (std::string msg, int c = 0) { return {Category::GL, std::move(msg), c}; }
    static OrbitalError Parse (std::string msg) { return {Category::Parse,      std::move(msg)}; }
    static OrbitalError Res   (std::string msg) { return {Category::Resource,   std::move(msg)}; }
    static OrbitalError Logic (std::string msg) { return {Category::Logic,      std::move(msg)}; }
};

// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Discriminated union holding either a value T or an OrbitalError.
 *
 * Closely mirrors the C++23 std::expected interface so migration is trivial.
 */
template <typename T>
class Result {
public:
    // ── Construction ──────────────────────────────────────────────────────────
    Result(T value) : m_Data(std::move(value)) {}                     // NOLINT
    Result(OrbitalError err) : m_Data(std::move(err)) {}             // NOLINT

    // ── Observers ─────────────────────────────────────────────────────────────
    [[nodiscard]] bool has_value() const noexcept { return std::holds_alternative<T>(m_Data); }
    explicit operator bool()       const noexcept { return has_value(); }

    [[nodiscard]] T&         value()       { return std::get<T>(m_Data); }
    [[nodiscard]] const T&   value() const { return std::get<T>(m_Data); }
    [[nodiscard]] T*         operator->()       noexcept { return &std::get<T>(m_Data); }
    [[nodiscard]] const T*   operator->() const noexcept { return &std::get<T>(m_Data); }
    [[nodiscard]] T&         operator*()        { return std::get<T>(m_Data); }
    [[nodiscard]] const T&   operator*()  const { return std::get<T>(m_Data); }

    [[nodiscard]] const OrbitalError& error() const { return std::get<OrbitalError>(m_Data); }

    /// Returns value or throws std::runtime_error with the error message.
    T& value_or_throw() {
        if (!has_value()) throw std::runtime_error(std::get<OrbitalError>(m_Data).message);
        return std::get<T>(m_Data);
    }

private:
    std::variant<T, OrbitalError> m_Data;
};

/// Specialisation for void — just wraps success/failure.
template <>
class Result<void> {
public:
    Result()                   : m_Error(std::nullopt)       {}  // success
    Result(OrbitalError err)   : m_Error(std::move(err))     {}  // NOLINT

    [[nodiscard]] bool has_value()      const noexcept { return !m_Error.has_value(); }
    explicit operator bool()            const noexcept { return has_value(); }
    [[nodiscard]] const OrbitalError& error() const    { return *m_Error; }

private:
    std::optional<OrbitalError> m_Error;
};

} // namespace Orbital
