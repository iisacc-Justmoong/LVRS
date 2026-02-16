# Theme

Location: `qml/Theme.qml`

`Theme` is the global design-token singleton for LVRS QML components.

## Token Groups

- Typography: font family resolution and text size/weight/style-name tokens.
- Surface: window color and 12-step panel background scale.
- Semantic color: `primary`/`accent`, `success`, `warning`, `danger`.
- Accent palette: iconset-derived color token set (`accentPaletteTokens`).
- Metrics: spacing, radius, control size, dialog size, and interaction timings.

## Window and Panel Surfaces

Window base:
- `window: "#141414"`

Panel background scale (dark, low-saturation, 12 steps):
- `panelBackground01: "#1B1B1C"`
- `panelBackground02: "#1D1D1D"`
- `panelBackground03: "#1F1F20"`
- `panelBackground04: "#212223"`
- `panelBackground05: "#242525"`
- `panelBackground06: "#262728"`
- `panelBackground07: "#292A2B"`
- `panelBackground08: "#2C2E2F"`
- `panelBackground09: "#303232"`
- `panelBackground10: "#343536"`
- `panelBackground11: "#373A3B"`
- `panelBackground12: "#3C3E3F"`

Derived surface aliases:
- `windowAlt -> panelBackground03`
- `subSurface -> panelBackground04`
- `surfaceSolid -> panelBackground05`
- `surfaceAlt -> panelBackground06`
- `surfaceGhost -> panelBackground02`

## Hex Format Rule

- Opaque surface tokens use 6-digit hex.
- Alpha-required tokens remain 8-digit hex (for example `overlayBackdrop`, text opacity tokens).

## Icon Path Resolution

`Theme.iconPath(iconName)` resolves logical icon names into:
`qrc:/qt/qml/LVRS/resources/iconset/`

Rules:
- Empty input returns empty string.
- Full resource path (`:/`) is returned as-is.
- `.svg` is appended when omitted.

## Accent Palette Tokens

There are two layers:

- Stable semantic accent properties (`accentBlue`, `accentRed`, `accentGreen`, etc.).
- Extracted palette list: `accentPaletteTokens`.

`accentPaletteTokens` item schema:
- `{ name: string, color: string }`

Count is available as:
- `accentPaletteTokenCount`

The extracted palette is generated from `resources/iconset/*.svg` fill/stroke colors.

## Related UI Defaults

- Context menu colors:
  - `contextMenuSurface`
  - `contextMenuDivider`
  - `contextMenuItemSelectedBackground`
  - `contextMenuItemInactiveBackground`
- Dialog sizing:
  - `dialogMinWidth: 280`
  - `dialogMaxWidth: 360`
- Common radii:
  - `radiusSm: 4`
  - `radiusLg: 12`

## Usage

```qml
import LVRS 1.0 as LV

Rectangle {
    color: LV.Theme.window
}
```

## Token Extension Workflow

When adding new tokens, apply this order:

1. Add primitive token in `Theme.qml`.
2. Add semantic alias when the token has domain meaning (for example, warning surface).
3. Update component defaults to consume semantic aliases instead of hard-coded colors.
4. Update docs with value, purpose, and consumption target.

## Accessibility and Contrast Checks

For text/surface combinations, validate contrast manually and in tooling for:

- `title/header` text on `panelBackground*` surfaces,
- `description/caption` text on low-contrast surfaces,
- disabled states (`disabledColor`, `textOctonary`) under dim overlays.

Prefer documenting intentional low-contrast exceptions rather than silently keeping ambiguous palettes.

## Design-System Synchronization Tips

If tokens are synchronized with external design tools:

- keep canonical token names stable,
- treat rename as a breaking change,
- stage deprecations by keeping old alias tokens for at least one release cycle.
