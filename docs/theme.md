<a id="theme"></a>

# 테마

위치: `src/qml/Theme.qml`

`Theme` 는 싱글턴 를 위한 전역 디자인 토큰 LVRS QML 컴포넌트입니다. 모든 플랫폼은 간격, 반지름, 컨트롤 크기, 아이콘, 타이포그래피, 줄 간격에 대해 동일한 작성된 논리적 크기를 사용합니다. iOS 와 Android 는 자동 크기 승수 추가를 하지 않습니다. 본문은 `13px / 13px` 로 유지됩니다.

<a id="token-groups"></a>

## 토큰 그룹

- 타이포그래피: 글꼴 모음 해상도 및 텍스트 크기/무게/스타일 이름 토큰.
- 표면: 창과 캔버스, 입력, 제어, 패널 등 역할별 12개 표면. 번호형 `panelBackground01..12`는 호환 별칭이다.
- 의미 색상: `primary`/`accent`, `success`, `warning`, `danger`.
- 강조 색상: 기존 원색 10개와 첨부 팔레트 원색 8개에 대응하는 이름 있는 색상 속성. iconset 추출 목록 `accentPaletteTokens` 386개는 기존 호환 API로 유지한다.
- 측정 항목: 간격, 반경, 컨트롤 크기, 대화 상자 크기 및 상호 작용 타이밍.

<a id="figma-theme-contract"></a>

## Figma 테마 계약

정식 계약은 [LVRS Figma](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS)의 실제 저장 상태를 읽은 `tests/fixtures/figma-theme-contract.json`이다. 초기 계약은 `Theme Colors`의 색상 33개와 `Theme Typography`의 타이포그래피 36개, 총 69개이며 텍스트 스타일은 7개이다. 2026-10-09 첨부 팔레트 원색 8개를 실제 Figma에 저장하고 재조회한 현재 계약은 색상 41개와 동일한 타이포그래피 36개, 총 77개이다. 현재 fixture의 캡처 시간은 `2026-10-09 00:35:00 UTC`이다. 기존 69개 변수와 7개 텍스트 스타일은 모두 보존하였으며 새 8개 소스 속성도 등록하였다. 전체 소스 빌드는 종료 코드 0으로 성공하였으며 전체 CTest 56/56 대상도 통과하였다. 초기 계획보다 실제 Figma 읽기 결과가 우선이며, 기존 RGB와 알파의 부동소수점 값을 8비트 색상으로 반올림하여 대체하지 않는다.

Figma 변수의 `codeSyntax.WEB`는 의도적으로 `Theme.canvas`, `Theme.textBody`와 같은 실제 QML 속성 참조를 기록한다. WEB 슬롯에 저장되지만 CSS 변수 선언이나 웹 전용 구현을 의미하지 않는다. 변수 ID와 기존 소비자의 바인딩은 이름을 정리해도 유지한다. 전체 값, 스타일 바인딩 및 호환 계약은 [Figma Theme Tokens](figma-theme-tokens.md)를 참조한다.

<a id="typography-recipes"></a>

## 타이포그래피 레시피

모든 레시피는 Pretendard를 사용한다. `fontBody`는 실제 실행 환경에서 `FontPolicy.resolveFamily(FontPolicy.preferredFamily)`로 해석되며, Figma의 선호 글꼴과 실행 시 대체 글꼴 선택을 구분한다.

| Figma 스타일 | 기본 속성 | 크기 / 줄 높이 | 무게 | 스타일 이름 | 자간 |
| --- | --- | --- | --- | --- | --- |
| Title | `textTitle` | 26 / 26 | 700 | Bold | 0 |
| Subtitle | `textSubtitle` | 22 / 22 | 700 | Bold | 0 |
| Header | `textHeader` | 17 / 17 | 600 | SemiBold | 0 |
| Subheader | `textSubheader` | 15 / 15 | 600 | SemiBold | 0 |
| Body | `textBody` | 13 / 13 | 500 | Medium | 0 |
| Description | `textDescription` | 12 / 12 | 600 | SemiBold | 0 |
| Caption | `textCaption` | 11 / 11 | 400 | Regular | 0 |

각 기본 속성에는 `Weight`, `StyleName`, `LineHeight`, `LetterSpacing`의 네 동반 속성이 있다. 7개 레시피의 35개 변수와 공유 `fontBody` 변수 1개가 타이포그래피 36개를 구성한다. 크기, 줄 높이, 자간은 논리 픽셀이다.

