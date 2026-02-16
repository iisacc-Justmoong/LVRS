# RenderQuality

Location: `backend/runtime/renderquality.h` / `backend/runtime/renderquality.cpp`

`RenderQuality` controls supersampling and text/MSAA quality defaults exposed to QML.

## Purpose

- Define supersampling scale policy.
- Apply quality settings per window and optionally as global defaults.
- Expose quality controls to runtime UI/debug tools.

## Properties

- `enabled: bool`
- `supersampleScale: real`
- `minimumSupersampleScale: real` (constant)
- `maximumSupersampleScale: real` (constant)
- `msaaSamples: int`
- `nativeTextRendering: bool`

## Methods

- `effectiveSupersampleScale(): real`
- `applyWindow(window): void`
- `applyGlobalDefaults(): void`
- `configureGlobalDefaults(msaaSamples = 8, nativeTextRendering = true)` (static)

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

- Effective supersample scale is clamped to min/max bounds.
- Disabling `enabled` effectively returns scale `1.0` behavior.
- App bootstrap can initialize global defaults before window creation.

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
