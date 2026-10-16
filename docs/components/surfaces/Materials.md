<a id="standard-materials"></a>

# 표준 재료

<a id="purpose"></a>

## 목적

`ApplicationWindow`는 Figma 997:3에 따라 `#141414` 66% 중립 배경 위에 `primaryColor`의 균일한 4% 오버레이를 사용한다. 기본 배경은 WindowMaterial을 재사용하되 방사형 강도를 모두 0으로 지정한다. 독립적인 `WindowMaterial`과 `PanelMaterial`은 기존 방사형 재질과 반투명 블러를 유지한다. 창 뒤쪽의 네이티브 블러는 macOS에서 활성화된다. ContextMenu와 메뉴는 더 옅은 코팅을 사용하고, 툴팁과 팝오버는 Glass 25를 사용한다.

| 재질 | 채우기 | 배경 흐림 | 기본 사용 |
| --- | --- | --- | --- |
| ApplicationWindow | #141414 66%와 강조색 4% 균일 오버레이 | 64 논리 px; macOS 네이티브 창 뒤 블러 | 기본 및 보조 창 |
| 독립 WindowMaterial | #0B0B0B 50%와 강조색 방사형 그레디언트 | 64 논리 px | 명시적인 창 재질 |
| ContextMenu / 메뉴 | 창 채색 12%와 옅은 강조색 | 64 논리 px | 컴팩트 및 일반 메뉴 |
| Dense 75 MaterialSurface / PanelMaterial | #141414 75%와 강조색 방사형 그레디언트 | 64 논리 px | 조밀한 패널 |
| Glass 25 | #141414 25%와 강조색 방사형 그레디언트 | 16 논리 px | 툴팁, 팝오버, 기본 패널 |

WindowMaterial과 PanelMaterial은 모두 `Theme.materialIntenseOpacity` / `Theme.materialFaintOpacity`의 0.40 / 0.11을 사용한다. 여덟 그레디언트는 크기 조정에 맞춰 중앙과 가장자리를 따른다. WindowMaterial의 Dense 75 설정은 64px 블러와 테두리 기본값을 공급하고, 독립 창 재질의 채색 불투명도는 별도로 50%이다. ApplicationWindow의 기본 배경은 두 방사형 강도를 0으로 재정의하고 강조색 4% 레이어를 중립 채색 위에 합성한다. 이 값은 전체 창과 전경 콘텐츠의 불투명도를 낮추지 않는다. 그레디언트가 겹치는 위치에서는 합성된 배경 알파가 50%보다 커진다.

메뉴는 그레디언트 강도를 기본값의 8%로 명시적으로 재정의한다.

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

`MaterialSurface`는 공유 렌더러이다. WindowMaterial과 PanelMaterial은 같은 방사형 처리를 사용하며 각자의 채색, 모서리와 그림자 기본값을 제공한다.

- `density`: `MaterialSurface.Dense75` 또는 `MaterialSurface.Glass25`.
- `primaryColor`: WindowMaterial은 소속 창의 `primaryColor`를 동적으로 상속한다. 창에 연결되지 않았거나 창에 해당 속성이 없으면 `Theme.primaryColor`를 사용한다. PanelMaterial의 기본값은 `Theme.primary`이다. 명시적인 재질 색상 지정은 계속 우선한다.
- `color`, `tintOpacity`: 전체 표면 색상 및 불투명도. 독립 WindowMaterial은 `materialWindowFill` / `materialWindowOpacity`의 #0B0B0B / 0.50을 사용한다. ApplicationWindow는 `applicationWindowFill` / `applicationWindowOpacity`의 #141414 / 0.66을 지정한다. PanelMaterial는 기본적으로 중성 색조로 설정됩니다.
- `blurRadius`, `blurEnabled`: 배경 확산 반경 및 스위치.
- `radius`, `borderWidth`, `borderColor`: 표면 실루엣과 가장자리.
- `intenseOpacity`, `faintOpacity`: 독립 재질의 방사형 강도이며 기본값은 0.40 / 0.11이다. ApplicationWindow 기본 배경은 0 / 0을 사용한다. 둘 다 0으로 설정하면 방사형 모양이 생성되지 않는다.
- `backdropSource`, `backdropBackground`: 과도 표면 뒤에서 캡처할 명시적인 형제 항목입니다. 팝업은 애플리케이션 콘텐츠와 배경을 별도로 캡처하므로 오버레이가 자체 캡처에 들어가지 않습니다.
- `outlinePath`: 로컬 좌표의 선택적 SVG 경로; 도구 설명은 기존 몸체와 꼬리 윤곽선을 사용합니다.
- `effectsActive`, `captureActive`, `resolvedBackdropSource`: 읽기 전용 렌더링/캡처 진단.

<a id="usage"></a>

## 사용법

