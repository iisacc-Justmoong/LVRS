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

## Text colors and accent-backed roles

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

2026-10-29 확인 기준 Figma의 `Action/*` 변수는 삭제되었다. 컴포넌트의 기본 역할은
Accent Blue, Green, Yellow, Red를 사용한다. 기존 QML 사용자를 위해 `Theme.primary`,
`success`, `warning`, `danger` 속성은 유지하며 각각 기본 Blue와 Green, Yellow, Red를
참조한다. `ApplicationWindow.primaryColor`는 앱별 primary 오버라이드를 계속 제공한다.
고정 Accent 팔레트와 상태 색상은 앱별 오버라이드로 바뀌지 않는다.

<a id="ten-original-accent-colors"></a>

## Ten authored accent colors

The ten swatches supplied by the user replace the previous canonical palette in place. Existing variable IDs and public property names are retained. Every alpha is 1. The exact floating-point oracle is `authoredAccentPalette.colors` in the fixture; HEX is for display and static SVG paints.

| Figma variable / QML property | HEX display | Source node | Retained variable ID |
| --- | --- | --- | --- |
| `Accent/Blue/Default` / `Theme.accentBlue` | `#1582FF` | `1477:30` | `VariableID:1305:76` |
| `Accent/Red/Default` / `Theme.accentRed` | `#FF4646` | `1477:31` | `VariableID:1305:77` |
| `Accent/Green/Default` / `Theme.accentGreen` | `#00E71F` | `1477:32` | `VariableID:1305:78` |
| `Accent/Yellow/Default` / `Theme.accentYellow` | `#FFD427` | `1477:33` | `VariableID:1305:79` |
| `Accent/Purple/Default` / `Theme.accentPurple` | `#874AFF` | `1477:34` | `VariableID:1305:80` |
| `Accent/Rose/Default` / `Theme.accentRose` | `#F94471` | `1477:36` | `VariableID:1305:81` |
| `Accent/Orange/Default` / `Theme.accentOrange` | `#FF7A27` | `1477:35` | `VariableID:1305:82` |
| `Accent/Brown/Default` / `Theme.accentBrown` | `#432318` | `1477:37` | `VariableID:1305:83` |
| `Accent/Cyan/Default` / `Theme.accentCyan` | `#21D3FF` | `1477:38` | `VariableID:1305:84` |
| `Accent/Teal/Default` / `Theme.accentTeal` | `#00C7DD` | `1477:40` | `VariableID:1305:85` |

`accentOrangeMuted` and `accentBrownMuted` retain their public names as aliases of the corresponding canonical colors. `accentBlueMuted` is the new Blue composited at 25% over the interactive surface. The **386** extracted `accentPaletteTokens` entries retain their names, order and `{ name: string, color: string }` schema. Their matching accent and composite strings are recolored; unrelated artwork colors remain. `accentPaletteTokenCount` remains `386`.

## Accent 역할 토큰

Figma는 열 가지 Accent 각각에 `Default`, `Hover`, `Pressed`, `Selected`, `Disabled`,
`Subtle`, `Outline`, `Overlay`, `Text`를 제공한다. QML에서는
`Theme.accentRoleColor("Green", "Hover")`, 네이티브 C++에서는
`Theme::accent_role(Accent::green, AccentRole::hover)`로 조회한다. 네이티브 기본 역할은
`AccentRole::standard`이다. 기본 Blue 버튼과 위험 Red 버튼의 호버·눌림 색상도 이 역할을
사용한다. 앱이 `primaryColor`를 별도로 지정하면 기존 사용자 정의 primary 동작을 유지한다.

Hover와 Pressed는 기본 RGB의 `1/1.12`, `1/1.2`, Disabled는 기본 알파의 30%, Overlay는
`64/255`이다. Selected·Subtle·Outline은 interactive 표면에 각각 25%·16%·36%를 합성한다.
Text는 기본적으로 흰색을 30% 혼합하고 Brown과 Blue는 Figma에 저장된 별도 값을 사용한다.
Blue는 `#5AAAFF`다. 원본 90개 RGBA 값과 변수 ID는
`tests/fixtures/figma-accent-roles.json`에 보관하고 QML과 C++ 테스트가 각각 대조한다.

