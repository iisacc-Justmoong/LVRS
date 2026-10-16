# Figma theme token contract

This document records the Figma-first color and typography contract for the LVRS
GUI theme. The authoritative captured data is
[`tests/fixtures/figma-theme-contract.json`](../tests/fixtures/figma-theme-contract.json),
from the [LVRS Figma file](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS).
The initial snapshot was captured at `2026-10-08 15:05:38 UTC` and contained
**69 core variables: 33 colors and 36 typography variables**, plus **seven text
styles**. The 2026-10-09 attached-palette extension adds eight named, opaque
colors. The current fixture was captured at `2026-10-09 00:35:00 UTC` and
contains **77 variables: 41 colors and the same 36 typography variables**. The
saved readback passes validation and preserves every original variable and text
style. The eight source properties are registered; the full source build and
all 56 CTest targets passed. The initial 69-variable evidence is retained below
separately from the current 77-variable results.

The source mapping is `src/qml/Theme.qml`. The fixture records the saved design;
source build and test evidence is recorded separately below. On 2026-10-09 the
initial full source build succeeded and all 56 CTest targets passed. The 133
initial Figma contract rows passed. Installed consumers, Dreamscapes/Society application runs
and Figma library publication were not verified. No non-GUI SDK dependency is
changed by this contract.

## Scope and naming

- `Theme Colors` contains 13 panel/window colors, six text colors, four action
  colors and 18 accent colors after the attached-palette extension: ten original
  accents and eight image-sampled palette colors. Names express roles and color
  names rather than a new numbered color scale.
- `Theme Typography` contains a shared family and seven five-property recipes.
- Existing IDs are retained when an existing variable or style is renamed. Old
  QML panel and typography property names remain compatibility aliases.
- Existing control-specific fills, pressed/selected state overrides, materials,
  icon-derived colors and platform-specific tokens are outside the core inventory.
  Their absence from the core set does not authorize deleting them.
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

| Figma variable | QML property | Normalized RGBA | Retained ID |
| --- | --- | --- | --- |
| `Action/primary` | `Theme.primary` | `(0.03921568766236305, 0.5176470875740051, 1, 1)` | `VariableID:44:463` |
| `Action/success` | `Theme.success` | `(0.19607843458652496, 0.843137264251709, 0.29411765933036804, 1)` | `VariableID:44:464` |
| `Action/warning` | `Theme.warning` | `(1, 0.8392156958580017, 0.03921568766236305, 1)` | `VariableID:44:465` |
| `Action/danger` | `Theme.danger` | `(1, 0.2705882489681244, 0.22745098173618317, 1)` | `VariableID:44:466` |

`Action/primary` records the default app primary `#0A84FF`. At runtime,
`ApplicationWindow.primaryColor` configures the mutable `Theme.primaryColor`.
`Theme.primary` and `Theme.accent` expose that app value. Derived action tints
follow the existing primary-color behavior. This does not replace the fixed
original accent blue, whose value is `#548AF7`.

## Ten original accent colors

The following ten original colors remain part of the visible Figma `Accent` core
and map to named QML `color` properties. Hexadecimal text is a readable 8-bit presentation;
the fixture retains the actual floating RGBA oracle. This core inventory is
separate from the existing extracted icon-color list.

| Figma variable | QML property | Hex display | Retained ID |
| --- | --- | --- | --- |
| `Accent/accentBlue` | `Theme.accentBlue` | `#548AF7` | `VariableID:1305:76` |
| `Accent/accentRed` | `Theme.accentRed` | `#DB5C5C` | `VariableID:1305:77` |
| `Accent/accentGreen` | `Theme.accentGreen` | `#57965C` | `VariableID:1305:78` |
| `Accent/accentYellow` | `Theme.accentYellow` | `#F2C55C` | `VariableID:1305:79` |
| `Accent/accentPurple` | `Theme.accentPurple` | `#A571E6` | `VariableID:1305:80` |
| `Accent/accentRose` | `Theme.accentRose` | `#E55765` | `VariableID:1305:81` |
| `Accent/accentOrange` | `Theme.accentOrange` | `#C77D55` | `VariableID:1305:82` |
| `Accent/accentBrown` | `Theme.accentBrown` | `#45322B` | `VariableID:1305:83` |
| `Accent/accentCyan` | `Theme.accentCyan` | `#40B6E0` | `VariableID:1305:84` |
| `Accent/accentTeal` | `Theme.accentTeal` | `#24A394` | `VariableID:1305:85` |

`accentOrange` and `accentBrown` expose the existing values of
`accentOrangeMuted` and `accentBrownMuted`; the old public properties remain.
`accentCyan` and `accentTeal` register existing original palette colors.
The existing `accentPaletteTokens` array retains all **386** extracted entries,
their order and the `{ name: string, color: string }` schema. Its `color` values
remain `#HEX` strings; `accentPaletteTokenCount` remains `386`. The named
`Theme.accent*` properties are QML colors, which are distinct from those strings.
Do not shrink the compatibility array to the core accents or append the new
palette colors to it. No additional abstract list is needed for eight named
properties.
No `50..900` numeric palette is introduced.

