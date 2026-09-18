pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Controls.Control {
    id: control
    // Figma TabBar/LVRS Mobile 1074:2064 and elastic motion 1076:1972.
    property var model: []
    // Omit Search unless an independent configuration object is supplied.
    property var search: null
    readonly property bool searchVisible: search !== null && typeof search === "object" && search.visible !== false
    property int currentIndex: count > 0 ? 0 : -1
    property bool autoSelect: true
    property bool motionEnabled: true
    property int platformStyle: MobileTab.Automatic
    // Non-negative values are explicit device geometry in logical pixels.
    property real deviceCornerRadius: -1
    property real bottomSafeInset: 0
    property string itemObjectNamePrefix: "mobileNavigationTab"
    readonly property int count: tabs.count
    readonly property int resolvedPlatformStyle: platformStyle === MobileTab.Automatic
        ? (Theme.effectiveTarget === "android" ? MobileTab.Android : MobileTab.IOS) : platformStyle
    readonly property real resolvedCornerRadius: Math.max(0, Math.min(Theme.mobileNavigationHeight / 2,
        deviceCornerRadius >= 0 ? deviceCornerRadius
        : resolvedPlatformStyle === MobileTab.Android && Platform.android && windowGeometry.bottomCornerRadius >= 0
            ? windowGeometry.bottomCornerRadius
        : resolvedPlatformStyle === MobileTab.Android ? Theme.mobileNavigationRadiusAndroid : Theme.mobileNavigationRadiusIOS))
    readonly property real maximumGroupWidth: Math.max(0, availableWidth
        - (searchVisible ? Theme.mobileNavigationHeight + Theme.gap12 : 0))
    readonly property real tabWidth: Math.max(Theme.mobileNavigationTouchSize,
        Math.min(Theme.mobileNavigationTabWidth, (maximumGroupWidth - 2 * Theme.gap4) / Math.max(1, count)))
    readonly property real groupWidth: count > 0 ? Math.min(maximumGroupWidth, count * tabWidth + 2 * Theme.gap4) : 0
    readonly property bool animated: motionEnabled && Motion.animated
    readonly property bool selectionVisible: currentIndex >= 0 && currentIndex < count
    property bool ready: false
    signal activated(int index)
    signal searchRequested()

    implicitWidth: resolvedPlatformStyle === MobileTab.Android ? 412 : 402
    implicitHeight: Theme.mobileNavigationHeight + Theme.gap16 * 2 + Math.max(0, bottomSafeInset)
    leftPadding: Theme.gap16
    rightPadding: Theme.gap16
    topPadding: Theme.gap16
    bottomPadding: Theme.gap16 + Math.max(0, bottomSafeInset)
    Accessible.role: Accessible.PageTabList
    Accessible.name: qsTr("Main navigation")

    WindowSafeAreaObserver { id: windowGeometry; window: control.Window.window }

    function itemAt(index) { return tabs.itemAt(index) }
    function activate(index) {
        const item = itemAt(index)
        if (!enabled || !item || !item.enabled) return
        if (autoSelect) currentIndex = index
        activated(index)
    }
    function reveal(index) {
        const item = itemAt(index)
        if (!item) return
        let offset = viewport.contentX
        if (item.x < offset) offset = item.x
        else if (item.x + item.width > offset + viewport.width) offset = item.x + item.width - viewport.width
        viewport.contentX = Math.max(0, Math.min(offset, viewport.contentWidth - viewport.width))
    }
    function focusItem(index) {
        const item = index === count ? searchButton : itemAt(index)
        if (!enabled || !item || !item.enabled || !item.visible) return false
        item.forceActiveFocus(Qt.TabFocusReason)
        if (index < count) {
            reveal(index)
            Qt.callLater(function() { if (item.activeFocus) reveal(index) })
        }
        return true
    }
    function moveFocus(from, direction) {
        for (let step = 1; step <= count + 1; ++step)
            if (focusItem(((from + direction * step) % (count + 1) + count + 1) % (count + 1))) return
    }
    function focusBoundary(last) {
        for (let step = 0; step <= count; ++step)
            if (focusItem(last ? count - step : step)) return
    }
    function revealFocusedOrSelected() {
        for (let i = 0; i < count; ++i) {
            if (itemAt(i)?.activeFocus) { reveal(i); return }
        }
        reveal(currentIndex)
    }
    function reconcileSelection() {
        if (autoSelect && enabled) {
            if (!(currentIndex >= 0 && currentIndex < count && itemAt(currentIndex)?.enabled)) {
                currentIndex = -1
                for (let i = 0; i < count; ++i)
                    if (itemAt(i)?.enabled) { currentIndex = i; break }
            }
        }
        Qt.callLater(function() { revealFocusedOrSelected(); indicator.retarget(false) })
    }
    onModelChanged: Qt.callLater(reconcileSelection)
    onCountChanged: Qt.callLater(reconcileSelection)
    onCurrentIndexChanged: Qt.callLater(function() { reveal(currentIndex); indicator.retarget(true) })
    onTabWidthChanged: Qt.callLater(function() { revealFocusedOrSelected(); indicator.retarget(false) })
    onAnimatedChanged: if (!animated) indicator.retarget(false)
    Component.onCompleted: Qt.callLater(function() { reconcileSelection(); ready = true })

    contentItem: Item {
        Rectangle {
            id: group
            objectName: "mobileNavigationGroup"
            x: 0 // Physical left, including RTL locales. Optional Search owns the physical right.
            width: control.groupWidth
            height: Theme.mobileNavigationHeight
            visible: control.count > 0
            radius: control.resolvedCornerRadius
            color: Theme.panelBackground03
            border.color: Theme.panelBackground08
            border.width: 1
            antialiasing: true

            Flickable {
                id: viewport
                objectName: "mobileNavigationViewport"
                x: Theme.gap4; y: Theme.gap4
                width: Math.max(0, group.width - Theme.gap4 * 2)
                height: Theme.mobileNavigationTouchSize
                contentWidth: row.width
                contentHeight: height
                flickableDirection: Flickable.HorizontalFlick
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentWidth > width
                clip: true
                onWidthChanged: Qt.callLater(control.revealFocusedOrSelected)
                onContentWidthChanged: Qt.callLater(control.revealFocusedOrSelected)

                Rectangle {
                    id: indicator
                    objectName: "mobileNavigationIndicator"
                    property real leftEdge: 0
                    property real rightEdge: control.tabWidth
                    property real stretchLeft: 0
                    property real stretchRight: 0
                    property real targetLeft: 0
                    property real targetRight: control.tabWidth
                    property real surfaceHeight: Theme.mobileNavigationTouchSize
                    property bool initialized: false
                    x: Math.min(leftEdge, rightEdge)
                    y: (viewport.height - height) / 2
                    width: Math.abs(rightEdge - leftEdge)
                    height: surfaceHeight
                    radius: Math.max(0, Math.min(height / 2, control.resolvedCornerRadius - Theme.gap4))
                    color: Theme.panelBackground10
                    visible: control.selectionVisible
                    antialiasing: true

                    function retarget(animate) {
                        travel.stop()
                        targetLeft = Math.max(0, control.currentIndex) * control.tabWidth
                        targetRight = targetLeft + control.tabWidth
                        if (!animate || !initialized || !control.ready || !control.animated || !control.selectionVisible) {
                            leftEdge = targetLeft; rightEdge = targetRight
                            surfaceHeight = Theme.mobileNavigationTouchSize
                        } else if (Math.abs(targetLeft - leftEdge) > 0.1 || Math.abs(targetRight - rightEdge) > 0.1) {
                            const forward = targetLeft + targetRight >= leftEdge + rightEdge
                            stretchLeft = leftEdge + (targetLeft - leftEdge) * (forward ? 0.15 : 0.88)
                            stretchRight = rightEdge + (targetRight - rightEdge) * (forward ? 0.88 : 0.15)
                            travel.start()
                        }
                        initialized = control.selectionVisible
                    }
                    SequentialAnimation {
                        id: travel
                        ParallelAnimation {
                            NumberAnimation { target: indicator; property: "leftEdge"; to: indicator.stretchLeft; duration: Motion.duration(Theme.mobileNavigationStretchDuration); easing.type: Easing.OutCubic }
                            NumberAnimation { target: indicator; property: "rightEdge"; to: indicator.stretchRight; duration: Motion.duration(Theme.mobileNavigationStretchDuration); easing.type: Easing.OutCubic }
                            NumberAnimation { target: indicator; property: "surfaceHeight"; to: Theme.mobileNavigationTouchSize - 6; duration: Motion.duration(Theme.mobileNavigationStretchDuration); easing.type: Easing.OutCubic }
                        }
                        ParallelAnimation {
                            NumberAnimation { target: indicator; property: "leftEdge"; to: indicator.targetLeft; duration: Motion.duration(Motion.surfaceDuration - Theme.mobileNavigationStretchDuration); easing.type: Easing.OutBack; easing.overshoot: 1.1 }
                            NumberAnimation { target: indicator; property: "rightEdge"; to: indicator.targetRight; duration: Motion.duration(Motion.surfaceDuration - Theme.mobileNavigationStretchDuration); easing.type: Easing.OutBack; easing.overshoot: 1.1 }
                            NumberAnimation { target: indicator; property: "surfaceHeight"; to: Theme.mobileNavigationTouchSize; duration: Motion.duration(Motion.surfaceDuration - Theme.mobileNavigationStretchDuration); easing.type: Easing.OutBack; easing.overshoot: 1.1 }
                        }
                    }
                }
                Row {
                    id: row
                    height: viewport.height
                    layoutDirection: Qt.LeftToRight
                    Repeater {
                        id: tabs
                        model: control.model
                        MobileNavigationTab {
                            required property int index
                            required property var modelData
                            readonly property var entry: modelData || ({})
                            objectName: entry.objectName || control.itemObjectNamePrefix + index
                            labelObjectName: entry.labelObjectName || objectName + "Label"
                            text: typeof modelData === "string" ? modelData : (entry.text || "")
                            Accessible.name: entry.accessibleName || text || qsTr("Tab %1").arg(index + 1)
                            iconName: entry.iconName || ""
                            iconSource: entry.iconSource || Theme.iconPath(iconName)
                            enabled: entry.enabled !== false
                            selected: index === control.currentIndex
                            drawSelection: false
                            cornerRadius: Math.max(0, control.resolvedCornerRadius - Theme.gap4)
                            width: control.tabWidth
                            height: viewport.height
                            motionEnabled: control.motionEnabled
                            navigationBar: control
                            tabIndex: index
                            LayoutMirroring.enabled: false
                            onClicked: control.activate(index)
                        }
                    }
                }
            }
        }
        MobileNavigationTab {
            id: searchButton
            objectName: "mobileNavigationSearch"
            visible: control.searchVisible
            x: parent.width - width
            width: Theme.mobileNavigationHeight
            height: Theme.mobileNavigationHeight
            text: control.search?.text || ""
            iconName: control.search?.iconName || "inputFieldSearch"
            iconSource: control.search?.iconSource || Theme.iconPath(iconName)
            enabled: control.searchVisible && control.search?.enabled !== false
            Accessible.role: Accessible.Button
            Accessible.ignored: !control.searchVisible
            Accessible.name: control.search?.accessibleName || text || qsTr("Search")
            Accessible.selectable: false
            Accessible.selected: false
            cornerRadius: control.resolvedCornerRadius
            drawSelection: false
            navigationBar: control
            tabIndex: control.count
            motionEnabled: control.motionEnabled
            LayoutMirroring.enabled: false
            onClicked: if (control.enabled && enabled) control.searchRequested()
            background: Rectangle {
                radius: control.resolvedCornerRadius
                color: searchButton.down ? Theme.panelBackground10 : searchButton.hovered ? Theme.panelBackground06 : Theme.panelBackground03
                border.color: Theme.panelBackground08
                border.width: 1
                antialiasing: true
            }
        }
    }
}

// LV.MobileNavigationBar { model: [{iconName: "home-1", accessibleName: "Home"}]; search: {accessibleName: "Search"}; onSearchRequested: searchPanel.open() }
