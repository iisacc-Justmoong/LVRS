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
    property string iconRole: "iconName"
    property string enabledRole: "enabled"
    property string selectedRole: "selected"
    property int selectedIndex: -1
    property bool interactive: true

    // Optional top toolbar support for existing callers.
    property bool toolbarVisible: false
    property string toolbarIcon1: ""
    property string toolbarIcon2: ""
    property string toolbarIcon3: ""

    property bool footerVisible: true
    property var footerButton1: ({ type: "icon", iconName: "addFile" })
    property var footerButton2: ({ type: "icon", iconName: "generaldelete" })
    property var footerButton3: ({ type: "menu", iconName: "settings" })

    property int listWidth: Theme.scaleMetric(170)
    property int minimumListHeight: Theme.scaleMetric(140)
    property int itemHeight: Theme.iconSm + (Theme.gap2 * 2)
    property int itemLabelLeftPadding: Theme.gap4
    property string defaultItemIconName: "nodesfolder"
    property bool expandToContent: false
    property color backgroundColor: Theme.panelBackground03
    property color selectedRowColor: Theme.primary
    property color separatorColor: "#1A000000"
    property real separatorOpacity: 0.5
    property Component itemDelegate: defaultItemDelegate

    signal itemTriggered(int index, var item)
    signal toolbarIconTriggered(int index, string source)
    signal footerButtonTriggered(int index, var config)

    readonly property bool usingModel: model !== undefined && model !== null
    readonly property var sourceModel: usingModel ? model : items
    readonly property int entryCount: listModelSource.count
    readonly property var itemDelegateItems: buildItemDelegateItems()

    Component {
        id: defaultItemDelegate

        ListItem {
            id: rowButton
            objectName: "list_defaultDelegate_" + index
            property var modelData: ({})
            property int index: modelData.index === undefined ? -1 : modelData.index
            readonly property var entry: modelData.entry
            readonly property bool rowSelected: modelData.selected === true
            readonly property bool rowEnabled: modelData.enabled === true

            width: parent ? parent.width : control.listWidth
            size: ListItem.Mini
            label: rowButton.modelData.label || ""
            iconName: rowButton.modelData.iconName || control.defaultItemIconName
            selected: rowSelected
            enabled: rowEnabled
            rowHorizontalPadding: control.itemLabelLeftPadding
            rowVerticalPadding: Theme.gap2
            miniItemWidth: control.listWidth
            minItemWidth: control.listWidth
            listBackgroundColor: "transparent"
            selectedBackgroundColor: control.selectedRowColor
            separatorColor: control.separatorColor
            separatorOpacity: control.separatorOpacity
            separatorVisible: false

            onClicked: control.triggerItem(index)
        }
    }

    ModelSource {
        id: listModelSource
        source: control.sourceModel
        column: control.modelColumn
    }

    function invalidateModel() {
        listModelSource.invalidate()
    }

    function entryAt(index) {
        listModelSource.revision
        return listModelSource.at(index)
    }

    function roleValue(entry, roleName, fallbackValue) {
        return listModelSource.roleValue(entry, roleName, fallbackValue)
    }

    function itemLabel(entry) {
        return listModelSource.textValue(entry, [labelRole, textRole, titleRole, "display", "edit"], "")
    }

    function itemEnabled(entry) {
        return listModelSource.boolValue(entry, enabledRole, true)
    }

    function itemIconName(entry) {
        const roles = [iconRole, "icon", "sourceIcon"]
        for (let index = 0; index < roles.length; ++index) {
            const value = roleValue(entry, roles[index], null)
            if (value !== null && value !== undefined && String(value).length > 0)
                return String(value)
        }
        return defaultItemIconName
    }

    function itemSelected(entry, index) {
        const value = roleValue(entry, selectedRole, null)
        if (value !== null && value !== undefined)
            return !!value
        return index === selectedIndex
    }

    function triggerItem(index) {
        const entry = entryAt(index)
        itemTriggered(index, entry)
    }

    function buildItemDelegateItems() {
        listModelSource.revision
        const count = entryCount
        const result = []
        for (let i = 0; i < count; i++) {
            const entry = entryAt(i)
            result.push({
                "index": i,
                "entry": entry,
                "label": itemLabel(entry),
                "iconName": itemIconName(entry),
                "enabled": interactive && itemEnabled(entry),
                "selected": itemSelected(entry, i),
                "trigger": function() { control.triggerItem(i) }
            })
        }
        return result
    }

    function createDelegateItem(parentItem, component, descriptor) {
        if (!component)
            return null
        return component.createObject(parentItem, {
            "modelData": descriptor
        })
    }

    readonly property int contentHeight: {
        const toolbarHeight = toolbar.visible ? toolbar.implicitHeight : 0
        const footerHeight = footer.visible ? footer.implicitHeight : 0
        return toolbarHeight + listItemsColumn.height + footerHeight
    }

    implicitWidth: control.listWidth
    implicitHeight: control.expandToContent
        ? Math.max(control.minimumListHeight, contentHeight)
        : control.minimumListHeight

    onSourceModelChanged: invalidateModel()

    Rectangle {
        objectName: "list_background"
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
            objectName: "list_itemsViewport"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Item {
                id: listItemsColumn
                objectName: "list_itemsColumn"
                width: listItemsViewport.width
                height: control.entryCount * control.itemHeight

                Repeater {
                    model: control.entryCount

                    delegate: Item {
                        id: delegateRoot
                        objectName: "list_delegateRoot_" + index
                        required property int index
                        readonly property var descriptor: index >= 0
                            && index < control.itemDelegateItems.length
                            ? control.itemDelegateItems[index]
                            : ({})
                        property Item delegateItem: null

                        y: index * control.itemHeight
                        width: listItemsColumn.width
                        height: delegateItem
                            ? Math.max(control.itemHeight, delegateItem.implicitHeight)
                            : control.itemHeight
                        implicitHeight: height

                        function rebuildDelegate() {
                            if (delegateItem) {
                                delegateItem.destroy()
                                delegateItem = null
                            }
                            delegateItem = control.createDelegateItem(delegateRoot,
                                                                      control.itemDelegate,
                                                                      descriptor)
                            if (!delegateItem)
                                return
                            delegateItem.width = Qt.binding(function() { return delegateRoot.width })
                            delegateItem.height = Qt.binding(function() { return delegateRoot.height })
                        }

                        Component.onCompleted: rebuildDelegate()
                        onDescriptorChanged: rebuildDelegate()

                        Connections {
                            target: control
                            function onItemDelegateChanged() {
                                delegateRoot.rebuildDelegate()
                            }
                        }
                    }
                }
            }
        }

        ListFooter {
            id: footer
            objectName: "list_footer"
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
