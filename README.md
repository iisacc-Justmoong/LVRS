# LVRS

LVRS is a Qt 6.5+ UI framework focused on deterministic rendering, event observability, and reusable QML components.

The repository ships three layers together:
- A reusable static QML module (`LVRS`) with components and C++ singletons.
- Runnable example applications under `example/`, including the visual-catalog demo.
- Tests covering event flow, text editing behavior, import API, and backend wiring.

The framework target itself does not build an application executable; all runnable apps are example targets.

## Requirements

- CMake 3.21+
- C++20 compiler
- Qt 6.5+
- Qt modules: `Quick`, `QuickControls2`, `Qml`, `Svg`, `Network`
- Qt `Test` module only when `LVRS_BUILD_TESTS=ON`
- `LVRS_ENFORCE_VULKAN` validates fixed backend Qt features where applicable (macOS/iOS: Metal, Android: Vulkan). Windows prefers D3D11 during bootstrap and falls back to OpenGL after runtime probing, while Linux/WASM use Qt default backend selection.

## Quick Install (Clone -> Install -> Use)

```bash
git clone <LVRS_REPO_URL>
cd LVRS
./install.sh
```

`install.sh` is a wrapper around Rust CLI `lvrs install`.
If `cargo` exists, it runs `cargo run --manifest-path rust-cli/Cargo.toml --bin lvrs -- install ...`; otherwise it falls back to `lvrs install` from `PATH`.
Direct `lvrs install` now also reuses the installed source metadata under `<prefix>/src/LVRS` when launched outside the repository tree. If the recorded checkout path became stale because an upper directory was renamed, the CLI relocates the project root by matching the preserved trailing path segments before falling back to the installed source snapshot itself.
If `LVRS_ROOT` or `LVRS_PROJECT_ROOT` points at an installed prefix such as `~/.local/LVRS`, the CLI treats it as an install prefix and resolves the source through `<prefix>/src/LVRS`.
The install flow builds `bootstrap_lvrs_all`.
By default, the bootstrap platform set follows the current host:
- Linux: `linux`
- macOS: `macos;ios;android;wasm`
- Windows: `windows;android;wasm`
Platforms without a discoverable Qt kit are skipped.
Use `./install.sh --platforms linux,android,wasm` (comma/semicolon list) to override bootstrap/install platforms explicitly.
On Linux hosts, `lvrs install` now runs a CMake preflight first: it verifies the host C++ toolchain, Qt 6.5+ modules (`Quick`, `QuickControls2`, `Qml`, `Svg`, `Network`), and required Qt host tools, then auto-resolves `Qt6_DIR`/`LVRS_BOOTSTRAP_QT_PREFIX_LINUX` from common Qt layouts (Qt online installer, `qtpaths`/`qmake`, distro multiarch installs) when possible.
If Linux dependencies are missing and the distro package manager is recognized, the CLI prints the exact install command and can execute it with `./install.sh --install-linux-deps`.
Use `lvrs doctor --fix` for a host-only dependency precheck/fix pass, and `lvrs doctor --bootstrap [--with-wasm|--platforms ...]` to validate `main.cpp` bootstrap readiness plus cross-platform toolchain hint auto-detection before a full build; it exits non-zero when required hints/toolchains are still missing for the requested bootstrap targets.
Install layout remains `<prefix>/platforms/<platform>` (`macos`, `linux`, `windows`, `ios`, `android`, `wasm`).
Set `--prefix <path>` or `LVRS_INSTALL_PREFIX=<path>` to move the install root.
After install, `env.sh` points `CMAKE_PREFIX_PATH` to the install root (`<prefix>`) and `QML2_IMPORT_PATH` to the host platform package path.
`find_package(LVRS CONFIG REQUIRED)` then resolves the active platform package via LVRS dispatcher logic.
The installer always performs a clean reinstall (build directory and previously installed LVRS artifacts are removed before configure/build).
Use `./install.sh --without-examples --without-tests` to disable host configure-time example/test targets.
When host examples are enabled, the installer builds the `lvrs_host_examples_all` target first. Each build-tree example emits its executable under `build/example/<ExampleName>/bin`, and Linux builds now stage `bin/lvrs-runtime/` with the LVRS shared library plus QML module beside the executable. The checked-in `example/*/bin/LVRSExample*` paths are launcher scripts: inside the repository they fall back to `build/example/.../bin`, and inside the installed source snapshot they exec a sibling refreshed runtime (`*.real`). If `./install.sh --without-examples` is used, those snapshot runtime payloads are omitted.

