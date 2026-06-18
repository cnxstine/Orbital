# Orbital

> **Interactive Scientific Visualization Platform for Quantum Mechanics** — v1.0

Orbital is a real-time 3D educational application that makes quantum mechanics tangible. It renders atomic wavefunctions, molecular orbitals, and hybrid orbital hybridization as live probability clouds using Monte Carlo sampling and direct OpenGL rendering.

---

## Features

### Hydrogen Orbital Explorer
Visualize the real-space probability density of every hydrogenic orbital up to n = 2 in real time.

- **Orbitals**: 1s, 2s, 2p_x, 2p_y, 2p_z
- **Rendering**: Stochastic point cloud (100k – 1M samples) using Monte Carlo sampling
- **Sampling strategy**: Direct analytical sampling for s-type orbitals (zero Markov autocorrelation); Metropolis-Hastings MCMC for all p-type orbitals
- **Per-orbital exposure presets**: Particle size, intensity scale, exposure, gamma, and contrast are tuned individually per orbital
- **Educational panel**: Quantum numbers (n, l, m), orbital type, radial and angular node counts, plain-language descriptions
- **Camera presets**: Standard, Top, Front, Side

### Molecular Orbital Explorer
Simulate diatomic molecular orbitals using the LCAO (Linear Combination of Atomic Orbitals) approximation.

- **Orbital types**: σ(1s), σ\*(1s), σ(2p), σ\*(2p), π(2p_x), π\*(2p_x), π(2p_y), π\*(2p_y)
- **Live separation control**: Drag the internuclear distance slider from 0.2 to 10.0 Bohr radii; the orbital cloud resamples instantly
- **Overlap integrals**: Analytical $S(R) = e^{-R}(1 + R + R^2/3)$ and numerical (60³ grid) computed and compared per frame
- **Energy display**: Live bonding energy $E_+(R)$ and antibonding energy $E_-(R)$, Coulomb integral $H_{AA}$, resonance integral $H_{AB}$, and bond order contribution
- **Educational metadata**: Nodal plane structure, orbital symmetry classification, bond order analysis per orbital type

### Energy Curve Explorer
An interactive canvas embedded in the Molecular Orbital Explorer panel.

- **Curves**: Bonding ($E_+$) in cyan, antibonding ($E_-$) in red, over R = 0.5 – 10.0 Bohr
- **Live marker**: Vertical line + filled dots track the current separation on both curves in real time
- **Mouse interaction**: Click or drag on the canvas to move the separation, updating the cloud, marker, and energy readout simultaneously
- **Reference line**: Dashed line at −13.6 eV marks the separated H(1s) atomic energy limit
- **Summary**: Displays equilibrium separation and minimum bonding energy
- **CSV export**: "Generate Energy Curve" button exports a full sweep to `exports/h2_energy_curve.csv`

### Hybrid Orbital Explorer
Visualize sp, sp², and sp³ orbital hybridization as LCAO linear combinations of hydrogenic 2s and 2p orbitals.

- **Types**: sp (Linear), sp² (Trigonal Planar), sp³ (Tetrahedral)
- **Per-orbital selection**: Switch between individual hybrid orbitals within each type
- **Geometry guides**: Toggleable line overlay showing bond vectors and molecular geometry outlines, independent of the cloud toggle
- **Metadata panel**: Hybridization type, orbital index, bond angle, geometry name (Linear/Trigonal Planar/Tetrahedral), constituent atomic orbitals
- **Linear combination formula**: Displays the normalized LCAO expression for the active hybrid orbital

---

## Technical Highlights

| Topic | Detail |
|---|---|
| Language | C++20 |
| Graphics API | OpenGL 4.6 (DSA) |
| Windowing | GLFW 3.4 |
| Math | GLM 1.0.1 |
| UI | Dear ImGui (docking disabled; custom font atlas with Greek glyphs) |
| Logging | spdlog 1.14 |
| Build | CMake 3.25+ with FetchContent |
| Sampling | Monte Carlo point cloud (analytical + MCMC) |
| Molecular math | LCAO / MO theory; Heitler-London-style energy model |
| Input routing | `ImGui::GetIO().WantCaptureMouse` gate on `MouseScrolledEvent` and `MouseButtonPressedEvent` |
| Screenshot output | BMP via shared `BMPWriter` utility |
| Testing | Google Test (16 unit tests) |

---

## Prerequisites

| Tool | Minimum Version | Notes |
|---|---|---|
| CMake | 3.25 | Required for `FetchContent` features |
| C++ Compiler | GCC 13 / Clang 16 / MSVC 2022 | Full C++20 support required |
| Python 3 | 3.8+ | Required by **glad2** at CMake configure time |
| GPU Driver | OpenGL 4.6 | NVIDIA 397.31+, AMD 18.Q1+, Intel Iris Xe+ |

---

## Building

```bash
# Configure (first time: downloads dependencies via FetchContent)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --parallel

# Run
./build/bin/orbital              # Linux/macOS
.\build\bin\Release\orbital.exe  # Windows (MSVC Release)
```

