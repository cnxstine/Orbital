# =============================================================================
# cmake/Dependencies.cmake
#
# Manages all third-party library acquisition.
# All libraries are pinned to specific release tags for reproducible builds.
#
# Dependency graph (build order):
#   spdlog  →  (no deps)
#   glfw    →  (no deps)
#   glm     →  (no deps)
#   json    →  (no deps)
#   glad2   →  (Python 3 required at configure time)
# =============================================================================

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ── spdlog 1.14.1 ─────────────────────────────────────────────────────────────
message(STATUS "[Deps] Fetching spdlog v1.14.1...")
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)
set(SPDLOG_BUILD_EXAMPLE  OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

# ── GLFW 3.4 ──────────────────────────────────────────────────────────────────
message(STATUS "[Deps] Fetching GLFW 3.4...")
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS        OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS       OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL           OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# ── GLM 1.0.1 ─────────────────────────────────────────────────────────────────
message(STATUS "[Deps] Fetching GLM 1.0.1...")
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)
set(GLM_ENABLE_CXX_20     ON  CACHE BOOL "" FORCE)
set(GLM_BUILD_TESTS       OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glm)

# ── nlohmann/json 3.11.3 ──────────────────────────────────────────────────────
message(STATUS "[Deps] Fetching nlohmann/json v3.11.3...")
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests         OFF CACHE BOOL "" FORCE)
set(JSON_Install            OFF CACHE BOOL "" FORCE)
set(JSON_MultipleHeaders    ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ── GLAD2 (OpenGL 4.6 Core Profile) ───────────────────────────────────────────
# Requirement: Python 3 must be available in PATH at CMake configure time.
# glad2 generates the GL loader source at configure time, then compiles it.
message(STATUS "[Deps] Fetching glad2 v2.0.6 (requires Python 3)...")
FetchContent_Declare(glad2
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v2.0.6
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  cmake
)
FetchContent_MakeAvailable(glad2)

# Generate the OpenGL 4.6 Core loader target.
# REPRODUCIBLE  → generates deterministic source (allows caching / CI caching)
# LOADER        → includes the function-pointer loading code (not just headers)
glad_add_library(glad_gl_46
    REPRODUCIBLE
    LOADER
    API gl:core=4.6
)

# ── Dear ImGui v1.90.4 ────────────────────────────────────────────────────────
message(STATUS "[Deps] Fetching Dear ImGui v1.90.4...")
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

