pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

Item {
    id: control

    property var cellItems: undefined
    property var columns: ["Column", "Column", "Column"]
    property int rowHeight: Theme.scaleMetric(24)
    property int cellHorizontalPadding: Theme.gap8
    property var columnWidths: []
    property int fallbackCellWidth: 0
    property int minColumnWidth: Theme.scaleMetric(32)
    property color textColor: Theme.descriptionColor
    property real separatorHeight: Theme.strokeThin
    property color separatorColor: Theme.panelBackground10

    TableHeaderModel {
        id: headerModel
        cellItems: control.cellItems
        columns: control.columns
        tableWidth: control.width
        rowHeight: control.rowHeight
        cellHorizontalPadding: control.cellHorizontalPadding
        columnWidths: control.columnWidths
        fallbackCellWidth: control.fallbackCellWidth
        minColumnWidth: control.minColumnWidth
    }

    readonly property var resolvedColumnSource: headerModel.resolvedColumnSource()
    readonly property int resolvedColumnCount: headerModel.columnCount
    readonly property var headerDescriptors: headerModel.descriptors

    function columnAt(index) {
        return headerModel.columnAt(index)
    }

    function normalizeColumnType(value) {
        return headerModel.normalizeColumnType(value)
    }

    function inferredColumnType(value) {
        return headerModel.inferredColumnType(value)
    }

    function columnType(index) {
        return headerModel.columnType(index)
    }

    function columnText(index) {
        return headerModel.columnText(index)
    }

    function columnPadding(index) {
        return headerModel.columnPadding(index)
    }

    function numericWidth(value, fallbackValue) {
        return headerModel.numericWidth(value, fallbackValue)
    }

    function autoColumnWidth() {
        return headerModel.autoColumnWidth()
    }

    function columnWidth(index) {
        return headerModel.columnWidth(index)
    }

    function columnX(index) {
        return headerModel.columnX(index)
    }

    implicitWidth: Theme.scaleMetric(717)
    implicitHeight: rowHeight + separatorHeight

    Column {
        anchors.fill: parent
        spacing: 0

        Item {
            width: parent.width
            height: control.rowHeight

            Repeater {
                model: Math.max(1, control.resolvedColumnCount)

                delegate: Item {
                    id: headerCell
                    required property int index
                    readonly property var descriptor: headerModel.descriptorAt(index)

                    x: descriptor.x
                    width: descriptor.width
                    height: control.rowHeight

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: headerCell.descriptor.padding
                        anchors.verticalCenter: parent.verticalCenter
                        style: description
                        text: headerCell.descriptor.text
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
// LV.TableHeader { cellItems: [{ label: "Name", type: "string" }, { label: "Count", type: "int" }] }
