# AppHeader

Location: `qml/components/layout/AppHeader.qml`

`AppHeader` is a top toolbar component with optional menu trigger, title/subtitle block, and trailing action slot.

## Purpose

- Provide a reusable page header with consistent spacing/typography.
- Support compact mode by reducing paddings and spacing.

## API

Text and behavior:

- `title`
- `subtitle`
- `menuVisible`
- `menuClicked()` signal

Density/layout controls:

- `compact`
- `contentHorizontalPadding`
- `contentVerticalPadding`
- `rowSpacing`
- `actionSpacing`
- `menuButtonPadding`

Slots:

- default `actions` slot (`actionRow.data`)

## Usage

```qml
import LVRS 1.0 as LV

LV.AppHeader {
    title: "Runs"
    subtitle: "Production"
    menuVisible: true
    onMenuClicked: openDrawer()

    LV.IconButton { iconName: "add" }
}
```

## How It Works

- Implicit height is computed from row implicit height and minimum header policy.
- Menu button appears only when `menuVisible == true`.
- Title block takes fill width; trailing action row aligns right.

## Extended Example: Compact Header Variant

```qml
import LVRS 1.0 as LV

LV.AppHeader {
    compact: true
    title: "Inspector"
    subtitle: "Selection Details"

    LV.LabelButton {
        text: "Apply"
        tone: LV.AbstractButton.Primary
    }
}
```

## Implementation Notes

- Compact mode adjusts internal spacing values instead of switching a separate style preset.
- Action slot is a default property alias, so child controls can be declared directly inside the component.
