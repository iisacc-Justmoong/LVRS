<a id="table"></a>

# 테이블

위치: `src/qml/components/control/display/Table.qml`

`Table`는 `TableHeader`를 구성하고 `TableCellItem` 대표자를 컴팩트한 스프레드시트 화면에 배치합니다. 데이터, 입력, 범위 변경, 정렬, 기하학 및 실행 취소 동작은 C++ `TableModel`에 의해 지원됩니다. QML는 렌더링, 선택 및 이벤트 어댑터로 유지됩니다.

<a id="purpose"></a>

## 목적

- 고정된 높이의 조밀한 표 형식 디스플레이를 제공합니다.
- 헤더와 행에 대한 배열/목록 모델 스타일 입력을 허용합니다.
- 셀/범위 선택, A1 참조, 형식화된 직사각형 쓰기, TSV 교환 및 안정적인 열 정렬을 노출합니다.
- 범위 메타데이터 및 테이블 수준 돌연변이 방법을 통해 셀 병합/분할 동작을 지원합니다.

## API

데이터:

- `headerCellItems`(선호)
- `columns`: `headerCellItems`의 인체공학적 별칭.
- `headerColumns`
- `rows`
- `model`: `rows`의 인체공학적 별칭.
- `editable`: `inputable`의 인체공학적 별칭.
- `headerDelegate`: `headerCellDelegate`의 인체공학적 별칭.
- `delegate`: `cellDelegate`의 인체공학적 별칭.
- `headerCellDelegate`: `TableHeader.cellDelegate`로 전달되는 선택적 구성 요소입니다.
- `cellDelegate`: 선택적 신체당 셀 구성 요소 위임. 기본 대리인은 `TableCellItem`입니다.

레이아웃:

- `rowHeight`
- `cellWidth`(`0`는 자동 너비를 의미함)
- `columnWidths`(열당 명시적 너비)
- `rowHeights`(행별 명시적 높이)
- `minColumnWidth`, `minRowHeight`
- `resizeHandlesVisible`(기본값 `true`)
- `columnResizeHandleWidth`, `rowResizeHandleHeight`
- `resizingColumnIndex`, `resizingRowIndex`
- `rowCount`, `headerCount`, `columnCount`(읽기 전용 공개 수)
- `resolvedRowCount`, `resolvedHeaderCount`, `resolvedColumnCount`(호환성 개수)
- `rowsModelBacked`(읽기 전용, `rows`가 C++ `QAbstractItemModel`인 경우 true)
- `cellEditingAvailable`(읽기 전용, 모델 지원 또는 어레이 지원 셀 편집 경로가 있는 경우 true)
- `visibleCellItems`(읽기 전용 평면 렌더링 모델)
- `canUndo`, `canRedo`(읽기 전용)
- `undoDepth`, `redoDepth`(읽기 전용)

시각적:

- `backgroundColor`
- `borderColor`, `borderWidth`
- `headerTextColor`
- `cellTextColor`
- `dividerColor`(행 구분선 기준선에 대한 레거시 별칭)
- `rowDividerColor`
- `headerSeparatorColor`
- `inputable`(기본값 `false`, 본문 셀에 대한 테이블 수준 편집 가능한 기본값)
- `structureControlsVisible`(기본값 `false`, 행/열 `+` 컨트롤 선택)
- `addRowControlsVisible`(기본값 `true`)
- `addColumnControlsVisible`(기본값 `true`)
- `deleteContextMenuEnabled`(기본값 `true`)
- `structureGutterWidth`, `structureGutterHeight`
- `defaultHeaderText`, `defaultCellText`
- `contextRowIndex`, `contextColumnIndex`

선택 및 정렬:

- `selectionEnabled`(기본값 `true`)
- `selectionMode`: `Table.NoSelection`, `Table.SingleCellSelection` 또는 `Table.RangeSelection`(기본값)
- `keyboardNavigationEnabled`, `selectOnContextClick`
- `selectionColor`, `currentCellBorderColor`, `currentCellBorderWidth`
- `currentRow`, `currentColumn`, `currentCell`(읽기 전용)
- `hasSelection`, `selectedRange`, `selectedCellCount`(읽기 전용)
- `sortingEnabled`(기본값 `false`), `sortOnHeaderClick`
- `sortingAvailable`(읽기 전용, 변경 가능한 배열 지원 행의 경우 true)
- `sortColumn`, `sortOrder`

신호:

