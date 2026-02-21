import QtQuick
import LVRS 1.0

Item {
    id: control

    property var itemData: null
    property string text: "Text"
    property int cellHeight: 24
    property int contentSpacing: Theme.gap8
    property color dividerColor: Theme.panelBackground03
    property color textColor: Theme.bodyColor
    property bool showDivider: true
    property bool clipContent: true
    property int textStyle: body

    readonly property int title: 0
    readonly property int title2: 1
    readonly property int header: 2
    readonly property int header2: 3
    readonly property int body: 4
    readonly property int description: 5
    readonly property int caption: 6
    readonly property int disabled: 7

    readonly property string resolvedText: {
        const entry = control.itemData
        if (entry && typeof entry === "object") {
            if (entry.label !== undefined && entry.label !== null)
                return String(entry.label)
            if (entry.text !== undefined && entry.text !== null)
                return String(entry.text)
            if (entry.title !== undefined && entry.title !== null)
                return String(entry.title)
        }
        return control.text
    }
    readonly property int resolvedCellHeight: {
        const value = control.itemValue("cellHeight", control.cellHeight)
        const parsed = Number(value)
        return isNaN(parsed) || parsed <= 0 ? control.cellHeight : parsed
    }
    readonly property int resolvedContentSpacing: {
        const value = control.itemValue("contentSpacing", control.contentSpacing)
        const parsed = Number(value)
        return isNaN(parsed) || parsed < 0 ? control.contentSpacing : parsed
    }
    readonly property color resolvedDividerColor: control.itemValue("dividerColor", control.dividerColor)
    readonly property color resolvedTextColor: control.itemValue("textColor", control.textColor)
    readonly property bool resolvedShowDivider: {
        const value = control.itemValue("showDivider", control.showDivider)
        return !!value
    }
    readonly property bool resolvedClipContent: {
        const value = control.itemValue("clipContent", control.clipContent)
        return !!value
    }
    readonly property int resolvedTextStyle: {
        const value = control.itemValue("textStyle", control.textStyle)
        const parsed = Number(value)
        return isNaN(parsed) ? control.textStyle : parsed
    }

    function itemValue(key, fallbackValue) {
        const entry = control.itemData
        if (!entry || typeof entry !== "object")
            return fallbackValue
        if (Object.prototype.hasOwnProperty.call(entry, key))
            return entry[key]
        return fallbackValue
    }

    implicitWidth: 234
    implicitHeight: resolvedCellHeight
    clip: resolvedClipContent

    Rectangle {
        id: dividerNode
        visible: control.resolvedShowDivider
        width: Theme.strokeThin
        height: parent.height
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        color: control.resolvedDividerColor
        antialiasing: false
    }

    Label {
        anchors.left: dividerNode.visible ? dividerNode.right : parent.left
        anchors.leftMargin: dividerNode.visible ? control.resolvedContentSpacing : 0
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        style: control.resolvedTextStyle
        text: control.resolvedText
        color: control.resolvedTextColor
        elide: Text.ElideRight
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.TableCellItem { itemData: ({ text: "Text" }) }
