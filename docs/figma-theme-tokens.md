# Figma theme token contract

This document records the Figma-first color and typography contract for the LVRS
GUI theme. The authoritative captured data is
[`tests/fixtures/figma-theme-contract.json`](../tests/fixtures/figma-theme-contract.json),
from the [LVRS Figma file](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS).
The 2026-10-19 accent revision uses the ten user-selected swatches on the Color page as its color authority. The fixture retains **77 variables: 41 colors and 36 typography variables**, plus **seven text styles**. All 41 color variables were read back after the update. Existing variable IDs and names, typography and style bindings are retained. Component bounds are retained; ColorPicker recent-color cells share the same row width across ten swatches. The authored RGBA values and source-node mapping are also recorded in [`accent-palette.json`](../tests/fixtures/accent-palette.json).

The source mapping is `src/qml/Theme.qml`. Hex values below are rounded display values; the canonical QML properties use original floating-point `Qt.rgba` values. Tests compare those channels at QColor precision. Building and testing this checkout is separate from updating an installed SDK. The user performs `lvrs install` for this revision.

## Scope and naming

- `Theme Colors` contains 13 panel/window colors, six text colors, four action
  colors and 18 named accent properties: ten canonical colors, seven compatibility
  aliases and one neutral Gray. Names express roles and color names.
- `Theme Typography` contains a shared family and seven five-property recipes.
- Existing IDs are retained when an existing variable or style is renamed. Old
  QML panel and typography property names remain compatibility aliases.
- Existing control-specific fills, pressed/selected state overrides, materials,
  icon-derived colors and platform-specific tokens are outside the core inventory.
  Their absence from the core set does not authorize deleting them. Existing
  paints that used the replaced accents, including their opacity stops and
  composites, follow the new colors while retaining their state behavior.
- The collection metadata slot `codeSyntax.WEB` intentionally contains **QML**
  `Theme.<property>` references. It is not CSS syntax or a web-only implementation.

| Collection | ID | Active core inventory |
| --- | --- | ---: |
| Theme Colors | `VariableCollectionId:44:457` | 41; 33 in the initial snapshot |
| Theme Typography | `VariableCollectionId:1305:73` | 36 |
| Compatibility Icon Colors | `VariableCollectionId:963:2` | 0; 1,548 retained hidden variables |

Colors use the captured `Dark` mode. Typography uses its captured mode; the
fixture records every mode ID. Variable scopes are compared as sets, not by
array ordering. Each variable has `codeSyntax.WEB: "Theme.<property>"`.

## Panel roles and exact values

These normalized RGB channels are copied from actual Figma `resolved` values.
Every panel alpha is `1`. Preserve the floating values rather than replacing
them with rounded hexadecimal colors. The order is a role inventory, not a
monotonic brightness ramp; do not recalculate steps 07 and 08 to match their names.

