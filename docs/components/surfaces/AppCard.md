# AppCard

Location: `qml/components/surfaces/AppCard.qml`

`AppCard` is a reusable titled surface with header, separator, and flexible content slot.

## Purpose

- Provide consistent card container for forms, summaries, and dashboard modules.
- Keep header/content spacing and radius policy centralized.

## Core API

Header:

- `title`
- `subtitle`

Shape and spacing:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`, `resolvedCornerRadius`
- `cardPadding` (readonly)
- `sectionSpacing` (readonly)

Content:

- default `content` slot (`contentSlot.data`)

## Behavior Contract

- Implicit width/height are computed from header/content size + card paddings.
- Subtitle row is hidden when `subtitle` is empty.
- Content region height is driven by slotted children `childrenRect`.

## Usage

```qml
import LVRS 1.0 as LV

LV.AppCard {
    title: "System Health"
    subtitle: "Last 15 minutes"

    LV.Label { text: "No incidents" }
}
```