```qml
import LVRS as LV

LV.ApplicationWindow {
    primaryColor: "#A571E6"
    // 기본값은 #141414 66%, primaryColor 4% 오버레이와 macOS 네이티브 배경 블러이다.
    LV.Popover {
        id: details
        width: 320; height: 180
        contentItem: DetailsView {}
    }
}
```

배경으로 재료를 사용하고 그 위에 콘텐츠를 배치합니다. `ApplicationWindow.windowColor` 는 기본적으로 테마를 사용합니다. applicationWindowFill ( #141414 ) 은 전체 표면 채움을 덮을 수 있으며, `windowBackgroundOpacity` 는 66% 기본값을 덮지만 창이나 콘텐츠를 페이드하지 않습니다. 소비자는 표준 `background` 속성을 여전히 덮을 수 있습니다. 네이티브 크롬이 전체 창 외곽선을 소유하므로 기본 창 배경은 자신의 테두리, 모서리 둥글기와 높이를 비활성화합니다.

<a id="how-it-works"></a>

## 동작 원리

기존 Qt Quick Shapes 및 Qt Quick Effects 모듈은 방사형 채우기, 실루엣 마스킹, 질감 포착 및 `MultiEffect` 확산을 제공합니다. 추가 라이브러리 의존성이 없습니다. 배경/내용 포착은 확산 전에 강조색과 합성되며, 중성 톤과 윤곽선은 그 후에 그려집니다. 포착은 조상 변환을 따르며, 피드백을 방지하기 위해 재료 자체, 조상 및 후손을 거부합니다.

소프트웨어 장면 그래프는 GPU 블러 없이 표면 색상, 투명도 및 윤곽선을 유지합니다. 네이티브 RHI 렌더링 (포함 macOS   Metal )은 배경 확산을 적용합니다. 일시적 표면은 창 내 소스를 흐리게 합니다. 별도로, macOS 의 네이티브 렌더 뷰 아래에 Qt 의 `ApplicationWindow` AppKit `NSVisualEffectView` 가 설치되어 있으며, `UnderWindowBackground` , `BehindWindow` 블렌딩과 활성 안개 처리된 재료를 사용합니다. Qt 는 투명하게 지우며, ApplicationWindow의 QML 중립 채색은 66%이고 그 위에 균일한 강조색 4% 레이어가 합성된다; 네이티브 창 및 전경은 알파 1를 유지합니다. 효과가 네이티브 리사이징을 따르며 입력을 가로채지 않고, `backgroundBlurEnabled` 가 거짓일 때 제거됩니다. 크롬 업데이트는 네이티브 효과를 보존합니다.

64px QML 확산, 66% 중립 채색과 4% 강조색 오버레이는 명시적인 LVRS 값입니다. AppKit 는 자체 네이티브 블러 커널 및 적응형 재료 톤을 제어하며, 해당 커널에 대한 공개 픽셀 반경 설정은 없습니다. 지원되지 않는 플랫폼은 불투명한 창 색상 대체 경로 를 유지하며 `backgroundBlurSupported: false` 를 보고합니다. 스크린 캡처, 개인 API 또는 추가 의존성이 필요하지 않습니다.

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

이전의 균일한 채색 구현은 2026-09-12에 macOS/Metal에서 검증했다. 해당 결과는 그레디언트 복원 전의 기록이다. 53개의 LVRS CTest 사례와 9개의 네이티브 재질 검사가 통과했다. 렌더링한 채우기 샘플은 50% 알파에서 #0B0B0B를 유지했고, 900×600와 1600×1000에서 파란색·보라색 강조색을 사용했다. 설치된 83개의 QML 리소스 전체가 현재 소스와 일치했다. Society는 16개 테스트를, Dreamscapes는 5개를 통과했으며, 검정에 가까운 채우기와 앱 강조색의 별도 기대값을 포함했다. Society, Dreamscapes, VisualCatalog를 갱신한 Workspace 런타임으로 다시 빌드하고 재실행했으며, 두 앱 창을 시각적으로 확인했다.

2026-10-15의 앞선 그레디언트 복원에서는 WindowMaterial의 두 방사형 강도를 0으로 고정하던 설정을 제거하였다. 이후 Figma 997:3 수정으로 ApplicationWindow 기본 배경은 다시 균일한 4% 강조색 오버레이를 사용하며, 아래 방사형 검사는 독립 WindowMaterial을 대상으로 한다. `LVRSTests_materials::window_gradient_tracks_accent_and_resize`는 900×600과 1600×1000에서 파란색·보라색 강조색의 실제 픽셀 차이, 가장자리 이동과 반투명을 검사한다. 균일한 색으로 돌아가면 이 검사가 실패한다.

`window_material_inherits_owning_window_accent`는 같은 QML 엔진의 창 두 개가 서로 다른 강조색을 유지하는지 검사한다. `application_window_accent_overlay_matches_figma`는 900×600과 1600×1000에서 파랑·녹색·보라색 전환의 균일한 4% 합성 색상과 알파, 그레디언트 부재, 보조 창의 강조색 및 전경의 불투명도를 검사한다.
