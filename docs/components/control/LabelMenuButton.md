# LabelMenuButton

Location: `qml/components/control/buttons/LabelMenuButton.qml`

`LabelMenuButton` is a text + chevron menu trigger built on `AbstractButton`.

## Purpose

- Provide compact menu invocation button for text-first toolbars.
- Keep indicator semantics aligned with tone/disabled state.

## API

Inherited `AbstractButton` API plus indicator behavior.

Layout contract:

- fixed height: `Theme.gap20`
- `horizontalPadding: Theme.gap8`
- `verticalPadding: Theme.gap2`
- `spacing: Theme.gap2`

## Indicator Mapping

- disabled -> `panDownSymbolicDisabled`
- borderless -> `panDownSymbolicBorderless`
- primary/destructive -> `panDownSymbolicAccent`
- default -> `panDownSymbolicDefault`

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelMenuButton {
    text: "Options"
    tone: LV.AbstractButton.Default
}
```

## Advanced Example: Borderless Toolbar Menu Trigger

```qml
import LVRS 1.0 as LV

LV.LabelMenuButton {
    text: "More"
    tone: LV.AbstractButton.Borderless
}
```

Indicator and text styles remain tone-aware while preserving compact geometry.

## FAQ

Q. Why is indicator color different between `Default` and `Primary` tones?  
A. Tone mapping intentionally uses accent indicator for high-emphasis tones.

Q. Can the indicator be replaced with a custom icon?  
A. Component contract does not expose direct indicator source override. Use a custom derivative component when required.

## Validation Checklist

- indicator tone mapping is correct for each button tone,
- text and indicator remain vertically aligned in compact rows,
- disabled state prevents focus/interaction propagation.
