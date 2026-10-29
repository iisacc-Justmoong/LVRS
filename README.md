# LVRS

The latest [native authoring verification record](../LVUI/docs/NativeReadinessVerification.md) distinguishes executed checks, installed-package proof, and platform limits from the historical implementation notes below.

[Native ViewModels](docs/native/ViewModelRegistry.md) provides live C++ model
access, aliases, per-key write ownership and borrowed/retained lifetimes. Registry
lookups do not create a shadow property store or rebuild every model descriptor.

[Native snapshot history](docs/native/ModelUndoStack.md) owns undo/redo values and
observable depths in C++. A committed transfer preserves its snapshot even when
an observer fails; generated LVUI views only bind data and native commands.

[Native ProgressBar](docs/native/ProgressBar.md) retains its C++ model, state source,
paint cache and shared Core motion across live LVUI input updates.
Compile/link evidence is distinct from the deferred runtime batch.

[Native SVG services](docs/native/SvgManager.md) own resource loading, bounded
LRU caches, SVGZ decoding, owner-loop HTTP requests and lazy PNG encoding.
LVUI's view-only Image binds immutable native images and applies its dependent
options as one validated C++ value. Native execution remains deferred.

[Native presentation models](docs/native/PresentationModels.md) provide Core-owned
ViewModel, StateModel and ProgressModel APIs with weak state sources and explicit
owner-thread notifications. Runtime verification remains deferred to the final batch.

Native [hierarchy data](docs/native/Hierarchy.md) now provides typed preorder rows,
indexed subtree moves and a separate visibility/height projection for each view.
Branch expansion and navigation queries preserve the shared model revision.
The full HierarchyItem/HierarchyList widget and LVUI integration are still pending;
all newly compiled tests remain deferred to the final whole-goal runtime batch.

Native [application surfaces](docs/native/AppSurfaces.md) now provide AppHeader
actions and AppCard header/body composition, original implicit metrics, retained
children and typed live LVUI inputs. The new NativeAppSurfaces example uses the
Core application entry point; runtime execution stays deferred to the final batch.

Native [navigation tracking](docs/native/NavigationTracking.md) now supplies a
separate PageMonitor visit log and ViewStateTracker logical-stack model. Navigator
synchronizes only its selected target, while applications explicitly record visits.
The native page gallery displays both models and keeps visit undo separate from navigation.
Runtime verification remains deferred to the final whole-goal batch.

Native [Link](docs/native/Link.md) now composes text or arbitrary native child
views in a transparent iiGUI action surface, with typed routing, state colors,
shaped underline and Core Observable inputs. The two-page gallery includes live
href rebinding. Execution remains deferred to the final whole-goal batch.


LVRS now has a native C++23 implementation built from LVUI AOT and iiGUI's own
GPU-rendered widgets. `LVRS_BACKEND=native` is the default; this path does not
find or link Qt. Cocoa provides the window, input/IME and accessibility boundary;
component layout, state and rendering belong to iiGUI. Native migration is in
progress: [architecture](docs/native/Architecture.md) and the
[full parity ledger](docs/native/Parity.md) distinguish implemented basic behavior
from the complete existing LVRS contract.

Build native LVRS in `build/native/build` using the updated iiSDKCore, LVUI and
iiGUI package prefixes. Resource services also require iiFileProvider Native,
iiJson, iiNetwork 0.5+ and iiVisualCodec 0.2+ with its Image component. Configure
their package prefixes and OpenSSL 3 through standard CMake search paths or
`OPENSSL_ROOT_DIR`; Video and USD codecs are not required by this path.
`examples/NativeGallery` is a runnable LVUI app using
`LVRS::LVRS` and Core's application entrypoint. The legacy Qt/QML implementation
below is available with `-DLVRS_BACKEND=qt`; existing installed Qt consumers have
not been replaced by this source migration.

