# RadioButton

Location: `qml/components/control/check/RadioButton.qml`

`RadioButton` is a compact radio selector with compatibility aliases for legacy API names.

## Purpose

- Provide deterministic circular indicator + optional text.
- Keep backward compatibility through `state`/`available` aliases.

## API

Primary state API:

- `checked`
- `enabled`
- `text`

Compatibility aliases:

- `state` <-> `checked`
- `available` <-> `enabled`

Visual API:

- `indicatorSize`, `dotSize`
- `onColor`, `offColor`
- `onColorDisabled`, `offColorDisabled`
- `dotColor`, `dotColorDisabled`

## Usage

```qml
import LVRS 1.0 as LV

LV.RadioButton {
    text: "Choice A"
    checked: true
}
```

## How It Works

- Alias synchronization handlers keep `state` and `checked` mirrored.
- Effective colors are computed from `enabled + checked` combination.
- Background visuals remain transparent to avoid button-like surface styling.

## Advanced Example: Legacy Alias Interop

```qml
import LVRS 1.0 as LV

LV.RadioButton {
    text: "Legacy Option"
    state: true
    available: true
}
```

Both alias and canonical properties stay synchronized by internal change handlers.

## Grouping Recommendation

Use `ButtonGroup` for mutually exclusive option sets.
Without grouping, multiple radio buttons can be checked simultaneously if state logic is not wired externally.
