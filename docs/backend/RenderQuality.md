# RenderQuality

Location: `backend/runtime/renderquality.h` / `backend/runtime/renderquality.cpp`

`RenderQuality` controls LVRS rendering backend quality policy exposed to QML.

## Purpose

- Enforce vector-first rendering policy for SVG/icon pipelines.
- Enforce HiDPI + HiRes `@3x` supersampling policy.
- Enforce antialiasing defaults (MSAA + native text rendering).
- Apply quality settings per window and optionally as global defaults.

## Properties

- `vectorFirst: bool` (constant, `true`)
- `textVectorFirst: bool` (constant, `true`)
- `hiDpiEnabled: bool` (constant, `true`)
- `hiResScale: real` (constant, `3.0`)
- `effectiveSupersampleScaleValue: real` (constant policy value)
- `supersamplingEnabled: bool` (constant, `true`)
- `antialiasingEnabled: bool` (constant, `true`)
- `sceneSupersampling: bool` (default `true`)
- `sceneSupersamplingActive: bool` (read-only, computed from bound window size/policy)
- `sceneSupersamplePixelBudget: int` (constant, default `6000000`)
- `enabled: bool`
- `supersampleScale: real`
- `minimumSupersampleScale: real` (constant)
- `maximumSupersampleScale: real` (constant)
- `msaaSamples: int`
- `nativeTextRendering: bool`

## Methods

- `effectiveSupersampleScale(): real`
- `shouldUseSceneSupersampling(width, height): bool`
- `resolveLayerTextureSize(width, height, sceneSupersamplingActive = true): size`
- `bindWindow(window): void`
- `unbindWindow(): void`
- `applyWindow(window): void`
- `applyGlobalDefaults(): void`
- `configureGlobalDefaults(msaaSamples = 4, nativeTextRendering = true)` (static)

## Usage Pattern

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.RenderQuality.enabled = true
    LV.RenderQuality.supersampleScale = 2.5
    LV.RenderQuality.applyWindow(window)
}
```

## Behavior Notes

- Effective supersample scale is forced to HiRes `@3x`.
- Full-scene supersampling is gated by pixel budget (`width * height * scale^2 <= sceneSupersamplePixelBudget`)
  to avoid large-window latency spikes.
- Scene-supersampling activation state is computed in C++ backend and exposed as
  `sceneSupersamplingActive` for low-level policy ownership.
- Layer texture size calculation is owned by C++ (`resolveLayerTextureSize`) so QML keeps
  display-only wiring.
- `enabled`/`supersampleScale` remain API-compatible, but backend policy keeps quality-on defaults.
- Text rendering is forced to native text rendering to keep text vector-priority.
- App bootstrap initializes HiDPI/MSAA/native-text global defaults before window creation.
- GPU-friendly defaults are applied (`QSG_RHI_PREFER_SOFTWARE_RENDERER=0`, `QSG_RENDER_LOOP=threaded` when unset).

## Tuning Guidance

- High-density desktop displays: start with `supersampleScale` in `2.0..3.0`.
- Mid/low-power devices: reduce supersample scale first before disabling quality entirely.
- Keep `msaaSamples` aligned with backend/platform constraints.

## Troubleshooting

If visual quality changes do not apply:

1. verify `enabled` is true,
2. verify `applyWindow(window)` is called on the active window,
3. verify requested values are not clamped by min/max bounds.

## FAQ

Q. Should supersample scale be maximized by default?  
A. No. Choose scale by target hardware budget and visual requirement.

## Validation Checklist

- requested supersample value stays within min/max range,
- active window receives updated quality policy via `applyWindow`,
- visual improvements are measured against render cost on target hardware.
