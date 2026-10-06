# ProgressModel

위치: `src/backend/model/progressmodel.h`, `src/backend/model/progressmodel.cpp`

`ProgressModel`는 `ProgressBar.qml`에 대한 숫자 범위 정규화를 소유합니다.

<a id="purpose"></a>

## 목적

- QML에서 진행 범위 상태 및 세그먼트 수학을 유지합니다.
- 최소, 최대, 시작 및 현재 값에 대한 선택적 `StateModel` 값을 읽으십시오.
- 렌더링을 위해 고정된 정규화된 값을 제공합니다.
- 둥근 직사각형과 원통형 진행 모양에 대한 반경 선택을 중앙 집중화합니다.

## API

입력:

- `minimumValue`
- `maximumValue`
- `startValue`
- `currentValue`
- `stateModel`
- `minimumValueStateKey`
- `maximumValueStateKey`
- `startValueStateKey`
- `currentValueStateKey`

읽기 전용:

- `usingStateModel`
- `stateRevision`
- `effectiveMinimumValue`
- `effectiveMaximumValue`
- `effectiveStartValue`
- `effectiveCurrentValue`
- `valueRange`
- `normalizedStart`
- `normalizedCurrent`
- `fillStart`
- `fillProgress`
- `progress`

방법:

- `stateNumber(key, fallbackValue)`
- `normalizedValue(value)`
- `radiusFor(shapeStyle, cornerRadius, rectWidth, rectHeight)`

<a id="how-it-works"></a>

## 동작 원리

- Direct QML 속성은 항상 대체 항목으로 유지됩니다.
- `stateModel`가 있는 경우 `StateModel::valueOr`를 통해 유효한 값을 읽습니다.
- 숫자가 아니거나 유한하지 않은 상태 값은 직접 속성 값으로 대체됩니다.
- `normalizedValue`는 `0..1`에 고정됩니다.
- 0에 가까운 범위는 이진 진행으로 렌더링됩니다. 최대값 이상의 현재 값은 `1`가 되고, 그렇지 않으면 `0`가 됩니다.
- `fillStart`는 정규화된 시작/전류 중 더 낮은 값이고 `fillProgress`는 절대 거리입니다.

<a id="qml-boundary"></a>

## QML 경계

`ProgressBar.qml`는 시각적 입력을 `ProgressModel`에 바인딩하고 추적/채우기 직사각형만 렌더링합니다. 수치 상태 분해능, 범위 강제 변환, 정규화된 진행, 세그먼트 계산 및 모양 반경 선택은 `ProgressModel`에 속합니다.
