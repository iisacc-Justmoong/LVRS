<a id="standard-materials"></a>

# 표준 재료

<a id="purpose"></a>

## 목적

`ApplicationWindow`의 기본 배경은 `Theme.window` (`#141414`) 98%와 앱 강조색 2%를 RGB로 직접 혼합한 중심색에 매우 고운 단색 노이즈를 더한 불투명 표면이다. 노이즈는 물리 픽셀 1개 크기이며 RGB 밝기 변화가 최대 ±2단계이다. 유리 레이어, Overlay 합성과 블러는 사용하지 않는다. 독립 `WindowMaterial`과 `PanelMaterial`은 기존 방사형 재질과 반투명 블러를 유지한다. ContextMenu와 메뉴는 더 옅은 코팅을 사용하고, 툴팁과 팝오버는 Glass 25를 사용한다.

| 재질 | 채우기 | 배경 흐림 | 기본 사용 |
| --- | --- | --- | --- |
| ApplicationWindow | `Theme.window` (`#141414`) 98% + 앱 강조색 2% 중심색과 미세 입자, 알파 1 | 없음 | 기본 및 보조 창 |
| 독립 WindowMaterial | #0B0B0B 50%와 강조색 방사형 그레디언트 | 64 논리 px | 명시적인 창 재질 |
| ContextMenu / 메뉴 | 창 채색 12%와 옅은 강조색 | 64 논리 px | 컴팩트 및 일반 메뉴 |
| Dense 75 MaterialSurface / PanelMaterial | #141414 75%와 강조색 방사형 그레디언트 | 64 논리 px | 조밀한 패널 |
| Glass 25 | #141414 25%와 강조색 방사형 그레디언트 | 16 논리 px | 툴팁, 팝오버, 기본 패널 |

WindowMaterial과 PanelMaterial은 `Theme.materialIntenseOpacity` / `Theme.materialFaintOpacity`의 0.40 / 0.11을 사용한다. 여덟 그레디언트는 크기 조정에 맞춰 중앙과 가장자리를 따른다. WindowMaterial의 Dense 75 설정은 64px 블러와 테두리 기본값을 공급하고, 독립 창 재질의 채색 불투명도는 50%이다. ApplicationWindow의 기본 Rectangle과 미세 노이즈는 이 독립 재질과 별도로 그린다.

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
- `color`, `tintOpacity`: 독립 재질의 표면 색상과 불투명도이다. 독립 WindowMaterial은 #0B0B0B / 0.50을 사용한다. ApplicationWindow의 기본 배경은 별도 Rectangle이며, RGB 혼합색과 알파 1을 사용한다.
- `blurRadius`, `blurEnabled`: 배경 확산 반경 및 스위치.
- `radius`, `borderWidth`, `borderColor`: 표면 실루엣과 가장자리.
- `intenseOpacity`, `faintOpacity`: 독립 재질의 방사형 강도이며 기본값은 0.40 / 0.11이다. 둘 다 0이면 방사형 모양이 생성되지 않는다.
- `backdropSource`, `backdropBackground`: 과도 표면 뒤에서 캡처할 명시적인 형제 항목입니다. 팝업은 애플리케이션 콘텐츠와 배경을 별도로 캡처하므로 오버레이가 자체 캡처에 들어가지 않습니다.
- `outlinePath`: 로컬 좌표의 선택적 SVG 경로; 도구 설명은 기존 몸체와 꼬리 윤곽선을 사용합니다.
- `effectsActive`, `captureActive`, `resolvedBackdropSource`: 읽기 전용 렌더링/캡처 진단.

<a id="usage"></a>

## 사용법

```qml
import LVRS as LV

LV.ApplicationWindow {
    primaryColor: "#A571E6"
    // `Theme.window` (`#141414`) 98%와 앱 강조색 2%의 중심색에 고운 단색 입자를 더한다.
    LV.Popover {
        id: details
        width: 320; height: 180
        contentItem: DetailsView {}
    }
}
```

`ApplicationWindow.windowColor`의 RGB가 배경 혼합의 바탕색이며 기본값은 `Theme.window` (`#141414`)이다. `Theme.applicationWindowAccentOpacity`의 0.02를 사용하여 `바탕 RGB × 0.98 + primaryColor RGB × 0.02`를 계산한다. 최종 알파는 1이고, 표준 `background`는 계속 재정의할 수 있다. 기존 네이티브 프레임을 유지한다.

