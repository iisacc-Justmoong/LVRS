# ToggleSwitch

위치: `src/qml/components/control/check/ToggleSwitch.qml`

`ToggleSwitch`는 `QtQuick.Controls.Switch`를 기반으로 구축되고 Figma 2상태 토글 세트에 맞춰 정렬된 LVRS 스타일 스위치입니다.

<a id="purpose"></a>

## 목적

- 플랫폼 전반에 걸쳐 스위치 구조와 애니메이션을 결정적으로 유지합니다.
- 명시적인 트랙, 노브, 섀도우, 팔레트 및 모양 컨트롤을 노출합니다.

<a id="core-api"></a>

## 코어 API

형태 및 지표:

- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `trackWidth`, `trackHeight`, `trackPadding`
- `knobSize`
- `trackCornerRadius`, `knobCornerRadius`
- `transitionDuration`: 총 이동/리바운드 시간(밀리초), 기본적으로 `320`; `0`는 모션을 비활성화합니다.

Figma 그림자:

- `trackShadowEnabled`
- `trackShadowColor`, `trackShadowOpacity`
- `trackShadowBlur`
- `trackShadowHorizontalOffset`, `trackShadowVerticalOffset`

팔레트:

- `onColor`, `offColor`
- `onColorHover`, `onColorPressed`
- `offColorHover`, `offColorPressed`
- `disabledTrackColor`
- `trackShadowColor`
- `knobFillColor`
- 해결됨: `resolvedTrackColor`

상태(상속됨):

- `checked`, `enabled`, `text`

Figma 호환성:

- `state` <-> `checked`

<a id="behavior-contract"></a>

## 행동 계약

- Figma 구성 요소 세트(`111:379`)에는 On 및 Off 변형, `2px` 내부 패딩이 있는 각 `38 x 22`, `18 x 18` 손잡이 및 `20px` 제작 반경이 포함되어 있습니다.
- On은 `Theme.accent`로 확인됩니다. 꺼짐은 `Theme.panelBackground12`로 확인됩니다. 손잡이는 `Theme.titleHeaderColor`로 확인됩니다.
- 트랙은 Figma `0px 4px 4px` 섀도우와 `Theme.shadowStrong`(`25%` 검정색)를 사용합니다. `QtQuick.Effects.MultiEffect`는 흐림 및 자동 패딩 오버플로를 제공합니다.
- 마우스, 터치 또는 스페이스바를 누르면 손잡이가 잠시 눌러집니다. 이동 경로를 따라 뻗어 목적지를 약간 통과한 다음 정지 원으로 되돌아옵니다. 기본 이동/리바운드에는 `320ms`가 사용됩니다. 언론 피드백은 그 기간의 1/4이 걸립니다.
- 조작대 x 위치는 `2` (Off) 에서 `18` (On) 에 위치합니다. 한계가 설정된 `OutBack` 이완 곡선이 반동 효과를 제공합니다. 중심 기원의 `Scale` 는 원의 `18 x 18` 레이아웃이나 타겟을 변경하지 않고 변형합니다. 이는 기존 Qt 퀵 애니메이션 유형을 사용하며 의존성을 추가하지 않습니다.
- 드래그는 오른쪽에서 왼쪽 미러링을 포함하여 누르고 있는 동안 상속된 스위치 `visualPosition`를 직접 따릅니다. 손을 떼면 고정 애니메이션이 다시 시작됩니다. 현재 시각적 위치에서 대상을 빠르게 전환합니다. 애니메이션은 `toggled` 신호를 지연하거나 반복하지 않습니다.
- 프로그래밍 방식의 `checked`/`state` 변경 사항도 애니메이션으로 표시됩니다. 초기 상태는 유휴 상태로 렌더링됩니다. 비활성화된 컨트롤에 대한 후속 변경 사항은 즉시 적용됩니다. 즉각적인 상태 변경 및 프레스 변형 없음을 위해 `transitionDuration: 0`를 설정하십시오.
- 손잡이 채우기는 앤티앨리어싱된 장면 그래프 `Rectangle`이므로 원은 래스터 캔버스 없이도 해상도 독립적으로 유지됩니다. 이전 읽기 전용 `knobSupersampleScale`, `knobHiDpiScale` 및 `knobRasterScale` 값은 소스 호환성을 위해 계속 사용할 수 있습니다.
- 트랙 색상은 `checked + hovered + down + enabled`에서 확인됩니다.
- 데스크탑과 모바일은 `38 x 22` 트랙, `2` 패딩, `18 x 18` 손잡이, 반경 `20` 및 `9`, On/Off x 위치 `18`/`2` 및 그림자 흐림/수직 오프셋을 공유합니다. `4`.
- 데스크톱 및 모바일에서 선택적 라벨 간격은 `8px`로 유지되며 본문 글꼴 및 줄 높이는 `13px/13px`로 유지됩니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.ToggleSwitch {
    checked: true
}
```

시각적 카탈로그 선택 → ToggleSwitch 미리 보기에는 대화형 켜기/끄기 예제, 느린 리바운드, 즉각적인 동작 및 비활성화된 상태가 포함되어 있습니다. 스위치를 길게 누르거나 탭하거나 끌거나 초점을 맞춘 다음 Space를 눌러 피드백을 비교하세요.

<a id="validation"></a>

## 검증

`ctest --test-dir build -R LVRSTests_toggle_switch --output-on-failure` 는 Qt 테스트 입력과 기하학적 검사를 실행하여 누름 변형, 양방향 반동, 빠른 재타겟팅, 키보드/취소 입력, 터치, 드래그/반사, 즉시/비활성 상태를 확인합니다. `LVRSTests_import_api` 는 Figma 휴지 상태 기하학과 내장된 QML /소스 검사를 유지합니다.

네이티브 렌더링 검사에는 `LVRSTests_toggle_switch press_and_rebound` 를 실행할 때 `build/` 하위의 디렉토리를 `LVRS_TOGGLE_CAPTURE_DIR` 로 설정합니다. 4x 확대된 윈도우 프레임과 CSV 타임스탬프/조작대 기하학을 저장하며 컨트롤의 레이아웃 지표를 변경하지 않습니다. macOS 에서는 `QT_QPA_PLATFORM=cocoa QSG_RHI_BACKEND=metal` 와 빌드 트리 라이브러리/ QML 경로를 사용합니다.

Qt 참조: [PropertyAnimation 완화](https://doc.qt.io/qt-6.8/qml-qtquick-propertyanimation.html#easing-prop), [스위치 visualPosition](https://doc.qt.io/qt-6.8/qml-qtquick-controls-switch.html#visualPosition-prop).

<a id="shared-motion"></a>

## 공유 모션

손잡이는 이동 중에 늘어나고 선택한 끝점으로 되돌아옵니다. 느리고 즉각적인 변형이 제공됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
