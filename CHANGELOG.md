# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Orbital uses [Semantic Versioning](https://semver.org/).

---

## [1.0.0] — 2026-06-18

### Added

#### Hydrogen Orbital Explorer
- Real-time 3D visualization of hydrogenic wavefunctions: 1s, 2s, 2p_x, 2p_y, 2p_z.
- Stochastic point cloud renderer using Monte Carlo sampling (100k – 1M points).
- Dual sampling strategy: direct analytical sampling for s-type orbitals; Metropolis-Hastings MCMC for p-type orbitals.
- Per-orbital exposure presets (particle size, intensity, exposure, gamma, contrast).
- Educational metadata panel: quantum numbers, node counts, orbital type descriptions.
- Camera presets: Standard, Top (XZ), Front (XY), Side (YZ).
- Screenshot generation button: captures all five orbitals × four camera presets as BMP files.

#### Molecular Orbital Explorer
- LCAO molecular orbital visualization for H₂-style diatomic systems.
- Orbital types: σ(1s), σ\*(1s), σ(2p), σ\*(2p), π(2p_x), π\*(2p_x), π(2p_y), π\*(2p_y).
- Live internuclear separation slider (0.2 – 10.0 Bohr), cloud resamples in real time.
- Real-time overlap integral: analytical $S(R)$ and numerical (60³ grid) with absolute error display.
- Energy panel: bonding energy $E_+(R)$, antibonding energy $E_-(R)$, $H_{AA}$, $H_{AB}$, bond order contribution.
- Per-orbital nodal structure descriptions and midpoint probability density.
- Screenshot generation: 8 orbital types × 4 separations × 4 camera presets (128 BMP files).

#### Energy Curve Explorer (embedded in Molecular Orbital Explorer)
- Interactive ImDrawList canvas showing bonding and antibonding potential energy curves.
- Separation range: 0.5 – 10.0 Bohr; energy range: −25 to +25 eV.
- Live vertical marker and filled dot indicators tracking current separation on both curves.
- Mouse-drag interaction: click or drag on the canvas to change the separation slider.
- Dashed reference line at −13.6 eV (separated H(1s) atomic energy limit).
- Summary display: equilibrium separation and minimum bonding energy from pre-cached sweep.
- CSV export via "Generate Energy Curve" button → `exports/h2_energy_curve.csv`.
- Collapsible educational section with three explanatory paragraphs.

#### Hybrid Orbital Explorer
- sp, sp², sp³ hybridization visualized as normalized LCAO linear combinations of 2s and 2p orbitals.
- Orbital metadata panel: hybridization type, orbital index, bond angle, geometry name, constituent AO formula.
- Probability cloud toggle and geometry guide toggle operate independently.
- sp guides: two-line linear arrangement; sp² guides: trigonal planar triangle; sp³ guides: tetrahedral cage.
- Camera presets and per-type exposure presets.
- Screenshot generation: sp (2 orbitals) + sp² (3 orbitals) + sp³ (4 orbitals) × 4 camera presets (36 BMP files).

#### Automated Verification Pipeline
- Environment variable `ORBITAL_GENERATE_VERIFICATION=1` triggers headless screenshot generation.
- Pipeline chains automatically: Hydrogen → Molecular → Hybrid → shutdown.
- Automated CSV export of energy sweep to `exports/h2_energy_curve.csv` during the Molecular step.

#### Input Event Routing
- `CameraManager` subscribes `MouseScrolledEvent` and `MouseButtonPressedEvent` with an `ImGui::GetIO().WantCaptureMouse` gate.
- Scroll wheel and mouse button events are marked `Handled = true` and do not reach the camera controller when ImGui captures the mouse.
- Existing viewport orbit (drag) and viewport zoom function normally when the cursor is over the render area.

#### Core Infrastructure
- `Engine` main loop with typed `EventBus`, `LayerStack`, `CameraManager`, `ResourceManager`.
- `ArcballController` camera with `SetViewPoint()` for programmatic preset transitions.
- Two-pass OpenGL rendering: R32F accumulation FBO (Pass 1) + tone-map resolve to backbuffer (Pass 2).
- Handle-based `ResourceManager` with `GLShader` caching (no recompile on module switch).
- `VisualizationModule` lifecycle interface: `OnEnter`, `OnExit`, `Update`, `Render`, `OnParameterPanel`, `OnEvent`.
- `ModuleLayer` combo-box navigator for instant module switching.
- Dear ImGui with custom font atlas (Segoe UI / Arial, 16pt) covering Greek and superscript/subscript glyph ranges.
- spdlog-backed logging with `ORB_CORE_*` macros.

---

### Changed (v1.0 Release Hardening)

#### BMP Screenshot Utility Consolidation
- **Removed**: Identical `SaveBMP()`, `BMPFileHeader`, and `BMPInfoHeader` definitions that were copy-pasted into `OrbitalViewerModule.cpp`, `MolecularOrbitalExplorerModule.cpp`, and `HybridOrbitalExplorerModule.cpp` (three separate translation units).
- **Added**: `src/utils/BMPWriter.hpp` and `src/utils/BMPWriter.cpp` — shared utility as `orbital_utils` static library.
- All three modules now call `BMPWriter::Save()`.

#### CSV Export in Automated Pipeline
- **Fixed**: `ORBITAL_GENERATE_VERIFICATION=1` now exports `exports/h2_energy_curve.csv` automatically in `MolecularOrbitalExplorerModule::OnEnter()`.
- **Preserved**: Manual "Generate Energy Curve" button export continues to work as before.

#### Module Event Forwarding
- **Fixed**: `ModuleLayer::OnEvent()` previously always returned `false` (events were never forwarded to the active `VisualizationModule`).
- Now delegates to `m_ActiveModule->OnEvent(event)` to enable future module-level event handling.
- **Added**: `virtual bool OnEvent(Event&)` with a default no-op implementation to `VisualizationModule` interface (no changes required in existing modules).

#### Histogram Exposure Mode Removal (Option B)
- **Removed**: `ExposureMode::HistogramBased` from `OrbitalViewerModule` and `MolecularOrbitalExplorerModule`.
- **Removed**: Corresponding `HistogramBased` label from the UI combo box in both modules.
- **Removed**: "Histogram mode: using manual exposure fallback." warning text.
- `ExposureMode` is now `{ Manual, PerOrbital }` in both modules.

#### Dead Asset Documentation
- `assets/shaders/geometry/mesh.vert` and `mesh.frag` now carry an explicit `STATUS: FUTURE INFRASTRUCTURE` comment identifying them as part of the planned ECS/RenderSystem path.

#### Release Metadata
- Project version bumped from `0.1.0` to `1.0.0` in `CMakeLists.txt`.
- `README.md` completely rewritten for v1.0: feature descriptions, technical highlights, architecture overview, controls, verification pipeline.
- `CHANGELOG.md` created (this file).

---

### Architecture Notes

The following systems are compiled into the static libraries but carry no runtime load in v1.0. They are preserved as foundations for future work:

| System | Status |
|---|---|
| `Scene`, `RenderSystem`, `TransformSystem` | ECS scaffolding — no entities created at runtime |
| `ScientificRenderer` | Facade initialized but no draw calls submitted |
| `DensityField` | Header-only; not instantiated |
| `FlyController` | Compiled; `ArcballController` is the active default |
| `mesh.vert` / `mesh.frag` | Retained as future ECS mesh rendering shaders |

---

## [Unreleased]

- Atom explorer module
- Crystal lattice viewer
- Band structure diagram
- Simulation thread + Eigen solvers
- ECS Scene Graph integration
- RenderSystem mesh pipeline
