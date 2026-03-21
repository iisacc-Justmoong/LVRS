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
- Windows: Vulkan required.
- Android: Vulkan preferred. Bootstrap probes the runtime loader first and falls back to OpenGL when Vulkan is unavailable at startup.
- Linux: Qt default backend selection.
- Other targets: no hard override; Qt default backend selection is used.

If a required backend is unavailable and no platform fallback exists, bootstrap returns `ok == false` with error message.

## Build-Time Enforcement

CMake option: `LVRS_ENFORCE_VULKAN` (default `ON`)

When enabled, configuration fails early if fixed backend Qt capability cannot be validated for target platform.
This moves failures from runtime to configure/build phase.

## Runtime Diagnostics

When backend bootstrap succeeds, startup logs include backend identity and loader details when available.

Examples:

- `LVRS graphics backend: metal`
- `LVRS graphics backend: vulkan, loader = ...`

## Interaction with RenderQuality

If `configureRenderQualityDefaults` is enabled, `RenderQuality::configureGlobalDefaults()` is applied before app construction.
This keeps text/MSAA defaults aligned with backend policy while using a lighter mobile bootstrap profile before per-window device-tier presets are applied.
Global defaults also seed GPU pipeline-cache env hints (`QSG_RHI_PIPELINE_CACHE_LOAD/SAVE`).
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
- Windows: confirm Vulkan loader discovery and startup.
- Linux: confirm stable startup with Qt default backend selection.
- Android: confirm both Vulkan-capable device behavior and OpenGL fallback policy.

## Deployment Troubleshooting

If startup fails at bootstrap stage:

- inspect bootstrap error string first,
- verify Qt build includes required rendering backend feature,
- verify runtime loader/driver presence for Vulkan targets (Windows/Android),
- verify Android fallback logs when Vulkan probing fails,
- confirm no environment override is forcing incompatible graphics API.

## Operational Recommendation

Keep backend bootstrap diagnostics enabled in non-production builds and CI smoke runs.
This catches environment regressions before app-level UI logic is involved.
