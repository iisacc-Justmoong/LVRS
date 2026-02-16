import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Window
import LVRS 1.0

Controls.ApplicationWindow {
    id: root

    // Platform + size-class signals to mimic media-query style rules.
    readonly property string platform: Qt.platform.os
    readonly property bool isMobilePlatform: platform === "android" || platform === "ios"
    readonly property bool isDesktopPlatform: platform === "osx" || platform === "windows" || platform === "linux"

    readonly property int compact: 0
    readonly property int medium: 1
    readonly property int expanded: 2

    readonly property int widthClass: width < 600 ? compact : (width < 1000 ? medium : expanded)
    readonly property int heightClass: height < 600 ? compact : (height < 900 ? medium : expanded)

    readonly property bool isCompact: widthClass === compact || heightClass === compact
    readonly property bool isExpanded: widthClass === expanded && heightClass === expanded


    property int desktopMinWidth: 900
    property int desktopMinHeight: 600
    property int mobileMinWidth: 360
    property int mobileMinHeight: 640
    // Keep view composition identical across platforms; only apply when explicitly enabled.
    property bool usePlatformSafeMargin: false
    property int safeMargin: usePlatformSafeMargin && isMobilePlatform ? 12 : 0
    property color windowColor: Theme.window
    property bool forceNativeDarkTitleBar: Theme.dark
    property bool autoAttachRuntimeEvents: false
    property bool autoHookBackendUserEvents: false
    property bool globalEventListenersEnabled: true

    property string subtitle: ""
    property var navItems: ["Overview", "Suites", "Runs", "Devices", "Reports", "Settings"]

    property alias navIndex: scaffold.navIndex
    property alias navigationEnabled: scaffold.navigationEnabled
    property alias navTitle: scaffold.navTitle
    property alias navTitleVisible: scaffold.navTitleVisible
    property alias navWidth: scaffold.navWidth
    property alias navDrawerWidth: scaffold.navDrawerWidth
    property alias wideBreakpoint: scaffold.wideBreakpoint
    property alias scaffoldLayoutMode: scaffold.layoutMode
    property alias scaffoldLayoutPlatform: scaffold.layoutPlatform
    property alias scaffoldForceDesktopOnLargeMobile: scaffold.forceDesktopOnLargeMobile
    property alias scaffoldMobileDesktopMinWidth: scaffold.mobileDesktopMinWidth
    property alias scaffoldPreferBottomNavigation: scaffold.preferBottomNavigation
    property alias scaffoldBottomNavigationMaxItems: scaffold.bottomNavigationMaxItems
    property alias scaffoldCompactSpacingEnabled: scaffold.compactSpacingEnabled
    property alias scaffoldCompactSpacingBreakpoint: scaffold.compactSpacingBreakpoint
    property alias scaffoldNavRailMaxWidthRatio: scaffold.navRailMaxWidthRatio
    property alias scaffoldDrawerMarginSafety: scaffold.drawerMarginSafety
    property alias navDelegate: scaffold.navDelegate
    property alias navHeader: scaffold.navHeader
    property alias navFooter: scaffold.navFooter
    property alias pageRouter: scaffold.pageRouter
    property alias pageRoutes: scaffold.routes
    property alias pageInitialPath: scaffold.initialPath
    property alias useInternalPageStack: scaffold.useInternalPageStack
    property alias internalRouterRegisterAsGlobalNavigator: scaffold.internalRouterRegisterAsGlobalNavigator
    readonly property bool internalPageStackEnabled: scaffold.internalPageStackEnabled
    readonly property var activePageRouter: scaffold.activePageRouter
    readonly property string adaptiveLayoutProfile: scaffold.layoutProfile
    readonly property string adaptiveNavigationMode: scaffold.navigationMode
    property alias headerActions: scaffold.headerActions
    default property alias content: scaffold.content
    readonly property bool adaptiveMobileLayout: scaffold.mobileLayout
    readonly property bool adaptiveDesktopLayout: scaffold.desktopLayout
    readonly property bool adaptiveRailNavigation: scaffold.navigationRailEnabled
    readonly property bool adaptiveDrawerNavigation: scaffold.drawerNavigationEnabled
    readonly property bool adaptiveBottomNavigation: scaffold.bottomNavigationEnabled
    property var lastGlobalPressedEventData: ({})
    property var lastGlobalContextEventData: ({})

    signal navActivated(int index, var item)
    signal globalPressedEvent(var eventData)
    signal globalContextEvent(var eventData)
    signal adaptiveLayoutStateChanged(string profile, string navigationMode)
    signal pageStackNavigated(string path, var params)
    signal pageStackNavigationFailed(string path)

    minimumWidth: isMobilePlatform ? mobileMinWidth : desktopMinWidth
    minimumHeight: isMobilePlatform ? mobileMinHeight : desktopMinHeight
    color: root.windowColor

    function matchesMedia(rule) {
        if (!rule)
            return true
        var token = String(rule).toLowerCase()
        if (token === "mobile")
            return isMobilePlatform
        if (token === "desktop")
            return isDesktopPlatform
        if (token === "compact")
            return isCompact
        if (token === "expanded")
            return isExpanded
        if (token === "medium")
            return widthClass === medium || heightClass === medium
        if (token === "mobile-layout")
            return adaptiveMobileLayout
        if (token === "desktop-layout")
            return adaptiveDesktopLayout
        if (token === "rail-nav")
            return adaptiveRailNavigation
        if (token === "drawer-nav")
            return adaptiveDrawerNavigation
        if (token === "bottom-nav")
            return adaptiveBottomNavigation
        if (token === "stack-enabled")
            return internalPageStackEnabled
        return false
    }

    function applyNativeWindowStyle() {
        if (!NativeWindowStyle.titleBarColorSupported)
            return false
        return NativeWindowStyle.applyTitleBarColor(root, root.windowColor, root.forceNativeDarkTitleBar)
    }

    onVisibleChanged: {
        if (visible)
            applyNativeWindowStyle()
    }
    onWindowColorChanged: applyNativeWindowStyle()
    onForceNativeDarkTitleBarChanged: applyNativeWindowStyle()
    onAutoAttachRuntimeEventsChanged: {
        if (!autoAttachRuntimeEvents)
            return
        RuntimeEvents.start()
        RuntimeEvents.attachWindow(root)
    }
    onAutoHookBackendUserEventsChanged: {
        if (!autoHookBackendUserEvents)
            return
        if (!Backend.hookUserEvents())
            Debug.warn("ApplicationWindow", "io-event-hook-failed", Backend.lastError)
    }

    readonly property real effectiveSupersampleScale: RenderQuality.enabled
        ? Math.max(RenderQuality.minimumSupersampleScale,
                   Math.min(RenderQuality.maximumSupersampleScale, RenderQuality.supersampleScale))
        : 1.0

    Item {
        id: supersampleHost
        anchors.fill: parent
        anchors.margins: root.safeMargin
        layer.enabled: RenderQuality.enabled && root.effectiveSupersampleScale > 1.0
        layer.smooth: layer.enabled
        layer.mipmap: layer.enabled
        layer.textureSize: layer.enabled
            ? Qt.size(
                  Math.max(1, Math.round(width * root.effectiveSupersampleScale)),
                  Math.max(1, Math.round(height * root.effectiveSupersampleScale)))
            : Qt.size(
                  Math.max(1, Math.round(width)),
                  Math.max(1, Math.round(height)))

        AppScaffold {
            id: scaffold
            anchors.fill: parent
            headerTitle: root.title
            headerSubtitle: root.subtitle
            navModel: root.navItems
            layoutPlatform: root.platform
            onLayoutStateChanged: function(profile, navigationMode) { root.adaptiveLayoutStateChanged(profile, navigationMode) }
            onStackNavigated: function(path, params) { root.pageStackNavigated(path, params) }
            onStackNavigationFailed: function(path) { root.pageStackNavigationFailed(path) }
            onNavActivated: function(index, item) { root.navActivated(index, item) }
        }
    }

    EventListener {
        id: globalPressedListener
        anchors.fill: parent
        enabled: root.globalEventListenersEnabled
        trigger: "globalPressed"
        action: function(eventData) {
            root.lastGlobalPressedEventData = eventData || ({})
            root.globalPressedEvent(eventData)
        }
    }

    EventListener {
        id: globalContextListener
        anchors.fill: parent
        enabled: root.globalEventListenersEnabled
        trigger: "globalContextRequested"
        action: function(eventData) {
            root.lastGlobalContextEventData = eventData || ({})
            root.globalContextEvent(eventData)
        }
    }

    QtObject {
        Component.onCompleted: {
            FontPolicy.enforceApplicationFallback()
            RenderQuality.applyWindow(root)
            if (SvgManager.minimumScale < root.effectiveSupersampleScale)
                SvgManager.minimumScale = root.effectiveSupersampleScale
            if (root.autoAttachRuntimeEvents) {
                RuntimeEvents.start()
                RuntimeEvents.attachWindow(root)
            }
            if (root.autoHookBackendUserEvents) {
                if (!Backend.hookUserEvents())
                    Debug.warn("ApplicationWindow", "io-event-hook-failed", Backend.lastError)
                else
                    Debug.log("ApplicationWindow", "io-event-hooked", Backend.hookedEventCount)
            }
            Debug.log("ApplicationWindow", "supersample-scale", root.effectiveSupersampleScale)
            root.applyNativeWindowStyle()
            Qt.callLater(root.applyNativeWindowStyle)
        }
    }

}

// API usage (external):
// import LVRS as LV
// LV.ApplicationWindow { title: "App" }
