# AppCard

Location: `qml/components/surfaces/AppCard.qml`

`AppCard` is a titled surface container with separator and flexible content slot.

## Purpose

- Provide reusable card shell for metrics/forms/summary blocks.
- Keep internal spacing and typography consistent.

## API

- `title`
- `subtitle`
- `cardPadding` (readonly)
- `sectionSpacing` (readonly)
- default `content` slot

## Layout Contract

- Header block: title + optional subtitle.
- Separator line below header.
- Flexible content region sized by child `childrenRect`.

Implicit size is computed from header/content width and padding contract.

## Usage

```qml
import LVRS 1.0 as LV

LV.AppCard {
    title: "System Health"
    subtitle: "Last 15 minutes"

    LV.Label { text: "No incidents detected"; style: body }
}
```

## Advanced Example: Form Card with Footer Actions

```qml
import LVRS 1.0 as LV

LV.AppCard {
    title: "Profile"
    subtitle: "Account Settings"

    LV.VStack {
        spacing: 8
        LV.InputField { placeholderText: "Display name" }
        LV.HStack {
            LV.Spacer {}
            LV.LabelButton { text: "Save"; tone: LV.AbstractButton.Primary }
        }
    }
}
```

## Note

Keep heavy scrollable content outside card when possible; `AppCard` is intended as compact surface block, not full-page container.

## FAQ

Q. Should `AppCard` manage page-level scrolling?  
A. No. Keep page scroll ownership outside and use cards as content blocks.
