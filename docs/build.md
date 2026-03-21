# Build and Runtime Setup

## Quick Install

```bash
./install.sh
```

`install.sh` is now a thin wrapper around Rust CLI `lvrs install`.
- If `cargo` is available, it runs `cargo run --manifest-path rust-cli/Cargo.toml --bin lvrs -- install ...`.
- If `cargo` is not available but `lvrs` exists in `PATH`, it runs `lvrs install ...`.
- If neither is available, install exits with guidance to build CLI first.
- Direct `lvrs install` can recover the repository root from `<prefix>/src/LVRS/INSTALL_SOURCE_INFO.txt` when launched outside the checkout. If the stored absolute path went stale after an upper-directory rename, the CLI re-locates the root by matching trailing path segments and otherwise falls back to the installed source snapshot in place.

The install flow builds `bootstrap_lvrs_all`.
By default, the framework bootstrap platform set follows the current host:
- Linux: `linux`
- macOS: `macos;ios;android;wasm`
- Windows: `windows;android;wasm`
Platforms without a matching Qt kit are skipped during bootstrap target generation.
Use `./install.sh --platforms linux,android,wasm` (comma/semicolon list) to constrain/override the platform set.
On Linux hosts, the installer runs a dependency preflight before cleaning/building. It validates the host C++ toolchain, required Qt 6.5+ modules, and Qt host tools, then auto-resolves `Qt6_DIR`/`LVRS_BOOTSTRAP_QT_PREFIX_LINUX` from common Qt layouts when available.
If those Linux dependencies are missing and the distro package manager is recognized, the CLI prints the exact install command and can execute it with `./install.sh --install-linux-deps`.
`lvrs doctor --fix` runs the same host-side dependency check/fix flow without starting an install. `lvrs doctor --bootstrap [--with-wasm|--platforms ...]` additionally validates the `main.cpp` bootstrap entry markers and reports any missing cross-platform Qt/Android/WASM auto-detect hints; it exits non-zero when the requested bootstrap target set is not ready.
Installed packages are written to `<prefix>/platforms/<platform>` (`macos`, `linux`, `windows`, `ios`, `android`, `wasm`), then the host platform path is registered in the CMake user package registry.
Set `--prefix <path>` or `LVRS_INSTALL_PREFIX=<path>` to change the install root.
The installer always performs a clean reinstall by removing the previous build directory and installed LVRS artifact paths before configuring.
`install.sh` configures examples/tests on the host build by default; pass `--without-examples --without-tests` to disable them.
When host examples are enabled, the installer builds the `lvrs_host_examples_all` target first. Each build-tree example emits its executable under `build/example/<ExampleName>/bin`; Linux builds additionally stage `bin/lvrs-runtime/` with the LVRS shared library plus QML module beside the executable. The checked-in `example/*/bin/LVRSExample*` paths are launcher scripts: repository launchers fall back to `build/example/.../bin`, while installed source snapshots receive refreshed desktop runtimes as sibling `*.real` files beside those launchers. If `--without-examples` is used, those snapshot runtime payloads are removed.

## Rust CLI Entry Points

Direct CLI invocation (without wrapper):

```bash
cargo run --manifest-path rust-cli/Cargo.toml --bin lvrs -- install
```

Main-entrypoint bootstrap profile:

```bash
cargo run --manifest-path rust-cli/Cargo.toml --bin lvrs -- bootstrap
```

`lvrs bootstrap` defaults to a host-matched target set unless `--platforms` is provided:
- Linux host: `linux`
- macOS host: `macos;ios;android`
- Windows host: `windows;android`
`--with-wasm` appends `wasm` to that host default set.
Before running, it validates `main.cpp` contains the expected LVRS bootstrap entry markers (`runBootstrappedQmlApp`, `rootObject = QStringLiteral("Main")`).

## Configure

```bash
cmake -S . -B build
```

## Build

```bash
cmake --build build -j
```

For repository-local development, prefer the root helper:

```bash
./build.sh
```

`build.sh` configures `build/` with `LVRS_BUILD_EXAMPLES=ON` and `LVRS_BUILD_TESTS=ON`, builds the host outputs, and aborts if the checked-in `example/*/bin/LVRSExample*` launchers no longer match their build-tree runtimes.

