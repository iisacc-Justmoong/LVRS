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
    readonly property int resizingColumnIndex: tableModel.resizingColumnIndex
    readonly property int resizingRowIndex: tableModel.resizingRowIndex
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
    }

    readonly property var resolvedHeaderSource: {
        tableModel.revision
        tableModel.headerCount
        tableModel.columnCount
        return tableModel.resolvedHeaderSource()
    }
    readonly property int resolvedRowCount: tableModel.rowCount
    readonly property int resolvedHeaderCount: tableModel.headerCount
    readonly property int resolvedColumnCount: tableModel.columnCount
    readonly property var visibleCellItems: buildVisibleCellItems()
    readonly property bool structureMutationAvailable: tableModel.structureMutationAvailable
    readonly property bool canUndo: tableModel.canUndo
    readonly property bool canRedo: tableModel.canRedo
    readonly property int undoDepth: tableModel.undoDepth
    readonly property int redoDepth: tableModel.redoDepth
    readonly property int resolvedBodyHeight: {
        tableModel.revision
        return tableModel.totalBodyHeight()
    }
    readonly property bool resolvedStructureControlsVisible: structureControlsVisible && structureMutationAvailable
    readonly property int resolvedStructureGutterWidth: resolvedStructureControlsVisible && addRowControlsVisible
        ? structureGutterWidth
        : 0
    readonly property int resolvedStructureGutterHeight: resolvedStructureControlsVisible && addColumnControlsVisible
        ? structureGutterHeight
        : 0

    function rowAt(index) {
        return tableModel.rowAt(index)
    }

    function cellAt(rowIndex, columnIndex) {
        return tableModel.cellAt(rowIndex, columnIndex)
    }

    function headerAt(index) {
        return tableModel.headerAt(index)
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
        return tableModel.visibleCells()
    }

    function syncRowsFromModel() {
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
        syncRowsFromModel()
        syncHeadersFromModel()
        syncColumnWidthsFromModel()
        syncRowHeightsFromModel()
        return true
    }

    function redo() {
        if (!tableModel.redo())
            return false
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

    implicitWidth: Theme.scaleMetric(405) + resolvedStructureGutterWidth
    implicitHeight: tableHeader.implicitHeight + resolvedBodyHeight + resolvedStructureGutterHeight

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
                height: control.resolvedBodyHeight

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
// LV.Table { rows: [[{ text: "A", columnSpan: 2 }, { text: "B" }, { text: "C" }]]; structureControlsVisible: true }
