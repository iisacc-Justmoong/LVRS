import QtQuick
import LVRS 1.0

QtObject {
    id: root

    property var router: (typeof Navigator !== "undefined" && Navigator) ? Navigator.router : null

    readonly property bool active: router && router.interactiveTransitionActive !== undefined
        ? router.interactiveTransitionActive
        : false
    readonly property real progress: router && router.interactiveTransitionProgress !== undefined
        ? router.interactiveTransitionProgress
        : 0.0
    readonly property string direction: router && router.interactiveTransitionDirection !== undefined
        ? router.interactiveTransitionDirection
        : "none"
    readonly property string operation: router && router.interactiveTransitionOperation !== undefined
        ? router.interactiveTransitionOperation
        : "none"
    readonly property string fromPath: router && router.interactiveTransitionFromPath !== undefined
        ? router.interactiveTransitionFromPath
        : ""
    readonly property string toPath: router && router.interactiveTransitionToPath !== undefined
        ? router.interactiveTransitionToPath
        : ""
    readonly property var fromParams: router && router.interactiveTransitionFromParams !== undefined
        ? router.interactiveTransitionFromParams
        : ({})
    readonly property var toParams: router && router.interactiveTransitionToParams !== undefined
        ? router.interactiveTransitionToParams
        : ({})
    readonly property bool canCommit: router && router.interactiveTransitionCanCommit !== undefined
        ? router.interactiveTransitionCanCommit
        : false

    signal started(var state)
    signal updated(var state)
    signal committed(var state)
    signal cancelled(var state)
    signal rejected(string reason, var state)

    function canControl() {
        return router && router.beginInteractiveTransition !== undefined
    }

    function begin(spec) {
        if (!canControl())
            return false
        return router.beginInteractiveTransition(spec || ({}))
    }

    function beginBack(meta) {
        if (!router || router.beginInteractiveBack === undefined)
            return false
        return router.beginInteractiveBack(meta || ({}))
    }

    function beginPush(path, params, meta) {
        if (!router || router.beginInteractivePush === undefined)
            return false
        return router.beginInteractivePush(path, params || ({}), meta || ({}))
    }

    function beginReplace(path, params, meta) {
        if (!router || router.beginInteractiveReplace === undefined)
            return false
        return router.beginInteractiveReplace(path, params || ({}), meta || ({}))
    }

    function beginSetRoot(path, params, meta) {
        if (!router || router.beginInteractiveSetRoot === undefined)
            return false
        return router.beginInteractiveSetRoot(path, params || ({}), meta || ({}))
    }

    function beginPushComponent(component, params, meta) {
        if (!router || router.beginInteractivePushComponent === undefined)
            return false
        return router.beginInteractivePushComponent(component, params || ({}), meta || ({}))
    }

    function beginReplaceComponent(component, params, meta) {
        if (!router || router.beginInteractiveReplaceComponent === undefined)
            return false
        return router.beginInteractiveReplaceComponent(component, params || ({}), meta || ({}))
    }

    function beginSetRootComponent(component, params, meta) {
        if (!router || router.beginInteractiveSetRootComponent === undefined)
            return false
        return router.beginInteractiveSetRootComponent(component, params || ({}), meta || ({}))
    }

    function update(progressValue, details) {
        if (!router || router.updateInteractiveTransition === undefined)
            return false
        return router.updateInteractiveTransition(progressValue, details || ({}))
    }

    function finish(commit) {
        if (!router || router.finishInteractiveTransition === undefined)
            return false
        return router.finishInteractiveTransition(commit)
    }

    function cancel() {
        if (!router || router.cancelInteractiveTransition === undefined)
            return false
        return router.cancelInteractiveTransition()
    }

    function shouldCommit(progressValue, velocityX, velocityY) {
        if (!router || router.shouldCommitInteractiveTransition === undefined)
            return false
        return router.shouldCommitInteractiveTransition(progressValue, velocityX, velocityY)
    }

    property var routerSignalBridge: Connections {
        target: root.router
        ignoreUnknownSignals: true

        function onInteractiveTransitionStarted(state) {
            root.started(state)
        }

        function onInteractiveTransitionUpdated(state) {
            root.updated(state)
        }

        function onInteractiveTransitionCommitted(state) {
            root.committed(state)
        }

        function onInteractiveTransitionCancelled(state) {
            root.cancelled(state)
        }

        function onInteractiveTransitionRejected(reason, state) {
            root.rejected(reason, state)
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.PageTransitionController {
//     router: somePageRouter
// }