The native gallery includes icon/help/dropdown buttons, icon-bearing tabs,
ComboBox fields, directional steppers and label/icon segmented rows. Segmented
rows keep independent child actions; they do not introduce a selection model.
Native Menu/ContextMenu families add owned popup scopes, scrolling, context
targets, shortcuts and regular/context typography. Backdrop blur, spring motion
and dynamic object models remain part of the full migration.
Authored SVG assets and aliases are embedded as native data; text/image foreground
textures survive panel-only state changes. See the architecture for supported
SVG semantics and the remaining component and platform contracts.

`examples/NativeColorGallery` shares one native color model across wheel, HS, SV,
spectrum and hue/brightness rails. It exercises RGB, HSV, CMYK, grayscale, alpha,
RRGGBB/RRGGBBAA input and apply/cancel through Core change queries. The original
wheel PNG is embedded unchanged. Color backgrounds use dependency-keyed caches;
selection changes reuse their images. `ColorPicker` now composes the six original
editing variants, channel ramps and numeric commits, comparison/recent swatches,
HEX/alpha and apply/cancel/eyedropper requests from native iiGUI widgets.
`examples/NativePickerGallery` AOT-compiles three shared-model pickers. See the
[native ColorPicker contract](docs/native/ColorPicker.md) for supported inputs and
remaining visual, reactive and platform parity.

`Popover` retains ordinary C++ widget content in the same native GPU overlay,
including ColorPicker editing, nested menus, modal/nonmodal input and owner-driven
apply/cancel. `examples/NativePopoverGallery` exercises reopening one scoped
backend and Core display frames. See the [native Popover contract](docs/native/Popover.md)
for close policies and remaining material, reactive and platform work.

The same example now opens its editor from a native `ColorPickerButton`, using
the separate original button-ring PNG, 22/28/36 metrics, authored state colors,
focus border and whole-button disabled opacity. Shared Core color state updates
the preview without rebuilding it. See [ColorPickerButton](docs/native/ColorPickerButton.md)
for the supported API and remaining motion/inherited contracts.

`examples/NativeActivationGallery` demonstrates compiled per-open content factories
above LVUI's outer view. Editor objects, models and subscriptions retire on close;
the parent backend and native GPU surface remain. See [activation ownership](docs/native/ActivationContent.md).
The native [Modal](docs/native/Modal.md) now uses that mechanism for centered,
width-aware persistent header/icon/action chrome and a fresh AOT-compiled body.
Actions remain open until their owner closes the dialog; cancellation is distinct
from programmatic closure. `examples/NativeModalGallery` exercises initial open,
reopening, 420ms/150ms native frame transitions, immediate body retirement, live
reduced-motion settings, Core worker delivery, resize and teardown. Reactive
options, inherited behavior and platform parity remain ongoing.

[Alert and AlertButton](docs/native/Alert.md) now compile from LVUI to native C++
widgets with the authored one/two/three-action layouts, typed method-before-click
dispatch, static reopen lifetime, original embedded SVGs and real Metal GPU glass.
`examples/NativeAlertGallery` exercises all six action positions and both C++
method signatures through Core's application runtime. Authored GPU shadows and
420ms OutBack / 150ms InCubic transitions now share native display frames, with
live reduced-motion/speed preferences and retained exit pixels. General reactive
bindings, AlertButton feedback, other components' motion and additional RHI
effects remain pending. See [native motion](../iiGUI/docs/NativeMotion.md).

The native [Sheet](docs/native/Sheet.md) retains its header and editor while closed,
with desktop centering and mobile bottom presentation, fit/medium/large detents,
explicit safe insets, fixed chrome, native body scrolling and grabber dismissal.
`examples/NativeSheetGallery` compiles both presentations from LVUI and exercises
retained edits, display-driven slide/snap-back motion and Core-owned teardown.
General reactive/inherited behavior, transition-completion signal timing and
additional platform adapters remain part of the full migration.

## Legacy Qt/QML implementation

[Windows build and execution](docs/WindowsBuildAndRun.md)