## Run

```bash
./build.sh
./example/VisualCatalog/bin/LVRSExampleVisualCatalog
```

Build every host example and populate all `build/example/*/bin` directories:

```bash
./build.sh --without-tests
```

The visual-catalog example target (`LVRSExampleVisualCatalog`) emits the executable as `build/example/VisualCatalog/bin/LVRSExampleVisualCatalog`. The checked-in launcher at `example/VisualCatalog/bin/LVRSExampleVisualCatalog` resolves that build-tree executable during in-repository development and a sibling snapshot runtime after install.
The framework library target (`LVRSCore`) itself does not emit a runnable app.

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Build Options

- `LVRS_BUILD_EXAMPLES` (`OFF`): build runnable examples.
- `LVRS_BUILD_TESTS` (`OFF`): build and register tests.
- `LVRS_INSTALL_QML_MODULE` (`ON`): install QML module artifacts (`qmldir`, qmltypes, plugin, QML files) under `<prefix>/lib/qt6/qml/LVRS`.
- `LVRS_ENFORCE_VULKAN` (`ON`): fail CMake configure when fixed graphics backend Qt feature requirements are missing for platforms that require feature-gated backends.
- `LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS` (`ON`): apply platform-specific release/relwithdebinfo/minsizerel compile+link optimization flags.
- `LVRS_ENABLE_IPO` (`ON`): enable interprocedural optimization (LTO) for release-like configs when toolchain support is available.
- `LVRS_SANITIZER` (`none`): sanitizer instrumentation (`none`, `address`, `thread`, `undefined`).
- `LVRS_FORCE_X86_QT_TOOLS` (`OFF`): run Qt host tools through Rosetta when required.
- `LVRS_ENABLE_FRAMEWORK_BOOTSTRAP_TARGETS` (`ON`): generate `bootstrap_lvrs_*` framework multi-platform targets.
- `LVRS_BOOTSTRAP_INSTALL_ROOT` (`<build>/lvrs-install`): install root used by `bootstrap_lvrs_*`.

## Downstream CMake Integration

Install LVRS into a prefix first:

```bash
cmake -S . -B build \
  -DLVRS_BUILD_EXAMPLES=OFF \
  -DLVRS_BUILD_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX=/path/to/lvrs-prefix
cmake --build build -j
cmake --install build
```

Then, in any Qt Quick project:

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

Set `CMAKE_PREFIX_PATH` to the install root (`/path/to/lvrs-prefix`) when configuring the downstream project.
`lvrs_configure_qml_app()` sets `QT_QML_IMPORT_PATH` for installed-package consumption, applies a default executable output directory (`<build>/bin`) when unset, auto-links/imports LVRS static QML plugin artifacts for static package builds, and on Linux stages `lvrs-runtime/` beside the executable with the LVRS shared library plus QML module. `runBootstrappedQmlApp()` automatically probes Linux runtime QML import locations such as `lvrs-runtime/qml`, installed `../lib/qt6/qml`, and snapshot platform layouts before loading the root object. `QmlAppLaunchSpec::initialProperties` is applied through `QQmlApplicationEngine::setInitialProperties(...)` immediately before `loadFromModule(...)`, so route or window startup state can be decided in C++ before the first frame. Qt runtime deployment itself remains target-environment specific.
`LV.ApplicationWindow` provides adaptive layout policy APIs for mobile/desktop reordering:
- `scaffoldLayoutMode` (`auto`, `mobile`, `desktop`)
- `scaffoldLayoutPlatform` override (default canonical platform token; aliases are normalized through `Platform.normalizeTarget()`)
- `scaffoldForceDesktopOnLargeMobile` + `scaffoldMobileDesktopMinWidth`
- `scaffoldPreferBottomNavigation` + `scaffoldBottomNavigationMaxItems`
- `scaffoldCompactSpacingEnabled` + `scaffoldCompactSpacingBreakpoint`
- `scaffoldNavRailMaxWidthRatio` + `scaffoldDrawerMarginSafety`
- runtime state flags: `adaptiveMobileLayout`, `adaptiveDesktopLayout`, `adaptiveRailNavigation`, `adaptiveDrawerNavigation`, `adaptiveBottomNavigation`
- `matchesMedia()` tokens: `mobile-layout`, `desktop-layout`, `rail-nav`, `drawer-nav`, `bottom-nav`
State uses page-stack routing (`LV.PageRouter`), and placement uses flex layout (`RowLayout`/`ColumnLayout`) inside `LV.ApplicationWindow`.
`LV.ApplicationWindow` page-stack API: `initialRoutePath`, `pageRoutes`, `pageInitialPath`, `useInternalPageStack`, `activePageRouter`, `pageStackNavigated`, `pageStackNavigationFailed`.
Default `auto` mode is mobile-first for canonical mobile targets and their normalized aliases (`android`, `android-arm64`, `ios`, `ios-simulator`, ...), and prevents wide-screen mobile windows from being forced into desktop rail layout unless explicitly configured. `desktop-compact` profile also selects bottom navigation when item count fits the configured limit.
Recommended app-root bootstrap profile for mobile/desktop single-project apps:

```cpp
lvrs::QmlAppLaunchSpec launchSpec;
launchSpec.bootstrap.applicationName = QStringLiteral("MyApp");
launchSpec.bootstrap.quickStyleName = QStringLiteral("Basic");
launchSpec.moduleUri = QStringLiteral("MyApp");
launchSpec.rootObject = QStringLiteral("Main");
launchSpec.initialProperties = QVariantMap{
    {QStringLiteral("initialRoutePath"), QStringLiteral("/")},
    {QStringLiteral("bootstrapTitle"), QStringLiteral("MyApp")}
};
```

```qml
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    visible: true
    pageRoutes: [
        { path: "/", component: homePage }
    ]

    Component {
        id: homePage
        Item {}
    }
}
```
`LV.ApplicationWindow` is the reusable downstream root for the imported bootstrap profile. It now owns platform-profile-driven runtime attach, global navigator registration, and internal page-stack initialization from `initialRoutePath` directly. `LV.ApplicationWindow` and `LV.Window` default `forcedDeviceTierPreset` to `-1`, which keeps automatic device-tier detection enabled unless a downstream app explicitly pins a preset. On iOS and Android, `LV.ApplicationWindow` also defaults to OS-managed mobile windowing/insets, so fullscreen transitions and content-root stretching are no longer forced unless a downstream app explicitly opts back into the legacy coverage overrides. `LV.AppBootstrapWindow` remains available as a compatibility wrapper when an existing codebase still wants the old type name plus `visible: true`.
It also creates cross-platform runtime targets automatically:
- `run_<target>_macos`
- `run_<target>_linux`
- `run_<target>_windows`
- `run_<target>_ios`
- `run_<target>_android`
- `run_<target>_wasm`
The host desktop target launches immediately, while non-host targets print a `CMAKE_SYSTEM_NAME` reconfigure hint.
It also creates cross-platform bootstrap targets:
- `bootstrap_<target>_macos`
- `bootstrap_<target>_linux`
- `bootstrap_<target>_windows`
- `bootstrap_<target>_ios`
- `bootstrap_<target>_android`
- `bootstrap_<target>_wasm`
- `bootstrap_<target>_all`
It also creates launch/export convenience targets:
- `launch_<target>_ios`
- `launch_<target>_android`
- `launch_<target>_wasm`
- `export_<target>_xcodeproj`
- `export_<target>_android_studio`
- `export_<target>_wasm_site`
`bootstrap_<target>_all` triggers all platform bootstrap actions in one build invocation.
Desktop bootstrap targets produce executable artifacts. Linux app targets built through `lvrs_add_qml_app()` stage `lvrs-runtime/` beside the executable so the LVRS shared library resolves through a relative `RPATH`.
iOS bootstrap generates an Xcode project by default and installs a simulator app via `xcrun simctl`.
Android bootstrap generates an Android Studio (Gradle) project by default and installs an APK via `adb`.
WASM bootstrap emits browser artifacts and writes `LVRSWasmArtifact.cmake` entry metadata in the wasm bootstrap build tree.
`launch_<target>_wasm` serves the wasm build tree via a local static HTTP server and can auto-open a browser.
`export_<target>_wasm_site` recursively collects wasm web assets (nested layout-safe) and generates an `index.html` redirect to the detected entry.
Any platform without a discoverable Qt kit is skipped with a configure-time status message, and its related `bootstrap_`, `launch_`, and `export_` targets are not generated.
Toolchain/prefix overrides:
- `LVRS_BOOTSTRAP_QT_PREFIX_<PLATFORM>`
- `LVRS_BOOTSTRAP_QT_HOST_PREFIX` (host Qt prefix for Android deploy tooling lookup)
- `LVRS_BOOTSTRAP_TOOLCHAIN_FILE_<PLATFORM>`
- `LVRS_BOOTSTRAP_GENERATOR_<PLATFORM>`
- `LVRS_BOOTSTRAP_GENERATE_IOS_XCODE_PROJECT` (default `ON` for iOS bootstrap)
- `LVRS_BOOTSTRAP_GENERATE_ANDROID_STUDIO_PROJECT` (default `ON` for Android bootstrap)
- `LVRS_ANDROID_STUDIO_PROJECT_DIR` (default: `<platform-build>/android-studio`)
- `LVRS_BOOTSTRAP_ANDROIDDEPLOYQT` (explicit path override for `androiddeployqt`)
- `LVRS_BOOTSTRAP_ANDROID_SDK_ROOT` / `LVRS_BOOTSTRAP_ANDROID_NDK` (Android SDK/NDK explicit override)
- `LVRS_BOOTSTRAP_WASM_HOST` / `LVRS_BOOTSTRAP_WASM_PORT` / `LVRS_BOOTSTRAP_WASM_OPEN_BROWSER` (WASM launch server/browser behavior)
- `LVRS_IOS_SIMULATOR_NAME` (default: `iPhone 17 Pro`)
- `LVRS_ANDROID_EMULATOR_SERIAL` (default: `emulator-5554`)
- `LVRS_BOOTSTRAP_LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS` (propagate `LVRS_ENABLE_PLATFORM_BUILD_OPTIMIZATIONS` into bootstrap reconfigure)
- `LVRS_BOOTSTRAP_LVRS_ENABLE_IPO` (propagate `LVRS_ENABLE_IPO` into bootstrap reconfigure)
`LVRS_DIR` and package-registry policy (`CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY`, `CMAKE_FIND_USE_PACKAGE_REGISTRY`) are propagated automatically from the host configure cache to per-platform bootstrap reconfigure.
When the host build consumes LVRS from an installed multi-platform prefix, bootstrap rewrites `LVRS_DIR` to the requested platform package directory automatically (for example `<prefix>/platforms/ios/lib/cmake/LVRS`) so iOS/Android/WASM toolchains do not depend on root-prefix package discovery.
LVRS package config exports platform/toolchain hint variables for scripts:
- `LVRS_LAYOUT_VERSION`
- `LVRS_ACTIVE_PLATFORM`
- `LVRS_ACTIVE_PREFIX`
- `LVRS_INSTALL_ROOT`
- `LVRS_QT_HOST_PREFIX_HINT`
- `LVRS_QT_IOS_PREFIX_HINT`
- `LVRS_QT_ANDROID_PREFIX_HINT`
- `LVRS_QT_WASM_PREFIX_HINT`
- `LVRS_ANDROID_SDK_HINT`
- `LVRS_ANDROID_NDK_HINT`
- `LVRS_EMSDK_HINT`
For a manual cross-platform configure outside LVRS bootstrap, set `LVRS_DIR` directly to `<prefix>/platforms/<platform>/lib/cmake/LVRS` if the toolchain roots `find_package()` lookups away from the install root.
Example one-shot bootstrap command:
```bash
cmake --build build --target bootstrap_MyApp_all
```
Use `lvrs_configure_qml_app(<target> NO_PLATFORM_RUNTIME_TARGETS)` to disable this behavior.
`lvrs_add_qml_app()` can generate a ready-to-run entrypoint automatically when `SOURCES` is omitted.
Use `lvrs_configure_project_defaults()` to centralize Apple bundle/plist/entitlements, Android package source dir/package id, and iOS plugin exclusion defaults.