<a id="how-it-works"></a>

## 동작 원리

기존 Qt Quick Shapes 및 Qt Quick Effects 모듈은 방사형 채우기, 실루엣 마스킹, 질감 포착 및 `MultiEffect` 확산을 제공합니다. 추가 라이브러리 의존성이 없습니다. 배경/내용 포착은 확산 전에 강조색과 합성되며, 중성 톤과 윤곽선은 그 후에 그려집니다. 포착은 조상 변환을 따르며, 피드백을 방지하기 위해 재료 자체, 조상 및 후손을 거부합니다.

ApplicationWindow의 기본 Rectangle은 채널별 RGB를 직접 혼합하고 8비트 색상으로 반올림한다. 그 위의 `WindowNoiseTexture`는 색상 노이즈와 블러를 0으로 설정하고 고정된 단색 입자만 더한다. `Theme.applicationWindowGrainStrength`는 `2 / 255`이며 최대 RGB 변화는 ±2단계이다. 물리 픽셀에 맞춰 타일을 그리므로 Retina에서도 입자가 커지지 않는다. 유리 합성, 화면 캡처나 블러 필터는 사용하지 않는다. 앱과 보조 창의 강조색 변경은 즉시 같은 혼합식에 반영된다. 소프트웨어 경로는 추가 콘텐츠 슈퍼샘플링 레이어를 사용하지 않으며, RHI의 기존 정책은 유지한다. 독립 방사형 재질의 소프트웨어 경로는 GPU 확산 없이 색상과 실루엣을 유지한다.

`ApplicationWindow.backgroundBlurEnabled`는 호환성 입력으로 남지만 기본 불투명 표면은 바꾸지 않는다. `backgroundBlurSupported`와 `backgroundBlurActive`는 false이다. 이전 `windowBackgroundOpacity` 입력이나 저알파 색상도 최종 창을 투명하게 만들지 않는다. macOS 솔리드 창은 알파 버퍼 0을 사용하고, `NSVisualEffectView`를 설치하지 않는다. 명시적으로 호출하는 `NativeWindowStyle.applyBackgroundBlur()` API는 알파 버퍼 8을 사용하며 해제 시 이전 설정을 복구한다.

