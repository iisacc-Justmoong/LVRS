# Theme

Location: `qml/Theme.qml`

`Theme` is the global design-token singleton for LVRS QML components.
It now applies a built-in mobile `2x` token profile, so spacing, radius, control sizes, and scalable typography values resolve to doubled mobile numbers on iOS and Android without relying on root-layer composition scaling. Body typography remains a fixed `13px` size and line height across targets.

## Token Groups

- Typography: font family resolution and text size/weight/style-name tokens.
- Surface: window color and 12-step panel background scale.
- Semantic color: `primary`/`accent`, `success`, `warning`, `danger`.
- Accent palette: iconset-derived color token set (`accentPaletteTokens`).
- Metrics: spacing, radius, control size, dialog size, and interaction timings.

Compact icon baseline:

- `iconSm` resolves to `18 x 18` logical pixels on desktop.
- The mobile token profile doubles that baseline to `36 x 36`.
- Stock action, menu, hierarchy, input, and selection-control icons consume this token unless a public size property is explicitly overridden.
- Icon images keep a square logical frame and `Image.PreserveAspectFit`, so non-square SVG artwork scales proportionally without distortion.

## Mobile `2x` Scaling

- `Theme` resolves the current runtime target through `Platform.runtimeProfile()`.
- When the effective target is mobile (`ios` or `android`), numeric UI tokens resolve at `2x`.
- This includes spacing, radius, stroke widths, control sizes, dialog bounds, icon sizes, and scalable text pixel sizes / line heights; `textBody` and `textBodyLineHeight` remain fixed at `13`.
- `FontPolicy` treats the `13px` Medium Body token as non-scalable. In particular, `16px` and `26px` are not Body aliases, so they remain available to their own typography contracts without a Body-token collision.
- `FontPolicy` accepts the desktop `1x` and mobile `2x` typography sizes; obsolete `1.25x`-only sizes are not treated as current Theme tokens.
- `ApplicationWindow.mobileViewScale` remains `1.0` by default; LVRS now prefers token-level `2x` sizing over scaled composition for the stock mobile path.

Preview/test helpers:

- `targetOverride: string`
- `effectiveTarget: string`
- `mobileTarget: bool`
- `metricScaleFactor: real`
- `typographyScaleFactor: real`
- `scaleMetric(value)`
- `scaleRealMetric(value)`
- `scaleTextMetric(value)`

## Window and Panel Surfaces

Window base:
- `window: "#141414"`

Panel background scale (dark, low-saturation, 12 steps):
- `panelBackground01: "#0C0C0D"`
- `panelBackground02: "#0E0E0E"`
- `panelBackground03: "#151516"`
- `panelBackground04: "#191919"`
- `panelBackground05: "#1D1E1E"`
- `panelBackground06: "#1F2021"`
- `panelBackground07: "#242424"`
- `panelBackground08: "#232424"`
- `panelBackground09: "#262727"`
- `panelBackground10: "#282828"`
- `panelBackground11: "#2D2E2F"`
- `panelBackground12: "#313233"`

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
- Legacy logical aliases are normalized to the shipped icon filenames (for example `projectStructure -> generalprojectStructure`, `add -> generaladd`, `viewMoreSymbolicDefault -> generalmoreHorizontal`, `panDownSymbolic* -> generalchevronDown*`).
- Group-style names are flattened before lookup (for example `general/projectStructure -> generalprojectStructure.svg`).

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
  - `contextMenuSurface: panelBackground03`
  - `contextMenuDivider: panelBackground08`
  - `contextMenuItemSelectedBackground`
  - `contextMenuItemInactiveBackground`
- Dialog sizing:
  - desktop: `dialogMinWidth: 280`, `dialogMaxWidth: 360`
  - mobile: `dialogMinWidth: 560`, `dialogMaxWidth: 720`
- Common radii:
  - desktop: `radiusSm: 4`, `radiusLg: 12`
  - mobile: `radiusSm: 8`, `radiusLg: 24`

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
