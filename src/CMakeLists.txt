# =============================================================================
# src/CMakeLists.txt
#
# Defines all Orbital library targets and the main executable.
#
# Strict layer ordering enforced via explicit PRIVATE/PUBLIC link visibility:
#   Layer 0  →  orbital_core
#   Layer 1  →  orbital_events, orbital_resources
#   Layer 2  →  orbital_renderer, orbital_camera, orbital_scene
#   Executable → orbital
#
# A target at layer N must NEVER link a target at layer M > N.
# Violations are build errors thanks to explicit target_link_libraries.
# =============================================================================

# ── Dear ImGui Target ─────────────────────────────────────────────────────────
set(IMGUI_DIR "${imgui_SOURCE_DIR}")
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw glad_gl_46)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD2)

# ── Layer 0 : Core ────────────────────────────────────────────────────────────
# Provides: logging, error types, layer/stack, timing, engine bootstrap,
#           GLFW window, input state, OpenGL context creation.
add_library(orbital_core STATIC
    core/Log.cpp
    core/LayerStack.cpp
    core/Time.cpp
    core/Engine.cpp
    core/Application.cpp
    platform/Window.cpp
    platform/Input.cpp
    platform/GLContext.cpp
)

target_include_directories(orbital_core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(orbital_core
    PUBLIC
        orbital_compiler_options
        spdlog::spdlog
        glfw
        glad_gl_46
        glm::glm
        nlohmann_json::nlohmann_json
        imgui
)

target_compile_definitions(orbital_core
    PUBLIC
        ORBITAL_ASSET_DIR="${ORBITAL_ASSET_DIR}"
        # GLM configuration: column-major, depth clip [-1,1]→[0,1] via explicit call
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_ENABLE_EXPERIMENTAL
)

# ── Layer 1 : Events ──────────────────────────────────────────────────────────
# Provides: typed event bus, RAII subscription tokens, cross-thread posting.
add_library(orbital_events STATIC
    events/EventBus.cpp
)

target_include_directories(orbital_events
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(orbital_events
    PUBLIC orbital_core
)

# ── Layer 1 : Resources ───────────────────────────────────────────────────────
# Provides: Handle<T>, ResourceManager, ShaderLoader, hot-reload watcher.
add_library(orbital_resources STATIC
    resources/ResourceManager.cpp
    resources/loaders/ShaderLoader.cpp
)

target_include_directories(orbital_resources
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(orbital_resources
    PUBLIC orbital_events
)

# ── Layer 2 : Renderer ────────────────────────────────────────────────────────
# Provides: GL backend objects, RenderContext, CommandBuffer, Renderer facade.
# THIS is the ONLY target that #includes OpenGL headers.
add_library(orbital_renderer STATIC
    renderer/backend/GLShader.cpp
    renderer/backend/GLBuffer.cpp
    renderer/backend/GLVertexArray.cpp
    renderer/backend/GLTexture.cpp
    renderer/backend/GLFramebuffer.cpp
    renderer/RenderContext.cpp
    renderer/CommandBuffer.cpp
    renderer/Renderer.cpp
)

target_include_directories(orbital_renderer
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(orbital_renderer
    PUBLIC orbital_resources
)

# ── Layer 2 : Camera ──────────────────────────────────────────────────────────
# Provides: Camera abstraction, Arcball/Fly controllers, CameraManager.
add_library(orbital_camera STATIC
    camera/PerspectiveCamera.cpp
    camera/controllers/ArcballController.cpp
    camera/controllers/FlyController.cpp
    camera/CameraManager.cpp
)

target_include_directories(orbital_camera
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(orbital_camera
    PUBLIC orbital_renderer
)

# ── Layer 2 : Scene ───────────────────────────────────────────────────────────
# Provides: ECS Registry, Scene container, component systems.
add_library(orbital_scene STATIC
    scene/Scene.cpp
    scene/systems/RenderSystem.cpp
    scene/systems/TransformSystem.cpp
)

target_include_directories(orbital_scene
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(orbital_scene
    PUBLIC orbital_camera
)

# ── Layer 2 : Visualization ───────────────────────────────────────────────────
# Provides: Scientific datasets, wavefunctions, and scientific rendering facade.
add_library(orbital_visualization STATIC
    visualization/HydrogenicOrbital.cpp
    visualization/ScientificRenderer.cpp
    visualization/MonteCarloSamplingPipeline.cpp
    visualization/StochasticProbabilityCloud.cpp
    visualization/OrbitalViewerModule.cpp
    visualization/ModuleLayer.cpp
    visualization/MolecularOrbitalExplorerModule.cpp
    visualization/OverlapIntegral.cpp
    visualization/MolecularEnergyModel.cpp
    visualization/EnergySweep.cpp
    visualization/LCAOMolecularOrbital.cpp
    visualization/MolecularOrbitalFactory.cpp
    visualization/HybridOrbital.cpp
    visualization/HybridOrbitalExplorerModule.cpp
)

target_include_directories(orbital_visualization
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(orbital_visualization
    PUBLIC orbital_scene
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(orbital main.cpp)

target_link_libraries(orbital
    PRIVATE
        orbital_core
        orbital_events
        orbital_resources
        orbital_renderer
        orbital_camera
        orbital_scene
        orbital_visualization
)

# Copy assets alongside the executable after every build
add_custom_command(TARGET orbital POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:orbital>/assets"
    COMMENT "[Orbital] Copying assets → build directory"
    VERBATIM
)

message(STATUS "[Orbital] Configured executable target: orbital")