명시적 네이티브 API 참조: [Apple NSVisualEffectView](https://developer.apple.com/documentation/appkit/nsvisualeffectview), [UnderWindowBackground](https://developer.apple.com/documentation/appkit/nsvisualeffectview/material-swift.enum/underwindowbackground), [Qt 창 알파 버퍼](https://doc.qt.io/qt-6.8/qquickwindow.html#setDefaultAlphaBuffer).

Figma 참조: [Window](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=944-31), [Panel](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=944-28), [ApplicationWindow](https://www.figma.com/design/0GkItQYSNIR0lZ3iJhfJzc?node-id=997-3). ApplicationWindow 링크는 이전 반투명 디자인의 참조이며 현재 미세 노이즈 배경의 시각 기준으로 사용하지 않는다.

검증: `LVRSTests_nativewindowblur`는 기본 창의 불투명도, 네이티브 뒤 배경 뷰의 부재, 크기 조정, 입력과 네이티브 표면 재생성을 검사한다. `application_window_theme_accent_mix`는 파랑·녹색·보라색·흰색·검정 강조색과 900×600·1600×1000 크기에서 98:2 중심색, 모든 배경 픽셀의 ±2 RGB 범위와 알파, 실제 입자 대비와 물리 픽셀별 변화, 고정 패턴, 강도 전환, 보조 창 상속, 전경 선명도와 저알파 입력을 검사한다. 독립 재질·팝업과 캡처 안전성 검사도 유지한다.

<a id="window-noise-texture"></a>

## WindowNoiseTexture

`LV.WindowNoiseTexture`는 불투명한 색상 노이즈와 미세 단색 입자를 그리는 래스터 항목이다. ApplicationWindow는 색상 노이즈와 블러를 해제하고 미세 입자만 사용한다.

| 속성 | 기본값 | 동작 |
|---|---|---|
| `color` | `#141414` | RGB 바탕색이며 알파는 항상 1로 정규화한다. |
| `primaryColor` | `#1582FF` | 노이즈의 강조색이며 알파는 항상 1로 정규화한다. |
| `noiseStrength` | `0.32` | 배경과 강조색 사이의 혼합 강도이며 0~1 범위이다. |
| `blurRadius` | `1.5` | 가우시안 표준편차이며 0~50 논리 px 범위이다. 0은 선명한 노이즈이다. |
| `grainStrength` | `0` | 블러 이후 더하는 단색 입자의 진폭이며 0~1 범위이다. ApplicationWindow는 `2 / 255`를 사용한다. |

유효하지 않은 색상과 유한하지 않은 수치는 기존 설정을 유지한다. 입자 강도가 0보다 크면 캐시된 타일은 페인트 대상의 물리 픽셀에 맞춰 반복한다. 각 RGB 채널에 같은 난수 값을 더하고 0~255로 제한한다. 입자는 블러 이후에 적용하므로 뭉개지지 않는다. 타일의 생성·블러·입자 처리는 설정 변경 때만 실행하며, 정적인 화면에는 타이머나 프레임별 난수 생성이 없다.

<a id="shared-motion"></a>

## 공유 모션

MaterialSurface를 통한 색조 및 불투명도 혼합; 호스트 창은 네이티브 이동 및 크기 조정 동작을 유지합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.

Figma 확산 레이어는 (0, 1) 좌표에 0블러 흰색 내장 하이라이트를 포함하며, 밀집 6%, 유리 14%입니다. `innerHighlightOpacity` 와 `innerHighlightOffsetY` 는 이 작성된 값을 노출합니다. 네이티브 RHI 렌더링은 기존 마스크를 사용하여 이동된 실루엣을 뺍니다; 커스텀 꼬리를 포함하며, 소프트웨어 렌더러는 기존 효과 없는 대체 경로 를 유지합니다.

ContextMenu 및 메뉴는 명시적 반투명 코팅이 있는 WindowMaterial를 사용합니다. 창 채우기 색상의 12%, 흐림 효과의 64px 및 일반 방사형 악센트 강도의 8%입니다. 도구 설명 및 팝오버는 PanelMaterial 유리 25를 유지합니다. [ContextMenu](../navigation/ContextMenu.md#window-derived-frosted-menu)를 참조하세요.

메뉴는 창의 `materialBackdropSource` 컨텐츠 호스트와 배경을 별도의 소스로 사용합니다. 호스트는 팝업 오버레이를 제외하여 기존 상위/하위 캡처 가드를 유지하면서 메뉴 뒤의 전경 콘텐츠가 확산되도록 허용합니다.

`backdropBaseColor` 는 기본적으로 투명합니다. 윈도우 내 메뉴는 불투명 중립 테마를 사용합니다. materialTint 를 캡처 기반으로 하여 흐릿한 합성 가 덮은 픽셀을 대체합니다; 그렇지 않으면 캡처된 윈도우 알파가 원래 날카로운 텍스트가 새어 나오게 합니다. 가시 메뉴 코팅은 12%로 유지됩니다.

이전의 균일한 채색 구현은 2026-09-12에 macOS/Metal에서 검증했다. 해당 결과는 그레디언트 복원 전의 기록이다. 53개의 LVRS CTest 사례와 9개의 네이티브 재질 검사가 통과했다. 렌더링한 채우기 샘플은 50% 알파에서 #0B0B0B를 유지했고, 900×600와 1600×1000에서 파란색·보라색 강조색을 사용했다. 설치된 83개의 QML 리소스 전체가 현재 소스와 일치했다. Society는 16개 테스트를, Dreamscapes는 5개를 통과했으며, 검정에 가까운 채우기와 앱 강조색의 별도 기대값을 포함했다. Society, Dreamscapes, VisualCatalog를 갱신한 Workspace 런타임으로 다시 빌드하고 재실행했으며, 두 앱 창을 시각적으로 확인했다.

2026-10-15의 균일한 4% 강조색 오버레이와 이후 노이즈·검은 유리 시안은 이전 구현의 기록이다. 현재 ApplicationWindow는 `Theme.window` (`#141414`) 98%와 강조색 2%의 중심색에 미세 단색 입자를 더한다. 독립 WindowMaterial은 `window_gradient_tracks_accent_and_resize`가 계속 검사한다.

`window_material_inherits_owning_window_accent`는 같은 QML 엔진의 창 두 개가 서로 다른 강조색을 유지하는지 검사한다. ApplicationWindow의 기본 중심색과 미세 노이즈도 기존 창 강조색 상속을 사용한다.
