# ModelAdapter

위치: `src/backend/navigation/modeladapter.h`, `src/backend/navigation/modeladapter.cpp`

`ModelAdapter`는 역할 이름을 통해 C++ `QAbstractItemModel` 행을 읽기 위한 레거시 QML 싱글턴입니다.

<a id="purpose"></a>

## 목적

- QML `var`에 `QAbstractItemModel`가 있는지 여부를 감지합니다.
- 행 수를 QML에 노출합니다.
- 행을 역할 이름으로 입력된 `QVariantMap`로 변환합니다.

## API

- `isItemModel(model) -> bool`
- `count(model) -> int`
- `row(model, row, column = 0) -> object`

<a id="behavior-contract"></a>

## 행동 계약

- `count()`는 최상위 행 수를 반환합니다.
- `row()`는 요청된 최상위 행과 열을 읽은 다음 모든 유효한 `roleNames()` 값을 반환된 맵에 삽입합니다.
- `row()`는 또한 해당 키가 없을 때 `Qt::DisplayRole` / `Qt::EditRole`에서 `display` 및 `edit` 대체를 삽입합니다.
- 반환된 맵에는 `index`, `row` 및 `column`가 포함됩니다.
- 유효하지 않거나 모델이 아닌 입력은 던지는 대신 `false`, `0` 또는 빈 맵을 반환합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

const count = LV.ModelAdapter.count(backendModel)
const first = LV.ModelAdapter.row(backendModel, 0)
```

현재 모델 베어링 구성 요소는 내부 투영에 `ModelSource` / `HierarchyModel`를 사용합니다. 이미 싱글턴를 사용하고 있는 발신자와의 직접적인 호환성을 위해 `ModelAdapter`를 유지하세요.
