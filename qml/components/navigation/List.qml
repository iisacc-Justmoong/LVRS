pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: control

    property var model: null
    property var items: ["Label", "Label", "Label", "Label", "Label", "Label"]
    property int modelColumn: 0
    property string labelRole: "label"
    property string textRole: "text"
    property string titleRole: "title"
    property string enabledRole: "enabled"
    property string selectedRole: "selected"
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

    property int listWidth: Theme.scaleMetric(170)
    property int minimumListHeight: Theme.scaleMetric(223)
    property int itemHeight: Theme.gap24
    property int itemLabelLeftPadding: Theme.gap8
    property color backgroundColor: Theme.panelBackground03
    property color selectedRowColor: Theme.primary
    property color separatorColor: "#1A000000"
    property real separatorOpacity: 0.5

    signal itemTriggered(int index, var item)
    signal toolbarIconTriggered(int index, string source)
    signal footerButtonTriggered(int index, var config)

    property int _modelRevision: 0
    readonly property bool usingModel: model !== undefined && model !== null
    readonly property var sourceModel: usingModel ? model : items
    readonly property int entryCount: {
        _modelRevision
        return modelCount(sourceModel)
    }

    function invalidateModel() {
        _modelRevision += 1
    }

    function modelCount(source) {
        if (!source)
            return 0
        if (ModelAdapter.isItemModel(source))
            return ModelAdapter.count(source)
        if (source.length !== undefined)
            return Math.max(0, Number(source.length) || 0)
        if (source.count !== undefined)
            return Math.max(0, Number(source.count) || 0)
        return 0
    }

    function entryAt(index) {
        const source = sourceModel
        if (!source)
            return null
        if (ModelAdapter.isItemModel(source))
            return ModelAdapter.row(source, index, modelColumn)
        if (source.get !== undefined)
            return source.get(index)
        if (source.length !== undefined)
            return source[index]
        if (source.data !== undefined)
            return source.data(index)
        if (source.at !== undefined)
            return source.at(index)
        return null
    }

    function roleValue(entry, roleName, fallbackValue) {
        if (!entry || typeof entry !== "object")
            return fallbackValue
        const key = roleName === undefined || roleName === null ? "" : String(roleName).trim()
        if (key.length === 0)
            return fallbackValue
        if (entry[key] !== undefined)
            return entry[key]
        return fallbackValue
    }

    function itemLabel(entry) {
        if (entry === undefined || entry === null)
            return ""
        if (typeof entry !== "object")
            return String(entry)
        let value = roleValue(entry, labelRole, undefined)
        if (value === undefined)
            value = roleValue(entry, textRole, undefined)
        if (value === undefined)
            value = roleValue(entry, titleRole, undefined)
        if (value === undefined)
            value = roleValue(entry, "display", undefined)
        if (value === undefined)
            value = roleValue(entry, "edit", "")
        if (value === undefined || value === null)
            return ""
        return String(value)
    }

    function itemEnabled(entry) {
        if (!entry || typeof entry !== "object")
            return true
        const value = roleValue(entry, enabledRole, undefined)
        if (value === undefined)
            return true
        return !!value
    }

    function itemSelected(entry, index) {
        if (!entry || typeof entry !== "object")
            return index === selectedIndex
        const value = roleValue(entry, selectedRole, undefined)
        if (value !== undefined)
            return !!value
        return index === selectedIndex
    }

    readonly property int contentHeight: {
        const toolbarHeight = toolbar.visible ? toolbar.implicitHeight : 0
        const footerHeight = footer.visible ? footer.implicitHeight : 0
        return toolbarHeight + listItemsColumn.implicitHeight + footerHeight
    }

    implicitWidth: control.listWidth
    implicitHeight: Math.max(control.minimumListHeight, contentHeight)

    onSourceModelChanged: invalidateModel()

    Connections {
        target: ModelAdapter.isItemModel(control.sourceModel) ? control.sourceModel : null
        enabled: ModelAdapter.isItemModel(control.sourceModel)
        ignoreUnknownSignals: true

        function onRowsInserted() { control.invalidateModel() }
        function onRowsRemoved() { control.invalidateModel() }
        function onRowsMoved() { control.invalidateModel() }
        function onModelReset() { control.invalidateModel() }
        function onLayoutChanged() { control.invalidateModel() }
        function onDataChanged() { control.invalidateModel() }
    }

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
                                font.pixelSize: Theme.scaleTextMetric(13)
                                font.weight: Font.Normal
                                font.styleName: "Regular"
                                lineHeight: Theme.scaleTextMetric(16)
                                lineHeightMode: Text.FixedHeight
                                horizontalAlignment: Text.AlignLeft
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: Theme.scaleMetric(1)
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
// LV.List { model: [{ label: "Item 1" }, { label: "Item 2" }] }
