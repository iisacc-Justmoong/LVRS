# IconButton

Location: `qml/components/control/buttons/IconButton.qml`

`IconButton` is an icon-first button variant with optional glyph/text.

## Purpose

- Provide compact icon action button with tone-aware icon fallback.
- Support icon source by URL, icon name token, or glyph text.

## API

Icon inputs:

- `iconSource` (alias to internal `url`)
- `iconName`
- `iconGlyph`
- `iconSize`

Computed/icon resolution:

- `resolvedIconName`
- `resolvedIconSource`
- `renderedIconSource`

Layout:

- fixed height: `Theme.gap20`
- compact paddings (`Theme.gap2`)

## Resolution Order

1. explicit `iconSource`
2. explicit `iconName`
3. `icon.name` from grouped icon API
4. tone-based default icon

Rendered source is normalized through `SvgManager.icon(...)` and refreshed by `SvgManager.revision`.

## Usage

```qml
import LVRS 1.0 as LV

LV.IconButton {
    tone: LV.AbstractButton.Borderless
    iconName: "add"
}
```

## Advanced Example: Glyph Fallback Icon

```qml
import LVRS 1.0 as LV

LV.IconButton {
    iconGlyph: "+"
    iconSize: 14
    text: "Add"
}
```

## Troubleshooting

If icon does not render:

1. verify icon name exists in icon set,
2. verify resource path from `Theme.iconPath(...)`,
3. verify SVG manager revision updates are not blocked.

## Recipe: Toolbar Icon Action + Tooltip

```qml
import QtQuick
import LVRS 1.0 as LV

LV.IconButton {
    id: refreshButton
    iconName: "refresh"
    tone: LV.AbstractButton.Borderless
    ToolTip.visible: hovered
    ToolTip.text: "Refresh"
}
```

## Production Notes

- Keep icon names semantically stable and avoid opaque one-off names.
- Prefer design-token icon size values for consistency with neighboring controls.
