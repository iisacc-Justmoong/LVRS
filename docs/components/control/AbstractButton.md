# AbstractButton

Location: `qml/components/control/buttons/AbstractButton.qml`

`AbstractButton` is the base control for the LVRS button family.

## Purpose

- Provide shared tone enum and color policy.
- Unify enabled/disabled interaction gating.
- Centralize padding/radius/implicit-size baseline for derived buttons.

## Tone Enum

`AbstractButton.ButtonTone`

- `Primary`
- `Default`
- `Borderless`
- `Destructive`
- `Disabled`

## Core API

- `tone`
- `effectiveEnabled` (`enabled && tone !== Disabled`)
- `textColor`, `textColorDisabled`
- `backgroundColor`, `backgroundColorHover`, `backgroundColorPressed`, `backgroundColorDisabled`
- `horizontalPadding`, `verticalPadding`, `cornerRadius`

## Interaction Contract

- Hover/focus are disabled when `effectiveEnabled == false`.
- Disabled state installs a blocking `MouseArea` to prevent accidental event propagation.
- `Default` and `Borderless` pressed state uses `Theme.accentBlueMuted`.
- `Default` tone base fill is `Theme.panelBackground12`, disabled fill is `Theme.panelBackground04`.

## Usage

```qml
import LVRS 1.0 as LV

LV.AbstractButton {
    text: "Action"
    tone: LV.AbstractButton.Default
}
```

## How It Works

- Tone-specific text/background colors are derived by readonly computed properties.
- Derived button components override sizing/layout while inheriting interaction and tone policy.
- `contentItem` defaults to `LV.Label` (`body` style) centered both axes.

## Advanced Example: Tone Override with Custom Colors

```qml
import LVRS 1.0 as LV

LV.AbstractButton {
    text: "Dangerous"
    tone: LV.AbstractButton.Destructive
    backgroundColorPressed: "#B83333"
}
```

## Debug Tip

When a button appears non-interactive, inspect `effectiveEnabled` rather than `enabled` only.
