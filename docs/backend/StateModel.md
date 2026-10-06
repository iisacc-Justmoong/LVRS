# StateModel

위치: `src/backend/state/statemodel.h` / `src/backend/state/statemodel.cpp`

`StateModel`는 백엔드 기반 LVRS 구성 요소를 위한 최초의 구체적인 C++ 상태 컨테이너입니다. `ViewModel`에서 파생되므로 `ViewModels`를 통해 등록하고 동일한 소유권 규칙을 사용하여 뷰에 바인딩할 수 있습니다.

<a id="purpose"></a>

## 목적

- 구성 요소 상태를 QML에서 C++ QObject로 이동합니다.
- 증분 구성 요소 마이그레이션을 위한 작은 키-값 상태 표면을 제공합니다.
- `revision`, `valuesChanged` 및 `valueChanged`를 통해 결정적 변경 알림을 노출합니다.
- 상태 개체를 `ViewModels` 설명자 및 소유권 게이트와 호환되게 유지하세요.

## API

속성:

- `values: map`
- `stateKeys: stringList`
- `revision: int`
- `empty: bool`

방법:

- `value(key, fallbackValue = undefined)`
- `valueOr(key, fallbackValue)`
- `hasValue(key)`
- `setValue(key, value)`
- `removeValue(key)`
- `applyPatch(patch)`
- `clearValues()`
- `stateSnapshot()`

신호:

- `valuesChanged()`
- `stateKeysChanged()`
- `revisionChanged()`
- `valueChanged(key, value, previousValue)`

<a id="behavior-contract"></a>

## 행동 계약

- 키는 트림 정규화됩니다.
- 빈 키는 거부되고 `error`를 `Empty state key`로 설정합니다.
- `setValue()`는 하나의 키를 변경하고 값이 실제로 변경될 때만 `revision`를 증가시킵니다.
- `applyPatch()`는 비어 있지 않은 패치 키를 현재 상태로 병합합니다.
- `clearValues()`는 모든 상태를 제거하고 키별 `valueChanged` 알림을 내보냅니다.
- `stateSnapshot()`에는 기본 `ViewModel` 스냅샷과 `values`, `stateKeys`, `revision` 및 `empty`가 포함됩니다.
- `ViewModels.descriptor(key)`는 등록된 인스턴스를 `stateModel=true`로 표시하고 현재 상태 필드를 포함합니다.

<a id="usage"></a>

## 사용법

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
    stateModel: progressState
}
```

<a id="migration-policy"></a>

## 마이그레이션 정책

아직 도메인별 `ViewModel` 하위 클래스를 사용할 자격이 없는 구성 요소 상태에는 `StateModel`를 사용하세요. 동작이 도메인별로 이루어지면 키-값 상태에서 명시적인 `Q_PROPERTY` 필드 및 명령을 사용하여 입력된 C++ `ViewModel`로 이동합니다.
