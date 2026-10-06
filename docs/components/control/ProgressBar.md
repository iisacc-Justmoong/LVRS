# ProgressBar

위치: `src/qml/components/control/display/ProgressBar.qml`

`ProgressBar`는 경량 범위 기반 진행률 표시기입니다.

<a id="purpose"></a>

## 목적

- 사용자 정의 숫자 범위에서 정규화된 진행 상황을 렌더링합니다.
- 제공된 경우 C++ `StateModel`에서 범위 상태를 읽습니다.
- 컴팩트한 크기 사전 설정 및 모양 정책을 지원합니다.

<a id="core-api"></a>

## 코어 API

크기와 모양:

- `size` (`large`, `regular`)
- `shapeStyle` (`shapeRoundRect`, `shapeCylinder`)
- `cornerRadius`
- `barHeight`(읽기 전용)

범위:

- `minimumValue`
- `maximumValue`
- `startValue`
- `currentValue`
- `endValue`(`maximumValue`의 호환성 별칭)
- `stateModel`
- `minimumValueStateKey`
- `maximumValueStateKey`
- `startValueStateKey`
- `currentValueStateKey`
- `usingStateModel`(읽기 전용)
- `effectiveMinimumValue`(읽기 전용)
- `effectiveMaximumValue`(읽기 전용)
- `effectiveStartValue`(읽기 전용)
- `effectiveCurrentValue`(읽기 전용)
- `valueRange`(읽기 전용)
- `normalizedStart`(읽기 전용, `0..1`에 고정됨)
- `normalizedCurrent`(읽기 전용, `0..1`에 고정됨)
- `fillStart`(읽기 전용, 고정된 세그먼트 시작)
- `fillProgress`(읽기 전용, 고정된 세그먼트 길이)
- `progress`(읽기 전용, `0..1`에 고정됨)

색상:

- `trackColor`
- `fillColor`

<a id="behavior-contract"></a>

## 행동 계약

- `ProgressBar`는 범위 정규화를 C++ `ProgressModel`에 위임합니다.
- `minimumValue` 및 `maximumValue`는 전체 숫자 범위를 정의합니다.
- `startValue` 및 `currentValue`는 해당 범위 내의 채워진 세그먼트를 정의합니다.
- `stateModel`가 있는 경우 막대는 해당 C++ 상태 개체에서 키별로 `effective*` 값을 읽고 키가 없거나 숫자가 아닌 경우 직접 QML 속성으로 대체됩니다.
- 클램핑 기능이 있는 `progress = (effectiveCurrentValue - effectiveMinimumValue) / (effectiveMaximumValue - effectiveMinimumValue)`.
- 시각적 채우기는 `fillStart = min(normalizedStart, normalizedCurrent)`에서 시작됩니다.
- 시각적 채우기 너비는 `fillProgress = abs(normalizedCurrent - normalizedStart)`입니다.
- 0에 가까운 범위는 이진 출력(`0` 또는 `1`)으로 대체됩니다.
- 원통 모양은 min(너비, 높이)/2 반경을 사용합니다.

<a id="backend-model"></a>

## 백엔드 모델

`ProgressModel`는 유효 값, 상태 모델 대체 경로, 정규화된 값, 채우기 세그먼트 수학 및 반경 계산을 소유합니다. `ProgressBar.qml`는 해당 모델 주변의 렌더 어댑터입니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.ProgressBar {
    width: 180
    size: regular
    minimumValue: 0
    maximumValue: 100
    startValue: 0
    currentValue: 64
}
```

<a id="c-state-example"></a>

## C++ 상태 예

```qml
import LVRS 1.0 as LV

LV.StateModel {
    id: progressState
    values: ({
        minimumValue: 0,
        maximumValue: 100,
        startValue: 0,
        currentValue: 64
    })
}

LV.ProgressBar {
    width: 180
    stateModel: progressState
}
```

<a id="segment-example"></a>

## 세그먼트 예

```qml
import LVRS 1.0 as LV

LV.ProgressBar {
    width: 180
    minimumValue: -50
    maximumValue: 150
    startValue: 50
    currentValue: 100
}
```

이것은 트랙의 50% 에서 75% 까지 섹션을 채웁니다. `currentValue` 가 `startValue` 보다 낮으면 컴포넌트는 여전히 2 값 사이의 섹션을 렌더링합니다.

<a id="shared-motion"></a>

## 공유 모션

표시된 채우기는 탄력적으로 고정됩니다. 숫자 진행 상황은 현재 상태로 유지되고 렌더링된 채우기는 트랙 내부에 유지됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
