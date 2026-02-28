# Changelog

## 2026-02-15 ~ 2026-02-28 (Sun-Sat)

Summary: A total of 61 commits were accumulated from 2026-02-15 to 2026-02-28. The main themes were migration of the installation flow to a Rust CLI, refinement of multi-platform bootstrap behavior, compatibility refactoring of QML components, routing/hierarchy navigation performance improvements, and expansion of P4 quality automation. Changes were concentrated in `rust-cli/`, `install.sh`, `cmake/`, `qml/components/`, `tests/`, and `docs/`.

Scope overview:
- Install/bootstrap: `install.sh` was converted to a Rust CLI wrapper (`lvrs install`), and `bootstrap`/`doctor`/`platform` commands were added.
- Multi-platform policy: bootstrap targets including WASM and the export/launch flow were strengthened, and automatic hint injection for Qt/SDK/NDK/emsdk was refined.
- QML/components: compatibility refactoring was applied to input/check/button/surface/navigation components, and Hierarchy chevron visibility was corrected based on `hasChildItems`.
- Routing/state: `RouteResolver` was introduced, and `HierarchyList` state management was optimized for cleaner path resolution/cache behavior and tree interaction.
- Quality/observability: P4 quality automation scripts (performance/visual/soak/sanitizer) and related tests/baseline docs were expanded.
- Platform compatibility: recurring cleanup around Apple AGL warning paths and iconset structure reduced build/runtime noise.

Key commit groups (representative commits):
- 2026-02-15 platform bootstrap expansion: `9c31546`, `d9a189e`, `cfc6aeb`, `fefebfa`, `ed431a0`, `4646fa2`, `85fefb1`, `e93699c`.
- 2026-02-16 large UI/docs cleanup: `f81f159`, `74526ca`, `db79cec`, `39f0fff`, `b6a7d43`, `eb8edb2`.
- 2026-02-18 quality automation and runtime performance observability: `4bb8fe5`, `eb4b1bd`, `8dada65`, `39b43fe`, `f1bd142`.
- 2026-02-19 routing/Hierarchy performance improvements: `a77d21b`, `b8f846a`, `628e2b1`, `ebfc355`.
- 2026-02-21 iconset/platform warning cleanup: `cf892bb`, `b18271e`, `3f73532`.
- 2026-02-28 full Rust CLI adoption and compatibility refactor: `868a0a7`, `e355f24`, `5823419`, `8516dd3`, `dbd539d`, `953e78b`.

Key file touchpoints:
- `rust-cli/src/*`, `rust-cli/README.md`: new and expanded install/bootstrap/diagnostic CLI.
- `install.sh`: moved from a shell-based installer to a CLI wrapper entrypoint.
- `CMakeLists.txt`, `cmake/LVRSHelpers.cmake`, `cmake/LVRSConfig.cmake.in`: strengthened QML module metadata compatibility, framework bootstrap targets, and platform hint/registry propagation.
- `qml/components/control/*`, `qml/components/navigation/*`, `qml/components/surfaces/*`, `qml/ApplicationWindow.qml`: compatibility refactoring and chevron/input behavior adjustments.
- `tests/ci/*`, `tests/tst_*`: expanded P4 quality gates and regression/performance coverage.
- `docs/*`: strengthened build/architecture/component docs and refined API descriptions.

Verification notes:
- This section was aggregated and summarized from `git log --since=2026-02-15` and per-commit changed file lists (`--name-only`).
- Current installation behavior was cross-checked between `install.sh` and `rust-cli/src/commands/{install,bootstrap}.rs`.

## 2026-02-14 (Sat)

Summary: On 2026-02-14, 23 commits were made. QML app composition automation, rendering backend policy hardening, runtime event console transition, large iconset additions, UI theme/tone naming cleanup, and new component introduction (including Hierarchy) were concentrated in one day. Major changes were spread across `CMakeLists.txt`, `cmake/`, `backend/runtime/`, `qml/`, `docs/`, and `resources/iconset/`.

