cmake_minimum_required(VERSION 3.25)

project(Orbital
    VERSION     1.0.0
    DESCRIPTION "Interactive Scientific Visualization Platform for Quantum Mechanics"
    LANGUAGES   CXX C
)

# ── Build configuration ───────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Runtime output to a predictable location so assets relative paths work
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# Default build type
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the build type." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "RelWithDebInfo" "Release")
    message(STATUS "[Orbital] Defaulting to Debug build")
endif()

message(STATUS "[Orbital] Version     : ${PROJECT_VERSION}")
message(STATUS "[Orbital] Build type  : ${CMAKE_BUILD_TYPE}")
message(STATUS "[Orbital] C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "[Orbital] Compiler    : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "[Orbital] Generator   : ${CMAKE_GENERATOR}")

# ── Custom module path ────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

# ── Fetch dependencies and set compiler options ───────────────────────────────
include(cmake/Dependencies.cmake)
include(cmake/CompilerOptions.cmake)

# ── Asset directory (available as compile definition) ─────────────────────────
set(ORBITAL_ASSET_DIR "${CMAKE_SOURCE_DIR}/assets"
    CACHE PATH "Path to Orbital runtime assets directory")
message(STATUS "[Orbital] Asset dir   : ${ORBITAL_ASSET_DIR}")

# ── Source tree ───────────────────────────────────────────────────────────────
add_subdirectory(src)

# ── Tests (optional) ──────────────────────────────────────────────────────────
option(ORBITAL_BUILD_TESTS "Build unit and integration tests" ON)
if(ORBITAL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
