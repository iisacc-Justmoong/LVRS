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

    property int rowHeight: 24
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

    function rowAt(index) {
        if (!rows)
            return null
        if (rows.length !== undefined)
            return rows[index]
        if (rows.get !== undefined)
            return rows.get(index)
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

    function rowInputable(rowEntry) {
        if (rowEntry && typeof rowEntry === "object" && Object.prototype.hasOwnProperty.call(rowEntry, "inputable"))
            return !!rowEntry.inputable
        return control.inputable
    }

    implicitWidth: 405
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

            Repeater {
                model: control.resolvedRowCount

                delegate: TableRow {
                    required property int index
                    readonly property var rowData: control.rowAt(index)

                    width: tableFrame.width
                    height: control.rowHeight
                    cellItems: rowData
                    cellWidth: control.cellWidth > 0 ? control.cellWidth : control.autoCellWidth(rowData)
                    cellHeight: control.rowHeight
                    dividerColor: control.rowDividerColor
                    textColor: control.cellTextColor
                    inputable: control.rowInputable(rowData)
                    onCellInputEdited: function(columnIndex, text) {
                        control.cellInputEdited(index, columnIndex, text)
                    }
                    onCellInputSubmitted: function(columnIndex, text) {
                        control.cellInputSubmitted(index, columnIndex, text)
                    }
                }
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Table { headerCellItems: [{ label: "Column" }, { label: "Column" }, { label: "Column" }]; rows: [[{ text: "A" }, { text: "B" }, { text: "C" }]]; inputable: true }