| Figma variable | QML property | Normalized RGB | Retained ID |
| --- | --- | --- | --- |
| `Panel/canvas` | `Theme.canvas` | `(0.04892867058515549, 0.04892867058515549, 0.05038686841726303)` | `VariableID:44:467` |
| `Panel/surfaceGhost` | `Theme.surfaceGhost` | `(0.05507083609700203, 0.05517596751451492, 0.05570162087678909)` | `VariableID:44:468` |
| `Panel/windowAlt` | `Theme.windowAlt` | `(0.08394163101911545, 0.08394163101911545, 0.08631309866905212)` | `VariableID:44:469` |
| `Panel/subSurface` | `Theme.subSurface` | `(0.09516431391239166, 0.09655113518238068, 0.09793794900178909)` | `VariableID:44:470` |
| `Panel/surfaceSolid` | `Theme.surfaceSolid` | `(0.11397916078567505, 0.11664287745952606, 0.11664287745952606)` | `VariableID:215:9618` |
| `Panel/surfaceAlt` | `Theme.surfaceAlt` | `(0.12302529811859131, 0.12528030574321747, 0.12753531336784363)` | `VariableID:215:9620` |
| `Panel/surfaceInteractive` | `Theme.surfaceInteractive` | `(0.1411764770746231, 0.1411764770746231, 0.1411764770746231)` | `VariableID:215:9621` |
| `Panel/surfacePanel` | `Theme.surfacePanel` | `(0.13709451258182526, 0.14029954373836517, 0.14190204441547394)` | `VariableID:215:9622` |
| `Panel/surfaceControl` | `Theme.surfaceControl` | `(0.14820532500743866, 0.15256911516189575, 0.15256911516189575)` | `VariableID:215:9623` |
| `Panel/surfaceInput` | `Theme.surfaceInput` | `(0.1555851548910141, 0.15707255899906158, 0.15855994820594788)` | `VariableID:215:9624` |
| `Panel/surfaceInputHover` | `Theme.surfaceInputHover` | `(0.17537428438663483, 0.1819191724061966, 0.18410080671310425)` | `VariableID:215:9625` |
| `Panel/surfaceRaised` | `Theme.surfaceRaised` | `(0.19355684518814087, 0.1978072077035904, 0.19993239641189575)` | `VariableID:215:9626` |
| `Panel/window` | `Theme.window` | `(0.0784313753247261, 0.0784313753247261, 0.0784313753247261)` | `VariableID:1305:74` |

The twelve legacy properties alias the corresponding role without creating a
second color authority. `window` remains independent of the twelve legacy steps.

| Legacy QML property | Role |
| --- | --- |
| `panelBackground01` | `canvas` |
| `panelBackground02` | `surfaceGhost` |
| `panelBackground03` | `windowAlt` |
| `panelBackground04` | `subSurface` |
| `panelBackground05` | `surfaceSolid` |
| `panelBackground06` | `surfaceAlt` |
| `panelBackground07` | `surfaceInteractive` |
| `panelBackground08` | `surfacePanel` |
| `panelBackground09` | `surfaceControl` |
| `panelBackground10` | `surfaceInput` |
| `panelBackground11` | `surfaceInputHover` |
| `panelBackground12` | `surfaceRaised` |

## Text and action colors

Text colors use white RGB `(1,1,1)` with the actual alpha values below. The base
is fully opaque. They describe text opacity independently from a text-size recipe.

| Figma variable | QML property | Normalized alpha | Retained ID |
| --- | --- | --- | --- |
| `Text/titleHeaderColor` | `Theme.titleHeaderColor` | `0.8999999761581421` | `VariableID:44:458` |
| `Text/bodyColor` | `Theme.bodyColor` | `0.800000011920929` | `VariableID:44:459` |
| `Text/descriptionColor` | `Theme.descriptionColor` | `0.6000000238418579` | `VariableID:44:460` |
| `Text/captionColor` | `Theme.captionColor` | `0.5` | `VariableID:44:461` |
| `Text/disabledColor` | `Theme.disabledColor` | `0.30000001192092896` | `VariableID:44:462` |
| `Text/textTokenBase` | `Theme.textTokenBase` | `1` | `VariableID:1305:75` |

| Figma variable | QML property | Canonical alias | Retained ID |
| --- | --- | --- | --- |
| `Action/primary` | `Theme.primary` | `Accent/accentBlue` | `VariableID:44:463` |
| `Action/success` | `Theme.success` | `Accent/accentGreen` | `VariableID:44:464` |
| `Action/warning` | `Theme.warning` | `Accent/accentYellow` | `VariableID:44:465` |
| `Action/danger` | `Theme.danger` | `Accent/accentRed` | `VariableID:44:466` |

`Action/primary` records the default Blue. `ApplicationWindow.primaryColor` still configures the mutable `Theme.primaryColor`; `Theme.primary` and `Theme.accent` expose that app value. Primary overlays and 25%/16%/36% surface composites follow it, including custom alpha. Status colors and canonical accent properties remain fixed when app branding changes.

