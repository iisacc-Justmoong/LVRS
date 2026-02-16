# Repository Guidelines

## Project Structure & Module Organization
- `main.cpp` boots the Qt application and loads the QML module entrypoint.
- `backend/` holds C++ singletons and helpers exposed to QML (e.g., `Backend`, `Platform`, `RenderMonitor`).
- `qml/` holds UI source. `qml/Main.qml` is the app window root.
- `qml/components/` is split by concern:
  - `buttons/` (e.g., `AbstractButton.qml`, `LabelButton.qml`)
  - `layout/` (e.g., `VStack.qml`, `HStack.qml`)
  - `navigation/` (e.g., `PageRouter.qml`, `Link.qml`)
  - `surfaces/` (e.g., `AppCard.qml`)
- `CMakeLists.txt` includes per-directory `CMakeLists.txt` to assemble sources and QML files.
- `build/` is the local build output folder (generated; do not edit by hand).

## Build, Test, and Development Commands
- Configure: `cmake -S . -B build`  
  Generates build files in `./build`.
- Build: `cmake --build build`  
  Compiles the C++ target and QML module.
- Run: `./build/LVRS`  
  Launches the Qt Quick UI.

_No automated tests are set up yet._

## Coding Style & Naming Conventions
- Indentation: 4 spaces for C++ and QML (match existing files).
- QML components: `PascalCase` filenames (e.g., `AppHeader.qml`).
- Properties and IDs: `camelCase` (e.g., `headerTitle`, `contentWrap`).
- Keep UI colors and spacing centralized within components when practical.
- Prefer Qt Quick Controls 2 types (`ApplicationWindow`, `ToolBar`, `Drawer`).
- QML components include a short API usage comment at the end of each file.

## Navigation & Routing
- `PageRouter` (QML singleton) provides StackView-based routing with Svelte-like paths:
  - static: `/reports`
  - param: `/runs/[id]`
  - rest: `/logs/[...path]`
- `Link` component mimics HTML `<a>`: set `href` and `router`, wrap child content.
- `ApplicationWindow` directly drives adaptive navigation and can route when `navItems` include `path` and `pageRouter` is set.

## Backend Notes
- `RenderMonitor` (QML singleton) attaches to a `QQuickWindow` to report FPS and frame timing.

## Testing Guidelines
- No test framework is configured. If you add tests, document:
  - Framework choice (e.g., Qt Test).
  - How to run them (`ctest`, etc.).
  - Naming conventions (e.g., `test_*.cpp`).

## Commit & Pull Request Guidelines
- This repository is not currently a Git repo; no commit history exists.
- If you initialize Git, use clear, action‑oriented commit messages (e.g., "Refine adaptive layout behavior").
- For PRs, include:
  - A short summary of UI/behavior changes.
  - Screenshots or recordings for UI updates.
  - Any build or platform notes (Qt version, OS).

## Configuration Notes
- Requires Qt 6.5+ with `QtQuick` and `QtQuickControls2`.
- Keep generated build artifacts out of source control (e.g., `build/`).
