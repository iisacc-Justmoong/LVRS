# AbstractButton

Location: `src/qml/components/control/buttons/AbstractButton.qml`

`AbstractButton` is the shared base for LVRS button-family components.

The concrete button family follows the Figma Button page, with label/icon PushButton sets and a DropdownButton set for label/icon menu triggers. Each complete component has its own input-state variants. See [component instance states](../../instance-states.md) for current node IDs and the runtime mapping.

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
- `releaseOnSignal`: false by default; PushButton / DropdownButton enable an explicit, short release rebound
- `interaction`: one owned nonvisual `InteractionState` per component instance
- `interactionPhase` / `interactionInput` (readonly): current input phase and pointer/keyboard modality
- `interaction.focusVisible`: enabled keyboard focus, independent of press/release

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

The `PushButton` and `DropdownButton` families and their four named presets default to `Primary`, matching the component set's default `Kind=accent`. `AbstractButton` itself remains the neutral shared base and keeps its own `Default` tone.

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

## Button families

`PushButton` and `DropdownButton` are independent children of `AbstractButton`.
The former owns plain label/icon actions; the latter owns label/icon menu triggers
with a trailing chevron. Their compact 8px radius and measured padding/gaps are
scoped to those families, so `AlertButton`, `Stepper`, and other direct
`AbstractButton` consumers retain their own contracts.

## Shared motion

Press deformation is shared by every button subclass, with a focus ring for Tab navigation. See [motion policy](../../motion.md) for global speed, reduced motion, local overrides and the component-specific VisualCatalog recipe.

`showFocusRing` defaults to true. Components with a Figma-authored focus border may set it false and render that border, while retaining keyboard focus and shared motion (ColorPickerButton does this).

`focusRingOutset` defaults to `0` and `focusRingRadius` defaults to `resolvedCornerRadius`. The authored PushButton/DropdownButton and Menu/List/Hierarchy row families use a 3px outset; compact buttons also increase the ring radius to 11px. This keeps the focus outline outside the visual surface without changing the input or layout bounds. `enterKeyActivation` defaults to false and is enabled by these authored families; unrelated controls retain their existing keyboard handlers. Enter/Return gives immediate down-state feedback, ignores repeated key-down, invokes the native `click()` path at key-up (preserving checkable/action semantics), and cancels when focus is lost.

The compact button families retain the existing default, hover, pressed and keyboard-focus policy. Their release is only a 180ms elastic return of the content and surface, triggered by `released()`; no separate release fill or border is applied. A fast tap receives a minimum visible compression before returning. Cancellation does not rebound or activate. Re-pressing interrupts the animation, and reduced motion, local motion opt-out or disabled interaction clears the transform immediately. Layout and hit geometry stay fixed; callbacks execute at activation without waiting for the animation.

The background and internal focus decoration consume the instance's state object.
`interactionPhase` prioritizes disabled, press, release, focus, hover and default.
Release follows `InteractionMotion.releasing`; disabling motion ends that transient
phase while logical press/focus still works. Selection, checked values and subclass
model-state APIs remain independent. No external state overlay is required.

Enter completion/cancellation resets `down` to `undefined`, restoring Qt's native
pressed-state tracking for later pointer or Space input. Leaving an explicit false
value would suppress those later press states; see the [Qt down contract](https://doc.qt.io/qt-6/qml-qtquick-controls-abstractbutton.html#down-prop).
