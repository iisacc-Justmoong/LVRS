pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

Item {
    id: control

    property var headerCellItems: undefined
    property var headerColumns: ["Column", "Column", "Column"]
    property var rows: [
        ["Text", "Text", "Text"],
        ["Text", "Text", "Text"],
        ["Text", "Text", "Text"],
        ["Text", "Text", "Text"]
    ]

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
    property int resizingColumnIndex: -1
    property int resizingRowIndex: -1
    property real _resizeStartPointerX: 0
    property real _resizeStartPointerY: 0
    property real _resizeStartColumnWidth: 0
    property real _resizeStartRowHeight: 0
    property color backgroundColor: "#282828"
    property color borderColor: Theme.surface
    property real borderWidth: Theme.strokeThin
    property color headerTextColor: Theme.descriptionColor
    property color cellTextColor: Theme.bodyColor
    property color dividerColor: Theme.panelBackground03
    property color rowDividerColor: dividerColor
    property color headerSeparatorColor: Theme.panelBackground10
    property bool inputable: false
    property bool structureControlsVisible: true
    property bool addRowControlsVisible: true
    property bool addColumnControlsVisible: true
    property bool deleteContextMenuEnabled: true
    property int structureGutterWidth: Theme.gap20
    property int structureGutterHeight: Theme.gap20
    property string defaultHeaderText: "Column"
    property string defaultCellText: "Text"
    property int contextRowIndex: -1
    property int contextColumnIndex: -1

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

    readonly property var resolvedHeaderSource: {
        if (control.headerCellItems !== undefined && control.headerCellItems !== null)
            return control.headerCellItems
        return control.headerColumns
    }

    readonly property int resolvedRowCount: {
        if (!rows)
            return 0
        if (rows.length !== undefined)
            return rows.length
        if (rows.count !== undefined)
            return rows.count
        return 0
    }
    readonly property int resolvedHeaderCount: {
        const source = control.resolvedHeaderSource
        if (!source)
            return 0
        if (source.length !== undefined)
            return source.length
        if (source.count !== undefined)
            return source.count
        return 0
    }
    readonly property int resolvedColumnCount: maxColumnCount()
    readonly property var visibleCellItems: buildVisibleCellItems()
    readonly property bool structureMutationAvailable: canMutateStructure()
    readonly property bool resolvedStructureControlsVisible: structureControlsVisible && structureMutationAvailable
    readonly property int resolvedStructureGutterWidth: resolvedStructureControlsVisible && addRowControlsVisible
        ? structureGutterWidth
        : 0
    readonly property int resolvedStructureGutterHeight: resolvedStructureControlsVisible && addColumnControlsVisible
        ? structureGutterHeight
        : 0

    function rowAt(index) {
        if (!rows)
            return null
        if (rows.length !== undefined)
            return rows[index]
        if (rows.get !== undefined)
            return rows.get(index)
        return null
    }

    function cellAt(rowIndex, columnIndex) {
        const rowEntry = rowAt(rowIndex)
        if (!rowEntry)
            return null
        if (rowEntry.length !== undefined)
            return rowEntry[columnIndex]
        if (rowEntry.get !== undefined)
            return rowEntry.get(columnIndex)
        return null
    }

    function headerAt(index) {
        const source = control.resolvedHeaderSource
        if (!source)
            return null
        if (source.length !== undefined)
            return source[index]
        if (source.get !== undefined)
            return source.get(index)
        return null
    }

    function columnCountForRow(rowEntry) {
        if (rowEntry) {
            if (rowEntry.length !== undefined)
                return Math.max(1, rowEntry.length)
            if (rowEntry.count !== undefined)
                return Math.max(1, rowEntry.count)
        }
        return Math.max(1, resolvedHeaderCount)
    }

    function autoCellWidth(rowEntry) {
        const count = columnCountForRow(rowEntry)
        return Math.max(1, Math.floor(tableFrame.width / count))
    }

    function numericSize(value, fallbackValue, minimumValue) {
        const parsed = Number(value)
        if (isNaN(parsed) || parsed <= 0)
            return Math.max(1, Math.round(fallbackValue))
        return Math.max(minimumValue, Math.round(parsed))
    }

    function autoColumnWidth() {
        const count = Math.max(1, resolvedColumnCount)
        return Math.max(minColumnWidth, Math.floor(tableFrame.width / count))
    }

    function columnWidth(columnIndex) {
        if (Array.isArray(columnWidths) && columnIndex >= 0 && columnIndex < columnWidths.length)
            return numericSize(columnWidths[columnIndex], cellWidth > 0 ? cellWidth : autoColumnWidth(), minColumnWidth)
        if (cellWidth > 0)
            return numericSize(cellWidth, autoColumnWidth(), minColumnWidth)
        return autoColumnWidth()
    }

    function columnX(columnIndex) {
        let xValue = 0
        for (let column = 0; column < columnIndex; column++)
            xValue += columnWidth(column)
        return xValue
    }

    function columnSpanWidth(columnIndex, columnSpan) {
        const spanValue = Math.max(1, columnSpan)
        let widthValue = 0
        for (let column = columnIndex; column < columnIndex + spanValue; column++)
            widthValue += columnWidth(column)
        return widthValue
    }

    function rowHeightAt(rowIndex) {
        if (Array.isArray(rowHeights) && rowIndex >= 0 && rowIndex < rowHeights.length)
            return numericSize(rowHeights[rowIndex], rowHeight, minRowHeight)
        return numericSize(rowHeight, rowHeight, minRowHeight)
    }

    function rowY(rowIndex) {
        let yValue = 0
        for (let row = 0; row < rowIndex; row++)
            yValue += rowHeightAt(row)
        return yValue
    }

    function rowSpanHeight(rowIndex, rowSpan) {
        const spanValue = Math.max(1, rowSpan)
        let heightValue = 0
        for (let row = rowIndex; row < rowIndex + spanValue; row++)
            heightValue += rowHeightAt(row)
        return heightValue
    }

    function totalBodyHeight() {
        let heightValue = 0
        for (let row = 0; row < resolvedRowCount; row++)
            heightValue += rowHeightAt(row)
        return heightValue
    }

    function rowCellWidth(rowEntry) {
        return control.cellWidth > 0 ? control.cellWidth : control.autoCellWidth(rowEntry)
    }

    function rowCellSpacing(rowEntry) {
        const count = columnCountForRow(rowEntry)
        if (count <= 1)
            return 0
        const widthValue = rowCellWidth(rowEntry)
        return Math.max(0, (tableFrame.width - (count * widthValue)) / (count - 1))
    }

    function structureCellWidth(columnIndex) {
        const targetColumn = columnIndex === undefined || columnIndex === null ? 0 : Math.max(0, Math.floor(Number(columnIndex)))
        return columnWidth(targetColumn)
    }

    function structureCellSpacing() {
        return 0
    }

    function structureColumnX(columnIndex) {
        return columnX(columnIndex)
    }

    function cellX(rowEntry, columnIndex) {
        return columnX(columnIndex)
    }

    function cellSpanWidth(rowEntry, columnSpan) {
        return columnSpanWidth(0, columnSpan)
    }

    function rowInputable(rowEntry) {
        if (rowEntry && typeof rowEntry === "object" && Object.prototype.hasOwnProperty.call(rowEntry, "inputable"))
            return !!rowEntry.inputable
        return control.inputable
    }

    function cellInputable(rowIndex, columnIndex) {
        const cellEntry = cellAt(rowIndex, columnIndex)
        if (cellEntry && typeof cellEntry === "object" && Object.prototype.hasOwnProperty.call(cellEntry, "inputable"))
            return !!cellEntry.inputable
        return rowInputable(rowAt(rowIndex))
    }

    function normalizeHeaderCellType(value) {
        if (value === undefined || value === null)
            return "string"

        const name = String(value).toLowerCase()
        if (name === "int" || name === "integer")
            return "int"
        if (name === "float" || name === "real" || name === "double" || name === "number" || name === "decimal")
            return "float"
        if (name === "bool" || name === "boolean")
            return "bool"
        return "string"
    }

    function inferredCellType(value) {
        if (typeof value === "boolean")
            return "bool"
        if (typeof value === "number")
            return Math.floor(value) === value ? "int" : "float"
        return "string"
    }

    function headerCellType(columnIndex) {
        const entry = headerAt(columnIndex)
        if (entry && typeof entry === "object") {
            if (entry.type !== undefined && entry.type !== null)
                return normalizeHeaderCellType(entry.type)
            if (entry.valueType !== undefined && entry.valueType !== null)
                return normalizeHeaderCellType(entry.valueType)
            if (entry.cellType !== undefined && entry.cellType !== null)
                return normalizeHeaderCellType(entry.cellType)
            if (entry.dataType !== undefined && entry.dataType !== null)
                return normalizeHeaderCellType(entry.dataType)
            if (entry.value !== undefined && entry.value !== null)
                return inferredCellType(entry.value)
            return "string"
        }
        return inferredCellType(entry)
    }

    function columnType(columnIndex) {
        return headerCellType(columnIndex)
    }

    function cellRawValue(rowIndex, columnIndex) {
        const entry = cellAt(rowIndex, columnIndex)
        if (entry && typeof entry === "object") {
            if (Object.prototype.hasOwnProperty.call(entry, "value") && entry.value !== undefined && entry.value !== null)
                return entry.value
            if (entry.text !== undefined && entry.text !== null)
                return entry.text
            if (entry.label !== undefined && entry.label !== null)
                return entry.label
            if (entry.title !== undefined && entry.title !== null)
                return entry.title
            return undefined
        }
        return entry
    }

    function typedDefaultValue(valueType) {
        const normalizedType = normalizeHeaderCellType(valueType)
        if (normalizedType === "int")
            return 0
        if (normalizedType === "float")
            return 0
        if (normalizedType === "bool")
            return false
        return defaultCellText
    }

    function coerceCellValue(value, valueType) {
        const normalizedType = normalizeHeaderCellType(valueType)
        if (normalizedType === "string") {
            const textValue = value === undefined || value === null ? "" : String(value)
            return {
                "accepted": true,
                "type": "string",
                "value": textValue,
                "text": textValue
            }
        }

        if (normalizedType === "int") {
            if (typeof value === "number" && !isNaN(value) && isFinite(value) && Math.floor(value) === value) {
                return {
                    "accepted": true,
                    "type": "int",
                    "value": value,
                    "text": String(value)
                }
            }

            const intText = value === undefined || value === null ? "" : String(value).trim()
            if (/^-?\d+$/.test(intText)) {
                const intValue = Number(intText)
                return {
                    "accepted": true,
                    "type": "int",
                    "value": intValue,
                    "text": String(intValue)
                }
            }
            return {
                "accepted": false,
                "type": "int",
                "value": value,
                "text": value === undefined || value === null ? "" : String(value)
            }
        }

        if (normalizedType === "float") {
            const floatValue = typeof value === "number" ? value : Number(String(value === undefined || value === null ? "" : value).trim())
            if (!isNaN(floatValue) && isFinite(floatValue)) {
                return {
                    "accepted": true,
                    "type": "float",
                    "value": floatValue,
                    "text": String(floatValue)
                }
            }
            return {
                "accepted": false,
                "type": "float",
                "value": value,
                "text": value === undefined || value === null ? "" : String(value)
            }
        }

        if (typeof value === "boolean") {
            return {
                "accepted": true,
                "type": "bool",
                "value": value,
                "text": value ? "true" : "false"
            }
        }
        if (typeof value === "number" && !isNaN(value) && isFinite(value) && (value === 0 || value === 1)) {
            const numberBool = value === 1
            return {
                "accepted": true,
                "type": "bool",
                "value": numberBool,
                "text": numberBool ? "true" : "false"
            }
        }

        const boolText = value === undefined || value === null ? "" : String(value).trim().toLowerCase()
        if (boolText === "true" || boolText === "1" || boolText === "yes" || boolText === "on") {
            return {
                "accepted": true,
                "type": "bool",
                "value": true,
                "text": "true"
            }
        }
        if (boolText === "false" || boolText === "0" || boolText === "no" || boolText === "off") {
            return {
                "accepted": true,
                "type": "bool",
                "value": false,
                "text": "false"
            }
        }
        return {
            "accepted": false,
            "type": "bool",
            "value": value,
            "text": value === undefined || value === null ? "" : String(value)
        }
    }

    function validateCellInput(rowIndex, columnIndex, value) {
        return coerceCellValue(value, headerCellType(columnIndex))
    }

    function cellValueAccepted(rowIndex, columnIndex, value) {
        return validateCellInput(rowIndex, columnIndex, value).accepted
    }

    function cellText(rowIndex, columnIndex) {
        const rawValue = cellRawValue(rowIndex, columnIndex)
        if (rawValue === undefined || rawValue === null)
            return defaultCellText

        const result = validateCellInput(rowIndex, columnIndex, rawValue)
        if (result.accepted)
            return result.text
        return String(rawValue)
    }

    function setCellValue(rowIndex, columnIndex, value) {
        if (!Array.isArray(rows)) {
            console.warn("LVRS Table.setCellValue requires array rows.")
            return false
        }

        const targetRow = Math.floor(Number(rowIndex))
        const targetColumn = Math.floor(Number(columnIndex))
        if (isNaN(targetRow) || isNaN(targetColumn) || targetRow < 0 || targetColumn < 0)
            return false
        if (targetRow >= rows.length || !Array.isArray(rows[targetRow]) || targetColumn >= rows[targetRow].length)
            return false

        const result = validateCellInput(targetRow, targetColumn, value)
        if (!result.accepted) {
            console.warn("LVRS Table.setCellValue rejected value for " + result.type + " column.")
            return false
        }

        const cell = normalizeCellObject(targetRow, targetColumn)
        if (!cell)
            return false
        cell.value = result.value
        cell.text = result.text
        refreshRows()
        return true
    }

    function integerObjectValue(entry, primaryKey, alternateKey, fallbackValue) {
        if (!entry || typeof entry !== "object")
            return fallbackValue
        let value = undefined
        if (Object.prototype.hasOwnProperty.call(entry, primaryKey))
            value = entry[primaryKey]
        else if (alternateKey && Object.prototype.hasOwnProperty.call(entry, alternateKey))
            value = entry[alternateKey]
        if (value === undefined || value === null)
            return fallbackValue
        const parsed = Math.floor(Number(value))
        return isNaN(parsed) || parsed < 1 ? fallbackValue : parsed
    }

    function cellRowSpan(rowIndex, columnIndex) {
        const availableRows = Math.max(1, control.resolvedRowCount - rowIndex)
        return Math.min(availableRows, integerObjectValue(cellAt(rowIndex, columnIndex), "rowSpan", "rowspan", 1))
    }

    function cellColumnSpan(rowIndex, columnIndex) {
        const rowEntry = rowAt(rowIndex)
        const availableColumns = Math.max(1, columnCountForRow(rowEntry) - columnIndex)
        return Math.min(availableColumns, integerObjectValue(cellAt(rowIndex, columnIndex), "columnSpan", "colSpan", 1))
    }

    function isMergeCoveredMarker(entry) {
        return !!(entry && typeof entry === "object" && entry._lvrsMerged === true)
    }

    function spanContains(anchorRow, anchorColumn, rowSpan, columnSpan, rowIndex, columnIndex) {
        return rowIndex >= anchorRow
            && rowIndex < anchorRow + rowSpan
            && columnIndex >= anchorColumn
            && columnIndex < anchorColumn + columnSpan
    }

    function mergeAnchorForCell(rowIndex, columnIndex) {
        const entry = cellAt(rowIndex, columnIndex)
        if (isMergeCoveredMarker(entry)
                && entry._lvrsMergeAnchorRow !== undefined
                && entry._lvrsMergeAnchorColumn !== undefined) {
            return {
                "rowIndex": Number(entry._lvrsMergeAnchorRow),
                "columnIndex": Number(entry._lvrsMergeAnchorColumn)
            }
        }

        for (let row = 0; row <= rowIndex; row++) {
            const rowEntry = rowAt(row)
            const count = columnCountForRow(rowEntry)
            for (let column = 0; column < count; column++) {
                if (row === rowIndex && column === columnIndex)
                    continue
                const candidate = cellAt(row, column)
                if (isMergeCoveredMarker(candidate))
                    continue
                const rowSpan = cellRowSpan(row, column)
                const columnSpan = cellColumnSpan(row, column)
                if ((rowSpan > 1 || columnSpan > 1)
                        && spanContains(row, column, rowSpan, columnSpan, rowIndex, columnIndex)) {
                    return {
                        "rowIndex": row,
                        "columnIndex": column
                    }
                }
            }
        }

        return {
            "rowIndex": rowIndex,
            "columnIndex": columnIndex
        }
    }

    function isCoveredCell(rowIndex, columnIndex) {
        const anchor = mergeAnchorForCell(rowIndex, columnIndex)
        return anchor.rowIndex !== rowIndex || anchor.columnIndex !== columnIndex
    }

    function maxColumnCount() {
        let count = Math.max(1, resolvedHeaderCount)
        for (let row = 0; row < resolvedRowCount; row++)
            count = Math.max(count, columnCountForRow(rowAt(row)))
        return count
    }

    function buildVisibleCellItems() {
        const result = []
        for (let row = 0; row < resolvedRowCount; row++) {
            const rowEntry = rowAt(row)
            const count = columnCountForRow(rowEntry)
            for (let column = 0; column < count; column++) {
                if (isCoveredCell(row, column))
                    continue
                const rowSpan = cellRowSpan(row, column)
                const columnSpan = cellColumnSpan(row, column)
                result.push({
                    "rowIndex": row,
                    "columnIndex": column,
                    "rowSpan": rowSpan,
                    "columnSpan": columnSpan,
                    "cellData": cellAt(row, column),
                    "text": cellText(row, column),
                    "valueType": headerCellType(column),
                    "x": columnX(column),
                    "y": rowY(row),
                    "width": columnSpanWidth(column, columnSpan),
                    "height": rowSpanHeight(row, rowSpan),
                    "inputable": cellInputable(row, column)
                })
            }
        }
        return result
    }

    function normalizeCellObject(rowIndex, columnIndex) {
        if (!Array.isArray(rows))
            return null
        const rowEntry = rows[rowIndex]
        if (!Array.isArray(rowEntry))
            return null

        const current = rowEntry[columnIndex]
        if (current && typeof current === "object")
            return current

        const normalized = { "text": current === undefined || current === null ? "" : String(current) }
        rowEntry[columnIndex] = normalized
        return normalized
    }

    function clearMergeMetadata(entry) {
        if (!entry || typeof entry !== "object")
            return
        delete entry._lvrsMerged
        delete entry._lvrsMergeAnchorRow
        delete entry._lvrsMergeAnchorColumn
        if (Object.prototype.hasOwnProperty.call(entry, "rowSpan"))
            entry.rowSpan = 1
        if (Object.prototype.hasOwnProperty.call(entry, "rowspan"))
            entry.rowspan = 1
        if (Object.prototype.hasOwnProperty.call(entry, "columnSpan"))
            entry.columnSpan = 1
        if (Object.prototype.hasOwnProperty.call(entry, "colSpan"))
            entry.colSpan = 1
    }

    function refreshRows() {
        if (Array.isArray(rows))
            rows = rows.slice()
    }

    function refreshColumnWidths() {
        if (Array.isArray(columnWidths))
            columnWidths = columnWidths.slice()
    }

    function refreshRowHeights() {
        if (Array.isArray(rowHeights))
            rowHeights = rowHeights.slice()
    }

    function normalizedColumnWidths() {
        const result = Array.isArray(columnWidths) ? columnWidths.slice() : []
        const count = Math.max(1, resolvedColumnCount)
        for (let column = 0; column < count; column++) {
            if (result[column] === undefined || result[column] === null || Number(result[column]) <= 0)
                result[column] = columnWidth(column)
        }
        return result
    }

    function normalizedRowHeights() {
        const result = Array.isArray(rowHeights) ? rowHeights.slice() : []
        for (let row = 0; row < resolvedRowCount; row++) {
            if (result[row] === undefined || result[row] === null || Number(result[row]) <= 0)
                result[row] = rowHeightAt(row)
        }
        return result
    }

    function setColumnWidth(columnIndex, widthValue) {
        const targetColumn = Math.floor(Number(columnIndex))
        if (isNaN(targetColumn) || targetColumn < 0 || targetColumn >= resolvedColumnCount)
            return false
        const sizes = normalizedColumnWidths()
        const nextWidth = numericSize(widthValue, columnWidth(targetColumn), minColumnWidth)
        sizes[targetColumn] = nextWidth
        columnWidths = sizes
        columnResized(targetColumn, nextWidth)
        return true
    }

    function setRowHeight(rowIndex, heightValue) {
        const targetRow = Math.floor(Number(rowIndex))
        if (isNaN(targetRow) || targetRow < 0 || targetRow >= resolvedRowCount)
            return false
        const sizes = normalizedRowHeights()
        const nextHeight = numericSize(heightValue, rowHeightAt(targetRow), minRowHeight)
        sizes[targetRow] = nextHeight
        rowHeights = sizes
        rowResized(targetRow, nextHeight)
        return true
    }

    function beginColumnResize(columnIndex, pointerX) {
        const targetColumn = Math.floor(Number(columnIndex))
        if (isNaN(targetColumn) || targetColumn < 0 || targetColumn >= resolvedColumnCount)
            return false
        resizingColumnIndex = targetColumn
        _resizeStartPointerX = Number(pointerX)
        _resizeStartColumnWidth = columnWidth(targetColumn)
        return true
    }

    function updateColumnResize(pointerX) {
        if (resizingColumnIndex < 0)
            return false
        const delta = Number(pointerX) - _resizeStartPointerX
        return setColumnWidth(resizingColumnIndex, _resizeStartColumnWidth + delta)
    }

    function endColumnResize() {
        resizingColumnIndex = -1
        _resizeStartPointerX = 0
        _resizeStartColumnWidth = 0
    }

    function beginRowResize(rowIndex, pointerY) {
        const targetRow = Math.floor(Number(rowIndex))
        if (isNaN(targetRow) || targetRow < 0 || targetRow >= resolvedRowCount)
            return false
        resizingRowIndex = targetRow
        _resizeStartPointerY = Number(pointerY)
        _resizeStartRowHeight = rowHeightAt(targetRow)
        return true
    }

    function updateRowResize(pointerY) {
        if (resizingRowIndex < 0)
            return false
        const delta = Number(pointerY) - _resizeStartPointerY
        return setRowHeight(resizingRowIndex, _resizeStartRowHeight + delta)
    }

    function endRowResize() {
        resizingRowIndex = -1
        _resizeStartPointerY = 0
        _resizeStartRowHeight = 0
    }

    function refreshHeaderColumns() {
        if (Array.isArray(headerCellItems)) {
            headerCellItems = headerCellItems.slice()
            return
        }
        if (Array.isArray(headerColumns))
            headerColumns = headerColumns.slice()
    }

    function canMutateStructure() {
        if (!Array.isArray(rows))
            return false
        for (let row = 0; row < rows.length; row++) {
            if (!Array.isArray(rows[row]))
                return false
        }
        return true
    }

    function createDefaultCell(rowIndex, columnIndex) {
        const type = headerCellType(columnIndex)
        const result = coerceCellValue(typedDefaultValue(type), type)
        return {
            "value": result.value,
            "text": result.text
        }
    }

    function createDefaultHeaderCell(columnIndex) {
        return { "label": defaultHeaderText }
    }

    function headerMutationArray() {
        if (Array.isArray(headerCellItems))
            return headerCellItems
        if (Array.isArray(headerColumns))
            return headerColumns
        return null
    }

    function normalizeStructureForMutation() {
        if (!canMutateStructure())
            return false
        for (let row = 0; row < rows.length; row++) {
            const rowEntry = rows[row]
            for (let column = 0; column < rowEntry.length; column++) {
                const cell = rowEntry[column]
                if (cell && typeof cell === "object")
                    clearMergeMetadata(cell)
            }
        }
        return true
    }

    function canInsertRow(rowIndex) {
        const parsedRow = Math.floor(Number(rowIndex))
        return canMutateStructure()
            && !isNaN(parsedRow)
            && parsedRow >= 0
            && parsedRow <= resolvedRowCount
    }

    function insertRow(rowIndex) {
        const targetRow = Math.floor(Number(rowIndex))
        if (!canInsertRow(targetRow)) {
            console.warn("LVRS Table.insertRow requires array rows and an in-bounds row index.")
            return false
        }

        normalizeStructureForMutation()
        const count = Math.max(1, resolvedColumnCount)
        const rowEntry = []
        for (let column = 0; column < count; column++)
            rowEntry.push(createDefaultCell(targetRow, column))
        rows.splice(targetRow, 0, rowEntry)
        if (Array.isArray(rowHeights)) {
            rowHeights.splice(targetRow, 0, rowHeight)
            refreshRowHeights()
        }
        refreshRows()
        rowInserted(targetRow)
        return true
    }

    function appendRow() {
        return insertRow(resolvedRowCount)
    }

    function canDeleteRow(rowIndex) {
        const parsedRow = Math.floor(Number(rowIndex))
        return canMutateStructure()
            && !isNaN(parsedRow)
            && parsedRow >= 0
            && parsedRow < resolvedRowCount
    }

    function deleteRow(rowIndex) {
        const targetRow = Math.floor(Number(rowIndex))
        if (!canDeleteRow(targetRow)) {
            console.warn("LVRS Table.deleteRow requires array rows and an in-bounds row index.")
            return false
        }

        normalizeStructureForMutation()
        rows.splice(targetRow, 1)
        if (Array.isArray(rowHeights) && targetRow < rowHeights.length) {
            rowHeights.splice(targetRow, 1)
            refreshRowHeights()
        }
        refreshRows()
        rowDeleted(targetRow)
        return true
    }

    function removeRow(rowIndex) {
        return deleteRow(rowIndex)
    }

    function canInsertColumn(columnIndex) {
        const parsedColumn = Math.floor(Number(columnIndex))
        return canMutateStructure()
            && !isNaN(parsedColumn)
            && parsedColumn >= 0
            && parsedColumn <= resolvedColumnCount
    }

    function insertColumn(columnIndex) {
        const targetColumn = Math.floor(Number(columnIndex))
        if (!canInsertColumn(targetColumn)) {
            console.warn("LVRS Table.insertColumn requires array rows and an in-bounds column index.")
            return false
        }

        normalizeStructureForMutation()
        const currentColumnCount = Math.max(1, resolvedColumnCount)
        for (let row = 0; row < rows.length; row++) {
            const rowEntry = rows[row]
            while (rowEntry.length < currentColumnCount)
                rowEntry.push(createDefaultCell(row, rowEntry.length))
            rowEntry.splice(targetColumn, 0, createDefaultCell(row, targetColumn))
        }

        const header = headerMutationArray()
        if (header) {
            while (header.length < currentColumnCount)
                header.push(createDefaultHeaderCell(header.length))
            header.splice(targetColumn, 0, createDefaultHeaderCell(targetColumn))
            refreshHeaderColumns()
        }
        if (Array.isArray(columnWidths)) {
            columnWidths.splice(targetColumn, 0, cellWidth > 0 ? cellWidth : autoColumnWidth())
            refreshColumnWidths()
        }

        refreshRows()
        columnInserted(targetColumn)
        return true
    }

    function appendColumn() {
        return insertColumn(resolvedColumnCount)
    }

    function canDeleteColumn(columnIndex) {
        const parsedColumn = Math.floor(Number(columnIndex))
        return canMutateStructure()
            && resolvedColumnCount > 1
            && !isNaN(parsedColumn)
            && parsedColumn >= 0
            && parsedColumn < resolvedColumnCount
    }

    function deleteColumn(columnIndex) {
        const targetColumn = Math.floor(Number(columnIndex))
        if (!canDeleteColumn(targetColumn)) {
            console.warn("LVRS Table.deleteColumn requires array rows, at least two columns, and an in-bounds column index.")
            return false
        }

        normalizeStructureForMutation()
        for (let row = 0; row < rows.length; row++) {
            const rowEntry = rows[row]
            if (targetColumn < rowEntry.length)
                rowEntry.splice(targetColumn, 1)
        }

        const header = headerMutationArray()
        if (header && targetColumn < header.length) {
            header.splice(targetColumn, 1)
            refreshHeaderColumns()
        }
        if (Array.isArray(columnWidths) && targetColumn < columnWidths.length) {
            columnWidths.splice(targetColumn, 1)
            refreshColumnWidths()
        }

        refreshRows()
        columnDeleted(targetColumn)
        return true
    }

    function removeColumn(columnIndex) {
        return deleteColumn(columnIndex)
    }

    function buildContextMenuItems() {
        const items = []
        if (contextRowIndex >= 0) {
            items.push({
                "label": "Delete row",
                "eventName": "table.deleteRow",
                "enabled": canDeleteRow(contextRowIndex),
                "onTriggered": function() {
                    control.deleteRow(control.contextRowIndex)
                }
            })
        }
        if (contextRowIndex >= 0 && contextColumnIndex >= 0)
            items.push({ "type": "divider" })
        if (contextColumnIndex >= 0) {
            items.push({
                "label": "Delete column",
                "eventName": "table.deleteColumn",
                "enabled": canDeleteColumn(contextColumnIndex),
                "onTriggered": function() {
                    control.deleteColumn(control.contextColumnIndex)
                }
            })
        }
        return items
    }

    function openContextMenuForCell(rowIndex, columnIndex, item, xPos, yPos) {
        if (!deleteContextMenuEnabled || !structureMutationAvailable)
            return false
        contextRowIndex = Math.floor(Number(rowIndex))
        contextColumnIndex = Math.floor(Number(columnIndex))
        if (item)
            tableContextMenu.openFor(item, xPos, yPos)
        else
            tableContextMenu.openAt(xPos, yPos)
        return true
    }

    function splitAnchorCell(anchorRow, anchorColumn, refresh) {
        const anchorCell = normalizeCellObject(anchorRow, anchorColumn)
        if (!anchorCell)
            return false

        const rowSpan = cellRowSpan(anchorRow, anchorColumn)
        const columnSpan = cellColumnSpan(anchorRow, anchorColumn)
        for (let row = anchorRow; row < anchorRow + rowSpan; row++) {
            for (let column = anchorColumn; column < anchorColumn + columnSpan; column++) {
                const cell = normalizeCellObject(row, column)
                clearMergeMetadata(cell)
            }
        }

        clearMergeMetadata(anchorCell)
        if (refresh !== false) {
            refreshRows()
            cellSplit(anchorRow, anchorColumn)
        }
        return true
    }

    function splitCellsIntersecting(rowIndex, columnIndex, rowSpan, columnSpan) {
        for (let row = 0; row < resolvedRowCount; row++) {
            const count = columnCountForRow(rowAt(row))
            for (let column = 0; column < count; column++) {
                const entry = cellAt(row, column)
                if (isMergeCoveredMarker(entry))
                    continue
                const candidateRowSpan = cellRowSpan(row, column)
                const candidateColumnSpan = cellColumnSpan(row, column)
                const intersects = row < rowIndex + rowSpan
                    && row + candidateRowSpan > rowIndex
                    && column < columnIndex + columnSpan
                    && column + candidateColumnSpan > columnIndex
                if (intersects && (candidateRowSpan > 1 || candidateColumnSpan > 1))
                    splitAnchorCell(row, column, false)
            }
        }
    }

    function canMergeCells(rowIndex, columnIndex, rowSpan, columnSpan) {
        if (!Array.isArray(rows))
            return false
        const parsedRow = Math.floor(Number(rowIndex))
        const parsedColumn = Math.floor(Number(columnIndex))
        const parsedRowSpan = Math.floor(Number(rowSpan))
        const parsedColumnSpan = Math.floor(Number(columnSpan))
        if (isNaN(parsedRow) || isNaN(parsedColumn) || isNaN(parsedRowSpan) || isNaN(parsedColumnSpan))
            return false
        if (parsedRow < 0 || parsedColumn < 0 || parsedRowSpan < 1 || parsedColumnSpan < 1)
            return false
        if (parsedRow + parsedRowSpan > resolvedRowCount)
            return false
        for (let row = parsedRow; row < parsedRow + parsedRowSpan; row++) {
            const rowEntry = rows[row]
            if (!Array.isArray(rowEntry))
                return false
            if (parsedColumn + parsedColumnSpan > rowEntry.length)
                return false
        }
        return true
    }

    function mergeCells(rowIndex, columnIndex, rowSpan, columnSpan) {
        const anchorRow = Math.floor(Number(rowIndex))
        const anchorColumn = Math.floor(Number(columnIndex))
        const spanRows = Math.floor(Number(rowSpan))
        const spanColumns = Math.floor(Number(columnSpan))
        if (!canMergeCells(anchorRow, anchorColumn, spanRows, spanColumns)) {
            console.warn("LVRS Table.mergeCells requires array rows and an in-bounds rectangular cell range.")
            return false
        }

        splitCellsIntersecting(anchorRow, anchorColumn, spanRows, spanColumns)

        const anchorCell = normalizeCellObject(anchorRow, anchorColumn)
        anchorCell.rowSpan = spanRows
        anchorCell.columnSpan = spanColumns
        delete anchorCell._lvrsMerged
        delete anchorCell._lvrsMergeAnchorRow
        delete anchorCell._lvrsMergeAnchorColumn

        for (let row = anchorRow; row < anchorRow + spanRows; row++) {
            for (let column = anchorColumn; column < anchorColumn + spanColumns; column++) {
                if (row === anchorRow && column === anchorColumn)
                    continue
                const cell = normalizeCellObject(row, column)
                cell._lvrsMerged = true
                cell._lvrsMergeAnchorRow = anchorRow
                cell._lvrsMergeAnchorColumn = anchorColumn
                if (Object.prototype.hasOwnProperty.call(cell, "rowSpan"))
                    cell.rowSpan = 1
                if (Object.prototype.hasOwnProperty.call(cell, "columnSpan"))
                    cell.columnSpan = 1
            }
        }

        refreshRows()
        cellsMerged(anchorRow, anchorColumn, spanRows, spanColumns)
        return true
    }

    function splitCell(rowIndex, columnIndex) {
        if (!Array.isArray(rows)) {
            console.warn("LVRS Table.splitCell requires array rows.")
            return false
        }

        const parsedRow = Math.floor(Number(rowIndex))
        const parsedColumn = Math.floor(Number(columnIndex))
        if (isNaN(parsedRow) || isNaN(parsedColumn) || parsedRow < 0 || parsedColumn < 0)
            return false
        if (parsedRow >= resolvedRowCount || parsedColumn >= columnCountForRow(rowAt(parsedRow)))
            return false

        const anchor = mergeAnchorForCell(parsedRow, parsedColumn)
        return splitAnchorCell(anchor.rowIndex, anchor.columnIndex, true)
    }

    implicitWidth: Theme.scaleMetric(405) + resolvedStructureGutterWidth
    implicitHeight: tableHeader.implicitHeight + totalBodyHeight() + resolvedStructureGutterHeight

    Rectangle {
        id: tableFrame
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
                width: tableFrame.width
                cellItems: control.resolvedHeaderSource
                columnWidths: control.columnWidths
                fallbackCellWidth: control.cellWidth
                minColumnWidth: control.minColumnWidth
                textColor: control.headerTextColor
                separatorColor: control.headerSeparatorColor
            }

            Item {
                id: bodyLayer
                width: tableFrame.width
                height: control.totalBodyHeight()

                Repeater {
                    model: control.visibleCellItems

                    delegate: TableCellItem {
                        required property var modelData

                        x: modelData.x
                        y: modelData.y
                        width: modelData.width
                        height: modelData.height
                        itemData: modelData.cellData
                        text: modelData.text
                        cellHeight: modelData.height
                        dividerColor: control.rowDividerColor
                        textColor: control.cellTextColor
                        clipContent: true
                        inputable: modelData.inputable
                        valueType: modelData.valueType
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

                        MouseArea {
                            id: cellContextArea
                            anchors.fill: parent
                            acceptedButtons: Qt.RightButton
                            enabled: control.deleteContextMenuEnabled && control.structureMutationAvailable
                            preventStealing: true

                            onPressed: function(mouse) {
                                control.openContextMenuForCell(modelData.rowIndex,
                                                               modelData.columnIndex,
                                                               cellContextArea,
                                                               mouse.x,
                                                               mouse.y)
                                mouse.accepted = true
                            }
                        }
                    }
                }
            }
        }

        Item {
            id: headerContextLayer
            x: 0
            y: 0
            width: tableFrame.width
            height: tableHeader.implicitHeight
            visible: control.deleteContextMenuEnabled && control.structureMutationAvailable

            Repeater {
                model: Math.max(1, control.resolvedColumnCount)

                delegate: MouseArea {
                    id: headerContextArea
                    required property int index

                    x: control.structureColumnX(index)
                    y: 0
                    width: control.structureCellWidth(index)
                    height: headerContextLayer.height
                    acceptedButtons: Qt.RightButton
                    preventStealing: true

                    onPressed: function(mouse) {
                        control.openContextMenuForCell(-1, index, headerContextArea, mouse.x, mouse.y)
                        mouse.accepted = true
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
            y: tableFrame.y + tableHeader.implicitHeight + control.totalBodyHeight()
            width: control.structureCellWidth(index)
            height: control.resolvedStructureGutterHeight
            text: "+"
            tone: AbstractButton.Borderless
            horizontalPadding: 0
            verticalPadding: 0
            onClicked: control.insertColumn(index + 1)
        }
    }

    ContextMenu {
        id: tableContextMenu
        items: control.buildContextMenuItems()
        itemWidth: Theme.scaleMetric(132)
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Table { rows: [[{ text: "A", columnSpan: 2 }, { text: "B" }, { text: "C" }]]; structureControlsVisible: true }
