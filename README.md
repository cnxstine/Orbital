# Orbital

> **Interactive Scientific Visualization Platform for Quantum Mechanics**

Orbital teaches how quantum mechanics gives rise to atoms, chemical bonds, crystal lattices, and material properties through real-time 3D visualization.

---

## Features

*   **Hydrogenic Wavefunction Solver:** Real-time 3D evaluation and rendering of atomic hydrogenic wavefunctions ($1s, 2s, 2p_x, 2p_y, 2p_z$).
*   **Molecular Orbital Explorer:** Real-time simulation of diatomic molecular orbitals (constructive/destructive interference) using the Linear Combination of Atomic Orbitals (LCAO) approximation. Supports $\sigma(1s), \sigma^*(1s)$, $\sigma(2p_z), \sigma^*(2p_z)$, $\pi(2p_x), \pi^*(2p_x)$, and $\pi(2p_y), \pi^*(2p_y)$ states.
*   **Overlap & Energy sweep Calculations:** Real-time numerical integration of the overlap integral $S(R) = \int \psi_A \psi_B \, dV$ and LCAO bonding/antibonding energy sweeps ($E_+(R), E_-(R)$) with respect to internuclear distance.
*   **Interactive Energy Curve Explorer:** Plots bonding and antibonding energy curves using a custom high-performance drawing canvas in Dear ImGui, with live marker tracking and interactive cursor dragging to change orbital separation instantly.
*   **Precise Input routing Boundary:** Prevents viewport camera zooming and drag-rotations when interacting with scrollable UI panels, sliders, and combo box dropdowns.

---

## Technology Stack

| Layer | Technology |
|-------|-----------|
| Language | C++20 |
| Graphics API | OpenGL 4.6 (DSA) |
| Windowing | GLFW 3.4 |
| Math | GLM 1.0.1 |
| Numerics | Eigen 3.4 |
| UI | Dear ImGui |
| Logging | spdlog 1.14 |
| Build | CMake 3.25+ |

---

## Prerequisites

| Tool | Minimum Version | Notes |
|------|----------------|-------|
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
./build/bin/orbital        # Linux/macOS
.\build\bin\Release\orbital.exe  # Windows (Release mode)
```

### Run Unit Tests

To run the full suite of simulation and input routing tests:
```bash
./build/bin/test_simulation.exe
```

### Build configurations

| Configuration | Use |
|---|---|
| `Debug` | Development — full debug info, logging, GL debug callback |
| `RelWithDebInfo` | Profiling — optimised but debuggable |
| `Release` | Distribution — `-O3 -march=native` |

---

## Project Structure

```
Orbital/
├── cmake/              CMake helpers (compiler options, dependencies)
├── assets/
│   └── shaders/        GLSL shaders (loaded at runtime)
├── src/
│   ├── core/           Layer 0: Engine, logging, time, layers
│   ├── platform/       Layer 0: Window (GLFW), input, GL context
│   ├── events/         Layer 1: Typed EventBus, RAII subscriptions
│   ├── resources/      Layer 1: Handle<T>, ResourceManager, loaders
│   ├── renderer/       Layer 2: Frame graph, GL backend (DSA)
│   ├── camera/         Layer 2: Camera abstraction, controllers
│   ├── scene/          Layer 2: ECS, systems
│   ├── visualization/  Layer 2: Quantum visualization modules, wavefunctions, LCAO math
│   └── main.cpp
├── tests/              Unit + integration tests (using GoogleTest)
└── tools/              CLI utilities (shader preprocessor, etc.)
```

### Dependency Layer Rule

> A file in Layer N may only `#include` from Layer ≤ N.  
> This is enforced by CMake target visibility — violations are build errors.

---

## Architecture

See [docs/architecture/orbital_architecture.md](docs/architecture/orbital_architecture.md) for the full architectural design document covering:

- Frame graph rendering pipeline
- ECS scene system
- Lock-free EventBus
- Handle-based resource management
- Arcball/Fly/Cinematic camera controllers
- Simulation thread isolation

---

## Roadmap

- [x] Rendering foundation (window, context, shader, camera)
- [x] Hydrogen orbital wavefunction visualization ($1s, 2s, 2p$)
- [x] Diatomic Molecular orbital explorer ($1s, 2p$ bonding/antibonding states)
- [x] LCAO overlap integration and energy modeling
- [x] Interactive UI plotting (Energy Curve Explorer)
- [x] Unicode symbol font support ($\sigma, \pi$) and input event routing boundary
- [ ] Atom explorer module
- [ ] Crystal lattice viewer
- [ ] Band structure diagram
- [ ] Simulation thread + Eigen solvers

---

## License

MIT — see [LICENSE](LICENSE)