## Eight attached palette colors

The 2026-10-09 user-attached image specifies eight additional accent colors in
this left-to-right order. The editable
[palette reference](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS?node-id=1325-10&m=dev)
contains eight 48-pixel circles, each bound to its corresponding saved variable. The 590 × 78 PNG declares sRGB metadata. Each swatch
was sampled in a central 21 × 21 region; all **441 pixels** in every region
matched exactly, and every sampled alpha is **255/255**. This avoids sampling
the antialiased circle boundary or the dark background. The sampled 8-bit values
are the source for these new opaque colors; existing floating panel and text
values keep their original precision.

| Figma variable | QML property | Exact sRGB hex | Saved ID |
| --- | --- | --- | --- |
| `Accent/accentPaletteBlue` | `Theme.accentPaletteBlue` | `#007AFF` | `VariableID:1325:2` |
| `Accent/accentPalettePurple` | `Theme.accentPalettePurple` | `#A550A7` | `VariableID:1325:3` |
| `Accent/accentPalettePink` | `Theme.accentPalettePink` | `#F74F9E` | `VariableID:1325:4` |
| `Accent/accentPaletteRed` | `Theme.accentPaletteRed` | `#FF5257` | `VariableID:1325:5` |
| `Accent/accentPaletteOrange` | `Theme.accentPaletteOrange` | `#F7821B` | `VariableID:1325:6` |
| `Accent/accentPaletteYellow` | `Theme.accentPaletteYellow` | `#FFC600` | `VariableID:1325:7` |
| `Accent/accentPaletteGreen` | `Theme.accentPaletteGreen` | `#62BA46` | `VariableID:1325:8` |
| `Accent/accentPaletteGray` | `Theme.accentPaletteGray` | `#8C8C8C` | `VariableID:1325:9` |

The `accentPalette<Color>` name identifies this supplied eight-color palette.
These properties are fixed QML `color` values, not intensity levels, generated
shades or dynamic app-primary aliases. In particular, `accentPaletteBlue`
(`#007AFF`), original `accentBlue` (`#548AF7`) and default action primary
(`#0A84FF`) retain separate values. Purple and Pink are separate named colors;
Gray is a selectable accent in this palette. The existing 386-entry
`accentPaletteTokens` icon snapshot and all original ten properties remain
unchanged.

The extraction record is
[`build/accent-palette-20261009/image-colors.json`](../build/accent-palette-20261009/image-colors.json).
It records the image hash, dimensions, sRGB metadata, each sample center and
region, all eight RGBA byte values and the uniform-pixel counts. The source PNG
SHA256 is
`cb3939c530f047ab1354972d7dbf5277ade1b9bea3d3762e7ac24a05f35c210d`.

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

## Initial 69-variable source, test and runtime evidence

The original 69-variable readback is preserved in
[`build/theme-token-refresh-20261008/figma-theme-readback.json`](../build/theme-token-refresh-20261008/figma-theme-readback.json).
The active test fixture can advance to the 77-variable extension without
relabeling the earlier snapshot or its test results.

The initial plan and read-only source inventory are stored in
`build/theme-token-refresh-20261008/token-plan.json`, `audit-inventory.json` and
`figma-before.json`. The plan describes intended names; the actual fixture is
authoritative for saved values, IDs, scopes and bindings. Scope ordering is not
meaningful. Float comparisons may allow only the documented Figma serialization
precision; they must not introduce 8-bit quantization or change authored values.

The initial Figma contract had `validation.passed: true` with no errors. That
validation was scoped to the captured 69-variable design contract. Its
`LVRSTests_figma_parity` extensions separately covered:

- `saved_theme_tokens`: 69 typed actual values and corresponding QML properties.
- `legacy_theme_aliases`: 12 panel aliases and ten Title2/Header2 companions.
- `saved_text_style_bindings`: seven styles × five exact variable-ID, property,
  resolved-value and runtime comparisons.
- `saved_text_style_font_recipes`: seven preferred families, style names and Qt
  weights, with matching bundled font-face availability.

The original fixture and earlier tests remain. These tests are distinct from
the compatibility palette's unchanged 386-entry string contract. All **133**
added rows passed: **69 + 22 + 35 + 7**. The complete `LVRSTests_figma_parity` Qt
result is **144 passed, 0 failed, 1 skipped**. Its existing
`material_inner_highlight` test requires a native RHI scene graph and was skipped
in the offscreen run. This is not a native-rendering pass.

The parent task executed these commands from the source checkout, using the
repository's fixed `build/` directory:

```sh
cmake --build build -j4
ctest --test-dir build --output-on-failure -j2 --output-junit build/theme-token-refresh-20261008/full-ctest.xml
```

Both returned exit status `0`. The full build completed; CTest reports **56/56
targets passed**, zero failed targets and **82.23 seconds** real time. The full
Qt test log contains **11 conditional skipped cases**, including the parity RHI
case, native screenshot/capture checks, offscreen hover delivery and Cocoa/RHI
checks. Passing CTest targets does not mean every conditional native case ran.

