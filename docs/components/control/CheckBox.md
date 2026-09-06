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
- Set `useFigmaCheckedAssets: false` to use the compatibility Canvas renderer when a consumer needs custom checked/checkmark palette properties. `shapeCylinder` also selects that renderer automatically. Its supersampled backing store (`RenderQuality` + HiDPI) remains available and repaints on state/color/stroke changes.
- `showInnerShadow` is disabled only when checked+enabled.

## Usage

```qml
import LVRS 1.0 as LV

LV.CheckBox {
    text: "Remember"
    checked: true
}
```
