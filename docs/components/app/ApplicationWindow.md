# ApplicationWindow

위치: `src/qml/ApplicationWindow.qml`

`ApplicationWindow` 는 LVRS 의 루트 쉘로, 적응형 네비게이션 레이아웃, 렌더/ 런타임 와이어링, 그리고 글로벌 이벤트 브리징을 결합합니다. `ApplicationWindow` 는 이제 표준 하위 소비 측 부트스트랩 계약을 포함하므로, 소비자 앱 루트는 `LV.AppBootstrapWindow` 를 거치지 않고 직접 마운트할 수 있습니다.

<a id="purpose"></a>

## 목적

- 자체 플랫폼/크기 등급 및 적응형 비계 상태.
- 백엔드 기반 렌더링 정책(`RenderQuality`)을 루트 레이어 동작에 연결합니다.
- 선택적으로 런타임 리스너 및 백엔드 사용자 이벤트 미러를 자동 시작합니다.
- 페이지 스택 호스트(`PageRouter`) 및 적응형 탐색 대리자를 제공합니다.
- 공유 LVRS 테마에 앱의 기본 악센트를 적용합니다.

<a id="app-primary-color"></a>

## 앱 기본 색상

앱 루트의 `primaryColor`를 브랜드 기본 버튼, 선택 강조 표시, 슬라이더, 확인된 컨트롤, 링크, 탐색 및 경고 작업으로 설정합니다.

```qml
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: app
    visible: true
    primaryColor: "#A571E6"

    LV.LabelButton { text: "Continue" }
}
```

초기 대체 경로는 LVRS 파란색, `Theme.defaultPrimary`(`#0A84FF`)입니다. 입력은 또한 QML 색상 바인딩 또는 `QQmlApplicationEngine::setInitialProperties()`의 `primaryColor` 항목을 허용합니다. 런타임에서 `app.primaryColor`를 변경하면 기존 컨트롤과 나중에 생성된 컨트롤이 업데이트됩니다. 파란색을 복원하려면 `LV.Theme.defaultPrimary`를 할당하세요.

같은 QML 엔진의 창들이 테마 색상을 공유합니다. 기본 앱 루트에서 색상을 설정하면, `primaryColor` 를 생략하는 보조 창들은 현재 테마를 재설정하지 않고 이를 상속받습니다. 여러 개의 명시적인 입력은 동일한 테마를 업데이트하며, 독립적인 창 팔레트를 설정하지 않습니다. 별도의 QML 엔진들은 별도의 테마를 유지합니다. `AppBootstrapWindow` 와 `AppShell` 도 입력을 상속받습니다.

명시적 컨트롤 색상 재정의는 계속 적용됩니다. 상태 색상(`danger`, `warning`, `success`), 중립 표면 및 명명된 아이콘 팔레트 색상은 기존 의미를 유지합니다. 창의 Qt 빠른 컨트롤 `palette.highlight` 및 `palette.link`도 공유 기본 색상을 따릅니다.

<a id="startup-sequence"></a>

## 시동 순서

완료 시 주요 흐름은 다음과 같습니다.

1. `FontPolicy.enforceApplicationFallback()`
2. `autoApplyDeviceTierPreset == true`인 경우 옵션 `RenderQuality.applyDeviceTierPreset(...)`
3. `RenderQuality.applyWindow(windowRoot)`
4. `SvgManager.ensureMinimumScale(effectiveSupersampleScale)`
5. 옵션 런타임 부착(`autoAttachRuntimeEvents`)
6. 선택적 백엔드 후크(`autoHookBackendUserEvents`)
7. 네이티브 창 스타일 + 모바일 디스플레이 적용 범위 재정의 새로 고침

<a id="core-property-groups"></a>

## 핵심 속성 그룹

<a id="platform-and-sizing"></a>

### 플랫폼 및 크기