<a id="ten-original-accent-colors"></a>

## Ten authored accent colors

The ten swatches supplied by the user replace the previous canonical palette in place. Existing variable IDs and public property names are retained. Every alpha is 1. The exact floating-point oracle is `authoredAccentPalette.colors` in the fixture; HEX is for display and static SVG paints.

| Figma variable / QML property | HEX display | Source node | Retained variable ID |
| --- | --- | --- | --- |
| `Accent/accentBlue` / `Theme.accentBlue` | `#1582FF` | `1477:30` | `VariableID:1305:76` |
| `Accent/accentRed` / `Theme.accentRed` | `#FF4646` | `1477:31` | `VariableID:1305:77` |
| `Accent/accentGreen` / `Theme.accentGreen` | `#66FF48` | `1477:32` | `VariableID:1305:78` |
| `Accent/accentYellow` / `Theme.accentYellow` | `#FFD427` | `1477:33` | `VariableID:1305:79` |
| `Accent/accentPurple` / `Theme.accentPurple` | `#874AFF` | `1477:34` | `VariableID:1305:80` |
| `Accent/accentRose` / `Theme.accentRose` | `#F94471` | `1477:36` | `VariableID:1305:81` |
| `Accent/accentOrange` / `Theme.accentOrange` | `#FF7A27` | `1477:35` | `VariableID:1305:82` |
| `Accent/accentBrown` / `Theme.accentBrown` | `#432318` | `1477:37` | `VariableID:1305:83` |
| `Accent/accentCyan` / `Theme.accentCyan` | `#21D3FF` | `1477:38` | `VariableID:1305:84` |
| `Accent/accentTeal` / `Theme.accentTeal` | `#00C7DD` | `1477:40` | `VariableID:1305:85` |

`accentOrangeMuted` and `accentBrownMuted` retain their public names as aliases of the corresponding canonical colors. `accentBlueMuted` is the new Blue composited at 25% over the interactive surface. The **386** extracted `accentPaletteTokens` entries retain their names, order and `{ name: string, color: string }` schema. Their matching accent and composite strings are recolored; unrelated artwork colors remain. `accentPaletteTokenCount` remains `386`.

<a id="eight-attached-palette-colors"></a>

## Compatibility accent aliases

The former independently sampled palette is superseded by the authored ten-color system. Its saved IDs and editable circles remain available. Seven properties alias canonical colors in Figma and QML; Gray stays neutral.

| Compatibility property | Canonical property / value | Retained ID |
| --- | --- | --- |
| `accentPaletteBlue` | `accentBlue` | `VariableID:1325:2` |
| `accentPalettePurple` | `accentPurple` | `VariableID:1325:3` |
| `accentPalettePink` | `accentRose` | `VariableID:1325:4` |
| `accentPaletteRed` | `accentRed` | `VariableID:1325:5` |
| `accentPaletteOrange` | `accentOrange` | `VariableID:1325:6` |
| `accentPaletteYellow` | `accentYellow` | `VariableID:1325:7` |
| `accentPaletteGreen` | `accentGreen` | `VariableID:1325:8` |
| `accentPaletteGray` | neutral `#8C8C8C` | `VariableID:1325:9` |

Purple and Rose remain distinct colors; the old Pink name points to Rose. `defaultPrimary`, `accentBlue` and `accentPaletteBlue` now share the authored Blue. App Primary overrides do not mutate these fixed colors. No numbered shade scale is introduced.

## Typography variables and recipes

The shared `Font/fontBody` STRING variable is `Pretendard` and maps to
`Theme.fontBody`. Its ID is `VariableID:1305:86`. QML retains
`FontPolicy.resolveFamily(FontPolicy.preferredFamily)`, so a runtime fallback does
not change the preferred Figma family contract. Pixel sizes, line heights and
letter spacing are authored logical pixels with no mobile multiplier.

