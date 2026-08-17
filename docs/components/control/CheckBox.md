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
- `framePadding` (desktop `0.5`, mobile `1`) keeps the indicator inside an `18 x 18` / `36 x 36` interaction frame.
- `boxRadius` (desktop `3.5`, mobile `7`), `checkMarkStrokeWidth`, and border widths retain the Figma 17px-frame ratios as the indicator scales.

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
- Under the mobile `2x` metric profile the indicator is `34 x 34`, frame padding is `1px`, gap is `12px`, radius is `7px`, and the fixed-Body label variant resolves to `81 x 36`.
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