Framework-only bootstrap targets are generated at project root:
- `bootstrap_lvrs_macos`
- `bootstrap_lvrs_linux`
- `bootstrap_lvrs_windows`
- `bootstrap_lvrs_ios`
- `bootstrap_lvrs_android`
- `bootstrap_lvrs_wasm`
- `bootstrap_lvrs_all`
`bootstrap_lvrs_all` builds the selected framework bootstrap platform set under `<build>/lvrs-bootstrap/framework/<platform>`, builds `LVRSCore`, and installs to `${LVRS_BOOTSTRAP_INSTALL_ROOT}/<platform>`.
Default framework bootstrap platform set is all runtime platforms (`macos;linux;windows;ios;android;wasm`) unless `LVRS_BOOTSTRAP_FRAMEWORK_PLATFORMS` is provided.
Any platform without a discoverable Qt kit is skipped with a configure-time status message.
Override per-platform install paths with `LVRS_BOOTSTRAP_INSTALL_PREFIX_<PLATFORM>`.
For cross-host platforms, provide matching Qt kits/toolchains through `LVRS_BOOTSTRAP_QT_PREFIX_<PLATFORM>` and `LVRS_BOOTSTRAP_TOOLCHAIN_FILE_<PLATFORM>`.

## Rendering Backend Enforcement

