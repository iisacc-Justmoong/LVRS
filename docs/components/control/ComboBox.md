# ComboBox

Location: `qml/components/control/buttons/ComboBox.qml`

`ComboBox` is a compact context-menu trigger row that follows the Figma contract (`97x20`) and uses `Stepper` as the trailing indicator.

## Purpose

- Provide a lightweight selector trigger for opening/closing menu-like popups.
- Keep API minimal with only two Figma-defined properties.

## Core API

- `text` (default: `"Label"`)
- `tone` (default: `ComboBox.Primary`)
  - `ComboBox.Primary`
  - `ComboBox.Borderless`
- `arrow` (default: `Stepper.UpDown`)
  - `Stepper.UpDown`
  - `Stepper.Up`
  - `Stepper.Down`

Signals:

- `clicked()`
- `pressed()`
- `released()`
- `canceled()`

Injected methods:

- `method`: one callable injected directly into the combo trigger
- `methods`: an array of callables or command objects
- `hasInjectedMethods` (readonly)
- `createMethodEvent(triggerName)`
- `invokeMethod(candidate, eventData)`
- `invokeMethods(eventData)`

## Visual Contract

- desktop logical frame: `97 x 20`
- mobile frame matches desktop: `97 x 20`
- container paddings: left `8`, right `1`, top/bottom `1`
- radius: `Theme.radiusControl` (`5`)
- container base/hover/pressed colors:
  - `Theme.panelBackground10`
  - `Theme.panelBackground11`
  - `Theme.panelBackground12`
- label text defaults to `"Label"`, is rendered with fixed Body `13px / 13px` typography in white, and elides within the combo frame
- trailing indicator is always `Stepper`, with tone mapped from combo tone
- label area and indicator slot are positioned explicitly:
  - label slot: `x=8`, `y=3.5`, `width=70`, `height=13`
  - Figma's default `"Label"` text node occupies `x=8`, `y=3.5`, `width=33`, `height=13`; the wider production slot preserves elision for dynamic text
  - stepper frame: `x=78`, `y=1`, `size=18`
- source component set: Figma node `254:889`; all six `Tone × Arrow` variants share the same frame geometry

## Usage

```qml
import LVRS 1.0 as LV

LV.ComboBox {
    text: "Control"
    tone: LV.ComboBox.Borderless
    arrow: LV.Stepper.Down
    method: function(eventData) { menu.open() }
}
```

## Practical Notes

- `tone` affects the `Stepper` appearance only (`Primary` blue / `Borderless` transparent).
- `arrow` expresses open direction state (`Up`, `Down`, `UpDown`).
- On `clicked()`, injected `method` and `methods` run in order as part of signal dispatch.
- The component does not rely on `RowLayout` for the indicator slot, so the stepper cannot stretch or collapse when used inside other layout containers.
- When paired with `ContextMenu`, popup sizing remains independent from the fixed `ComboBox` frame; the menu may expand beyond the trigger width when content or explicit popup width requires it, and a narrow trigger-width binding no longer clamps the popup below its implicit content width.