### Run Unit Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DORBITAL_BUILD_TESTS=ON
cmake --build build --parallel
./build/bin/test_simulation       # Linux/macOS
.\build\bin\Debug\test_simulation.exe  # Windows
```

All 16 tests should pass.

### Build Configurations

| Configuration | Use |
|---|---|
| `Debug` | Development — full debug info, logging, GL debug callback |
| `RelWithDebInfo` | Profiling — optimised but debuggable |
| `Release` | Distribution — `-O3` |

---

## Project Structure

```
Orbital/
├── cmake/              CMake helpers (compiler options, dependencies)
├── assets/
│   └── shaders/        GLSL shaders (loaded at runtime)
│       ├── common/     Shared UBO definitions (uniforms.glsl)
│       └── geometry/   Point density, density resolve, line shaders
│                       (mesh.vert / mesh.frag — future ECS infrastructure)
├── src/
│   ├── core/           Layer 0: Engine, logging, time, layers
│   ├── platform/       Layer 0: Window (GLFW), input, GL context
│   ├── events/         Layer 1: Typed EventBus, RAII subscriptions
│   ├── resources/      Layer 1: Handle<T>, ResourceManager, loaders
│   ├── renderer/       Layer 2: Frame graph, GL backend (DSA)
│   ├── camera/         Layer 2: Camera abstraction, Arcball controller
│   ├── scene/          Layer 2: ECS scaffolding (future use)
│   ├── utils/          Shared utilities (BMPWriter)
│   ├── visualization/  Layer 2: Quantum visualization modules, wavefunctions
│   └── main.cpp
├── tests/              Unit + integration tests (GoogleTest)
├── exports/            CSV exports (energy sweeps)
└── screenshots/        BMP captures (manual + automated verification)
```

### Dependency Layer Rule

> A file in Layer N may only `#include` from Layer ≤ N.  
> This is enforced by CMake target visibility — violations are build errors.

---

## Architecture

### Runtime Path (v1.0)

```
Engine::Run()
  ├── EventBus::Dispatch()         — GLFW input → typed events
  ├── CameraManager::OnUpdate()    — Arcball orbit
  ├── LayerStack::OnUpdate()
  │     └── ModuleLayer::OnUpdate()
  │           └── ActiveModule::Update()    — resample if params changed
  ├── Renderer::BeginFrame()       — clear + camera UBO upload
  ├── LayerStack::OnRender()
  │     └── ModuleLayer::OnRender()
  │           └── ActiveModule::Render()    — direct OpenGL 2-pass render
  │                 Pass 1: accumulate point cloud → R32F FBO
  │                 Pass 2: tone-map resolve → backbuffer
  ├── ImGui frame
  │     └── ModuleLayer::OnImGui()
  │           ├── Module Navigator combo (switches active module)
  │           └── ActiveModule::OnParameterPanel()
  └── Window::SwapBuffers()
```

### Module Switching

All three explorer modules implement `VisualizationModule`. Switching is instant: `OnExit()` frees all GPU resources; `OnEnter()` reloads shaders from the resource cache (no recompile) and rebuilds samplers.

### Known Future Infrastructure (not active at runtime)

The following systems are scaffolded but not connected in v1.0:

| System | Files | Purpose |
|---|---|---|
| ECS Scene Graph | `src/scene/Scene.hpp`, `RenderSystem`, `TransformSystem` | General-purpose entity + component rendering |
| ScientificRenderer | `src/visualization/ScientificRenderer.hpp` | Volume ray-march, vector glyphs, lattice boxes |
| DensityField | `src/visualization/DensityField.hpp` | 3D voxel grid for volumetric rendering |
| FlyController | `src/camera/controllers/FlyController` | Free-fly camera (Arcball is active default) |
| mesh.vert / mesh.frag | `assets/shaders/geometry/` | ECS mesh rendering (requires MeshComponent) |

---

## Controls

| Action | Control |
|---|---|
| Orbit camera | Left mouse drag |
| Zoom | Mouse wheel (viewport only) |
| Camera presets | Buttons in the active module panel |
| Orbital / MO selection | Combo box in the panel |
| Separation slider | Drag in Molecular Orbital Explorer |
| Energy graph drag | Click/drag on the Energy Curve canvas |
| Geometry guides toggle | Checkboxes in Hybrid Orbital Explorer |
| Screenshot capture | "Generate Verification Package" button |
| CSV export | "Generate Energy Curve" button in MO Explorer |

> **Note**: Mouse wheel and button events are blocked from reaching the 3D camera whenever `ImGui::GetIO().WantCaptureMouse` is true. Scrolling inside any panel does not zoom the viewport.

---

## Verification & Testing

### Unit Tests (16 tests)

```bash
./build/bin/test_simulation
```

Covers: orbital normalization, 2p symmetry, LCAO validation, overlap integral convergence, energy model physics, energy sweep monotonicity, input event routing (ImGui gate), hybrid orbital normalization, hybrid geometry bond angles.

### Automated Screenshot Pipeline

```bash
ORBITAL_GENERATE_VERIFICATION=1 ./build/bin/orbital
```

Generates:
- `screenshots/hydrogen_{1s,2s,2px,2py,2pz}_{standard,front,side,top}.bmp` (20 files)
- `screenshots/hydrogen_{sigma_1s,sigma_1s_star,...}_{R}bohr_{camera}.bmp` (128 files)
- `screenshots/hybrid_{sp,sp2,sp3}_orb{N}_{standard,front,side,top}.bmp` (36 files)
- `exports/h2_energy_curve.csv`

The pipeline chains automatically: Hydrogen → Molecular → Hybrid → shutdown.

---

## License

MIT — see [LICENSE](LICENSE)
