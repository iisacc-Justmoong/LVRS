# Build and Runtime Setup

## Quick Install

```bash
./install.sh
```

The installer runs one multi-platform bootstrap build (`bootstrap_lvrs_all`) and installs LVRS for every runtime platform in one pass.
Installed packages are written to `<prefix>/platforms/<platform>` (`macos`, `linux`, `windows`, `ios`, `android`, `wasm`), then the host platform path is registered in the CMake user package registry.
The installer always performs a clean reinstall by removing the previous build directory and installed LVRS artifact paths before configuring.
`install.sh` configures examples/tests on the host build by default; pass `--without-examples --without-tests` to disable them.

## Configure

```bash
cmake -S . -B build
```

## Build

```bash
cmake --build build -j
```

## Run

```bash
cmake -S . -B build \
  -DLVRS_BUILD_EXAMPLES=ON \
  -DLVRS_BUILD_TESTS=ON
cmake --build build --target LVRSExampleVisualCatalog
./build/bin/LVRSExampleVisualCatalog
```

The visual-catalog example target (`LVRSExampleVisualCatalog`) emits the executable as `build/bin/LVRSExampleVisualCatalog`.
The framework library target (`LVRSCore`) itself does not emit a runnable app.

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Build Options

- `LVRS_BUILD_EXAMPLES` (`OFF`): build runnable examples.
- `LVRS_BUILD_TESTS` (`OFF`): build and register tests.
- `LVRS_INSTALL_QML_MODULE` (`ON`): install QML module artifacts (`qmldir`, qmltypes, plugin, QML files) under `<prefix>/lib/qt6/qml/LVRS`.
- `LVRS_ENFORCE_VULKAN` (`ON`): fail CMake configure when the platform-fixed graphics backend requirements are missing.
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
`lvrs_configure_qml_app()` sets `QT_QML_IMPORT_PATH` for installed-package consumption, applies a default executable output directory (`<build>/bin`) when unset, and auto-links/imports LVRS static QML plugin artifacts for static package builds.
`LV.ApplicationWindow` and `LV.AppScaffold` provide adaptive layout policy APIs for mobile/desktop reordering:
- `scaffoldLayoutMode` (`auto`, `mobile`, `desktop`)
- `scaffoldLayoutPlatform` override (default `Qt.platform.os`)
- `scaffoldForceDesktopOnLargeMobile` + `scaffoldMobileDesktopMinWidth`
- `scaffoldPreferBottomNavigation` + `scaffoldBottomNavigationMaxItems`
- runtime state flags: `adaptiveMobileLayout`, `adaptiveDesktopLayout`, `adaptiveRailNavigation`, `adaptiveDrawerNavigation`, `adaptiveBottomNavigation`
- `matchesMedia()` tokens: `mobile-layout`, `desktop-layout`, `rail-nav`, `drawer-nav`, `bottom-nav`
State uses page-stack routing (`LV.PageRouter`), and placement uses flex layout (`RowLayout`/`ColumnLayout`) in `LV.AppScaffold`.
`LV.ApplicationWindow` page-stack API: `pageRoutes`, `pageInitialPath`, `useInternalPageStack`, `activePageRouter`, `pageStackNavigated`, `pageStackNavigationFailed`.
Default `auto` mode is mobile-first for `android`/`ios` and prevents wide-screen mobile windows from being forced into desktop rail layout unless explicitly configured.
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
Desktop bootstrap targets produce executable artifacts.
iOS bootstrap generates an Xcode project by default and installs a simulator app via `xcrun simctl`.
Android bootstrap generates an Android Studio (Gradle) project by default and installs an APK via `adb`.
WASM bootstrap emits browser artifacts and writes `LVRSWasmArtifact.cmake` entry metadata in the wasm bootstrap build tree.
`launch_<target>_wasm` serves the wasm build tree via a local static HTTP server and can auto-open a browser.
`export_<target>_wasm_site` recursively collects wasm web assets (nested layout-safe) and generates an `index.html` redirect to the detected entry.
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
`LVRS_DIR` and package-registry policy (`CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY`, `CMAKE_FIND_USE_PACKAGE_REGISTRY`) are propagated automatically from the host configure cache to per-platform bootstrap reconfigure.
LVRS package config exports platform/toolchain hint variables for scripts:
- `LVRS_LAYOUT_VERSION`
- `LVRS_ACTIVE_PLATFORM`
- `LVRS_ACTIVE_PREFIX`
- `LVRS_QT_HOST_PREFIX_HINT`
- `LVRS_QT_IOS_PREFIX_HINT`
- `LVRS_QT_ANDROID_PREFIX_HINT`
- `LVRS_QT_WASM_PREFIX_HINT`
- `LVRS_ANDROID_SDK_HINT`
- `LVRS_ANDROID_NDK_HINT`
- `LVRS_EMSDK_HINT`
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
`bootstrap_lvrs_all` configures each platform build under `<build>/lvrs-bootstrap/framework/<platform>`, builds `LVRSCore`, and installs to `${LVRS_BOOTSTRAP_INSTALL_ROOT}/<platform>`.
Override per-platform install paths with `LVRS_BOOTSTRAP_INSTALL_PREFIX_<PLATFORM>`.
For cross-host platforms, provide matching Qt kits/toolchains through `LVRS_BOOTSTRAP_QT_PREFIX_<PLATFORM>` and `LVRS_BOOTSTRAP_TOOLCHAIN_FILE_<PLATFORM>`.

## Rendering Backend Enforcement

At configure time, when `LVRS_ENFORCE_VULKAN=ON`:

- macOS/iOS must provide Qt Metal support (`QT_FEATURE_metal >= 0`).
- Windows/Linux/Android must provide Qt Vulkan support (`QT_FEATURE_vulkan >= 0`) and a linkable Vulkan runtime target (`Vulkan::Vulkan`).

At runtime:

- macOS/iOS are fixed to Metal.
- Windows/Linux/Android are fixed to Vulkan.
- Other platforms use Qt default backend selection as fallback.
- Startup fails fast if a fixed backend cannot be initialized.

## Notes

- Qt 6.5+ with `Quick` and `QuickControls2` is required.
- Vulkan and Qt feature checks happen in `CMakeLists.txt`.
- Backend selection logic lives in `backend/runtime/vulkanbootstrap.cpp`.
- Downstream app bootstrap template is provided at `main.cpp` (not built by default).
- Recommended reusable bootstrap API is `backend/runtime/appbootstrap.h`.
