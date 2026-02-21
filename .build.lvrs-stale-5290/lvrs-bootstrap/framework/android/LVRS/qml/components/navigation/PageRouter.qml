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
    property var _trackedViewIds: []
    property bool _presentationSyncScheduled: false
    property bool _routeResolverDirty: true
    property bool _routeResolverSyncScheduled: false

    readonly property bool canGoBack: stackView.depth > 1
    readonly property int depth: stackView.depth
    readonly property var currentPageItem: stackView.currentItem

    signal navigated(string path, var params)
    signal navigationFailed(string path)
    signal componentNavigated(var component)

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

    function syncActivePagePresentation() {
        if (!isolateInactivePages)
            return

        var topIndex = stackView.depth - 1
        var retainDepth = Math.max(0, retainInactivePageCount)
        var keepFromIndex = Math.max(0, topIndex - retainDepth)
        for (var i = 0; i < stackView.depth; i++) {
            var page = stackView.get(i)
            if (!page)
                continue

            var active = i >= keepFromIndex
            if (page.visible !== undefined)
                page.visible = active
            if (page.enabled !== undefined)
                page.enabled = active
            if (page.opacity !== undefined)
                page.opacity = active ? 1.0 : 0.0
            if (!active && page.focus !== undefined)
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
        if (mode === "replace") {
            item = stackView.replace(target, payload)
        } else if (mode === "set") {
            stackView.clear()
            item = stackView.push(target, payload)
        } else {
            item = stackView.push(target, payload)
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
        if (stackView.depth > 1) {
            stackView.pop()
            applyPageViewportContract(stackView.currentItem)
            scheduleActivePagePresentationSync()
            if (path.length > 1) {
                var nextPath = path.slice(0, path.length - 1)
                setPathInternal(nextPath)
                applyCurrentFromPathEntry(nextPath[nextPath.length - 1])
            } else if (path.length === 1) {
                applyCurrentFromPathEntry(path[0])
            } else {
                setCurrent("", {})
            }
        }
    }

    function popToRoot() {
        if (stackView.depth > 1) {
            stackView.pop(stackView.get(0))
            applyPageViewportContract(stackView.currentItem)
            scheduleActivePagePresentationSync()
            if (path.length > 0) {
                setPathInternal([path[0]])
                applyCurrentFromPathEntry(path[0])
            } else {
                setCurrent("", {})
            }
        }
    }

    function normalizePath(path) {
        if (typeof RouteMatcher !== "undefined"
                && RouteMatcher
                && RouteMatcher.normalizePath) {
            return RouteMatcher.normalizePath(String(path === undefined || path === null ? "" : path))
        }

        var value = String(path || "/")
        if (!value.startsWith("/"))
            value = "/" + value
        if (value.length > 1 && value.endsWith("/"))
            value = value.slice(0, -1)
        return value
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
        _syncingPath = true
        path = nextPath
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
        if (pathValue && typeof pathValue === "object" && pathValue.path !== undefined) {
            return {
                path: pathValue.path === "" ? "" : normalizePath(pathValue.path),
                params: pathValue.params !== undefined ? pathValue.params : ({})
            }
        }
        if (pathValue === undefined || pathValue === null || pathValue === "") {
            return {
                path: "",
                params: params !== undefined ? params : ({})
            }
        }
        return {
            path: normalizePath(pathValue),
            params: params !== undefined ? params : ({})
        }
    }

    function updatePathStack(pathValue, params, mode) {
        var nextEntry = createPathEntry(pathValue, params)
        if (mode === "set" || path.length === 0) {
            setPathInternal([nextEntry])
        } else if (mode === "replace") {
            if (path.length === 0)
                setPathInternal([nextEntry])
            else {
                var next = path.slice(0)
                next[next.length - 1] = nextEntry
                setPathInternal(next)
            }
        } else {
            var nextPush = path.slice(0)
            nextPush.push(nextEntry)
            setPathInternal(nextPush)
        }
    }

    function createComponentPathEntry(component, params) {
        return {
            path: "",
            params: params !== undefined ? params : ({}),
            component: component
        }
    }

    function createViewTrackingEntry(entry, index) {
        if (typeof entry === "string") {
            var normalized = normalizePath(entry)
            return {
                viewId: normalized,
                path: normalized,
                enabled: true
            }
        }

        if (typeof entry !== "object" || entry === null)
            return null

        var pathValue = ""
        if (entry.path !== undefined && entry.path !== null && entry.path !== "")
            pathValue = normalizePath(entry.path)

        var viewId = ""
        if (entry.viewId !== undefined && entry.viewId !== null) {
            var candidate = String(entry.viewId).trim()
            if (candidate.length > 0)
                viewId = candidate
        }
        if (viewId === "" && pathValue !== "")
            viewId = pathValue
        if (viewId === "")
            viewId = "_component_" + index

        var enabled = true
        if (entry.enabled !== undefined)
            enabled = !!entry.enabled
        if (entry.disabled !== undefined && !!entry.disabled)
            enabled = false
        if (entry.params !== undefined && entry.params !== null && entry.params.disabled !== undefined && !!entry.params.disabled)
            enabled = false

        return {
            viewId: viewId,
            path: pathValue,
            enabled: enabled
        }
    }

    function buildViewTrackingEntries() {
        var entries = []
        if (!path || path.length === undefined)
            return entries

        for (var i = 0; i < path.length; i++) {
            var entry = createViewTrackingEntry(path[i], i)
            if (entry)
                entries.push(entry)
        }
        return entries
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

        var nextIds = {}
        for (var i = 0; i < entries.length; i++) {
            var entry = entries[i]
            if (!entry || !entry.viewId)
                continue
            nextIds[String(entry.viewId)] = true
        }

        for (var j = 0; j < _trackedViewIds.length; j++) {
            var existingId = _trackedViewIds[j]
            if (!nextIds[existingId])
                ViewModels.unbindView(existingId)
        }

        _trackedViewIds = Object.keys(nextIds)
    }

    function releaseTrackedViewBindings() {
        if (typeof ViewModels === "undefined" || !ViewModels || !ViewModels.unbindView) {
            _trackedViewIds = []
            return
        }

        for (var i = 0; i < _trackedViewIds.length; i++)
            ViewModels.unbindView(_trackedViewIds[i])
        _trackedViewIds = []
    }

    function updateComponentPathStack(component, params, mode) {
        var nextEntry = createComponentPathEntry(component, params)
        if (mode === "set" || path.length === 0) {
            setPathInternal([nextEntry])
        } else if (mode === "replace") {
            if (path.length === 0)
                setPathInternal([nextEntry])
            else {
                var next = path.slice(0)
                next[next.length - 1] = nextEntry
                setPathInternal(next)
            }
        } else {
            var nextPush = path.slice(0)
            nextPush.push(nextEntry)
            setPathInternal(nextPush)
        }
    }

}

// API usage (external):
// import LVRS 1.0 as LV
// LV.PageRouter {
//     routes: [{ path: "/", component: homePage, viewModelKey: "OverviewVM", writable: true }]
// }