- `platform`, `isMobilePlatform`, `isDesktopPlatform`
- `backendRuntimeProfile`, `canonicalPlatform`
- `layoutClassWidth`, `layoutClassHeight`, `widthClass`, `heightClass`, `isCompact`, `isExpanded`
- `desktopMinWidth/Height`, `mobileMinWidth/Height`
- `useBackendMobileScale`, `mobileViewScale`, `effectiveMobileViewScale`
- `usePlatformSafeMargin`, `safeMargin`
- `mobileSystemSafeLeftInset/TopInset/RightInset/BottomInset`
- `mobileSystemSafeAreaResolved`, `mobileSystemSafeAreaBounds`
- `layoutSafeLeftInset/TopInset/RightInset/BottomInset`
- `renderSurfaceBounds`, `layoutSafeAreaBounds`

<a id="window-and-platform-overrides"></a>

### 창 및 플랫폼 재정의

- `primaryColor`(앱 액센트, 초기에는 `Theme.defaultPrimary`, `#0A84FF`)
- `windowColor`
- `windowBackgroundOpacity`(기본값 0.50, 배경 채우기만)
- `backgroundBlurEnabled`(네이티브 백엔드가 지원하면 기본적으로 활성화된다)
- read-only `backgroundBlurSupported`, `backgroundBlurActive`
- `forceNativeDarkTitleBar`
- `nativeTitleBarHeight`: 선택적으로 활성화하는 macOS 제목 표시줄 행의 높이이며 논리 픽셀 단위이다. `0`는 네이티브 기본값을 유지한다.
- `nativeTitleBarLeftMargin`: 네이티브 버튼 그룹의 시작 여백이며 기본값은 `Theme.gap12`이다.
- `nativeTitleBarControlsRect` (읽기 전용): 창 콘텐츠 좌표의 실제 네이티브 버튼 경계. 비활성화되거나 지원되지 않거나 전체 화면인 경우 비어 있습니다.
- `solidChrome`
- 시스템 크롬 상호작용:
  - `windowChromeInteractionsEnabled`
  - `windowDragHandleEnabled`, `windowDragHandleHeight`
  - `windowDragHandleTopMargin/LeftMargin/RightMargin`
  - `windowDragExclusionItems`
  - `windowResizeHandlesEnabled`, `windowResizeEdges`
  - `windowResizeBorderThickness`, `windowResizeCornerSize`
  - `windowChromeInteractionZ`
  - read-only `windowChromeInteractionLayer`, `windowDragHandleItem`
- 프로필 기반 모바일 정책 도우미:
  - `runtimeEventsAutoAttachRecommended`
  - `mobileSystemWindowDelegationRecommended`
  - `mobileSystemInsetsDelegationRecommended`
  - `mobileDisplayCoverageOverrideRecommended`
  - `mobileFullscreenVisibilityRecommended`
  - `mobileFullscreenGeometryHintRecommended`
- OS-위임 기본값:
  - `delegateMobileWindowingToSystem`
  - `delegateMobileInsetsToSystem`
- 모바일 범위 재정의:
  - `forceFullWindowAreaOnMobile`(모바일에서는 기본값이 `true`이므로 렌더 표면이 풀 블리드 상태로 유지됨)
  - `mobileDisplayCoverageOverrideEnabled`(프로파일 기반 기능, `delegateMobileWindowingToSystem=true` 동안 마스크됨)
  - `mobileFullscreenVisibilityOverride`(프로필 기반 기능, `delegateMobileWindowingToSystem=true` 동안 마스크됨, iOS에서는 상태 표시줄을 숨기는 대신 가장자리 간 적용 범위를 최대화합니다)
  - `mobileFullscreenGeometryHintOverride`(프로파일 기반 기능, `delegateMobileWindowingToSystem=true` 동안 마스크됨)
  - `mobileOversizedHeightEnabled`(기본값 `false`, 명시적인 대형 표면 해결 방법에만 선택)
  - `mobileOversizedHeight`
  - `mobileLayoutHeightHint`
  - `mobileOversizedHeightActive`
  - `mobileLayoutViewportHeight`
  - `mobileTopMarginFill`, `mobileBottomMarginFill`

<a id="runtime-and-event-bridge"></a>

### 런타임 및 이벤트 브리지

