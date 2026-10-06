# TableHeaderModel

위치: `src/backend/model/tableheadermodel.h`, `src/backend/model/tableheadermodel.cpp`

`TableHeaderModel`는 `TableHeader.qml`에 대한 헤더 행 소스 해상도와 형상을 소유합니다.

<a id="purpose"></a>

## 목적

- `cellItems` 및 레거시 `columns`를 형식화된 헤더 설명자로 확인합니다.
- C++의 기본 및 개체 기반 헤더 항목을 정규화합니다.
- 문자열, 정수, 부동 소수점 및 부울 헤더 값에서 열 값 유형을 추론합니다.
- 렌더링을 위한 열 패딩, 너비 및 x 오프셋을 계산합니다.

## API

입력:

- `cellItems`
- `columns`
- `tableWidth`
- `rowHeight`
- `cellHorizontalPadding`
- `columnWidths`
- `fallbackCellWidth`
- `minColumnWidth`

읽기 전용:

- `descriptors`
- `columnCount`
- `revision`

방법:

- `resolvedColumnSource()`
- `columnAt(index)`
- `normalizeColumnType(value)`
- `inferredColumnType(value)`
- `columnType(index)`
- `columnText(index)`
- `columnPadding(index)`
- `numericWidth(value, fallbackValue)`
- `autoColumnWidth()`
- `columnWidth(index)`
- `columnX(index)`
- `descriptorAt(index)`

<a id="descriptor-contract"></a>

## 설명자 계약

각 설명자에는 다음이 포함됩니다.

- `index`
- `sourceData`
- `text`
- `valueType`
- `x`
- `width`
- `height`
- `padding`

<a id="how-it-works"></a>

## 동작 원리

- `cellItems`는 `columns`보다 선호됩니다.
- 객체 항목은 `label`, `text`, `title`, `value`에서 레이블 텍스트를 읽습니다.
- 객체 항목은 `type`, `valueType`, `cellType`, `dataType`에서 유형 메타데이터를 읽습니다.
- 기본 헤더 항목은 유형을 직접 추론합니다. 부울은 `bool`, 정수는 `int`, 비정수는 `float`, 그렇지 않으면 `string`입니다.
- 열 너비는 명시적 `columnWidths[index]`를 사용한 다음 `fallbackCellWidth`를 사용한 다음 `tableWidth / columnCount`의 동일한 자동 너비를 사용합니다.
- 너비는 `minColumnWidth`로 고정됩니다.
- `descriptorAt(index)`는 현재 설명자 목록 외부의 음수가 아닌 인덱스에 대해 대체 경로 `"Column"` 설명자를 반환하므로 `TableHeader.qml`는 여전히 1열 빈 상태 대체 경로를 렌더링할 수 있습니다.

<a id="qml-boundary"></a>

## QML 경계

`TableHeader.qml`는 `descriptors`를 렌더링하고 호환성 도우미 호출을 `TableHeaderModel`로 전달합니다. 헤더 텍스트 해상도, 유형 추론, 패딩, 너비 및 x 오프셋 계산은 모델 책임입니다.