## Build (Framework-First Default)

Configure:

```bash
cmake -S . -B build
```

Build:

```bash
cmake --build build -j
```

By default, LVRS builds as an installable framework package (no example app, no tests).

For repository-local development, prefer the root build helper:

```bash
./build.sh
```

`build.sh` configures `build/` with `LVRS_BUILD_EXAMPLES=ON` and `LVRS_BUILD_TESTS=ON`, builds the host outputs, and fails if any checked-in example launcher no longer matches its build-tree runtime.

## Build and Run Examples

```bash
./build.sh
```

Populate every example `bin/` directory in one pass:

```bash
./build.sh --without-tests
```

Run visual-catalog demo:

```bash
./example/VisualCatalog/bin/LVRSExampleVisualCatalog
```

The checked-in launcher resolves the build-tree executable from `build/example/VisualCatalog/bin/LVRSExampleVisualCatalog`. You can still run that build-tree executable directly if needed.

Run tests:

```bash
ctest --test-dir build --output-on-failure
```

## Use in Any Qt Quick Project

Install LVRS once:

```bash
cmake -S . -B build \
  -DLVRS_BUILD_EXAMPLES=OFF \
  -DLVRS_BUILD_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX=/path/to/lvrs-prefix
cmake --build build -j
cmake --install build
```

In your downstream app `CMakeLists.txt`:

```cmake
find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2)
find_package(LVRS CONFIG REQUIRED)

lvrs_add_qml_app(
    TARGET MyApp
    URI MyApp
    QML_FILES
        Main.qml
)
```

In your QML:

```qml
import QtQuick
import LVRS 1.0 as LV
```