- `globalEventListenersEnabled`(기본값 `false`)
- `autoAttachRuntimeEvents`(기본값은 `globalEventListenersEnabled`를 따르며 재고 런타임 프로필은 `runtimeEventsAutoAttachRecommended=false`를 유지함)
- `autoHookBackendUserEvents`(기본값 `false`)
- `lastGlobalPressedEventData`, `lastGlobalContextEventData`
- 신호: `globalPressedEvent(...)`, `globalContextEvent(...)`

<a id="render-quality-bridge"></a>

### 렌더링 품질 브리지

- `inactiveRenderDowngradeEnabled`
- `inactiveRenderMsaaSamples`
- `autoApplyDeviceTierPreset`
- `forcedDeviceTierPreset`
- `effectiveSupersampleScale`, `sceneSupersamplingActive`
- 내부 슈퍼샘플 호스트는 백엔드 확인 텍스처 크기 조정 및 밉맵 정책을 사용합니다.

현재 구현의 기본 런타임-직접 품질 프로필:

- `mobileViewScale: 1.0`(기본 경로에서 불필요한 크기 조절 컴포지션 흐림 방지)
- `inactiveRenderDowngradeEnabled: false`
- `inactiveRenderMsaaSamples: 8`
- `autoApplyDeviceTierPreset: false`
- `forcedDeviceTierPreset: -1`(옵트인 장치 계층 애플리케이션용으로 예약됨)

현재 구현의 기본 모바일 크기 계약:

- `Theme`는 데스크탑, iOS 및 Android에서 동일한 작성된 메트릭 및 입력 체계 값을 사용합니다. 두 토큰 규모 요소는 모두 `1.0`입니다. 본체 크기와 라인 높이는 `13px`로 유지됩니다.
- `usePlatformSafeMargin`는 iOS 및 Android에서 기본값으로 `true`로 설정되고, `safeMargin`는 각 측면에서 고정된 `16` 논리 픽셀로 기본값으로 설정됩니다.
- 렌더 표면은 모바일에서 풀 블리드 상태로 유지됩니다. `ApplicationWindow`는 자동 `contentItem` 안전 영역 패딩을 기본 모바일 경로의 `0`로 되돌립니다.
- 스캐폴드 콘텐츠도 기본적으로 풀 블리드입니다. `safeMargin`는 더 이상 내부 콘텐츠 호스트를 자동으로 제한하지 않습니다.
- `layoutSafeAreaBounds`는 이제 도우미 직사각형일 뿐입니다. `safeMargin`에서 파생된 고정 레이아웃 삽입을 설명하지만 LVRS는 해당 상자 안에 앱 콘텐츠를 자동으로 배치하지 않습니다.
- `mobileSystemSafeLeftInset/TopInset/RightInset/BottomInset` 및 `mobileSystemSafeAreaBounds`는 실제 플랫폼 안전 영역 마진을 노출하므로 하위 소비 측 앱이 예약할 지역을 결정할 수 있습니다.
- `mobileViewScale`의 기본값은 `1.0`입니다. 기본 경로에는 테마 또는 구성 승수가 추가되지 않습니다.

현재 구현의 기본 앱 루트 부트스트랩 프로필:

- `navigationEnabled: false`
- `useInternalPageStack: true`
- `internalRouterRegisterAsGlobalNavigator: true`
- `mobileOversizedHeightEnabled: false`
- `initialRoutePath: "/"`
- `pageInitialPath`는 하위 소비 측 앱이 직접 재정의할 때까지 `initialRoutePath`를 따릅니다.

<a id="adaptive-scaffold-and-page-stack-api"></a>

### 적응형 스캐폴드 및 페이지 스택 API

내부 비계에 대한 별칭은 다음과 같습니다.

- 네비게이션 모델: `navItems`, `navIndex`, `navigationEnabled`
- 탐색 아이콘 크기: `navigationIconSize`(기본값은 `Theme.iconSm`)
- 레이아웃 정책: `scaffoldLayoutMode`, `scaffoldLayoutPlatform`, `scaffoldForceDesktopOnLargeMobile`, `scaffoldMobileDesktopMinWidth`
- 탐색 모드 정책: `scaffoldPreferBottomNavigation`, `scaffoldBottomNavigationMaxItems`, `scaffoldNavRailMaxWidthRatio`, `scaffoldDrawerMarginSafety`
- 페이지 스택: `initialRoutePath`, `pageRoutes`, `pageInitialPath`, `useInternalPageStack`, `activePageRouter`, `internalPageStackEnabled`
- 대화형 전이 브리지: `pageTransitionController`

