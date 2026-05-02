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
    property color backgroundColor: "#282828"
    property color borderColor: Theme.surface
    property real borderWidth: Theme.strokeThin
    property color headerTextColor: Theme.descriptionColor
    property color cellTextColor: Theme.bodyColor
    property color dividerColor: Theme.panelBackground03
    property color rowDividerColor: dividerColor
    property color headerSeparatorColor: Theme.panelBackground10
    property bool inputable: false

    signal cellInputEdited(int rowIndex, int columnIndex, string text)
    signal cellInputSubmitted(int rowIndex, int columnIndex, string text)
    signal cellsMerged(int rowIndex, int columnIndex, int rowSpan, int columnSpan)
    signal cellSplit(int rowIndex, int columnIndex)

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

    function cellX(rowEntry, columnIndex) {
        const widthValue = rowCellWidth(rowEntry)
        const spacingValue = rowCellSpacing(rowEntry)
        return columnIndex * (widthValue + spacingValue)
    }

    function cellSpanWidth(rowEntry, columnSpan) {
        const widthValue = rowCellWidth(rowEntry)
        const spacingValue = rowCellSpacing(rowEntry)
        const spanValue = Math.max(1, columnSpan)
        return (spanValue * widthValue) + ((spanValue - 1) * spacingValue)
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

    function cellText(rowIndex, columnIndex) {
        const entry = cellAt(rowIndex, columnIndex)
        if (typeof entry === "string" || typeof entry === "number")
            return String(entry)
        if (!entry || typeof entry !== "object")
            return "Text"
        return entry.label || entry.text || entry.title || "Text"
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
                    "x": cellX(rowEntry, column),
                    "y": row * rowHeight,
                    "width": cellSpanWidth(rowEntry, columnSpan),
                    "height": rowSpan * rowHeight,
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

    implicitWidth: Theme.scaleMetric(405)
    implicitHeight: tableHeader.implicitHeight + (resolvedRowCount * rowHeight)

    Rectangle {
        id: tableFrame
        anchors.fill: parent
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
                textColor: control.headerTextColor
                separatorColor: control.headerSeparatorColor
            }

            Item {
                id: bodyLayer
                width: tableFrame.width
                height: control.resolvedRowCount * control.rowHeight

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
                        onInputEdited: function(text) {
                            control.cellInputEdited(modelData.rowIndex, modelData.columnIndex, text)
                        }
                        onInputSubmitted: function(text) {
                            control.cellInputSubmitted(modelData.rowIndex, modelData.columnIndex, text)
                        }
                    }
                }
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Table { rows: [[{ text: "A", columnSpan: 2 }, { text: "B" }, { text: "C" }]]; inputable: true }
