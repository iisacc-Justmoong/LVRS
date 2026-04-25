# AppBootstrap

Location: `backend/runtime/appbootstrap.h` / `backend/runtime/appbootstrap.cpp` / `backend/runtime/appentry.h`

`AppBootstrap` provides pre/post application initialization routines for graphics backend policy, style setup, and font fallback setup.

## API

- `lvrs::preApplicationBootstrap(options) -> AppBootstrapState`
- `lvrs::postApplicationBootstrap(app, options) -> void`
- `lvrs::loadQmlRootObjects(engine, roots, options) -> QmlRootLoadResult`
- `lvrs::runQmlAppLifecycleStage(context, hooks, stage, logDiagnostics) -> QmlBootstrapQueueResult`
- `lvrs::scheduleQmlAppLifecycleStage(receiver, context, hooks, stage, logDiagnostics) -> bool`
- `lvrs::runBootstrappedQmlApp(argc, argv, launchSpec) -> int`

For Qt Quick module apps, prefer `backend/runtime/appentry.h` and `QmlAppLaunchSpec` as the standard wrapper around the pre/post bootstrap sequence. It keeps the required bootstrap order intact and can seed root QML properties through `initialProperties` before `QQmlApplicationEngine::loadFromModule(...)`. `runBootstrappedQmlApp()` now delegates root creation to `loadQmlRootObjects()`, so downstream apps can load one root through the legacy `moduleUri/rootObject` fields or several roots through `QmlAppLaunchSpec::roots`.

## QML Root Loading

`QmlRootLoadSpec` describes one root object:

- `moduleUri: QString`
- `rootObject: QString` (default `Main`)
- `initialProperties: QVariantMap`
- `windowActivationPolicy: QmlWindowActivationPolicy` (default `Inherit`)

`QmlAppLaunchSpec` keeps the single-root fields for compatibility and adds:

- `roots: QList<QmlRootLoadSpec>` for multi-root apps
- `windowActivationPolicy: QmlWindowActivationPolicy` as the app-level fallback

When `roots` is empty, LVRS builds one `QmlRootLoadSpec` from `moduleUri`, `rootObject`, `initialProperties`, and `windowActivationPolicy`. When `roots` is non-empty, each root can omit `moduleUri` to inherit the app-level module URI; root-specific initial properties are not merged with app-level initial properties.

`QmlRootLoadResult` reports:

- `ok`
- `errors`
- `rootObjects`
- `windows`
- `errorMessage()`

Root loading is strict: an empty module URI/root object or a `loadFromModule(...)` call that does not append a new engine root object is reported as a load failure instead of being ignored.

## Window Activation Policy

`QmlWindowActivationPolicy` controls what LVRS does when a loaded root object is a `QWindow`:

- `Inherit`: use the app/default policy
- `None`: leave QML visibility and activation untouched
- `Show`: call `show()`
- `ShowAndRaise`: call `show()` and `raise()`
- `ShowRaiseAndActivate`: call `show()`, `raise()`, and `requestActivate()`

The default app-level policy is `None` to preserve existing QML-driven visibility. Apps that previously repeated `show/raise/activate` after each root load can now set `windowActivationPolicy = QmlWindowActivationPolicy::ShowRaiseAndActivate` on the app spec or individual root specs.

## Lifecycle Hooks and Queue

`QmlAppLaunchSpec::lifecycle` provides three bootstrap stages:

- `AfterRootLoaded` (`after-root-loaded`)
- `AfterWindowActivated` (`after-window-activated`)
- `AfterFirstIdle` (`after-first-idle`)

Each stage supports a direct hook plus named queue tasks:

- `afterRootLoaded`
- `afterWindowActivated`
- `afterFirstIdle`
- `tasks: QList<QmlBootstrapTask>`

`QmlBootstrapTask` fields:

- `name: QString`
- `stage: QmlAppLifecycleStage`
- `priority: int` (lower values run first; insertion order is preserved for equal priority)
- `fatal: bool`
- `run(context, errorMessage) -> bool`

