# ToggleSwitch

Location: `qml/components/control/check/ToggleSwitch.qml`

`ToggleSwitch` is an LVRS-styled switch built on `QtQuick.Controls.Switch` and aligned to the Figma two-state toggle set.

## Purpose

- Keep switch geometry and animation deterministic across platforms.
- Expose explicit track, knob, shadow, palette, and shape control.

## Core API

Shape and metrics:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `trackWidth`, `trackHeight`, `trackPadding`
- `knobSize`
- `trackCornerRadius`, `knobCornerRadius`
- `transitionDuration`: total travel/rebound time in milliseconds, `320` by default; `0` disables motion.

Figma shadow:

- `trackShadowEnabled`
- `trackShadowColor`, `trackShadowOpacity`
- `trackShadowBlur`
- `trackShadowHorizontalOffset`, `trackShadowVerticalOffset`

Palette:

- `onColor`, `offColor`
- `onColorHover`, `onColorPressed`
- `offColorHover`, `offColorPressed`
- `disabledTrackColor`
- `trackShadowColor`
- `knobFillColor`
- resolved: `resolvedTrackColor`

State (inherited):

- `checked`, `enabled`, `text`

Figma compatibility:

- `state` <-> `checked`

## Behavior Contract

- The Figma component set (`111:379`) contains On and Off variants, each `38 x 22` with `2px` inner padding, an `18 x 18` knob, and a `20px` authored radius.
- On resolves to `Theme.accent`; Off resolves to `Theme.panelBackground12`; the knob resolves to `Theme.titleHeaderColor`.
- The track uses the Figma `0px 4px 4px` shadow with `Theme.shadowStrong` (`25%` black). `QtQuick.Effects.MultiEffect` supplies the blur and automatically padded overflow.
- Pressing by mouse, touch, or Space briefly squashes the knob. It stretches along its travel, passes the destination slightly, then rebounds into its resting circle. The default travel/rebound takes `320ms`; press feedback takes one quarter of that duration.
- Knob x-position rests at `2` (Off) and `18` (On). A bounded `OutBack` easing curve supplies the rebound; a center-origin `Scale` deforms the circle without changing its `18 x 18` layout or hit target. This uses existing Qt Quick animation types and adds no dependency.
- Dragging follows the inherited Switch `visualPosition` directly while held, including right-to-left mirroring. Releasing resumes the settling animation. Rapid toggles retarget from the current visual position; animation never delays or repeats the `toggled` signal.
- Programmatic `checked`/`state` changes animate as well. Initial state renders at rest; subsequent changes on disabled controls are immediate. Set `transitionDuration: 0` for immediate state changes and no press deformation.
- Knob fill is an antialiased scene-graph `Rectangle`, so its circle remains resolution-independent without a raster canvas. The previous read-only `knobSupersampleScale`, `knobHiDpiScale`, and `knobRasterScale` values remain available for source compatibility.
- Track color resolves from `checked + hovered + down + enabled`.
- Desktop and mobile share the `38 x 22` track, `2` padding, `18 x 18` knob, radii `20` and `9`, On/Off x positions `18`/`2`, and shadow blur/vertical offset `4`.
- Optional label spacing remains `8px`, with a Body font and line height of `13px/13px`, on desktop and mobile.

## Usage

```qml
import LVRS 1.0 as LV

LV.ToggleSwitch {
    checked: true
}
```

The Visual Catalog Selection → ToggleSwitch preview includes interactive On/Off examples, a slower rebound, immediate motion, and disabled states. Hold, tap, drag, or focus a switch and press Space to compare the feedback.

## Validation

`ctest --test-dir build -R LVRSTests_toggle_switch --output-on-failure` runs Qt Test input and geometry checks for press deformation, rebound in both directions, rapid retargeting, keyboard/cancelled input, touch, drag/mirroring, and immediate/disabled states. `LVRSTests_import_api` retains the Figma resting-geometry and embedded-QML/source checks.

For native rendering inspection, set `LVRS_TOGGLE_CAPTURE_DIR` to a directory under `build/` when running `LVRSTests_toggle_switch press_and_rebound`. It saves 4x magnified window frames and CSV timestamps/knob geometry without altering the control's layout metrics. On macOS, use `QT_QPA_PLATFORM=cocoa QSG_RHI_BACKEND=metal` and the build-tree library/QML paths.

Qt references: [PropertyAnimation easing](https://doc.qt.io/qt-6.8/qml-qtquick-propertyanimation.html#easing-prop), [Switch visualPosition](https://doc.qt.io/qt-6.8/qml-qtquick-controls-switch.html#visualPosition-prop).

## Shared motion

The knob stretches during travel and rebounds into the selected endpoint. Slow and immediate variants are provided. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.
