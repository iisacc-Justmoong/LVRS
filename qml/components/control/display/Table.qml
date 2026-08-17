pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

Item {
    id: control

    enum SelectionMode {
        NoSelection,
        SingleCellSelection,
        RangeSelection
    }

    property var headerCellItems: undefined
    property var headerColumns: ["Column", "Column", "Column"]
    property var rows: [
        ["Text", "Text", "Text"],
        ["Text", "Text", "Text"],
        ["Text", "Text", "Text"],
        ["Text", "Text", "Text"]
    ]
    property alias columns: control.headerCellItems
    property alias model: control.rows
    property alias editable: control.inputable
    property alias headerDelegate: control.headerCellDelegate
    property alias delegate: control.cellDelegate

    property int rowHeight: Theme.scaleMetric(24)
    property int cellWidth: 0
    property var columnWidths: []
    property var rowHeights: []
    property int minColumnWidth: Theme.scaleMetric(32)
    property int minRowHeight: Theme.scaleMetric(18)
    property bool resizeHandlesVisible: true
    property int columnResizeHandleWidth: Theme.gap6
    property int rowResizeHandleHeight: Theme.gap6
    property color resizeHandleColor: "transparent"
    property color resizeHandleHoverColor: Theme.accentBlueMuted
    readonly property int resizingColumnIndex: tableModel.resizingColumnIndex
    readonly property int resizingRowIndex: tableModel.resizingRowIndex
    property color backgroundColor: "#1e1e1e"
    property color borderColor: Theme.panelBackground10
    property real borderWidth: Theme.strokeThin
    property color headerTextColor: Theme.descriptionColor
    property color cellTextColor: Theme.bodyColor
    property color dividerColor: Theme.panelBackground10
    property color rowDividerColor: dividerColor
    property color headerSeparatorColor: Theme.panelBackground10
    property bool inputable: false
    property bool structureControlsVisible: false
    property bool addRowControlsVisible: true
    property bool addColumnControlsVisible: true
    property bool deleteContextMenuEnabled: true
    property int structureGutterWidth: Theme.gap20
    property int structureGutterHeight: Theme.gap20
    property string defaultHeaderText: "Column"
    property string defaultCellText: "Text"
    property Component headerCellDelegate: null
    property Component cellDelegate: defaultCellDelegate
    property bool selectionEnabled: true
    property int selectionMode: Table.RangeSelection
    property bool keyboardNavigationEnabled: true
    property bool selectOnContextClick: true
    property color selectionColor: Theme.accentTint
    property color currentCellBorderColor: Theme.accent
    property real currentCellBorderWidth: Theme.strokeThin
    property bool sortingEnabled: false
    property bool sortOnHeaderClick: sortingEnabled
    property int sortColumn: -1
    property int sortOrder: Qt.AscendingOrder
    property int _currentRow: -1
    property int _currentColumn: -1
    property int _selectionAnchorRow: -1
    property int _selectionAnchorColumn: -1
    property int _selectionExtentRow: -1
    property int _selectionExtentColumn: -1
    readonly property int contextRowIndex: tableModel.contextRowIndex
    readonly property int contextColumnIndex: tableModel.contextColumnIndex
    property bool _syncingFromTableModel: false
    property bool _syncingGeometryFromTableModel: false

    signal cellInputEdited(int rowIndex, int columnIndex, string text)
    signal cellInputSubmitted(int rowIndex, int columnIndex, string text)
    signal cellInputRejected(int rowIndex, int columnIndex, string text, string valueType)
    signal cellsMerged(int rowIndex, int columnIndex, int rowSpan, int columnSpan)
    signal cellSplit(int rowIndex, int columnIndex)
    signal rowInserted(int rowIndex)
    signal rowDeleted(int rowIndex)
    signal columnInserted(int columnIndex)
    signal columnDeleted(int columnIndex)
    signal columnResized(int columnIndex, int width)
    signal rowResized(int rowIndex, int height)
    signal cellActivated(int rowIndex, int columnIndex, var cellData)
    signal selectionChanged(var range)
    signal rangeValuesChanged(var range)
    signal rowsSorted(int columnIndex, int sortOrder)

    Component {
        id: defaultCellDelegate

        TableCellItem {
            property var modelData: ({})
            property int index: modelData.index === undefined ? -1 : modelData.index

            objectName: control.objectName.length > 0
                ? control.objectName + "_cell_" + modelData.rowIndex + "_" + modelData.columnIndex
                : ""
            itemData: modelData.cellData
            text: modelData.text || ""
            cellHeight: modelData.height || control.rowHeight
            dividerColor: control.rowDividerColor
            textColor: control.cellTextColor
            clipContent: true
            inputable: modelData.inputable === true
            valueType: modelData.valueType || "string"
            selected: modelData.selected === true
            current: modelData.current === true
            selectionColor: control.selectionColor
            currentBorderColor: control.currentCellBorderColor
            currentBorderWidth: control.currentCellBorderWidth
            valueValidator: function(value) {
                return control.validateCellInput(modelData.rowIndex, modelData.columnIndex, value)
            }
            onInputEdited: function(text) {
                control.cellInputEdited(modelData.rowIndex, modelData.columnIndex, text)
            }
            onInputSubmitted: function(text) {
                control.cellInputSubmitted(modelData.rowIndex, modelData.columnIndex, text)
            }
            onInputRejected: function(text, valueType) {
                control.cellInputRejected(modelData.rowIndex, modelData.columnIndex, text, valueType)
            }
        }
    }

    TableModel {
        id: tableModel
        defaultHeaderText: control.defaultHeaderText
        defaultCellText: control.defaultCellText
        inputable: control.inputable
        tableWidth: tableFrame.width
        rowHeight: control.rowHeight
        cellWidth: control.cellWidth
        minColumnWidth: control.minColumnWidth
        minRowHeight: control.minRowHeight
        onColumnWidthsChanged: control.syncColumnWidthsFromModel()
        onRowHeightsChanged: control.syncRowHeightsFromModel()
        onRowsChanged: Qt.callLater(control.normalizeSelectionAfterModelChange)
    }

    readonly property int _modelRevision: tableModel.revision
    readonly property var resolvedHeaderSource: _modelRevision >= 0
        ? tableModel.resolvedHeaderSource()
        : []
    readonly property int resolvedRowCount: tableModel.rowCount
    readonly property int resolvedHeaderCount: tableModel.headerCount
    readonly property int resolvedColumnCount: tableModel.columnCount
    readonly property int rowCount: resolvedRowCount
    readonly property int headerCount: resolvedHeaderCount
    readonly property int columnCount: resolvedColumnCount
    readonly property bool rowsModelBacked: tableModel.rowsModelBacked
    readonly property bool cellEditingAvailable: tableModel.cellEditingAvailable
    readonly property bool sortingAvailable: tableModel.sortingAvailable
    readonly property var visibleCellItems: _modelRevision >= 0
        ? buildVisibleCellItems()
        : []
    readonly property bool structureMutationAvailable: tableModel.structureMutationAvailable
    readonly property bool canUndo: tableModel.canUndo
    readonly property bool canRedo: tableModel.canRedo
    readonly property int undoDepth: tableModel.undoDepth
    readonly property int redoDepth: tableModel.redoDepth
    readonly property int currentRow: _currentRow
    readonly property int currentColumn: _currentColumn
    readonly property bool hasSelection: _selectionAnchorRow >= 0
        && _selectionAnchorColumn >= 0
        && _selectionExtentRow >= 0
        && _selectionExtentColumn >= 0
    readonly property var selectedRange: normalizedSelectionRange()
    readonly property int selectedCellCount: hasSelection
        ? selectedRange.rowCount * selectedRange.columnCount
        : 0
    readonly property var currentCell: currentCellDescriptor()
    readonly property int resolvedBodyHeight: _modelRevision >= 0
        ? tableModel.totalBodyHeight()
        : 0
    readonly property bool resolvedStructureControlsVisible: structureControlsVisible && structureMutationAvailable
    readonly property int resolvedStructureGutterWidth: resolvedStructureControlsVisible && addRowControlsVisible
        ? structureGutterWidth
        : 0
    readonly property int resolvedStructureGutterHeight: resolvedStructureControlsVisible && addColumnControlsVisible
        ? structureGutterHeight
        : 0

    activeFocusOnTab: selectionEnabled && keyboardNavigationEnabled

    function rowAt(index) {
        return tableModel.rowAt(index)
    }

    function cellAt(rowIndex, columnIndex) {
        return tableModel.cellAt(rowIndex, columnIndex)
    }

    function headerAt(index) {
        return tableModel.headerAt(index)
    }

    function isValidCell(rowIndex, columnIndex) {
        const row = Math.floor(Number(rowIndex))
        const column = Math.floor(Number(columnIndex))
        return !isNaN(row)
            && !isNaN(column)
            && row >= 0
            && row < resolvedRowCount
            && column >= 0
            && column < resolvedColumnCount
            && column < columnCountForRow(rowAt(row))
    }

    function normalizedCell(rowIndex, columnIndex) {
        const row = Math.floor(Number(rowIndex))
        const column = Math.floor(Number(columnIndex))
        if (!isValidCell(row, column))
            return { "valid": false, "rowIndex": -1, "columnIndex": -1 }
        const anchor = mergeAnchorForCell(row, column)
        return {
            "valid": true,
            "rowIndex": anchor.rowIndex,
            "columnIndex": anchor.columnIndex
        }
    }

    function normalizedSelectionRange() {
        if (!hasSelection) {
            return {
                "valid": false,
                "startRow": -1,
                "startColumn": -1,
                "endRow": -1,
                "endColumn": -1,
                "rowCount": 0,
                "columnCount": 0
            }
        }
        const startRow = Math.min(_selectionAnchorRow, _selectionExtentRow)
        const startColumn = Math.min(_selectionAnchorColumn, _selectionExtentColumn)
        const endRow = Math.max(_selectionAnchorRow, _selectionExtentRow)
        const endColumn = Math.max(_selectionAnchorColumn, _selectionExtentColumn)
        return {
            "valid": true,
            "startRow": startRow,
            "startColumn": startColumn,
            "endRow": endRow,
            "endColumn": endColumn,
            "rowCount": endRow - startRow + 1,
            "columnCount": endColumn - startColumn + 1
        }
    }

    function currentCellDescriptor() {
        if (!isValidCell(currentRow, currentColumn)) {
            return {
                "valid": false,
                "rowIndex": -1,
                "columnIndex": -1,
                "address": "",
                "cellData": null,
                "value": undefined,
                "text": "",
                "valueType": "string"
            }
        }
        return {
            "valid": true,
            "rowIndex": currentRow,
            "columnIndex": currentColumn,
            "address": cellReference(currentRow, currentColumn),
            "cellData": cellAt(currentRow, currentColumn),
            "value": cellRawValue(currentRow, currentColumn),
            "text": cellText(currentRow, currentColumn),
            "valueType": columnType(currentColumn)
        }
    }

    function commitSelection(anchorRow, anchorColumn, extentRow, extentColumn, currentRowIndex, currentColumnIndex) {
        _selectionAnchorRow = anchorRow
        _selectionAnchorColumn = anchorColumn
        _selectionExtentRow = extentRow
        _selectionExtentColumn = extentColumn
        _currentRow = currentRowIndex
        _currentColumn = currentColumnIndex
        selectionChanged(selectedRange)
    }

    function selectCell(rowIndex, columnIndex, extendSelection) {
        if (!selectionEnabled || selectionMode === Table.NoSelection)
            return false
        const target = normalizedCell(rowIndex, columnIndex)
        if (!target.valid)
            return false

        const rowSpan = cellRowSpan(target.rowIndex, target.columnIndex)
        const columnSpan = cellColumnSpan(target.rowIndex, target.columnIndex)
        const targetEndRow = Math.min(resolvedRowCount - 1, target.rowIndex + rowSpan - 1)
        const targetEndColumn = Math.min(resolvedColumnCount - 1, target.columnIndex + columnSpan - 1)
        const extend = extendSelection === true
            && hasSelection
            && selectionMode === Table.RangeSelection
        const anchorRow = extend ? _selectionAnchorRow : target.rowIndex
        const anchorColumn = extend ? _selectionAnchorColumn : target.columnIndex
        commitSelection(anchorRow,
                        anchorColumn,
                        targetEndRow,
                        targetEndColumn,
                        target.rowIndex,
                        target.columnIndex)
        cellActivated(target.rowIndex,
                      target.columnIndex,
                      cellAt(target.rowIndex, target.columnIndex))
        return true
    }

    function selectRange(startRow, startColumn, endRow, endColumn) {
        if (!selectionEnabled || selectionMode === Table.NoSelection)
            return false
        const start = normalizedCell(startRow, startColumn)
        const end = normalizedCell(endRow, endColumn)
        if (!start.valid || !end.valid)
            return false
        if (selectionMode === Table.SingleCellSelection)
            return selectCell(end.rowIndex, end.columnIndex, false)

        const endRowSpan = cellRowSpan(end.rowIndex, end.columnIndex)
        const endColumnSpan = cellColumnSpan(end.rowIndex, end.columnIndex)
        commitSelection(start.rowIndex,
                        start.columnIndex,
                        Math.min(resolvedRowCount - 1, end.rowIndex + endRowSpan - 1),
                        Math.min(resolvedColumnCount - 1, end.columnIndex + endColumnSpan - 1),
                        end.rowIndex,
                        end.columnIndex)
        return true
    }

    function selectRow(rowIndex) {
        const row = Math.floor(Number(rowIndex))
        if (row < 0 || row >= resolvedRowCount)
            return false
        return selectRange(row, 0, row, resolvedColumnCount - 1)
    }

    function selectColumn(columnIndex) {
        const column = Math.floor(Number(columnIndex))
        if (column < 0 || column >= resolvedColumnCount)
            return false
        return selectRange(0, column, resolvedRowCount - 1, column)
    }

    function selectAll() {
        if (resolvedRowCount <= 0 || resolvedColumnCount <= 0)
            return false
        return selectRange(0, 0, resolvedRowCount - 1, resolvedColumnCount - 1)
    }

    function clearSelection() {
        const changed = hasSelection || currentRow >= 0 || currentColumn >= 0
        _selectionAnchorRow = -1
        _selectionAnchorColumn = -1
        _selectionExtentRow = -1
        _selectionExtentColumn = -1
        _currentRow = -1
        _currentColumn = -1
        if (changed)
            selectionChanged(selectedRange)
        return changed
    }

    function normalizeSelectionAfterModelChange() {
        if (!hasSelection)
            return false
        if (resolvedRowCount <= 0 || resolvedColumnCount <= 0)
            return clearSelection()

        const clampRow = function(rowIndex) {
            return Math.max(0, Math.min(resolvedRowCount - 1, rowIndex))
        }
        const clampColumn = function(rowIndex, columnIndex) {
            const available = Math.max(1, columnCountForRow(rowAt(rowIndex)))
            return Math.max(0, Math.min(available - 1, columnIndex))
        }
        const anchorRow = clampRow(_selectionAnchorRow)
        const extentRow = clampRow(_selectionExtentRow)
        const currentRowIndex = clampRow(currentRow)
        const anchorColumn = clampColumn(anchorRow, _selectionAnchorColumn)
        const extentColumn = clampColumn(extentRow, _selectionExtentColumn)
        const currentColumnIndex = clampColumn(currentRowIndex, currentColumn)
        if (anchorRow === _selectionAnchorRow
            && anchorColumn === _selectionAnchorColumn
            && extentRow === _selectionExtentRow
            && extentColumn === _selectionExtentColumn
            && currentRowIndex === currentRow
            && currentColumnIndex === currentColumn) {
            return false
        }
        commitSelection(anchorRow,
                        anchorColumn,
                        extentRow,
                        extentColumn,
                        currentRowIndex,
                        currentColumnIndex)
        return true
    }

    function isCellSelected(rowIndex, columnIndex) {
        if (!hasSelection)
            return false
        const row = Math.floor(Number(rowIndex))
        const column = Math.floor(Number(columnIndex))
        return row >= selectedRange.startRow
            && row <= selectedRange.endRow
            && column >= selectedRange.startColumn
            && column <= selectedRange.endColumn
    }

    function moveCurrentCell(rowDelta, columnDelta, extendSelection) {
        if (!selectionEnabled || resolvedRowCount <= 0 || resolvedColumnCount <= 0)
            return false
        const baseRow = currentRow >= 0 ? currentRow : 0
        const baseColumn = currentColumn >= 0 ? currentColumn : 0
        const targetRow = Math.max(0,
                                   Math.min(resolvedRowCount - 1,
                                            baseRow + Math.floor(Number(rowDelta) || 0)))
        const targetColumn = Math.max(0,
                                      Math.min(resolvedColumnCount - 1,
                                               baseColumn + Math.floor(Number(columnDelta) || 0)))
        return selectCell(targetRow, targetColumn, extendSelection === true)
    }

    function columnName(columnIndex) {
        let value = Math.floor(Number(columnIndex))
        if (isNaN(value) || value < 0)
            return ""
        let name = ""
        do {
            name = String.fromCharCode(65 + (value % 26)) + name
            value = Math.floor(value / 26) - 1
        } while (value >= 0)
        return name
    }

    function columnIndexFromName(name) {
        const text = String(name === undefined || name === null ? "" : name).trim().toUpperCase()
        if (!/^[A-Z]+$/.test(text))
            return -1
        let result = 0
        for (let i = 0; i < text.length; i++)
            result = result * 26 + (text.charCodeAt(i) - 64)
        return result - 1
    }

    function cellReference(rowIndex, columnIndex) {
        const row = Math.floor(Number(rowIndex))
        const column = Math.floor(Number(columnIndex))
        if (isNaN(row) || row < 0 || isNaN(column) || column < 0)
            return ""
        return columnName(column) + String(row + 1)
    }

    function cellCoordinates(reference) {
        const text = String(reference === undefined || reference === null ? "" : reference).trim()
        const match = /^([A-Za-z]+)([1-9][0-9]*)$/.exec(text)
        if (!match)
            return { "valid": false, "rowIndex": -1, "columnIndex": -1 }
        return {
            "valid": true,
            "rowIndex": Number(match[2]) - 1,
            "columnIndex": columnIndexFromName(match[1])
        }
    }

    function selectedCellDescriptors() {
        if (!hasSelection)
            return []
        const result = []
        for (let row = selectedRange.startRow; row <= selectedRange.endRow; row++) {
            for (let column = selectedRange.startColumn; column <= selectedRange.endColumn; column++) {
                if (!isValidCell(row, column))
                    continue
                result.push({
                    "rowIndex": row,
                    "columnIndex": column,
                    "address": cellReference(row, column),
                    "cellData": cellAt(row, column),
                    "value": cellRawValue(row, column),
                    "text": cellText(row, column),
                    "valueType": columnType(column)
                })
            }
        }
        return result
    }

    function rangeValues(startRow, startColumn, endRow, endColumn) {
        return tableModel.rangeValues(Math.floor(Number(startRow)),
                                      Math.floor(Number(startColumn)),
                                      Math.floor(Number(endRow)),
                                      Math.floor(Number(endColumn)))
    }

    function selectionValues() {
        if (!hasSelection)
            return []
        return rangeValues(selectedRange.startRow,
                           selectedRange.startColumn,
                           selectedRange.endRow,
                           selectedRange.endColumn)
    }

    function matrixDimensions(values) {
        if (!values || values.length === undefined || values.length <= 0)
            return { "rowCount": 0, "columnCount": 0 }
        const first = values[0]
        if (!first || typeof first !== "object" || first.length === undefined)
            return { "rowCount": 1, "columnCount": values.length }
        let columns = 0
        for (let row = 0; row < values.length; row++) {
            const rowValues = values[row]
            columns = Math.max(columns,
                               rowValues && rowValues.length !== undefined
                                   ? rowValues.length
                                   : 0)
        }
        return { "rowCount": values.length, "columnCount": columns }
    }

    function setRangeValues(startRow, startColumn, values) {
        const row = Math.floor(Number(startRow))
        const column = Math.floor(Number(startColumn))
        const dimensions = matrixDimensions(values)
        if (dimensions.rowCount <= 0 || dimensions.columnCount <= 0)
            return false
        if (!tableModel.setRangeValues(row, column, values))
            return false
        if (!rowsModelBacked)
            syncRowsFromModel()
        const range = {
            "valid": true,
            "startRow": row,
            "startColumn": column,
            "endRow": row + dimensions.rowCount - 1,
            "endColumn": column + dimensions.columnCount - 1,
            "rowCount": dimensions.rowCount,
            "columnCount": dimensions.columnCount
        }
        rangeValuesChanged(range)
        if (selectionEnabled)
            selectRange(range.startRow, range.startColumn, range.endRow, range.endColumn)
        return true
    }

    function setSelectionValues(values) {
        if (!hasSelection)
            return false
        return setRangeValues(selectedRange.startRow, selectedRange.startColumn, values)
    }

    function encodeTsvCell(value) {
        const text = value === undefined || value === null ? "" : String(value)
        if (!/[\t\r\n"]/.test(text))
            return text
        return "\"" + text.replace(/"/g, "\"\"") + "\""
    }

    function valuesAsTsv(values) {
        if (!values || values.length === undefined || values.length <= 0)
            return ""
        const first = values[0]
        const rows = first && typeof first === "object" && first.length !== undefined
            ? values
            : [values]
        const lines = []
        for (let row = 0; row < rows.length; row++) {
            const cells = []
            const rowValues = rows[row]
            for (let column = 0; column < rowValues.length; column++)
                cells.push(encodeTsvCell(rowValues[column]))
            lines.push(cells.join("\t"))
        }
        return lines.join("\n")
    }

    function selectionAsTsv() {
        return valuesAsTsv(selectionValues())
    }

    function parseTsv(text) {
        const source = String(text === undefined || text === null ? "" : text)
        const rows = []
        let row = []
        let cell = ""
        let quoted = false
        for (let index = 0; index < source.length; index++) {
            const character = source[index]
            if (quoted) {
                if (character === "\"") {
                    if (index + 1 < source.length && source[index + 1] === "\"") {
                        cell += "\""
                        index += 1
                    } else {
                        quoted = false
                    }
                } else {
                    cell += character
                }
                continue
            }
            if (character === "\"" && cell.length === 0) {
                quoted = true
            } else if (character === "\t") {
                row.push(cell)
                cell = ""
            } else if (character === "\n" || character === "\r") {
                if (character === "\r" && index + 1 < source.length && source[index + 1] === "\n")
                    index += 1
                row.push(cell)
                rows.push(row)
                row = []
                cell = ""
            } else {
                cell += character
            }
        }
        row.push(cell)
        rows.push(row)
        if (rows.length > 1
            && rows[rows.length - 1].length === 1
            && rows[rows.length - 1][0] === "")
            rows.pop()
        return rows
    }

    function pasteTsv(startRow, startColumn, text) {
        return setRangeValues(startRow, startColumn, parseTsv(text))
    }

    function pasteSelectionTsv(text) {
        if (!hasSelection)
            return false
        return pasteTsv(selectedRange.startRow, selectedRange.startColumn, text)
    }

    function sortByColumn(columnIndex, order) {
        const column = Math.floor(Number(columnIndex))
        const requestedOrder = order === undefined ? Qt.AscendingOrder : Number(order)
        if (!sortingAvailable || !columnSortable(column))
            return false
        if (!tableModel.sortRows(column, requestedOrder))
            return false
        if (!rowsModelBacked)
            syncRowsFromModel()
        sortColumn = column
        sortOrder = requestedOrder
        rowsSorted(column, requestedOrder)
        return true
    }

    function sortAscending(columnIndex) {
        return sortByColumn(columnIndex, Qt.AscendingOrder)
    }

    function sortDescending(columnIndex) {
        return sortByColumn(columnIndex, Qt.DescendingOrder)
    }

    function columnSortable(columnIndex) {
        if (columnIndex < 0 || columnIndex >= resolvedColumnCount)
            return false
        const header = headerAt(columnIndex)
        return !(header && typeof header === "object" && header.sortable === false)
    }

    function toggleSortForColumn(columnIndex) {
        const nextOrder = sortColumn === columnIndex && sortOrder === Qt.AscendingOrder
            ? Qt.DescendingOrder
            : Qt.AscendingOrder
        return sortByColumn(columnIndex, nextOrder)
    }

    function columnCountForRow(rowEntry) {
        return tableModel.columnCountForRow(rowEntry)
    }

    function autoCellWidth(rowEntry) {
        return tableModel.autoCellWidth(rowEntry)
    }

    function numericSize(value, fallbackValue, minimumValue) {
        return tableModel.numericSize(value, fallbackValue, minimumValue)
    }

    function autoColumnWidth() {
        return tableModel.autoColumnWidth()
    }

    function columnWidth(columnIndex) {
        return tableModel.columnWidth(columnIndex)
    }

    function columnX(columnIndex) {
        return tableModel.columnX(columnIndex)
    }

    function columnSpanWidth(columnIndex, columnSpan) {
        return tableModel.columnSpanWidth(columnIndex, columnSpan)
    }

    function rowHeightAt(rowIndex) {
        return tableModel.rowHeightAt(rowIndex)
    }

    function rowY(rowIndex) {
        return tableModel.rowY(rowIndex)
    }

    function rowSpanHeight(rowIndex, rowSpan) {
        return tableModel.rowSpanHeight(rowIndex, rowSpan)
    }

    function totalBodyHeight() {
        return tableModel.totalBodyHeight()
    }

    function rowCellWidth(rowEntry) {
        return tableModel.rowCellWidth(rowEntry)
    }

    function rowCellSpacing(rowEntry) {
        return tableModel.rowCellSpacing(rowEntry)
    }

    function structureCellWidth(columnIndex) {
        return tableModel.structureCellWidth(columnIndex === undefined || columnIndex === null ? 0 : columnIndex)
    }

    function structureCellSpacing() {
        return tableModel.structureCellSpacing()
    }

    function structureColumnX(columnIndex) {
        return tableModel.structureColumnX(columnIndex)
    }

    function cellX(rowEntry, columnIndex) {
        return tableModel.cellX(rowEntry, columnIndex)
    }

    function cellSpanWidth(rowEntry, columnSpan) {
        return tableModel.cellSpanWidth(rowEntry, columnSpan)
    }

    function rowInputable(rowEntry) {
        return tableModel.rowInputable(rowEntry)
    }

    function cellInputable(rowIndex, columnIndex) {
        return tableModel.cellInputable(rowIndex, columnIndex)
    }

    function normalizeHeaderCellType(value) {
        return tableModel.normalizeHeaderCellType(value)
    }

    function inferredCellType(value) {
        return tableModel.inferredCellType(value)
    }

    function headerCellType(columnIndex) {
        return tableModel.headerCellType(columnIndex)
    }

    function columnType(columnIndex) {
        return tableModel.columnType(columnIndex)
    }

    function cellRawValue(rowIndex, columnIndex) {
        return tableModel.cellRawValue(rowIndex, columnIndex)
    }

    function typedDefaultValue(valueType) {
        return tableModel.typedDefaultValue(valueType)
    }

    function coerceCellValue(value, valueType) {
        return tableModel.coerceCellValue(value, valueType)
    }

    function validateCellInput(rowIndex, columnIndex, value) {
        return tableModel.validateCellInput(rowIndex, columnIndex, value)
    }

    function cellValueAccepted(rowIndex, columnIndex, value) {
        return tableModel.cellValueAccepted(rowIndex, columnIndex, value)
    }

    function cellText(rowIndex, columnIndex) {
        return tableModel.cellText(rowIndex, columnIndex)
    }

    function setCellValue(rowIndex, columnIndex, value) {
        const result = tableModel.validateCellInput(rowIndex, columnIndex, value)
        if (!tableModel.setCellValue(rowIndex, columnIndex, value)) {
            if (!result.accepted)
                console.warn("LVRS Table.setCellValue rejected value for " + result.type + " column.")
            return false
        }
        if (!rowsModelBacked)
            syncRowsFromModel()
        return true
    }

    function cellRowSpan(rowIndex, columnIndex) {
        return tableModel.cellRowSpan(rowIndex, columnIndex)
    }

    function cellColumnSpan(rowIndex, columnIndex) {
        return tableModel.cellColumnSpan(rowIndex, columnIndex)
    }

    function mergeAnchorForCell(rowIndex, columnIndex) {
        return tableModel.mergeAnchorForCell(rowIndex, columnIndex)
    }

    function isCoveredCell(rowIndex, columnIndex) {
        return tableModel.isCoveredCell(rowIndex, columnIndex)
    }

    function buildVisibleCellItems() {
        tableModel.revision
        tableModel.rowCount
        tableModel.columnCount
        tableModel.headerCount
        const source = tableModel.visibleCells()
        const result = []
        for (let index = 0; index < source.length; index++) {
            const sourceDescriptor = source[index]
            const descriptor = {}
            for (const key in sourceDescriptor)
                descriptor[key] = sourceDescriptor[key]
            descriptor.selected = isCellSelected(descriptor.rowIndex, descriptor.columnIndex)
            descriptor.current = currentRow === descriptor.rowIndex
                && currentColumn === descriptor.columnIndex
            descriptor.address = cellReference(descriptor.rowIndex, descriptor.columnIndex)
            result.push(descriptor)
        }
        return result
    }

    function syncRowsFromModel() {
        if (rowsModelBacked)
            return
        _syncingFromTableModel = true
        rows = tableModel.rows
        _syncingFromTableModel = false
    }

    function syncHeadersFromModel() {
        _syncingFromTableModel = true
        if (headerCellItems !== undefined && headerCellItems !== null)
            headerCellItems = tableModel.headerCellItems
        else
            headerColumns = tableModel.headerColumns
        _syncingFromTableModel = false
    }

    function syncRowsToModel() {
        if (!_syncingFromTableModel)
            tableModel.rows = rows
    }

    function syncHeadersToModel() {
        if (_syncingFromTableModel)
            return
        tableModel.headerCellItems = headerCellItems
        tableModel.headerColumns = headerColumns
    }

    function syncColumnWidthsFromModel() {
        _syncingGeometryFromTableModel = true
        columnWidths = tableModel.columnWidths
        _syncingGeometryFromTableModel = false
    }

    function syncRowHeightsFromModel() {
        _syncingGeometryFromTableModel = true
        rowHeights = tableModel.rowHeights
        _syncingGeometryFromTableModel = false
    }

    function syncColumnWidthsToModel() {
        if (!_syncingGeometryFromTableModel)
            tableModel.columnWidths = columnWidths
    }

    function syncRowHeightsToModel() {
        if (!_syncingGeometryFromTableModel)
            tableModel.rowHeights = rowHeights
    }

    function refreshRows() {
        if (rowsModelBacked)
            syncRowsToModel()
        else
            syncRowsFromModel()
    }

    function refreshColumnWidths() {
        syncColumnWidthsFromModel()
    }

    function refreshRowHeights() {
        syncRowHeightsFromModel()
    }

    function normalizedColumnWidths() {
        return tableModel.normalizedColumnWidths()
    }

    function normalizedRowHeights() {
        return tableModel.normalizedRowHeights()
    }

    function setColumnWidth(columnIndex, widthValue) {
        if (!tableModel.setColumnWidth(columnIndex, widthValue))
            return false
        const targetColumn = Math.floor(Number(columnIndex))
        syncColumnWidthsFromModel()
        columnResized(targetColumn, tableModel.columnWidth(targetColumn))
        return true
    }

    function setRowHeight(rowIndex, heightValue) {
        if (!tableModel.setRowHeight(rowIndex, heightValue))
            return false
        const targetRow = Math.floor(Number(rowIndex))
        syncRowHeightsFromModel()
        rowResized(targetRow, tableModel.rowHeightAt(targetRow))
        return true
    }

    function beginColumnResize(columnIndex, pointerX) {
        return tableModel.beginColumnResize(columnIndex, pointerX)
    }

    function updateColumnResize(pointerX) {
        const targetColumn = tableModel.resizingColumnIndex
        if (!tableModel.updateColumnResize(pointerX))
            return false
        syncColumnWidthsFromModel()
        columnResized(targetColumn, tableModel.columnWidth(targetColumn))
        return true
    }

    function endColumnResize() {
        tableModel.endColumnResize()
    }

    function beginRowResize(rowIndex, pointerY) {
        return tableModel.beginRowResize(rowIndex, pointerY)
    }

    function updateRowResize(pointerY) {
        const targetRow = tableModel.resizingRowIndex
        if (!tableModel.updateRowResize(pointerY))
            return false
        syncRowHeightsFromModel()
        rowResized(targetRow, tableModel.rowHeightAt(targetRow))
        return true
    }

    function endRowResize() {
        tableModel.endRowResize()
    }

    function refreshHeaderColumns() {
        syncHeadersFromModel()
    }

    function undo() {
        if (!tableModel.undo())
            return false
        if (!rowsModelBacked)
            syncRowsFromModel()
        syncHeadersFromModel()
        syncColumnWidthsFromModel()
        syncRowHeightsFromModel()
        return true
    }

    function redo() {
        if (!tableModel.redo())
            return false
        if (!rowsModelBacked)
            syncRowsFromModel()
        syncHeadersFromModel()
        syncColumnWidthsFromModel()
        syncRowHeightsFromModel()
        return true
    }

    function clearUndoStack() {
        tableModel.clearUndoStack()
    }

    Component.onCompleted: {
        syncRowsToModel()
        syncHeadersToModel()
        syncColumnWidthsToModel()
        syncRowHeightsToModel()
    }

    onRowsChanged: syncRowsToModel()
    onHeaderCellItemsChanged: syncHeadersToModel()
    onHeaderColumnsChanged: syncHeadersToModel()
    onColumnWidthsChanged: syncColumnWidthsToModel()
    onRowHeightsChanged: syncRowHeightsToModel()
    onRowHeightChanged: tableModel.rowHeight = rowHeight
    onCellWidthChanged: tableModel.cellWidth = cellWidth
    onMinColumnWidthChanged: tableModel.minColumnWidth = minColumnWidth
    onMinRowHeightChanged: tableModel.minRowHeight = minRowHeight

    function canMutateStructure() {
        return tableModel.structureMutationAvailable
    }

    function canInsertRow(rowIndex) {
        return tableModel.canInsertRow(rowIndex)
    }

    function insertRow(rowIndex) {
        const targetRow = Math.floor(Number(rowIndex))
        if (!tableModel.insertRow(targetRow)) {
            console.warn("LVRS Table.insertRow requires array rows and an in-bounds row index.")
            return false
        }

        syncRowsFromModel()
        syncRowHeightsFromModel()
        rowInserted(targetRow)
        return true
    }

    function appendRow() {
        const targetRow = resolvedRowCount
        if (!tableModel.appendRow()) {
            console.warn("LVRS Table.appendRow requires array rows.")
            return false
        }
        syncRowsFromModel()
        syncRowHeightsFromModel()
        rowInserted(targetRow)
        return true
    }

    function canDeleteRow(rowIndex) {
        return tableModel.canDeleteRow(rowIndex)
    }

    function deleteRow(rowIndex) {
        const targetRow = Math.floor(Number(rowIndex))
        if (!tableModel.deleteRow(targetRow)) {
            console.warn("LVRS Table.deleteRow requires array rows and an in-bounds row index.")
            return false
        }

        syncRowsFromModel()
        syncRowHeightsFromModel()
        rowDeleted(targetRow)
        return true
    }

    function removeRow(rowIndex) {
        return deleteRow(rowIndex)
    }

    function canInsertColumn(columnIndex) {
        return tableModel.canInsertColumn(columnIndex)
    }

    function insertColumn(columnIndex) {
        const targetColumn = Math.floor(Number(columnIndex))
        if (!tableModel.insertColumn(targetColumn)) {
            console.warn("LVRS Table.insertColumn requires array rows and an in-bounds column index.")
            return false
        }

        syncRowsFromModel()
        syncHeadersFromModel()
        syncColumnWidthsFromModel()

        columnInserted(targetColumn)
        return true
    }

    function appendColumn() {
        const targetColumn = resolvedColumnCount
        if (!tableModel.appendColumn()) {
            console.warn("LVRS Table.appendColumn requires array rows.")
            return false
        }
        syncRowsFromModel()
        syncHeadersFromModel()
        syncColumnWidthsFromModel()
        columnInserted(targetColumn)
        return true
    }

    function canDeleteColumn(columnIndex) {
        return tableModel.canDeleteColumn(columnIndex)
    }

    function deleteColumn(columnIndex) {
        const targetColumn = Math.floor(Number(columnIndex))
        if (!tableModel.deleteColumn(targetColumn)) {
            console.warn("LVRS Table.deleteColumn requires array rows, at least two columns, and an in-bounds column index.")
            return false
        }

        syncRowsFromModel()
        syncHeadersFromModel()
        syncColumnWidthsFromModel()

        columnDeleted(targetColumn)
        return true
    }

    function removeColumn(columnIndex) {
        return deleteColumn(columnIndex)
    }

    function buildContextMenuItems(rowIndex, columnIndex) {
        const targetRow = rowIndex === undefined || rowIndex === null
            ? contextRowIndex
            : Math.floor(Number(rowIndex))
        const targetColumn = columnIndex === undefined || columnIndex === null
            ? contextColumnIndex
            : Math.floor(Number(columnIndex))
        const items = []
        if (isNaN(targetRow) || isNaN(targetColumn))
            return items
        const descriptors = tableModel.contextMenuDescriptors(targetRow, targetColumn)
        for (let i = 0; i < descriptors.length; i++) {
            const descriptor = descriptors[i]
            if (descriptor.type === "divider") {
                items.push({ "type": "divider" })
                continue
            }
            const action = descriptor.action
            const actionRow = descriptor.rowIndex
            const actionColumn = descriptor.columnIndex
            items.push({
                "label": descriptor.label,
                "eventName": descriptor.eventName,
                "enabled": descriptor.enabled,
                "onTriggered": function() {
                    control.triggerContextAction(action, actionRow, actionColumn)
                }
            })
        }
        return items
    }

    function triggerContextAction(action, rowIndex, columnIndex) {
        const result = tableModel.triggerContextAction(action, rowIndex, columnIndex)
        if (!result.accepted)
            return false
        syncRowsFromModel()
        syncHeadersFromModel()
        syncColumnWidthsFromModel()
        syncRowHeightsFromModel()
        if (result.action === "deleteRow")
            rowDeleted(result.rowIndex)
        else if (result.action === "deleteColumn")
            columnDeleted(result.columnIndex)
        return true
    }

    function openContextMenuForCell(rowIndex, columnIndex, menu, item, xPos, yPos) {
        if (!deleteContextMenuEnabled)
            return false
        const targetRow = Math.floor(Number(rowIndex))
        const targetColumn = Math.floor(Number(columnIndex))
        if (isNaN(targetRow) || isNaN(targetColumn))
            return false
        if (!tableModel.setContextCell(targetRow, targetColumn))
            return false

        if (!menu)
            return true

        menu.items = buildContextMenuItems(targetRow, targetColumn)
        if (item)
            menu.openFor(item, xPos, yPos)
        else
            menu.openAt(xPos, yPos)
        return true
    }

    function canMergeCells(rowIndex, columnIndex, rowSpan, columnSpan) {
        return tableModel.canMergeCells(rowIndex, columnIndex, rowSpan, columnSpan)
    }

    function mergeCells(rowIndex, columnIndex, rowSpan, columnSpan) {
        const anchorRow = Math.floor(Number(rowIndex))
        const anchorColumn = Math.floor(Number(columnIndex))
        const spanRows = Math.floor(Number(rowSpan))
        const spanColumns = Math.floor(Number(columnSpan))
        if (!tableModel.mergeCells(anchorRow, anchorColumn, spanRows, spanColumns)) {
            console.warn("LVRS Table.mergeCells requires array rows and an in-bounds rectangular cell range.")
            return false
        }

        syncRowsFromModel()
        cellsMerged(anchorRow, anchorColumn, spanRows, spanColumns)
        return true
    }

    function splitCell(rowIndex, columnIndex) {
        const parsedRow = Math.floor(Number(rowIndex))
        const parsedColumn = Math.floor(Number(columnIndex))
        if (isNaN(parsedRow) || isNaN(parsedColumn) || parsedRow < 0 || parsedColumn < 0)
            return false
        const anchor = mergeAnchorForCell(parsedRow, parsedColumn)
        if (!tableModel.splitCell(parsedRow, parsedColumn)) {
            console.warn("LVRS Table.splitCell requires array rows and an in-bounds cell.")
            return false
        }
        syncRowsFromModel()
        cellSplit(anchor.rowIndex, anchor.columnIndex)
        return true
    }

    implicitWidth: Theme.scaleMetric(528) + resolvedStructureGutterWidth
    implicitHeight: tableHeader.implicitHeight + resolvedBodyHeight + resolvedStructureGutterHeight

    Rectangle {
        id: tableFrame
        objectName: control.objectName.length > 0 ? control.objectName + "_frame" : ""
        x: 0
        y: 0
        width: Math.max(0, control.width - control.resolvedStructureGutterWidth)
        height: Math.max(0, control.height - control.resolvedStructureGutterHeight)
        color: control.backgroundColor
        border.color: control.borderColor
        border.width: control.borderWidth
        antialiasing: false
        clip: true

        Column {
            anchors.fill: parent
            spacing: 0

            TableHeader {
                id: tableHeader
                objectName: control.objectName.length > 0 ? control.objectName + "_header" : ""
                width: tableFrame.width
                cellItems: control.resolvedHeaderSource
                columnWidths: control.columnWidths
                fallbackCellWidth: control.cellWidth
                minColumnWidth: control.minColumnWidth
                textColor: control.headerTextColor
                separatorColor: control.headerSeparatorColor
                cellDelegate: control.headerCellDelegate
                interactive: control.sortOnHeaderClick && control.sortingAvailable
                sortColumn: control.sortColumn
                sortOrder: control.sortOrder
                onColumnClicked: function(columnIndex, columnData) {
                    control.toggleSortForColumn(columnIndex)
                }
            }

            Item {
                id: bodyLayer
                width: tableFrame.width
                height: control.resolvedBodyHeight

                Repeater {
                    model: control.visibleCellItems

                    delegate: Item {
                        id: cellDelegateRoot
                        required property int index
                        required property var modelData
                        property Item delegateItem: null

                        x: modelData.x
                        y: modelData.y
                        width: modelData.width
                        height: modelData.height

                        function rebuildDelegate() {
                            if (delegateItem) {
                                delegateItem.destroy()
                                delegateItem = null
                            }
                            if (!control.cellDelegate)
                                return
                            delegateItem = control.cellDelegate.createObject(cellDelegateRoot, {
                                "modelData": modelData
                            })
                            if (!delegateItem)
                                return
                            delegateItem.width = Qt.binding(function() { return cellDelegateRoot.width })
                            delegateItem.height = Qt.binding(function() { return cellDelegateRoot.height })
                        }

                        Component.onCompleted: rebuildDelegate()
                        onModelDataChanged: rebuildDelegate()

                        Connections {
                            target: control
                            function onCellDelegateChanged() {
                                cellDelegateRoot.rebuildDelegate()
                            }
                        }

                        TapHandler {
                            enabled: control.selectionEnabled
                                && control.selectionMode !== Table.NoSelection
                            acceptedButtons: Qt.LeftButton
                            acceptedModifiers: Qt.NoModifier
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            onTapped: {
                                control.forceActiveFocus()
                                control.selectCell(modelData.rowIndex, modelData.columnIndex, false)
                            }
                        }

                        TapHandler {
                            enabled: control.selectionEnabled
                                && control.selectionMode === Table.RangeSelection
                            acceptedButtons: Qt.LeftButton
                            acceptedModifiers: Qt.ShiftModifier
                            gesturePolicy: TapHandler.ReleaseWithinBounds
                            onTapped: {
                                control.forceActiveFocus()
                                control.selectCell(modelData.rowIndex, modelData.columnIndex, true)
                            }
                        }

                        MouseArea {
                            id: cellContextArea
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            enabled: control.deleteContextMenuEnabled && control.structureMutationAvailable
                            preventStealing: true

                            onPressed: function(mouse) {
                                if (control.selectOnContextClick)
                                    control.selectCell(modelData.rowIndex, modelData.columnIndex, false)
                                control.forceActiveFocus()
                                control.openContextMenuForCell(modelData.rowIndex,
                                                               modelData.columnIndex,
                                                               cellDeleteContextMenu,
                                                               cellContextArea,
                                                               mouse.x,
                                                               mouse.y)
                                mouse.accepted = true
                            }
                        }

                        ContextMenu {
                            id: cellDeleteContextMenu
                            itemWidth: Theme.scaleMetric(132)
                            items: control.buildContextMenuItems(modelData.rowIndex, modelData.columnIndex)
                        }
                    }
                }
            }
        }

        Item {
            id: resizeHandleLayer
            anchors.fill: parent
            visible: control.resizeHandlesVisible
            z: 20

            Repeater {
                model: Math.max(1, control.resolvedColumnCount)

                delegate: MouseArea {
                    id: columnResizeArea
                    required property int index
                    property bool hoveredNow: containsMouse || control.resizingColumnIndex === index

                    x: control.columnX(index) + control.columnWidth(index) - (width * 0.5)
                    y: 0
                    width: control.columnResizeHandleWidth
                    height: tableFrame.height
                    acceptedButtons: Qt.LeftButton
                    hoverEnabled: true
                    cursorShape: Qt.SplitHCursor
                    preventStealing: true

                    Rectangle {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: Theme.strokeThin
                        color: columnResizeArea.hoveredNow
                            ? control.resizeHandleHoverColor
                            : control.resizeHandleColor
                    }

                    onPressed: function(mouse) {
                        const point = columnResizeArea.mapToItem(tableFrame, mouse.x, mouse.y)
                        control.beginColumnResize(index, point.x)
                        mouse.accepted = true
                    }

                    onPositionChanged: function(mouse) {
                        if (!pressed)
                            return
                        const point = columnResizeArea.mapToItem(tableFrame, mouse.x, mouse.y)
                        control.updateColumnResize(point.x)
                    }

                    onReleased: control.endColumnResize()
                    onCanceled: control.endColumnResize()
                }
            }

            Repeater {
                model: control.resolvedRowCount

                delegate: MouseArea {
                    id: rowResizeArea
                    required property int index
                    property bool hoveredNow: containsMouse || control.resizingRowIndex === index

                    x: 0
                    y: tableHeader.implicitHeight + control.rowY(index) + control.rowHeightAt(index) - (height * 0.5)
                    width: tableFrame.width
                    height: control.rowResizeHandleHeight
                    acceptedButtons: Qt.LeftButton
                    hoverEnabled: true
                    cursorShape: Qt.SplitVCursor
                    preventStealing: true

                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: Theme.strokeThin
                        color: rowResizeArea.hoveredNow
                            ? control.resizeHandleHoverColor
                            : control.resizeHandleColor
                    }

                    onPressed: function(mouse) {
                        const point = rowResizeArea.mapToItem(tableFrame, mouse.x, mouse.y)
                        control.beginRowResize(index, point.y)
                        mouse.accepted = true
                    }

                    onPositionChanged: function(mouse) {
                        if (!pressed)
                            return
                        const point = rowResizeArea.mapToItem(tableFrame, mouse.x, mouse.y)
                        control.updateRowResize(point.y)
                    }

                    onReleased: control.endRowResize()
                    onCanceled: control.endRowResize()
                }
            }
        }
    }

    Keys.onPressed: function(event) {
        if (!keyboardNavigationEnabled || !selectionEnabled)
            return
        const extend = (event.modifiers & Qt.ShiftModifier) !== 0
        if ((event.modifiers & (Qt.ControlModifier | Qt.MetaModifier)) !== 0
            && event.key === Qt.Key_A) {
            event.accepted = selectAll()
        } else if (event.key === Qt.Key_Left) {
            event.accepted = moveCurrentCell(0, -1, extend)
        } else if (event.key === Qt.Key_Right) {
            event.accepted = moveCurrentCell(0, 1, extend)
        } else if (event.key === Qt.Key_Up) {
            event.accepted = moveCurrentCell(-1, 0, extend)
        } else if (event.key === Qt.Key_Down) {
            event.accepted = moveCurrentCell(1, 0, extend)
        } else if (event.key === Qt.Key_Home) {
            if (resolvedRowCount > 0 && resolvedColumnCount > 0) {
                const targetRow = currentRow >= 0 ? currentRow : 0
                event.accepted = selectCell(targetRow, 0, extend)
            }
        } else if (event.key === Qt.Key_End) {
            if (resolvedRowCount > 0 && resolvedColumnCount > 0) {
                const targetRow = currentRow >= 0 ? currentRow : 0
                event.accepted = selectCell(targetRow, resolvedColumnCount - 1, extend)
            }
        } else if (event.key === Qt.Key_Escape) {
            event.accepted = clearSelection()
        }
    }

    Repeater {
        model: control.resolvedStructureControlsVisible && control.addRowControlsVisible
            ? control.resolvedRowCount
            : 0

        delegate: LabelButton {
            required property int index

            x: tableFrame.x + tableFrame.width
            y: tableFrame.y + tableHeader.implicitHeight + control.rowY(index)
            width: control.resolvedStructureGutterWidth
            height: control.rowHeightAt(index)
            text: "+"
            tone: AbstractButton.Borderless
            horizontalPadding: 0
            verticalPadding: 0
            onClicked: control.insertRow(index + 1)
        }
    }

    Repeater {
        model: control.resolvedStructureControlsVisible && control.addColumnControlsVisible
            ? Math.max(1, control.resolvedColumnCount)
            : 0

        delegate: LabelButton {
            required property int index

            x: tableFrame.x + control.structureColumnX(index)
            y: tableFrame.y + tableHeader.implicitHeight + control.resolvedBodyHeight
            width: control.structureCellWidth(index)
            height: control.resolvedStructureGutterHeight
            text: "+"
            tone: AbstractButton.Borderless
            horizontalPadding: 0
            verticalPadding: 0
            onClicked: control.insertColumn(index + 1)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Table { columns: [{ label: "Name" }, { label: "Count", type: "int" }]; model: [["Renderer", 3]]; editable: true; sortingEnabled: true }
