# Orbital

> **Interactive Scientific Visualization Platform for Quantum Mechanics**

Orbital teaches how quantum mechanics gives rise to atoms, chemical bonds, crystal lattices, and material properties through real-time 3D visualization.

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
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build --parallel

# Run
./build/bin/orbital        # Linux/macOS
.\build\bin\Debug\orbital.exe  # Windows
```

### Build configurations

| Configuration | Use |
|---|---|
| `Debug` | Development — full debug info, logging, GL debug callback |
| `RelWithDebInfo` | Profiling — optimised but debuggable |
| `Release` | Distribution — `-O3 -march=native` |

```bash
# Release build
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel
```

### Options

| CMake Option | Default | Description |
|---|---|---|
| `ORBITAL_BUILD_TESTS` | `ON` | Build unit tests |
| `ORBITAL_SANITIZERS` | `OFF` | Enable ASan + UBSan (Debug only) |
| `ORBITAL_ASSET_DIR` | `<source>/assets` | Path to runtime assets |

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
│   └── main.cpp
├── tests/              Unit + integration tests
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
- [ ] Hydrogen orbital wavefunction visualization
- [ ] Atom explorer module
- [ ] Molecular orbital builder
- [ ] Crystal lattice viewer
- [ ] Band structure diagram
- [ ] Dear ImGui panel system
- [ ] Simulation thread + Eigen solvers

---

## License

MIT — see [LICENSE](LICENSE)
