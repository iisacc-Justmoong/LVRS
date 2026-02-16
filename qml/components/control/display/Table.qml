import QtQuick
import LVRS 1.0

Item {
    id: control

    property var headerColumns: ["Column", "Column", "Column"]
    property var rows: [
        ["Text", "Text", "Text"],
        ["Text", "Text", "Text"],
        ["Text", "Text", "Text"],
        ["Text", "Text", "Text"]
    ]

    property int rowHeight: 24
    property int cellWidth: 234
    property color backgroundColor: "#282828"
    property color borderColor: Theme.surface
    property real borderWidth: Theme.strokeThin
    property color headerTextColor: Theme.descriptionColor
    property color cellTextColor: Theme.bodyColor
    property color dividerColor: Theme.surface

    readonly property int resolvedRowCount: {
        if (!rows)
            return 0
        if (rows.length !== undefined)
            return rows.length
        if (rows.count !== undefined)
            return rows.count
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
                columns: control.headerColumns
                textColor: control.headerTextColor
                separatorColor: control.dividerColor
            }

            Repeater {
                model: control.resolvedRowCount

                delegate: TableRow {
                    required property int index

                    width: tableFrame.width
                    height: control.rowHeight
                    cells: control.rowAt(index)
                    cellWidth: control.cellWidth
                    cellHeight: control.rowHeight
                    dividerColor: control.dividerColor
                    textColor: control.cellTextColor
                }
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Table { headerColumns: ["Column", "Column", "Column"]; rows: [["A", "B", "C"]] }