적응형 내비게이션 위임자는  `icon` ,  `iconName` , 또는  `symbol` 텍스트 글리프를 정사각형  `navigationIconSize` 프레임에 렌더링합니다. 기본값은  `Theme.iconSm` 를 따르며 (데스크톱에서는  `18 x 18` , 모바일에서는  `36 x 36` ) 따라서 레일, 드래워, 그리고 하단 내비게이션 아이콘은 동일한 컴팩트 아이콘 계약을 따르면서도 명시적으로 오버라이드할 수 있습니다.

적응형 상태 출력:

- `adaptiveLayoutProfile`
- `adaptiveNavigationMode`
- `adaptiveMobileLayout`, `adaptiveDesktopLayout`
- `adaptiveRailNavigation`, `adaptiveDrawerNavigation`, `adaptiveBottomNavigation`

신호:

- `navActivated(index, item)`
- `adaptiveLayoutStateChanged(profile, navigationMode)`
- `pageStackNavigated(path, params)`
- `pageStackNavigationFailed(path)`

<a id="backend-adaptive-policy"></a>

### 백엔드 적응형 정책

- `useBackendAdaptivePolicy`
- `backendAdaptivePolicyOverrides`
- `backendRuntimeProfile`
- `backendAdaptivePolicyDefaults`
- `backendAdaptivePolicy`
- 런타임 프로파일 구동 적응형 키:
  - `adaptiveWideBreakpoint`, `adaptiveNavWidth`, `adaptiveNavDrawerWidth`
  - `adaptiveMobileDesktopMinWidth`, `adaptiveBottomNavigationMaxItems`
  - `adaptiveCompactSpacingBreakpoint`, `adaptiveNavRailMaxWidthRatio`, `adaptiveDrawerMarginSafety`
  - `adaptiveDrawerEnterDuration`, `adaptiveDrawerExitDuration`, `adaptiveAnimatedTransitions`
- 해결된 숫자 정책 출력:
  - `backendWideBreakpoint`, `backendNavWidth`, `backendNavDrawerWidth`
  - `backendMobileDesktopMinWidth`, `backendBottomNavigationMaxItems`
  - `backendCompactSpacingBreakpoint`, `backendNavRailMaxWidthRatio`, `backendDrawerMarginSafety`
  - `backendDrawerEnterDuration`, `backendDrawerExitDuration`
  - `backendAnimatedTransitions`

<a id="key-methods"></a>

## 주요 방법

- `matchesMedia(rule)`
- `ensureRuntimeEventsAttached()`
- `applyNativeWindowStyle()`
- `mobileCoverageTargetVisibilityForPlatform(platformName)`(iOS를 `Window.Maximized`에 매핑하고 기타 모바일 적용 범위 경로를 `Window.FullScreen`에 매핑)
- `applyMobileDisplayCoverageOverride()`(프레임워크 관리 가장자리 간 가시성, 전체 화면 기하학 및 확장된 클라이언트 영역 힌트 적용 또는 릴리스)
- `requestWindowMove()`
- `requestWindowResize(edges)`

<a id="behavior-notes"></a>

## 행동 참고 사항

