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
- `onColor`, `offColor`
- `onColorDisabled`, `offColorDisabled`
- `dotColor`, `dotColorDisabled`
- resolved: `indicatorColor`, `indicatorDotColor`

## Behavior Contract

- Alias pairs are synchronized both directions (`onStateChanged`, `onCheckedChanged`, `onAvailableChanged`, `onEnabledChanged`).
- Indicator color reacts to hover/down when enabled.
- Component uses transparent background policy (`tone: Borderless`).

## Usage

```qml
import LVRS 1.0 as LV

LV.RadioButton {
    text: "Choice A"
    checked: true
}
```