Windows shared builds retain the LVRS DLL dependency for QML-only consumers on
both MSVC and MinGW. The native resource initialization symbol is retained at
link time so QML types register even when no C++ LVRS API is called. The Windows
QML link regression creates an EventListener without referencing a backend API.
See [Windows validation](docs/Windows-validation.md) for the current full-suite
results and remaining Windows failures.

Windows shared builds export the C++ runtime API as well as QML registration
symbols. MinGW uses `--export-all-symbols`; MSVC uses CMake's automatic exports.
`LVRSTests_windows_exports` links and invokes bootstrap and foreground-service
APIs through the resulting DLL import library.
Tooltip test fixtures construct QByteArray explicitly so conditional expressions
have the same type on MSVC, Clang and GCC.
MinGW consumers retain the module's resource initializer as a link dependency,
including QML-only consumers verified by `LVRSTests_import_api`.
Public QObject classes also use generated DLL import/export annotations so
pointer-to-member signal connections resolve to the same address across DLLs.
The Windows export smoke test verifies a typed signal connection and delivery.
The example aggregate-target check is registered only when `LVRS_BUILD_EXAMPLES`
is enabled; all independent example configuration checks remain available.

LVRS is a Qt 6.5+ UI framework focused on deterministic rendering, event observability, and reusable QML components.

The repository ships three layers together:
- A reusable static QML module (`LVRS`) with components and C++ singletons.
- Runnable example applications under `example/`, including the visual-catalog demo.
- Tests covering event flow, text editing behavior, import API, and backend wiring.

The framework target itself does not build an application executable; all runnable apps are example targets.

## Requirements

- CMake 3.21+
- C++23 compiler
- Qt 6.5+
- Qt modules: `Quick`, `QuickControls2`, `Qml`, `Svg`, `Network`
- Qt `Test` module only when `LVRS_BUILD_TESTS=ON`
- Local Qt kits are discovered under `/Volumes/Storage/Qt` (for example, `/Volumes/Storage/Qt/6.8.3/macos`). Explicit `QT_VERSION_ROOT`, platform Qt hints, and `Qt6_DIR` still take precedence.
- `LVRS_ENFORCE_VULKAN` validates fixed backend Qt features where applicable (macOS/iOS: Metal, Android: Vulkan). Windows prefers D3D11 during bootstrap and falls back to OpenGL after runtime probing, while Linux/WASM use Qt default backend selection.

## Quick Install (Clone -> Install -> Use)

```bash
git clone <LVRS_REPO_URL>
cd LVRS
./install.sh
```

