# Slider

`LV.Slider` is the horizontal value input from the LVRS Figma Slider library. It uses Qt Quick Templates' Slider for pointer, touch, keyboard, range, and accessibility behavior, with LVRS geometry, colors, icons, and typography. No additional dependency is required.

```qml
import LVRS 1.0 as LV

LV.Slider {
    width: 320
    type: LV.Slider.CenterBiasedTicks
    size: LV.Slider.Regular
    from: -100
    to: 100
    value: 0
    stepSize: 10
    Accessible.name: "Exposure"
    onMoved: exposureModel.exposure = value
}
```

## Variants

| `type` | Appearance | Default endpoints | Default steps |
| --- | --- | --- | --- |
| `Slider.Default` | Thin track and circular thumb | Hidden | Continuous |
| `Slider.CenterBiased` | Fill starts at the range midpoint | Hidden | Continuous |
| `Slider.Ticks` | Thin track with 11 ticks | Hidden | Continuous |
| `Slider.CenterBiasedTicks` | Midpoint fill, center marker, 11 ticks | Hidden | Continuous |
| `Slider.Filled` | Capsule track, optional leading symbol | Hidden | Continuous |
| `Slider.MinMaxLabels` | Capsule track with endpoint labels | Visible | Continuous |
| `Slider.Segmented` | Capsule track, five stops and visible thumb | Visible | 25% of the range |

`type` and `size` are independent of the current value and interaction state. Ticks are visual guides; they do not impose snapping on continuous variants.

| `size` | Control height | Thin track | Thin thumb | Capsule track | Capsule thumb |
| --- | ---: | ---: | ---: | ---: | ---: |
| `Slider.Mini` | 22 | 4 | 12 | 12 | 8 |
| `Slider.Small` | 24 | 6 | 14 | 16 | 8 |
| `Slider.Regular` | 32 | 6 | 18 | 28 | 20 |
| `Slider.Large` | 44 | 8 | 22 | 44 | 36 |

The default size is Regular and the implicit width is 320. Width can change freely; endpoint text elides before it consumes the track. The metrics stay the same on desktop and mobile. These Figma designs are horizontal controls.

## Values and input

The native `from`, `to`, `value`, `position`, `visualPosition`, `stepSize`, `snapMode`, `live`, `pressed`, `hovered`, `moved()`, `increase()`, `decrease()`, and `valueAt()` API remains available. The default range is 0–1 and the initial value is 0.5. Programmatic values are clamped by Qt to the range; `moved()` is for user input, while `valueChanged` also observes programmatic updates.

Mouse/touch dragging and track presses change the value. Arrow keys adjust it, Home/End select the range endpoints, and Tab reaches the control. Mouse-wheel input is opt-in with `wheelEnabled: true`. Set an application-specific `Accessible.name` for each control.

For Segmented, `segmentCount` defaults to 5 and is clamped to at least 2. The default `stepSize` follows the range and count, and `snapMode` is `Slider.SnapAlways`. Prefer changing `segmentCount` to keep the dots and snap positions aligned. Explicitly overriding native `stepSize` or `snapMode` is supported; keep those settings consistent with the displayed stops. As with Qt's Slider, snapping governs user input, not direct assignments to `value`.

```qml
LV.Slider {
    type: LV.Slider.Segmented
    from: 1
    to: 5
    value: 3
    segmentCount: 5
    minimumLabel: "Low"
    maximumLabel: "High"
    Accessible.name: "Quality"
}
```

Center-biased types use `(from + to) / 2` as their neutral point, including nonzero or descending ranges. Filled types preserve a rounded start cap at the minimum and fill the complete track at the maximum. Right-to-left layout mirrors the track, endpoints, fill, ticks, and symbol together.

## Labels, icons, and appearance

| Property | Default / purpose |
| --- | --- |
| `showMinMax` | True for MinMaxLabels and Segmented; false otherwise |
| `minimumLabel`, `maximumLabel` | `"0%"`, `"100%"`; editable strings, not autoformatted values |
| `showLabels` | True; uses LVRS Body 13 / Pretendard Medium |
| `showEndpointIcons` | False |
| `minimumIconName`, `maximumIconName` | `"sun"`; existing LVRS icon names |
| `minimumIconSource`, `maximumIconSource` | Optional URL overrides the corresponding icon name |
| `symbolName`, `symbolSource` | `"sun"` and optional URL override |
| `showSymbol` | True; visible only in Filled Regular/Large |
| `showTicks`, `tickCount` | Type-dependent visibility; 11 thin-track ticks by default |
| `showThumb` | True for thin and Segmented types; false for Filled/MinMaxLabels |
| `showFocusRing` | False; explicit ring in addition to hover/press/keyboard focus |
| `active` | True; false uses the inactive gray fill while keeping input enabled |
| `trackColor`, `pressedTrackColor` | Panel 12 and surfaceSolid |
| `fillColor`, `inactiveFillColor` | Accent and Disabled text token |
| `thumbColor`, `labelColor`, `tickColor` | TitleHeader, Body, Caption |

The `sun` glyph is the existing LVRS resource, rendered at 18×18. The Figma icon is the same eight paths scaled from 16 to 18. Icon URLs and names can be replaced without changing the control geometry.

Use `enabled: false` for unavailable actions; this blocks input and gives the whole control 32% opacity. Use `active` for window activation independently, for example `active: applicationWindow.active`. Hover and press reveal the capsule thumb and a 1px/2px accent ring. Keyboard focus also reveals the thumb and ring.

`displayState` defaults to `Slider.Automatic`. `IdleState`, `HoverState`, `PressedState`, and `DisabledState` provide deterministic catalog/design previews. DisabledState supplies `enabled: false`; applications should use the native `enabled` property for runtime availability. Explicit QML assignments to `enabled` replace this default binding. Preview states do not write `value` or emulate pointer input.

Thumb elevation and capsule inner shadows use the same cached Canvas technique as InputField, supporting software and RHI renderers. Disabled controls use a layer so 32% opacity applies to the complete composition, preserving the translucent thumb over the fill, following [Qt's layer opacity rules](https://doc.qt.io/qt-6/qml-qtquick-item.html#layer-opacity-vs-item-opacity). The Figma capsule paint is opaque, so its background blur does not change the visible result; no offscreen background capture is needed.

## Figma and validation

- [Public Slider designs](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=859-4157): seven families × four sizes × four states.
- [Internal value geometry](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/Layerd-Visual-Render-System?node-id=859-4158): continuous positions, centered fills, capsule fills, and segments.
- [Qt Slider API](https://doc.qt.io/qt-6/qml-qtquick-controls-slider.html): inherited input/range semantics.

VisualCatalog → Control → Input → Slider contains interactive examples and the complete 112-variant matrix. `tests/tst_slider.cpp` covers Figma geometry, state preservation, resizing, range behavior, input, labels/icons, and rendered output. The installed-consumer test also instantiates every type and verifies embedded QML against source.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build -R 'LVRSTests_(slider|examples|import_api)' --output-on-failure
```

## Shared motion

Pointer tracking is direct. The thumb deforms on hold; discrete position changes settle with rebound. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.
