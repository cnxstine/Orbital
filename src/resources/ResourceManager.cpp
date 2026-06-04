#include "resources/ResourceManager.hpp"
// ResourceManager is almost entirely template code in the .hpp.
// This .cpp exists to satisfy the CMake source list and to provide
// explicit instantiations if needed in the future.

// Loader<GLShader> specialization is in loaders/ShaderLoader.cpp
// which is compiled as part of orbital_resources.

namespace Orbital {
    // Intentionally empty — see ResourceManager.hpp for all implementation.
} // namespace Orbital
