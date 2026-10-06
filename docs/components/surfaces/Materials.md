<a id="standard-materials"></a>

# 표준 재료

<a id="purpose"></a>

## 목적

`WindowMaterial` 와 `PanelMaterial` 는 Figma Material 컴포넌트에서 시작되었습니다. 2026-09-12 사용자 수정본은 독립적으로든 `ApplicationWindow.background` 내에서도든 Window 그라데이션을 일정한 거의 검은색 채움 ( #0B0B0B ) 으로 대체하며, 50% 불투명도를 가집니다. 창 뒤쪽 네이티브 블러는 macOS 에서 계속 활성화됩니다. ContextMenu 와 메뉴는 더 밝은 안개 낀 코팅을 유지하며, 툴팁과 팝오버는 Glass 25를 유지합니다.

|재질|채우기|배경 흐림|기본 사용|
| --- | --- | --- | --- |
| ApplicationWindow / WindowMaterial |일정한 #0B0B0B 50%에 그라데이션 없음|64 논리적 픽셀; 네이티브 안개 낀 배경 ApplicationWindow|메인, 보조 및 독립 창 표면|
|ContextMenu / 메뉴|캡처된 배경 위에 12%의 창 채우기 색상|64 논리 px|컴팩트 및 일반 메뉴|
|조밀한 75 MaterialSurface / PanelMaterial|#141414 at 75%|64 논리 px|명시적으로 조밀한 패널|
|유리 25|#141414 at 25%|16 논리 px|도구 설명, 팝오버, 기본 패널|

WindowMaterial 는 테마를 사용합니다. materialWindowFill ( #0B0B0B ) 은 주색상과 독립적으로 전체 표면 채움으로 사용되며, 두 가지 반지름 강도를 비활성화합니다. 크기 조정 후에도 색상 변경 중에도 일관성을 유지합니다. 상속된 밀집75 설정은 여전히 64px 블러 및 밀집 테두리 기본값을 공급하며, 명시적 채움 불투명도는 50%입니다. 배경 콘텐츠는 흐려지거나 페이드되지 않습니다.

PanelMaterial는 40% 및 11% 강도에서 8개의 기본 색상 그라데이션을 유지합니다. 아래 표에서는 해당 패널 처리에 대해 설명합니다. 메뉴는 이러한 강도를 일반 강도의 8%로 명시적으로 재정의합니다.

|액센트|센터 바인딩|X/Y 반경|강도|
| --- | --- | --- | --- |
|중앙 강렬한|센터 −129 / −28| 200 / 200 | 40% |
|중앙 희미함|중앙 +136 / +62| 180 / 180 | 11% |
|왼쪽 상단|왼쪽 +48, 상단 +32| 200 / 200 | 40% |
|오른쪽 상단|오른쪽 −64, 상단 +40| 260 / 150 | 11% |
|왼쪽 가장자리|왼쪽, 수직 중앙| 140 / 300 | 11% |
|오른쪽 가장자리|오른쪽, 수직 중앙| 160 / 300 | 40% |
|하단 가장자리|수평 중앙, 하단| 380 / 150 | 11% |
|왼쪽 하단|왼쪽 +72, 하단 −40| 180 / 180 | 40% |

## API

`MaterialSurface`는 공유 렌더러입니다. WindowMaterial는 균일한 충진을 제공하고 PanelMaterial는 방사형 처리를 제공합니다. 둘 다 모퉁이 및 입면 기본값을 제공합니다.

- `density`: `MaterialSurface.Dense75` 또는 `MaterialSurface.Glass25`.
- `primaryColor`: 기본값은 `Theme.primary`이며, `#0A84FF`로 대체됩니다.
- `color`, `tintOpacity`: 전체 표면 색상 및 불투명도. WindowMaterial는 0.50에서 #0B0B0B로 기본 설정됩니다. PanelMaterial는 기본적으로 중성 색조로 설정됩니다.
- `blurRadius`, `blurEnabled`: 배경 확산 반경 및 스위치.
- `radius`, `borderWidth`, `borderColor`: 표면 실루엣과 가장자리.
- `intenseOpacity`, `faintOpacity`: 방사형 강도, WindowMaterial의 경우 0 / 0, PanelMaterial의 경우 0.40 / 0.11. 강도가 0이면 방사형 모양이 생성되지 않습니다.
- `backdropSource`, `backdropBackground`: 과도 표면 뒤에서 캡처할 명시적인 형제 항목입니다. 팝업은 애플리케이션 콘텐츠와 배경을 별도로 캡처하므로 오버레이가 자체 캡처에 들어가지 않습니다.
- `outlinePath`: 로컬 좌표의 선택적 SVG 경로; 도구 설명은 기존 몸체와 꼬리 윤곽선을 사용합니다.
- `effectsActive`, `captureActive`, `resolvedBackdropSource`: 읽기 전용 렌더링/캡처 진단.

<a id="usage"></a>

## 사용법

```qml
import LVRS as LV

LV.ApplicationWindow {
    primaryColor: "#A571E6"
    // 기본값은 50%의 균일한 #0B0B0B 채우기와 macOS 네이티브 배경 흐림이다.
    LV.Popover {
        id: details
        width: 320; height: 180
        contentItem: DetailsView {}
    }
}
```

배경으로 재료를 사용하고 그 위에 콘텐츠를 배치합니다. `ApplicationWindow.windowColor` 는 기본적으로 테마를 사용합니다. materialWindowFill ( #0B0B0B ) 은 전체 표면 채움을 덮을 수 있으며, `windowBackgroundOpacity` 는 50% 기본값을 덮지만 창이나 콘텐츠를 페이드하지 않습니다. 소비자는 표준 `background` 속성을 여전히 덮을 수 있습니다. 네이티브 크롬이 전체 창 외곽선을 소유하므로 기본 창 배경은 자신의 테두리, 모서리 둥글기와 높이를 비활성화합니다.

<a id="how-it-works"></a>

## 동작 원리

기존 Qt Quick Shapes 및 Qt Quick Effects 모듈은 방사형 채우기, 실루엣 마스킹, 질감 포착 및 `MultiEffect` 확산을 제공합니다. 추가 라이브러리 의존성이 없습니다. 배경/내용 포착은 확산 전에 강조색과 합성되며, 중성 톤과 윤곽선은 그 후에 그려집니다. 포착은 조상 변환을 따르며, 피드백을 방지하기 위해 재료 자체, 조상 및 후손을 거부합니다.

소프트웨어 장면 그래프는 GPU 블러 없이 표면 색상, 투명도 및 윤곽선을 유지합니다. 네이티브 RHI 렌더링 (포함 macOS   Metal )은 배경 확산을 적용합니다. 일시적 표면은 창 내 소스를 흐리게 합니다. 별도로, macOS 의 네이티브 렌더 뷰 아래에 Qt 의 `ApplicationWindow` AppKit `NSVisualEffectView` 가 설치되어 있으며, `UnderWindowBackground` , `BehindWindow` 블렌딩과 활성 안개 처리된 재료를 사용합니다. Qt 는 투명하게 지우며, 균일한 검정색에 가까운 QML 채우는 것은 50% 입니다; 네이티브 창 및 전경은 알파 1를 유지합니다. 효과가 네이티브 리사이징을 따르며 입력을 가로채지 않고, `backgroundBlurEnabled` 가 거짓일 때 제거됩니다. 크롬 업데이트는 네이티브 효과를 보존합니다.

64px   QML 확산 및 50% 채우는 것은 명시적인 LVRS 값입니다. AppKit 는 자체 네이티브 블러 커널 및 적응형 재료 톤을 제어하며, 해당 커널에 대한 공개 픽셀 반경 설정은 없습니다. 지원되지 않는 플랫폼은 불투명한 창 색상 대체 경로 를 유지하며 `backgroundBlurSupported: false` 를 보고합니다. 스크린 캡처, 개인 API 또는 추가 의존성이 필요하지 않습니다.

네이티브 참조: [Apple NSVisualEffectView](https://developer.apple.com/documentation/appkit/nsvisualeffectview), [UnderWindowBackground](https://developer.apple.com/documentation/appkit/nsvisualeffectview/material-swift.enum/underwindowbackground), [Qt 창 알파 버퍼](https://doc.qt.io/qt-6.8/qquickwindow.html#setDefaultAlphaBuffer).

Figma 참조: [Window](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=944-31), [Panel](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=944-28), [ApplicationWindow](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=997-3).

검증: `LVRSTests_nativewindowblur` 는 실제 macOS 뷰 배치, 알파, 블러 모드, 리사이징, 입력, 토글링 및 네이티브 표면 재생성 ( `QT_QPA_PLATFORM=cocoa QSG_RHI_BACKEND=metal` 로 실행) 을 확인합니다. `LVRSTests_materials` 는 900×600 와 1600×1000로 리사이즈 후 에지 색상을 덮으며, 기본값, 앱 강조 변경, 일시적 표면, 안전하지 않은 캡처 거부, 네이티브 배경 대비 감소 및 VisualCatalog 의 팝업 상호작용을 포함합니다. `LVRSTests_tooltip` 는 배치, 꼬리 및 콘텐츠 동작을 유지하며, 평범한 `QQuickWindow` 에 의해 호스팅되는 갤러리를 포함합니다. VisualCatalog 는 모든 4 공개 재료/팝오버 유형을 노출합니다.

<a id="shared-motion"></a>

## 공유 모션

MaterialSurface를 통한 색조 및 불투명도 혼합; 호스트 창은 네이티브 이동 및 크기 조정 동작을 유지합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

Figma 확산 레이어는 (0, 1) 좌표에 0블러 흰색 내장 하이라이트를 포함하며, 밀집 6%, 유리 14%입니다. `innerHighlightOpacity` 와 `innerHighlightOffsetY` 는 이 작성된 값을 노출합니다. 네이티브 RHI 렌더링은 기존 마스크를 사용하여 이동된 실루엣을 뺍니다; 커스텀 꼬리를 포함하며, 소프트웨어 렌더러는 기존 효과 없는 대체 경로 를 유지합니다.

ContextMenu 및 메뉴는 명시적 반투명 코팅이 있는 WindowMaterial를 사용합니다. 창 채우기 색상의 12%, 흐림 효과의 64px 및 일반 방사형 악센트 강도의 8%입니다. 도구 설명 및 팝오버는 PanelMaterial 유리 25를 유지합니다. [ContextMenu](../navigation/ContextMenu.md#window-derived-frosted-menu)를 참조하세요.

메뉴는 창의 `materialBackdropSource` 컨텐츠 호스트와 배경을 별도의 소스로 사용합니다. 호스트는 팝업 오버레이를 제외하여 기존 상위/하위 캡처 가드를 유지하면서 메뉴 뒤의 전경 콘텐츠가 확산되도록 허용합니다.

`backdropBaseColor` 는 기본적으로 투명합니다. 윈도우 내 메뉴는 불투명 중립 테마를 사용합니다. materialTint 를 캡처 기반으로 하여 흐릿한 합성 가 덮은 픽셀을 대체합니다; 그렇지 않으면 캡처된 윈도우 알파가 원래 날카로운 텍스트가 새어 나오게 합니다. 가시 메뉴 코팅은 12%로 유지됩니다.

검정에 가까운 채우기 변경은 2026-09-12에 macOS/Metal에서 검증했다. 53개의 LVRS CTest 사례와 9개의 네이티브 재질 검사가 통과했다. 렌더링한 채우기 샘플은 50% 알파에서 #0B0B0B를 유지했고, 900×600와 1600×1000에서 파란색·보라색 강조색을 사용했다. 설치된 83개의 QML 리소스 전체가 현재 소스와 일치했다. Society는 16개 테스트를, Dreamscapes는 5개를 통과했으며, 검정에 가까운 채우기와 앱 강조색의 별도 기대값을 포함했다. Society, Dreamscapes, VisualCatalog를 갱신한 Workspace 런타임으로 다시 빌드하고 재실행했으며, 두 앱 창을 시각적으로 확인했다.
