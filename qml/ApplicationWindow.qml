import QtQuick
import QtQuick.Controls
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtQuick.Window
import LVRS 1.0

Controls.ApplicationWindow {
    id: windowRoot

    // Platform + size-class signals to mimic media-query style rules.
    readonly property string platform: Qt.platform.os
    readonly property bool isMobilePlatform: platform === "android" || platform === "ios"
    readonly property bool isDesktopPlatform: platform === "osx" || platform === "windows" || platform === "linux"
    readonly property bool backendMobilePlatform: Platform.mobile
    readonly property Item overlayLayer: Controls.Overlay.overlay

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
    property bool useBackendMobileScale: true
    property real mobileViewScale: 1.5
    readonly property real effectiveMobileViewScale: useBackendMobileScale && backendMobilePlatform
        ? Math.max(1.0, mobileViewScale)
        : 1.0
    property bool forceFullWindowAreaOnMobile: true
    readonly property bool fullWindowAreaOnMobileEnabled: forceFullWindowAreaOnMobile && backendMobilePlatform
    // Keep view composition identical across platforms; only apply when explicitly enabled.
    property bool usePlatformSafeMargin: false
    property int safeMargin: usePlatformSafeMargin && isMobilePlatform ? 12 : 0
    property color windowColor: Theme.window
    property bool forceNativeDarkTitleBar: Theme.dark
    property bool solidChrome: true
    // Keep release defaults conservative: opt in when global listeners are required.
    property bool globalEventListenersEnabled: false
    // Runtime daemon attach remains opt-in via this property or global listener enablement.
    property bool autoAttachRuntimeEvents: globalEventListenersEnabled
    // Backend mirrored event cache is opt-in because duplicate buffering can add overhead.
    property bool autoHookBackendUserEvents: false
    property bool windowDragHandleEnabled: isDesktopPlatform
    property int windowDragHandleHeight: 28
    property int windowDragHandleTopMargin: 0
    property bool inactiveRenderDowngradeEnabled: true
    property int inactiveRenderMsaaSamples: 0
    property bool autoApplyDeviceTierPreset: true
    property int forcedDeviceTierPreset: -1
    property int pageRouterRetainInactivePages: 0
    property int pageRouterCacheCapacity: 256

    property string subtitle: ""
    property var navItems: []

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
    default property alias content: scaffold.content
    readonly property bool adaptiveMobileLayout: scaffold.mobileLayout
    readonly property bool adaptiveDesktopLayout: scaffold.desktopLayout
    readonly property bool adaptiveRailNavigation: scaffold.navigationRailEnabled
    readonly property bool adaptiveDrawerNavigation: scaffold.drawerNavigationEnabled
    readonly property bool adaptiveBottomNavigation: scaffold.bottomNavigationEnabled
    property var lastGlobalPressedEventData: ({})
    property var lastGlobalContextEventData: ({})
    property bool useBackendAdaptivePolicy: true
    property var backendAdaptivePolicyOverrides: ({})
    readonly property var backendRuntimeProfile: Platform.runtimeProfile(platform)
    readonly property var backendAdaptivePolicyDefaults: resolveBackendAdaptivePolicy(backendRuntimeProfile)
    readonly property var backendAdaptivePolicy: useBackendAdaptivePolicy
        ? mergePolicyMaps(backendAdaptivePolicyDefaults, backendAdaptivePolicyOverrides)
        : ({})
    readonly property int backendWideBreakpoint: backendAdaptiveNumber("wideBreakpoint", 980, 600)
    readonly property int backendNavWidth: backendAdaptiveNumber("navWidth", 220, 140)
    readonly property int backendNavDrawerWidth: backendAdaptiveNumber("navDrawerWidth", 240, 160)
    readonly property int backendMobileDesktopMinWidth: backendAdaptiveNumber("mobileDesktopMinWidth", 1200, 800)
    readonly property int backendBottomNavigationMaxItems: backendAdaptiveNumber("bottomNavigationMaxItems", 5, 1)
    readonly property int backendCompactSpacingBreakpoint: backendAdaptiveNumber("compactSpacingBreakpoint", 900, 480)
    readonly property real backendNavRailMaxWidthRatio: backendAdaptiveNumber("navRailMaxWidthRatio", 0.32, 0.10)
    readonly property int backendDrawerMarginSafety: backendAdaptiveNumber("drawerMarginSafety", Theme.gap16, 0)
    readonly property int backendDrawerEnterDuration: backendAdaptiveNumber("drawerEnterDuration", 170, 0)
    readonly property int backendDrawerExitDuration: backendAdaptiveNumber("drawerExitDuration", 130, 0)
    readonly property bool backendAnimatedTransitions: backendAdaptiveBool("enableAnimatedTransitions", true)

    signal navActivated(int index, var item)
    signal globalPressedEvent(var eventData)
    signal globalContextEvent(var eventData)
    signal adaptiveLayoutStateChanged(string profile, string navigationMode)
    signal pageStackNavigated(string path, var params)
    signal pageStackNavigationFailed(string path)

    minimumWidth: isMobilePlatform ? mobileMinWidth : desktopMinWidth
    minimumHeight: isMobilePlatform ? mobileMinHeight : desktopMinHeight
    color: windowRoot.windowColor

    background: Rectangle {
        x: 0
        y: 0
        width: windowRoot.width
        height: windowRoot.height
        color: windowRoot.windowColor
    }

    Binding {
        target: windowRoot
        property: "topPadding"
        value: 0
        when: windowRoot.fullWindowAreaOnMobileEnabled
    }

    Binding {
        target: windowRoot
        property: "rightPadding"
        value: 0
        when: windowRoot.fullWindowAreaOnMobileEnabled
    }

    Binding {
        target: windowRoot
        property: "bottomPadding"
        value: 0
        when: windowRoot.fullWindowAreaOnMobileEnabled
    }

    Binding {
        target: windowRoot
        property: "leftPadding"
        value: 0
        when: windowRoot.fullWindowAreaOnMobileEnabled
    }

    function mergePolicyMaps(basePolicy, overridePolicy) {
        const merged = ({})
        if (basePolicy && typeof basePolicy === "object") {
            for (const key in basePolicy)
                merged[key] = basePolicy[key]
        }
        if (overridePolicy && typeof overridePolicy === "object") {
            for (const overrideKey in overridePolicy) {
                if (overridePolicy[overrideKey] !== undefined)
                    merged[overrideKey] = overridePolicy[overrideKey]
            }
        }
        return merged
    }

    function backendAdaptiveNumber(key, fallback, minimum) {
        if (!useBackendAdaptivePolicy)
            return fallback
        const value = backendAdaptivePolicy && backendAdaptivePolicy[key] !== undefined
            ? Number(backendAdaptivePolicy[key])
            : NaN
        if (!isFinite(value))
            return fallback
        if (minimum !== undefined)
            return Math.max(minimum, value)
        return value
    }

    function backendAdaptiveBool(key, fallback) {
        if (!useBackendAdaptivePolicy)
            return fallback
        if (!backendAdaptivePolicy || backendAdaptivePolicy[key] === undefined)
            return fallback
        return !!backendAdaptivePolicy[key]
    }

    function resolveBackendAdaptivePolicy(profile) {
        const safeProfile = profile && typeof profile === "object" ? profile : ({})
        const mobile = safeProfile.mobile === true
        const backendReady = safeProfile.backendFeatureReady !== false
        const knownTarget = safeProfile.known === true
        const backendName = safeProfile.backend !== undefined && safeProfile.backend !== null
            ? String(safeProfile.backend).toLowerCase()
            : "default"
        const gpuAccelerated = backendName === "metal" || backendName === "vulkan"
        const transitionScale = backendReady && gpuAccelerated ? 1.0 : 0.85

        return {
            wideBreakpoint: mobile ? 940 : 980,
            navWidth: mobile ? 208 : 220,
            navDrawerWidth: mobile ? 252 : 240,
            mobileDesktopMinWidth: mobile ? 1080 : 1200,
            bottomNavigationMaxItems: mobile ? 4 : 5,
            compactSpacingBreakpoint: mobile ? 840 : 900,
            navRailMaxWidthRatio: mobile ? 0.34 : 0.32,
            drawerMarginSafety: mobile ? Theme.gap20 : Theme.gap16,
            drawerEnterDuration: Math.round(170 * transitionScale),
            drawerExitDuration: Math.round(130 * transitionScale),
            enableAnimatedTransitions: backendReady && knownTarget
        }
    }

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
        if (windowRoot.solidChrome && NativeWindowStyle.solidChromeSupported)
            return NativeWindowStyle.applySolidChrome(windowRoot, windowRoot.windowColor, windowRoot.forceNativeDarkTitleBar)
        if (!NativeWindowStyle.titleBarColorSupported)
            return false
        return NativeWindowStyle.applyTitleBarColor(windowRoot, windowRoot.windowColor, windowRoot.forceNativeDarkTitleBar)
    }

    function requestWindowMove() {
        if (typeof windowRoot.startSystemMove === "function")
            return !!windowRoot.startSystemMove()
        return false
    }

    function ensureRuntimeEventsAttached() {
        RuntimeEvents.start()
        RuntimeEvents.attachWindow(windowRoot)
    }

    onVisibleChanged: {
        RenderQuality.applyWindow(windowRoot)
        if (visible)
            applyNativeWindowStyle()
    }
    onWindowColorChanged: applyNativeWindowStyle()
    onForceNativeDarkTitleBarChanged: applyNativeWindowStyle()
    onSolidChromeChanged: applyNativeWindowStyle()
    onInactiveRenderDowngradeEnabledChanged: {
        RenderQuality.inactiveRenderDowngradeEnabled = inactiveRenderDowngradeEnabled
        RenderQuality.applyWindow(windowRoot)
    }
    onInactiveRenderMsaaSamplesChanged: {
        RenderQuality.inactiveMsaaSamples = inactiveRenderMsaaSamples
        RenderQuality.applyWindow(windowRoot)
    }
    onAutoApplyDeviceTierPresetChanged: {
        if (!autoApplyDeviceTierPreset)
            return
        RenderQuality.applyDeviceTierPreset(forcedDeviceTierPreset)
        RenderQuality.applyWindow(windowRoot)
    }
    onForcedDeviceTierPresetChanged: {
        if (!autoApplyDeviceTierPreset)
            return
        RenderQuality.applyDeviceTierPreset(forcedDeviceTierPreset)
        RenderQuality.applyWindow(windowRoot)
    }
    onAutoAttachRuntimeEventsChanged: {
        if (!autoAttachRuntimeEvents)
            return
        windowRoot.ensureRuntimeEventsAttached()
    }
    onAutoHookBackendUserEventsChanged: {
        if (!autoHookBackendUserEvents)
            return
        if (!Backend.hookUserEvents())
            Debug.warn("ApplicationWindow", "io-event-hook-failed", Backend.lastError)
    }

    readonly property real effectiveSupersampleScale: RenderQuality.enabled
        ? RenderQuality.effectiveSupersampleScaleValue
        : 1.0
    readonly property bool sceneSupersamplingActive: RenderQuality.sceneSupersamplingActive

    component AdaptiveLayoutHost: Item {
            id: root
        
            property var navModel: ["Overview", "Suites", "Runs", "Devices", "Reports", "Settings"]
            property int navIndex: 0
            property bool navigationEnabled: true
            property string navTitle: "Navigation"
            property bool navTitleVisible: true
            property int navWidth: windowRoot.backendNavWidth
            property int navDrawerWidth: windowRoot.backendNavDrawerWidth
            property int wideBreakpoint: windowRoot.backendWideBreakpoint
            property string layoutMode: "auto" // auto, mobile, desktop
            property string layoutPlatform: Qt.platform.os
            property bool forceDesktopOnLargeMobile: false
            property int mobileDesktopMinWidth: windowRoot.backendMobileDesktopMinWidth
            property bool preferBottomNavigation: true
            property int bottomNavigationMaxItems: windowRoot.backendBottomNavigationMaxItems
            property bool compactSpacingEnabled: true
            property int compactSpacingBreakpoint: windowRoot.backendCompactSpacingBreakpoint
            property real navRailMaxWidthRatio: windowRoot.backendNavRailMaxWidthRatio
            property int drawerMarginSafety: windowRoot.backendDrawerMarginSafety
            property int drawerEnterDuration: windowRoot.backendDrawerEnterDuration
            property int drawerExitDuration: windowRoot.backendDrawerExitDuration
            property bool animatedTransitions: windowRoot.backendAnimatedTransitions
            property Component navDelegate: null
            property Component navHeader: null
            property Component navFooter: null
            property var pageRouter: null
            property var routes: []
            property string initialPath: "/"
            property bool useInternalPageStack: true
            property bool internalRouterRegisterAsGlobalNavigator: false
        
            signal navActivated(int index, var item)
            signal layoutStateChanged(string profile, string navigationMode)
            signal stackNavigated(string path, var params)
            signal stackNavigationFailed(string path)
            signal transitionRejected(string kind, string fromState, string toState, string fallbackState)
        
            default property alias content: contentArea.data
        
            readonly property bool wide: width >= wideBreakpoint
            readonly property bool hasNav: navigationEnabled && root.navModelCount() > 0
            readonly property string normalizedLayoutMode: root.normalizeLayoutMode(layoutMode)
            readonly property bool platformMobile: root.isMobilePlatform(layoutPlatform)
            readonly property bool requestedMobileLayout: root.normalizedLayoutMode === "mobile"
                || (root.normalizedLayoutMode === "auto"
                    && root.platformMobile
                    && (!root.forceDesktopOnLargeMobile || root.width < root.mobileDesktopMinWidth))
            readonly property bool requestedDesktopLayout: !root.requestedMobileLayout
            readonly property string requestedLayoutProfile: root.requestedMobileLayout
                ? (root.wide ? "mobile-wide" : "mobile-compact")
                : (root.wide ? "desktop-wide" : "desktop-compact")
            readonly property string requestedNavigationMode: root.requestedNavigationModeForProfile(root.requestedLayoutProfile)
            property string layoutProfile: root.requestedLayoutProfile
            property string navigationMode: root.requestedNavigationMode
            readonly property bool mobileLayout: root.layoutProfile.indexOf("mobile-") === 0
            readonly property bool desktopLayout: !root.mobileLayout
            readonly property bool navigationRailEnabled: root.hasNav && root.navigationMode === "rail"
            readonly property bool bottomNavigationEnabled: root.hasNav && root.navigationMode === "bottom"
            readonly property bool drawerNavigationEnabled: root.hasNav && root.navigationMode === "drawer"
            property bool _adaptiveUpdateInProgress: false
            property string lastRejectedLayoutTransition: ""
            property string lastRejectedNavigationTransition: ""
            readonly property bool compactSpacing: root.compactSpacingEnabled
                && (root.mobileLayout || root.width < root.compactSpacingBreakpoint)
            // Keep scaffold content full-bleed by default instead of forcing horizontal narrowing.
            readonly property int adaptiveOuterMargin: 0
            readonly property int adaptiveNavigationInset: root.compactSpacing ? Theme.gap12 : Theme.gap16
            readonly property int adaptiveContentInset: 0
            readonly property int adaptiveBottomInset: root.compactSpacing ? Theme.gap6 : Theme.gap8
            readonly property int effectiveNavRailWidth: root.resolveNavRailWidth()
            readonly property int effectiveNavDrawerWidth: root.resolveDrawerWidth()
            readonly property var effectiveRoutes: root.collectEffectiveRoutes()
            readonly property bool internalPageStackEnabled: root.useInternalPageStack && root.effectiveRoutes.length > 0
            readonly property var activePageRouter: root.resolveRouter()
        
            implicitWidth: 1200
            implicitHeight: 760
        
            function routeForItem(item) {
                if (item && typeof item === "object")
                    return item.path || item.route || ""
                return ""
            }
        
            function paramsForItem(item) {
                if (item && typeof item === "object" && item.params !== undefined)
                    return item.params
                return ({})
            }
        
            function resolveRouter() {
                if (pageRouter)
                    return pageRouter
                if (internalPageStackEnabled)
                    return internalPageRouter
                if (typeof Navigator !== "undefined" && Navigator && Navigator.router)
                    return Navigator.router
                return null
            }
        
            function navigateTo(path, params) {
                var targetRouter = resolveRouter()
                if (!targetRouter || !path)
                    return false
                targetRouter.go(path, params !== undefined ? params : ({}))
                return true
            }
        
            function normalizedPath(value) {
                var token = String(value || "").trim()
                if (!token)
                    return ""
                if (!token.startsWith("/"))
                    token = "/" + token
                if (token.length > 1 && token.endsWith("/"))
                    token = token.slice(0, -1)
                return token
            }
        
            function itemAt(index) {
                if (!navModel)
                    return null
                if (typeof navModel.length === "number")
                    return navModel[index]
                if (typeof navModel.get === "function")
                    return navModel.get(index)
                return null
            }
        
            function navModelCount() {
                if (!navModel)
                    return 0
                if (typeof navModel.length === "number")
                    return navModel.length
                if (typeof navModel.count === "number")
                    return navModel.count
                return 0
            }
        
            function normalizeLayoutMode(value) {
                var token = String(value || "").trim().toLowerCase()
                if (token === "mobile" || token === "desktop" || token === "auto")
                    return token
                return "auto"
            }
        
            function isMobilePlatform(value) {
                var token = String(value || "").trim().toLowerCase()
                return token === "android" || token === "ios"
            }
        
            function resolveNavRailWidth() {
                if (!root.navigationRailEnabled)
                    return 0
        
                var ratio = Number(root.navRailMaxWidthRatio)
                if (!isFinite(ratio) || ratio <= 0)
                    ratio = 0.32
        
                var ratioWidth = Math.floor(root.width * ratio)
                var availableWidth = Math.max(140, Math.floor(root.width - (root.adaptiveOuterMargin * 2) - Theme.gap24))
                var capped = Math.min(root.navWidth, ratioWidth, availableWidth)
                return Math.max(140, capped)
            }
        
            function resolveDrawerWidth() {
                var safety = Number(root.drawerMarginSafety)
                if (!isFinite(safety) || safety < 0)
                    safety = 0
        
                var availableWidth = Math.max(160, Math.floor(root.width - safety))
                var capped = Math.min(root.navDrawerWidth, availableWidth)
                return Math.max(160, capped)
            }
        
            function requestedNavigationModeForProfile(profile) {
                if (!root.hasNav)
                    return "none"
        
                var canUseBottomNavigation = root.preferBottomNavigation
                        && root.navModelCount() > 0
                        && root.navModelCount() <= root.bottomNavigationMaxItems
        
                if (profile === "desktop-wide")
                    return "rail"
                if (canUseBottomNavigation
                        && (profile.indexOf("mobile-") === 0 || profile === "desktop-compact")) {
                    return "bottom"
                }
                return "drawer"
            }
        
            function isAllowedLayoutTransition(fromState, toState) {
                if (!fromState || fromState === toState)
                    return true
        
                if (fromState === "mobile-compact")
                    return toState === "mobile-wide" || toState === "desktop-compact"
                if (fromState === "mobile-wide")
                    return toState === "mobile-compact" || toState === "desktop-wide"
                if (fromState === "desktop-compact")
                    return toState === "desktop-wide" || toState === "mobile-compact"
                if (fromState === "desktop-wide")
                    return toState === "desktop-compact" || toState === "mobile-wide"
                return true
            }
        
            function nextLayoutTransitionState(fromState, toState) {
                if (root.isAllowedLayoutTransition(fromState, toState))
                    return toState
        
                var fromParts = String(fromState).split("-")
                var toParts = String(toState).split("-")
                var fromFamily = fromParts.length > 0 ? fromParts[0] : ""
                var fromSize = fromParts.length > 1 ? fromParts[1] : ""
                var toFamily = toParts.length > 0 ? toParts[0] : ""
                var toSize = toParts.length > 1 ? toParts[1] : ""
        
                if (fromFamily && fromSize && toFamily && toSize && fromFamily !== toFamily && fromSize !== toSize)
                    return toFamily + "-" + fromSize
                return toState
            }
        
            function isAllowedNavigationTransition(fromState, toState) {
                if (!fromState || fromState === toState)
                    return true
                if (fromState === "none" || toState === "none")
                    return true
                if (fromState === "rail")
                    return toState === "drawer"
                if (fromState === "drawer")
                    return toState === "rail" || toState === "bottom"
                if (fromState === "bottom")
                    return toState === "drawer"
                return true
            }
        
            function nextNavigationTransitionState(fromState, toState) {
                if (root.isAllowedNavigationTransition(fromState, toState))
                    return toState
                if ((fromState === "rail" && toState === "bottom")
                        || (fromState === "bottom" && toState === "rail")) {
                    return "drawer"
                }
                return toState
            }
        
            function updateAdaptiveState() {
                if (root._adaptiveUpdateInProgress)
                    return
        
                root._adaptiveUpdateInProgress = true
                var needsFollowUp = false
        
                var currentLayoutProfile = root.layoutProfile
                var targetLayoutProfile = root.requestedLayoutProfile
                var nextLayoutProfile = root.nextLayoutTransitionState(currentLayoutProfile, targetLayoutProfile)
                if (nextLayoutProfile !== currentLayoutProfile)
                    root.layoutProfile = nextLayoutProfile
                if (nextLayoutProfile !== targetLayoutProfile) {
                    root.lastRejectedLayoutTransition = currentLayoutProfile + "->" + targetLayoutProfile
                    root.transitionRejected("layoutProfile", currentLayoutProfile, targetLayoutProfile, nextLayoutProfile)
                    needsFollowUp = true
                }
        
                var currentNavigationMode = root.navigationMode
                var targetNavigationMode = root.requestedNavigationModeForProfile(nextLayoutProfile)
                var nextNavigationMode = root.nextNavigationTransitionState(currentNavigationMode, targetNavigationMode)
                if (nextNavigationMode !== currentNavigationMode)
                    root.navigationMode = nextNavigationMode
                if (nextNavigationMode !== targetNavigationMode) {
                    root.lastRejectedNavigationTransition = currentNavigationMode + "->" + targetNavigationMode
                    root.transitionRejected("navigationMode", currentNavigationMode, targetNavigationMode, nextNavigationMode)
                    needsFollowUp = true
                }
        
                root._adaptiveUpdateInProgress = false
                if (needsFollowUp)
                    Qt.callLater(root.updateAdaptiveState)
            }
        
            function collectEffectiveRoutes() {
                if (routes) {
                    if (typeof routes.length === "number" && routes.length > 0)
                        return routes
                    if (typeof routes.count === "number" && routes.count > 0 && typeof routes.get === "function") {
                        var explicitRoutes = []
                        for (var r = 0; r < routes.count; r++)
                            explicitRoutes.push(routes.get(r))
                        if (explicitRoutes.length > 0)
                            return explicitRoutes
                    }
                }
        
                var derived = []
                var count = root.navModelCount()
                for (var i = 0; i < count; i++) {
                    var item = root.itemAt(i)
                    if (!item || typeof item !== "object")
                        continue
                    var path = root.normalizedPath(root.routeForItem(item))
                    if (!path)
                        continue
        
                    var hasComponent = item.component !== undefined && item.component !== null
                    var hasSource = item.source !== undefined && item.source !== null && String(item.source).trim().length > 0
                    if (!hasComponent && !hasSource)
                        continue
        
                    var route = { path: path }
                    if (hasComponent)
                        route.component = item.component
                    else
                        route.source = item.source
                    derived.push(route)
                }
                return derived
            }
        
            function syncNavIndexToCurrentPath() {
                var targetRouter = resolveRouter()
                if (!targetRouter || targetRouter.currentPath === undefined)
                    return
        
                var current = normalizedPath(targetRouter.currentPath)
                if (!current) {
                    if (navIndex !== -1)
                        navIndex = -1
                    return
                }
        
                var count = root.navModelCount()
                var matchedIndex = -1
                for (var i = 0; i < count; i++) {
                    var candidate = normalizedPath(routeForItem(itemAt(i)))
                    if (!candidate)
                        continue
                    if (current === candidate || (candidate !== "/" && current.startsWith(candidate + "/"))) {
                        matchedIndex = i
                        break
                    }
                }
        
                if (navIndex !== matchedIndex)
                    navIndex = matchedIndex
            }
        
            onNavigationRailEnabledChanged: {
                if (navigationRailEnabled && navDrawer.opened)
                    navDrawer.close()
            }
            onDrawerNavigationEnabledChanged: {
                if (!drawerNavigationEnabled && navDrawer.opened)
                    navDrawer.close()
            }
            onRequestedLayoutProfileChanged: root.updateAdaptiveState()
            onRequestedNavigationModeChanged: root.updateAdaptiveState()
            onNavModelChanged: {
                root.syncNavIndexToCurrentPath()
                root.updateAdaptiveState()
            }
            onPageRouterChanged: Qt.callLater(syncNavIndexToCurrentPath)
            onLayoutProfileChanged: root.layoutStateChanged(layoutProfile, navigationMode)
            onNavigationModeChanged: root.layoutStateChanged(layoutProfile, navigationMode)
        
            Connections {
                target: root.resolveRouter()
                ignoreUnknownSignals: true
                function onCurrentPathChanged() {
                    root.syncNavIndexToCurrentPath()
                }
                function onNavigated(path, params) {
                    root.syncNavIndexToCurrentPath()
                }
            }
        
            Component {
                id: defaultNavDelegate
        
                ItemDelegate {
                    id: control
                    required property int index
                    property var item: root.itemAt(control.index)
                    property string itemLabel: typeof item === "string" ? item : (item.label || item.title || item.text || "")
                    property string itemIcon: typeof item === "object" ? (item.icon || item.iconName || item.symbol || "") : ""
                    property string itemBadge: typeof item === "object" && item.badge !== undefined ? String(item.badge) : ""
                    property bool itemEnabled: typeof item === "object" && item.enabled !== undefined ? item.enabled : true
        
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    Layout.preferredHeight: implicitHeight
                    width: parent ? parent.width : implicitWidth
                    text: itemLabel
                    enabled: itemEnabled
                    highlighted: control.index === root.navIndex
                    padding: Theme.gap10
        
                    contentItem: RowLayout {
                        spacing: Theme.gap8
                        Layout.fillWidth: true
        
                        Label {
                            style: description
                            visible: control.itemIcon.length > 0
                            text: control.itemIcon
                            color: control.highlighted ? Theme.textPrimary : Theme.textTertiary
                        }
        
                        Label {
                            style: body
                            text: control.text
                            color: control.highlighted ? Theme.textPrimary : Theme.textSecondary
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
        
                        Rectangle {
                            visible: control.itemBadge.length > 0
                            radius: Theme.radiusMd
                            color: control.highlighted ? Theme.accent : Theme.surfaceSolid
                            Layout.preferredHeight: Theme.textDisplaySm
                            Layout.preferredWidth: Math.max(Theme.textDisplaySm, badgeText.implicitWidth + Theme.gap10)
        
                            Label {
                                style: caption
                                id: badgeText
                                anchors.centerIn: parent
                                text: control.itemBadge
                                color: control.highlighted ? Theme.textPrimary : Theme.textPrimary
                            }
                        }
                    }
        
                    background: Rectangle {
                        radius: Theme.radiusSm
                        color: control.highlighted ? Theme.accent : "transparent"
                    }
        
                    onClicked: {
                        root.navIndex = control.index
                        root.navActivated(control.index, control.item)
                        var path = root.routeForItem(control.item)
                        root.navigateTo(path, root.paramsForItem(control.item))
                    }
                }
            }
        
            Component {
                id: defaultDrawerDelegate
        
                ItemDelegate {
                    id: control
                    required property int index
                    property var item: root.itemAt(control.index)
                    property string itemLabel: typeof item === "string" ? item : (item.label || item.title || item.text || "")
                    property string itemIcon: typeof item === "object" ? (item.icon || item.iconName || item.symbol || "") : ""
                    property string itemBadge: typeof item === "object" && item.badge !== undefined ? String(item.badge) : ""
                    property bool itemEnabled: typeof item === "object" && item.enabled !== undefined ? item.enabled : true
        
                    Layout.fillWidth: true
                    Layout.fillHeight: false
                    Layout.preferredHeight: implicitHeight
                    width: parent ? parent.width : implicitWidth
                    text: itemLabel
                    enabled: itemEnabled
                    highlighted: control.index === root.navIndex
                    padding: Theme.gap10
        
                    contentItem: RowLayout {
                        spacing: Theme.gap8
                        Layout.fillWidth: true
        
                        Label {
                            style: description
                            visible: control.itemIcon.length > 0
                            text: control.itemIcon
                            color: control.highlighted ? Theme.textPrimary : Theme.textTertiary
                        }
        
                        Label {
                            style: body
                            text: control.text
                            color: control.highlighted ? Theme.textPrimary : Theme.textSecondary
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
        
                        Rectangle {
                            visible: control.itemBadge.length > 0
                            radius: Theme.radiusMd
                            color: control.highlighted ? Theme.accent : Theme.surfaceSolid
                            Layout.preferredHeight: Theme.textDisplaySm
                            Layout.preferredWidth: Math.max(Theme.textDisplaySm, badgeText.implicitWidth + Theme.gap10)
        
                            Label {
                                style: caption
                                id: badgeText
                                anchors.centerIn: parent
                                text: control.itemBadge
                                color: control.highlighted ? Theme.textPrimary : Theme.textPrimary
                            }
                        }
                    }
        
                    background: Rectangle {
                        radius: Theme.radiusSm
                        color: control.highlighted ? Theme.accent : "transparent"
                    }
        
                    onClicked: {
                        root.navIndex = control.index
                        root.navActivated(control.index, control.item)
                        var path = root.routeForItem(control.item)
                        root.navigateTo(path, root.paramsForItem(control.item))
                        navDrawer.close()
                    }
                }
            }
        
            Component {
                id: defaultBottomDelegate
        
                ItemDelegate {
                    id: control
                    required property int index
                    property var item: root.itemAt(control.index)
                    property string itemLabel: typeof item === "string" ? item : (item.label || item.title || item.text || "")
                    property string itemIcon: typeof item === "object" ? (item.icon || item.iconName || item.symbol || "") : ""
                    property bool itemEnabled: typeof item === "object" && item.enabled !== undefined ? item.enabled : true
        
                    Layout.fillWidth: true
                    Layout.preferredWidth: 1
                    enabled: itemEnabled
                    highlighted: control.index === root.navIndex
                    padding: Theme.gap8
        
                    contentItem: Column {
                        spacing: Theme.gap4
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width
        
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            style: description
                            visible: control.itemIcon.length > 0
                            text: control.itemIcon
                            color: control.highlighted ? Theme.textPrimary : Theme.textTertiary
                        }
        
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            style: caption
                            text: control.itemLabel
                            color: control.highlighted ? Theme.textPrimary : Theme.textSecondary
                            elide: Text.ElideRight
                        }
                    }
        
                    background: Rectangle {
                        radius: Theme.radiusMd
                        color: control.highlighted ? Theme.accent : "transparent"
                    }
        
                    onClicked: {
                        root.navIndex = control.index
                        root.navActivated(control.index, control.item)
                        var path = root.routeForItem(control.item)
                        root.navigateTo(path, root.paramsForItem(control.item))
                    }
                }
            }
        
            ColumnLayout {
                anchors.fill: parent
                spacing: Theme.gapNone
        
                Item {
                    id: contentRoot
                    Layout.fillWidth: true
                    Layout.fillHeight: true
        
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: root.adaptiveOuterMargin
                        spacing: Theme.gap12
        
                        Rectangle {
                            id: navRail
                            visible: root.navigationRailEnabled
                            Layout.preferredWidth: root.navigationRailEnabled ? root.effectiveNavRailWidth : 0
                            Layout.fillHeight: true
                            radius: Theme.radiusLg
                            color: Theme.surfaceSolid
        
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: root.adaptiveNavigationInset
                                spacing: Theme.gap12
        
                                Loader {
                                    active: root.navHeader !== null
                                    sourceComponent: root.navHeader
                                    visible: active
                                    Layout.fillWidth: true
                                }
        
                                Label {
                                    style: caption
                                    visible: root.navTitleVisible
                                    text: root.navTitle
                                    color: Theme.textTertiary
                                }
        
                                Repeater {
                                    model: root.navModel
                                    delegate: root.navDelegate ? root.navDelegate : defaultNavDelegate
                                }
        
                                Loader {
                                    active: root.navFooter !== null
                                    sourceComponent: root.navFooter
                                    visible: active
                                    Layout.fillWidth: true
                                }
                            }
                        }
        
                        Item {
                            id: contentWrap
                            Layout.fillWidth: true
                            Layout.fillHeight: true
        
                            Item {
                                id: contentHost
                                anchors.fill: parent
                                anchors.margins: root.adaptiveContentInset
        
                                PageRouter {
                                    id: internalPageRouter
                                    anchors.fill: parent
                                    visible: root.internalPageStackEnabled
                                    enabled: visible
                                    retainInactivePageCount: windowRoot.pageRouterRetainInactivePages
                                    routeResolveCacheCapacity: windowRoot.pageRouterCacheCapacity
                                    routes: root.effectiveRoutes
                                    initialPath: root.internalPageStackEnabled ? root.initialPath : ""
                                    registerAsGlobalNavigator: root.internalPageStackEnabled && root.internalRouterRegisterAsGlobalNavigator
                                    onNavigated: function(path, params) { root.stackNavigated(path, params) }
                                    onNavigationFailed: function(path) { root.stackNavigationFailed(path) }
                                }
        
                                Item {
                                    id: contentArea
                                    anchors.fill: parent
                                    visible: !root.internalPageStackEnabled
                                }
                            }
                        }
                    }
                }
        
                Rectangle {
                    id: bottomNav
                    visible: root.bottomNavigationEnabled
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? Theme.controlHeightMd + Theme.gap16 : 0
                    Layout.leftMargin: root.adaptiveOuterMargin
                    Layout.rightMargin: root.adaptiveOuterMargin
                    Layout.bottomMargin: root.adaptiveOuterMargin
                    radius: Theme.radiusLg
                    color: Theme.surfaceSolid
        
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: root.adaptiveBottomInset
                        spacing: Theme.gap4
        
                        Repeater {
                            model: root.navModel
                            delegate: defaultBottomDelegate
                        }
                    }
                }
            }
        
            Drawer {
                id: navDrawer
                width: root.effectiveNavDrawerWidth
                height: root.height
                edge: Qt.LeftEdge
                modal: true
                interactive: root.drawerNavigationEnabled
                enter: Transition {
                    NumberAnimation {
                        property: "x"
                        from: -navDrawer.width
                        to: 0
                        duration: root.animatedTransitions ? root.drawerEnterDuration : 0
                        easing.type: Easing.OutCubic
                    }
                }
                exit: Transition {
                    NumberAnimation {
                        property: "x"
                        to: -navDrawer.width
                        duration: root.animatedTransitions ? root.drawerExitDuration : 0
                        easing.type: Easing.OutCubic
                    }
                }

                background: Rectangle {
                    color: Theme.surfaceSolid
                }
        
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: root.adaptiveNavigationInset
                    spacing: Theme.gap12
        
                    Loader {
                        active: root.navHeader !== null
                        sourceComponent: root.navHeader
                        visible: active
                        Layout.fillWidth: true
                    }
        
                    Label {
                        style: caption
                        visible: root.navTitleVisible
                        text: root.navTitle
                        color: Theme.textTertiary
                    }
        
                    Repeater {
                        model: root.navModel
                        delegate: root.navDelegate ? root.navDelegate : defaultDrawerDelegate
                    }
        
                    Loader {
                        active: root.navFooter !== null
                        sourceComponent: root.navFooter
                        visible: active
                        Layout.fillWidth: true
                    }
                }
            }
            QtObject {
                Component.onCompleted: {
                    root.syncNavIndexToCurrentPath()
                    root.layoutStateChanged(root.layoutProfile, root.navigationMode)
                }
            }
    }

    Item {
        id: supersampleHost
        anchors.fill: parent
        anchors.margins: windowRoot.safeMargin
        layer.enabled: windowRoot.sceneSupersamplingActive
        layer.smooth: layer.enabled
        layer.mipmap: RenderQuality.mipmapEnabled
        layer.textureSize: RenderQuality.resolveLayerTextureSize(width, height, layer.enabled)

        Item {
            id: scaledContentHost
            x: 0
            y: 0
            width: Math.max(1, Math.round(parent.width / windowRoot.effectiveMobileViewScale))
            height: Math.max(1, Math.round(parent.height / windowRoot.effectiveMobileViewScale))
            scale: windowRoot.effectiveMobileViewScale
            transformOrigin: Item.TopLeft

            AdaptiveLayoutHost {
                id: scaffold
                anchors.fill: parent
                navModel: windowRoot.navItems
                layoutPlatform: windowRoot.platform
                onLayoutStateChanged: function(profile, navigationMode) { windowRoot.adaptiveLayoutStateChanged(profile, navigationMode) }
                onStackNavigated: function(path, params) { windowRoot.pageStackNavigated(path, params) }
                onStackNavigationFailed: function(path) { windowRoot.pageStackNavigationFailed(path) }
                onNavActivated: function(index, item) { windowRoot.navActivated(index, item) }
            }
        }
    }

    Binding {
        target: windowRoot.overlayLayer
        property: "scale"
        value: windowRoot.effectiveMobileViewScale
        when: windowRoot.overlayLayer !== null && windowRoot.effectiveMobileViewScale !== 1.0
    }

    Binding {
        target: windowRoot.overlayLayer
        property: "transformOrigin"
        value: Item.TopLeft
        when: windowRoot.overlayLayer !== null && windowRoot.effectiveMobileViewScale !== 1.0
    }

    Binding {
        target: windowRoot.overlayLayer
        property: "width"
        value: Math.max(1, Math.round(windowRoot.width / windowRoot.effectiveMobileViewScale))
        when: windowRoot.overlayLayer !== null && windowRoot.effectiveMobileViewScale !== 1.0
    }

    Binding {
        target: windowRoot.overlayLayer
        property: "height"
        value: Math.max(1, Math.round(windowRoot.height / windowRoot.effectiveMobileViewScale))
        when: windowRoot.overlayLayer !== null && windowRoot.effectiveMobileViewScale !== 1.0
    }

    EventListener {
        id: globalPressedListener
        anchors.fill: parent
        enabled: windowRoot.globalEventListenersEnabled
        trigger: "globalPressed"
        includeUiHit: false
        includeInputState: false
        action: function(eventData) {
            windowRoot.lastGlobalPressedEventData = eventData || ({})
            windowRoot.globalPressedEvent(eventData)
        }
    }

    EventListener {
        id: globalContextListener
        anchors.fill: parent
        enabled: windowRoot.globalEventListenersEnabled
        trigger: "globalContextRequested"
        includeUiHit: true
        includeInputState: false
        action: function(eventData) {
            windowRoot.lastGlobalContextEventData = eventData || ({})
            windowRoot.globalContextEvent(eventData)
        }
    }

    Item {
        id: windowDragHandle
        visible: windowRoot.windowDragHandleEnabled && windowRoot.windowDragHandleHeight > 0
        enabled: visible
        z: 10000
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: windowRoot.windowDragHandleTopMargin
        height: windowRoot.windowDragHandleHeight

        MouseArea {
            anchors.fill: parent
            enabled: windowDragHandle.enabled
            acceptedButtons: Qt.LeftButton

            onPressed: function(mouse) {
                if (mouse.button !== Qt.LeftButton)
                    return
                if (!windowRoot.requestWindowMove())
                    mouse.accepted = false
            }
        }
    }

    QtObject {
        Component.onCompleted: {
            FontPolicy.enforceApplicationFallback()
            if (windowRoot.autoApplyDeviceTierPreset)
                RenderQuality.applyDeviceTierPreset(windowRoot.forcedDeviceTierPreset)
            RenderQuality.inactiveRenderDowngradeEnabled = windowRoot.inactiveRenderDowngradeEnabled
            RenderQuality.inactiveMsaaSamples = windowRoot.inactiveRenderMsaaSamples
            RenderQuality.applyWindow(windowRoot)
            SvgManager.ensureMinimumScale(windowRoot.effectiveSupersampleScale)
            if (windowRoot.autoAttachRuntimeEvents) {
                windowRoot.ensureRuntimeEventsAttached()
            }
            if (windowRoot.autoHookBackendUserEvents) {
                if (!Backend.hookUserEvents())
                    Debug.warn("ApplicationWindow", "io-event-hook-failed", Backend.lastError)
                else
                    Debug.log("ApplicationWindow", "io-event-hooked", Backend.hookedEventCount)
            }
            Debug.log("ApplicationWindow", "supersample-scale", windowRoot.effectiveSupersampleScale)
            windowRoot.applyNativeWindowStyle()
            Qt.callLater(windowRoot.applyNativeWindowStyle)
        }
    }
}

// API usage (external):
// import LVRS as LV
// LV.ApplicationWindow { title: "App" }
