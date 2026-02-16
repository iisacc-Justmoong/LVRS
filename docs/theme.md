# Theme

Location: `qml/Theme.qml`

`Theme` is the central design-token singleton used by all LVRS components.

## Token Groups

- Typography: family resolution, text size/weight/style-name triplets, line-height metadata.
- Surface: window and panel background layers.
- Semantic color: `primary`, `success`, `warning`, `danger`.
- Accent palette: full iconset-derived colors for component states and diagnostics.
- Metrics: spacing, radius, control heights, icon sizes.

## Icon Path Resolution

`Theme.iconPath(iconName)` resolves a logical icon name to `qrc:/qt/qml/LVRS/resources/iconset/`.

- Empty input returns empty string.
- Full resource path (`:/`) is preserved.
- `.svg` suffix is appended automatically when omitted.

## Accent Palette

Accent colors are now split into two layers:

- Legacy semantic accent tokens (for stable component styling): `accentBlue`, `accentRed`, `accentGreen`, `accentSlateMuted`, etc.
- Full extracted iconset palette: `accentIconPaletteTokens` (`[{ name, hex, color }]`).

`accentIconPaletteTokens` is generated from `resources/iconset/*.svg` fill/stroke color values.
Current snapshot includes `386` unique colors across `2182` icons.
New non-semantic colors are exposed as deterministic descriptive names (for example `accentDeepRed`, `accentDarkerYellow`, `accentLightOrangeVividBirch`).

## Important Mappings

- `accent` aliases `primary`.
- Context menu tokens are explicit: `contextMenuSurface`, `contextMenuDivider`, `contextMenuItemSelectedBackground`, `contextMenuItemInactiveBackground`.
- Button and control defaults depend on `controlHeightSm`, `controlHeightMd`, `radiusControl`, and gap tokens.

## Usage

```qml
import LVRS 1.0 as LV

Rectangle {
    color: LV.Theme.window
}
```
