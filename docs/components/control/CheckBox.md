# CheckBox

Location: `qml/components/control/check/CheckBox.qml`

`CheckBox` is a compact checkable control with optional text label.

## Purpose

- Provide deterministic check indicator rendering independent of platform style.
- Support explicit palette overrides for checked/unchecked/disabled states.

## API

State:

- `checked` (inherited)
- `enabled` (inherited)
- `text` (inherited)

Visual customization:

- `boxSize`
- `checkColor`
- `checkedColor`
- `uncheckedColor`
- `disabledCheckedColor`
- `disabledUncheckedColor`
- `checkMarkColorDisabled`
- `checkMarkStrokeWidth`

## Usage

```qml
import LVRS 1.0 as LV

LV.CheckBox {
    text: "Remember me"
    checked: true
}
```

## How It Works

- Component is `checkable` and borderless by default.
- Check mark is painted by `Canvas` and repainted on state/color changes.
- Background interaction states are forced transparent to keep the control visually minimal.

## Advanced Example: Custom Checkmark Weight

```qml
import LVRS 1.0 as LV

LV.CheckBox {
    text: "Strict validation"
    checkMarkStrokeWidth: 2
    checkedColor: "#3A8DFF"
}
```

## Troubleshooting

If checkmark appears clipped, verify control/container height and custom `boxSize` combination.

## State Management Recommendation

For dynamic lists, bind each checkbox directly to model index properties to avoid stale closure/index bugs during reorder operations.