- `cellInputEdited(rowIndex, columnIndex, text)`
- `cellInputSubmitted(rowIndex, columnIndex, text)`
- `cellInputRejected(rowIndex, columnIndex, text, valueType)`
- `cellsMerged(rowIndex, columnIndex, rowSpan, columnSpan)`
- `cellSplit(rowIndex, columnIndex)`
- `rowInserted(rowIndex)`
- `rowDeleted(rowIndex)`
- `columnInserted(columnIndex)`
- `columnDeleted(columnIndex)`
- `columnResized(columnIndex, width)`
- `rowResized(rowIndex, height)`
- `cellActivated(rowIndex, columnIndex, cellData)`
- `selectionChanged(range)`
- `rangeValuesChanged(range)`
- `rowsSorted(columnIndex, sortOrder)`

도우미 방법:

- `rowAt(index)`
- `cellAt(rowIndex, columnIndex)`
- `columnCountForRow(rowEntry)`
- `autoCellWidth(rowEntry)`
- `columnWidth(columnIndex)`, `columnX(columnIndex)`, `columnSpanWidth(columnIndex, columnSpan)`
- `rowHeightAt(rowIndex)`, `rowY(rowIndex)`, `rowSpanHeight(rowIndex, rowSpan)`, `totalBodyHeight()`
- `rowCellWidth(rowEntry)`
- `rowCellSpacing(rowEntry)`
- `cellX(rowEntry, columnIndex)`
- `cellSpanWidth(rowEntry, columnSpan)`
- `rowInputable(rowEntry)`
- `cellInputable(rowIndex, columnIndex)`
- `headerAt(index)`
- `headerCellType(columnIndex)`, `columnType(columnIndex)`
- `normalizeHeaderCellType(value)`, `inferredCellType(value)`
- `cellRawValue(rowIndex, columnIndex)`
- `typedDefaultValue(valueType)`
- `coerceCellValue(value, valueType)`
- `validateCellInput(rowIndex, columnIndex, value)`
- `cellValueAccepted(rowIndex, columnIndex, value)`
- `cellText(rowIndex, columnIndex)`
- `setCellValue(rowIndex, columnIndex, value)`
- `columnName(columnIndex)`, `columnIndexFromName(name)`
- `cellReference(rowIndex, columnIndex)`, `cellCoordinates(reference)`
- `selectCell(rowIndex, columnIndex, extendSelection)`
- `selectRange(startRow, startColumn, endRow, endColumn)`
- `selectRow(rowIndex)`, `selectColumn(columnIndex)`, `selectAll()`, `clearSelection()`
- `moveCurrentCell(rowDelta, columnDelta, extendSelection)`, `isCellSelected(rowIndex, columnIndex)`
- `selectedCellDescriptors()`, `rangeValues(...)`, `selectionValues()`
- `setRangeValues(startRow, startColumn, values)`, `setSelectionValues(values)`
- `valuesAsTsv(values)`, `selectionAsTsv()`, `parseTsv(text)`
- `pasteTsv(startRow, startColumn, text)`, `pasteSelectionTsv(text)`
- `sortByColumn(columnIndex, order)`, `sortAscending(columnIndex)`, `sortDescending(columnIndex)`, `toggleSortForColumn(columnIndex)`
- `cellRowSpan(rowIndex, columnIndex)`
- `cellColumnSpan(rowIndex, columnIndex)`
- `isCoveredCell(rowIndex, columnIndex)`
- `mergeAnchorForCell(rowIndex, columnIndex)`
- `canMergeCells(rowIndex, columnIndex, rowSpan, columnSpan)`
- `mergeCells(rowIndex, columnIndex, rowSpan, columnSpan)`
- `splitCell(rowIndex, columnIndex)`
- `canMutateStructure()`
- `insertRow(rowIndex)`, `appendRow()`, `deleteRow(rowIndex)`, `removeRow(rowIndex)`
- `insertColumn(columnIndex)`, `appendColumn()`, `deleteColumn(columnIndex)`, `removeColumn(columnIndex)`
- `canInsertRow(rowIndex)`, `canDeleteRow(rowIndex)`
- `canInsertColumn(columnIndex)`, `canDeleteColumn(columnIndex)`
- `buildContextMenuItems(rowIndex, columnIndex)`
- `openContextMenuForCell(rowIndex, columnIndex, menu, item, xPos, yPos)`
- `setColumnWidth(columnIndex, width)`, `setRowHeight(rowIndex, height)`
- `beginColumnResize(columnIndex, pointerX)`, `updateColumnResize(pointerX)`, `endColumnResize()`
- `beginRowResize(rowIndex, pointerY)`, `updateRowResize(pointerY)`, `endRowResize()`
- `undo()`, `redo()`, `clearUndoStack()`

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.Table {
    columns: [
        { label: "Name", type: "string" },
        { label: "State", type: "bool" },
        { label: "Score", type: "float" }
    ]
    model: [
        [{ text: "Renderer" }, { value: true }, { value: 0.98 }],
        [{ text: "Metrics" }, { value: false }, { value: 0.72 }]
    ]
    editable: true
    sortingEnabled: true
    columnWidths: [160, 80, 120]
    rowHeights: [28, 24]
}
```

<a id="how-it-works"></a>

## 동작 원리

- `TableModel`는 헤더 확인, 본문 행 조회, 열 유형 추론, 값 강제 변환, 메타데이터 병합/분할 및 행/열 구조 변형을 소유합니다.
- `TableModel`는 또한 테이블 기하학, 행/열 크기 조정 상태, 셀 컨텍스트 메뉴 설명자 및 컨텍스트 작업 디스패치를 소유합니다.
- `Table.qml`는 모델 메서드를 `TableModel`로 전달하고, 백엔드에서 제공하는 셀 설명자를 렌더링하고, 백엔드 변형이 성공한 후 공개 QML 신호를 내보냅니다.
- `TableModel`는 배열 지원 셀 편집, 병합/분할 작업, 행/열 삽입/삭제 작업 및 행/열 크기 조정 작업을 C++ `ModelUndoStack`에 기록합니다.
- `rows`가 C++ `QAbstractItemModel`인 경우 `TableModel`는 해당 모델 객체를 소스로 유지하고 `rowCount`, `columnCount`, `data` 및 수평 `headerData`에서 렌더링합니다.
- 모델 지원 `setCellValue(...)`는 소스 모델의 `setData(index, coercedValue, Qt::EditRole)`를 호출한 다음 존재하는 경우 명명된 `value`, `edit` 및 `text` 역할로 대체됩니다. QML는 편집 후 `rows`를 스냅샷으로 바꾸지 않습니다.
- 헤더 항목은 본문 열 유형을 정의합니다. 객체는 `type`, `valueType`, `cellType` 또는 `dataType`를 선언할 수 있습니다. 기본 항목은 기본 항목 자체에서 유형을 추론합니다.
- `TableModel.visibleCells()`에는 이미 `x`, `y`, `width` 및 `height`가 포함되어 있습니다. 덮인 셀은 건너뛰고 앵커 셀은 병합된 너비/높이를 받습니다.
- 각 가시화 가능한 바디 셀은 `cellDelegate` 를 인스턴스화하고 하나의 `modelData` 객체를 주입합니다. 설명자는 `index`, `rowIndex`, `columnIndex`, `address`, `cellData`, `text`, `valueType`, `inputable`, `selected`, `current`, `rowSpan`, `columnSpan`, `x`, `y`, `width`, 및 `height` 를 포함합니다.
- 사용자 정의 본문 셀 대리자는 `property var modelData`를 선언해야 합니다. 루트 항목의 크기는 백엔드에서 계산된 셀 직사각형에 맞게 조정됩니다.
- `headerCellDelegate`는 `TableHeader.cellDelegate` 계약을 사용하므로 헤더 및 본문 렌더링을 독립적으로 사용자 정의할 수 있습니다.
- 편집 가능한 동작은 `Table.inputable -> row inputable -> cell inputable -> TableCellItem.inputable`를 통해 `TableModel`를 통해 전파됩니다.
- 각 본문 `TableCellItem`는 `Table`에서 주입된 유효성 검사기를 수신하며, 이는 `TableModel`에 위임되므로 인라인 편집은 헤더 열 유형에 의해 제한됩니다.
- JS 배열, 모델 유사 객체 및 C++ 항목 모델에 대한 헤더 및 행 개수가 해결되었습니다. 구조 편집은 변경 가능한 배열/목록 입력으로 제한됩니다.
- 백엔드 지오메트리는 `columnWidths`, `cellWidth`, 열 너비 자동 맞춤에서 너비를 계산합니다.
- 백엔드 기하학은 `rowHeights`에서 높이를 계산한 다음 `rowHeight`에서 높이를 계산합니다.
- 구조 컨트롤은 `rows`가 변경 가능한 경우 행 추가 버튼에 대한 오른쪽 여백과 열 추가 버튼에 대한 하단 여백을 예약합니다.
- `resizeHandlesVisible`가 활성화되면 크기 조정 핸들이 열 오른쪽 테두리와 행 아래쪽 테두리에 위치합니다.
- 테이블 컨테이너는 콘텐츠를 잘라내고 내부 구분선 계약을 시행합니다.

<a id="spreadsheet-api"></a>

## 스프레드시트 API

행과 열은 0부터 시작하는 인덱스를 사용합니다. A1 참조는 1부터 시작하는 행 번호를 사용합니다. `cellReference(11, 26)`는 `AA12`를 반환하고, `cellCoordinates("AA12")`는 `{ valid: true, rowIndex: 11, columnIndex: 26 }`를 반환합니다.

```qml
LV.Table {
    id: sheet
    columns: [
        { label: "Name", type: "string" },
        { label: "Count", type: "int" },
        { label: "Enabled", type: "bool" }
    ]
    model: [
        [{ value: "Beta" }, { value: 2 }, { value: true }],
        [{ value: "Alpha" }, { value: 10 }, { value: false }]
    ]
    editable: true
    sortingEnabled: true

    Component.onCompleted: {
        sheet.selectRange(0, 0, 1, 1)
        const clipboardText = sheet.selectionAsTsv() // "Beta\t2\nAlpha\t10"
        sheet.pasteTsv(0, 0, "Gamma\t5\ttrue")
        sheet.sortByColumn(1, Qt.DescendingOrder)
    }
}
```

`setRangeValues(...)` 는 한 행 ( `["Name", 3]` ) 또는 2차원 행렬 ( `[["A", 1], ["B", 2]]` ) 중 하나를 받습니다. `pasteTsv(...)` 와 함께 전체 직사각형을 변형 전에 검증합니다. 잘못된 타입 값, 병합된 셀, 불규칙한 빈 입력 행, 또는 범위를 벗어난 타겟은 전체 작업을 거부합니다. 성공적인 직사각형 쓰기 또는 정렬은 하나의 되돌리기 단계이며, 셀/행마다 하나의 단계가 아닙니다. TSV 파싱은 탭, CRLF / LF 행, 인용된 필드, 내장된 줄 바꿈, 및 이중 인용을 지원합니다.

범위 쓰기, 구조적 편집, 및 내장 정렬은 작업이 원자적이고 되돌릴 수 있도록 mutable 배열 기반 행을 의도적으로 요구합니다. `QAbstractItemModel` 은 렌더링 및 단일 셀 `setCellValue(...)` 를 위해 계속 지원됩니다. 해당 백엔드를 사용할 때 소스/프록시 모델에서 정렬 또는 배치 편집을 제공하세요.

키보드 선택은 스프레드시트 규칙을 따릅니다. 화살표는 현재 셀을 이동하고, Shift+화살표는 범위를 확장하고, Home/End는 첫 번째/마지막 열로 이동하고, Ctrl/Command+A는 모든 셀을 선택하고, Escape는 선택을 지웁니다.

<a id="typed-columns"></a>

## 유형이 지정된 열

지원되는 열 유형은 다음과 같습니다.

- `string`
- `int`
- `float`
- `bool`

객체 헤더는 다음과 같은 동등한 유형 키를 선언할 수 있습니다.

```qml
headerCellItems: [
    { label: "Name", type: "string" },
    { label: "Count", valueType: "int" },
    { label: "Ratio", cellType: "float" },
    { label: "Enabled", dataType: "bool" }
]
```

기본 헤더는 유형을 직접 추론합니다.

```qml
headerCellItems: ["Name", 1, 1.5, true]
```

`coerceCellValue(value, valueType)`는 `{ accepted, type, value, text }`를 반환합니다. `setCellValue(rowIndex, columnIndex, value)`는 `TableModel`를 통해 동일한 유형 검사를 적용하고 허용된 값을 배열 행에 다시 동기화하거나 `setData`를 통해 C++ 항목 모델에 씁니다.

```qml
LV.Table {
    id: table
    headerCellItems: [
        { label: "Name", type: "string" },
        { label: "Count", type: "int" },
        { label: "Visible", type: "bool" }
    ]
    rows: [[{ value: "Renderer" }, { value: 3 }, { value: true }]]

    Component.onCompleted: {
        table.setCellValue(0, 1, "4")      // 허용하며 숫자 4를 저장한다
        table.setCellValue(0, 1, "4.2")    // int에 대해 거부한다
        table.setCellValue(0, 2, "false")  // 허용하며 불리언 false를 저장한다
    }
}
```

<a id="delegate-example"></a>

## 대리인 예

```qml
Component {
    id: bodyCell

    LV.TableCellItem {
        property var modelData: ({})
        itemData: modelData.cellData
        text: modelData.text
        valueType: modelData.valueType
        inputable: modelData.inputable === true
    }
}

