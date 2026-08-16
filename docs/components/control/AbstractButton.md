# AbstractButton

Location: `qml/components/control/buttons/AbstractButton.qml`

`AbstractButton` is the shared base for LVRS button-family components.

The concrete button family follows Figma node `44:599`, which defines four types (`LabelButton`, `IconButton`, `LabelMenuButton`, `IconMenuButton`) across five kinds (`accent`, `default`, `borderless`, `destructive`, `disabled`).

## Purpose

- Unify tone-based color policy (`Primary`, `Default`, `Borderless`, `Destructive`, `Disabled`).
- Centralize interaction gating (`effectiveEnabled`) and focus behavior.
- Provide shared paddings, radius policy, and implicit size baseline.

## Core API

Tone and shape:

- `tone` (`AbstractButton.ButtonTone`)
- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`
- `resolvedCornerRadius` (readonly)

Interaction:

- `effectiveEnabled` (readonly, `enabled && tone !== Disabled`)
- `hoverEnabled`/`focusPolicy` are derived from `effectiveEnabled`

Injected methods:

- `method`: one callable injected directly into the button
- `methods`: an array of callables or command objects
- `hasInjectedMethods` (readonly)
- `createMethodEvent(triggerName)`
- `invokeMethod(candidate, eventData)`
- `invokeMethods(eventData)`

Colors:

- `textColor`, `textColorDisabled`
- `backgroundColor`, `backgroundColorHover`, `backgroundColorPressed`, `backgroundColorDisabled`
- tone-derived readonly colors: `toneTextColor`, `toneBackgroundColor*`

Figma kind mapping:

- `accent` -> `AbstractButton.Primary` / `Theme.primary`
- `default` -> `AbstractButton.Default` / `Theme.panelBackground12`
- `borderless` -> `AbstractButton.Borderless` / transparent background and `Theme.primary` text or indicator
- `destructive` -> `AbstractButton.Destructive` / `Theme.danger`
- `disabled` -> `AbstractButton.Disabled` / `Theme.panelBackground04` and `Theme.disabledColor`

The four concrete Figma variants default to `Primary`, matching the component set's default `Kind=accent`. `AbstractButton` itself remains the neutral shared base and keeps its own `Default` tone.

Layout:

- `horizontalPadding`, `verticalPadding`
- `implicitHeight`/`implicitWidth` from content + paddings

## Behavior Contract

- `tone: Disabled` disables interaction even when `enabled: true`.
- `Borderless` tone keeps transparent base fill and uses surface hover/pressed colors.
- A blocking `MouseArea` is installed when `effectiveEnabled == false` to prevent click-through.
- If disabled while focused, the component clears focus.
- On `clicked()`, injected `method` and `methods` run in order. `method` runs before entries in `methods`.
- `methods` entries may be JavaScript functions or objects exposing `invoke(eventData)`/`trigger(eventData)`.
- `invokeMethods()` can also be called directly for manual command dispatch.

## Usage

```qml
import LVRS 1.0 as LV

LV.AbstractButton {
    text: "Action"
    tone: LV.AbstractButton.Primary
    method: function(eventData) {
        saveModel(eventData.source)
    }
    methods: [
        function(eventData) { audit("clicked", eventData.trigger) }
    ]
}
```
