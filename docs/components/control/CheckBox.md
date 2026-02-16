# CheckBox

Location: `qml/components/control/check/CheckBox.qml`

`CheckBox` is a compact checkable control with optional text label.
Current defaults are aligned to Figma node `44:724` (`State=On/Off` x `Available=True/False`).

## Purpose

- Provide deterministic check indicator rendering independent of platform style.
- Match Figma checkbox states with explicit color and border contracts.
- Support explicit palette overrides for checked/unchecked/disabled states.

## API

State:

- `checked` (inherited)
- `enabled` (inherited)
- `text` (inherited)

Visual customization:

- `boxSize`
- `boxRadius`
- `checkColor`
- `checkedColor`
- `uncheckedColor`
- `disabledCheckedColor`
- `disabledUncheckedColor`
- `checkMarkColorDisabled`
- `checkMarkStrokeWidth`
- `boxBorderWidthCheckedEnabled`
- `boxBorderWidthCheckedDisabled`
- `boxBorderWidthUncheckedEnabled`
- `boxBorderWidthUncheckedDisabled`
- `boxBorderColorCheckedEnabled`
- `boxBorderColorCheckedDisabled`
- `boxBorderColorUncheckedEnabled`
- `boxBorderColorUncheckedDisabled`
- `innerShadowSoftColor`
- `innerShadowStrongColor`

Resolved read-only state:

- `showInnerShadow`
- `resolvedBoxBorderWidth`
- `resolvedBoxBorderColor`

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
- Indicator box uses explicit state mapping:
  - Checked + enabled: accent fill, no border, no inner shadow.
  - Checked + disabled: `panelBackground12` fill, `0.5` border, inner shadow.
  - Unchecked + enabled: `bodyColor` fill, `0.5` border, inner shadow.
  - Unchecked + disabled: `panelBackground12` fill, no border, inner shadow.
- Check mark is painted by `Canvas` and repainted on checked/enabled/color/stroke changes.
- Label uses body typography (`12 / Medium`) with color mapping:
  - Enabled: `Theme.bodyColor`
  - Disabled: `Theme.disabledColor`
- Background interaction states stay transparent so only indicator/label visuals are rendered.

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
