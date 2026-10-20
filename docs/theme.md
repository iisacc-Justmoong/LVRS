<a id="theme"></a>

# 테마

위치: `src/qml/Theme.qml`

`Theme` 는 싱글턴 를 위한 전역 디자인 토큰 LVRS QML 컴포넌트입니다. 모든 플랫폼은 간격, 반지름, 컨트롤 크기, 아이콘, 타이포그래피, 줄 간격에 대해 동일한 작성된 논리적 크기를 사용합니다. iOS 와 Android 는 자동 크기 승수 추가를 하지 않습니다. 본문은 `13px / 13px` 로 유지됩니다.

<a id="token-groups"></a>

## 토큰 그룹

- 타이포그래피: 글꼴 모음 해상도 및 텍스트 크기/무게/스타일 이름 토큰.
- 표면: 창과 캔버스, 입력, 제어, 패널 등 역할별 12개 표면. 번호형 `panelBackground01..12`는 호환 별칭이다.
- 의미 색상: `primary`/`accent`, `success`, `warning`, `danger`.
- 강조 색상: 지정한 Figma 견본 10개가 단일 색상 기준이다. 이전 팔레트 7색은 정식 속성의 별칭이며 Gray는 중립색으로 보존한다. `accentPaletteTokens` 386개는 이름·순서·문자열 스키마를 유지하고 해당 악센트 값만 교체한다.
- 측정 항목: 간격, 반경, 컨트롤 크기, 대화 상자 크기 및 상호 작용 타이밍.

<a id="figma-theme-contract"></a>

## Figma 테마 계약

정식 계약은 [LVRS Figma](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc/LVRS)의 저장 상태를 기록한 `tests/fixtures/figma-theme-contract.json`이다. 현재 계약은 색상 41개, 타이포그래피 36개, 텍스트 스타일 7개이다. 2026-10-19에는 사용자가 지정한 `1477:30..38`, `1477:40`의 10색으로 악센트를 교체하고 색상 변수 41개를 재조회하였다. 기존 변수 ID와 이름, 타이포그래피 및 스타일은 유지한다. 정식 10색의 원본 RGBA와 노드 연결은 `tests/fixtures/accent-palette.json`에 기록한다. HEX 표시는 읽기 위한 반올림값이며 QML 구현과 정밀도 검사는 원본 부동소수점 채널을 사용한다.

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