<a id="eight-attached-palette-colors"></a>

On 2026-10-21, the user set `accentGreen` to **#00E71F**. The canonical variable,
the compatibility Green alias, and the Green gradient stops
retain their IDs. The stops retain their existing alpha. Matching SVG paints
and their current manifest hashes follow the new Green while geometry remains
unchanged. The authored fixture records this per-color revision.

## Compatibility accent aliases

기존의 독립 팔레트는 열 가지 Accent로 대체되었다. Figma에서 일곱 호환 별칭 변수는
삭제되었으나, 기존 QML 호출을 위해 아래 일곱 속성은 정식 Accent를 계속 참조한다.
Gray 변수는 Figma의 `Neutral/Gray`로 유지된다.

| Compatibility property | Canonical property / value | Figma status |
| --- | --- | --- |
| `accentPaletteBlue` | `accentBlue` | removed |
| `accentPalettePurple` | `accentPurple` | removed |
| `accentPalettePink` | `accentRose` | removed |
| `accentPaletteRed` | `accentRed` | removed |
| `accentPaletteOrange` | `accentOrange` | removed |
| `accentPaletteYellow` | `accentYellow` | removed |
| `accentPaletteGreen` | `accentGreen` | removed |
| `accentPaletteGray` | neutral `#8C8C8C` | `Neutral/Gray`, `VariableID:1325:9` |

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

각 QML 레시피의 `Weight`, `StyleName`, `LineHeight`, `LetterSpacing` 공개 속성은
호환성을 위해 유지한다. 현재 Figma 타이포그래피는 글꼴 1개, 일곱 쌍의 크기와
행 높이 14개, 공유 굵기 4개, 공유 자간 1개로 모두 20개 변수를 사용한다.

| Figma 공유 변수 | 값 | ID |
| --- | --- | --- |
| `Font/fontBody` | Pretendard | `VariableID:1305:86` |
| `Weight/Bold` | 700 | `VariableID:1305:90` |
| `Weight/SemiBold` | 600 | `VariableID:1305:100` |
| `Weight/Medium` | 500 | `VariableID:1305:110` |
| `Weight/Regular` | 400 | `VariableID:1305:120` |
| `Tracking/Normal` | 0 | `VariableID:1305:91` |

### Actual style bindings

일곱 Figma 텍스트 스타일은 공유 글꼴, 해당 스타일의 크기와 행 높이, 공유 굵기와
자간을 각각 바인딩한다. `fontWeight`가 실제 글꼴 스타일을 선택하며 별도의
`fontStyle` 바인딩은 없다. 이전 개별 굵기·자간·스타일 이름 변수는 Figma에서
삭제되었고, 대응하는 QML 속성은 앱 호환성을 위해 남아 있다.

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
- The 50 retained Figma variables, all 90 accent roles, and shared typography/style contracts.
- Rendered pixel samples for all ten canonical swatches.
- Default and custom app Primary propagation, controls, overlays, and ten catalog choices.
- SVG resource hashes, unchanged non-paint geometry, raster alpha silhouettes, and legacy resource aliases.
- ColorPicker defaults and recent colors from the same canonical palette.

The audit files are in `build/accent-palette-20261019/`: `figma-readback.json`, `figma-variable-update.json`, `figma-page-updates.json`, `figma-extra-page-updates.json`, `source-icon-updates.json`, build and test logs. The current runtime swatch capture is `build/accent-palette-preview.png`.

The original 2026-10-08/09 69-variable and eight-color extension records remain historical snapshots under `build/theme-token-refresh-20261008/` and `build/accent-palette-20261009/`. Their former separate palette values and earlier pass counts do not describe this revision. Installation, consumer rebuilds and library publication have separate evidence; no installation is performed by this task.

The final macOS source build succeeded. A fresh focused CTest run passed **12/12 targets** with **0 failures** in **64.29 seconds** after the final helper-resource update. The JUnit report is [`final-ctest.xml`](../build/accent-palette-20261019/final-ctest.xml), with a machine-readable summary in [`verification.json`](../build/accent-palette-20261019/verification.json). Six conditional cases for native RHI, hover delivery and optional captures remain skipped and are recorded in `final-qt-tests.log`; this run does not claim all-platform native execution or installation.