`install.sh` is a wrapper around Rust CLI `lvrs install`.
If `cargo` exists, it runs `cargo run --manifest-path src/rust-cli/Cargo.toml --target-dir build/rust-cli --bin lvrs -- install ...`; otherwise it falls back to `lvrs install` from `PATH`.
Direct `lvrs install` now also reuses the installed source metadata under `<prefix>/src/LVRS` when launched outside the repository tree. If the recorded checkout path became stale because an upper directory was renamed, the CLI relocates the project root by matching the preserved trailing path segments before falling back to the installed source snapshot itself.
Source snapshots omit repository build directories (including hidden `.build.lvrs-stale-*` cleanup remnants), `build/rust-cli`, and legacy `src/rust-cli/target`, so local Cargo build artifacts are never copied into the installed framework prefix.
If `LVRS_ROOT` or `LVRS_PROJECT_ROOT` points at an installed prefix such as `~/.local/SDK/LVRS`, the CLI treats it as an install prefix and resolves the source through `<prefix>/src/LVRS`; if that snapshot is missing, commands launched inside the checkout fall back to the current repository root.
The install flow builds `bootstrap_lvrs_all`.
By default, the bootstrap platform set follows the current host:
- Linux: `linux`
- macOS: `macos;ios;android;wasm`
- Windows: `windows;android;wasm`
Platforms without a discoverable Qt kit are skipped.
Use `./install.sh --platforms linux,android,wasm` (comma/semicolon list) to override bootstrap/install platforms explicitly.
On Linux hosts, `lvrs install` now runs a CMake preflight first: it verifies the host C++ toolchain, Qt 6.5+ modules (`Quick`, `QuickControls2`, `Qml`, `Svg`, `Network`), and required Qt host tools, then auto-resolves `Qt6_DIR`/`LVRS_BOOTSTRAP_QT_PREFIX_LINUX` from common Qt layouts (Qt online installer, `qtpaths`/`qmake`, distro multiarch installs) when possible.
If Linux dependencies are missing and the distro package manager is recognized, the CLI prints the exact install command and can execute it with `./install.sh --install-linux-deps`.
Use `lvrs doctor --fix` for a host-only dependency precheck/fix pass, and `lvrs doctor --bootstrap [--with-wasm|--platforms ...]` to validate `src/main.cpp` bootstrap readiness plus cross-platform toolchain hint auto-detection before a full build; it exits non-zero when required hints/toolchains are still missing for the requested bootstrap targets.
Install layout remains `<prefix>/platforms/<platform>` (`macos`, `linux`, `windows`, `ios`, `android`, `wasm`).
The checkout lives under `Workspace/SDK/LVRS`. The default install root is `~/.local/SDK/LVRS` for both CMake and the install CLI. Set `--prefix <path>`, `LVRS_INSTALL_PREFIX=<path>`, or an explicit `CMAKE_INSTALL_PREFIX` to move the install root.
After install, `env.sh` points `CMAKE_PREFIX_PATH` to the install root (`<prefix>`) and `QML2_IMPORT_PATH` to the host platform package path.
`find_package(LVRS CONFIG REQUIRED)` then resolves the active platform package via LVRS dispatcher logic. The root dispatcher version file is architecture independent so the same install root can select 64-bit native packages and the 32-bit WASM package; each selected platform package retains its own binary architecture check.
The installer also copies the running CLI into `<prefix>/bin/lvrs` (`lvrs.exe` on Windows), and `env.sh` adds that directory to `PATH`. Shell wrappers keep Cargo output under `build/rust-cli/` unless `CARGO_TARGET_DIR` is explicitly set, so the CMake clean reinstall does not remove the running CLI.
The installer recreates the CMake build state on every install while preserving `build/rust-cli/`, which contains the running installer. Existing LVRS artifacts are removed only after configuration succeeds.
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