`runBootstrappedQmlApp()` runs `AfterRootLoaded` and `AfterWindowActivated` synchronously after root creation. A fatal failure in either synchronous stage aborts startup with `-1`. `AfterFirstIdle` is scheduled with a zero-delay event-loop turn through `scheduleQmlAppLifecycleStage()`, matching the common `QTimer::singleShot(0, ...)` delayed-bootstrap pattern. A fatal failure in the scheduled idle stage exits the application with `-1`.

The lifecycle context exposes:

- `application`
- `engine`
- `rootLoadResult`
- `stage`

LVRS owns only stage timing, task ordering, and diagnostics. Downstream apps still decide which domain services or data loaders belong in each stage.

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
- `LVRS bootstrap.entry.root-load-request`
- `LVRS bootstrap.entry.root-loaded`
- `LVRS bootstrap.entry.root-load-failed`
- `LVRS bootstrap.lifecycle.stage-start`
- `LVRS bootstrap.lifecycle.queue-start`
- `LVRS bootstrap.lifecycle.task-start`
- `LVRS bootstrap.lifecycle.task-complete`
- `LVRS bootstrap.lifecycle.task-failed`
- `LVRS bootstrap.lifecycle.stage-complete`

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

Multi-root app entry:

```cpp
lvrs::QmlAppLaunchSpec launchSpec;
launchSpec.bootstrap.applicationName = QStringLiteral("MyApp");
launchSpec.bootstrap.quickStyleName = QStringLiteral("Basic");
launchSpec.moduleUri = QStringLiteral("MyApp");
launchSpec.windowActivationPolicy = lvrs::QmlWindowActivationPolicy::ShowRaiseAndActivate;

lvrs::QmlRootLoadSpec shellRoot;
shellRoot.rootObject = QStringLiteral("Main");
shellRoot.initialProperties.insert(QStringLiteral("initialRoutePath"), QStringLiteral("/"));

lvrs::QmlRootLoadSpec inspectorRoot;
inspectorRoot.rootObject = QStringLiteral("InspectorWindow");

launchSpec.roots = {shellRoot, inspectorRoot};
return lvrs::runBootstrappedQmlApp(argc, argv, launchSpec);
```

Delayed bootstrap queue:

```cpp
lvrs::QmlBootstrapTask loadPresets;
loadPresets.name = QStringLiteral("load-presets");
loadPresets.stage = lvrs::QmlAppLifecycleStage::AfterFirstIdle;
loadPresets.priority = 20;
loadPresets.run = [](const lvrs::QmlAppLifecycleContext &, QString *errorMessage) {
    const bool ok = startPresetLoad();
    if (!ok && errorMessage)
        *errorMessage = QStringLiteral("Preset load failed");
    return ok;
};

launchSpec.lifecycle.afterWindowActivated = [](const lvrs::QmlAppLifecycleContext &context) {
    qInfo() << "Activated windows:" << context.rootLoadResult.windows.size();
};
launchSpec.lifecycle.tasks = {loadPresets};
```

Parallel domain loading can be nested inside a lifecycle task when several startup domains can be loaded
independently and then applied on the main thread. Use `docs/backend/BootstrapParallel.md` for the executor
contract; keep domain parsing and ViewModel mutation policy in the app.

Foreground service startup can be guarded with `ForegroundServiceGate` from an `AfterWindowActivated` task
when services should start only once after at least one root window is visible. See
`docs/backend/ForegroundServices.md`; service identity, permissions, and scheduler semantics remain app policy.

Permission bootstrap can use `PermissionRequestSequencer` inside a foreground service or lifecycle task to run
app-defined permission checks in order while preserving request history. See
`docs/backend/PermissionSequencer.md`; native permission APIs and prompt policy remain app/platform code.

## Troubleshooting

- `state.ok == false`: use `state.errorMessage` as primary root-cause string.
- Bootstrap stdout lines already include structured payloads for render defaults, backend probing, and fallback reasons; capture them in CI and crash reports when possible.
- Missing style changes: verify `quickStyleName` is set before app construction.
- Unexpected font fallback: verify bundled font resources and fallback enforcement results.

## FAQ

Q. Can `postApplicationBootstrap` be skipped?  
A. It can be skipped technically, but recommended startup baseline (fonts/fallbacks/app name) will be incomplete.