| Text style | QML recipe prefix | Size / line height | Weight | Style name | Letter spacing |
| --- | --- | --- | --- | --- | --- |
| Title | `textTitle` | 26 / 26 | 700 | Bold | 0 |
| Subtitle | `textSubtitle` | 22 / 22 | 700 | Bold | 0 |
| Header | `textHeader` | 17 / 17 | 600 | SemiBold | 0 |
| Subheader | `textSubheader` | 15 / 15 | 600 | SemiBold | 0 |
| Body | `textBody` | 13 / 13 | 500 | Medium | 0 |
| Description | `textDescription` | 12 / 12 | 600 | SemiBold | 0 |
| Caption | `textCaption` | 11 / 11 | 400 | Regular | 0 |

Each recipe exposes its size plus `Weight`, `StyleName`, `LineHeight` and
`LetterSpacing`. Seven recipes × five variables, plus one shared family,
produce the 36-variable typography inventory. The complete variable mapping is:

| Figma variable | Type | QML property | Value | ID |
| --- | --- | --- | --- | --- |
| `Font/fontBody` | STRING | `Theme.fontBody` | `Pretendard` | `VariableID:1305:86` |
| `Title/textTitle` | FLOAT | `Theme.textTitle` | `26` | `VariableID:1305:87` |
| `Title/textTitleLineHeight` | FLOAT | `Theme.textTitleLineHeight` | `26` | `VariableID:1305:88` |
| `Title/textTitleStyleName` | STRING | `Theme.textTitleStyleName` | `Bold` | `VariableID:1305:89` |
| `Title/textTitleWeight` | FLOAT | `Theme.textTitleWeight` | `700` | `VariableID:1305:90` |
| `Title/textTitleLetterSpacing` | FLOAT | `Theme.textTitleLetterSpacing` | `0` | `VariableID:1305:91` |
| `Subtitle/textSubtitle` | FLOAT | `Theme.textSubtitle` | `22` | `VariableID:1305:92` |
| `Subtitle/textSubtitleLineHeight` | FLOAT | `Theme.textSubtitleLineHeight` | `22` | `VariableID:1305:93` |
| `Subtitle/textSubtitleStyleName` | STRING | `Theme.textSubtitleStyleName` | `Bold` | `VariableID:1305:94` |
| `Subtitle/textSubtitleWeight` | FLOAT | `Theme.textSubtitleWeight` | `700` | `VariableID:1305:95` |
| `Subtitle/textSubtitleLetterSpacing` | FLOAT | `Theme.textSubtitleLetterSpacing` | `0` | `VariableID:1305:96` |
| `Header/textHeader` | FLOAT | `Theme.textHeader` | `17` | `VariableID:1305:97` |
| `Header/textHeaderLineHeight` | FLOAT | `Theme.textHeaderLineHeight` | `17` | `VariableID:1305:98` |
| `Header/textHeaderStyleName` | STRING | `Theme.textHeaderStyleName` | `SemiBold` | `VariableID:1305:99` |
| `Header/textHeaderWeight` | FLOAT | `Theme.textHeaderWeight` | `600` | `VariableID:1305:100` |
| `Header/textHeaderLetterSpacing` | FLOAT | `Theme.textHeaderLetterSpacing` | `0` | `VariableID:1305:101` |
| `Subheader/textSubheader` | FLOAT | `Theme.textSubheader` | `15` | `VariableID:1305:102` |
| `Subheader/textSubheaderLineHeight` | FLOAT | `Theme.textSubheaderLineHeight` | `15` | `VariableID:1305:103` |
| `Subheader/textSubheaderStyleName` | STRING | `Theme.textSubheaderStyleName` | `SemiBold` | `VariableID:1305:104` |
| `Subheader/textSubheaderWeight` | FLOAT | `Theme.textSubheaderWeight` | `600` | `VariableID:1305:105` |
| `Subheader/textSubheaderLetterSpacing` | FLOAT | `Theme.textSubheaderLetterSpacing` | `0` | `VariableID:1305:106` |
| `Body/textBody` | FLOAT | `Theme.textBody` | `13` | `VariableID:1305:107` |
| `Body/textBodyLineHeight` | FLOAT | `Theme.textBodyLineHeight` | `13` | `VariableID:1305:108` |
| `Body/textBodyStyleName` | STRING | `Theme.textBodyStyleName` | `Medium` | `VariableID:1305:109` |
| `Body/textBodyWeight` | FLOAT | `Theme.textBodyWeight` | `500` | `VariableID:1305:110` |
| `Body/textBodyLetterSpacing` | FLOAT | `Theme.textBodyLetterSpacing` | `0` | `VariableID:1305:111` |
| `Description/textDescription` | FLOAT | `Theme.textDescription` | `12` | `VariableID:1305:112` |
| `Description/textDescriptionLineHeight` | FLOAT | `Theme.textDescriptionLineHeight` | `12` | `VariableID:1305:113` |
| `Description/textDescriptionStyleName` | STRING | `Theme.textDescriptionStyleName` | `SemiBold` | `VariableID:1305:114` |
| `Description/textDescriptionWeight` | FLOAT | `Theme.textDescriptionWeight` | `600` | `VariableID:1305:115` |
| `Description/textDescriptionLetterSpacing` | FLOAT | `Theme.textDescriptionLetterSpacing` | `0` | `VariableID:1305:116` |
| `Caption/textCaption` | FLOAT | `Theme.textCaption` | `11` | `VariableID:1305:117` |
| `Caption/textCaptionLineHeight` | FLOAT | `Theme.textCaptionLineHeight` | `11` | `VariableID:1305:118` |
| `Caption/textCaptionStyleName` | STRING | `Theme.textCaptionStyleName` | `Regular` | `VariableID:1305:119` |
| `Caption/textCaptionWeight` | FLOAT | `Theme.textCaptionWeight` | `400` | `VariableID:1305:120` |
| `Caption/textCaptionLetterSpacing` | FLOAT | `Theme.textCaptionLetterSpacing` | `0` | `VariableID:1305:121` |

