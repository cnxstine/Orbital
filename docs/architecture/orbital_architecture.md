# Orbital — Architectural Design Document

> **Edition**: 1.0 — Initial Architecture  
> **Role**: Senior Software Architect / Graphics Engineer / C++ Technical Lead  
> **Stack**: C++20 · OpenGL 4.6 · GLFW · GLM · Eigen · Dear ImGui · CMake

---

## Table of Contents

1. [Philosophy & Governing Principles](#1-philosophy--governing-principles)
2. [Complete Folder Structure](#2-complete-folder-structure)
3. [Core Engine Architecture](#3-core-engine-architecture)
4. [Rendering Architecture](#4-rendering-architecture)
5. [Scene & Module System](#5-scene--module-system)
6. [Event System](#6-event-system)
7. [Resource Management System](#7-resource-management-system)
8. [Camera Architecture](#8-camera-architecture)
9. [UI Architecture](#9-ui-architecture)
10. [Simulation Architecture](#10-simulation-architecture)
11. [Dependency Relationships](#11-dependency-relationships)
12. [UML-Style Diagrams](#12-uml-style-diagrams)
13. [Build Pipeline](#13-build-pipeline)

---

## 1. Philosophy & Governing Principles

Before any folder or class, architecture starts with philosophy. Every structural decision below flows from these axioms.

### 1.1 The Five Axioms

| # | Axiom | Implication |
|---|-------|-------------|
| A1 | **Separation of Concerns** | Physics/math never touch GPU code. UI never touches simulation state directly. |
| A2 | **Inversion of Control** | High-level policy (Engine) depends on abstractions, not concrete GPU backends. |
| A3 | **Data-Oriented Thinking** | Hot paths (particle systems, wavefunction sampling) are cache-friendly arrays, not pointer-chasing object trees. |
| A4 | **Additive Growth** | Adding a new quantum module (e.g., molecular orbitals) adds files, never modifies existing ones (Open/Closed). |
| A5 | **Fail Fast, Trace Deep** | Every subsystem has structured logging, assertions, and typed error returns — never silent corruption. |

### 1.2 Why These Choices Matter for Orbital

Quantum visualization is numerically demanding (Eigen solvers, wavefunction sampling on grids), GPU demanding (volumetric ray-marching, isosurface extraction), and pedagogically structured (lessons, modules, progression). These three axes — compute, render, content — must be completely independent layers that communicate only through well-defined interfaces.

---

## 2. Complete Folder Structure

```
Orbital/
├── CMakeLists.txt                    # Root build entry
├── cmake/
│   ├── CompilerOptions.cmake         # C++20 flags, warnings, sanitizers
│   ├── Dependencies.cmake            # FetchContent / vcpkg integration
│   ├── Packaging.cmake               # CPack rules
│   └── Modules/
│       └── FindEigen3.cmake          # Custom finders for non-CMake libs
│
├── docs/
│   ├── architecture/                 # This document + diagrams
│   ├── api/                          # Doxygen config + generated output
│   └── content/                      # Scientific reference material
│
├── assets/
│   ├── fonts/
│   ├── icons/
│   ├── textures/
│   │   ├── luts/                     # Color transfer function LUTs
│   │   └── noise/
│   └── shaders/                      # GLSL source (loaded at runtime)
│       ├── common/                   # #include-able GLSL headers
│       ├── compute/                  # Compute shaders
│       ├── geometry/                 # Geometry pass shaders
│       ├── post/                     # Post-processing shaders
│       └── ui/                       # ImGui / overlay shaders
│
├── src/
│   │
│   ├── core/                         # ── LAYER 0: Platform & Primitives ──
│   │   ├── Application.hpp/cpp       # Bootstrap: owns Engine, main loop
│   │   ├── Engine.hpp/cpp            # Orchestrator of all subsystems
│   │   ├── Layer.hpp                 # Abstract layer (scene, UI, debug)
│   │   ├── LayerStack.hpp/cpp        # Ordered list of active layers
│   │   ├── Window.hpp/cpp            # GLFW wrapper, context creation
│   │   ├── Time.hpp/cpp              # Delta-time, fixed-step accumulator
│   │   ├── Log.hpp/cpp               # Structured logging (spdlog wrapper)
│   │   ├── Assert.hpp                # Conditional abort + log macros
│   │   ├── Error.hpp                 # Result<T,E> and OrbitalError types
│   │   └── UUID.hpp/cpp              # Fast 64-bit entity/asset IDs
│   │
│   ├── platform/                     # ── LAYER 0: OS Abstractions ──
│   │   ├── FileSystem.hpp/cpp        # Path resolution, hot-reload watch
│   │   ├── Input.hpp/cpp             # GLFW input state + mapping
│   │   └── GLContext.hpp/cpp         # OpenGL loader (GLAD), debug CB
│   │
│   ├── events/                       # ── LAYER 1: Event Bus ──
│   │   ├── Event.hpp                 # Base event + EventType enum
│   │   ├── EventBus.hpp/cpp          # Lock-free multi-queue dispatcher
│   │   ├── EventQueue.hpp            # Per-category ring buffer
│   │   └── events/
│   │       ├── WindowEvents.hpp      # Resize, close, focus
│   │       ├── InputEvents.hpp       # Key, mouse button, scroll, cursor
│   │       ├── SimulationEvents.hpp  # StateChanged, StepCompleted
│   │       └── SceneEvents.hpp       # ModuleLoaded, CameraChanged
│   │
│   ├── resources/                    # ── LAYER 1: Asset Pipeline ──
│   │   ├── ResourceManager.hpp/cpp   # Central cache + lifetime ownership
│   │   ├── Handle.hpp                # Typed, ref-counted handle<T>
│   │   ├── loaders/
│   │   │   ├── ShaderLoader.hpp/cpp
│   │   │   ├── TextureLoader.hpp/cpp
│   │   │   └── MeshLoader.hpp/cpp
│   │   └── hot_reload/
│   │       ├── HotReloadWatcher.hpp/cpp   # inotify / ReadDirectoryChanges
│   │       └── ReloadableShader.hpp/cpp
│   │
│   ├── renderer/                     # ── LAYER 2: Rendering Engine ──
│   │   ├── Renderer.hpp/cpp          # High-level render API (submit calls)
│   │   ├── RenderGraph.hpp/cpp       # Frame-graph: nodes, edges, passes
│   │   ├── RenderPass.hpp            # Abstract pass (records draw calls)
│   │   ├── RenderContext.hpp/cpp     # Per-frame transient state
│   │   ├── CommandBuffer.hpp/cpp     # Deferred draw-call recorder
│   │   │
│   │   ├── backend/                  # OpenGL HAL (could swap to Vulkan)
│   │   │   ├── GLBuffer.hpp/cpp
│   │   │   ├── GLTexture.hpp/cpp
│   │   │   ├── GLFramebuffer.hpp/cpp
│   │   │   ├── GLShader.hpp/cpp
│   │   │   ├── GLVertexArray.hpp/cpp
│   │   │   └── GLComputeShader.hpp/cpp
│   │   │
│   │   ├── passes/                   # Concrete render passes
│   │   │   ├── GeometryPass.hpp/cpp
│   │   │   ├── VolumetricPass.hpp/cpp     # Wavefunction density ray-march
│   │   │   ├── IsosurfacePass.hpp/cpp     # Marching Cubes via compute
│   │   │   ├── BondPass.hpp/cpp           # Cylinder-impostor bonds
│   │   │   ├── AtomPass.hpp/cpp           # Sphere-impostor atoms
│   │   │   ├── LatticePass.hpp/cpp        # Instanced crystal cells
│   │   │   ├── PostProcessPass.hpp/cpp    # Bloom, SSAO, tonemapping
│   │   │   └── UIPass.hpp/cpp             # ImGui composite
│   │   │
│   │   ├── techniques/               # Reusable GPU algorithms
│   │   │   ├── MarchingCubes.hpp/cpp
│   │   │   ├── VolumeRayMarcher.hpp/cpp
│   │   │   ├── SphericalHarmonicBaker.hpp/cpp
│   │   │   └── SSAOKernel.hpp/cpp
│   │   │
│   │   └── debug/
│   │       ├── DebugRenderer.hpp/cpp      # Immediate-mode lines/shapes
│   │       └── GPUProfiler.hpp/cpp        # GL_TIME_ELAPSED queries
│   │
│   ├── scene/                        # ── LAYER 2: Scene Graph ──
│   │   ├── Scene.hpp/cpp             # World container (ECS root)
│   │   ├── Entity.hpp                # Thin ECS entity wrapper
│   │   ├── Registry.hpp/cpp          # EnTT-style component storage
│   │   ├── SceneSerializer.hpp/cpp   # JSON scene save/load
│   │   ├── components/               # Plain aggregate structs (POD-friendly)
│   │   │   ├── TransformComponent.hpp
│   │   │   ├── MeshComponent.hpp
│   │   │   ├── MaterialComponent.hpp
│   │   │   ├── LightComponent.hpp
│   │   │   ├── CameraComponent.hpp
│   │   │   ├── AtomComponent.hpp
│   │   │   ├── BondComponent.hpp
│   │   │   ├── OrbitalComponent.hpp
│   │   │   └── LatticeComponent.hpp
│   │   └── systems/                  # Logic that operates on components
│   │       ├── RenderSystem.hpp/cpp
│   │       ├── TransformSystem.hpp/cpp
`│       └── PhysicsProxySystem.hpp/cpp
│
├── camera/                       # ── LAYER 2: Camera System ──
│   ├── Camera.hpp                # Abstract: ViewMatrix, ProjMatrix
│   ├── CameraController.hpp      # Abstract: ProcessInput(dt)
│   ├── PerspectiveCamera.hpp/cpp
│   ├── OrthographicCamera.hpp/cpp
│   ├── controllers/
│   │   ├── ArcballController.hpp/cpp   # Orbital rotation (primary)
│   │   ├── FlyController.hpp/cpp       # Free-fly for exploration
│   │   └── CinematicController.hpp/cpp # Keyframe path animation
│   └── CameraManager.hpp/cpp           # Active camera + stack
│
├── simulation/                   # ── LAYER 3: Physics & Math ──
│   ├── SimulationEngine.hpp/cpp  # Tick scheduler, fixed-step loop
│   ├── SimulationState.hpp       # Immutable snapshot for renderer
│   │
│   ├── quantum/
│   │   ├── QuantumNumbers.hpp    # n, l, m, s value types
│   │   ├── HydrogenicOrbital.hpp/cpp    # Analytic ψ(r,θ,φ)
│   │   ├── WavefunctionSampler.hpp/cpp  # Grid sampling on CPU/GPU
│   │   ├── SpinState.hpp/cpp            # Bloch sphere representation
│   │   ├── OperatorAlgebra.hpp/cpp      # Lx, Ly, Lz, H operators
│   │   └── PerturbationTheory.hpp/cpp   # First/second order PT
│   │
│   ├── molecules/
│   │   ├── Molecule.hpp/cpp             # Atom graph + bonds
│   │   ├── LCAO.hpp/cpp                 # Linear combination AOs
│   │   ├── HuckelSolver.hpp/cpp         # Hückel tight-binding (Eigen)
│   │   └── MODiagram.hpp/cpp            # MO energy level diagram
│   │
│   ├── lattice/
│   │   ├── CrystalLattice.hpp/cpp       # Bravais + basis
│   │   ├── BrillouinZone.hpp/cpp        # Reciprocal space
│   │   ├── BandStructure.hpp/cpp        # k-path E(k) solver
│   │   └── PhononDispersion.hpp/cpp     # Dynamical matrix
│   │
│   └── solvers/
│       ├── EigenSolver.hpp/cpp          # Thin Eigen wrapper
│       ├── NumerovIntegrator.hpp/cpp    # Radial Schrödinger eq.
│       └── FiniteDifference.hpp/cpp     # 1D/2D/3D FD grid
│
├── modules/                      # ── LAYER 4: Content Modules ──
│   ├── IModule.hpp               # Abstract module interface
│   ├── ModuleRegistry.hpp/cpp    # Dynamic registration + discovery
│   ├── ModuleContext.hpp         # Services injected into modules
│   │
│   ├── AtomExplorer/
│   │   ├── AtomExplorerModule.hpp/cpp
│   │   ├── AtomExplorerUI.hpp/cpp
│   │   └── AtomExplorerScene.hpp/cpp
│   │
│   ├── OrbitalViewer/
│   │   ├── OrbitalViewerModule.hpp/cpp
│   │   ├── OrbitalViewerUI.hpp/cpp
│   │   └── OrbitalViewerScene.hpp/cpp
│   │
│   ├── MoleculeBuilder/
│   │   ├── MoleculeBuilderModule.hpp/cpp
│   │   ├── MoleculeBuilderUI.hpp/cpp
│   │   └── MoleculeBuilderScene.hpp/cpp
│   │
│   ├── CrystalLatticeViewer/
│   │   ├── CrystalLatticeModule.hpp/cpp
│   │   ├── CrystalLatticeUI.hpp/cpp
│   │   └── CrystalLatticeScene.hpp/cpp
│   │
│   └── BandStructureViewer/
│       ├── BandStructureModule.hpp/cpp
│       ├── BandStructureUI.hpp/cpp
│       └── BandStructureScene.hpp/cpp
│
└── ui/                           # ── LAYER 4: UI Framework ──
    ├── UIManager.hpp/cpp         # ImGui init, font atlas, theme
    ├── Theme.hpp/cpp             # Color palette, spacing tokens
    ├── Widgets.hpp/cpp           # Custom ImGui widgets
    ├── panels/
    │   ├── MainMenuBar.hpp/cpp
    │   ├── ModulePanel.hpp/cpp   # Module selector sidebar
    │   ├── PropertiesPanel.hpp/cpp
    │   ├── TimelinePanel.hpp/cpp
    │   ├── ConsolePanel.hpp/cpp
    │   └── ViewportPanel.hpp/cpp # Owns the framebuffer texture
    └── overlays/
        ├── QuantumNumberOverlay.hpp/cpp
        ├── EnergyLevelOverlay.hpp/cpp
        └── PerformanceOverlay.hpp/cpp
```

---

## 3. Core Engine Architecture

### 3.1 Bootstrap Sequence

```
main()
  └─► Application::Run()
        ├─► Window::Create()        (GLFW + GL context)
        ├─► Engine::Initialize()
        │     ├─► Log::Init()
        │     ├─► EventBus::Init()
        │     ├─► ResourceManager::Init()
        │     ├─► Renderer::Init()
        │     ├─► SimulationEngine::Init()
        │     ├─► UIManager::Init()
        │     └─► ModuleRegistry::Init()
        │           └─► [Register all built-in modules]
        └─► Application::MainLoop()
              ├─► Time::Tick()
              ├─► EventBus::Dispatch()
              ├─► LayerStack::OnUpdate(dt)
              ├─► SimulationEngine::Step(fixedDt)  [fixed-step if dirty]
              ├─► Renderer::BeginFrame()
              │     └─► RenderGraph::Execute()
              ├─► UIManager::Render()
              └─► Window::SwapBuffers()
```

### 3.2 Engine Class Responsibilities

```
Engine
  ├── owns: Window, EventBus, ResourceManager
  ├── owns: Renderer, SimulationEngine, UIManager
  ├── owns: LayerStack
  └── provides: ServiceLocator::Get<T>()  ← subsystems register here
```

**ServiceLocator** is a compile-time typed registry. Subsystems register themselves during `Init()`. Layers retrieve services by type. This avoids global singletons while keeping access ergonomic. It is NOT a general DI container — only subsystem-level objects are registered.

### 3.3 LayerStack

Layers are the primary extension point for the main loop. They are ordered; input events propagate top-to-bottom and are consumed once handled.

| Layer (top → bottom) | Responsibility |
|---|---|
| `DebugLayer` | Dev overlays, profiler, console |
| `UILayer` | ImGui panels, all UI interaction |
| `ModuleLayer` | Active module update + input |
| `SceneLayer` | Entity/component tick, camera |
| `SimulationLayer` | Async simulation result polling |

### 3.4 Time System

- `Time::Delta()` — variable frame delta for animations/camera
- `Time::Fixed()` — fixed timestep for simulation (default 16.667ms)
- `Time::Accumulator` — leftover time carried to next frame (standard game loop pattern)
- All simulation code receives `fixedDt` only, ensuring determinism

---

## 4. Rendering Architecture

### 4.1 Design Philosophy

The renderer is a **Frame Graph** (inspired by Frostbite's design). Each render pass declares its read/write resources. The graph validates dependencies at startup, sorts passes topologically, and culls unused passes. This enables:
- Automatic resource lifetime management (transient framebuffers)
- Pass reordering for optimal texture cache usage
- Zero-cost disabling of passes (e.g., SSAO off → pass simply culled)

### 4.2 Frame Graph Structure

```
RenderGraph
  ├── PassNode: GeometryPass
  │     writes: [GBuffer_Albedo, GBuffer_Normal, GBuffer_Depth]
  │     reads:  [SceneUBO, MaterialSSBO]
  │
  ├── PassNode: VolumetricPass       ← wavefunction density
  │     writes: [VolumeColor, VolumeAlpha]
  │     reads:  [GBuffer_Depth, WavefunctionTexture3D, CameraUBO]
  │
  ├── PassNode: IsosurfacePass       ← marching cubes (compute)
  │     writes: [IsosurfaceVBO]      (persistent, not a framebuffer)
  │     reads:  [WavefunctionTexture3D]
  │
  ├── PassNode: LightingPass         ← deferred PBR
  │     writes: [HdrColor]
  │     reads:  [GBuffer_*, VolumeColor, VolumeAlpha, SSAO]
  │
  ├── PassNode: SSAOPass
  │     writes: [SSAO]
  │     reads:  [GBuffer_Normal, GBuffer_Depth, NoiseTexture]
  │
  ├── PassNode: PostProcessPass
  │     writes: [FinalColor]
  │     reads:  [HdrColor]
  │
  └── PassNode: UIPass
        writes: [SwapChain]
        reads:  [FinalColor]
```

### 4.3 Deferred Rendering for Orbital

Orbital uses a **deferred rendering** path for the main 3D scene:

- **GBuffer layout**:

| Attachment | Format | Contents |
|---|---|---|
| Albedo | RGBA8 | Base color + material ID |
| Normal | RGB16F | World-space normals |
| Material | RGBA8 | Metallic, roughness, AO, emission |
| Depth | D32F | Hardware depth |

### 4.4 Volumetric Wavefunction Rendering

This is the most technically demanding pass.

```
WavefunctionTexture3D (R32F, 128³ → 512³)
         │
         ▼
  VolumetricPass (fragment shader)
  ├── Ray-marching: front-to-back alpha compositing
  ├── Phase-coded color: Re(ψ) → hue, |ψ|² → opacity
  ├── Transfer function: 1D LUT texture (artist-controlled)
  ├── Early-out: empty-space skipping via min-max mipmap
  └── Adaptive step size: coarser far from isosurface
```

The 3D texture is updated by `WavefunctionSampler` on a compute shader or CPU thread, then uploaded once per simulation step change.

### 4.5 Sphere Impostor Atoms

Real atoms are rendered as screen-space impostors — one quad per atom, reconstructing a perfect sphere in the fragment shader. This gives:
- Pixel-perfect silhouettes at any zoom level
- Correct depth buffer output (enables SSAO)
- ~10× less geometry than tessellated spheres
- Trivially instanced via `gl_InstanceID` + SSBO

### 4.6 Backend Abstraction

All raw OpenGL types (`GLuint`, etc.) are confined to `renderer/backend/`. The rest of the renderer sees only:

```
Handle<GLBuffer>    ← typed, non-owning reference
Handle<GLTexture>
Handle<GLShader>
```

This single boundary is the seam for a future Vulkan/Metal port.

---

## 5. Scene & Module System

### 5.1 Entity-Component System (ECS)

Orbital uses a **sparse-set ECS** (EnTT-style, or a lightweight custom implementation). Key design points:

- **Components are plain data** — no virtual functions, no logic
- **Systems are free functions / functors** — operate on component views
- **No inheritance hierarchy** on entities — composition only

```
Scene
  └── Registry
        ├── Entity 0: [Transform, Atom, Material]
        ├── Entity 1: [Transform, Atom, Material]
        ├── Entity 2: [Transform, Bond]
        ├── Entity 3: [Transform, OrbitalField]  ← wavefunction volume
        └── Entity 4: [Transform, Camera]
```

Systems iterate views: `registry.view<Transform, Atom>()` — cache-friendly iteration over packed arrays.

### 5.2 Module System

A **Module** is the content unit. It owns:
- A `Scene` instance
- Its own UI panels (registered into UIManager)
- Its own simulation parameters
- Its own camera preset

```
IModule
  ├── OnAttach(ModuleContext&)      ← called when user switches to module
  ├── OnDetach()                    ← cleanup, serialize state
  ├── OnUpdate(dt)                  ← per-frame logic
  ├── OnSimulationStep()            ← simulation result consumed here
  ├── OnRenderSubmit(Renderer&)     ← submit draw calls to render graph
  └── OnImGui()                     ← render module-specific UI

ModuleContext  (injected services)
  ├── EventBus&
  ├── ResourceManager&
  ├── SimulationEngine&
  └── Renderer&
```

**ModuleRegistry** maintains a map of `{string → ModuleFactory}`. Modules self-register via a macro at static init time:

```
ORBITAL_REGISTER_MODULE(OrbitalViewerModule, "Orbital Viewer")
```

This means adding a new module requires zero changes to engine code.

### 5.3 Scene Serialization

Scenes serialize to JSON (nlohmann::json). Each component type provides:
- `Serialize(entity, json&)` — write fields
- `Deserialize(json&, Registry&)` — reconstruct entity

Scene files are stored in `assets/scenes/`. The `scene_validator` tool (in `tools/`) validates schemas before runtime loading.

---

## 6. Event System

### 6.1 Design: Typed, Queued, Multi-Category

Orbital's event system is **NOT** a simple callback bus. It is a **lock-free, category-partitioned ring-buffer queue** with typed dispatch.

**Rationale**: Quantum simulations run on a background thread. Events must safely cross thread boundaries. A naive `std::function` callback list would require locks everywhere or risk data races.

### 6.2 Event Categories

```
EventCategory (bit flags for filtering)
  ├── Input        (0x01)
  ├── Window       (0x02)
  ├── Simulation   (0x04)
  ├── Scene        (0x08)
  └── UI           (0x10)
```

### 6.3 Event Dispatch Flow

```
Producer Thread(s)
    │  EventBus::Post(event)  → enqueues to per-category lock-free queue
    │
Main Thread (EventBus::Dispatch() — called once per frame)
    │  Drains all queues
    │  For each event:
    └──► LayerStack::OnEvent(event)
              │
              ▼  (top → bottom, consumed when handled)
         UILayer::OnEvent()
         ModuleLayer::OnEvent()
         SceneLayer::OnEvent()
```

### 6.4 Event Type System

Events are plain structs inheriting a base:

```
Event
├── WindowResizeEvent    { uint32_t width, height }
├── KeyPressedEvent      { KeyCode key, int mods }
├── MouseScrollEvent     { float dx, float dy }
├── SimulationStepEvent  { uint64_t stepId, SimulationState snapshot }
└── ModuleChangedEvent   { std::string moduleName }
```

`SimulationStepEvent` carries an **immutable snapshot** of simulation state — the renderer never reads mutable simulation data. This is the critical thread-safety boundary.

### 6.5 Subscription Model

Layers subscribe using typed handlers:

```
EventBus::Subscribe<SimulationStepEvent>(handler, EventCategory::Simulation)
```

The subscription returns a `SubscriptionToken` (RAII — auto-unsubscribes on destruction).

---

## 7. Resource Management System

### 7.1 Central Cache with Typed Handles

```
ResourceManager
  ├── Cache<Shader>       : std::unordered_map<UUID, Shader>
  ├── Cache<Texture>      : std::unordered_map<UUID, Texture>
  ├── Cache<Mesh>         : std::unordered_map<UUID, Mesh>
  └── Cache<ComputeData>  : std::unordered_map<UUID, ComputeData>
```

All resources are accessed via `Handle<T>` — a lightweight struct holding a `UUID`. The handle is:
- **Typed** — `Handle<Shader>` cannot be used where `Handle<Texture>` is expected
- **Non-owning** — callers hold handles; ResourceManager owns the data
- **Ref-counted** — resources unload when handle count reaches zero

### 7.2 Resource Loading

```
ResourceManager::Load<Shader>("shaders/volumetric.glsl")
  │
  ├── Check cache (by path hash → UUID)
  ├── If miss: ShaderLoader::Load(path)
  │              ├── ShaderPreprocessor: resolve #include directives
  │              ├── Compile vertex + fragment/compute stages
  │              ├── Link program
  │              └── Reflect uniforms → UniformMap
  ├── Store in Cache<Shader>
  └── Return Handle<Shader>
```

### 7.3 Hot Reload System

Shader hot-reload is essential for iterative development of wavefunction visualizations.

```
HotReloadWatcher
  ├── Monitors: assets/shaders/** (OS file-change API)
  ├── On change: post ShaderChangedEvent to EventBus
  └── Renderer receives event → recompile shader → swap in RenderGraph
```

Hot reload is **opt-in per resource type** and disabled in release builds.

### 7.4 Resource Lifetime Tiers

| Tier | Lifetime | Examples |
|------|----------|---------|
| **Permanent** | App lifetime | Core shaders, UI fonts |
| **Module** | Module attach/detach | Module-specific textures, meshes |
| **Frame** | Single frame | Transient framebuffers (Frame Graph) |
| **Compute** | One simulation step | Temporary wavefunction grids |

ResourceManager tracks tier membership and bulk-releases on tier transitions.

---

## 8. Camera Architecture

### 8.1 Design: Separated Data & Controller

```
Camera  (data: view/proj matrices, frustum)
    └── is controlled by ─► CameraController  (behavior: input → transform)
```

The camera is a **component** on a scene entity (`CameraComponent`). The `CameraManager` holds the active camera entity and routes input to the active controller.

This separation means:
- The renderer reads only the Camera data (no controller dependency)
- Controllers can be hot-swapped without reinitializing the camera
- Cinematic paths animate the Camera directly, bypassing controllers

### 8.2 Camera Types

```
PerspectiveCamera
  ├── fov, near, far, aspect
  ├── ViewMatrix:  glm::lookAt(eye, target, up)
  └── ProjMatrix:  glm::perspective(fov, aspect, near, far)
      (Reversed-Z for maximum depth precision)

OrthographicCamera
  ├── width, height, near, far
  └── Used for: energy level diagrams, Brillouin zone 2D views
```

> **Reversed-Z**: Orbital visualizes objects ranging from atomic scales (sub-Ångström) to crystal unit cells (nanometers). Reversed-Z projection distributes floating-point depth precision where it matters (near plane), eliminating z-fighting across 6+ orders of magnitude of scale.

### 8.3 Controller Hierarchy

```
CameraController (abstract)
  ├── ArcballController
  │     ├── Pivot: scene centroid (auto-computed from selection)
  │     ├── Left-drag: rotate around pivot
  │     ├── Right-drag: pan
  │     ├── Scroll: dolly (true depth approach, not FOV change)
  │     └── Double-click: animate to selected entity
  │
  ├── FlyController
  │     ├── WASD + QE: 6DOF movement
  │     ├── Shift: speed boost
  │     └── Mouse: look direction
  │
  └── CinematicController
        ├── Keyframe list: [(time, position, target, fov)]
        ├── Hermite spline interpolation
        └── Used for: tutorial fly-throughs, auto-demo mode
```

### 8.4 Camera UBO

```glsl
layout(std140, binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
    mat4 viewProj;
    mat4 invViewProj;
    vec3 eye;
    float near;
    vec3 forward;
    float far;
    vec2 resolution;
    float time;
    float padding;
};
```

Bound once per frame. All shaders read from this UBO — no per-draw camera uniform uploads.

---

## 9. UI Architecture

### 9.1 ImGui Integration Strategy

ImGui is used as the **entire UI layer** — no additional widget toolkit. This is intentional:
- Zero styling conflicts with OS themes
- Per-frame immediate-mode rendering matches the real-time nature of the app
- Custom widgets (wavefunction sliders, energy diagrams) are trivially extensible

However, ImGui is **not used for the 3D viewport** — that is an OpenGL framebuffer rendered to a texture and displayed as an ImGui image.

### 9.2 Panel System

```
UIManager
  ├── manages: DockSpace (full-window ImGui docking layout)
  ├── maintains: std::vector<IPanel*> activePanels
  └── per frame: calls IPanel::OnImGui() for each active panel

IPanel (abstract)
  ├── OnImGui()     ← emit ImGui calls
  ├── IsOpen()
  └── SetOpen(bool)
```

Panels are registered by modules on attach and unregistered on detach. The docking layout persists to `imgui.ini` + a custom `layout.json` (per-module saved layout).

### 9.3 Panel Inventory

```
MainMenuBar
  ├── File: New Scene, Open, Save, Export
  ├── Modules: sub-menu listing all registered modules
  ├── View: toggle panels
  └── Help: about, documentation links

ViewportPanel
  ├── Owns: GLFramebuffer (color + depth)
  ├── Renders: framebuffer texture as ImGui::Image
  ├── Handles: viewport-local mouse input (passed to CameraController)
  └── Overlay: gizmos (atom labels, bond lengths, energy annotations)

PropertiesPanel
  ├── Context-sensitive: shows properties of selected entity
  ├── Atom: element, position, orbital occupancy
  ├── Orbital: n, l, m sliders + |ψ|² visualization settings
  └── Lattice: a, b, c, α, β, γ unit cell parameters

TimelinePanel
  ├── Simulation time scrubber
  ├── Play/pause/step buttons
  ├── Frame rate display
  └── Keyframe markers (for cinematic camera)

ConsolePanel
  ├── Ring-buffer of spdlog messages
  ├── Severity filter (Info/Warn/Error)
  └── Command input (for scripted simulations)
```

### 9.4 Custom Widgets

The `Widgets.hpp` library provides reusable ImGui extensions:

| Widget | Purpose |
|--------|---------|
| `QuantumNumberSelector` | n/l/m spin boxes with validity rules enforced |
| `EnergyLevelDiagram` | Canvas-drawn horizontal lines with labels |
| `BlochSphereWidget` | Rendered sphere + state vector arrow |
| `BrillouinZoneWidget` | Interactive k-path selector overlay |
| `ColorTransferEditor` | 1D LUT editor for wavefunction coloring |
| `PeriodicTablePicker` | Compact element selector grid |

### 9.5 Theme System

```
Theme
  ├── Colors:   ImVec4 palette[ThemeColor::COUNT]
  ├── Fonts:    ImFont* body, heading, monospace, math
  ├── Spacing:  float itemSpacing, windowPadding, ...
  └── Apply():  pushes ImGui style stack
```

Themes are loaded from JSON. A `DarkScientific` theme (default) uses a deep navy background with phosphor-green accents — optimized for long observation sessions in dark rooms.

---

## 10. Simulation Architecture

### 10.1 Threading Model

```
Main Thread                     Simulation Thread
    │                                │
    │── SimulationEngine::Step() ──► │  RunPhysicsStep()
    │   [non-blocking, posts job]    │  [Eigen solvers, sampling]
    │                                │
    │   [continues rendering]        │  on completion:
    │                                │  EventBus::Post(SimulationStepEvent{
    │                                │      snapshot = ImmutableState::Capture()
    │                                │  })
    │◄── Event arrives next frame ───│
    │    ModuleLayer::OnEvent()
    │    → uploads new 3D texture
```

The simulation thread **never writes to GPU resources**. It produces an `ImmutableSimulationState` snapshot (copy of computed arrays), then signals the main thread via the event bus. The main thread then uploads to GPU.

### 10.2 SimulationEngine

```
SimulationEngine
  ├── owns: std::thread simulationThread
  ├── owns: std::atomic<bool> running
  ├── owns: std::atomic<bool> dirty  ← set true when parameters change
  ├── owns: JobQueue<SimulationJob>  ← lock-free MPSC queue
  │
  ├── Main-thread API:
  │     RequestStep(params)      ← post job, non-blocking
  │     SetFixedTimeStep(dt)
  │     Pause() / Resume()
  │
  └── Simulation-thread internals:
        ProcessJobQueue()
        RunQuantumStep(job)
          └─► WavefunctionSampler::Sample(n, l, m, grid)
                └─► HydrogenicOrbital::Psi(r, theta, phi)
```

### 10.3 SimulationState Snapshot

```
ImmutableSimulationState  (value type, copyable)
  ├── stepId: uint64_t
  ├── wavefunctionGrid: std::vector<float>  (128³ or 256³)
  ├── gridResolution: glm::ivec3
  ├── gridBounds: AABB
  ├── energyLevels: std::vector<EnergyLevel>
  ├── moleculeState: MoleculeSnapshot
  └── timestamp: std::chrono::steady_clock::time_point
```

This is a pure data object. No mutexes, no pointers to simulation-internal state. Copying it is safe from any thread.

### 10.4 Quantum Simulation Hierarchy

```
QuantumSystem (abstract)
  ├── HydrogenAtom
  │     ├── AnalyticWavefunction(n, l, m)  → grid
  │     └── EnergySpectrum()              → {E_n}
  │
  ├── MultiElectronAtom
  │     ├── HartreeFock approximation (Eigen-based)
  │     └── ElectronConfiguration()
  │
  ├── DiatomicMolecule
  │     ├── LCAO(bondLength, atomicNumbers)
  │     └── MODiagram()
  │
  └── CrystalSystem
        ├── BandStructure(kPath)  → E(k) curves
        └── PhononDispersion(kPath)
```

### 10.5 Numerical Backend

All matrix operations use **Eigen** exclusively:
- `Eigen::MatrixXcd` for complex Hamiltonians
- `Eigen::SelfAdjointEigenSolver` for Hermitian diagonalization
- `Eigen::SparseMatrix` for large tight-binding models

Wavefunction grids are `std::vector<float>` (row-major, z-inner) for direct upload to `GL_TEXTURE_3D`.

---

## 11. Dependency Relationships

### 11.1 Layer Dependency DAG

```
Layer 4: modules/, ui/
    │ depends on ▼
Layer 3: simulation/
    │ depends on ▼
Layer 2: renderer/, scene/, camera/
    │ depends on ▼
Layer 1: events/, resources/
    │ depends on ▼
Layer 0: core/, platform/
    │ depends on ▼
Third Party: glfw, glad, glm, eigen, imgui, spdlog
```

**Strict rule**: Layer N may only depend on Layer ≤ N. Violations are enforced at CMake target level — link dependencies are explicitly listed per target, and circular deps cause build failure.

### 11.2 Inter-Subsystem Communication Rules

| From → To | Mechanism | Rationale |
|-----------|-----------|-----------|
| Simulation → Renderer | EventBus (snapshot) | Thread safety |
| Module → Renderer | Direct submit call | Same thread, performance |
| Module → SimulationEngine | Direct API call | Same thread |
| UI → Module | Direct call (UI owns module ref) | Same thread |
| Any → EventBus | Post() | Decoupled, async-safe |
| Renderer → Resources | Handle<T> lookup | Cache locality |

### 11.3 Third-Party Dependency Map

| Library | Used By | Why |
|---------|---------|-----|
| **GLFW** | `platform/Window`, `platform/Input` | Cross-platform window + input |
| **GLAD** | `platform/GLContext`, `renderer/backend/` | OpenGL function loading |
| **GLM** | `camera/`, `renderer/`, `scene/components/` | Vector/matrix math (header-only) |
| **Eigen** | `simulation/solvers/`, `simulation/quantum/` | Numerical linear algebra |
| **Dear ImGui** | `ui/` only | Immediate-mode UI |
| **spdlog** | `core/Log` only | Structured, fast logging |
| **nlohmann/json** | `scene/SceneSerializer`, `resources/`, `ui/Theme` | JSON I/O |
| **stb_image** | `resources/loaders/TextureLoader` | PNG/JPEG loading |

---

## 12. UML-Style Diagrams

### 12.1 Core Engine — Class Relationships

```
┌─────────────────────────────────────────────────────────────────┐
│                         Application                              │
│  + Run()                                                         │
└─────────────────────┬───────────────────────────────────────────┘
                      │ owns
                      ▼
┌─────────────────────────────────────────────────────────────────┐
│                           Engine                                 │
│  + Initialize()                                                  │
│  + Shutdown()                                                    │
│  + PushLayer(Layer*)                                             │
│  ─────────────────────────────────────────────────────────────  │
│  - window_        : Window                                       │
│  - eventBus_      : EventBus                                     │
│  - resourceMgr_   : ResourceManager                              │
│  - renderer_      : Renderer                                     │
│  - simEngine_     : SimulationEngine                             │
│  - uiManager_     : UIManager                                    │
│  - layerStack_    : LayerStack                                   │
│  - moduleRegistry_: ModuleRegistry                               │
└─────────────────────────────────────────────────────────────────┘
```

### 12.2 Renderer — Frame Graph

```
┌───────────────────────────────────────────────────────┐
│                      RenderGraph                       │
│  + AddPass(PassNode)                                   │
│  + Compile()   ← topological sort, resource alloc      │
│  + Execute(RenderContext&)                             │
└───────────────────────┬───────────────────────────────┘
                        │ contains N
                        ▼
              ┌─────────────────────┐
              │      PassNode        │
              │ - name: string       │
              │ - reads: [ResID]     │
              │ - writes: [ResID]    │
              │ + Execute(ctx)  ─────┼──► RenderPass::Execute()
              └─────────────────────┘

RenderPass (abstract)
  ├── GeometryPass
  ├── VolumetricPass
  ├── IsosurfacePass
  ├── LightingPass
  ├── PostProcessPass
  └── UIPass
```

### 12.3 Module System — Lifecycle

```
ModuleRegistry
  │  Register(factory)
  │  Activate("OrbitalViewer")
  │       │
  │       ├─► factory() → IModule* m
  │       ├─► m->OnAttach(context_)
  │       │         ├─► Creates Scene
  │       │         ├─► Registers UI panels
  │       │         └─► Posts SimulationJob
  │       └─► layerStack_.PushLayer(m)

Per frame while active:
  m->OnUpdate(dt)
  m->OnRenderSubmit(renderer_)
  m->OnImGui()

On switch:
  m->OnDetach()
  layerStack_.PopLayer(m)
```

### 12.4 Simulation — Thread Boundary

```
Main Thread                          Simulation Thread
────────────────                     ────────────────────────────
SimulationEngine::RequestStep(P)
  jobQueue_.Push({params: P})  ─────► workerLoop():
                                         job = jobQueue_.Pop()
                                         state = RunStep(job)
                                         snap = ImmutableState(state)
                                         eventBus_.Post(
                                           SimulationStepEvent{snap}
                                         )
EventBus::Dispatch() (next frame) ◄──
  SimulationStepEvent received
  Module::OnSimulationStep(event)
    UploadGridToGPU(event.snapshot)
```

### 12.5 Event Bus — Dispatch Flow

```
               ┌─────────────────────────────────────┐
               │             EventBus                 │
               │                                      │
Producer ──────► Post<E>(event)                       │
               │   queue_[E::Category].Push(event)    │
               │                                      │
               │  Dispatch() [main thread, per frame] │
               │   for each queue:                    │
               │     while !empty:                    │
               │       e = queue.Pop()                │
               │       for sub in subscribers_[E]:    │
               │         if sub.handler(e): break     │◄── consumed
               └─────────────────────────────────────┘
```

### 12.6 ECS — Data Layout (Memory View)

```
Registry
  ComponentPool<TransformComponent>: [ T0 | T1 | T2 | T3 | ... ]  ← contiguous array
  ComponentPool<AtomComponent>:      [ A0 | A1 | A2 | _  | ... ]  ← sparse set
  ComponentPool<OrbitalComponent>:   [ _  | _  | O2 | _  | ... ]

  SparseSet<TransformComponent>:
    sparse[entityID] → dense index
    dense[]          → entity IDs (for reverse lookup)

View<Transform, Atom>:
  iterates dense intersection → guaranteed cache-line locality
```

---

## 13. Build Pipeline

### 13.1 CMake Target Hierarchy

```
orbital (executable)
  ├── links: orbital_engine
  └── links: orbital_modules

orbital_engine (static lib)
  ├── links: orbital_core
  ├── links: orbital_renderer
  ├── links: orbital_simulation
  ├── links: orbital_ui
  └── links: orbital_scene

orbital_core (static lib)
  └── links: spdlog, nlohmann_json

orbital_renderer (static lib)
  └── links: glad, glm, orbital_core

orbital_simulation (static lib)
  └── links: eigen, orbital_core

orbital_ui (static lib)
  └── links: imgui, orbital_core, orbital_renderer

orbital_modules (static lib)
  └── links: orbital_engine (all subsystems)

orbital_tests (executable)
  └── links: orbital_simulation, orbital_core, GTest
```

> **Why static libraries per subsystem?**  
> Each `orbital_*` target has explicit `target_link_libraries()` with visibility modifiers (`PRIVATE`/`PUBLIC`). This makes illegal cross-layer dependencies a **compile error**, not a review finding. It also parallelizes compilation across targets.

### 13.2 CMake Configuration

```cmake
# CompilerOptions.cmake (excerpt — concept, not implementation)
target_compile_features(orbital_core PUBLIC cxx_std_20)
target_compile_options(orbital_core PRIVATE
    -Wall -Wextra -Wpedantic
    -Werror=return-type
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    $<$<CONFIG:Release>:-O3 -DNDEBUG -march=native>
)
```

### 13.3 Build Configurations

| Config | Flags | Purpose |
|--------|-------|---------|
| `Debug` | `-O0 -g`, ASan+UBSan | Development, full diagnostics |
| `RelWithDebInfo` | `-O2 -g`, no sanitizers | Profiling, performance testing |
| `Release` | `-O3 -march=native`, NDEBUG | Distribution |

### 13.4 Dependency Management

**Primary**: `FetchContent` (CMake 3.14+) for all third-party libraries. Pinned to specific git tags for reproducible builds:

```
FetchContent_Declare(glfw  GIT_TAG 3.3.9 ...)
FetchContent_Declare(glm   GIT_TAG 0.9.9.8 ...)
FetchContent_Declare(eigen GIT_TAG 3.4.0 ...)
FetchContent_Declare(imgui GIT_TAG v1.90.4 ...)
```

**Fallback**: `find_package()` for system-installed libraries (CI environments). `cmake/Modules/` contains custom finders for Eigen (which doesn't always export CMake targets).

### 13.5 Shader Build Integration

Shaders live in `assets/shaders/` as GLSL source. The build pipeline:

1. **Development**: Shaders loaded at runtime (hot-reload enabled)
2. **Release**: `shader_preprocessor` tool (in `tools/`) runs at build time:
   - Resolves all `#include` directives
   - Optionally runs SPIRV-Cross for cross-compilation
   - Embeds into a `shaders.pak` binary
3. CMake custom target: `add_custom_target(orbital_shaders COMMAND shader_preprocessor ...)`

### 13.6 CI Pipeline (GitHub Actions concept)

```yaml
jobs:
  build-linux:   Ubuntu 22.04, GCC 13, Debug + Release
  build-windows: Windows 2022, MSVC 2022, Release
  build-macos:   macOS 14, Clang 17, Release
  unit-tests:    Run orbital_tests (headless, no GPU required)
  static-analysis: clang-tidy + cppcheck
  docs:          Doxygen → GitHub Pages
```

GPU render tests (`integration/render_headless/`) use Mesa's software rasterizer (`LIBGL_ALWAYS_SOFTWARE=1`) in CI for deterministic pixel output.

### 13.7 Packaging

```
CPack (cpack.cmake):
  - Windows: NSIS installer (bundles MSVC runtime)
  - Linux:   DEB + AppImage
  - macOS:   .dmg bundle
```

---

## Appendix A: Key Design Decisions Summary

| Decision | Alternative Considered | Why This Choice |
|----------|----------------------|-----------------|
| Frame Graph render pipeline | Forward rendering | Orbital's overdraw from volumetrics + many atoms makes deferred essential |
| ECS for scene | Scene graph with node inheritance | Cache efficiency for hundreds of atoms; no coupling between data and logic |
| Immutable simulation snapshots | Shared mutex-protected state | Eliminates entire class of data race bugs; renderer is always consistent |
| Typed Handle<T> resources | Raw UUID or raw pointers | Type safety prevents mixing Shader/Texture handles; ref-counting avoids leaks |
| Static libs per CMake target | Single monolithic target | Enforces layering at build time; enables parallel compilation |
| Reversed-Z depth | Standard depth | Orbital spans atomic to crystal scales; reversed-Z prevents z-fighting |
| Arcball as primary camera | Trackball / turntable | Arcball maintains consistent "up" feel; better for molecular rotations |
| Module self-registration macro | Manual factory registration | Zero engine changes to add modules; scales to dozens of modules |
| Lock-free event queue | std::mutex-guarded callbacks | Simulation thread can safely post without blocking render loop |

---

## Appendix B: Growth Roadmap (Architecture-Ready)

The following future features are architecturally accommodated **today** without requiring structural changes:

| Future Feature | Accommodated By |
|----------------|----------------|
| Time-dependent Schrödinger (TDSE) | `SimulationEngine` fixed-step loop, `TimelinePanel` |
| Molecular dynamics | New `simulation/dynamics/` module, same snapshot pattern |
| Export to video | `CinematicController` + offscreen `GLFramebuffer` |
| Python scripting API | `ConsolePanel` command input, `IModule` exposed via pybind11 |
| VR/stereo rendering | `Camera` produces two view matrices; `RenderGraph` runs twice |
| Collaborative sessions | `SimulationState` snapshots are trivially serializable to network |
| Plugin system (user modules) | `ModuleRegistry` already uses factory pattern; load `.dll`/`.so` |
| WebAssembly port | `simulation/` is GL-free; WASM + WebGL2 backend for `renderer/backend/` |
