# Figma theme token contract

This document records the Figma-first color and typography contract for the LVRS GUI theme. The current color authority is the 2026-10-31 readback of [the LVRS Figma file](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS), stored at [`resources/figma-color-tokens.json`](../resources/figma-color-tokens.json). It contains 90 accent-state colors, 29 derived icon colors and 22 window, panel and material colors. Typography and compatibility details are retained in [`tests/fixtures/figma-theme-contract.json`](../tests/fixtures/figma-theme-contract.json); the earlier 2026-10-19 capture is historical.

`scripts/sync_figma_tokens.py` generates the C++ color table and updates the generated sections of `src/qml/Theme.qml` and the window-noise defaults. Its `--check` option verifies those outputs against the checked-in snapshot. Hexadecimal values are rounded for display; exact comparisons use the captured floating-point RGBA channels. The palette rendering test compares pixels in memory; an optional `LVRS_ACCENT_CAPTURE_PATH` can be supplied explicitly for a capture under `resources/`. Source builds and tests do not install the SDK. The user performs `lvrs install` for this revision.

## Scope and naming

- The theme exposes 13 panel/window properties, six text colors, ten canonical
  accent colors, compatibility aliases and neutral Gray. Names express roles.
- The QML typography API retains a shared family and seven five-property recipes;
  the current Figma styles use 20 shared typography variables.
- Existing IDs are retained when an existing variable or style is renamed. Old
  QML panel and typography property names remain compatibility aliases.
- The current snapshot also contains the nine accent states per color, 29 derived icon colors and material/background roles. Unrelated platform and control-specific tokens retain their existing behavior.
- The collection metadata slot `codeSyntax.WEB` intentionally contains **QML**
  `Theme.<property>` references. It is not CSS syntax or a web-only implementation.

| Collection | ID | Recorded contract |
| --- | --- | ---: |
| Theme Colors | `VariableCollectionId:44:457` | Core roles and the Figma color snapshot |
| Theme Typography | `VariableCollectionId:1305:73` | 20 current Figma variables; 36 QML recipe properties |
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
| `Panel/canvas` | `Theme.canvas` | `(0.06189476698637009, 0.06189476698637009, 0.06373938918113708)` | `VariableID:44:467` |
| `Panel/surfaceGhost` | `Theme.surfaceGhost` | `(0.0696646124124527, 0.06979759782552719, 0.07046254724264145)` | `VariableID:44:468` |
| `Panel/windowAlt` | `Theme.windowAlt` | `(0.1061861664056778, 0.1061861664056778, 0.1091860681772232)` | `VariableID:44:469` |
| `Panel/subSurface` | `Theme.subSurface` | `(0.12038286030292511, 0.12213718146085739, 0.12389150261878967)` | `VariableID:44:470` |
| `Panel/surfaceSolid` | `Theme.surfaceSolid` | `(0.14418363571166992, 0.1475532352924347, 0.1475532352924347)` | `VariableID:215:9618` |
| `Panel/surfaceAlt` | `Theme.surfaceAlt` | `(0.15562701225280762, 0.15847958624362946, 0.1613321751356125)` | `VariableID:215:9620` |
| `Panel/surfaceInteractive` | `Theme.surfaceInteractive` | `(0.17858824133872986, 0.17858824133872986, 0.17858824133872986)` | `VariableID:215:9621` |
| `Panel/surfacePanel` | `Theme.surfacePanel` | `(0.17342455685138702, 0.17747892439365387, 0.1795060932636261)` | `VariableID:215:9622` |
| `Panel/surfaceControl` | `Theme.surfaceControl` | `(0.18747973442077637, 0.192999929189682, 0.192999929189682)` | `VariableID:215:9623` |
| `Panel/surfaceInput` | `Theme.surfaceInput` | `(0.19681522250175476, 0.1986967921257019, 0.20057833194732666)` | `VariableID:215:9624` |
| `Panel/surfaceInputHover` | `Theme.surfaceInputHover` | `(0.2218484729528427, 0.23012775182724, 0.23288752138614655)` | `VariableID:215:9625` |
| `Panel/surfaceRaised` | `Theme.surfaceRaised` | `(0.24484941363334656, 0.25022611021995544, 0.2529144883155823)` | `VariableID:215:9626` |
| `Panel/window` | `Theme.window` | `(0.09921568632125854, 0.09921568632125854, 0.09921568632125854)` | `VariableID:1305:74` |

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
| `Accent/Blue/Default` / `Theme.accentBlue` | `#2880E5` | `1477:30` | `VariableID:1305:76` |
| `Accent/Red/Default` / `Theme.accentRed` | `#E55050` | `1477:31` | `VariableID:1305:77` |
| `Accent/Green/Default` / `Theme.accentGreen` | `#15D02E` | `1477:32` | `VariableID:1305:78` |
| `Accent/Yellow/Default` / `Theme.accentYellow` | `#E5C236` | `1477:33` | `VariableID:1305:79` |
| `Accent/Purple/Default` / `Theme.accentPurple` | `#8453E5` | `1477:34` | `VariableID:1305:80` |
| `Accent/Rose/Default` / `Theme.accentRose` | `#E04D72` | `1477:36` | `VariableID:1305:81` |
| `Accent/Orange/Default` / `Theme.accentOrange` | `#E57936` | `1477:35` | `VariableID:1305:82` |
| `Accent/Brown/Default` / `Theme.accentBrown` | `#3C2319` | `1477:37` | `VariableID:1305:83` |
| `Accent/Cyan/Default` / `Theme.accentCyan` | `#32C2E5` | `1477:38` | `VariableID:1305:84` |
| `Accent/Teal/Default` / `Theme.accentTeal` | `#14B5C7` | `1477:40` | `VariableID:1305:85` |