| Evidence | Collected artifact |
| --- | --- |
| Full build | [build.log](../build/theme-token-refresh-20261008/build.log) |
| Full CTest | [full-ctest.log](../build/theme-token-refresh-20261008/full-ctest.log) |
| JUnit XML | [full-ctest.xml](../build/theme-token-refresh-20261008/full-ctest.xml) |
| Figma parity details | [canonical_green.log](../build/theme-token-refresh-20261008/canonical_green.log) |
| Preserved full Qt case output | [full-qt-tests.log](../build/theme-token-refresh-20261008/full-qt-tests.log) |

CTest interpreted the `--output-junit` relative path after entering `build/`,
initially producing `build/build/theme-token-refresh-20261008/full-ctest.xml`.
The parent moved that XML into `build/theme-token-refresh-20261008/` after the
run to collect the evidence. The recorded invocation above is the actual command.

Updating `~/.local`, testing installed consumers, launching/restarting consumer
applications and verifying Figma library publication were not performed. The
successful source build and full CTest run do not establish those outcomes.

LVRS is already the GUI/QML layer. This refresh does not add Qt to a non-GUI
leaf SDK, modify image-processing modules or migrate another application. A
passing source test or Figma contract is insufficient evidence for an installed
LVRS package or a running Dreamscapes/Society session.

Initial 69-variable fixture SHA256:
`663cf9fca8bfb7ee6544866df45f7ddc9b152de7a2c67ed8bb81511b6ff51514`.

## Attached-palette extension evidence

The extension follows the same Figma-first sequence: add the eight named
variables in the existing `Theme Colors` collection, save and read their actual
IDs/values/scopes/code syntax, update the independent fixture from that readback,
then register the matching QML properties. The fixture contains 77 variables
and retains the original 69 values and IDs, 36 typography variables, seven text
styles and existing compatibility aliases. Runtime tests compare every saved
typed value to its corresponding `Theme` property and separately preserve the
386-entry icon palette contract.

The actual saved readback is recorded in
[`build/accent-palette-20261009/figma-readback.json`](../build/accent-palette-20261009/figma-readback.json).
The preserved original fixture is
[`figma-contract-before.json`](../build/accent-palette-20261009/figma-contract-before.json).
All original 69 variable IDs, names, values and scopes and all seven styles are
unchanged. The 1,548 hidden compatibility variables remain hidden. The eight
new variables are visible in the existing Dark-mode color collection, use
`FRAME_FILL`, `SHAPE_FILL`, `TEXT_FILL` and `STROKE_COLOR` scopes, and record
the matching QML code syntax. The editable specimen passed variable binding,
48-pixel ellipse, frame-boundary and font checks.

Before registering the eight source properties, the updated tests produced the
expected Red result: **144 passed, 16 failed, 1 skipped**. Each missing property
failed once in `saved_theme_tokens` and once in
`image_accent_palette_remains_fixed`. The latter has eight rows that compare
independent image samples, saved Figma values and QML colors, then change
`primaryColor` to `#A571E6`, `#FF9F45` and `#0A84FF` while requiring each fixed
palette color to remain unchanged. The
captured failing test evidence is [red-ctest.log](../build/accent-palette-20261009/red-ctest.log)
and [red-summary.json](../build/accent-palette-20261009/red-summary.json).
The eight properties are now registered. The current extension was verified
with the following source-checkout commands, both returning exit status `0`:

```sh
cmake --build build -j4
ctest --test-dir build --output-on-failure -j2 --output-junit accent-palette-20261009/full-ctest.xml
```

The full build succeeded. The full CTest run passed **56/56 targets**, failed
**zero targets** and took **84.62 seconds**. All **149 current contract rows**
passed: `saved_theme_tokens` **77**, `image_accent_palette_remains_fixed` **8**,
`legacy_theme_aliases` **22**, `saved_text_style_bindings` **35** and
`saved_text_style_font_recipes` **7**. This total includes the retained earlier
contract and the new palette requirements; it is distinct from the historical
69-variable / 133-row results above.

The parity Qt result is **160 passed, 0 failed, 1 skipped**. The full Qt output
retains the same **11 conditional skipped cases** as the earlier run. The parity
skip still requires a native RHI scene graph; skipped capture, hover and
Cocoa/RHI cases are not native-rendering proof.

| Current extension evidence | Collected artifact |
| --- | --- |
| Full build | [build.log](../build/accent-palette-20261009/build.log) |
| Full CTest | [full-ctest.log](../build/accent-palette-20261009/full-ctest.log) |
| JUnit XML | [full-ctest.xml](../build/accent-palette-20261009/full-ctest.xml) |
| Figma parity details | [canonical-green.log](../build/accent-palette-20261009/canonical-green.log) |
| Full Qt case output | [full-qt-tests.log](../build/accent-palette-20261009/full-qt-tests.log) |
| Machine-readable test summary | [test-summary.json](../build/accent-palette-20261009/test-summary.json) |

Updating installed packages, testing installed consumers, launching/restarting
consumer applications and verifying Figma library publication were not
performed for this extension. The current source build and tests do not
establish those outcomes.

Current 77-variable fixture SHA256:
`4abd11e1c8bdc19ea92c8b17522674dc6915756ac71385198a21290911940dbb`.
