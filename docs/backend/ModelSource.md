# ModelSource

위치: `src/backend/model/modelsource.h`, `src/backend/model/modelsource.cpp`

`ModelSource`는 모델 베어링 QML 구성 요소용 공유 C++ 리더입니다.

<a id="purpose"></a>

## 목적

- 하나의 주입된 `source`를 통해 JavaScript 배열, 기본 배열, QML 목록형 객체 및 `QAbstractItemModel` 인스턴스를 허용합니다.
- QML 측 어댑터 로직 없이 항목 모델 행을 역할 이름 맵으로 프로젝트합니다.
- 지원 C++ 항목 모델이 변경되면 개정/개수 변경 사항을 내보냅니다.

## API

속성:

- `source`
- `column`
- `count`(읽기 전용)
- `revision`(읽기 전용)
- `itemModel`(읽기 전용)

방법:

- `at(index)`
- `row(index)`
- `roleValue(entry, roleName, fallbackValue)`
- `textValue(entry, roleNames, fallbackValue)`
- `boolValue(entry, roleName, fallbackValue)`
- `intValue(entry, roleName, fallbackValue)`
- `invalidate()`

<a id="how-it-works"></a>

## 동작 원리

- `QAbstractItemModel` 입력은 요청된 `column`에서 읽혀지고 `roleNames()`에 의해 키가 지정된 맵으로 변환됩니다.
- `display`, `edit`, `index`, `row` 및 `column`가 안정적인 대체 경로 필드로 추가되었습니다.
- JS/목록 유사 입력은 가능한 경우 인덱스, `get(index)` 또는 `at(index)`로 읽습니다.
- `rowsInserted`, `rowsRemoved`, `rowsMoved`, `modelReset`, `layoutChanged` 및 `dataChanged`는 모두 `revision`를 무효화합니다.

<a id="consumers"></a>

## 소비자

- `List`는 행 수, 역할 조회, 레이블 확인, 활성화 상태 및 선택 상태에 `ModelSource`를 사용합니다.
- `HierarchyModel`는 트리 행 설명자를 투영하기 전에 `ModelSource`를 사용합니다.
