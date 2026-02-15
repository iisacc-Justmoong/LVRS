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
- Vulkan SDK/runtime available to CMake when `LVRS_ENFORCE_VULKAN=ON` (default)

## Quick Install (Clone -> Install -> Use)

```bash
git clone <LVRS_REPO_URL>
cd LVRS
./install.sh
```

`install.sh` now performs a single multi-platform bootstrap build (`bootstrap_lvrs_all`) and installs LVRS for all runtime platforms in one run.
Default install layout is `<prefix>/platforms/<platform>` for `macos`, `linux`, `windows`, `ios`, `android`, `wasm`.
After install, `env.sh` points `CMAKE_PREFIX_PATH` to the install root (`<prefix>`) and `QML2_IMPORT_PATH` to the host platform package path.
`find_package(LVRS CONFIG REQUIRED)` then resolves the active platform package via LVRS dispatcher logic.
The installer always performs a clean reinstall (build directory and previously installed LVRS artifacts are removed before configure/build).
Use `./install.sh --without-examples --without-tests` to disable host configure-time example/test targets.

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

## Build and Run Examples

```bash
cmake -S . -B build-dev \
  -DLVRS_BUILD_EXAMPLES=ON \
  -DLVRS_BUILD_TESTS=ON
cmake --build build-dev -j
```

Run visual-catalog demo:

```bash
cmake --build build-dev --target LVRSExampleVisualCatalog
./build-dev/bin/LVRSExampleVisualCatalog
```

Run tests:

```bash
ctest --test-dir build-dev --output-on-failure
```

## Use in Any Qt Quick Project

Install LVRS once:

