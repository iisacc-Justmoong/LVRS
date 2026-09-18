pragma ComponentBehavior: Bound
import QtQuick
import LVRS 1.0

TabBar {
    id: control
    enum Presentation { Expanded, Search, Minimized }
    property int platformStyle: MobileTab.Automatic
    property int presentation: MobileTabBar.Expanded
    property real bottomSafeInset: 0
    property Item backdropSource: null
    readonly property int resolvedPlatformStyle: platformStyle === MobileTab.Automatic
        ? (Theme.effectiveTarget === "android" ? MobileTab.Android : MobileTab.IOS) : platformStyle
    readonly property bool iosStyle: resolvedPlatformStyle === MobileTab.IOS
    readonly property bool minimized: iosStyle && presentation === MobileTabBar.Minimized && currentIndex >= 0
    readonly property bool separateSearch: iosStyle && presentation === MobileTabBar.Search && count > 1
    readonly property real searchSpan: separateSearch ? 68 : 0
    signal expandRequested()
    itemSpacing: 0
    widthPolicy: TabBar.Equal
    leftPadding: iosStyle ? 22 : 0
    rightPadding: leftPadding
    topPadding: iosStyle ? 16 : 0
    bottomPadding: Math.max(0, bottomSafeInset)
    implicitWidth: iosStyle ? 402 : 412
    implicitHeight: (iosStyle ? 70 : 64) + bottomPadding
    Accessible.name: qsTr("Main navigation")

    delegate: MobileTab {
        required property int index
        required property var modelData
        readonly property bool searchItem: control.separateSearch && index === control.count - 1
        objectName: modelData.objectName || control.itemObjectNamePrefix + index
        labelObjectName: modelData.labelObjectName || objectName + "Label"
        text: typeof modelData === "string" ? modelData : (modelData.text || "")
        Accessible.name: modelData.accessibleName || text
        iconName: modelData.iconName || ""
        badge: modelData.badge || ""
        badgeDot: modelData.badgeDot === true
        enabled: modelData.enabled !== false
        selected: index === control.currentIndex
        platformStyle: control.resolvedPlatformStyle
        motionEnabled: control.motionEnabled
        leadingInset: searchItem ? 12 : 0
        showLabel: !control.minimized && !searchItem
        visible: !control.minimized || selected
        width: control.minimized ? 48 : searchItem ? control.searchSpan
            : Math.max(0, (control.availableWidth - control.searchSpan) / Math.max(1, control.count - (control.separateSearch ? 1 : 0)))
        height: control.availableHeight
        navigationBar: control
        tabIndex: index
        onClicked: {
            if (control.minimized) control.expandRequested()
            else control.activate(index)
        }
    }
    background: Item {
        Rectangle {
            anchors.fill: parent
            visible: !control.iosStyle
            color: Theme.mobileTabAndroidSurface
        }
        MaterialSurface {
            objectName: "mobileTabGlass"
            visible: control.iosStyle
            x: control.mirrored && control.separateSearch ? control.leftPadding + control.searchSpan : control.leftPadding
            y: control.topPadding
            width: control.minimized ? 48 : Math.max(0, control.availableWidth - control.searchSpan)
            height: control.availableHeight
            radius: height / 2
            color: Theme.panelBackground01
            tintOpacity: 0.75
            primaryColor: "transparent"
            intenseOpacity: 0
            faintOpacity: 0
            borderColor: Qt.rgba(1, 1, 1, 0.2)
            backdropSource: control.backdropSource
            motionEnabled: control.motionEnabled
        }
        MaterialSurface {
            visible: control.separateSearch
            x: control.mirrored ? control.leftPadding : control.width - control.rightPadding - 56
            y: control.topPadding
            width: 56
            height: control.availableHeight
            radius: height / 2
            color: Theme.panelBackground01
            tintOpacity: 0.75
            primaryColor: "transparent"
            intenseOpacity: 0
            faintOpacity: 0
            borderColor: Qt.rgba(1, 1, 1, 0.2)
            backdropSource: control.backdropSource
            motionEnabled: control.motionEnabled
        }
    }
}

// LV.MobileTabBar { model: destinations; currentIndex: view.index; autoSelect: false; onActivated: index => view.index = index }
