pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: control

    property var items: []
    property bool toolbarVisible: true
    property string toolbarIcon1: ""
    property string toolbarIcon2: ""
    property string toolbarIcon3: ""
    property int selectedIndex: -1
    property bool interactive: true

    property color backgroundColor: Theme.surfaceSolid
    property color rowColor: Theme.surfaceGhost
    property int rowSpacing: Theme.gap4
    property int horizontalPadding: Theme.gap8
    property int verticalPadding: Theme.gap8

    signal itemTriggered(int index, var item)
    signal toolbarIconTriggered(int index, string source)

    readonly property int entryCount: {
        if (!items)
            return 0
        if (items.length !== undefined)
            return items.length
        if (items.count !== undefined)
            return items.count
        return 0
    }

    function entryAt(index) {
        if (!items)
            return null
        if (items.length !== undefined)
            return items[index]
        if (items.get !== undefined)
            return items.get(index)
        return null
    }

    function itemLabel(entry) {
        if (typeof entry === "string")
            return entry
        if (!entry || typeof entry !== "object")
            return ""
        return entry.label || entry.text || entry.title || ""
    }

    function itemDetail(entry) {
        if (!entry || typeof entry !== "object")
            return ""
        return entry.detail || entry.key || entry.subtitle || ""
    }

    function itemIcon(entry) {
        if (!entry || typeof entry !== "object")
            return ""
        return entry.icon || entry.iconName || entry.source || ""
    }

    function itemEnabled(entry) {
        if (!entry || typeof entry !== "object")
            return true
        if (entry.enabled === undefined)
            return true
        return !!entry.enabled
    }

    function itemSelected(entry, index) {
        if (entry && typeof entry === "object" && entry.selected !== undefined)
            return !!entry.selected
        return index === selectedIndex
    }

    function itemShowChevron(entry) {
        if (!entry || typeof entry !== "object")
            return false
        if (entry.showChevron !== undefined)
            return !!entry.showChevron
        return !!entry.hasSubmenu
    }

    implicitWidth: Math.max(Theme.inputWidthMd + Theme.gap14, listColumn.implicitWidth + (horizontalPadding * 2))
    implicitHeight: listColumn.implicitHeight + (verticalPadding * 2)

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusMd
        color: control.backgroundColor
        antialiasing: true
    }

    ColumnLayout {
        id: listColumn
        anchors.fill: parent
        anchors.leftMargin: control.horizontalPadding
        anchors.rightMargin: control.horizontalPadding
        anchors.topMargin: control.verticalPadding
        anchors.bottomMargin: control.verticalPadding
        spacing: control.rowSpacing

        ListToolbar {
            id: toolbar
            visible: control.toolbarVisible
            Layout.fillWidth: true
            icon1: control.toolbarIcon1
            icon2: control.toolbarIcon2
            icon3: control.toolbarIcon3
            interactive: control.interactive
            onIconClicked: (index, source) => control.toolbarIconTriggered(index, source)
        }

        Repeater {
            model: control.entryCount

            delegate: ListItem {
                required property int index
                readonly property var entry: control.entryAt(index)
                Layout.fillWidth: true
                label: control.itemLabel(entry)
                detail: control.itemDetail(entry)
                iconName: control.itemIcon(entry)
                showChevron: control.itemShowChevron(entry)
                selected: control.itemSelected(entry, index)
                listBackgroundColor: control.rowColor
                enabled: control.interactive && control.itemEnabled(entry)
                onClicked: control.itemTriggered(index, entry)
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.List { toolbarIcon1: "iconname"; toolbarIcon2: "iconname"; toolbarIcon3: "iconname"; items: [{label: "Item"}] }
