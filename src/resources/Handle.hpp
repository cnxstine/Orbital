#pragma once

/**
 * @file resources/Handle.hpp
 * @brief Typed, opaque, lightweight resource handle.
 *
 * Handle<T> is an 8-byte struct wrapping a UUID (uint64_t). It is:
 *  - Typed:        Handle<GLShader> is a distinct type from Handle<GLTexture>.
 *                  Mixing them is a compile error.
 *  - Non-owning:   ResourceManager owns the actual resource data.
 *  - Trivially copyable: safe to store in components, pass by value.
 *  - Hashable:     Can be used as std::unordered_map key.
 *  - Comparable:   Supports == / != and total ordering via id.
 *
 * Null handle: id == 0 (default). Use Handle<T>::Null() or default-construct.
 *
 * Example:
 *   Handle<GLShader> shader = rm.Load<GLShader>("mesh");
 *   GLShader* ptr = rm.Get(shader);   // nullptr if evicted
 */

#include <cstdint>
#include <functional>
#include <compare>

namespace Orbital {

template <typename T>
struct Handle {
    uint64_t id = 0; ///< UUID; 0 = invalid/null

    // ── Factories ─────────────────────────────────────────────────────────────
    [[nodiscard]] static constexpr Handle Null()            noexcept { return {}; }
    [[nodiscard]] static constexpr Handle FromId(uint64_t i) noexcept { return {i}; }

    // ── Observers ─────────────────────────────────────────────────────────────
    [[nodiscard]] constexpr bool IsValid()  const noexcept { return id != 0; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }

    // ── Comparison ────────────────────────────────────────────────────────────
    constexpr bool operator==(const Handle&) const noexcept = default;
    constexpr auto operator<=>(const Handle&) const noexcept = default;
};

} // namespace Orbital

// ── std::hash specialization ──────────────────────────────────────────────────
namespace std {

template <typename T>
struct hash<Orbital::Handle<T>> {
    [[nodiscard]] std::size_t operator()(const Orbital::Handle<T>& h) const noexcept {
        return std::hash<uint64_t>{}(h.id);
    }
};

} // namespace std
