# TableModel

위치: `src/backend/model/tablemodel.h`, `src/backend/model/tablemodel.cpp`

`TableModel`는 `Table.qml`에 대한 비시각적 테이블 모델 동작을 소유합니다.

<a id="purpose"></a>

## 목적

- C++에서 머리글, 행, 표시되는 셀, 셀 범위 및 열 유형을 확인합니다.
- 런타임 셀 병합/분할 및 행/열 삽입/삭제 돌연변이를 소유합니다.
- 자체 테이블 기하학, 행/열 크기 조정 상태 및 셀 컨텍스트 작업 설명자.
- 헤더 선언 유형에서 본문 셀 값을 검증하고 강제합니다.

## API

입력:

- `rows`
- `headerCellItems`
- `headerColumns`
- `defaultHeaderText`
- `defaultCellText`
- `inputable`
- `tableWidth`
- `rowHeight`
- `cellWidth`
- `columnWidths`
- `rowHeights`
- `minColumnWidth`
- `minRowHeight`

읽기 전용:

- `rowCount`
- `headerCount`
- `columnCount`
- `rowsModelBacked`
- `cellEditingAvailable`
- `structureMutationAvailable`
- `resizingColumnIndex`, `resizingRowIndex`
- `contextRowIndex`, `contextColumnIndex`
- `revision`
- `undoStack`
- `canUndo`, `canRedo`
- `undoDepth`, `redoDepth`

방법:

- 데이터: `resolvedHeaderSource()`, `rowAt(index)`, `cellAt(rowIndex, columnIndex)`, `headerAt(columnIndex)`
- 입력: `headerCellType(columnIndex)`, `columnType(columnIndex)`, `coerceCellValue(value, valueType)`, `validateCellInput(...)`
- 편집: `setCellValue(rowIndex, columnIndex, value)`
- 범위: `cellRowSpan(...)`, `cellColumnSpan(...)`, `mergeAnchorForCell(...)`, `isCoveredCell(...)`, `visibleCells()`
- 기하학: `columnWidth(...)`, `columnX(...)`, `columnSpanWidth(...)`, `rowHeightAt(...)`, `rowY(...)`, `rowSpanHeight(...)`, `totalBodyHeight()`
- 크기 조정: `setColumnWidth(...)`, `setRowHeight(...)`, `beginColumnResize(...)`, `updateColumnResize(...)`, `endColumnResize()`
- 구조: `insertRow(...)`, `appendRow()`, `deleteRow(...)`, `removeRow(...)`, `insertColumn(...)`, `appendColumn()`, `deleteColumn(...)`, `removeColumn(...)`
- 컨텍스트: `setContextCell(...)`, `contextMenuDescriptors(...)`, `triggerContextAction(...)`
- 병합/분할: `canMergeCells(...)`, `mergeCells(...)`, `splitCell(...)`
- 입력 가능 여부: `rowInputable(rowEntry)`, `cellInputable(rowIndex, columnIndex)`
- 내역: `undo()`, `redo()`, `clearUndoStack()`

<a id="how-it-works"></a>

## 동작 원리

- 헤더 소스는 먼저 `headerCellItems`에서 확인한 다음 `headerColumns`에서 확인합니다.
- `rows`가 `QAbstractItemModel`인 경우, `TableModel`는 C++ 모델을 기준 원본로 유지하고 `rowCount()`, `columnCount()`, `data(index, Qt::DisplayRole)`에서 렌더링 행을 빌드합니다. `data(index, Qt::EditRole)`.
- 기본 테이블 헤더가 아직 사용 중인 경우 C++ 항목 모델의 수평 `headerData(..., Qt::DisplayRole)`가 표시되는 헤더 레이블을 제공합니다.
- 헤더 개체는 `type`, `valueType`, `cellType` 또는 `dataType`를 선언할 수 있습니다.
- 기본 헤더는 해당 값에서 `string`, `int`, `float` 또는 `bool`를 추론합니다.
- `visibleCells()`는 `x`, `y`, `width` 및 `height`를 사용하여 평면화된 렌더링 모델을 반환하고 포함된 병합 멤버를 제외합니다.
- `count`/`get(index)`가 있는 객체를 포함하여 배열 및 목록과 같은 객체를 표시할 수 있습니다.
- C++ 항목 모델은 모델 개체를 `rows`에 할당하여 직접 표시할 수 있습니다.
- `setCellValue(rowIndex, columnIndex, value)`는 표준 `QAbstractItemModel::setData(index, coercedValue, Qt::EditRole)` 경로를 통해 C++ 항목 모델 행을 작성합니다. 모델이 `value`, `edit` 또는 `text` 역할을 노출하는 경우 해당 역할은 `Qt::EditRole` 이후에 시도됩니다.
- C++ 항목 모델 편집은 소스 소유입니다. 모델은 적절하게 `dataChanged`, `rowsInserted`, `rowsRemoved`, `columnsInserted`, `columnsRemoved`, `layoutChanged`, `modelReset` 또는 `headerDataChanged`를 방출해야 합니다. `TableModel`는 이를 수신하고 렌더링 설명자를 새로 고칩니다.
- 구조적 변형은 행이 변경 가능한 배열/목록 값에서 나온 경우에만 허용됩니다. 읽기 전용 목록형 객체는 표시 전용으로 유지됩니다.
- 행/열 구조 편집 및 셀 병합/분할에는 여전히 변경 가능한 배열 행이 필요합니다. C++ 모델은 자체 구조 편집 API를 소유하고 있습니다.
- 구조를 변경하기 전에 병합 메타데이터가 정규화되므로 이전 범위가 잘못된 행이나 열을 가리키지 않습니다.
- 행/열 삽입 및 삭제 작업을 통해 `rowHeights` 및 `columnWidths`가 편집된 구조에 맞게 정렬됩니다.
- 크기 조정 작업은 백엔드 변형이며 데이터 및 구조 편집과 동일한 실행 취소 스택을 사용합니다.
- 셀 상황에 맞는 메뉴는 백엔드 설명입니다. QML는 설명자를 표시 가능한 `ContextMenu` 항목에만 매핑하고 선택한 작업을 전달합니다.
- 각 허용된 배열 지원 변형 이전에 `TableModel`는 `ModelUndoStack`에 스냅샷을 기록합니다.
- `undo()` 및 `redo()`는 `rows`, 존재하는 경우 유지된 C++ 소스 객체, `headerCellItems`, `headerColumns`, `columnWidths`, `rowHeights` 및 행 변경 가능 상태를 복원한 다음 정상적인 돌연변이에서 사용되는 것과 동일한 모델/헤더/기하학 변경 신호를 내보냅니다.

<a id="qml-boundary"></a>

## QML 경계

`Table.qml` 는 이제 렌더링 및 이벤트 어댑터입니다. 시각적 입력을 `TableModel` 에 바인딩하고, `visibleCells()` 를 렌더링하며, 리사이즈 핸들 및 셀 컨텍스트 메뉴에서 포인터 이벤트를 전달하고, 백엔드 변형이 성공한 후 공개 QML 신호를 방출합니다. 기하학, 리사이징, 구조 편집, 병합/분할, 값 유효성 검사, 컨텍스트 메뉴 사용 가능성, 및 취소/재실행은 `TableModel` 에 의해 소유됩니다.