### Actual style bindings

Every saved text style has exactly five bound fields:

| Style field | Variable |
| --- | --- |
| `fontFamily` | Shared `Font/fontBody` |
| `fontSize` | Recipe size property |
| `fontWeight` | Recipe `Weight` property |
| `lineHeight` | Recipe `LineHeight` property; unit `PIXELS` |
| `letterSpacing` | Recipe `LetterSpacing` property; unit `PIXELS` |

The observed Figma behavior is that `fontWeight` binding selects the static
font style and replaces a `fontStyle` binding. The seven `StyleName` STRING
variables record the matching QML recipe; they are not a sixth active style
binding. Do not report simultaneous `fontStyle` and `fontWeight` bindings.
Existing text-style IDs are retained:

| Style | ID |
| --- | --- |
| Title | `S:9be61cca334d9c7ad10f6748611727c4fc6340a5,` |
| Subtitle | `S:3e4074fd4df418ade1039a2b0184720c6c2ffbe5,` |
| Header | `S:62f4d4aa9883bcf345b9273542940cb7e312a6aa,` |
| Subheader | `S:1bb19003757942c30f3fafada1b8400cab71cf0e,` |
| Body | `S:cc3cf61d5106b1d70f4ce271776b7f93fc3ae046,` |
| Description | `S:85e5273ae601efad5b2a9d337177cc9d9762eae1,` |
| Caption | `S:1fad39b1005ddd0b4ed008f63820dcd864b00ba4,` |

### Typography compatibility