LV.Table {
    headerCellItems: [{ label: "Name" }, { label: "Count", type: "int" }]
    rows: [[{ value: "Renderer" }, { value: 3 }]]
    cellDelegate: bodyCell
}
```

<a id="merge-and-split"></a>

## 병합 및 분할

정적 데이터는 다음을 선언할 수 있습니다.

- `rowSpan`: 앵커 셀이 포함하는 행 수.
- `columnSpan`: 앵커 셀이 포함하는 열 수.
- `colSpan`: `columnSpan`의 호환성 별칭입니다.

런타임 방법:

```qml
LV.Table {
    id: table
    rows: [
        [{ text: "A1" }, { text: "B1" }, { text: "C1" }],
        [{ text: "A2" }, { text: "B2" }, { text: "C2" }]
    ]

    Component.onCompleted: {
        table.mergeCells(0, 0, 2, 2)
        table.splitCell(0, 0)
    }
}
```

`mergeCells(...)`는 적용된 셀을 개체로 정규화하고 내부 병합 멤버로 표시하는 `TableModel`에 위임합니다. `splitCell(...)`는 앵커 셀이나 덮인 셀을 수용하고 덮인 셀을 보이는 셀로 복원합니다.

`mergeCells(...)`는 `false`를 반환하고 `rows`가 행 배열의 JavaScript 배열이 아니거나 요청된 사각형이 범위를 벗어난 경우 경고합니다. 모델과 유사한 읽기 전용 입력은 여전히 ​​정적 `rowSpan`/`columnSpan`를 렌더링할 수 있지만 런타임 변형에는 배열 행이 필요합니다.

<a id="structure-editing"></a>

## 구조 편집

`structureControlsVisible`가 활성화되고 `rows`가 행 배열의 JavaScript 배열인 경우:

- `+` 버튼은 각 본문 행의 오른쪽 끝에 나타납니다. 클릭하면 해당 행 뒤에 행이 삽입됩니다.
- `+` 버튼은 각 열의 하단에 나타납니다. 클릭하면 해당 열 뒤에 열이 삽입됩니다.
- 본문 셀을 마우스 오른쪽 버튼으로 클릭하면 행 및 열 삭제가 포함된 해당 셀 자체의 상황에 맞는 메뉴가 열립니다.

프로그래밍 방식 호출:

```qml
LV.Table {
    id: table
    rows: [
        [{ text: "A1" }, { text: "B1" }],
        [{ text: "A2" }, { text: "B2" }]
    ]

    Component.onCompleted: {
        table.appendRow()
        table.appendColumn()
        table.deleteRow(0)
        table.deleteColumn(0)
    }
}
```

런타임 구조 편집에는 수정 가능한 배열 행이 필요하다. 행/열 삽입 또는 삭제 전에 기존 span 메타데이터를 정규화하여, 구조 변경 후 오래된 병합 셀 기준점이 잘못된 행이나 열을 가리키지 않도록 한다. 열이 하나만 남은 표에서는 열 삭제를 거부한다.

삭제 상황에 맞는 메뉴는 테이블 수준 팝업이 아닙니다. 렌더링된 각 본문 셀은 자체 `ContextMenu`를 소유하고 `openContextMenuForCell(...)`는 헤더 전용 또는 행 전용 컨텍스트 요청과 같은 셀이 아닌 좌표를 거부합니다.

<a id="c-model-rows"></a>

## C++ 모델 행

`rows`는 QML에 노출된 C++ `QAbstractItemModel`를 직접 가리킬 수 있습니다.

```qml
LV.Table {
    inputable: true
    rows: nativeTableModel
    headerCellItems: [
        { label: "Name", type: "string" },
        { label: "Count", type: "int" }
    ]
}
```

최소 C++ 편집 계약은 표준 Qt 모델 계약입니다.

```cpp
QVariant data(const QModelIndex &index, int role) const override;
bool setData(const QModelIndex &index, const QVariant &value, int role) override;
Qt::ItemFlags flags(const QModelIndex &index) const override;
```

`Qt::DisplayRole` 및 `Qt::EditRole`에서 표시/편집 값을 반환하고, `Qt::ItemIsEditable`로 편집 가능한 셀을 표시하고, `setData(index, value, Qt::EditRole)`에서 업데이트를 수락합니다. 헤더 라벨은 `headerCellItems` 또는 사용자 정의 `headerColumns`가 제공되지 않는 한 수평 `headerData(..., Qt::DisplayRole)`에서 나옵니다.

<a id="undo-and-redo"></a>

## 실행 취소 및 다시 실행

`Table`는 다음을 통해 C++ 기록 스택을 노출합니다.

- `canUndo`, `canRedo`
- `undoDepth`, `redoDepth`
- `undo()`, `redo()`, `clearUndoStack()`

기록된 작업:

- `setCellValue(...)`
- 하나의 원자 연산으로 `setRangeValues(...)` 및 `pasteTsv(...)`
- `sortByColumn(...)`
- `mergeCells(...)`
- `splitCell(...)`
- `insertRow(...)`, `deleteRow(...)`
- `insertColumn(...)`, `deleteColumn(...)`
- `setColumnWidth(...)`, `setRowHeight(...)`
- `begin*/update*/end*Resize(...)`를 통해 드래그 업데이트 크기 조정

```qml
LV.Table {
    id: table
    rows: [[{ value: "Renderer" }, { value: 3 }]]

    Component.onCompleted: {
        table.setCellValue(0, 1, "4")
        table.undo()
        table.redo()
    }
}
```

<a id="resize-editing"></a>

## 크기 조정 편집

사용자는 열 오른쪽 테두리를 끌어 `columnWidths[columnIndex]`를 업데이트하고 행 아래쪽 테두리를 끌어 `rowHeights[rowIndex]`를 업데이트할 수 있습니다. 드래그 상태 및 크기 변형은 `TableModel`에 있습니다. QML는 보이는 핸들에서 정방향 포인터 좌표만 처리합니다.

프로그래밍 방식 호출은 동일한 크기 조정 경로를 사용합니다.

```qml
LV.Table {
    id: table
    rows: [
        [{ text: "A1" }, { text: "B1" }],
        [{ text: "A2" }, { text: "B2" }]
    ]

    Component.onCompleted: {
        table.setColumnWidth(0, 180)
        table.setRowHeight(1, 36)
    }
}
```

너비와 높이는 `minColumnWidth` 및 `minRowHeight`에 의해 고정됩니다. `beginColumnResize/updateColumnResize/endColumnResize` 및 해당 행이 노출되므로 테스트 또는 사용자 정의 핸들이 내장된 드래그 상태 시스템을 재사용할 수 있습니다.

<a id="advanced-example-object-rows"></a>

## 고급 예: 개체 행

```qml
import LVRS 1.0 as LV

