import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: control

    property var columns: ["Column", "Column", "Column"]
    property int rowHeight: 24
    property int cellHorizontalPadding: Theme.gap8
    property color textColor: Theme.descriptionColor
    property real separatorHeight: Theme.strokeThin
    property color separatorColor: Theme.surface

    readonly property int resolvedColumnCount: {
        if (!columns)
            return 0
        if (columns.length !== undefined)
            return columns.length
        if (columns.count !== undefined)
            return columns.count
        return 0
    }

    function columnAt(index) {
        if (!columns)
            return null
        if (columns.length !== undefined)
            return columns[index]
        if (columns.get !== undefined)
            return columns.get(index)
        return null
    }

    function columnText(index) {
        if (resolvedColumnCount <= 0)
            return "Column"

        const entry = columnAt(index)
        if (typeof entry === "string" || typeof entry === "number")
            return String(entry)
        if (!entry || typeof entry !== "object")
            return "Column"
        return entry.label || entry.text || entry.title || "Column"
    }

    implicitWidth: 717
    implicitHeight: rowHeight + separatorHeight

    Column {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            width: parent.width
            height: control.rowHeight
            spacing: 0

            Repeater {
                model: Math.max(1, control.resolvedColumnCount)

                delegate: Item {
                    required property int index

                    Layout.fillWidth: true
                    Layout.preferredHeight: control.rowHeight

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: control.cellHorizontalPadding
                        anchors.verticalCenter: parent.verticalCenter
                        style: description
                        text: control.columnText(index)
                        color: control.textColor
                        elide: Text.ElideRight
                    }
                }
            }
        }

        Rectangle {
            width: parent.width
            height: control.separatorHeight
            color: control.separatorColor
            antialiasing: false
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.TableHeader { columns: ["Column", "Column", "Column"] }
