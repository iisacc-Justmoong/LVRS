# AppBootstrap

Location: `backend/runtime/appbootstrap.h` / `backend/runtime/appbootstrap.cpp` / `backend/runtime/appentry.h`

`AppBootstrap` provides pre/post application initialization routines for graphics backend policy, style setup, and font fallback setup.

## API

- `lvrs::preApplicationBootstrap(options) -> AppBootstrapState`
- `lvrs::postApplicationBootstrap(app, options) -> void`
- `lvrs::runBootstrappedQmlApp(argc, argv, launchSpec) -> int`

For Qt Quick module apps, prefer `backend/runtime/appentry.h` and `QmlAppLaunchSpec` as the standard wrapper around the pre/post bootstrap sequence. It keeps the required bootstrap order intact and can seed root QML properties through `initialProperties` before `QQmlApplicationEngine::loadFromModule(...)`.

## `AppBootstrapOptions`

- `applicationName: QString`
- `quickStyleName: QString`
- `configureRenderQualityDefaults: bool` (default `true`)
- `bootstrapGraphicsBackend: bool` (default `true`)
- `logBootstrapDiagnostics: bool` (default `true`)
- `logGraphicsBackend: bool` (default `true`)
- `installBundledFonts: bool` (default `true`)
- `installPretendardFallbacks: bool` (default `true`)
- `enforcePretendardFallback: bool` (default `true`)

## `AppBootstrapState`

- `ok: bool`
- `errorMessage: QString`
- `graphicsBackend: GraphicsBackendBootstrapResult`

## Required Call Order

1. Call `preApplicationBootstrap` before creating `QGuiApplication`.
2. If `state.ok == false`, abort startup.
3. Construct `QGuiApplication`.
4. Call `postApplicationBootstrap` immediately after app creation.

## What `preApplicationBootstrap` Does

- Optional `RenderQuality::configureGlobalDefaults()`.
- Optional `QQuickStyle::setStyle(quickStyleName)`.
- Stage-by-stage bootstrap diagnostics logging with compact JSON payloads.
- Optional graphics backend bootstrap and diagnostics logging.
- Seeds scenegraph environment hints (for example pipeline-cache and atlas sizing) through the platform bootstrap profile before `RenderQuality` global defaults are applied.
- Keeps bootstrap responsibility limited to backend selection and Qt pre-window defaults; live quality policy remains owned by runtime `RenderQuality.applyWindow(...)`.

### Platform bootstrap policy

- macOS / iOS: fixed Metal backend; `4x/3` (macOS) or `4x/2` (iOS) MSAA/frames-in-flight bootstrap profile.
- Windows: D3D11-first bootstrap with runtime probing and OpenGL fallback; `4x/2` bootstrap render profile.
- Android: Vulkan-first bootstrap with OpenGL fallback; `4x/2` bootstrap render profile and reduced texture-atlas edge.
- Linux: Qt default backend selection; `4x/2` bootstrap render profile.
- WASM: Qt default backend selection; lighter `2x/1` bootstrap render profile with partial-update, batch-renderer, and pipeline-cache hints explicitly forced off, and without forcing desktop-grade depth/stencil defaults at bootstrap.

### Diagnostics output

When `logBootstrapDiagnostics == true`, bootstrap writes compact structured lines to stdout. Major events include:

- `LVRS bootstrap.pre.options`
- `LVRS bootstrap.pre.render-quality`
- `LVRS bootstrap.pre.quick-style`
- `LVRS bootstrap.graphics.probe`
- `LVRS bootstrap.graphics.selected`
- `LVRS bootstrap.graphics.fallback`
- `LVRS bootstrap.pre.complete`
- `LVRS bootstrap.post.application`
- `LVRS bootstrap.post.font-policy`
- `LVRS bootstrap.entry.import-paths`
- `LVRS bootstrap.entry.load-request`

The payload includes platform tag, requested bootstrap options, effective render-profile defaults, scenegraph environment values, backend probe candidates, fallback reasons, and font-policy decisions.

## What `postApplicationBootstrap` Does

- Applies application name (if provided).
- Loads bundled fonts from the shared `FontPolicy` bootstrap path.
- Installs Pretendard fallbacks.
- Optionally enforces Pretendard fallback and warns if enforcement fails.

## Typical C++ Usage

```cpp
lvrs::AppBootstrapOptions options;
options.applicationName = QStringLiteral("MyApp");
options.quickStyleName = QStringLiteral("Basic");

const lvrs::AppBootstrapState state = lvrs::preApplicationBootstrap(options);
if (!state.ok)
    return -1;

QGuiApplication app(argc, argv);
lvrs::postApplicationBootstrap(app, options);
```

## Option Tuning Examples

Minimal startup (disable backend bootstrap for constrained test harness):

```cpp
lvrs::AppBootstrapOptions options;
options.bootstrapGraphicsBackend = false;
options.logGraphicsBackend = false;
```

Font-focused startup (keep typography policies only):

```cpp
lvrs::AppBootstrapOptions options;
options.configureRenderQualityDefaults = false;
options.bootstrapGraphicsBackend = false;
options.installBundledFonts = true;
options.installPretendardFallbacks = true;
```

## Troubleshooting

- `state.ok == false`: use `state.errorMessage` as primary root-cause string.
- Bootstrap stdout lines already include structured payloads for render defaults, backend probing, and fallback reasons; capture them in CI and crash reports when possible.
- Missing style changes: verify `quickStyleName` is set before app construction.
- Unexpected font fallback: verify bundled font resources and fallback enforcement results.

## FAQ

Q. Can `postApplicationBootstrap` be skipped?  
A. It can be skipped technically, but recommended startup baseline (fonts/fallbacks/app name) will be incomplete.