`textTitle2` aliases `textSubtitle`; `textHeader2` aliases `textSubheader`.
Their `Weight`, `StyleName`, `LineHeight` and `LetterSpacing` companions follow
exactly the same mapping, preserving all ten old public properties. Existing
Label `title2`/`header2` enum names and unrelated consumer files are unchanged.
`textDisabled` and its four companions continue to use Caption metrics; Disabled
has a separate text color, not an eighth size recipe.

## Hidden icon-color compatibility

`Compatibility Icon Colors` retains **1,548** existing variables. The collection
and every variable are hidden from publishing; every variable has an empty
scope set. This removes the extracted palette from normal token selection
without deleting IDs or severing existing paint bindings. Hiding is not removal.
Older icon-specific values and gradient-stop variables remain available to their
existing consumers. The captured selected original-color aliases are:

| Compatibility variable | Retained ID | Core target ID |
| --- | --- | --- |
| `Primitive/defaultPrimary` | `VariableID:964:2` | `VariableID:44:463` |
| `Primitive/accentWhite` | `VariableID:964:10` | `VariableID:1305:75` |
| `Primitive/accentBlue` | `VariableID:964:18` | `VariableID:1305:76` |
| `Primitive/accentRed` | `VariableID:964:22` | `VariableID:1305:77` |
| `Primitive/accentGreen` | `VariableID:964:30` | `VariableID:1305:78` |
| `Primitive/accentOrangeMuted` | `VariableID:964:38` | `VariableID:1305:82` |
| `Primitive/accentYellow` | `VariableID:964:46` | `VariableID:1305:79` |
| `Primitive/accentBrownMuted` | `VariableID:964:62` | `VariableID:1305:83` |
| `Primitive/accentPurple` | `VariableID:964:66` | `VariableID:1305:80` |
| `Primitive/accentRose` | `VariableID:964:94` | `VariableID:1305:81` |
| `Primitive/accentDimTeal` | `VariableID:964:186` | `VariableID:1305:85` |
| `Primitive/accentLightCyan` | `VariableID:964:222` | `VariableID:1305:84` |

## Accent revision verification — 2026-10-19

The Figma update preserves all ten source-node bounds and binds them to the existing canonical variables. It updates matching variables, unbound component paints, gradient stops, shared paint styles and reference labels across the file. The ColorPicker current-color variable is an alias of canonical Purple. Its six master views and six existing instances expose all ten colors inside their existing row bounds. Both the design reference and source examples use the new system.

Source validation covers:

- Original RGBA precision, ten source-node IDs and seven fixed compatibility aliases.
- All 77 saved typed Theme properties and retained typography/style contracts.
- Rendered pixel samples for all ten canonical swatches.
- Default and custom app Primary propagation, controls, overlays, and ten catalog choices.
- SVG resource hashes, unchanged non-paint geometry, raster alpha silhouettes, and legacy resource aliases.
- ColorPicker defaults and recent colors from the same canonical palette.

The audit files are in `build/accent-palette-20261019/`: `figma-readback.json`, `figma-variable-update.json`, `figma-page-updates.json`, `figma-extra-page-updates.json`, `source-icon-updates.json`, build and test logs. The current runtime swatch capture is `build/accent-palette-preview.png`.

The original 2026-10-08/09 69-variable and eight-color extension records remain historical snapshots under `build/theme-token-refresh-20261008/` and `build/accent-palette-20261009/`. Their former separate palette values and earlier pass counts do not describe this revision. Installation, consumer rebuilds and library publication have separate evidence; no installation is performed by this task.

The final macOS source build succeeded. A fresh focused CTest run passed **12/12 targets** with **0 failures** in **64.29 seconds** after the final helper-resource update. The JUnit report is [`final-ctest.xml`](../build/accent-palette-20261019/final-ctest.xml), with a machine-readable summary in [`verification.json`](../build/accent-palette-20261019/verification.json). Six conditional cases for native RHI, hover delivery and optional captures remain skipped and are recorded in `final-qt-tests.log`; this run does not claim all-platform native execution or installation.