`accentOrangeMuted` and `accentBrownMuted` retain their public names as aliases of the corresponding canonical colors. `accentBlueMuted` follows the captured `Accent/Blue/Selected` role. The **386** extracted `accentPaletteTokens` entries retain their names, order and `{ name: string, color: string }` schema. Their matching accent and composite strings are recolored; unrelated artwork colors remain. `accentPaletteTokenCount` remains `386`.

## Accent 역할 토큰

Figma는 열 가지 Accent 각각에 `Default`, `Hover`, `Pressed`, `Selected`, `Disabled`,
`Subtle`, `Outline`, `Overlay`, `Text`를 제공한다. QML에서는
`Theme.accentRoleColor("Green", "Hover")`, 네이티브 C++에서는
`Theme::accent_role(Accent::green, AccentRole::hover)`로 조회한다. 네이티브 기본 역할은
`AccentRole::standard`이다. 기본 Blue 버튼과 위험 Red 버튼의 호버·눌림 색상도 이 역할을
사용한다. 앱이 `primaryColor`를 별도로 지정하면 기존 사용자 정의 primary 동작을 유지한다.

각 상태값은 현재 Figma 변수에 저장된 RGBA를 그대로 사용한다. 배경이 바뀌었다고 Selected·Subtle·Outline을 새 배경 위에서 다시 합성하지 않는다. 기본 아이콘의 Default 연결은 해당 색상의 Hover, Hover 연결은 Pressed를 사용한다. Figma가 명시한 `Icon/Accent/.../Darker` 29개는 각각 실제 저장값을 읽으며, 현재 값은 원래 역할의 RGB를 90%로 낮추고 알파를 유지한다. 명시된 아이콘 변수가 없는 나머지 역할에는 같은 90% 규칙을 대체값으로 사용한다. QML `Theme.iconAccentRoleColor`와 C++ `Theme::icon_accent_role`은 이 연결을 제공한다. 90개 상태, 29개 아이콘 및 22개 배경의 값과 ID는 `tests/fixtures/figma-accent-roles.json`에서 검증한다.

<a id="eight-attached-palette-colors"></a>

Historically, on 2026-10-21, the user set `accentGreen` to **#00E71F**; the current 2026-10-31 value is shown above. The canonical variable,
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

The 2026-10-19 audit and runtime capture described an earlier revision; build directories are not long-term evidence storage.

The original 2026-10-08/09 69-variable and eight-color extension records are historical snapshots. Their former separate palette values and earlier pass counts do not describe this revision. Installation, consumer rebuilds and library publication have separate evidence; no installation is performed by this task.

The 2026-10-19 macOS focused run reported 12/12 CTest targets; it is historical and does not certify the 2026-10-31 changes or an installation.