CTest selects this build's library and QML paths even when the shell has loaded an installed LVRS `env.sh`. The import test also compares embedded QML hashes with the current checkout. Installed packages are checked separately with the [installed consumer](docs/build.md#installed-package-regression).

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
On desktop, the default solid chrome for `LV.Window` and `LV.ApplicationWindow` includes system-owned move plus native-first edge/corner resize handling. Qt 6.8 Cocoa does not implement `QWindow::startSystemResize()`, so macOS transparently falls back to pointer-driven, min/max-constrained window geometry. Built-in resize handles forward the exact global press position, keeping remote, tablet, and synthesized input aligned with the fallback session. Configure the hit regions with `windowDragHandle*`, `windowDragExclusionItems`, and `windowResize*`, or disable those regions and call `requestWindowMove()` / `requestWindowResize(edges)` from application-owned chrome.
`LV.ApplicationWindow` uses a fully opaque background centered on 95% `windowColor` RGB plus 5% app `primaryColor` RGB. The default `windowColor` follows `Theme.window` (`#141414`). A rectangle defines that center color, while a cached opaque texture adds fixed monochrome grain at one physical pixel per sample, with a maximum variation of two 8-bit RGB levels. The grain is unblurred and uses no glass layer or Overlay blending. Legacy `windowBackgroundOpacity` and `backgroundBlurEnabled` inputs do not change this opaque surface; `backgroundBlurSupported` and `backgroundBlurActive` are false. macOS retains an opaque native window and its existing frame/input behavior. See [ApplicationWindow material](docs/components/app/ApplicationWindow.md#default-background-material). LVRS is built as C++23.

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
iOS framework bootstrap also requires a selected full Xcode with an iPhoneOS SDK; Command Line Tools-only hosts skip the iOS target instead of failing the aggregate install.
Per-platform install prefixes can be overridden with `LVRS_BOOTSTRAP_INSTALL_PREFIX_<PLATFORM>`.
Cross-host targets (`linux`, `windows`, `android`, `ios`, `wasm`) require matching Qt kits and toolchains; set `LVRS_BOOTSTRAP_QT_PREFIX_<PLATFORM>` and `LVRS_BOOTSTRAP_TOOLCHAIN_FILE_<PLATFORM>` as needed.
For Apple compatibility, configure the parent with `CMAKE_OSX_DEPLOYMENT_TARGET` for macOS or set `LVRS_BOOTSTRAP_OSX_DEPLOYMENT_TARGET_<PLATFORM>` explicitly. The macOS child inherits the parent target, while iOS requires an explicit generic or iOS-specific value so the host version is not applied to the mobile build.

## Rendering Backend Policy

At runtime, graphics backend selection is bootstrapped through `src/backend/runtime/appbootstrap.*` from each app entrypoint.

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

Both application and framework bootstrap propagation preserve explicit `OFF` values for these options and forward the resolved Apple deployment target. Empty values alone are omitted from a nested platform configure.

When enabled, configure fails if:
- The platform-fixed backend requirements are not satisfied (`QT_FEATURE_metal` for macOS/iOS, `QT_FEATURE_vulkan` for Android).

## Project Layout

- `src/backend/`: C++ singletons (`RuntimeEvents`, `Backend`, `RenderMonitor`, `RenderQuality`, etc.).
- `src/backend/runtime/appbootstrap.h`, `src/backend/runtime/appbootstrap.cpp`: reusable pre/post app bootstrap API for downstream apps.
- `src/qml/`: QML module entry files and components.
- `src/main.cpp`: downstream app template entrypoint (reference only, not built by framework CMake targets). CLI/env overrides can inject `module/root/app-name/style`.
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

## Shared Desktop and Mobile Sizing

Mobile no longer doubles Theme sizes. iOS, Android, and desktop use the same spacing, controls, icons, font sizes, and line heights at `1x`. Body remains `13px`; Title remains `26px`; compact icons remain `18px`. Explicit component overrides and optional window composition scaling remain available. See [Theme](docs/theme.md) for the sizing and regression-test contract.

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

Control → Input → Slider contains seven interactive `LV.Slider` types and the complete 112-variant Figma matrix. The component supports Mini/Small/Regular/Large, centered and capsule fills, endpoint labels/icons, ticks, segmented snapping, and native pointer/touch/keyboard input. See [Slider API](docs/components/control/Slider.md).

Control → Input → ColorPicker contains six embeddable color editing views, including the hue wheel, two color planes, grayscale, RGB and CMYK. Pass an `LV.ColorPicker` through `Component` to `LV.Modal.contentComponent` or another content host. The host owns presentation and dismissal. See [ColorPicker API](docs/components/control/ColorPicker.md).

## Documentation

Surfaces → Sheet presents reusable content as a mobile bottom sheet or centered desktop modal. `LV.Sheet.cornerRadius` accepts the device's measured corner radius in logical pixels; physical-pixel measurements must first be divided by the matching display DPR. The catalog includes share, export, folder selection with fixed actions, and inline-frame examples. See [Sheet API](docs/components/surfaces/Sheet.md).

Surfaces → Tooltip presents a Component or inline view in a speech bubble. Its tail stays at a target-local point while the body flips, shifts, and constrains itself to the available display area. Hover, keyboard focus, held touch, delay/timeout, and manual origins are supported. See [Tooltip API](docs/components/surfaces/Tooltip.md).

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

<a id="android-시스템-안전-영역"></a>

### Android system safety zone

Even if Qt 6.8 Android's default QPA safety margin is 0, `WindowSafeAreaObserver` reads Android WindowInsets system bar and display cutout and converts them to Qt logical coordinates. Apps use existing `ApplicationWindow.mobileSystemSafe*Inset` properties as is. Margins are refreshed on rotation, window size, and focus changes. Desktop and iOS maintain existing QPA margin paths. `PlatformIntegrationTests::safe_area_tracks_window_lifetime` checks window connections, rotation size, and connection removal, and Society Android emulator's bottom Files button and actual file app entry constitute device integration checks.

### Standard window and popup materials

`ApplicationWindow` uses an opaque 98% `Theme.window` (`#141414`) + 2% app-accent RGB center color with very fine, fixed monochrome grain (one physical pixel, at most ±2 RGB levels). Standalone `WindowMaterial` retains its near-black 50% coating, radial accents, and 64px backdrop diffusion. `ContextMenu` and `Menu` use a 12% frosted WindowMaterial coating with 64px blur. `Tooltip` and `Popover` retain PanelMaterial Glass 25 and its radial accents. See [Materials](docs/components/surfaces/Materials.md) and [Popover](docs/components/surfaces/Popover.md).

## Interaction and motion

See [the motion system](docs/motion.md) for shared elastic feedback, accessibility controls, component coverage and VisualCatalog verification.

The [Figma parity audit](docs/figma-parity.md) records the current component inventory, exact color tokens and corrected compact/regular menu contracts.

ContextMenu and Menu now share a lighter frosted WindowMaterial coating. See the [menu material contract](docs/components/navigation/ContextMenu.md#window-derived-frosted-menu) for tint, blur and accent overrides.

## Source layout

Implementation files and their headers live together under `src/`. Existing feature and platform subdirectories retain their responsibilities. Build configuration, tests, documentation, resources, and maintenance scripts remain at the project root. Configure and build using the repository-local `build/` directory.

<a id="lvrs-모바일-탐색"></a>

## LVRS mobile navigation

`MobileNavigationBar` · `MobileNavigationTab` provides [Figma spec](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS?node-id=1072-1243)device curvature, optional independent search, left collapse placement, sticky selection movement, and optional labels. Search is disabled by default and added via `search: ({})`. Usage refers to [component documentation](docs/components/navigation/MobileNavigationBar.md).

## Recovering an interrupted install

LVRS now requests iiFileProvider 0.7.1 or a compatible newer package to match
the current SDK interfaces. Framework bootstrap builds iiFileProvider
from the sibling SDK checkout for iOS, Android and WebAssembly with the same Qt
kit, ABI, sysroot and toolchain. Each cross package owns its dependency under
`deps/iiFileProvider`; its installed config selects this target package before
ambient host packages. `LVRS_BOOTSTRAP_FILE_PROVIDER_SOURCE_DIR` overrides the
source location. iOS, Android and WASM use the provider's pinned SQLite source.
Bootstrap and installed consumers discover the installed provider package set
and resolve Native, Catalog, Objects, Temporary and SQLite from that target
prefix. Explicit package directories retain this closure under root-only
cross-toolchain searches and exclude ambient host package hints.
LVRS remains version 1.0.1. The provider-bootstrap CTest exercises target
selection, ABI forwarding, installation and dependency-failure propagation.
The source snapshot also preserves the sibling iiFileProvider sources, excluding
its build outputs, so reinstalling from the installed snapshot can rebuild
cross dependencies when the original checkout is unavailable.
Android discovery rejects an incompatible explicit CMake NDK override and skips
obsolete shell hints when a compatible installed NDK is available. The provider
requires libc++ `<stop_token>` and `<expected>` support. NDK r30 and the Qt
multithread WASM kit provide the required C++23 library/thread facilities.
Both parent CMake target generation and the Rust CLI check shell NDK hints before
promoting them into explicit child settings. This includes a stale
`LVRS_BOOTSTRAP_ANDROID_NDK` in the shell. Explicit `-DLVRS_BOOTSTRAP_ANDROID_NDK=...`
command-line overrides remain authoritative and incompatible explicit paths
produce a diagnostic.

`cmake --build build` requires a completed configure step and `build/CMakeCache.txt`.
Sourcing a toolchain environment only exports variables; it does not create the cache.
If configuration was interrupted, rerun `./install.sh` (or the shell's `lvrs install`
wrapper). The installer configures the fixed `build/` directory before building.
Install third-party toolchains in system locations, outside `Workspace/SDK`.
The Unix wrapper inherits `JAVA_HOME`, `ANDROID_HOME`, `ANDROID_NDK_ROOT`, and
`EMSDK` from the system shell environment; it never auto-loads a sibling toolchain
directory. Set `LVRS_TOOLCHAIN_ENV_FILE` only for an explicit environment file
override. An explicit missing file is an error.

Regression checks: `sh tests/test_install_wrapper.sh` and
`cargo test --manifest-path src/rust-cli/Cargo.toml --target-dir build/rust-cli`.
They cover system environment inheritance, ignored sibling toolchains, explicit
overrides, argument boundaries, preservation of the
Cargo executable during CMake cleanup, and recreation of a missing build directory.

## License

SPDX-License-Identifier: MIT

The original code and documentation in this repository are licensed under the
[MIT License](LICENSE). Third-party code, libraries, fonts, tools, and model
weights retain their respective licenses.

`TableCellItem`, `TableRow`, and `TableHeader` now generate native C++ cells,
editable rows, and interactive column headers with typed validation callbacks.
The [table contract](docs/native/Tables.md) distinguishes these standalone parts
from the model-backed Table view and the remaining full-parity work.

## Native table data model

`lvrs::TableModel` adds a Qt-free C++23 value-owned model for typed edits, atomic
range paste, stable sorting, merges, row/column mutation, coalesced resizing and
undo/redo. Axis updates and prefix queries use a Fenwick index; viewport queries
include intersecting merged anchors without enumerating preceding ordinary rows.
Core `QueryPort` receivers report scoped changes. See
[TableModel](docs/native/TableModel.md) for ownership, costs, limits and remaining
parity. The new model tests are compiled; execution is deferred to the final
verification batch. The full native `Table` view and automatic LVUI model binding
remain in scope and are not complete.

## Model-backed native Table

The native `Table` view connects to `lvrs::TableModel` through retained viewport
drawing, one reusable editor, selection, navigation, sorting and a scoped LVUI
C++ model reference. See [TableView](docs/native/TableView.md) and
`examples/NativeDataTableGallery`. Execution remains deferred; full inherited APIs,
virtual accessibility, full reactive/delegate contracts and cross-platform
verification remain in scope. Native resize gestures, structure gutters,
deletion menus, model replacement and bounded TSV/clipboard transfer now have
C++ implementations and AOT options. See [Table transfer](docs/native/TableTransfer.md).
The current native
path requires iiSDKCore 0.18.1 for its portable number contract.

The Table viewport now supports atomic two-axis positions, horizontal/Shift-wheel
scrolling, offscreen cell reveal and horizontal editor activation. Header sorting,
resize handles, structure gutters and context menus use the same logical X axis.
The gallery includes five columns wider than its viewport. Runtime tests remain
deferred to the final batch; the retained text layout still has an 8192-unit limit.

## Native route and navigation models

`lvrs::RouteMatcher`, `lvrs::RouteResolver` and `lvrs::NavigationStackModel` now
provide C++23 path matching, compiled route lookup, bounded immutable result
caching and typed presentation history through Core value/change contracts.
See [Native navigation](docs/native/Navigation.md) and
`example/NativeNavigationGallery`. The gallery connects an AOT LVUI backend to
the models; it does not yet implement the full routed-page lifecycle host.
Runtime and sanitizer execution remain deferred to the final whole-goal batch.

## Native routed pages

[`lvrs::PageRouter`](docs/native/PageRouter.md) now connects typed history to
independent iiGUI content generations. Compiled page factories use LVUI's `build`
entry point within one existing surface. A successful candidate replaces the old
page; failure preserves its widgets and history. Back constructs a fresh page
scope. `example/NativePageRouterGallery` contains two independently generated
pages, native input, Core background-result delivery, frame progression, resize
and shutdown contracts. Compilation, packaging and relocated-consumer checks
remain separate from the final runtime batch. Inactive-page caching, global
navigation, interactive transitions and full source parity remain in scope.

The native [Navigator](docs/native/Navigator.md) now manages weak target stacks,
same-parent sibling order and scoped registration through a replaceable
`NavigationTarget` interface. PageRouter registers by default; an explicitly
shared Navigator can isolate an application or editor domain. The page gallery
uses that shared service and retires registrations with the widget scope.
Interactive forwarding and native Link remain pending. All execution is deferred
to the final whole-goal validation batch.

Native ListToolbar, ListFooter, HierarchyToolbar and ToolbarButton provide retained
slots, stable-ID selection, typed event payloads and LVUI bindings. See
[native toolbars](docs/native/Toolbars.md). Runtime checks remain deferred.

Native [Card and Avatar](docs/native/Media.md) and the [media gallery](example/NativeMediaGallery/README.md) use retained C++23 widgets and immutable image inputs. Runtime verification remains deferred.

Native [MaterialSurface, PanelMaterial and WindowMaterial](docs/native/Materials.md) provide masked GPU blur, authored accent fields and on-demand Core color motion. See the [material gallery](example/NativeMaterialGallery/README.md); runtime execution remains deferred.

The Qt-free [text document foundation](docs/native/TextDocuments.md) provides chunked, file-backed UTF-8 storage, native UTF-16 positions, exact edit metadata, incremental edits, bounded history and iiFileProvider Native publication. TextMarkup supplies the existing inline conversion contract. Native [CodeEditor](docs/native/CodeEditor.md) connects a virtual PlainText/NoWrap viewport and IME projection to that model, rebases independent view selections and retains bounded selection metadata for undo/redo. Its native glyph cache preserves RGB and color-font palettes with separate opacity. TextEditor's rich-text viewport and full source parity remain pending; runtime checks remain deferred.

The [text document gallery](example/NativeTextDocumentGallery/README.md) connects that model to a retained LVUI/iiGUI scope without materializing the whole document during view queries.

The [native CodeEditor gallery](example/NativeCodeEditorGallery/README.md) compiles
the editor from LVUI into a Core-owned C++23 application with retained property
bindings and explicit iiFileProvider Native file publication. Build/package
checks remain separate from the final runtime batch.

The native [Tooltip](docs/native/Tooltip.md) adds anchored rounded-tail placement,
PanelMaterial, retained scrollable content, focus-preserving interaction and
Core-owned show/hide/expiry deadlines. The [Tooltip gallery](example/NativeTooltipGallery/README.md)
uses real LVUI AOT bindings and one retained C++ backend. Runtime checks are deferred.


Native presentation telemetry is provided by `iiGUI::PresentationSource` and
`lvrs::RenderMonitor`. The host publishes accepted presentation submissions via
its existing Core event loop; LVUI projects native statistics and commands.
Submission timing is distinct from scanout and GPU completion. Native LVRS
execution and full platform/legacy parity remain part of the final batch.

The [Native Creation Workbench](example/NativeCreationWorkbench/README.md) combines
LVUI controls with retained C++ canvas/timeline providers and real procedural
frame generation on iiSDKCore. Its final self-test checks GPU pixels, native input,
result selection and retirement; build-only evidence is not a runtime pass.

Native sources and adjacent headers are grouped by responsibility under `src/`; the installed `<LVRS/...>` include paths remain stable. See [native source layout](docs/native/SourceLayout.md).

<!-- workspace-docs:readme:start -->
## Documentation entry points

- Human overview: this English `README.md`.
- Agent instructions: [AGENT.md](AGENT.md).
- Human documentation: [documentation index](docs/README.md).
- Structured agent documentation: [agent index](docs/agent-index.json).
<!-- workspace-docs:readme:end -->
