# IconButton

Location: `qml/components/control/buttons/IconButton.qml`

`IconButton` is the `iconMode: true` preset of [PushButton](PushButton.md), with optional glyph/text.

## Purpose

- Provide compact icon action button with project-structure fallback icon.
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

Layout:

- default tone: `Primary` (`Kind=accent`)
- default icon frame: `Theme.iconSm` (`18 x 18` on desktop, `36 x 36` on mobile)
- fixed frame: `22 x 22` on desktop and `44 x 44` on mobile
- compact inset: `Theme.gap2` (`2` desktop, `4` mobile)
- `cornerRadius: Theme.radiusMd` (`8` desktop, `16` mobile)
- `spacing`: `0` for accent, `Theme.gap7` for the other Figma tones
- `iconGlyph` uses the same square frame as SVG icons

## Figma Visual Contract

- Source: `44:599`, `Type=IconButton`.
- The icon occupies `x=2, y=2, 18 x 18` inside the desktop `22 x 22` frame.
- The fallback `generalprojectStructure.svg` matches Figma's `#CED0D6` folder and `#548AF7` modifier geometry and is reused without a replacement asset.
- All five tones preserve the same geometry; the project-structure icon keeps its own colors.

Injected methods:

- inherited from `AbstractButton`: `method`, `methods`, `hasInjectedMethods`, `invokeMethods(...)`

## Resolution Order

1. explicit `iconSource`
2. explicit `iconName`
3. `icon.name` from grouped icon API
4. default fallback icon (`projectStructure`)

Rendered source is resolved through `RenderQuality.resolveTextureSource(...)`; its square supersampled `sourceSize` tracks the logical icon size and device pixel ratio.

## Usage

```qml
import LVRS 1.0 as LV

LV.IconButton {
    iconName: "add"
    method: function(eventData) {
        addItem()
    }
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

The explicit `14` above demonstrates the supported per-instance override; the stock default remains `Theme.iconSm` (`18` on desktop, `36` on mobile).

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
