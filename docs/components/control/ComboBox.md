# ComboBox

Location: `qml/components/control/buttons/ComboBox.qml`

`ComboBox` is a compact context-menu trigger row that follows the Figma contract (`97x20`) and uses `Stepper` as the trailing indicator.

## Purpose

- Provide a lightweight selector trigger for opening/closing menu-like popups.
- Keep API minimal with only two Figma-defined properties.

## Core API

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

## Visual Contract

- fixed frame: `97 x 20`
- container paddings: left `8`, right `2`, top/bottom `2`
- radius: `Theme.radiusBase` (`6`)
- container base/hover/pressed colors:
  - `Theme.panelBackground10`
  - `Theme.panelBackground11`
  - `Theme.panelBackground12`
- label text is fixed to `"Label"` and rendered with body typography in white
- trailing indicator is always `Stepper`, with tone mapped from combo tone

## Usage

```qml
import LVRS 1.0 as LV

LV.ComboBox {
    tone: LV.ComboBox.Borderless
    arrow: LV.Stepper.Down
    onClicked: menu.open()
}
```

## Practical Notes

- `tone` affects the `Stepper` appearance only (`Primary` blue / `Borderless` transparent).
- `arrow` expresses open direction state (`Up`, `Down`, `UpDown`).