Only CMake configure/build/install is required. Manual file copy or custom plugin wiring is not required.
`lvrs_configure_qml_app()` applies a safe default runtime output directory (`<build>/bin`) when none is set, auto-links/imports LVRS static QML plugin artifacts when the package is consumed as a static build, and on Linux stages `lvrs-runtime/` beside the executable with the LVRS shared library plus QML module. `runBootstrappedQmlApp()` automatically adds Linux runtime QML import candidates such as `lvrs-runtime/qml`, installed `../lib/qt6/qml`, and snapshot platform layouts before loading QML roots. `QmlAppLaunchSpec::initialProperties` is forwarded through `QQmlApplicationEngine::setInitialProperties(...)` immediately before the legacy single-root `loadFromModule(...)` path, while `QmlAppLaunchSpec::roots` can declare multiple `QmlRootLoadSpec` entries with per-root initial properties. Set `QmlAppLaunchSpec::windowActivationPolicy` or a root-specific policy to have LVRS call `show()`, `raise()`, and/or `requestActivate()` for loaded `QWindow` roots. `QmlAppLaunchSpec::lifecycle` adds `after-root-loaded`, `after-window-activated`, and zero-delay `after-first-idle` hooks/queue tasks for delayed bootstrap work. `ForegroundServiceGate` can start app foreground services once after a visible root window exists. `PermissionRequestSequencer` can run app-defined permission steps sequentially while storing request history and aggregate diagnostics. `BootstrapParallel` can fan out independent startup domains on worker threads and apply the collected results on a chosen QObject thread. `QmlContextBinder` can apply C++ context-property exposure plans and register dedicated C++ `ViewModel` instances into `LV.ViewModels` before QML is loaded. `QmlTypeRegistrar` can register app-owned QML types from a manifest while collecting duplicate and failure diagnostics. Qt runtime deployment itself remains a target-environment concern.
`lvrs_configure_qml_app()` now also generates platform runtime targets automatically: `run_<YourTarget>_macos`, `run_<YourTarget>_linux`, `run_<YourTarget>_windows`, `run_<YourTarget>_ios`, `run_<YourTarget>_android`, `run_<YourTarget>_wasm`.
On the configured host desktop platform, the matching runtime target directly launches the built executable; non-host targets provide an immediate reconfigure hint via `CMAKE_SYSTEM_NAME`.
`LV.ApplicationWindow` now includes adaptive layout policy properties:
- `scaffoldLayoutMode` (`auto`, `mobile`, `desktop`)
- `scaffoldLayoutPlatform` (platform token override; default `Qt.platform.os`)
- `scaffoldForceDesktopOnLargeMobile` + `scaffoldMobileDesktopMinWidth`
- `scaffoldPreferBottomNavigation` + `scaffoldBottomNavigationMaxItems`
- `scaffoldCompactSpacingEnabled` + `scaffoldCompactSpacingBreakpoint`
- `scaffoldNavRailMaxWidthRatio` + `scaffoldDrawerMarginSafety`
- runtime booleans: `adaptiveMobileLayout`, `adaptiveDesktopLayout`, `adaptiveRailNavigation`, `adaptiveDrawerNavigation`, `adaptiveBottomNavigation`
- `matchesMedia()` extra tokens: `mobile-layout`, `desktop-layout`, `rail-nav`, `drawer-nav`, `bottom-nav`
State is handled through page-stack routing (internal `LV.PageRouter` or injected `pageRouter`), while placement is handled through flex layout (`RowLayout`/`ColumnLayout`) inside `LV.ApplicationWindow`.
Page-stack API on `LV.ApplicationWindow`: `initialRoutePath`, `pageRoutes`, `pageInitialPath`, `useInternalPageStack`, `activePageRouter`, `pageStackNavigated`, `pageStackNavigationFailed`.
By default (`auto`), mobile platforms (`android`, `ios`) stay mobile-first even at wide widths and use bottom navigation when item count allows. `desktop-compact` also uses bottom navigation when item count fits the configured limit.
`LV.ApplicationWindow` is now the standard downstream app root. It carries the bootstrap contract directly: platform-profile-driven runtime attach, `useInternalPageStack: true`, `internalRouterRegisterAsGlobalNavigator: true`, `pageInitialPath` seeded from `initialRoutePath`, `mobileOversizedHeightEnabled: false`, and `navigationEnabled: false`. `LV.ApplicationWindow` and `LV.Window` default `forcedDeviceTierPreset` to auto-detect mode (`-1`), and mobile display-coverage/fullscreen overrides follow `Platform.runtimeProfile(...)` so Android keeps the edge-to-edge bootstrap path while iOS uses maximized edge-to-edge coverage instead of a forced fullscreen transition. `LV.AppBootstrapWindow` remains only as a compatibility wrapper that presets `visible: true`.
Root content is now full-bleed by default on mobile: `LV.ApplicationWindow` and `LV.Window` no longer auto-place downstream content inside `safeMargin`. `LV.ApplicationWindow` also drives the native fullscreen/client-area coverage hints needed to reach the iOS status-bar and home-indicator regions, and on iOS it does so through `Window.Maximized + MaximizeUsingFullscreenGeometryHint` so the system status indicators can remain visible. Use `layoutSafeAreaBounds` as a helper rect or bind `LV.WindowSafeAreaObserver` when the app wants to reserve the actual iOS status-bar/notch/home-indicator insets explicitly.
In addition, LVRS generates bootstrap targets for cross-platform output/installation:
- `bootstrap_<YourTarget>_macos`
- `bootstrap_<YourTarget>_linux`
- `bootstrap_<YourTarget>_windows`
- `bootstrap_<YourTarget>_ios`
- `bootstrap_<YourTarget>_android`
- `bootstrap_<YourTarget>_wasm`
- `bootstrap_<YourTarget>_all`
LVRS also generates launch/export convenience targets:
- `launch_<YourTarget>_ios`
- `launch_<YourTarget>_android`
- `launch_<YourTarget>_wasm`
- `export_<YourTarget>_xcodeproj`
- `export_<YourTarget>_android_studio`
- `export_<YourTarget>_wasm_site`
`bootstrap_*` targets configure isolated per-platform build trees under `<build>/lvrs-bootstrap/<target>/...`, build the app target, then:
- desktop targets emit executable artifact paths (`macOS`/`Linux` binaries, `Windows .exe`). Linux app targets built through `lvrs_add_qml_app()` also stage `lvrs-runtime/` next to the executable so the LVRS shared library is resolved through a relative `RPATH`.
- `ios` generates an Xcode project by default and installs the built `.app` to the iOS Simulator via `xcrun simctl`
- `android` generates an Android Studio (Gradle) project by default and installs the built `.apk` to emulator/device via `adb`
- `wasm` emits browser artifacts (`.html`, `.js`, `.wasm`) in the wasm bootstrap build tree and writes `LVRSWasmArtifact.cmake` entry metadata
- `launch_<YourTarget>_wasm` serves the wasm build tree with a local static HTTP server and optionally opens the browser
- `export_<YourTarget>_wasm_site` collects wasm web assets recursively (including nested output layouts) and writes an `index.html` redirect to the detected app entry
Any platform without a discoverable Qt kit is skipped with a configure-time status message, and its matching `bootstrap_`, `launch_`, and `export_` targets are omitted from the generated target graph.
Override paths/toolchains with `LVRS_BOOTSTRAP_QT_PREFIX_<PLATFORM>` and `LVRS_BOOTSTRAP_TOOLCHAIN_FILE_<PLATFORM>` (`PLATFORM`: `MACOS`, `LINUX`, `WINDOWS`, `IOS`, `ANDROID`, `WASM`).
Project-generation defaults can be controlled with `LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT` and `LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT`.
Android Studio output path can be overridden with `LVRS_ANDROID_STUDIO_PROJECT_DIR`.
`androiddeployqt` lookup can be pinned with `LVRS_BOOTSTRAP_ANDROIDDEPLOYQT` (or `LVRS_BOOTSTRAP_QT_HOST_PREFIX`).
Android SDK/NDK auto-detection can be overridden with `LVRS_BOOTSTRAP_ANDROID_SDK_ROOT` and `LVRS_BOOTSTRAP_ANDROID_NDK`.
WASM launch behavior can be overridden with `LVRS_BOOTSTRAP_WASM_HOST`, `LVRS_BOOTSTRAP_WASM_PORT`, `LVRS_BOOTSTRAP_WASM_OPEN_BROWSER`.
`LVRS_DIR` and package-registry policy cache values are forwarded automatically to bootstrap reconfigure.
When LVRS is consumed from an installed multi-platform prefix, bootstrap resolves `LVRS_DIR` to the target platform package directory automatically (for example `<prefix>/platforms/ios/lib/cmake/LVRS`) so cross toolchains do not rely on root-prefix package discovery.
LVRS package config exports toolchain hint variables for downstream scripts:
- `LVRS_INSTALL_ROOT`
- `LVRS_QT_HOST_PREFIX_HINT`
- `LVRS_QT_IOS_PREFIX_HINT`
- `LVRS_QT_ANDROID_PREFIX_HINT`
- `LVRS_QT_WASM_PREFIX_HINT`
- `LVRS_ANDROID_SDK_HINT`
- `LVRS_ANDROID_NDK_HINT`
- `LVRS_EMSDK_HINT`
If a manual cross-platform configure still bypasses the root dispatcher because the toolchain roots package search paths, set `LVRS_DIR` directly to `<prefix>/platforms/<platform>/lib/cmake/LVRS`.
Example:
```bash
cmake --build build --target bootstrap_MyApp_all
```
`lvrs_add_qml_app()` further reduces bootstrap overhead by auto-generating an app entrypoint when `SOURCES` is omitted.
Project-wide platform defaults can be configured through:
```cmake
lvrs_configure_project_defaults(
    TARGET MyApp
    APPLE_BUNDLE_ID com.example.myapp
    APPLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/platform/apple/Info.plist
    APPLE_ENTITLEMENTS ${CMAKE_SOURCE_DIR}/platform/apple/MyApp.entitlements
    ANDROID_PACKAGE_ID com.example.myapp
    ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/platform/android/package
    IOS_EXCLUDE_QMLTOOLING
)
```

