pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: control

    property var items: ["Label", "Label", "Label", "Label", "Label", "Label"]
    property int selectedIndex: 1
    property bool interactive: true

    // Optional top toolbar support for existing callers.
    property bool toolbarVisible: false
    property string toolbarIcon1: ""
    property string toolbarIcon2: ""
    property string toolbarIcon3: ""

    property bool footerVisible: true
    property var footerButton1: ({ type: "icon", iconName: "projectStructure" })
    property var footerButton2: ({ type: "icon", iconName: "delete" })
    property var footerButton3: ({ type: "icon", iconName: "cwmPermissionView" })

    property int listWidth: 170
    property int minimumListHeight: 223
    property int itemHeight: Theme.gap24
    property int itemLabelLeftPadding: Theme.gap8
    property color backgroundColor: Theme.panelBackground03
    property color selectedRowColor: Theme.primary
    property color separatorColor: "#1A000000"
    property real separatorOpacity: 0.5

    signal itemTriggered(int index, var item)
    signal toolbarIconTriggered(int index, string source)
    signal footerButtonTriggered(int index, var config)

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

    readonly property int contentHeight: {
        const toolbarHeight = toolbar.visible ? toolbar.implicitHeight : 0
        const footerHeight = footer.visible ? footer.implicitHeight : 0
        return toolbarHeight + listItemsColumn.implicitHeight + footerHeight
    }

    implicitWidth: control.listWidth
    implicitHeight: Math.max(control.minimumListHeight, contentHeight)

    Rectangle {
        anchors.fill: parent
        color: control.backgroundColor
    }

    ColumnLayout {
        id: rootColumn
        anchors.fill: parent
        spacing: Theme.gapNone

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

        Item {
            id: listItemsViewport
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Column {
                id: listItemsColumn
                width: listItemsViewport.width
                spacing: Theme.gapNone

                Repeater {
                    model: control.entryCount

                    delegate: AbstractButton {
                        id: rowButton
                        required property int index
                        readonly property var entry: control.entryAt(index)
                        readonly property bool rowSelected: control.itemSelected(entry, index)
                        readonly property bool rowEnabled: control.interactive && control.itemEnabled(entry)

                        width: listItemsColumn.width
                        height: control.itemHeight
                        implicitHeight: control.itemHeight
                        tone: AbstractButton.Borderless
                        enabled: rowEnabled
                        horizontalPadding: Theme.gapNone
                        verticalPadding: Theme.gapNone
                        spacing: Theme.gapNone
                        cornerRadius: Theme.gapNone
                        backgroundColor: rowSelected ? control.selectedRowColor : "transparent"
                        backgroundColorHover: rowSelected ? control.selectedRowColor : "transparent"
                        backgroundColorPressed: rowSelected ? control.selectedRowColor : Theme.accentBlueMuted
                        backgroundColorDisabled: rowSelected ? control.selectedRowColor : "transparent"
                        textColor: Theme.bodyColor
                        textColorDisabled: Theme.disabledColor

                        contentItem: Item {
                            Label {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.leftMargin: control.itemLabelLeftPadding
                                anchors.rightMargin: Theme.gap8
                                anchors.verticalCenter: parent.verticalCenter
                                style: body
                                text: control.itemLabel(rowButton.entry)
                                color: rowButton.rowEnabled ? Theme.bodyColor : Theme.disabledColor
                                font.pixelSize: 13
                                font.weight: Font.Normal
                                font.styleName: "Regular"
                                lineHeight: 16
                                lineHeightMode: Text.FixedHeight
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: control.separatorColor
                                opacity: control.separatorOpacity
                                visible: !rowButton.rowSelected
                            }
                        }

                        onClicked: control.itemTriggered(index, entry)
                    }
                }
            }
        }

        ListFooter {
            id: footer
            visible: control.footerVisible
            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            interactive: control.interactive
            button1: control.footerButton1
            button2: control.footerButton2
            button3: control.footerButton3
            onButtonClicked: (index, config) => control.footerButtonTriggered(index, config)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.List { items: [{ label: "Item 1" }, { label: "Item 2" }] }
