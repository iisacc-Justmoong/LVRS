pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Controls.Control {
    id: control
    enum WidthPolicy { Equal, Content, Scrollable }
    property var model: []
    property int currentIndex: count > 0 ? 0 : -1
    property bool autoSelect: true
    property bool motionEnabled: true
    property int tabStyle: Tab.Underline
    property int widthPolicy: TabBar.Equal
    property real minimumTabWidth: 120
    property real scrollableTabWidth: 192
    property real itemSpacing: 4
    property string itemObjectNamePrefix: "tab"
    property Component delegate: Tab {
        required property int index
        required property var modelData
        objectName: modelData.objectName || control.itemObjectNamePrefix + index
        labelObjectName: modelData.labelObjectName || objectName + "Label"
        text: typeof modelData === "string" ? modelData : (modelData.text || "")
        iconName: modelData.iconName || ""
        badge: modelData.badge || ""
        enabled: modelData.enabled !== false
        selected: index === control.currentIndex
        tabStyle: control.tabStyle
        motionEnabled: control.motionEnabled
        width: control.itemWidth(index, implicitWidth)
        height: control.availableHeight
        navigationBar: control
        tabIndex: index
        onClicked: control.activate(index)
    }
    readonly property int count: items.count
    readonly property real contentWidth: row.width
    signal activated(int index)

    padding: 4
    spacing: itemSpacing
    implicitWidth: 640
    implicitHeight: 40
    Accessible.role: Accessible.PageTabList
    Accessible.name: qsTr("Tabs")

    function itemAt(index) { return items.itemAt(index) }
    function itemWidth(index, naturalWidth) {
        if (widthPolicy === TabBar.Equal)
            return Math.max(0, (availableWidth - itemSpacing * Math.max(0, count - 1)) / Math.max(1, count))
        return Math.max(naturalWidth, widthPolicy === TabBar.Scrollable ? scrollableTabWidth : minimumTabWidth)
    }
    function activate(index) {
        const item = itemAt(index)
        if (!item || !item.visible || !item.enabled) return
        if (autoSelect) currentIndex = index
        activated(index)
    }
    function focusItem(index) {
        const item = itemAt(index)
        if (!item || !item.visible || !item.enabled) return false
        item.forceActiveFocus(Qt.TabFocusReason)
        if (item.x < viewport.contentX) viewport.contentX = item.x
        else if (item.x + item.width > viewport.contentX + viewport.width)
            viewport.contentX = Math.max(0, item.x + item.width - viewport.width)
        return true
    }
    function moveFocus(from, direction) {
        for (let step = 1; step <= count; ++step) {
            const index = ((from + direction * step) % count + count) % count
            if (focusItem(index)) return
        }
    }
    function focusBoundary(last) {
        for (let step = 0; step < count; ++step)
            if (focusItem(last ? count - 1 - step : step)) return
    }
    function reconcileSelection() {
        if (!autoSelect) return
        if (currentIndex >= 0 && currentIndex < count && itemAt(currentIndex)?.enabled) return
        currentIndex = -1
        for (let i = 0; i < count; ++i) {
            if (itemAt(i)?.enabled) { currentIndex = i; break }
        }
    }
    onCountChanged: Qt.callLater(reconcileSelection)
    onModelChanged: Qt.callLater(reconcileSelection)
    onCurrentIndexChanged: Qt.callLater(function() {
        const item = itemAt(currentIndex)
        if (item && item.visible && widthPolicy === TabBar.Scrollable) {
            viewport.contentX = Math.max(0, Math.min(item.x, viewport.contentWidth - viewport.width))
        }
    })

    background: Rectangle {
        color: control.tabStyle === Tab.Surface ? Theme.panelBackground04 : "transparent"
        radius: 8
    }
    contentItem: Flickable {
        id: viewport
        objectName: "tabViewport"
        contentWidth: row.width
        contentHeight: height
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        interactive: contentWidth > width
        clip: true
        Row {
            id: row
            height: viewport.height
            spacing: control.itemSpacing
            layoutDirection: control.mirrored ? Qt.RightToLeft : Qt.LeftToRight
            Repeater { id: items; model: control.model; delegate: control.delegate }
        }
    }
}

// LV.TabBar { model: [{text: "Overview"}, {text: "Activity"}]; onActivated: index => stack.currentIndex = index }
