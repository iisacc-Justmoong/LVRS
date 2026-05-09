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
    property Component cellDelegate: null
    readonly property Component resolvedCellDelegate: cellDelegate || defaultCellDelegate

    Component {
        id: defaultCellDelegate

        Item {
            property var modelData: ({})
            property int index: modelData.index === undefined ? -1 : modelData.index
            readonly property var descriptor: modelData.descriptor || ({})

            Label {
                anchors.left: parent.left
                anchors.leftMargin: descriptor.padding || 0
                anchors.verticalCenter: parent.verticalCenter
                style: description
                text: descriptor.text || ""
                color: control.textColor
                elide: Text.ElideRight
            }
        }
    }

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
    readonly property var cellDelegateItems: buildCellDelegateItems()

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

    function buildCellDelegateItems() {
        headerModel.revision
        const count = Math.max(1, resolvedColumnCount)
        const result = []
        for (let i = 0; i < count; i++) {
            const descriptor = headerModel.descriptorAt(i)
            result.push({
                "index": i,
                "descriptor": descriptor,
                "cellData": descriptor.cellData,
                "text": descriptor.text,
                "valueType": descriptor.valueType,
                "x": descriptor.x,
                "width": descriptor.width,
                "height": control.rowHeight,
                "padding": descriptor.padding
            })
        }
        return result
    }

    function createCellDelegate(parentItem, descriptor) {
        if (!resolvedCellDelegate)
            return null
        return resolvedCellDelegate.createObject(parentItem, {
            "modelData": descriptor
        })
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
                model: control.cellDelegateItems

                delegate: Item {
                    id: headerCell
                    required property var modelData
                    property Item delegateItem: null

                    x: modelData.x
                    width: modelData.width
                    height: control.rowHeight

                    function rebuildDelegate() {
                        if (delegateItem) {
                            delegateItem.destroy()
                            delegateItem = null
                        }
                        delegateItem = control.createCellDelegate(headerCell, modelData)
                        if (!delegateItem)
                            return
                        delegateItem.width = Qt.binding(function() { return headerCell.width })
                        delegateItem.height = Qt.binding(function() { return headerCell.height })
                    }

                    Component.onCompleted: rebuildDelegate()
                    onModelDataChanged: rebuildDelegate()

                    Connections {
                        target: control
                        function onCellDelegateChanged() {
                            headerCell.rebuildDelegate()
                        }
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
