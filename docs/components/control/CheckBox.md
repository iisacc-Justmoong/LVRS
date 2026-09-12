# CheckBox

Location: `qml/components/control/check/CheckBox.qml`

`CheckBox` is a custom-painted checkbox (`AbstractButton` based) with deterministic state visuals.

## Purpose

- Keep checkbox visuals independent from platform style variance.
- Expose explicit checked/unchecked + enabled/disabled palette and border policy.

## Core API

State:

- `checked` (inherited)
- `enabled` (inherited)
- `text` (inherited)

Shape and metrics:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `boxSize` (desktop `17 x 17`, mobile `34 x 34`)
- `framePadding` (`0.5`) keeps the indicator inside an `18 x 18` interaction frame on desktop and mobile.
- `boxRadius` (`3.5`), `checkMarkStrokeWidth`, and border widths retain the Figma 17px-frame ratios on desktop and mobile.

Palette and border:

- `checkedColor`, `uncheckedColor`
- `disabledCheckedColor`, `disabledUncheckedColor`
- `checkColor`, `checkMarkColorDisabled`
- `boxBorderWidth*`, `boxBorderColor*`
- `innerShadowSoftColor`, `innerShadowStrongColor`
- `useFigmaCheckedAssets`
- `checkedAssetSourceEnabled`, `checkedAssetSourceDisabled`

Resolved values:

- `resolvedCheckedFillColor`, `resolvedUncheckedFillColor`
- `resolvedCheckedAssetSource`, `usingFigmaCheckedAsset`
- `resolvedBoxRadius`
- `resolvedBoxBorderWidth`, `resolvedBoxBorderColor`
- `showInnerShadow`

## Behavior Contract

- `checkable: true`, `tone: Borderless`, transparent background layers.
- The Figma component set (`44:724`) contains four `57 x 18` desktop variants: checked/unchecked crossed with enabled/disabled.
- The indicator begins at `(0.5, 0.5)`, the label begins at `(23.5, 2.5)`, and the indicator-to-label gap is `6px`. Body text remains fixed at `13px/13px` Medium.
- Mobile uses the same `17 x 17` indicator, `0.5px` frame padding, `6px` gap, `3.5px` radius, and `57 x 18` labeled bounds as desktop.
- Checked states use the exact exported Figma SVG assets by default: enabled is `#0A84FF` with an 80% white mark; disabled is `Theme.panelBackground12` with a 30% white mark and the exported inner-shadow treatment.
- `useFigmaCheckedAssets` defaults to true for the original blue checked color and false for a custom `checkedColor`, including an app's custom `ApplicationWindow.primaryColor`. The drawn checkmark then exposes the configured fill instead of covering it with the stock blue image. Set it explicitly to true to opt into custom snapshot assets, or false for custom checkmark palette properties. `shapeCylinder` also selects the Canvas renderer automatically. Its supersampled backing store (`RenderQuality` + HiDPI) remains available and repaints on state/color/stroke changes and renderer switches.
- `showInnerShadow` is disabled only when checked+enabled.

## Usage

```qml
import LVRS 1.0 as LV

LV.CheckBox {
    text: "Remember"
    checked: true
}
```

## Shared motion

The checked asset or drawn mark grows into place; the fill blends and the button rebounds. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.
