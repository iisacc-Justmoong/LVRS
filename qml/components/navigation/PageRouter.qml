import QtQuick
import QtQuick.Controls
import LVRS 1.0

Item {
    id: root

    property var routes: []
    property string initialPath: "/"
    // SwiftUI-like navigation stack (array of path strings or { path, params } entries).
    property var path: []
    property string _currentPath: ""
    property var _currentParams: ({})
    readonly property string currentPath: _currentPath
    readonly property var currentParams: _currentParams

    property Component notFoundComponent: null
    property url notFoundSource: ""
    property bool registerAsGlobalNavigator: true
    property bool enforcePageViewport: true
    property bool isolateInactivePages: true
    property int retainInactivePageCount: 0
    property int routeResolveCacheCapacity: 256
    property bool interactiveTransitionsEnabled: true
    property int interactiveTransitionSettleDuration: 0
    property real interactiveTransitionCommitProgress: 0.42
    property real interactiveTransitionVelocityThreshold: 960
    property real interactiveTransitionOutgoingParallaxFactor: 0.25
    property real interactiveTransitionIncomingPreviewFactor: 0.35
    property var _trackedViewIds: []
    property bool _presentationSyncScheduled: false
    property bool _routeResolverDirty: true
    property bool _routeResolverSyncScheduled: false

    readonly property bool canGoBack: stackView.depth > 1
    readonly property int depth: stackView.depth
    readonly property var currentPageItem: stackView.currentItem
    readonly property bool interactiveTransitionActive: interactiveTransitionDriver.active
    readonly property real interactiveTransitionProgress: interactiveTransitionDriver.progress
    readonly property string interactiveTransitionDirection: interactiveTransitionDriver.direction
    readonly property string interactiveTransitionOperation: interactiveTransitionDriver.operation
    readonly property string interactiveTransitionFromPath: interactiveTransitionDriver.fromPath
    readonly property string interactiveTransitionToPath: interactiveTransitionDriver.toPath
    readonly property var interactiveTransitionFromParams: interactiveTransitionDriver.fromParams
    readonly property var interactiveTransitionToParams: interactiveTransitionDriver.toParams
    readonly property real interactiveTransitionVelocityX: interactiveTransitionDriver.velocityX
    readonly property real interactiveTransitionVelocityY: interactiveTransitionDriver.velocityY
    readonly property var interactiveTransitionMeta: interactiveTransitionDriver.meta
    readonly property var interactiveTransitionPreviewItem: interactiveTransitionDriver.previewItem
    readonly property bool interactiveTransitionCanCommit: interactiveTransitionDriver.canCommit

    signal navigated(string path, var params)
    signal navigationFailed(string path)
    signal componentNavigated(var component)
    signal interactiveTransitionStarted(var state)
    signal interactiveTransitionUpdated(var state)
    signal interactiveTransitionCommitted(var state)
    signal interactiveTransitionCancelled(var state)
    signal interactiveTransitionRejected(string reason, var state)

    NavigationStackModel {
        id: navigationStackModel
    }

    function hasAnyAnchors(item) {
        if (!item || item.anchors === undefined)
            return false
        return item.anchors.left || item.anchors.right
            || item.anchors.top || item.anchors.bottom
            || item.anchors.horizontalCenter || item.anchors.verticalCenter
    }


    function applyPageViewportContract(item) {
        if (!item || !enforcePageViewport)
            return

        if (item.x !== undefined)
            item.x = 0
        if (item.y !== undefined)
            item.y = 0

        if (item.width !== undefined)
            item.width = Qt.binding(function() { return stackView.width })
        if (item.height !== undefined)
            item.height = Qt.binding(function() { return stackView.height })

        if (item.clip !== undefined)
            item.clip = true
    }

    function applySingleChildViewportContract(container) {
        if (!container || !enforcePageViewport || container.children === undefined || container.children.length !== 1)
            return

        var child = container.children[0]
        if (!child || child === container || child.width === undefined || child.height === undefined)
            return

        var childHasExternalAnchors = child.anchors !== undefined
            && hasAnyAnchors(child)
            && child.anchors.fill !== stackView
        if (childHasExternalAnchors)
            return

        if (container.width !== undefined
                && container.height !== undefined
                && child.width >= container.width - 0.5
                && child.height >= container.height - 0.5) {
            return
        }

        if (child.x !== undefined)
            child.x = 0
        if (child.y !== undefined)
            child.y = 0
        child.width = Qt.binding(function() { return stackView.width })
        child.height = Qt.binding(function() { return stackView.height })
        if (child.clip !== undefined)
            child.clip = true
    }

    function interactiveTransitionState() {
        return interactiveTransitionDriver.currentState()
    }

    function abortInteractiveTransition() {
        return interactiveTransitionDriver.abortTransition()
    }

    function beginInteractiveTransition(spec) {
        return interactiveTransitionDriver.beginTransition(spec)
    }

    function beginInteractiveBack(meta) {
        return interactiveTransitionDriver.beginBack(meta)
    }

    function beginInteractivePush(pathValue, params, meta) {
        return interactiveTransitionDriver.beginPush(pathValue, params, meta)
    }

    function beginInteractiveReplace(pathValue, params, meta) {
        return interactiveTransitionDriver.beginReplace(pathValue, params, meta)
    }

    function beginInteractiveSetRoot(pathValue, params, meta) {
        return interactiveTransitionDriver.beginSetRoot(pathValue, params, meta)
    }

    function beginInteractivePushComponent(component, params, meta) {
        return interactiveTransitionDriver.beginPushComponent(component, params, meta)
    }

    function beginInteractiveReplaceComponent(component, params, meta) {
        return interactiveTransitionDriver.beginReplaceComponent(component, params, meta)
    }

    function beginInteractiveSetRootComponent(component, params, meta) {
        return interactiveTransitionDriver.beginSetRootComponent(component, params, meta)
    }

    function updateInteractiveTransition(progress, details) {
        return interactiveTransitionDriver.updateTransition(progress, details)
    }

    function shouldCommitInteractiveTransition(progress, velocityX, velocityY) {
        return interactiveTransitionDriver.shouldCommitTransition(progress, velocityX, velocityY)
    }

    function finishInteractiveTransition(commit) {
        return interactiveTransitionDriver.finishTransition(commit)
    }

    function cancelInteractiveTransition() {
        return interactiveTransitionDriver.cancelTransition()
    }

    function syncActivePagePresentation() {
        if (!isolateInactivePages)
            return

        var topIndex = stackView.depth - 1
        var retainDepth = Math.max(0, retainInactivePageCount)
        var keepFromIndex = Math.max(0, topIndex - retainDepth)
        var transitionLocked = interactiveTransitionDriver.transitionLocked
        var previewIndex = -1

        if (transitionLocked
                && interactiveTransitionDriver.secondaryItem
                && interactiveTransitionDriver.operation === "pop"
                && topIndex > 0) {
            previewIndex = topIndex - 1
            keepFromIndex = Math.min(keepFromIndex, previewIndex)
        }

        for (var i = 0; i < stackView.depth; i++) {
            var page = stackView.get(i)
            if (!page)
                continue

            var active = i >= keepFromIndex || i === previewIndex
            var transitionParticipant = transitionLocked && (i === topIndex || i === previewIndex)
            if (page.visible !== undefined)
                page.visible = active
            if (page.enabled !== undefined)
                page.enabled = transitionParticipant ? false : active
            if (page.opacity !== undefined)
                page.opacity = active ? 1.0 : 0.0
            if ((!active || transitionParticipant) && page.focus !== undefined)
                page.focus = false
        }
    }

    function scheduleActivePagePresentationSync() {
        if (_presentationSyncScheduled)
            return
        _presentationSyncScheduled = true
        Qt.callLater(function() {
            _presentationSyncScheduled = false
            syncActivePagePresentation()
        })
    }

    function applyStackOperation(target, params, mode) {
        var payload = params !== undefined ? params : ({})
        var item = null
        var immediate = interactiveTransitionDriver.commitApplying ? StackView.Immediate : undefined
        if (mode === "replace") {
            item = immediate !== undefined
                ? stackView.replace(target, payload, immediate)
                : stackView.replace(target, payload)
        } else if (mode === "set") {
            stackView.clear()
            item = immediate !== undefined
                ? stackView.push(target, payload, immediate)
                : stackView.push(target, payload)
        } else {
            item = immediate !== undefined
                ? stackView.push(target, payload, immediate)
                : stackView.push(target, payload)
        }

        applyPageViewportContract(item)
        applySingleChildViewportContract(item)
        scheduleActivePagePresentationSync()
        return item
    }

    function go(path, params) {
        navigate(path, params, "push")
    }

    function replace(path, params) {
        navigate(path, params, "replace")
    }

    function setRoot(path, params) {
        navigate(path, params, "set")
    }

    function goTo(component, params) {
        navigateComponent(component, params, "push")
    }

    function replaceWith(component, params) {
        navigateComponent(component, params, "replace")
    }

    function setRootComponent(component, params) {
        navigateComponent(component, params, "set")
    }

    function back() {
        pop()
    }

    function push(path, params) {
        navigate(path, params, "push")
    }

    function pop() {
        if (interactiveTransitionDriver.transitionLocked && !interactiveTransitionDriver.commitApplying)
            abortInteractiveTransition()
        if (stackView.depth > 1) {
            if (interactiveTransitionDriver.commitApplying)
                stackView.pop(null, StackView.Immediate)
            else
                stackView.pop()
            applyPageViewportContract(stackView.currentItem)
            scheduleActivePagePresentationSync()
            navigationStackModel.path = path
            var nextPath = navigationStackModel.stackAfterPop()
            if (nextPath.length > 0) {
                setPathInternal(nextPath)
                applyCurrentFromPathEntry(nextPath[nextPath.length - 1])
            } else {
                setCurrent("", {})
            }
        }
    }

    function popToRoot() {
        if (interactiveTransitionDriver.transitionLocked && !interactiveTransitionDriver.commitApplying)
            abortInteractiveTransition()
        if (stackView.depth > 1) {
            if (interactiveTransitionDriver.commitApplying)
                stackView.pop(stackView.get(0), StackView.Immediate)
            else
                stackView.pop(stackView.get(0))
            applyPageViewportContract(stackView.currentItem)
            scheduleActivePagePresentationSync()
            navigationStackModel.path = path
            var nextPath = navigationStackModel.stackAfterPopToRoot()
            if (nextPath.length > 0) {
                setPathInternal(nextPath)
                applyCurrentFromPathEntry(nextPath[0])
            } else {
                setCurrent("", {})
            }
        }
    }

    function normalizePath(path) {
        return navigationStackModel.normalizePath(path)
    }

    function routeModelCount() {
        if (!routes)
            return 0
        if (routes.length !== undefined)
            return Math.max(0, Number(routes.length) || 0)
        if (routes.count !== undefined)
            return Math.max(0, Number(routes.count) || 0)
        return 0
    }

    function routeModelAt(index) {
        if (!routes || index < 0)
            return null
        if (routes.length !== undefined)
            return routes[index]
        if (routes.get !== undefined)
            return routes.get(index)
        return routes[index]
    }

    function markRouteResolverDirty() {
        _routeResolverDirty = true
    }

    function syncRouteResolverRoutes(forceSync) {
        if (!routeResolver || !routeResolver.updateRoutes)
            return

        var nextCapacity = Math.max(16, routeResolveCacheCapacity)
        if (routeResolver.cacheCapacity !== nextCapacity)
            routeResolver.cacheCapacity = nextCapacity

        if (!forceSync && !_routeResolverDirty)
            return

        routeResolver.updateRoutes(routes)
        _routeResolverDirty = false
    }

    function scheduleRouteResolverSync() {
        markRouteResolverDirty()
        if (_routeResolverSyncScheduled)
            return

        _routeResolverSyncScheduled = true
        Qt.callLater(function() {
            _routeResolverSyncScheduled = false
            syncRouteResolverRoutes(false)
        })
    }

    function ensureRouteResolverReady() {
        syncRouteResolverRoutes(false)
    }

    function resolveRoute(path) {
        if (!routes)
            return null

        ensureRouteResolverReady()
        var nativeResolved = routeResolver.resolve(path)
        if (!nativeResolved || !nativeResolved.matched)
            return null

        var routeIndex = nativeResolved.index !== undefined ? Number(nativeResolved.index) : -1
        if (routeIndex < 0 || routeIndex >= routeModelCount())
            return null

        var route = routeModelAt(routeIndex)
        if (!route || !route.path)
            return null

        return {
            route: route,
            params: nativeResolved.params !== undefined ? nativeResolved.params : ({})
        }
    }

    function bindRouteViewModel(pathValue, route, params, fallbackIndex) {
        if (typeof ViewModels === "undefined" || !ViewModels || !ViewModels.bindRouteViewModel)
            return
        ViewModels.bindRouteViewModel(pathValue, route, params, fallbackIndex)
    }

    function navigate(path, params, mode) {
        if (interactiveTransitionDriver.transitionLocked && !interactiveTransitionDriver.commitApplying)
            abortInteractiveTransition()
        var resolved = resolveRoute(path)
        var targetParams = ({})
        if (resolved && resolved.params) {
            for (var key in resolved.params)
                targetParams[key] = resolved.params[key]
        }
        if (params) {
            for (var paramKey in params)
                targetParams[paramKey] = params[paramKey]
        }
        var normalized = normalizePath(path)
        if (!resolved) {
            if (notFoundComponent || notFoundSource) {
                var fallback = notFoundComponent ? notFoundComponent : notFoundSource
                applyStackOperation(fallback, targetParams, mode)
                updatePathStack(normalized, targetParams, mode)
                setCurrent(normalized, targetParams)
                navigated(normalized, targetParams)
                return
            }
            navigationFailed(normalized)
            return
        }
        var route = resolved.route
        var target = route.component ? route.component : route.source
        if (!target) {
            navigationFailed(normalized)
            return
        }
        applyStackOperation(target, targetParams, mode)
        updatePathStack(normalized, targetParams, mode)
        setCurrent(normalized, targetParams)
        bindRouteViewModel(normalized,
                           route,
                           targetParams,
                           root.path && root.path.length !== undefined ? Math.max(0, root.path.length - 1) : 0)
        navigated(normalized, targetParams)
    }

    function navigateComponent(component, params, mode) {
        if (interactiveTransitionDriver.transitionLocked && !interactiveTransitionDriver.commitApplying)
            abortInteractiveTransition()
        if (!component)
            return
        var targetParams = params || {}
        applyStackOperation(component, targetParams, mode)
        updateComponentPathStack(component, targetParams, mode)
        setCurrent("", targetParams)
        bindRouteViewModel("",
                           null,
                           targetParams,
                           root.path && root.path.length !== undefined ? Math.max(0, root.path.length - 1) : 0)
        componentNavigated(component)
    }

    Component.onCompleted: {
        navigationStackModel.path = path
        if (registerAsGlobalNavigator)
            Navigator.registerRouter(root)
        syncRouteResolverRoutes(true)
        if (initialPath)
            setRoot(initialPath)
        else {
            syncViewStateTracker()
            scheduleActivePagePresentationSync()
        }
    }

    Component.onDestruction: {
        releaseTrackedViewBindings()
        if (registerAsGlobalNavigator)
            Navigator.unregisterRouter(root)
    }

    RouteResolver {
        id: routeResolver
        cacheCapacity: Math.max(16, root.routeResolveCacheCapacity)
    }

    StackView {
        id: stackView
        anchors.fill: parent
        clip: true
        focus: true
        onDepthChanged: root.scheduleActivePagePresentationSync()
    }

    Item {
        id: interactivePreviewHost
        anchors.fill: parent
        z: 1
        visible: root.interactiveTransitionActive && root.interactiveTransitionDirection === "forward"
        enabled: false
        clip: true
    }

    PageRouterTransitionDriver {
        id: interactiveTransitionDriver
        router: root
        stackView: stackView
        previewHost: interactivePreviewHost
    }

    property bool _syncingPath: false

    onRoutesChanged: {
        scheduleRouteResolverSync()
    }

    onRouteResolveCacheCapacityChanged: syncRouteResolverRoutes(false)

    Connections {
        target: (root.routes && root.routes.countChanged !== undefined) ? root.routes : null
        ignoreUnknownSignals: true
        function onCountChanged() {
            root.scheduleRouteResolverSync()
        }
        function onDataChanged() {
            root.scheduleRouteResolverSync()
        }
        function onModelReset() {
            root.scheduleRouteResolverSync()
        }
        function onRowsInserted() {
            root.scheduleRouteResolverSync()
        }
        function onRowsRemoved() {
            root.scheduleRouteResolverSync()
        }
    }

    onPathChanged: {
        if (_syncingPath)
            return
        navigationStackModel.path = path
        if (interactiveTransitionDriver.transitionLocked && !interactiveTransitionDriver.commitApplying)
            abortInteractiveTransition()
        rebuildFromPath()
    }

    function rebuildFromPath() {
        if (!path)
            return
        _syncingPath = true
        stackView.clear()
        if (path.length === 0) {
            _syncingPath = false
            setCurrent("", {})
            syncViewStateTracker()
            scheduleActivePagePresentationSync()
            return
        }
        for (var i = 0; i < path.length; i++) {
            var entry = path[i]
            var entryPath = typeof entry === "string" ? entry : entry.path
            var entryParams = typeof entry === "object" && entry.params !== undefined ? entry.params : undefined
            var hasComponentEntry = typeof entry === "object" && entry.component !== undefined
            if (hasComponentEntry && entry.component) {
                applyStackOperation(entry.component, entryParams || ({}), "push")
                continue
            }
            var resolved = resolveRoute(entryPath)
            var normalized = normalizePath(entryPath)
            if (!resolved) {
                var fallback = notFoundComponent ? notFoundComponent : notFoundSource
                if (fallback)
                    applyStackOperation(fallback, entryParams || ({}), "push")
                else
                    navigationFailed(normalized)
                continue
            }
            var target = resolved.route.component ? resolved.route.component : resolved.route.source
            if (!target) {
                navigationFailed(normalized)
                continue
            }
            var mergedParams = entryParams !== undefined ? entryParams : resolved.params
            applyStackOperation(target, mergedParams || ({}), "push")
            bindRouteViewModel(normalized, resolved.route, mergedParams || {}, i)
        }
        _syncingPath = false
        applyCurrentFromPathEntry(path[path.length - 1])
        syncViewStateTracker()
    }

    function setPathInternal(nextPath) {
        navigationStackModel.path = nextPath
        _syncingPath = true
        path = navigationStackModel.path
        _syncingPath = false
        syncViewStateTracker()
    }

    function applyCurrentFromPathEntry(entry) {
        if (entry === undefined || entry === null) {
            setCurrent("", {})
            return
        }
        if (typeof entry === "string") {
            setCurrent(normalizePath(entry), {})
            return
        }
        if (typeof entry === "object") {
            var pathValue = entry.path !== undefined ? entry.path : ""
            setCurrent(
                pathValue === "" ? "" : normalizePath(pathValue),
                entry.params !== undefined ? entry.params : ({})
            )
            return
        }
        setCurrent("", {})
    }

    function setCurrent(pathValue, params) {
        if (pathValue === undefined || pathValue === null || pathValue === "")
            _currentPath = ""
        else
            _currentPath = normalizePath(pathValue)
        _currentParams = params !== undefined ? params : ({})
    }

    function createPathEntry(pathValue, params) {
        return navigationStackModel.createPathEntry(pathValue, params)
    }

    function updatePathStack(pathValue, params, mode) {
        navigationStackModel.path = path
        setPathInternal(navigationStackModel.stackAfterPathOperation(pathValue, params, mode))
    }

    function createComponentPathEntry(component, params) {
        return navigationStackModel.createComponentPathEntry(component, params)
    }

    function createViewTrackingEntry(entry, index) {
        return navigationStackModel.createViewTrackingEntry(entry, index)
    }

    function buildViewTrackingEntries() {
        return navigationStackModel.buildViewTrackingEntries(path)
    }

    function syncViewStateTracker() {
        var entries = buildViewTrackingEntries()
        syncViewModelBindings(entries)

        if (typeof Navigator !== "undefined"
                && Navigator
                && Navigator.router
                && Navigator.router !== root)
            return

        if (typeof ViewStateTracker === "undefined" || !ViewStateTracker || !ViewStateTracker.syncStack)
            return
        ViewStateTracker.syncStack(entries)
    }

    function syncViewModelBindings(entries) {
        if (typeof ViewModels === "undefined" || !ViewModels || !ViewModels.unbindView)
            return

        const removedIds = navigationStackModel.updateTrackedViewIds(entries)
        for (var i = 0; i < removedIds.length; i++)
            ViewModels.unbindView(removedIds[i])
        _trackedViewIds = navigationStackModel.trackedViewIds
    }

    function releaseTrackedViewBindings() {
        if (typeof ViewModels === "undefined" || !ViewModels || !ViewModels.unbindView) {
            _trackedViewIds = []
            return
        }

        const trackedIds = navigationStackModel.trackedViewIds
        for (var i = 0; i < trackedIds.length; i++)
            ViewModels.unbindView(trackedIds[i])
        _trackedViewIds = []
    }

    function updateComponentPathStack(component, params, mode) {
        navigationStackModel.path = path
        setPathInternal(navigationStackModel.stackAfterComponentOperation(component, params, mode))
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.PageRouter {
//     routes: [{ path: "/", component: homePage, viewModelKey: "OverviewVM", writable: true }]
// }
