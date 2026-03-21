# Rendering Backend Policy

This document defines how LVRS selects, validates, and reports graphics backend policy across platforms.

## Bootstrap Entry

Primary bootstrap path:

- `lvrs::preApplicationBootstrap(options)`
- `lvrs::postApplicationBootstrap(app, options)`

Location: `backend/runtime/appbootstrap.h`, `backend/runtime/appbootstrap.cpp`

`preApplicationBootstrap` is responsible for graphics backend bootstrap and optional render-quality global defaults.

## Platform Backend Matrix

- macOS / iOS: Metal required.
- Windows: D3D11 preferred. Bootstrap probes the DirectX runtime first and falls back to OpenGL when D3D11 cannot be initialized.
- Android: Vulkan preferred. Bootstrap probes the runtime loader first and falls back to OpenGL when Vulkan is unavailable at startup.
- Linux: Qt default backend selection.
- WASM / other targets: no hard override; Qt default backend selection is used.

If a required backend is unavailable and no platform fallback exists, bootstrap returns `ok == false` with error message.

## Build-Time Enforcement

CMake option: `LVRS_ENFORCE_VULKAN` (default `ON`)

When enabled, configuration fails early if fixed backend Qt capability cannot be validated for target platform.
This moves failures from runtime to configure/build phase.

## Runtime Diagnostics

When bootstrap diagnostics are enabled, startup logs include stage-by-stage compact JSON payloads for render defaults, environment seeding, backend probe candidates, selected loader, and fallback reasons.

Examples:

- `LVRS bootstrap.pre.render-quality {"platform":"android", ...}`
- `LVRS bootstrap.graphics.probe {"requestedBackend":"vulkan","candidates":[...], ...}`
- `LVRS bootstrap.graphics.selected {"requestedBackend":"d3d11","selectedBackend":"d3d11","loader":"d3d11", ...}`
- `LVRS bootstrap.graphics.fallback {"requestedBackend":"vulkan","selectedBackend":"opengl","reason":"...", ...}`
- `LVRS graphics backend: opengl, loader = windows-fallback`

## Interaction with RenderQuality

If `configureRenderQualityDefaults` is enabled, `RenderQuality::configureGlobalDefaults()` is applied before app construction.
This keeps text/MSAA defaults aligned with backend policy while using a platform-tuned bootstrap profile before per-window device-tier presets are applied.
The bootstrap profile also seeds scenegraph env hints such as pipeline-cache enablement and atlas sizing before global defaults are applied.
Android/iOS keep reduced atlas sizing but no longer lower the visible MSAA floor during bootstrap, while WASM uses a lighter single-frame bootstrap profile, explicitly disables batch/pipeline-cache hints by default, and does not force desktop depth/stencil defaults during bootstrap.
Per-window PSO cache file binding and device-tier presets are then applied at `RenderQuality.applyWindow(...)` / `RenderQuality.applyDeviceTierPreset(...)`.

## Failure Handling Guidance

If bootstrap fails:

1. stop startup immediately,
2. print bootstrap error message,
3. verify platform SDK/runtime (Metal/Vulkan) and Qt feature support,
4. rerun with explicit diagnostics enabled.

## Related Files

- `backend/runtime/vulkanbootstrap.h`
- `backend/runtime/vulkanbootstrap.cpp`
- `backend/runtime/renderquality.h`
- `backend/runtime/renderquality.cpp`

## Practical Target Matrix Validation

Before shipping, validate one real device/simulator per target family:

- macOS: confirm Metal backend selection and stable startup.
- iOS: confirm Metal path and text rendering quality.
- Windows: confirm both the D3D11 path and the OpenGL fallback path can reach first frame.
- Linux: confirm stable startup with Qt default backend selection.
- Android: confirm both Vulkan-capable device behavior and OpenGL fallback policy.
- WASM: confirm browser startup with the lighter bootstrap profile.

## Deployment Troubleshooting

If startup fails at bootstrap stage:

- inspect `LVRS bootstrap.pre.*` / `LVRS bootstrap.graphics.*` lines first,
- verify Qt build includes required rendering backend feature,
- verify runtime loader/driver presence for fallback-capable targets (Windows D3D11, Android Vulkan),
- verify Android fallback logs when Vulkan probing fails,
- verify Windows fallback logs when D3D11 probing fails,
- confirm no environment override is forcing incompatible graphics API.

## Operational Recommendation

Keep backend bootstrap diagnostics enabled in non-production builds and CI smoke runs.
This catches environment regressions before app-level UI logic is involved.
