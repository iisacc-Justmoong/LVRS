pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

Item {
    id: control

    property var cells: ["Text", "Text", "Text"]
    property int cellWidth: 234
    property int cellHeight: 24
    property int contentSpacing: Theme.gap8
    property color dividerColor: Theme.surface
    property color textColor: Theme.bodyColor

    readonly property int resolvedCellCount: {
        if (!cells)
            return 0
        if (cells.length !== undefined)
            return cells.length
        if (cells.count !== undefined)
            return cells.count
        return 0
    }

    readonly property real resolvedSpacing: {
        const count = Math.max(1, resolvedCellCount)
        if (count <= 1)
            return 0
        const computed = (width - (count * cellWidth)) / (count - 1)
        return Math.max(0, computed)
    }

    function cellAt(index) {
        if (!cells)
            return null
        if (cells.length !== undefined)
            return cells[index]
        if (cells.get !== undefined)
            return cells.get(index)
        return null
    }

    function cellText(index) {
        if (resolvedCellCount <= 0)
            return "Text"

        const entry = cellAt(index)
        if (typeof entry === "string" || typeof entry === "number")
            return String(entry)
        if (!entry || typeof entry !== "object")
            return "Text"
        return entry.label || entry.text || entry.title || "Text"
    }

    implicitWidth: 717
    implicitHeight: cellHeight

    Row {
        anchors.fill: parent
        spacing: control.resolvedSpacing

        Repeater {
            model: Math.max(1, control.resolvedCellCount)

            delegate: TableCellItem {
                required property int index

                width: control.cellWidth
                height: control.cellHeight
                text: control.cellText(index)
                contentSpacing: control.contentSpacing
                dividerColor: control.dividerColor
                textColor: control.textColor
                clipContent: true
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.TableRow { cells: ["Text", "Text", "Text"] }