Scope overview:
- Build/install: added QML app composition functions (`lvrs_configure_qml_app`, `lvrs_add_qml_app`), expanded install scripts, and improved static plugin/module detection.
- Graphics backend: added Vulkan enforcement/runtime validation, macOS/iOS Metal enforcement policy, and Vulkan bootstrap utility.
- Runtime events/console: expanded RuntimeEvents capabilities, improved input state tracking, and switched to Runtime Event Daemon Console.
- UI/QML: reorganized `Main.qml`, added Hierarchy components, refined button/menu/alert styling, and comprehensively cleaned theme color naming.
- Resources: added a large iconset and renamed existing icons.
- Docs/examples/tests: broad documentation reinforcement and updates to examples/tests.

Commit-by-commit trace (summarized from each commit's diff stats):
- d1d20717: QML app composition automation (`lvrs_add_qml_app`) and install script expansion. Added CMake helpers/static target templates, updated docs, and improved runtime service tests.
- e16cccd: merge from remote master branch. Includes LICENSE changes.
- 0d2060d: introduced `lvrs_configure_qml_app`, reorganized QML example project setup, expanded LVRSConfig, and cleaned docs/examples.
- bedbbb3: LICENSE update.
- 22a13b0: strengthened per-platform rendering backend selection policy, expanded Vulkan validation, and documented macOS/iOS Metal enforcement.
- 4465254: removed the old `Main.qml`-based catalog and migrated to VisualCatalog, added AppBootstrap/AppEntry, expanded debug logger, and updated many tests/docs.
- f9100c0: converted event monitor data structure to `ListModel` for improved performance/manageability.
- 06478ff: refactored `LVRS` to `LVRSCore`, added event monitor functionality, and strengthened component integration.
- e8d405d: added many new QML controls, expanded RuntimeEvents/Backend, and added event pipeline/render policy docs.
- db3b178: made RuntimeEvents input-state tracking more detailed, expanded EventListener integration, and strengthened tests.
- 0cd35a3: replaced Design System Console with Runtime Event Daemon Console, adding real-time monitoring/filtering/summary features.
- dd3f32a: added Hierarchy components and button style consistency updates, plus broad RuntimeEvents/Alert/ContextMenu changes.
- 4d72b86: initial Hierarchy addition and related component/doc updates.
- 954bfd8: style refinements for TextEditor/ContextMenu/Alert and icon color standardization.
- 058d6fa: added Vulkan enforcement option and Qt Vulkan feature detection logic.
- 208c807: introduced Vulkan bootstrap utility and removed duplicated logic in main/examples.
- 7e12fc4: enforced Vulkan backend in main/examples and changed Apple GL link handling.
- 2ea2160: full namespace migration from `UIF` to `LV`.
- 8cb923f: refactored accent color naming in `Main.qml`/`Theme.qml` to clearer names.
- 92f8615: renamed `Accent` tone to `Primary` and updated related docs/examples/QML broadly.
- 1825f4b: renamed `accent` to `primary` in `Theme.qml` and expanded palette definitions.
- e706ad1: iconset additions and renames.
- 315dd82: large icon resource addition.

Key file touchpoints:
- `CMakeLists.txt`: Vulkan enforcement/validation options, integrated QML app composition functions, and runtime platform option cleanup.
- `cmake/LVRSHelpers.cmake`, `cmake/LVRSConfig.cmake.in`, `cmake/LVRSTargetsStatic.cmake.in`, `cmake/LVRSAppEntryPoint.cpp.in`: QML app automation and static plugin handling support.
- `backend/runtime/`: introduced `vulkanbootstrap`, added `appbootstrap`/`appentry`, and expanded `runtimeevents`.
- `qml/Main.qml`: switched to Runtime Event Console and significantly restructured monitoring UI.
- `qml/components/navigation/Hierarchy*.qml`: added Hierarchy tree navigation composition.
- `qml/components/control/*`, `qml/components/surfaces/*`: styling and behavior improvements for button/alert/input components.
- `resources/iconset/`: large new icon additions and existing icon renames.
- `docs/`: broad build/backend/component documentation updates and new docs.
- `install.sh`: expanded install workflow options and snapshot support.

Verification notes:
- Work was tracked using commit logs and diff stats, with per-commit changed files and change size reviewed.
- Because no automated test framework is globally configured, manual build/run validation may be required when needed (`cmake -S . -B build`, `cmake --build build`, `./build/LVRS`).