`ApplicationWindow.primaryColor`를 앱 루트에 설정한다. 예를 들어 `primaryColor: LV.Theme.accentPurple`이다. 기본값 `Theme.defaultPrimary`는 `Theme.accentBlue`이며 HEX 표시는 `#1582FF`이다. 앱이 명시한 임의의 색상과 알파도 계속 지원한다. 변경은 같은 QML 엔진의 기존 컨트롤, 새로 로드된 콘텐츠 및 별도 색상을 지정하지 않은 보조 창에 전파된다. [ApplicationWindow](components/app/ApplicationWindow.md#app-primary-color)를 참조한다.

| 토큰 | 기본값과 계산 규칙 |
| --- | --- |
| `primary`, `accent`, `defaultPrimary` | 기본 `accentBlue`; 앱에서 `primaryColor` 변경 가능 |
| `success`, `warning`, `danger` | 고정 `accentGreen`, `accentYellow`, `accentRed` |
| `accentTint` | 현재 Primary RGB와 원본 알파 × `31/255` |
| `primaryOverlay`, `accentOverlay` | 현재 Primary RGB와 원본 알파 × `64/255` |
| `accentMuted` | `surfaceInteractive` 위에 Primary를 25% 합성 |
| `accentDetail`, `alertActionPrimary` | 현재 Primary |
| `alertIconSurface`, `alertIconBorder` | 같은 표면 위에 Primary를 각각 16%, 36% 합성 |
| `dangerTint`, `dangerOverlay` | 고정 Red RGB와 알파 `31/255`, `89/255` |

이름 있는 악센트 원색과 상태색은 앱의 가변 Primary와 독립적이다. 명시적인 컨트롤별 색상 재정의도 유지한다. CheckBox는 기본 Blue일 때 새 팔레트로 갱신한 SVG를 사용하며, 사용자 지정 색상에서는 해당 색상과 체크 표시를 직접 그린다. VisualCatalog의 앱 강조색 선택과 ColorPicker의 기본 최근 색상은 10개 정식 속성을 사용한다.

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

창 기초는 `window`이며 `Panel/window`와 대응한다. 기본 렌더링 색상은 `#141414`이다. `applicationWindowFill`은 이 `window` 토큰을 참조한다. ApplicationWindow는 이 바탕색 95%와 앱 강조색 5%를 혼합하고 미세 단색 입자를 더한 불투명 표면을 사용한다. 패널은 밝기 번호보다 실제 용도를 나타내는 이름을 우선한다. 아래 호환 별칭은 기존 색상값과 소비자의 동작을 유지하며 각 의미 역할을 가리킨다.

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
| `alertActionPrimary` | `#1582FF` |기본 버튼|
| `alertActionBorder` | `#596168` |보조 아웃라인|
| `alertDivider` | `#363B3F` |콘텐츠/작업 구분 기호|
| `alertIconSurface` | `#223347` |아이콘 프레임 채우기|
| `alertIconBorder` | `#1E4673` |아이콘 프레임 가장자리|

폐기 텍스트는 기존 `danger` 토큰(`#FF4646`)을 사용합니다. 참조
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

디렉토리는 하나의 평평한 레이어에 2,846개 정식 아이콘과 14개 제어 변형으로 구성된 아이콘세트 스냅샷의 전체 내용을 포함합니다. [에서](figma-iconset-import.md) 이름 지정 및 확인을 참조하세요. 모든 제공된 SVG 아이콘은 상대적 루트 차원, 중앙 정렬된 사각형 뷰 박스, `xMidYMid meet` 비율 보존을 사용합니다. 가시적인 아트워크는 소비되는 컨트롤의 크기가 변경되더라도 중앙에 유지되며, 기본 논리 아이콘 크기는 18 픽셀로 유지됩니다.

규칙:
- 빈 입력은 빈 문자열을 반환합니다.
- 전체 리소스 경로(`:/`)는 있는 그대로 반환됩니다.
- 생략시 `.svg`가 추가됩니다.
- 레거시 논리적 별칭은 제공된 아이콘 파일 이름(예: `projectStructure -> generalprojectStructure`, `add -> generaladd`, `viewMoreSymbolicDefault -> generalmoreHorizontal`, `panDownSymbolic* -> generalchevronDown*`)으로 정규화됩니다.
- 그룹 스타일 이름은 조회 전에 평면화됩니다(예: `general/projectStructure -> generalprojectStructure.svg`).

<a id="accent-palette-tokens"></a>

## 액센트 원색 토큰

2026-10-19 지정한 Figma 견본을 단일 기준으로 사용한다. 모든 원색은 불투명하다. `Theme.qml`은 각 견본의 RGBA를 `Qt.rgba`로 보존하며 아래 HEX는 8비트 표시값이다.

| QML 속성 | HEX 표시 | Figma 원본 노드 |
| --- | --- | --- |
| `accentBlue` | `#1582FF` | `1477:30` |
| `accentRed` | `#FF4646` | `1477:31` |
| `accentGreen` | `#66FF48` | `1477:32` |
| `accentYellow` | `#FFD427` | `1477:33` |
| `accentPurple` | `#874AFF` | `1477:34` |
| `accentRose` | `#F94471` | `1477:36` |
| `accentOrange` | `#FF7A27` | `1477:35` |
| `accentBrown` | `#432318` | `1477:37` |
| `accentCyan` | `#21D3FF` | `1477:38` |
| `accentTeal` | `#00C7DD` | `1477:40` |

기존 팔레트 속성은 같은 정식 색상을 가리킨다. `accentPaletteGray`는 중립색 `#8C8C8C`로 유지한다.

| 기존 속성 | 정식 속성 |
| --- | --- |
| `accentPaletteBlue` | `accentBlue` |
| `accentPalettePurple` | `accentPurple` |
| `accentPalettePink` | `accentRose` |
| `accentPaletteRed` | `accentRed` |
| `accentPaletteOrange` | `accentOrange` |
| `accentPaletteYellow` | `accentYellow` |
| `accentPaletteGreen` | `accentGreen` |

`accentOrangeMuted`와 `accentBrownMuted`도 각각 `accentOrange`와 `accentBrown`의 호환 별칭이다. `accentBlueMuted`는 새 Blue의 25% 표면 합성색이다. 기존 `accentPaletteTokens` 386개의 이름·순서와 `{ name: string, color: string }` 스키마는 유지하며, 구형 악센트 및 그에 연결된 합성색 문자열은 새 값으로 교체한다. 나머지 아트워크 색상과 중립색은 유지한다.

Figma `Compatibility Icon Colors`의 기존 변수 1,548개는 숨김 상태와 ID를 보존한다. 연결된 원색과 알파 stop은 새 악센트를 사용한다. 별도 숫자 단계나 생성된 명도 팔레트는 도입하지 않는다. [상세 계약](figma-theme-tokens.md#ten-authored-accent-colors)을 참조한다.

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

색상 변경의 검증은 소스 빌드, 저장된 Figma 계약과 실제 QML 색상 비교, 10색 렌더링 픽셀 비교, 호환 별칭 및 앱 Primary 변경, 아이콘 해시·도형 보존 검사로 구성한다. `LVRSTests_accent_palette`, `LVRSTests_figma_parity`, `LVRSTests_primary_color`, `LVRSTests_svg_manager`, `LVRSTests_colorpicker`가 해당 계약을 검사한다. 실행 기록은 `build/accent-palette-20261019/`에 보관한다.

설치본과 소비자 앱은 별도 배포 범위이다. 이번 변경에서 `lvrs install`은 사용자가 직접 수행한다.

최종 macOS 소스 빌드에 성공하였으며, 마지막 보조 리소스 변경 이후 새로 실행한 관련 CTest 12/12 대상이 실패 없이 통과하였다(64.29초). [검증 기록](../build/accent-palette-20261019/verification.json)에 대상별 결과와 offscreen 조건의 개별 생략을 구분하여 기록한다.

개별 조건부 검사 6개는 네이티브 RHI·호버 전달·선택적 캡처 조건에 따라 생략되었으며, 전체 Qt 기록에 별도로 보존한다.
