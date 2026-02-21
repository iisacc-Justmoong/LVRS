pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: control

    property var cellItems: undefined
    property var columns: ["Column", "Column", "Column"]
    property int rowHeight: 24
    property int cellHorizontalPadding: Theme.gap8
    property color textColor: Theme.descriptionColor
    property real separatorHeight: Theme.strokeThin
    property color separatorColor: Theme.panelBackground10

    readonly property var resolvedColumnSource: {
        if (control.cellItems !== undefined && control.cellItems !== null)
            return control.cellItems
        return control.columns
    }
    readonly property int resolvedColumnCount: {
        const source = control.resolvedColumnSource
        if (!source)
            return 0
        if (source.length !== undefined)
            return source.length
        if (source.count !== undefined)
            return source.count
        return 0
    }

    function columnAt(index) {
        const source = control.resolvedColumnSource
        if (!source)
            return null
        if (source.length !== undefined)
            return source[index]
        if (source.get !== undefined)
            return source.get(index)
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

    function columnPadding(index) {
        const entry = columnAt(index)
        if (!entry || typeof entry !== "object")
            return cellHorizontalPadding
        if (entry.contentSpacing !== undefined && entry.contentSpacing !== null) {
            const spacing = Number(entry.contentSpacing)
            if (!isNaN(spacing) && spacing >= 0)
                return spacing
        }
        if (entry.horizontalPadding !== undefined && entry.horizontalPadding !== null) {
            const padding = Number(entry.horizontalPadding)
            if (!isNaN(padding) && padding >= 0)
                return padding
        }
        return cellHorizontalPadding
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
                    id: headerCell
                    required property int index

                    Layout.fillWidth: true
                    Layout.preferredHeight: control.rowHeight

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: control.columnPadding(headerCell.index)
                        anchors.verticalCenter: parent.verticalCenter
                        style: description
                        text: control.columnText(headerCell.index)
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
// LV.TableHeader { cellItems: [{ label: "Column" }, { label: "Column" }, { label: "Column" }] }