For framework-only multi-platform install, LVRS also generates:
- `bootstrap_lvrs_macos`
- `bootstrap_lvrs_linux`
- `bootstrap_lvrs_windows`
- `bootstrap_lvrs_ios`
- `bootstrap_lvrs_android`
- `bootstrap_lvrs_wasm`
- `bootstrap_lvrs_all`
`bootstrap_lvrs_*` targets configure isolated per-platform build trees under `<build>/lvrs-bootstrap/framework/...`, build `LVRSCore`, and install each platform package into `${LVRS_BOOTSTRAP_INSTALL_ROOT}/<platform>` (default: `<build>/lvrs-install/<platform>`).
Nested framework builds run with `--parallel 1` and clear inherited Make/jobserver parallel environment (`MAKEFLAGS`, `MFLAGS`, and `CMAKE_BUILD_PARALLEL_LEVEL`) before invoking the per-platform build.
Default framework bootstrap platform set is all runtime platforms unless `LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS` is set.
Any platform without a matching Qt kit is skipped with a configure-time status message.
Per-platform install prefixes can be overridden with `LVRS_BOOTSTRAP_INSTALL_PREFIX_<PLATFORM>`.
Cross-host targets (`linux`, `windows`, `android`, `ios`, `wasm`) require matching Qt kits and toolchains; set `LVRS_BOOTSTRAP_QT_PREFIX_<PLATFORM>` and `LVRS_BOOTSTRAP_TOOLCHAIN_FILE_<PLATFORM>` as needed.

