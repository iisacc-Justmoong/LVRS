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

    function normalizeColumnType(value) {
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

    function inferredColumnType(value) {
        if (typeof value === "boolean")
            return "bool"
        if (typeof value === "number")
            return Math.floor(value) === value ? "int" : "float"
        return "string"
    }

    function columnType(index) {
        if (resolvedColumnCount <= 0)
            return "string"

        const entry = columnAt(index)
        if (entry && typeof entry === "object") {
            if (entry.type !== undefined && entry.type !== null)
                return normalizeColumnType(entry.type)
            if (entry.valueType !== undefined && entry.valueType !== null)
                return normalizeColumnType(entry.valueType)
            if (entry.cellType !== undefined && entry.cellType !== null)
                return normalizeColumnType(entry.cellType)
            if (entry.dataType !== undefined && entry.dataType !== null)
                return normalizeColumnType(entry.dataType)
            if (entry.value !== undefined && entry.value !== null)
                return inferredColumnType(entry.value)
            return "string"
        }
        return inferredColumnType(entry)
    }

    function columnText(index) {
        if (resolvedColumnCount <= 0)
            return "Column"

        const entry = columnAt(index)
        if (typeof entry === "string" || typeof entry === "number" || typeof entry === "boolean")
            return String(entry)
        if (!entry || typeof entry !== "object")
            return "Column"
        if (entry.label !== undefined && entry.label !== null)
            return String(entry.label)
        if (entry.text !== undefined && entry.text !== null)
            return String(entry.text)
        if (entry.title !== undefined && entry.title !== null)
            return String(entry.title)
        if (entry.value !== undefined && entry.value !== null)
            return String(entry.value)
        return "Column"
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

    function numericWidth(value, fallbackValue) {
        const parsed = Number(value)
        if (isNaN(parsed) || parsed <= 0)
            return fallbackValue
        return Math.max(minColumnWidth, Math.round(parsed))
    }

    function autoColumnWidth() {
        const count = Math.max(1, resolvedColumnCount)
        return Math.max(minColumnWidth, Math.floor(width / count))
    }

    function columnWidth(index) {
        if (Array.isArray(columnWidths) && index >= 0 && index < columnWidths.length)
            return numericWidth(columnWidths[index], fallbackCellWidth > 0 ? fallbackCellWidth : autoColumnWidth())
        if (fallbackCellWidth > 0)
            return numericWidth(fallbackCellWidth, autoColumnWidth())
        return autoColumnWidth()
    }

    function columnX(index) {
        let xValue = 0
        for (let i = 0; i < index; i++)
            xValue += columnWidth(i)
        return xValue
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

                    x: control.columnX(index)
                    width: control.columnWidth(index)
                    height: control.rowHeight

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
// LV.TableHeader { cellItems: [{ label: "Name", type: "string" }, { label: "Count", type: "int" }] }