LV.Table {
    headerCellItems: [
        { label: "Service" },
        { label: "State" },
        { label: "Owner" }
    ]
    rows: [
        [{ text: "Renderer", rowSpan: 2 }, { text: "Active" }, { text: "Core" }],
        [{ text: "Input" }, { text: "Idle" }, { text: "UX" }]
    ]
}
```

셀 텍스트 대체 경로는 `label/text/title` 개체 키를 지원합니다.

<a id="figma-contract"></a>

## Figma 계약

기본 데스크톱 표면은 노드 `203:3647` , `203:3648` , `203:3863` , 및 `203:4366` 를 따릅니다: `528 × 121` 테이블, `25` 헤더, 4 `24` 바디 행, `#1e1e1e` 바디, `panelBackground10` 바깥쪽/구분선, 12/12 설명 헤더, 및 고정된 13/13 바디 셀입니다. 독립형 헤더는 `717 × 25` , `717 × 24` 행, 및 `234 × 24` 셀입니다. 모바일은 데스크톱과 동일한 `528 × 121` 표면, `24px` 행, 및 `13px` 바디 폰트를 사용합니다.

소스 Figma 테이블 인스턴스에는 잘리거나 겹쳐진 본문 구분선이 포함되어 있습니다. 열 너비, 선택 좌표, 범위 쓰기, 크기 조정 및 외부 대리자에는 일관된 스프레드시트 그리드가 필요하기 때문에 `Table`는 의도적으로 겹치지 않는 계산 열을 유지합니다.

<a id="shared-motion"></a>

## 공유 모션

TableCellItem를 통한 선택 색조 혼합; 활성 입력은 포커스 링을 사용합니다. 크기 조정 추적은 직접적으로 유지됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
