#pragma once

/**
 * @file scene/Entity.hpp
 * @brief Lightweight entity identifier for the Orbital ECS.
 *
 * An Entity is nothing but a 64-bit ID. All data lives in component pools
 * inside the Registry (Scene). This keeps entities trivially copyable and
 * avoids pointer chasing.
 *
 * EntityID 0 is reserved as the null entity (same convention as UUID 0 in Handle<T>).
 */

#include <cstdint>
#include <functional>

namespace Orbital {

using EntityID = uint64_t;

struct Entity {
    EntityID id = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return id != 0; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }

    constexpr bool operator==(const Entity&) const noexcept = default;
    constexpr auto operator<=>(const Entity&) const noexcept = default;

    [[nodiscard]] static constexpr Entity Null() noexcept { return {0}; }
};

} // namespace Orbital

// std::hash for use in unordered_map<Entity, ...>
namespace std {
    template <>
    struct hash<Orbital::Entity> {
        [[nodiscard]] std::size_t operator()(const Orbital::Entity& e) const noexcept {
            return std::hash<Orbital::EntityID>{}(e.id);
        }
    };
} // namespace std
