# IconButton

Location: `qml/components/control/buttons/IconButton.qml`

`IconButton` is the icon-first button variant with optional text.

## Layout Contract

- Fixed visual height: `20px`
- Horizontal padding: `2`
- Vertical padding: `2`
- Corner radius: `Theme.radiusSm`

## Icon Resolution

Priority order:
1. explicit `iconSource` (`url`)
2. explicit `iconName`
3. grouped icon name (`icon.name`)
4. tone-based fallback icon

Rendered icon source is processed through `SvgManager.icon(...)` to match current icon-size scaling policy.

## Tone-Aware Defaults

When no explicit icon is provided:
- Borderless uses borderless symbolic icon.
- Disabled uses disabled symbolic icon.
- Others use default symbolic icon.

## Usage

```qml
LV.IconButton {
    tone: LV.AbstractButton.Default
    iconName: "add"
}
```

`iconName` is resolved from LVRS built-in icon set (`resources/iconset`), so no `qrc:/...` path is required.
