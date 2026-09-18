pragma Singleton
import QtQuick

QtObject {
    id: root

    property var router: null
    property var routerStack: []

    readonly property bool hasRouter: router !== null
    readonly property string currentPath:
        hasRouter && router.currentPath !== undefined ? router.currentPath : ""
    readonly property int depth:
        hasRouter && router.depth !== undefined ? router.depth : 0
    readonly property bool interactiveTransitionActive:
        hasRouter && router.interactiveTransitionActive !== undefined ? router.interactiveTransitionActive : false
    readonly property real interactiveTransitionProgress:
        hasRouter && router.interactiveTransitionProgress !== undefined ? router.interactiveTransitionProgress : 0.0
    readonly property string interactiveTransitionDirection:
        hasRouter && router.interactiveTransitionDirection !== undefined ? router.interactiveTransitionDirection : "none"
    readonly property string interactiveTransitionOperation:
        hasRouter && router.interactiveTransitionOperation !== undefined ? router.interactiveTransitionOperation : "none"

    function childIndexInParent(targetRouter) {
        if (!targetRouter || !targetRouter.parent || targetRouter.parent.children === undefined)
            return -1

        var siblings = targetRouter.parent.children
        var count = siblings.length !== undefined ? siblings.length : 0
        for (var i = 0; i < count; i++) {
            if (siblings[i] === targetRouter)
                return i
        }
        return -1
    }

    function insertRouterBySiblingOrder(next, targetRouter) {
        var targetIndex = childIndexInParent(targetRouter)
        if (targetIndex < 0)
            return false

        for (var i = 0; i < next.length; i++) {
            var candidate = next[i]
            if (!candidate || candidate.parent !== targetRouter.parent)
                continue
            var candidateIndex = childIndexInParent(candidate)
            if (candidateIndex < 0)
                continue
            if (candidateIndex > targetIndex) {
                next.splice(i, 0, targetRouter)
                return true
            }
        }
        return false
    }

    function updateActiveRouterFromStack() {
        var activeRouter = routerStack.length > 0 ? routerStack[routerStack.length - 1] : null
        if (router === activeRouter)
            return
        router = activeRouter
        if (router && router.syncViewStateTracker !== undefined)
            router.syncViewStateTracker()
    }

    function registerRouter(targetRouter) {
        if (!targetRouter)
            return false

        var next = []
        for (var i = 0; i < routerStack.length; i++) {
            var candidate = routerStack[i]
            if (candidate && candidate !== targetRouter)
                next.push(candidate)
        }
        if (!insertRouterBySiblingOrder(next, targetRouter))
            next.push(targetRouter)
        routerStack = next
        updateActiveRouterFromStack()
        return true
    }

    function unregisterRouter(targetRouter) {
        if (!targetRouter) {
            routerStack = []
            updateActiveRouterFromStack()
            return
        }

        var next = []
        for (var i = 0; i < routerStack.length; i++) {
            var candidate = routerStack[i]
            if (candidate && candidate !== targetRouter)
                next.push(candidate)
        }
        routerStack = next
        updateActiveRouterFromStack()
    }

    function canNavigate() {
        return hasRouter && router.go !== undefined
    }

    function go(path, params) {
        if (!canNavigate() || !path)
            return false
        router.go(path, params || ({}))
        return true
    }

    function replace(path, params) {
        if (!canNavigate() || !path)
            return false
        router.replace(path, params || ({}))
        return true
    }

    function setRoot(path, params) {
        if (!canNavigate() || !path)
            return false
        router.setRoot(path, params || ({}))
        return true
    }

    function goTo(component, params) {
        if (!canNavigate() || !component || router.goTo === undefined)
            return false
        router.goTo(component, params || ({}))
        return true
    }

    function replaceWith(component, params) {
        if (!canNavigate() || !component || router.replaceWith === undefined)
            return false
        router.replaceWith(component, params || ({}))
        return true
    }

    function setRootComponent(component, params) {
        if (!canNavigate() || !component || router.setRootComponent === undefined)
            return false
        router.setRootComponent(component, params || ({}))
        return true
    }

    function back() {
        if (!hasRouter || router.back === undefined)
            return false
        router.back()
        return true
    }

    function popToRoot() {
        if (!hasRouter || router.popToRoot === undefined)
            return false
        router.popToRoot()
        return true
    }

    function beginInteractiveTransition(spec) {
        if (!hasRouter || router.beginInteractiveTransition === undefined)
            return false
        return router.beginInteractiveTransition(spec || ({}))
    }

    function beginInteractiveBack(meta) {
        if (!hasRouter || router.beginInteractiveBack === undefined)
            return false
        return router.beginInteractiveBack(meta || ({}))
    }

    function beginInteractivePush(path, params, meta) {
        if (!hasRouter || !path || router.beginInteractivePush === undefined)
            return false
        return router.beginInteractivePush(path, params || ({}), meta || ({}))
    }

    function beginInteractiveReplace(path, params, meta) {
        if (!hasRouter || !path || router.beginInteractiveReplace === undefined)
            return false
        return router.beginInteractiveReplace(path, params || ({}), meta || ({}))
    }

    function beginInteractiveSetRoot(path, params, meta) {
        if (!hasRouter || !path || router.beginInteractiveSetRoot === undefined)
            return false
        return router.beginInteractiveSetRoot(path, params || ({}), meta || ({}))
    }

    function updateInteractiveTransition(progress, details) {
        if (!hasRouter || router.updateInteractiveTransition === undefined)
            return false
        return router.updateInteractiveTransition(progress, details || ({}))
    }

    function finishInteractiveTransition(commit) {
        if (!hasRouter || router.finishInteractiveTransition === undefined)
            return false
        return router.finishInteractiveTransition(commit)
    }

    function cancelInteractiveTransition() {
        if (!hasRouter || router.cancelInteractiveTransition === undefined)
            return false
        return router.cancelInteractiveTransition()
    }

    function shouldCommitInteractiveTransition(progress, velocityX, velocityY) {
        if (!hasRouter || router.shouldCommitInteractiveTransition === undefined)
            return false
        return router.shouldCommitInteractiveTransition(progress, velocityX, velocityY)
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Navigator.go("/reports")