```bash
cmake -S . -B build-install \
  -DLVRS_BUILD_EXAMPLES=OFF \
  -DLVRS_BUILD_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX=/path/to/lvrs-prefix
cmake --build build-install -j
cmake --install build-install
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
`lvrs_configure_qml_app()` applies a safe default runtime output directory (`<build>/bin`) when none is set, and auto-links/imports LVRS static QML plugin artifacts when the package is consumed as a static build.
`lvrs_configure_qml_app()` now also generates platform runtime targets automatically: `run_<YourTarget>_macos`, `run_<YourTarget>_linux`, `run_<YourTarget>_windows`, `run_<YourTarget>_ios`, `run_<YourTarget>_android`, `run_<YourTarget>_wasm`.
On the configured host desktop platform, the matching runtime target directly launches the built executable; non-host targets provide an immediate reconfigure hint via `CMAKE_SYSTEM_NAME`.
`LV.ApplicationWindow`/`LV.AppScaffold` now include adaptive layout policy properties:
- `scaffoldLayoutMode` (`auto`, `mobile`, `desktop`)
- `scaffoldLayoutPlatform` (platform token override; default `Qt.platform.os`)
- `scaffoldForceDesktopOnLargeMobile` + `scaffoldMobileDesktopMinWidth`
- `scaffoldPreferBottomNavigation` + `scaffoldBottomNavigationMaxItems`
- runtime booleans: `adaptiveMobileLayout`, `adaptiveDesktopLayout`, `adaptiveRailNavigation`, `adaptiveDrawerNavigation`, `adaptiveBottomNavigation`
- `matchesMedia()` extra tokens: `mobile-layout`, `desktop-layout`, `rail-nav`, `drawer-nav`, `bottom-nav`
State is handled through page-stack routing (internal `LV.PageRouter` or injected `pageRouter`), while placement is handled through flex layout (`RowLayout`/`ColumnLayout`) inside `LV.AppScaffold`.
Page-stack API on `LV.ApplicationWindow`: `pageRoutes`, `pageInitialPath`, `useInternalPageStack`, `activePageRouter`, `pageStackNavigated`, `pageStackNavigationFailed`.
By default (`auto`), mobile platforms (`android`, `ios`) stay mobile-first even at wide widths and use bottom navigation when item count allows.
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
- desktop targets emit executable artifact paths (`macOS`/`Linux` binaries, `Windows .exe`)
- `ios` generates an Xcode project by default and installs the built `.app` to the iOS Simulator via `xcrun simctl`
- `android` generates an Android Studio (Gradle) project by default and installs the built `.apk` to emulator/device via `adb`
- `wasm` emits browser artifacts (`.html`, `.js`, `.wasm`) in the wasm bootstrap build tree and writes `LVRSWasmArtifact.cmake` entry metadata
- `launch_<YourTarget>_wasm` serves the wasm build tree with a local static HTTP server and optionally opens the browser
- `export_<YourTarget>_wasm_site` collects wasm web assets recursively (including nested output layouts) and writes an `index.html` redirect to the detected app entry
Override paths/toolchains with `LVRS_BOOTSTRAP_QT_PREFIX_<PLATFORM>` and `LVRS_BOOTSTRAP_TOOLCHAIN_FILE_<PLATFORM>` (`PLATFORM`: `MACOS`, `LINUX`, `WINDOWS`, `IOS`, `ANDROID`, `WASM`).
Project-generation defaults can be controlled with `LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT` and `LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT`.
Android Studio output path can be overridden with `LVRS_ANDROID_STUDIO_PROJECT_DIR`.
`androiddeployqt` lookup can be pinned with `LVRS_BOOTSTRAP_ANDROIDDEPLOYQT` (or `LVRS_BOOTSTRAP_QT_HOST_PREFIX`).
Android SDK/NDK auto-detection can be overridden with `LVRS_BOOTSTRAP_ANDROID_SDK_ROOT` and `LVRS_BOOTSTRAP_ANDROID_NDK`.
WASM launch behavior can be overridden with `LVRS_BOOTSTRAP_WASM_HOST`, `LVRS_BOOTSTRAP_WASM_PORT`, `LVRS_BOOTSTRAP_WASM_OPEN_BROWSER`.
`LVRS_DIR` and package-registry policy cache values are forwarded automatically to bootstrap reconfigure.
LVRS package config exports toolchain hint variables for downstream scripts:
- `LVRS_QT_HOST_PREFIX_HINT`
- `LVRS_QT_IOS_PREFIX_HINT`
- `LVRS_QT_ANDROID_PREFIX_HINT`
- `LVRS_QT_WASM_PREFIX_HINT`
- `LVRS_ANDROID_SDK_HINT`
- `LVRS_ANDROID_NDK_HINT`
- `LVRS_EMSDK_HINT`
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
Per-platform install prefixes can be overridden with `LVRS_BOOTSTRAP_INSTALL_PREFIX_<PLATFORM>`.
Cross-host targets (`linux`, `windows`, `android`, `ios`, `wasm`) require matching Qt kits and toolchains; set `LVRS_BOOTSTRAP_QT_PREFIX_<PLATFORM>` and `LVRS_BOOTSTRAP_TOOLCHAIN_FILE_<PLATFORM>` as needed.

## Rendering Backend Policy

At runtime, graphics backend selection is bootstrapped through `backend/runtime/appbootstrap.*` from each app entrypoint.

- macOS/iOS: Metal is fixed.
- Windows/Linux/Android: Vulkan is fixed and runtime loader availability is validated on desktop Vulkan platforms.
- Other platforms: Qt default backend selection is used as fallback.
- If a fixed backend cannot be initialized, app startup fails fast with a clear error message.

Build-time backend enforcement is controlled by:
- `LVRS_ENFORCE_VULKAN` (default `ON`)

When enabled, configure fails if:
- The platform-fixed backend requirements are not satisfied (`QT_FEATURE_metal` for macOS/iOS, `QT_FEATURE_vulkan` and Vulkan runtime for Windows/Linux/Android).

## Project Layout

- `backend/`: C++ singletons (`RuntimeEvents`, `Backend`, `RenderMonitor`, `RenderQuality`, etc.).
- `backend/runtime/appbootstrap.h`, `backend/runtime/appbootstrap.cpp`: reusable pre/post app bootstrap API for downstream apps.
- `qml/`: QML module entry files and components.
- `main.cpp`: downstream app template entrypoint (reference only, not built by framework CMake targets). CLI/env overrides can inject `module/root/app-name/style`.
- `example/VisualCatalog/main.cpp`: visual-catalog app entrypoint, backend bootstrap, font loading.
- `example/VisualCatalog/qml/Main.qml`: visual catalog with tab pages and EventListener runtime console.
- `resources/iconset/`: SVG icon source set used for theme accent extraction.
- `tests/`: Qt tests for components and runtime services.
- `docs/`: full technical documentation index.

## Event and Input Architecture

The event system now centers on a daemon-style flow:

1. `RuntimeEvents` installs a global event filter and records input/UI lifecycle events.
2. `Backend.hookUserEvents()` subscribes to the runtime stream and keeps a bounded cache.
3. `EventListener` consumes backend state first (`currentUserInputState()`), then runtime fallback.
4. `ApplicationWindow` emits `globalPressedEvent` and `globalContextEvent`.
5. UI features such as `ContextMenu`, runtime console, and hierarchy scroll guards are driven from this unified stream.

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
