# RadioButton

위치: `src/qml/components/control/check/RadioButton.qml`

`RadioButton`는 레거시 호환성 별칭을 갖춘 소형 원형 선택기입니다.

<a id="purpose"></a>

## 목적

- 결정론적 무선 표시기 시각적 자료를 제공합니다.
- 기존 API(`state`, `available`)를 표준 Qt 속성과 호환되게 유지하세요.

<a id="core-api"></a>

## 코어 API

정식 상태:

- `checked`, `enabled`, `text`

호환성 별칭:

- `state` <-> `checked`
- `available` <-> `enabled`

시각적:

- `indicatorSize`, `dotSize`
- 해결된 반경: `indicatorRadius`, `dotRadius`
- `onColor`, `offColor`
- `onColorDisabled`, `offColorDisabled`
- `dotColor`, `dotColorDisabled`
- 해결됨: `indicatorColor`, `indicatorDotColor`

<a id="behavior-contract"></a>

## 행동 계약

- 별칭 쌍은 양방향(`onStateChanged`, `onCheckedChanged`, `onAvailableChanged`, `onEnabledChanged`)으로 동기화됩니다.
- Figma 구성 요소 세트(`44:630`)에는 4개의 `18 x 18` 변형이 포함되어 있습니다. On/Off와 Available True/False가 교차됩니다.
- 상태에서는 `18 x 18` 표시기 내부의 `(5, 5)`에 `8 x 8` 점을 배치합니다. 외부 반경과 내부 반경은 각각 `9` 및 `4`입니다.
- 4가지 상태 팔레트는 Figma 토큰으로 고정됩니다. 활성화된 On은 `TitleHeader` 도트가 있는 `Accent`를 사용하고, 활성화된 Off는 `TitleHeader`를 사용하고, 비활성화된 On은 `Caption` 도트가 있는 `PanelBackground12`를 사용하고, 비활성화된 Off는 사용합니다. `PanelBackground12`.
- 데스크톱과 모바일은 `18 x 18` 표시기, `(5, 5)`의 `8 x 8` 점, 반경 `9` 및 `4`를 공유합니다.
- 선택적 QML 레이블 확장은 `8px` 간격 및 본문 `13px/13px`를 사용하여 데스크톱 및 모바일에서 텍스트 `Label`에 대한 `59 x 18` 경계를 생성합니다.
- 표시기 지오메트리는 명시적으로 배치되므로 런타임 대상 변경 사항은 암시적 크기를 결정론적으로 다시 계산합니다.
- 구성 요소는 투명한 배경 정책(`tone: Borderless`)을 사용합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.RadioButton {
    text: "Choice A"
    checked: true
}
```

<a id="shared-motion"></a>

## 공유 모션

내부 점은 체크된 채우기가 혼합되는 동안 공통 오버슈트가 있는 위치로 커집니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