## Rendering Backend Policy

At runtime, graphics backend selection is bootstrapped through `backend/runtime/appbootstrap.*` from each app entrypoint.

- macOS/iOS: Metal is fixed.
- Windows: D3D11 is preferred, the runtime is probed first, and startup falls back to OpenGL when DirectX cannot be initialized during bootstrap.
- Android: Vulkan is preferred, runtime loader availability is probed first, and startup falls back to OpenGL when Vulkan cannot be initialized during bootstrap.
- Linux: Qt default backend selection is used.
- WASM/other platforms: Qt default backend selection is used as fallback.
- If a required fixed backend cannot be initialized and no platform fallback exists, app startup fails fast with a clear error message.

Bootstrap render defaults are also applied conservatively before the first window is created. iOS/Android use a lower MSAA profile and reduced atlas sizing than desktop targets, while WASM uses a lighter single-frame bootstrap profile with partial-update, batch-renderer, and pipeline-cache hints disabled.

Build-time backend enforcement is controlled by:
- `LVRS_ENFORCE_VULKAN` (default `ON`)

Build-time optimization policy is controlled by:
- `LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS` (default `ON`)
- `LVRS_ENABLE_IPO` (default `ON`)

Both application and framework bootstrap propagation preserve explicit `OFF` values for these options. Empty values alone are omitted from a nested platform configure.

When enabled, configure fails if:
- The platform-fixed backend requirements are not satisfied (`QT_FEATURE_metal` for macOS/iOS, `QT_FEATURE_vulkan` for Android).

## Project Layout

- `backend/`: C++ singletons (`RuntimeEvents`, `Backend`, `RenderMonitor`, `RenderQuality`, etc.).
- `backend/runtime/appbootstrap.h`, `backend/runtime/appbootstrap.cpp`: reusable pre/post app bootstrap API for downstream apps.
- `qml/`: QML module entry files and components.
- `main.cpp`: downstream app template entrypoint (reference only, not built by framework CMake targets). CLI/env overrides can inject `module/root/app-name/style`.
- `example/VisualCatalog/main.cpp`: visual-catalog app entrypoint, backend bootstrap, font loading.
- `example/VisualCatalog/qml/Main.qml`: visual catalog with tab pages and EventListener runtime console.
- `resources/iconset/`: SVG icon source set used for theme accent extraction. Most icons in this set are sourced from JetBrains Int Icons (IntelliJ Platform Icons).
- `tests/`: Qt tests for components and runtime services.
- `docs/`: full technical documentation index.

## Event and Input Architecture

The event system now centers on a daemon-style flow:

1. `RuntimeEvents` installs a global event filter and records input/UI lifecycle events, including native touch point detail.
2. `Backend.hookUserEvents()` subscribes to the runtime stream and keeps a bounded cache.
3. `GestureEvents` classifies mobile press, scroll, hold, drag, swipe, and native gesture payloads.
4. `EventListener` consumes backend state first (`currentUserInputState()`), then runtime fallback.
5. `ApplicationWindow` emits `globalPressedEvent` and `globalContextEvent`.
6. UI features such as `ContextMenu`, runtime console, and hierarchy scroll guards are driven from this unified stream.

## Main Visual Catalog

`example/VisualCatalog/qml/Main.qml` is a tab-oriented design-system console with dedicated pages for:
- Overview
- Typography
- EventListener console
- Buttons
- Accent tokens
- Inputs / Editors
- Checks
- Navigation
- Layout
- Hierarchy
- Scaffold

The runtime console section exposes daemon health, event sequence, pointer target, pressed keys/buttons, and recent route/render events.

## Documentation

Start at:
- `docs/README.md`

Key references:
- `docs/architecture/event-pipeline.md`
- `docs/architecture/rendering-backend.md`
- `docs/backend/RuntimeEvents.md`
- `docs/backend/Backend.md`
- `docs/components/control/EventListener.md`
- `docs/components/navigation/ContextMenu.md`
- `docs/components/navigation/Hierarchy.md`
- `docs/components/control/InputMethodGuard.md`
