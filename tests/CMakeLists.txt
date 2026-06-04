# =============================================================================
# tests/CMakeLists.txt
#
# Placeholder for unit and integration tests.
# Add test targets here as the codebase grows.
#
# Structure:
#   tests/unit/         → Pure logic tests (no GL context required)
#   tests/integration/  → Headless render tests (Mesa software rasterizer)
# =============================================================================

# Ensure test infrastructure is available

include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# ── Test targets ──────────────────────────────────────────────────────────────
add_executable(test_simulation unit/simulation/HydrogenOrbitalTests.cpp)
target_link_libraries(test_simulation PRIVATE orbital_visualization gtest_main)
gtest_discover_tests(test_simulation)

message(STATUS "[Orbital] Configured test target: test_simulation")
