import QtQuick
import QtQuick.Controls
import LVRS 1.0

Rectangle {
    id: control

    property int minimumPanelWidth: Theme.scaleMetric(200)
    property int minimumPanelHeight: Theme.scaleMetric(530)
    property color panelColor: Theme.panelBackground05
    property bool clipList: true

    property alias toolbarButtons: toolbar.buttons
    property alias toolbarItems: toolbar.buttonItems
    property alias activeToolbarButton: toolbar.activeButton
    property alias activeToolbarButtonId: toolbar.activeButtonId
    property alias activeToolbarIndex: toolbar.activeIndex
    property alias activeListItem: hierarchyList.activeItem
    property alias activeListItemId: hierarchyList.activeItemId
    property alias activeListItemKey: hierarchyList.activeItemKey
    property alias model: hierarchyList.model
    property alias treeModel: hierarchyList.model
    property alias depthRole: hierarchyList.depthRole
    property alias draggableRole: hierarchyList.draggableRole
    property alias keyboardListNavigationEnabled: hierarchyList.keyboardNavigationEnabled
    property alias editable: hierarchyList.editable
    default property alias listItems: hierarchyList.items

    // Optional bottom-left ListFooter API.
    property bool footerVisible: false
    property bool footerInteractive: true
    property var footerButton1: ({ type: "icon", iconName: "projectStructure" })
    property var footerButton2: ({ type: "icon", iconName: "delete" })
    property var footerButton3: ({ type: "icon", iconName: "cwmPermissionView" })

    signal toolbarActivated(var button, var buttonId, int index)
    signal toolbarButtonTriggered(var button, var buttonId, int index, var item)
    signal toolbarEventTriggered(string eventName, var payload, int index, var item, var buttonId)
    signal listItemActivated(var item, int itemId, int index)
    signal listItemExpanded(var item, int itemId, int index, bool expanded)
    signal listItemMoved(var item, int itemId, string itemKey, int fromIndex, int toIndex, int depth)
    signal footerButtonTriggered(int index, var config)

    implicitWidth: minimumPanelWidth
    implicitHeight: minimumPanelHeight
    color: panelColor
    clip: true

    function ensureListItemVisible(itemY, itemHeight) {
        const contentTop = listViewport.contentY
        const contentBottom = contentTop + listViewport.height
        const targetTop = Math.max(0, itemY - Theme.gap4)
        const targetBottom = itemY + itemHeight + Theme.gap4

        if (targetTop < contentTop) {
            listViewport.contentY = targetTop
            return
        }

        if (targetBottom > contentBottom) {
            const nextContentY = targetBottom - listViewport.height
            const maxContentY = Math.max(0, listViewport.contentHeight - listViewport.height)
            listViewport.contentY = Math.max(0, Math.min(nextContentY, maxContentY))
        }
    }

    function expandAll() {
        hierarchyList.expandAll()
    }

    function collapseAll(keepRootExpanded) {
        hierarchyList.collapseAll(keepRootExpanded)
    }

    function activateListItemById(itemId) {
        return hierarchyList.activateById(itemId)
    }

    function activateListItemByKey(itemKey) {
        return hierarchyList.activateByKey(itemKey)
    }

    function triggerFooterButton(index) {
        if (!footer.visible || !footer.dispatchClicked)
            return false
        footer.dispatchClicked(index)
        return true
    }

    HierarchyToolbar {
        id: toolbar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        z: 10
        onActiveChanged: function(button, buttonId, index) {
            control.toolbarActivated(button, buttonId, index)
        }
        onButtonTriggered: function(button, buttonId, index, item) {
            control.toolbarButtonTriggered(button, buttonId, index, item)
        }
        onButtonEventTriggered: function(eventName, payload, index, item, buttonId) {
            control.toolbarEventTriggered(eventName, payload, index, item, buttonId)
        }
    }

    Flickable {
        id: listViewport
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: toolbar.bottom
        anchors.bottom: footer.visible ? footer.top : parent.bottom
        clip: control.clipList
        contentWidth: width
        contentHeight: hierarchyList.implicitHeight
        boundsBehavior: Flickable.StopAtBounds

        HierarchyList {
            id: hierarchyList
            width: listViewport.width
            onActiveChanged: function(item, itemId, index) {
                control.listItemActivated(item, itemId, index)
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }
    }

    ListFooter {
        id: footer
        objectName: "hierarchyFooter"
        visible: control.footerVisible
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        interactive: control.footerInteractive
        button1: control.footerButton1
        button2: control.footerButton2
        button3: control.footerButton3
        onButtonClicked: function(index, config) {
            control.footerButtonTriggered(index, config)
        }
    }

    WheelScrollGuard {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: toolbar.bottom
        anchors.bottom: listViewport.bottom
        targetFlickable: listViewport
        consumeInside: true
    }

    Connections {
        target: hierarchyList
        function onEnsureVisibleRequested(y, height) {
            control.ensureListItemVisible(y, height)
        }
        function onExpansionChanged(item, expanded, index) {
            control.listItemExpanded(item, hierarchyList.effectiveItemId(item, index), index, expanded)
        }
        function onItemMoved(item, itemId, itemKey, fromIndex, toIndex, depth) {
            control.listItemMoved(item, itemId, itemKey, fromIndex, toIndex, depth)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Hierarchy {
//     toolbarItems: [
//         { id: "structure", iconName: "projectStructure", eventName: "hierarchy.structure" },
//         { id: "layers", iconName: "projectStructure", events: ["hierarchy.layers", "analytics.layers"] }
//     ]
//     model: [
//         { key: "root", depth: 0, label: "Root", expanded: true },
//         { key: "child", depth: 1, label: "Child" }
//     ]
//     footerVisible: true
//     footerButton1: ({ type: "icon", iconName: "projectStructure" })
// }
