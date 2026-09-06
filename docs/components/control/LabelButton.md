# LabelButton

Location: `qml/components/control/buttons/LabelButton.qml`

`LabelButton` is the text-only [PushButton](PushButton.md) preset.

## Purpose

- Offer compact fixed-height text action button.
- Preserve tone behavior from base button while reducing visual noise.

## API

Inherited from `PushButton` plus its label-mode defaults:

- default tone: `Primary` (`Kind=accent`)
- injected method API: `method`, `methods`, `invokeMethods(...)`
- fixed height: `Theme.iconSm + Theme.gap2 * 2` (`22` desktop, `44` mobile)
- `horizontalPadding: Theme.gap8` (`8` desktop, `16` mobile)
- vertical padding is derived from the fixed frame and Body line height (`4.5` desktop, `15.5` mobile)
- `cornerRadius: Theme.radiusMd` (`8` desktop, `16` mobile)
- `spacing: Theme.gap10` (`10` desktop; a single label does not consume a gap)

## Figma Visual Contract

- Source: `44:599`, `Type=LabelButton`.
- With text `Button`, the desktop frame is `56 x 22`; the text bounds are `x=8, y=4.5, 40 x 13`.
- Typography is Pretendard Body `13px Medium / 13px` and remains `13px` on mobile.
- The corresponding mobile token composition is `72 x 44`.
- All five tones preserve the same geometry.

## Usage

```qml
import LVRS 1.0 as LV

LV.LabelButton {
    text: "Save"
    method: function(eventData) {
        save()
    }
}
```

## How It Works

- Uses the `PushButton` label content with `iconMode: false`.
- Rounds the label width up to a whole logical pixel, then adds horizontal paddings.
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
