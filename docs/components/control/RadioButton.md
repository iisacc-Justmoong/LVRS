# RadioButton

Location: `qml/components/control/check/RadioButton.qml`

`RadioButton` is a compact circular selector with legacy compatibility aliases.

## Purpose

- Provide deterministic radio indicator visuals.
- Keep old API (`state`, `available`) compatible with canonical Qt properties.

## Core API

Canonical state:

- `checked`, `enabled`, `text`

Compatibility aliases:

- `state` <-> `checked`
- `available` <-> `enabled`

Visual:

- `indicatorSize`, `dotSize`
- resolved radii: `indicatorRadius`, `dotRadius`
- `onColor`, `offColor`
- `onColorDisabled`, `offColorDisabled`
- `dotColor`, `dotColorDisabled`
- resolved: `indicatorColor`, `indicatorDotColor`

## Behavior Contract

- Alias pairs are synchronized both directions (`onStateChanged`, `onCheckedChanged`, `onAvailableChanged`, `onEnabledChanged`).
- The Figma component set (`44:630`) contains four `18 x 18` variants: On/Off crossed with Available True/False.
- On states place an `8 x 8` dot at `(5, 5)` inside the `18 x 18` indicator. The outer and inner radii are `9` and `4` respectively.
- The four-state palette is fixed to the Figma tokens: enabled On uses `Accent` with a `TitleHeader` dot, enabled Off uses `TitleHeader`, disabled On uses `PanelBackground12` with a `Caption` dot, and disabled Off uses `PanelBackground12`.
- Under the mobile `2x` metric profile the indicator becomes `36 x 36`, the dot becomes `16 x 16` at `(10, 10)`, and the radii become `18` and `8`.
- The optional QML label extension uses an `8px` desktop / `16px` mobile gap. Its Body font remains fixed at `13px/13px`, producing `59 x 18` desktop and `85 x 36` mobile bounds for the text `Label`.
- Indicator geometry is explicitly positioned so runtime target changes recompute its implicit size deterministically.
- Component uses transparent background policy (`tone: Borderless`).

## Usage

```qml
import LVRS 1.0 as LV

LV.RadioButton {
    text: "Choice A"
    checked: true
}
```
