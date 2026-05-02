# LabelButton

Location: `qml/components/control/buttons/LabelButton.qml`

`LabelButton` is the text-only button variant built on `AbstractButton`.

## Purpose

- Offer compact fixed-height text action button.
- Preserve tone behavior from base button while reducing visual noise.

## API

Inherited from `AbstractButton` plus fixed layout contract:

- default tone fallback: `Borderless`
- injected method API: `method`, `methods`, `invokeMethods(...)`
- fixed height: `Theme.gap20`
- `horizontalPadding: Theme.gap8`
- `verticalPadding: Theme.gap4`
- `cornerRadius: Theme.radiusSm`

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelButton {
    text: "Save"
    tone: LV.AbstractButton.Borderless
    method: function(eventData) {
        save()
    }
}
```

## How It Works

- Overrides `contentItem` with body-style label.
- Keeps implicit width dynamic by label width + horizontal paddings.
- Keeps implicit/explicit height locked to Figma-compatible compact contract.

## Practical Notes

- Use `LabelButton` for dense toolbars and short action labels.
- For icon-heavy actions, prefer `IconButton` to avoid label truncation pressure.

## FAQ

Q. Why does `LabelButton` keep fixed compact height even with larger text?  
A. This is by design for dense toolbar rhythm. Long labels should be shortened or moved to larger button variants.

Q. Can radius/padding be changed?  
A. Yes, inherited layout properties are overridable, but visual parity with button family presets may break.

## Mistake Patterns

- putting verbose sentence-length labels into compact buttons,
- mixing custom paddings across sibling buttons and losing row alignment.

## Validation Checklist

- tone contrast is acceptable in enabled/disabled states,
- text remains short enough to avoid heavy elision in compact layouts,
- sibling button heights match for row consistency.