실제 Figma 텍스트 스타일의 변수 바인딩은 `fontFamily`, `fontSize`, `fontWeight`, `lineHeight`, `letterSpacing`의 5개이다. `fontWeight` 바인딩이 해당 정적 글꼴 스타일을 선택하며 `fontStyle` 바인딩을 대체한다. `StyleName` STRING 변수는 QML의 Bold/SemiBold/Medium/Regular 레시피를 기록하며, 여섯 번째 Figma 바인딩으로 주장하지 않는다.

기존 `textTitle2`와 네 동반 속성은 `textSubtitle` 계열의 호환 별칭이다. 기존 `textHeader2` 계열 5개도 `textSubheader` 계열을 가리킨다. Label의 기존 `title2`/`header2` 스타일 enum 및 다른 소비자의 명칭은 이 토큰 정리에서 변경하지 않는다. `textDisabled`는 별도 크기를 만들지 않고 Caption의 크기·무게·스타일·줄 높이·자간을 유지한다.

<a id="app-accent"></a>

## 앱 악센트

`ApplicationWindow.primaryColor` 를 앱 루트에 설정하세요. 예를 들어 보라색을 위한 `primaryColor: "#A571E6"` 입니다. `Theme.primaryColor` 를 작성하며, 기존 읽기 전용 `primary` 와 `accent` 토큰은 현재 값을 노출합니다. 초기 대체 경로, `defaultPrimary` 는 `#0A84FF` 로 유지됩니다. 변화는 기존 컨트롤과 새로 로드된 콘텐츠로 전파됩니다. 구성되지 않은 보조 창은 동일한 테마를 상속합니다. 이 범위는 윈도우에 의해 공유되는 하나의 QML 엔진이며, 주요 루트에서 기본 색상을 할당합니다. [ApplicationWindow](components/app/ApplicationWindow.md#app-primary-color)를 참조하세요.

|토큰|기본 파란색|사용자 지정 기본|
| --- | --- | --- |
| `primary`, `accent` | `#0A84FF` |제공 색상|
| `accentTint` | `#1F0A84FF` |기본 RGB, 알파에 `31/255`를 곱한 값|
| `primaryOverlay`, `accentOverlay` | `#400A84FF` |기본 RGB, 알파에 `64/255`를 곱한 값|
| `accentMuted` | `#25324D` |25%에서 기본이 `panelBackground07` 위에 있습니다.|
| `accentDetail` | `#548AF7` |기본 색상|
| `alertActionPrimary` | `#027DFF` |기본 색상|
| `alertIconSurface` | `#192840` |16%에서 기본이 `panelBackground07` 위에 있습니다.|
| `alertIconBorder` | `#244B7E` |36%에서 기본이 `panelBackground07` 위에 있습니다.|

사용자 지정 기본 알파는 불투명한 땀빛 및 경고 아이콘 표면의 틴트 강도에도 곱해집니다. 파란색 대체 경로 는 기존 작성된 색상을 유지합니다. Figma 의 파란색은 브랜드화된 애플리케이션을 위한 고정된 색상이 아닌 기본 테마입니다: Society 의 `#57965C` 1 차원 또한 알림 작업 및 아이콘 강조에 적용됩니다. 의미론적 누름/선택 상태는 `accentMuted` / `accentDetail` 를 소모하며, 이름이 지정된 아이콘 팔레트 ( `accentBlue` ,  `accentBlueMuted` , 그리고  `accentPaletteTokens` ) 는 고정되어 있습니다. 상태 색상 및 명시적인 컨트롤별 오버라이드도 고정됩니다. CheckBox 자동으로 사용자 지정 선택색을 가진 그어진 체크 표시를 사용하여 제공된 강조 색상이 표준 파란색 이미지로 덮일 수 없도록 합니다.

<a id="interaction-timing"></a>

## 상호작용 타이밍

`toggleTransitionDuration`는 모든 대상에서 `320ms`입니다. ToggleSwitch는 손잡이의 이동 및 리바운드에 이를 사용하며, 프레스 피드백은 해당 기간의 1/4을 차지합니다. 속도를 변경하려면 컨트롤의 `transitionDuration`를 재정의하거나 즉각적인 피드백을 위해 `0`를 사용하세요.

<a id="compact-icons"></a>

## 컴팩트 아이콘

- `iconSm`는 데스크톱 및 모바일에서 `18 x 18` 논리 픽셀로 확인됩니다.
- 공개 크기 속성이 명시적으로 재정의되지 않는 한 스톡 작업, 메뉴, 계층 구조, 입력 및 선택 제어 아이콘은 이 토큰을 사용합니다.
- 아이콘 이미지는 정사각형 논리 프레임과 `Image.PreserveAspectFit`를 유지하므로 정사각형이 아닌 SVG 아트워크는 왜곡 없이 비례적으로 크기가 조정됩니다.

<a id="shared-desktop-and-mobile-sizing"></a>

## 공유 데스크탑 및 모바일 크기 조정

- `Theme`는 `Platform.runtimeProfile()`를 통해 현재 런타임 대상을 확인합니다.
- `metricScaleFactor` 및 `typographyScaleFactor`는 유효 대상이 `ios` 또는 `android`인 경우를 포함하여 항상 `1.0`입니다.
- `scaleMetric()` 및 `scaleTextMetric()`는 숫자 변환 및 반올림을 유지합니다. `scaleRealMetric()`는 분수 값을 유지합니다. 없음은 플랫폼 승수를 추가합니다.
- 텍스트 크기와 일치하는 줄 높이는 모든 대상에서 `26`, `22`, `17`, `15`, `13`, `12`, `11`이다.
- `FontPolicy`는 이러한 승인된 크기만 인식합니다. 이전 `2x` 및 `1.25x` 별칭은 거부됩니다. `22px`는 Bold Subtitle(기존 Title2 별칭)에 속하고 `26px`는 Bold Title에 속하며 이중 캡션이나 본문 별칭은 없습니다.
- `ApplicationWindow.mobileViewScale` 및 `Window.mobileViewScale`는 기본적으로 `1.0`로 설정됩니다. 명시적으로 구성된 구성 크기 조정은 계속 사용할 수 있습니다. 장치 픽셀 비율 처리 및 렌더링 품질 샘플링은 테마 크기 조정과 별도로 유지됩니다.

미리보기/테스트 도우미:

- `targetOverride: string`
- `effectiveTarget: string`
- `mobileTarget: bool`
- `metricScaleFactor: real`
- `typographyScaleFactor: real`
- `scaleMetric(value)`
- `scaleRealMetric(value)`
- `scaleTextMetric(value)`

<a id="window-and-panel-surfaces"></a>

## 창 및 패널 표면

창 기초는 `window`이며 `Panel/window`와 대응한다. 기본 렌더링 색상은 `#141414`이다. 패널은 밝기 번호보다 실제 용도를 나타내는 이름을 우선한다. 아래 호환 별칭은 기존 색상값과 소비자의 동작을 유지하며 각 의미 역할을 가리킨다.

| 기존 호환 속성 | 정식 역할 속성 | Figma 변수 |
| --- | --- | --- |
| `panelBackground01` | `canvas` | `Panel/canvas` |
| `panelBackground02` | `surfaceGhost` | `Panel/surfaceGhost` |
| `panelBackground03` | `windowAlt` | `Panel/windowAlt` |
| `panelBackground04` | `subSurface` | `Panel/subSurface` |
| `panelBackground05` | `surfaceSolid` | `Panel/surfaceSolid` |
| `panelBackground06` | `surfaceAlt` | `Panel/surfaceAlt` |
| `panelBackground07` | `surfaceInteractive` | `Panel/surfaceInteractive` |
| `panelBackground08` | `surfacePanel` | `Panel/surfacePanel` |
| `panelBackground09` | `surfaceControl` | `Panel/surfaceControl` |
| `panelBackground10` | `surfaceInput` | `Panel/surfaceInput` |
| `panelBackground11` | `surfaceInputHover` | `Panel/surfaceInputHover` |
| `panelBackground12` | `surfaceRaised` | `Panel/surfaceRaised` |

`canvas`는 작업 캔버스, `surfaceGhost`는 약한 표면, `windowAlt`는 보조 창 표면, `subSurface`는 내부 영역, `surfaceSolid`는 불투명한 표면, `surfaceAlt`는 보조 표면의 역할이다. `surfaceInteractive`는 상호작용 표면, `surfacePanel`은 패널, `surfaceControl`은 제어, `surfaceInput`과 `surfaceInputHover`는 입력의 기본/호버 상태, `surfaceRaised`는 상승된 표면의 역할이다.

이 표는 동일 간격의 명도 팔레트나 `50..900` 숫자 스케일을 정의하지 않는다. 특히 07과 08의 실제 RGB 관계를 이름 순서에 맞춰 다시 계산하지 않는다. 정확한 채널값은 Figma 계약의 `resolved` 값과 [상세 표](figma-theme-tokens.md#panel-roles-and-exact-values)를 따른다. `panelBackground04`의 8비트 표시 예시는 `#181919`이지만 구현값을 이 문자열로 반올림하지 않는다.

<a id="alert-glass"></a>

## 경고 유리

업데이트된 Figma 경고는 전용 색상 토큰을 사용합니다. 타이포그래피는 기존 제목 및 본문 토큰을 계속 사용합니다. 이러한 색상은 텍스트 스타일을 생성하지 않습니다.

|토큰|기본값|사용|
| --- | --- | --- |
| `alertGlassTint` |`#1D1F21`, 72% 불투명도|반투명 카드 틴트|
| `alertGlassEdge` |흰색, 20% 불투명도|카드 엣지|
| `alertTitleColor` | `#F4F5F7` |제목 및 중립 작업|
| `alertBodyColor` | `#D6D9DF` |메시지|
| `alertActionPrimary` | `#027DFF` |기본 버튼|
| `alertActionBorder` | `#596168` |보조 아웃라인|
| `alertDivider` | `#363B3F` |콘텐츠/작업 구분 기호|
| `alertIconSurface` | `#192840` |아이콘 프레임 채우기|
| `alertIconBorder` | `#244B7E` |아이콘 프레임 가장자리|

폐기 텍스트는 기존 `danger` 토큰(`#FF453A`)을 사용합니다. 참조
자재 캡처, 레이아웃 및 대체 경로 동작에 대한 [Alert](components/surfaces/Alert.md). 경고의 500px 기본 너비는 아래의 이전 공유 대화 상자 경계와 무관합니다. 기본 작업과 아이콘 프레임 색상은 위에서 설명한 대로 맞춤 앱 강조를 따릅니다.

<a id="textfield-glass"></a>

## TextField 유리

Figma TextField `114:179`는 기존 `panelBackground10` RGB 채널에서 파생된 이러한 의미 채우기를 사용합니다. 기존 패널 및 경고 토큰은 변경되지 않습니다.

|토큰|알파|사용|
| --- | --- | --- |
| `inputFieldGlassTint` | 64% |둥근/원통|
| `inputFieldGlassTintDisabled` | 36% |비활성화 둥근/원통|
| `inputFieldGlassTintInline` | 16% |내선, 비활성화 포함|
| `inputFieldGlassReflection` |1.8% 흰색|공유 자료 반사|

효과 거리와 색상 알파는 데스크톱과 모바일에서 동일한 값을 사용합니다. 그라데이션, 삽입 그림자, 흐림, 캡처 소스 및 렌더러 대체 경로 계약은 [InputField](components/control/InputField.md#textfield-material)를 참조하세요.

<a id="hex-format-rule"></a>

## 색상값 표현 규칙

- 새 고정 8비트 색상은 6자리 16진수로 표시할 수 있으며, 알파가 필요한 기존 고정 토큰은 8자리 표기를 사용할 수 있다.
- 실제 Figma에서 읽은 RGB와 알파는 정규화된 부동소수점 값을 보존한다. 특히 패널 색상과 텍스트 알파를 6자리/8자리 16진수로 양자화하지 않는다.
- 재료 채움과 정확한 불투명도 비율은 기존 GUI 계층의 `Qt.rgba`를 사용한다. 이 QML 계약은 Qt가 없는 하위 SDK에 Qt 의존성을 추가하는 작업이 아니다.
- 상세 문서의 원색 HEX는 읽기 쉬운 표시이다. 실제 비교의 기준은 fixture의 `resolved` 채널값이며 Figma 직렬화 정밀도만 허용 오차로 구분한다.

<a id="icon-path-resolution"></a>

## 아이콘 경로 해상도

`Theme.iconPath(iconName)`는 논리적 아이콘 이름을 다음으로 확인합니다. `qrc:/qt/qml/LVRS/resources/iconset/`

디렉토리는 하나의 평평한 레이어에 2,863-아이콘 Figma 아이콘세트 스냅샷의 전체 내용을 포함합니다. [에서](figma-iconset-import.md) 이름 지정 및 확인을 참조하세요. 모든 제공된 SVG 아이콘은 상대적 루트 차원, 중앙 정렬된 사각형 뷰 박스, `xMidYMid meet` 비율 보존을 사용합니다. 가시적인 아트워크는 소비되는 컨트롤의 크기가 변경되더라도 중앙에 유지되며, 기본 논리 아이콘 크기는 18 픽셀로 유지됩니다.

규칙:
- 빈 입력은 빈 문자열을 반환합니다.
- 전체 리소스 경로(`:/`)는 있는 그대로 반환됩니다.
- 생략시 `.svg`가 추가됩니다.
- 레거시 논리적 별칭은 제공된 아이콘 파일 이름(예: `projectStructure -> generalprojectStructure`, `add -> generaladd`, `viewMoreSymbolicDefault -> generalmoreHorizontal`, `panDownSymbolic* -> generalchevronDown*`)으로 정규화됩니다.
- 그룹 스타일 이름은 조회 전에 평면화됩니다(예: `general/projectStructure -> generalprojectStructure.svg`).

<a id="accent-palette-tokens"></a>

## 액센트 원색 토큰

Figma의 핵심 `Accent` 변수는 기존 원색 10개와 첨부 팔레트 원색 8개로 구성한다. 다음 기존 원색 10개는 QML의 이름 있는 `color` 속성과 대응하며 값을 유지한다. `accentOrange`와 `accentBrown`은 기존 `accentOrangeMuted`/`accentBrownMuted`의 동일한 값을 노출하며 기존 속성을 제거하지 않는다.

| QML 속성 | 고정 색상 표시 | Figma 변수 |
| --- | --- | --- |
| `accentBlue` | `#548AF7` | `Accent/accentBlue` |
| `accentRed` | `#DB5C5C` | `Accent/accentRed` |
| `accentGreen` | `#57965C` | `Accent/accentGreen` |
| `accentYellow` | `#F2C55C` | `Accent/accentYellow` |
| `accentPurple` | `#A571E6` | `Accent/accentPurple` |
| `accentRose` | `#E55765` | `Accent/accentRose` |
| `accentOrange` | `#C77D55` | `Accent/accentOrange` |
| `accentBrown` | `#45322B` | `Accent/accentBrown` |
| `accentCyan` | `#40B6E0` | `Accent/accentCyan` |
| `accentTeal` | `#24A394` | `Accent/accentTeal` |

2026-10-09 첨부 이미지의 왼쪽부터 Blue, Purple, Pink, Red, Orange, Yellow, Green, Gray 원색을 다음 별도 속성에 추가한다. `accentPalette<Color>`는 이 8색 팔레트의 실제 색상명을 나타내며 밝기 단계나 숫자 레벨을 뜻하지 않는다. 모든 색상은 불투명도 1이다. [편집 가능한 Figma 팔레트](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS?node-id=1325-10&m=dev)에 각 변수와 연결한 48픽셀 원 8개를 추가하였다.

| QML 속성 | 정확한 sRGB 색상 | Figma 변수 |
| --- | --- | --- |
| `accentPaletteBlue` | `#007AFF` | `Accent/accentPaletteBlue` |
| `accentPalettePurple` | `#A550A7` | `Accent/accentPalettePurple` |
| `accentPalettePink` | `#F74F9E` | `Accent/accentPalettePink` |
| `accentPaletteRed` | `#FF5257` | `Accent/accentPaletteRed` |
| `accentPaletteOrange` | `#F7821B` | `Accent/accentPaletteOrange` |
| `accentPaletteYellow` | `#FFC600` | `Accent/accentPaletteYellow` |
| `accentPaletteGreen` | `#62BA46` | `Accent/accentPaletteGreen` |
| `accentPaletteGray` | `#8C8C8C` | `Accent/accentPaletteGray` |

원본 PNG는 590 × 78이며 sRGB 메타데이터를 포함한다. 각 원의 중앙 21 × 21 영역에 있는 441개 픽셀이 모두 동일한 RGBA 값임을 확인하여 외곽 안티앨리어싱이나 배경 색상이 표본에 포함되지 않도록 하였다. 색상 추출 증거는 [image-colors.json](../build/accent-palette-20261009/image-colors.json)이며 표본 위치·영역·RGBA·균일 픽셀 수·원본 SHA256을 기록한다. 새 8색은 이 실제 8비트 원색값을 사용하고, 기존 패널과 텍스트의 부동소수점 정밀도는 그대로 유지한다.

기존 `accentPaletteTokens`는 iconset에서 추출된 386개 항목을 그대로 유지하는 호환 API이다. 아이템 스키마는 `{ name: string, color: string }`이며 `color`에는 기존 `#HEX` 문자열이 들어간다. `accentPaletteTokenCount`는 이 추출 목록의 길이인 386이다. 이 목록을 핵심 원색 18개로 축소하거나 새 8색을 이 목록에 추가하지 않는다. 이름 있는 `Theme.accent*` 속성은 QML `color`이며 추출 목록의 문자열 스키마와 구분한다.

앱의 가변 `primary` 기본값 `#0A84FF`, 기존 고정 원색 `accentBlue`의 `#548AF7`, 첨부 팔레트의 `accentPaletteBlue` `#007AFF`는 각각 다른 값이다. 앱 브랜딩을 바꾸어도 고정 원색은 바뀌지 않는다. Purple과 Pink는 별도 색상이며 Gray도 이 팔레트에서 선택 가능한 악센트이다.

기존 iconset 추출 색상과 파생 속성은 호환 용도로 보존하며 Figma 핵심 원색을 늘리는 데 사용하지 않는다. Figma의 `Compatibility Icon Colors` 컬렉션에는 기존 변수 1,548개가 남아 있다. 컬렉션과 각 변수는 게시 대상에서 숨기고 변수 scope는 비어 있지만, 기존 ID와 바인딩은 삭제하지 않는다. 주요 중복 원색은 핵심 변수에 alias로 연결한다. 이 호환 목록은 새 `50..900` 숫자 단계 팔레트를 정의하지 않는다.

<a id="related-ui-defaults"></a>

## 관련 UI 기본값

- 상황에 맞는 메뉴 색상:
  - `contextMenuSurface: materialTint` (`#141414`; ContextMenu는 25% 색조를 적용합니다)
  - `contextMenuDivider: panelBackground08`
  - `contextMenuItemSelectedBackground`
  - `contextMenuItemInactiveBackground`
- 대화상자 크기:
  - 데스크톱 및 모바일: `dialogMinWidth: 280`, `dialogMaxWidth: 360`
- 공통 반경:
  - 데스크톱 및 모바일: `radiusSm: 4`, `radiusLg: 12`

<a id="validation"></a>

## 검증

초기 2026-10-09 토큰 정리는 실제 Figma 계약의 변수 69개, 텍스트 스타일 7개, 숨긴 호환 변수 1,548개를 소스에 반영하였다. `cmake --build build -j4`는 종료 코드 0으로 전체 빌드에 성공하였다. 전체 CTest는 56/56 테스트 대상이 통과하였고 실패한 대상은 0개이며 실제 소요 시간은 82.23초이다.

`LVRSTests_figma_parity`의 새 계약 검사는 `saved_theme_tokens` 69행, `legacy_theme_aliases` 22행, `saved_text_style_bindings` 35행, `saved_text_style_font_recipes` 7행으로 총 133행이 모두 통과하였다. 기존 검사를 포함한 이 대상의 Qt 결과는 144 passed / 0 failed / 1 skipped이다. 기존 `material_inner_highlight` 1개는 네이티브 RHI scene graph가 필요한 검사이므로 offscreen 환경에서 skip하였다. 이 skip은 네이티브 렌더링 검증 완료를 의미하지 않는다.

실행한 전체 테스트 명령은 다음과 같다. `--output-junit`의 경로는 CTest가 `build/`에 진입한 뒤 상대 경로로 해석하여 처음에는 `build/build/theme-token-refresh-20261008/full-ctest.xml`을 생성하였다. 실행 후 XML을 아래 증거 폴더로 이동하여 수집하였다.

```sh
ctest --test-dir build --output-on-failure -j2 --output-junit build/theme-token-refresh-20261008/full-ctest.xml
```

| 증거 | 파일 |
| --- | --- |
| 전체 빌드 | [build.log](../build/theme-token-refresh-20261008/build.log) |
| 전체 CTest | [full-ctest.log](../build/theme-token-refresh-20261008/full-ctest.log) |
| 실행 후 수집한 JUnit XML | [full-ctest.xml](../build/theme-token-refresh-20261008/full-ctest.xml) |
| Figma 계약 검사 상세 | [canonical_green.log](../build/theme-token-refresh-20261008/canonical_green.log) |
| 보존한 전체 Qt 개별 검사 기록 | [full-qt-tests.log](../build/theme-token-refresh-20261008/full-qt-tests.log) |

전체 Qt 기록에는 위 RHI 검사와 기존 네이티브 캡처, 호버 포인터 전달, Cocoa/RHI 조건 검사를 포함하여 총 11개의 조건부 skip이 있다. 따라서 56개 테스트 대상의 통과와 모든 개별 네이티브 검사의 실행 완료를 구분한다. `~/.local` 설치본 갱신, 설치된 LVRS 소비자 검증, Dreamscapes/Society 실행 또는 재시작, Figma 라이브러리 게시 검증은 수행하지 않았다. 이 결과는 소스 checkout의 전체 빌드·CTest와 실제 Figma 계약 범위의 증거이다.

### 첨부 팔레트 8색 확장 검증

첨부 팔레트 확장은 같은 Figma 우선 순서를 따른다. 기존 `Theme Colors`에 새 8개 변수를 저장하고 실제 ID·값·scope·code syntax를 재조회한 뒤 독립 fixture를 갱신하고 동일한 QML 속성을 등록하였다. 갱신된 계약은 총 77개 변수이며 기존 69개 변수의 값과 ID, 타이포그래피 36개, 텍스트 스타일 7개와 호환 별칭을 보존한다. 실제 저장 ID와 값은 [figma-readback.json](../build/accent-palette-20261009/figma-readback.json), 기존 69개 fixture는 [figma-contract-before.json](../build/accent-palette-20261009/figma-contract-before.json)에 기록하였다.

원래 변수의 ID·이름·값·scope와 7개 스타일은 모두 동일하며, 숨긴 호환 변수 1,548개도 유지한다. 새 8개 변수는 Dark 모드의 기존 컬렉션에 저장되고 `FRAME_FILL`, `SHAPE_FILL`, `TEXT_FILL`, `STROKE_COLOR` scope와 대응하는 QML code syntax를 가진다. 팔레트 표본의 바인딩·48픽셀 원·프레임 경계·폰트 검사를 통과하였다. 최종 77개 fixture의 SHA256은 `4abd11e1c8bdc19ea92c8b17522674dc6915756ac71385198a21290911940dbb`이다.

새 8개 속성을 등록하기 전 테스트에서 144 passed / 16 failed / 1 skipped의 Red를 확인하였다. 각 누락 속성이 `saved_theme_tokens`와 `image_accent_palette_remains_fixed`에서 한 번씩 실패한 결과이다. 후자의 8행은 독립 이미지 표본·저장된 Figma 값·QML 색상을 비교하고, `primaryColor`를 `#A571E6`, `#FF9F45`, `#0A84FF`로 변경한 뒤에도 원색값이 그대로 유지되는지 확인한다. Red 증거는 [red-ctest.log](../build/accent-palette-20261009/red-ctest.log)와 [red-summary.json](../build/accent-palette-20261009/red-summary.json)에 기록하였다.

속성 등록 후 다음 전체 빌드와 전체 CTest 명령이 모두 종료 코드 0으로 성공하였다. CTest는 56/56 대상 통과, 실패 0개, 실제 소요 시간 84.62초이다. 현재 계약은 `saved_theme_tokens` 77행, `image_accent_palette_remains_fixed` 8행, `legacy_theme_aliases` 22행, `saved_text_style_bindings` 35행, `saved_text_style_font_recipes` 7행으로 총 149행이 모두 통과하였다. 위 초기 69개 변수·133행 검증과 현재 77개 변수·149행 검증은 각각의 기록이다.

```sh
cmake --build build -j4
ctest --test-dir build --output-on-failure -j2 --output-junit accent-palette-20261009/full-ctest.xml
```

| 현재 확장 증거 | 파일 |
| --- | --- |
| 전체 빌드 | [build.log](../build/accent-palette-20261009/build.log) |
| 전체 CTest | [full-ctest.log](../build/accent-palette-20261009/full-ctest.log) |
| JUnit XML | [full-ctest.xml](../build/accent-palette-20261009/full-ctest.xml) |
| Figma 계약 검사 상세 | [canonical-green.log](../build/accent-palette-20261009/canonical-green.log) |
| 전체 Qt 개별 검사 기록 | [full-qt-tests.log](../build/accent-palette-20261009/full-qt-tests.log) |
| 기계 판독 테스트 요약 | [test-summary.json](../build/accent-palette-20261009/test-summary.json) |

현재 parity Qt 결과는 160 passed / 0 failed / 1 skipped이다. 전체 Qt의 조건부 skip은 이전과 같은 11개이며, 네이티브 RHI·캡처·호버·Cocoa 검사의 실행 완료를 의미하지 않는다. 이 확장에서 설치본 갱신, 설치된 소비자 검증, 소비 앱 실행·재시작, Figma 라이브러리 게시 검증은 수행하지 않았다.

`LVRSTests_primary_color` 은 파란색 대체 경로 , 선언적 및 초기 속성 입력, QML 팔레트 바인딩, 런타임 변경 사항, 상속된 창 색상, 엔진 격리, 명시적인 컨트롤 오버라이드, 파생된 선택 색상, 그리고 렌더링된 버튼/ CheckBox 픽셀을 다룹니다. `ctest --test-dir build -R primary_color --output-on-failure` 로 실행하세요.

`LVRSTests_platform_integration` 은 Android 와 iOS 에서 작성된 컴포넌트 차원을 확인합니다. `LVRSTests_import_api` 와 `LVRSTests_list_composites` 은 플랫폼 전환, 컨트롤, 그리고 모든 ListItem 변형을 다룹니다. `LVRSTests_font_policy` 는 구식 스케일된 별칭을 거부합니다. 설치된 소비자는 `find_package(LVRS)` 를 통해 공유 크기를 확인하며, 설치된 QML 모듈을 사용합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

Rectangle {
    color: LV.Theme.window
}
```

<a id="token-extension-workflow"></a>

## 토큰 확장 워크플로우

새 토큰이나 기존 역할을 변경할 때는 Figma를 먼저 정리한 뒤 소스에 반영한다.

1. 실제 Figma 소비자와 저장된 변수/스타일 값을 읽고 역할, 원색, 타이포그래피 레시피를 결정한다. 기존 ID와 실제 RGB/알파를 보존한다.
2. Figma 이름, scope, `Theme.property` 코드 참조 및 텍스트 스타일 바인딩을 갱신하고 실제 재조회로 계약 fixture를 저장한다.
3. 실제 계약과 호환 별칭을 검증하는 테스트를 작성하여 소스 반영 전 실패를 확인한다.
4. `Theme.qml`에 역할 속성 및 필요한 단방향 호환 별칭을 반영한다. 값이 같은 새 이름을 위해 소비자 전체를 무조건 바꾸거나 순환 별칭을 만들지 않는다.
5. `build/`에서 빌드·테스트하고 문서를 실제 결과와 맞춘다. 설치본과 애플리케이션 실행은 해당 검증을 실제 수행한 경우에만 별도 완료로 기록한다.

정식 계약에 없는 iconset 추출 색상을 핵심 원색으로 승격하거나, 용도 없이 숫자 단계 팔레트를 새로 추가하지 않는다.

<a id="accessibility-and-contrast-checks"></a>

## 접근성 및 대비 검사

텍스트/표면 조합의 경우 다음을 위해 도구에서 수동으로 대비를 확인합니다.

- `panelBackground*` 표면의 `title/header` 텍스트,
- 대비가 낮은 표면의 `description/caption` 텍스트,
- 흐린 오버레이에서 비활성화된 상태(`disabledColor`, `textOctonary`).

모호한 팔레트를 유지하는 아무런 알림 없이보다 의도적인 저대비 예외를 문서화하는 것을 선호합니다.

<a id="design-system-synchronization-tips"></a>

## 디자인-시스템 동기화 팁

토큰이 외부 디자인 도구와 동기화되는 경우:

- 정식 토큰 이름을 안정적으로 유지합니다.
- 이름 바꾸기를 주요 변경으로 처리합니다.
- 적어도 한 번의 릴리스 주기 동안 이전 별칭 토큰을 유지하여 지원 중단을 단계화합니다.

<a id="standard-material-tokens"></a>

## 표준 재료 토큰

`applicationWindowOpacity`는 중립 채색 불투명도 0.66이며, `applicationWindowFill`은 #141414이다. 독립 WindowMaterial은 `materialWindowFill` #0B0B0B와 `materialWindowOpacity` 0.50을 유지한다. ApplicationWindow는 그 위에 `primaryColor`를 `applicationWindowAccentOpacity`의 0.04로 균일하게 합성한다. 기본 창 배경은 방사형 강도를 0 / 0으로 지정한다. 독립 WindowMaterial과 PanelMaterial의 `materialIntenseOpacity` / `materialFaintOpacity`는 0.40 / 0.11을 유지한다. `materialDenseOpacity` / `materialGlassOpacity`는 0.75 / 0.25, `materialDenseBlur` / `materialGlassBlur`는 64 / 16 논리 px이다. 전경과 창의 불투명도는 1이다.

Figma 패널 RGB 값과 흰색 텍스트 알파 값은 8비트 반올림 없이 저장됩니다. `panelBackground04` 는 #181919 로 렌더링됩니다. `menuDivider` 는 panelBackground08 입니다. `contextMenuDivider` 는 30% 흰색입니다. 아이콘 이름 nodesTest 와 wechat 는 대소문자를 구분하는 리소스 파일 이름으로 정규화됩니다. [감사](figma-parity.md)를 참조하세요.

메뉴 재료 토큰: `contextMenuOpacity = 0.12`, `contextMenuBlur = materialDenseBlur`(64px), `contextMenuAccentStrength = 0.08`. 이는 현재 사용자가 요청한 유리 처리입니다. 원래 Figma 재료 밀도/유리 값은 다른 소비자를 위해 유지됩니다.

<a id="mobile-tab-typography-and-colors"></a>

## 모바일 탭 타이포그래피 및 색상

`FontPolicy.systemFamily` 는 애플리케이션 Pretendard 정책을 변경하지 않고 `QFontDatabase::systemFont(GeneralFont)` 를 보고합니다. MobileTab 는 iOS 프레젠테이션에 이를 사용합니다. Android 는 확립된 대체 경로 정책을 사용하여 사용 가능한 Roboto 를 사용합니다. `mobileTabAndroidSurface`, `mobileTabAndroidIndicator`, `mobileTabAndroidSelectedText`, `mobileTabAndroidText` 및 `mobileTabAndroidBadge` 는 Figma 재료 3 블루 어두운 색상을 유지합니다. [탭](components/navigation/Tabs.md)를 참조하세요.