- 적응형 레이아웃 전환은 유효하지 않은 1단계 전환과 진동 크기 조정을 방지하기 위해 보호됩니다.
- 고체 데스크톱 크롬은 8-영역 네이티브 우선 리사이즈 레이어를 사용하여 동일한 시스템 이동 및 `LV.Window` 와 같은 기능을 포함하며, Qt 에서 Cocoa 가 시스템 리사이즈를 거부할 때 macOS 대체 경로 를 설정합니다. 애플리케이션 소유 히트 영역이 요청 메서드를 직접 호출하는 경우 `windowDragHandleEnabled` 또는 `windowResizeHandlesEnabled` 를 `false` 로 설정하고, `windowDragExclusionItems` 를 사용하여 제목 표시줄 컨트롤을 인터랙티브하게 유지합니다.
- macOS 에서 솔리드 크롬은 AppKit 의 전체 창 배경 드래깅을 비활성화합니다. 창 이동은 구성된 LVRS 이동 핸들 또는 명시적인 `requestWindowMove()` 호출에 의해만 시작되므로, 페인팅, 텍스트 선택 및 기타 콘텐츠 드래그는 해당 항목에 의해 소유됩니다. `LVRSTests_nativewindowblur::solid_chrome_disables_background_movement` 는 초기 스타일링, 스타일 새로고침 및 Cocoa 와의 네이티브 창 재창조 후 네이티브 플래그를 확인합니다.
- 유니파이드 툴바는 `nativeTitleBarHeight: 56` 를 설정할 수 있으며, 내용을 y= 0에서 시작할 수 있습니다. 툴바 컨트롤 전에 `nativeTitleBarControlsRect.x + nativeTitleBarControlsRect.width` 를 예약한 후, 애플리케이션의 일반적인 간격을 추가합니다. `windowDragHandleHeight` 행의 동일한 높이로 설정하고 인터랙티브 툴바 항목을 제외합니다. LVRS 는 AppKit 의 실제 버튼과 그들의 네이티브 간격을 유지하며, 리사이즈/재열기 시 기하학을 업데이트하고 풀스크린 중 시스템 레이아웃을 복원합니다. 지원되지 않는 플랫폼은 빈 컨트롤 직사각형을 반환합니다. `LVRSTests_nativewindowblur::unified_titlebar_controls` 는 네이티브 버튼 중심, 타격 영역, 초기화, 리사이즈, 풀스크린 왕복 변환 왕복 변환 및 Cocoa 와 네이티브 창 재생성을 확인합니다.
- `windowMoveAttempted(started)` 및 `windowResizeAttempted(edges, started)`는 플랫폼이 각 포인터 시작 요청을 수락했는지 여부를 보고합니다.
- 적응형 스캐폴드 메트릭은 이제 대략적인 모바일/데스크톱 제품군 분할에서 추론되는 대신 OS 기준으로 `Platform.runtimeProfile()`에서 제공됩니다.
- `globalEventListenersEnabled` 및 `autoHookBackendUserEvents`는 독립적입니다. 백엔드 사용자 이벤트 미러링을 활성화해도 전역 리스너가 강제 실행되지는 않습니다.
- 런타임 연결 및 백엔드 후크에는 기능 플래그가 지정되어 있습니다. 제한된 호스트에서는 둘 다 완전히 비활성화될 수 있습니다.
- `scaffoldLayoutPlatform`는 적응형 모바일/데스크톱 정책이 해결되기 전에 `Platform.normalizeTarget()`를 통해 정규화되므로 `osx`, `ios-simulator` 및 `android-arm64`와 같은 별칭은 안전합니다.
- 표준 부트스트랩 경로 계약은 이제 `ApplicationWindow` 자체에 있으므로 하위 소비 측 프로젝트는 루트 유형을 래핑하지 않고 `QmlAppLaunchSpec::initialProperties`를 통해 `initialRoutePath`를 시드할 수 있습니다.
- 기본 쉘은 이제 런타임 직접 `RenderQuality` 경로로 기본 설정됩니다. 하위 소비 측 앱이 명시적으로 `autoApplyDeviceTierPreset`를 다시 활성화하지 않으면 자동 장치 계층 사전 설정 애플리케이션이 비활성화됩니다.
- 모바일 시스템 위임 기본값은 플랫폼을 인식합니다. Android는 여전히 OS 관리 창/삽입을 선호하는 반면, iOS는 이제 기본적으로 프레임워크 관리 전체 창 적용 범위 경로를 사용하므로 렌더링 표면이 상태 표시줄, 노치 및 홈 표시기 영역으로 확장될 수 있습니다.
- 해당 프레임워크 관리 적용 범위 경로가 활성화되면 `ApplicationWindow`는 이제 `MaximizeUsingFullscreenGeometryHint`를 통해 기본 `QWindow`를 푸시하고, Qt 6.9+에서는 `ExpandedClientAreaHint`/`NoTitleBarBackgroundHint`도 푸시합니다.
- iOS 적용 범위는 이제 `MaximizeUsingFullscreenGeometryHint`와 함께 `Window.Maximized`를 사용하여 시스템 상태 표시기를 계속 표시하는 동시에 렌더링 표면이 상태 표시줄, 노치 및 홈 표시기 영역에 도달하도록 합니다.
- Android는 여전히 `mobileDisplayCoverageOverrideEnabled`, `mobileFullscreenVisibilityOverride` 및 `mobileFullscreenGeometryHintOverride`를 통해 레거시 전체 화면 적용 범위 경로를 노출합니다. `delegateMobileWindowingToSystem`를 비활성화하는 것은 하위 소비 측 앱이 의도적으로 해당 경로를 다시 원할 때 첫 번째 단계입니다.
- `WindowSafeAreaObserver`는 루트 편의 속성을 통해 라우팅하지 않고 플랫폼 안전 영역 여백에 직접 액세스하려는 하위 소비 측 앱을 위한 하위 수준 API입니다.
- 모바일 안전 영역 채우기는 기본 레이아웃 경계를 메타데이터로만 사용할 수 있도록 유지합니다. 앱에 이전의 대형 표면 해결 방법이 명시적으로 필요한 경우에만 `mobileOversizedHeightEnabled`를 활성화하세요.
- 너무 큰 나머지 부분은 레이아웃이 아닌 위쪽/아래쪽 여백 채우기로 처리되고 `windowColor`로 칠해집니다.
- 기본 모바일 크기 조정은 데스크탑과 동일한 테마 토큰을 사용합니다. 하위 소비 측 앱은 개별 구성요소 측정항목을 재정의하거나 명시적으로 `mobileViewScale`를 선택할 수 있습니다.
- `pageTransitionController`는 항상 `activePageRouter`가 현재 확인하는 라우터를 따르므로 셸 수준 제스처 드라이버는 라우터 조회를 반복할 필요가 없습니다.