At configure time, when `LVRS_ENFORCE_VULKAN=ON`:

- macOS/iOS must provide Qt Metal support (`QT_FEATURE_metal >= 0`).
- Android must provide Qt Vulkan support (`QT_FEATURE_vulkan >= 0`).
- `Vulkan::Vulkan` is used when discoverable for Vulkan-fixed targets, but absence at configure time is treated as warning and runtime loader discovery is used instead.

At runtime:

- macOS/iOS are fixed to Metal.
- Windows prefers D3D11, probes the DirectX runtime during bootstrap, and falls back to OpenGL when DirectX cannot be initialized.
- Android prefers Vulkan, probes runtime loader availability during bootstrap, and falls back to OpenGL when Vulkan cannot be initialized.
- Linux uses Qt default backend selection.
- WASM/other platforms use Qt default backend selection as fallback.
- Startup fails fast if a required fixed backend cannot be initialized and no platform fallback is available.
- Bootstrap stdout now emits structured `LVRS bootstrap.*` lines with render-profile, scenegraph environment, probe candidate, fallback-reason, import-path, and font-policy details so downstream apps can capture first-frame diagnostics without enabling `LV.Debug`.

Bootstrap render defaults are selected conservatively before app construction. Mobile targets use a lighter MSAA / frames-in-flight profile than desktop targets so the first window starts with lower memory pressure before per-window `RenderQuality` presets are applied.

## Notes

- Qt 6.5+ with `Quick` and `QuickControls2` is required.
- Fixed backend Qt feature checks happen in `CMakeLists.txt`.
- Backend selection logic lives in `backend/runtime/vulkanbootstrap.cpp`.
- Downstream app bootstrap template is provided at `main.cpp` (not built by default).
- Recommended reusable bootstrap API is `backend/runtime/appbootstrap.h`.

## CI Build Pipeline Example

A minimal CI pipeline usually runs the following sequence:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

If test targets are not present, keep `ctest` but allow no-test pass semantics or gate it behind a project option.

## P4 Quality Automation Commands

P4 quality automation scripts are shipped under `tests/ci/`.

```bash
# PR gate (performance + visual)
./tests/ci/run_p4_quality.sh

# Sanitizer matrix
./tests/ci/run_p4_sanitizers.sh address
./tests/ci/run_p4_sanitizers.sh undefined

# Soak batch
LVRS_SOAK_ITERATIONS=5000 ./tests/ci/run_p4_soak.sh
```

Detailed contracts and thresholds are documented in `docs/quality-automation-p4.md`.

## Packaging Checklist

When shipping LVRS-based binaries:

1. Confirm Qt runtime deployment for the target OS.
2. Confirm icon/font resources are available in packaged artifacts.
3. Confirm graphics backend requirements (Metal/Vulkan) on target machines.
4. Confirm environment overrides (`LVRS_APP_*`) are documented for operators.

## Runtime Smoke Test Script

For release candidates, run a smoke test that verifies:

- app process starts without backend bootstrap failure,
- root QML loads successfully,
- route navigation works for at least one static and one dynamic route,
- input/runtime events are captured when interacting with the window.
