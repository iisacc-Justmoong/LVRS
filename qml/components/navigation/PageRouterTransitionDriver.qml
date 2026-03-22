import QtQuick

QtObject {
    id: root

    property var router: null
    property var stackView: null
    property Item previewHost: null

    property bool active: false
    property real progress: 0.0
    property string direction: "none"
    property string operation: "none"
    property string fromPath: ""
    property string toPath: ""
    property var fromParams: ({})
    property var toParams: ({})
    property real velocityX: 0.0
    property real velocityY: 0.0
    property var meta: ({})
    property var previewItem: null
    property var currentItem: null
    property var secondaryItem: null
    property bool settling: false
    property bool pendingCommit: false
    property string pendingCommitMode: ""
    property string pendingCommitPath: ""
    property var pendingCommitParams: ({})
    property var pendingCommitComponent: null
    property bool commitApplying: false

    readonly property bool transitionLocked: active || settling
    readonly property bool canCommit: active && shouldCommitTransition(progress, velocityX, velocityY)

    onProgressChanged: {
        if (active)
            applyVisuals()
        if (transitionLocked)
            emitUpdated()
    }

    function cloneMap(source) {
        var clone = ({})
        if (!source || typeof source !== "object")
            return clone

        for (var key in source)
            clone[key] = source[key]
        return clone
    }

    function normalizedOperation(value) {
        var token = String(value || "").trim().toLowerCase()
        if (token === "push" || token === "replace" || token === "set" || token === "pop")
            return token
        return "push"
    }

    function normalizedDirection(value, nextOperation) {
        var token = String(value || "").trim().toLowerCase()
        if (token === "forward" || token === "backward")
            return token
        return nextOperation === "pop" ? "backward" : "forward"
    }

    function pathValueForEntry(entry) {
        if (!router || entry === undefined || entry === null)
            return ""
        if (typeof entry === "string")
            return router.normalizePath(entry)
        if (typeof entry === "object" && entry.path !== undefined && entry.path !== null && entry.path !== "")
            return router.normalizePath(entry.path)
        return ""
    }

    function paramsForEntry(entry) {
        if (typeof entry === "object" && entry !== null && entry.params !== undefined)
            return cloneMap(entry.params)
        return ({})
    }

    function interactiveTransitionsEnabled() {
        return router ? !!router.interactiveTransitionsEnabled : true
    }

    function settleDuration() {
        var value = router ? Number(router.interactiveTransitionSettleDuration) : 220
        return isFinite(value) ? Math.max(0, Math.round(value)) : 220
    }

    function commitProgressThreshold() {
        var value = router ? Number(router.interactiveTransitionCommitProgress) : 0.42
        if (!isFinite(value))
            return 0.42
        return Math.max(0.0, Math.min(1.0, value))
    }

    function velocityThreshold() {
        var value = router ? Number(router.interactiveTransitionVelocityThreshold) : 960
        return isFinite(value) ? Math.max(0, value) : 960
    }

    function outgoingParallaxFactor() {
        var value = router ? Number(router.interactiveTransitionOutgoingParallaxFactor) : 0.25
        return isFinite(value) ? Math.max(0.0, value) : 0.25
    }

    function incomingPreviewFactor() {
        var value = router ? Number(router.interactiveTransitionIncomingPreviewFactor) : 0.35
        return isFinite(value) ? Math.max(0.0, value) : 0.35
    }

    function currentState() {
        return {
            active: active,
            progress: progress,
            direction: direction,
            operation: operation,
            fromPath: fromPath,
            toPath: toPath,
            fromParams: cloneMap(fromParams),
            toParams: cloneMap(toParams),
            velocityX: velocityX,
            velocityY: velocityY,
            canCommit: canCommit,
            meta: cloneMap(meta),
            depth: router && router.depth !== undefined ? router.depth : 0,
            hasPreviewItem: previewItem !== null
        }
    }

    function emitStarted() {
        if (router)
            router.interactiveTransitionStarted(currentState())
    }

    function emitUpdated() {
        if (router)
            router.interactiveTransitionUpdated(currentState())
    }

    function emitCommitted(state) {
        if (router)
            router.interactiveTransitionCommitted(state)
    }

    function emitCancelled(state) {
        if (router)
            router.interactiveTransitionCancelled(state)
    }

    function emitRejected(reason) {
        if (router)
            router.interactiveTransitionRejected(reason, currentState())
    }

    function applyVisuals() {
        if (!active || !stackView)
            return

        var nextProgress = Math.max(0, Math.min(1, progress))
        var viewportWidth = Math.max(1, stackView.width > 0 ? stackView.width : (router ? router.width : 1))

        if (direction === "backward") {
            if (secondaryItem) {
                if (secondaryItem.visible !== undefined)
                    secondaryItem.visible = true
                if (secondaryItem.opacity !== undefined)
                    secondaryItem.opacity = 1.0
                if (secondaryItem.enabled !== undefined)
                    secondaryItem.enabled = false
                if (secondaryItem.x !== undefined)
                    secondaryItem.x = -viewportWidth * incomingPreviewFactor() * (1.0 - nextProgress)
            }
            if (currentItem && currentItem.x !== undefined)
                currentItem.x = viewportWidth * nextProgress
            return
        }

        if (currentItem) {
            if (currentItem.visible !== undefined)
                currentItem.visible = true
            if (currentItem.opacity !== undefined)
                currentItem.opacity = 1.0
            if (currentItem.enabled !== undefined)
                currentItem.enabled = false
            if (currentItem.x !== undefined)
                currentItem.x = -viewportWidth * outgoingParallaxFactor() * nextProgress
        }

        if (secondaryItem) {
            if (secondaryItem.visible !== undefined)
                secondaryItem.visible = true
            if (secondaryItem.opacity !== undefined)
                secondaryItem.opacity = 1.0
            if (secondaryItem.enabled !== undefined)
                secondaryItem.enabled = false
            if (secondaryItem.x !== undefined)
                secondaryItem.x = viewportWidth * (1.0 - nextProgress)
        }
    }

    function stopSettleAnimation() {
        if (!settleAnimation.running)
            return

        settling = false
        settleAnimation.stop()
    }

    function resetState() {
        active = false
        progress = 0.0
        direction = "none"
        operation = "none"
        fromPath = ""
        toPath = ""
        fromParams = ({})
        toParams = ({})
        velocityX = 0.0
        velocityY = 0.0
        meta = ({})
        previewItem = null
        currentItem = null
        secondaryItem = null
        settling = false
        pendingCommit = false
        pendingCommitMode = ""
        pendingCommitPath = ""
        pendingCommitParams = ({})
        pendingCommitComponent = null
    }

    function clearPreviewItem() {
        if (!previewItem)
            return

        var item = previewItem
        previewItem = null
        if (item.destroy !== undefined)
            item.destroy()
    }

    function abortTransition() {
        if (!transitionLocked)
            return false

        var snapshot = currentState()
        stopSettleAnimation()

        if (currentItem && currentItem.x !== undefined)
            currentItem.x = 0
        if (secondaryItem && secondaryItem.x !== undefined)
            secondaryItem.x = 0

        clearPreviewItem()
        resetState()
        if (router)
            router.scheduleActivePagePresentationSync()
        emitCancelled(snapshot)
        return true
    }

    function resolvePathTarget(pathValue, params) {
        if (!router)
            return null

        var resolved = router.resolveRoute(pathValue)
        var targetParams = ({})
        if (resolved && resolved.params) {
            for (var resolvedKey in resolved.params)
                targetParams[resolvedKey] = resolved.params[resolvedKey]
        }
        if (params && typeof params === "object") {
            for (var paramKey in params)
                targetParams[paramKey] = params[paramKey]
        }

        var normalized = router.normalizePath(pathValue)
        if (!resolved) {
            if (!router.notFoundComponent && !router.notFoundSource)
                return null
            return {
                mode: "path",
                path: normalized,
                params: targetParams,
                target: router.notFoundComponent ? router.notFoundComponent : router.notFoundSource
            }
        }

        var route = resolved.route
        var target = route.component ? route.component : route.source
        if (!target)
            return null

        return {
            mode: "path",
            path: normalized,
            params: targetParams,
            target: target
        }
    }

    function resolveComponentTarget(component, params) {
        if (!component)
            return null

        return {
            mode: "component",
            path: "",
            params: cloneMap(params),
            target: component
        }
    }

    function createPreviewItem(target, params) {
        if (!target || !previewHost)
            return null

        var targetParams = params !== undefined ? params : ({})
        var item = null

        if (target.createObject !== undefined) {
            item = target.createObject(previewHost, targetParams)
        } else {
            var dynamicComponent = Qt.createComponent(target)
            if (!dynamicComponent)
                return null
            if (dynamicComponent.status === Component.Error) {
                console.warn("PageRouter interactive preview failed:", dynamicComponent.errorString())
                return null
            }
            if (dynamicComponent.status !== Component.Ready)
                return null
            item = dynamicComponent.createObject(previewHost, targetParams)
        }

        if (!item)
            return null

        if (item.parent !== previewHost)
            item.parent = previewHost
        if (item.visible !== undefined)
            item.visible = true
        if (item.enabled !== undefined)
            item.enabled = false

        if (router) {
            router.applyPageViewportContract(item)
            router.applySingleChildViewportContract(item)
        }
        return item
    }

    function rejectTransition(reason) {
        emitRejected(reason)
        return false
    }

    function beginTransition(spec) {
        if (!interactiveTransitionsEnabled())
            return rejectTransition("interactiveTransitionsDisabled")

        if (transitionLocked)
            return rejectTransition("interactiveTransitionAlreadyActive")

        if (!router || !stackView)
            return rejectTransition("interactiveTransitionContextMissing")

        var request = spec && typeof spec === "object" ? spec : ({})
        var nextOperation = normalizedOperation(request.operation)
        var nextDirection = normalizedDirection(request.direction, nextOperation)
        var targetSpec = null

        if (nextOperation === "pop") {
            if (stackView.depth < 2 || !router.path || router.path.length < 2)
                return rejectTransition("interactivePopUnavailable")

            targetSpec = {
                mode: "pop",
                path: pathValueForEntry(router.path[router.path.length - 2]),
                params: paramsForEntry(router.path[router.path.length - 2]),
                target: stackView.get(stackView.depth - 2)
            }
            if (!targetSpec.target)
                return rejectTransition("interactivePopTargetMissing")
        } else if (request.component !== undefined && request.component !== null) {
            targetSpec = resolveComponentTarget(request.component,
                                               request.params !== undefined ? request.params : ({}))
        } else if (request.path !== undefined && request.path !== null && request.path !== "") {
            targetSpec = resolvePathTarget(request.path,
                                           request.params !== undefined ? request.params : ({}))
        } else {
            return rejectTransition("interactiveTargetMissing")
        }

        if (!targetSpec)
            return rejectTransition("interactiveTargetResolveFailed")

        stopSettleAnimation()

        currentItem = stackView.currentItem
        secondaryItem = null
        previewItem = null

        if (nextOperation === "pop") {
            secondaryItem = targetSpec.target
        } else {
            var createdPreview = createPreviewItem(targetSpec.target, targetSpec.params)
            if (!createdPreview)
                return rejectTransition("interactivePreviewCreateFailed")
            previewItem = createdPreview
            secondaryItem = createdPreview
        }

        active = true
        progress = 0.0
        direction = nextDirection
        operation = nextOperation
        fromPath = router.currentPath
        toPath = targetSpec.path
        fromParams = cloneMap(router.currentParams)
        toParams = cloneMap(targetSpec.params)
        velocityX = 0.0
        velocityY = 0.0
        meta = cloneMap(request.meta)
        pendingCommit = false
        pendingCommitMode = targetSpec.mode
        pendingCommitPath = targetSpec.path
        pendingCommitParams = cloneMap(targetSpec.params)
        pendingCommitComponent = targetSpec.mode === "component" ? targetSpec.target : null

        applyVisuals()
        emitStarted()
        return true
    }

    function beginBack(metaValue) {
        return beginTransition({
            operation: "pop",
            direction: "backward",
            meta: metaValue !== undefined ? metaValue : ({})
        })
    }

    function beginPush(pathValue, params, metaValue) {
        return beginTransition({
            operation: "push",
            direction: "forward",
            path: pathValue,
            params: params !== undefined ? params : ({}),
            meta: metaValue !== undefined ? metaValue : ({})
        })
    }

    function beginReplace(pathValue, params, metaValue) {
        return beginTransition({
            operation: "replace",
            direction: "forward",
            path: pathValue,
            params: params !== undefined ? params : ({}),
            meta: metaValue !== undefined ? metaValue : ({})
        })
    }

    function beginSetRoot(pathValue, params, metaValue) {
        return beginTransition({
            operation: "set",
            direction: "forward",
            path: pathValue,
            params: params !== undefined ? params : ({}),
            meta: metaValue !== undefined ? metaValue : ({})
        })
    }

    function beginPushComponent(component, params, metaValue) {
        return beginTransition({
            operation: "push",
            direction: "forward",
            component: component,
            params: params !== undefined ? params : ({}),
            meta: metaValue !== undefined ? metaValue : ({})
        })
    }

    function beginReplaceComponent(component, params, metaValue) {
        return beginTransition({
            operation: "replace",
            direction: "forward",
            component: component,
            params: params !== undefined ? params : ({}),
            meta: metaValue !== undefined ? metaValue : ({})
        })
    }

    function beginSetRootComponent(component, params, metaValue) {
        return beginTransition({
            operation: "set",
            direction: "forward",
            component: component,
            params: params !== undefined ? params : ({}),
            meta: metaValue !== undefined ? metaValue : ({})
        })
    }

    function updateTransition(progressValue, details) {
        if (!active)
            return false

        stopSettleAnimation()

        var nextProgress = Number(progressValue)
        if (!isFinite(nextProgress))
            nextProgress = progress
        progress = Math.max(0.0, Math.min(1.0, nextProgress))

        var detailMap = details && typeof details === "object" ? details : ({})
        var nextVelocityX = Number(detailMap.velocityX)
        var nextVelocityY = Number(detailMap.velocityY)
        if (isFinite(nextVelocityX))
            velocityX = nextVelocityX
        if (isFinite(nextVelocityY))
            velocityY = nextVelocityY
        if (detailMap.meta !== undefined)
            meta = cloneMap(detailMap.meta)
        return true
    }

    function shouldCommitTransition(progressValue, velocityXValue, velocityYValue) {
        if (!transitionLocked)
            return false

        var nextProgress = Number(progressValue)
        if (!isFinite(nextProgress))
            nextProgress = progress
        nextProgress = Math.max(0.0, Math.min(1.0, nextProgress))

        var nextVelocityX = Number(velocityXValue)
        if (!isFinite(nextVelocityX))
            nextVelocityX = velocityX

        var nextVelocityY = Number(velocityYValue)
        if (!isFinite(nextVelocityY))
            nextVelocityY = velocityY

        var commitVelocity = Math.abs(nextVelocityX) >= Math.abs(nextVelocityY) ? nextVelocityX : nextVelocityY
        if (direction === "forward")
            commitVelocity = -nextVelocityX
        else if (direction === "backward")
            commitVelocity = nextVelocityX
        else
            commitVelocity = Math.abs(commitVelocity)

        return nextProgress >= commitProgressThreshold()
            || commitVelocity >= velocityThreshold()
    }

    function finalizeTransition(commit) {
        var snapshot = currentState()

        if (secondaryItem && secondaryItem.x !== undefined)
            secondaryItem.x = 0
        if (operation !== "pop" && currentItem && currentItem.x !== undefined)
            currentItem.x = 0

        if (commit) {
            commitApplying = true
            if (operation === "pop") {
                resetState()
                router.pop()
            } else if (pendingCommitMode === "component") {
                var componentMode = operation === "replace"
                    ? "replace"
                    : (operation === "set" ? "set" : "push")
                router.navigateComponent(pendingCommitComponent,
                                         cloneMap(pendingCommitParams),
                                         componentMode)
                clearPreviewItem()
                resetState()
            } else {
                var pathMode = operation === "replace"
                    ? "replace"
                    : (operation === "set" ? "set" : "push")
                router.navigate(pendingCommitPath,
                                cloneMap(pendingCommitParams),
                                pathMode)
                clearPreviewItem()
                resetState()
            }
            commitApplying = false
            emitCommitted(snapshot)
            return
        }

        if (currentItem && currentItem.x !== undefined)
            currentItem.x = 0
        if (secondaryItem && secondaryItem.x !== undefined)
            secondaryItem.x = 0
        clearPreviewItem()
        resetState()
        if (router)
            router.scheduleActivePagePresentationSync()
        emitCancelled(snapshot)
    }

    function finishTransition(commitValue) {
        if (!active)
            return false

        stopSettleAnimation()

        var shouldCommit = commitValue
        if (shouldCommit === undefined)
            shouldCommit = shouldCommitTransition(progress, velocityX, velocityY)
        pendingCommit = !!shouldCommit

        var targetProgress = pendingCommit ? 1.0 : 0.0
        if (settleDuration() <= 0 || Math.abs(targetProgress - progress) < 0.0001) {
            progress = targetProgress
            finalizeTransition(pendingCommit)
            return true
        }

        settling = true
        settleAnimation.from = progress
        settleAnimation.to = targetProgress
        settleAnimation.duration = Math.max(
            1,
            Math.round(settleDuration() * Math.max(0.25, Math.abs(targetProgress - progress)))
        )
        settleAnimation.restart()
        return true
    }

    function cancelTransition() {
        return finishTransition(false)
    }

    property var routerResizeBridge: Connections {
        target: root.router
        ignoreUnknownSignals: true

        function onWidthChanged() {
            if (root.active)
                root.applyVisuals()
        }

        function onHeightChanged() {
            if (root.active)
                root.applyVisuals()
        }
    }

    property var settleAnimation: NumberAnimation {
        target: root
        property: "progress"
        easing.type: Easing.OutCubic
        onRunningChanged: {
            if (!running && root.settling) {
                var commit = root.pendingCommit
                root.settling = false
                root.finalizeTransition(commit)
            }
        }
    }
}
