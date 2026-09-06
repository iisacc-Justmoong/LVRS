# Theme

Location: `qml/Theme.qml`

`Theme` is the global design-token singleton for LVRS QML components.
All platforms use the same authored logical sizes for spacing, radius, control sizes, icons, typography, and line heights. iOS and Android add no automatic size multiplier. Body remains `13px / 13px`.

## Token Groups

- Typography: font family resolution and text size/weight/style-name tokens.
- Surface: window color and 12-step panel background scale.
- Semantic color: `primary`/`accent`, `success`, `warning`, `danger`.
- Accent palette: iconset-derived color token set (`accentPaletteTokens`).
- Metrics: spacing, radius, control size, dialog size, and interaction timings.

Compact icon baseline:

- `iconSm` resolves to `18 x 18` logical pixels on desktop and mobile.
- Stock action, menu, hierarchy, input, and selection-control icons consume this token unless a public size property is explicitly overridden.
- Icon images keep a square logical frame and `Image.PreserveAspectFit`, so non-square SVG artwork scales proportionally without distortion.

## Shared Desktop and Mobile Sizing

- `Theme` resolves the current runtime target through `Platform.runtimeProfile()`.
- `metricScaleFactor` and `typographyScaleFactor` are always `1.0`, including when the effective target is `ios` or `android`.
- `scaleMetric()` and `scaleTextMetric()` retain numeric conversion and rounding; `scaleRealMetric()` retains fractional values. None adds a platform multiplier.
- Text sizes and matching line heights are Title `26`, Title2 `22`, Header `17`, Header2 `15`, Body `13`, Description `12`, and Caption `11` on every target.
- `FontPolicy` recognizes only these authored sizes. Former `2x` and `1.25x` aliases are rejected; `22px` belongs to Bold Title2, and `26px` belongs to Bold Title, with no doubled Caption or Body aliases.
- `ApplicationWindow.mobileViewScale` and `Window.mobileViewScale` default to `1.0`; explicitly configured composition scaling remains available. Device-pixel-ratio handling and render-quality sampling remain separate from Theme sizing.

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

## Alert Glass

The updated Figma Alert uses dedicated color tokens. Its typography continues to
use the existing Title and Body tokens; these colors do not create text styles.

| Token | Value | Use |
| --- | --- | --- |
| `alertGlassTint` | `#1D1F21`, 72% opacity | Translucent card tint |
| `alertGlassEdge` | White, 20% opacity | Card edge |
| `alertTitleColor` | `#F4F5F7` | Title and neutral actions |
| `alertBodyColor` | `#D6D9DF` | Message |
| `alertActionPrimary` | `#027DFF` | Primary button |
| `alertActionBorder` | `#596168` | Secondary outline |
| `alertDivider` | `#363B3F` | Content/action separator |
| `alertIconSurface` | `#192840` | Icon frame fill |
| `alertIconBorder` | `#244B7E` | Icon frame edge |

Discard text uses the existing `danger` token (`#FF453A`). See
[Alert](components/surfaces/Alert.md) for material capture, layout, and fallback
behavior. The Alert's 500px preferred width is independent of the older shared
dialog bounds below.

## TextField Glass

Figma TextField `114:179` uses these semantic fills derived from the existing
`panelBackground10` RGB channels. Existing panel and Alert tokens are unchanged.

| Token | Alpha | Use |
| --- | --- | --- |
| `inputFieldGlassTint` | 64% | Rounded / Cylinder |
| `inputFieldGlassTintDisabled` | 36% | Disabled Rounded / Cylinder |
| `inputFieldGlassTintInline` | 16% | Inline, including disabled |
| `inputFieldGlassReflection` | 1.8% white | Shared material reflection |

Effect distances and color alphas use the same values on desktop and mobile.
See [InputField](components/control/InputField.md#textfield-material) for the
gradient, inset shadow, blur, capture source, and renderer fallback contracts.

## Hex Format Rule

- Opaque surface tokens use 6-digit hex.
- Alpha-required tokens use 8-digit hex (for example `overlayBackdrop`, text opacity
  tokens), or `Qt.rgba` to express exact Figma opacity percentages for material fills.

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
  - desktop and mobile: `dialogMinWidth: 280`, `dialogMaxWidth: 360`
- Common radii:
  - desktop and mobile: `radiusSm: 4`, `radiusLg: 12`

## Validation

`LVRSTests_platform_integration` checks the same authored component dimensions on Android and iOS. `LVRSTests_import_api` and `LVRSTests_list_composites` cover platform switching, controls, and all ListItem variants. `LVRSTests_font_policy` rejects obsolete scaled aliases. The installed consumer verifies shared sizes through `find_package(LVRS)` and the installed QML module.

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
