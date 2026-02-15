import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import LVRS 1.0

Item {
    id: root

    property string headerTitle: "LVRS"
    property string headerSubtitle: ""
    property var navModel: ["Overview", "Suites", "Runs", "Devices", "Reports", "Settings"]
    property int navIndex: 0
    property bool navigationEnabled: true
    property string navTitle: "Navigation"
    property bool navTitleVisible: true
    property int navWidth: 220
    property int navDrawerWidth: 240
    property int wideBreakpoint: 980
    property string layoutMode: "auto" // auto, mobile, desktop
    property string layoutPlatform: Qt.platform.os
    property bool forceDesktopOnLargeMobile: false
    property int mobileDesktopMinWidth: 1200
    property bool preferBottomNavigation: true
    property int bottomNavigationMaxItems: 5
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

    property alias headerActions: appHeader.actions

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

    function requestedNavigationModeForProfile(profile) {
        if (!root.hasNav)
            return "none"
        if (profile === "desktop-wide")
            return "rail"
        if (profile.indexOf("mobile-") === 0
                && root.preferBottomNavigation
                && root.navModelCount() <= root.bottomNavigationMaxItems) {
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
            property var item: root.itemAt(index)
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
            highlighted: index === root.navIndex
            padding: Theme.gap10

            contentItem: RowLayout {
                spacing: Theme.gap8
                Layout.fillWidth: true

                Label {
                    style: description
                    visible: itemIcon.length > 0
                    text: itemIcon
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
                    visible: itemBadge.length > 0
                    radius: Theme.radiusMd
                    color: control.highlighted ? Theme.accent : Theme.surfaceSolid
                    Layout.preferredHeight: Theme.textDisplaySm
                    Layout.preferredWidth: Math.max(Theme.textDisplaySm, badgeText.implicitWidth + Theme.gap10)

                    Label {
                        style: caption
                        id: badgeText
                        anchors.centerIn: parent
                        text: itemBadge
                        color: control.highlighted ? Theme.textPrimary : Theme.textPrimary
                    }
                }
            }

            background: Rectangle {
                radius: Theme.radiusSm
                color: control.highlighted ? Theme.accent : "transparent"
            }

            onClicked: {
                root.navIndex = index
                root.navActivated(index, item)
                var path = root.routeForItem(item)
                root.navigateTo(path, root.paramsForItem(item))
            }
        }
    }

    Component {
        id: defaultDrawerDelegate

        ItemDelegate {
            id: control
            property var item: root.itemAt(index)
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
            highlighted: index === root.navIndex
            padding: Theme.gap10

            contentItem: RowLayout {
                spacing: Theme.gap8
                Layout.fillWidth: true

                Label {
                    style: description
                    visible: itemIcon.length > 0
                    text: itemIcon
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
                    visible: itemBadge.length > 0
                    radius: Theme.radiusMd
                    color: control.highlighted ? Theme.accent : Theme.surfaceSolid
                    Layout.preferredHeight: Theme.textDisplaySm
                    Layout.preferredWidth: Math.max(Theme.textDisplaySm, badgeText.implicitWidth + Theme.gap10)

                    Label {
                        style: caption
                        id: badgeText
                        anchors.centerIn: parent
                        text: itemBadge
                        color: control.highlighted ? Theme.textPrimary : Theme.textPrimary
                    }
                }
            }

            background: Rectangle {
                radius: Theme.radiusSm
                color: control.highlighted ? Theme.accent : "transparent"
            }

            onClicked: {
                root.navIndex = index
                root.navActivated(index, item)
                var path = root.routeForItem(item)
                root.navigateTo(path, root.paramsForItem(item))
                navDrawer.close()
            }
        }
    }

    Component {
        id: defaultBottomDelegate

        ItemDelegate {
            id: control
            property var item: root.itemAt(index)
            property string itemLabel: typeof item === "string" ? item : (item.label || item.title || item.text || "")
            property string itemIcon: typeof item === "object" ? (item.icon || item.iconName || item.symbol || "") : ""
            property bool itemEnabled: typeof item === "object" && item.enabled !== undefined ? item.enabled : true

            Layout.fillWidth: true
            Layout.preferredWidth: 1
            enabled: itemEnabled
            highlighted: index === root.navIndex
            padding: Theme.gap8

            contentItem: Column {
                spacing: Theme.gap4
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: parent.width

                Label {
                    anchors.horizontalCenter: parent.horizontalCenter
                    style: description
                    visible: itemIcon.length > 0
                    text: itemIcon
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
                root.navIndex = index
                root.navActivated(index, item)
                var path = root.routeForItem(item)
                root.navigateTo(path, root.paramsForItem(item))
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.window

        gradient: Gradient {
            GradientStop { position: 0.0; color: Theme.window }
            GradientStop { position: 0.6; color: Theme.windowAlt }
            GradientStop { position: 1.0; color: Theme.window }
        }

        Rectangle {
            width: Theme.scaffoldBlobPrimarySize
            height: Theme.scaffoldBlobPrimarySize
            radius: Theme.scaffoldBlobPrimaryRadius
            color: Theme.accent
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: Theme.scaffoldBlobPrimaryRightMargin
            anchors.topMargin: Theme.scaffoldBlobPrimaryTopMargin
        }

        Rectangle {
            width: Theme.scaffoldBlobSecondaryWidth
            height: Theme.scaffoldBlobSecondaryHeight
            radius: Theme.scaffoldBlobSecondaryRadius
            color: Theme.accent
            opacity: Theme.scaffoldBlobSecondaryOpacity
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: Theme.scaffoldBlobSecondaryLeftMargin
            anchors.bottomMargin: Theme.scaffoldBlobSecondaryBottomMargin
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.gapNone

        AppHeader {
            id: appHeader
            title: root.headerTitle
            subtitle: root.headerSubtitle
            menuVisible: root.drawerNavigationEnabled
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight
            onMenuClicked: {
                if (root.drawerNavigationEnabled)
                    navDrawer.open()
            }
        }

        Item {
            id: contentRoot
            Layout.fillWidth: true
            Layout.fillHeight: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.radiusXl
                spacing: Theme.gap12

                Rectangle {
                    id: navRail
                    visible: root.navigationRailEnabled
                    Layout.preferredWidth: root.navigationRailEnabled ? root.navWidth : 0
                    Layout.fillHeight: true
                    radius: Theme.radiusLg
                    color: Theme.surfaceSolid

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.gap16
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

                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radiusXl
                        color: Theme.surfaceAlt
                    }

                    Item {
                        id: contentHost
                        anchors.fill: parent
                        anchors.margins: Theme.radiusLg

                        PageRouter {
                            id: internalPageRouter
                            anchors.fill: parent
                            visible: root.internalPageStackEnabled
                            enabled: visible
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
            Layout.leftMargin: Theme.radiusXl
            Layout.rightMargin: Theme.radiusXl
            Layout.bottomMargin: Theme.radiusXl
            radius: Theme.radiusLg
            color: Theme.surfaceSolid

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.gap8
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
        width: root.navDrawerWidth
        height: root.height
        edge: Qt.LeftEdge
        modal: true
        interactive: root.drawerNavigationEnabled

        background: Rectangle {
            color: Theme.surfaceSolid
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.radiusXl
            spacing: Theme.gap12

            Label {
                style: header
                text: root.headerTitle
                color: Theme.textPrimary
            }

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

// API usage (external):
// import LVRS 1.0 as LV
// LV.AppScaffold { headerTitle: "LVRS"; navModel: ["Overview"] }