<a id="usage"></a>

## 사용법

```qml
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    visible: true
    width: 430
    height: 932

    title: "MyApp"
    pageRoutes: [
        { path: "/", component: homePage }
    ]

    Component {
        id: homePage
        Item {}
    }
}
```

<a id="default-background-material"></a>

## 기본 배경 자료

표준 `background` 은 WindowMaterial 으로, 50% 투명도에서 균일한 검정색에 가까운 채색 ( #0B0B0B ) 을 가지며 그라데이션이 없습니다. macOS 에서 활성 네이티브 안개 낀 배경판은 이 창 뒤의 다른 창과 데스크톱도 흐리게 합니다. 창/전경 투명도는 1 로 유지되며, `windowBackgroundOpacity` 는 배경 채색만 제어합니다. `windowColor` 는 테마로 기본값입니다. materialWindowFill ( #0B0B0B ) 은 primaryColor 와 독립적으로 명시적으로 오버라이드할 수 있습니다. 버튼과 선택지는 앱 악센트를 유지합니다. 64px 재료 안개 낀 효과는 캡처된 배경판에 여전히 사용 가능합니다. 배경은 풀 블리드로 유지되며, 외부 테두리/모서리/높이는 네이티브 창 크로스에 위임됩니다. 소비자는 `background` 를 일반적으로 오버라이드할 수 있습니다. [재료](../surfaces/Materials.md)를 참조하세요.

<a id="shared-motion"></a>

## 공유 모션

서랍은 한계가 설정된 리바운드로 들어갑니다. 내비게이션 컨트롤은 공유된 누르기 응답을 사용합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

`materialBackdropSource`는 형제 팝업 오버레이 없이 앱 콘텐츠/슈퍼샘플링 호스트를 노출합니다. ContextMenu는 이 항목을 창 배경과 함께 캡처하므로 메뉴 자체를 캡처하지 않고도 메뉴 뒤의 콘텐츠가 흐려집니다. 이렇게 하면 모션 및 크기 조정 중에 캡처 안전성이 유지됩니다.
