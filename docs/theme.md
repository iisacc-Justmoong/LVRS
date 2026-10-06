<a id="theme"></a>

# 테마

위치: `src/qml/Theme.qml`

`Theme` 는 싱글턴 를 위한 전역 디자인 토큰 LVRS QML 컴포넌트입니다. 모든 플랫폼은 간격, 반지름, 컨트롤 크기, 아이콘, 타이포그래피, 줄 간격에 대해 동일한 작성된 논리적 크기를 사용합니다. iOS 와 Android 는 자동 크기 승수 추가를 하지 않습니다. 본문은 `13px / 13px` 로 유지됩니다.

<a id="token-groups"></a>

## 토큰 그룹

- 타이포그래피: 글꼴 모음 해상도 및 텍스트 크기/무게/스타일 이름 토큰.
- 표면: 창 색상 및 12단계 패널 배경 크기.
- 의미 색상: `primary`/`accent`, `success`, `warning`, `danger`.
- 강조 팔레트: iconset 파생 색상 토큰 세트(`accentPaletteTokens`).
- 측정 항목: 간격, 반경, 컨트롤 크기, 대화 상자 크기 및 상호 작용 타이밍.

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
- 텍스트 크기와 일치하는 줄 높이는 모든 타겟에서 `26`,2 `22`, `17`,2 `15`, `13`, `12`, 그리고 `11` 입니다.
- `FontPolicy`는 이러한 승인된 크기만 인식합니다. 이전 `2x` 및 `1.25x` 별칭은 거부됩니다. `22px`는 Bold Title2에 속하고 `26px`는 Bold Title에 속하며 이중 캡션이나 본문 별칭은 없습니다.
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

창 기초:
- `window: "#141414"`

패널 배경 스케일(어두움, 낮은 채도, 12 단계):
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

파생된 표면 별칭:
- `windowAlt -> panelBackground03`
- `subSurface -> panelBackground04`
- `surfaceSolid -> panelBackground05`
- `surfaceAlt -> panelBackground06`
- `surfaceGhost -> panelBackground02`

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

## 16진수 형식 규칙

- 불투명 표면 토큰은 6숫자 16진수를 사용합니다.
- 알파가 필요한 토큰은 8-digit 헥스(예: `overlayBackdrop`, 텍스트 불투명도 토큰) 또는 `Qt.rgba`를 사용하여 재료 채우기에 대한 정확한 Figma 불투명도 비율을 나타냅니다.

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

## 액센트 팔레트 토큰

2 레이어가 있습니다:

- 안정적인 이름의 아이콘 색상(`accentBlue`, `accentRed`, `accentGreen` 등).
- 추출된 팔레트 목록: `accentPaletteTokens`.

`accentPaletteTokens` 아이템 스키마:
- `{ name: string, color: string }`

개수는 다음과 같이 사용할 수 있습니다.
- `accentPaletteTokenCount`

추출된 팔레트는 `resources/iconset/*.svg` 채우기/획 색상에서 생성됩니다.

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

새 토큰을 추가할 때 다음 순서를 적용하세요.

1. `Theme.qml`에 기본 토큰을 추가합니다.
2. 토큰에 도메인 의미(예: 경고 표면)가 있는 경우 의미 별칭을 추가합니다.
3. 하드 코딩된 색상 대신 의미 체계 별칭을 사용하도록 구성 요소 기본값을 업데이트합니다.
4. 가치, 목적, 소비 목표로 문서를 업데이트하세요.

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

`applicationWindowOpacity` 는 0.50 입니다. ApplicationWindow 와 독립형 WindowMaterial 에 대해 동일합니다. 그들의 채움은 `materialWindowFill` ( #0B0B0B )을 전체 표면에 사용하며, Primary 와 무관하게 그리고 방사형 그라데이션 없이 적용됩니다. 명시적인 windowColor /material 색상 오버라이드는 계속 지원됩니다. 독립형 `materialDenseOpacity` / `materialGlassOpacity` 는 0.75 / 0.25 입니다. `materialDenseBlur` / `materialGlassBlur` 는 64 / 16 논리 픽셀입니다. `materialIntenseOpacity` / `materialFaintOpacity` 는 0.40 / 0.11입니다. `materialTint` 은 #141414 입니다. `materialPanelRadius` / `materialWindowRadius` 는 12 / 16 논리 픽셀이며, `materialDenseEdge` / `materialGlassEdge` 는 12% / 22%에서 흰색을 사용합니다. 재료 강조는 `Theme.primary` 를 상속하며 기본 창 채색은 검정에 가깝게 유지됩니다.

Figma 패널 RGB 값과 흰색 텍스트 알파 값은 8비트 반올림 없이 저장됩니다. `panelBackground04` 는 #181919 로 렌더링됩니다. `menuDivider` 는 panelBackground08 입니다. `contextMenuDivider` 는 30% 흰색입니다. 아이콘 이름 nodesTest 와 wechat 는 대소문자를 구분하는 리소스 파일 이름으로 정규화됩니다. [감사](figma-parity.md)를 참조하세요.

메뉴 재료 토큰: `contextMenuOpacity = 0.12`, `contextMenuBlur = materialDenseBlur`(64px), `contextMenuAccentStrength = 0.08`. 이는 현재 사용자가 요청한 유리 처리입니다. 원래 Figma 재료 밀도/유리 값은 다른 소비자를 위해 유지됩니다.

<a id="mobile-tab-typography-and-colors"></a>

## 모바일 탭 타이포그래피 및 색상

`FontPolicy.systemFamily` 는 애플리케이션 Pretendard 정책을 변경하지 않고 `QFontDatabase::systemFont(GeneralFont)` 를 보고합니다. MobileTab 는 iOS 프레젠테이션에 이를 사용합니다. Android 는 확립된 대체 경로 정책을 사용하여 사용 가능한 Roboto 를 사용합니다. `mobileTabAndroidSurface`, `mobileTabAndroidIndicator`, `mobileTabAndroidSelectedText`, `mobileTabAndroidText` 및 `mobileTabAndroidBadge` 는 Figma 재료 3 블루 어두운 색상을 유지합니다. [탭](components/navigation/Tabs.md)를 참조하세요.
