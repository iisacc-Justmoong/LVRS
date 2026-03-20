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
- Optional graphics backend bootstrap and diagnostics logging.
- Initializes default GPU cache hints (`QSG_RHI_PIPELINE_CACHE_LOAD/SAVE`) through `RenderQuality` global defaults path.

## What `postApplicationBootstrap` Does

- Applies application name (if provided).
- Loads bundled fonts from resource set.
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
- Missing style changes: verify `quickStyleName` is set before app construction.
- Unexpected font fallback: verify bundled font resources and fallback enforcement results.

## FAQ

Q. Can `postApplicationBootstrap` be skipped?  
A. It can be skipped technically, but recommended startup baseline (fonts/fallbacks/app name) will be incomplete.
