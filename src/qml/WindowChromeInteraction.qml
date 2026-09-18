import QtQuick
import QtQuick.Window
import LVRS 1.0

Item {
    id: root

    required property var targetWindow
    property bool interactionEnabled: true
    property bool moveHandleEnabled: true
    property real moveHandleHeight: Theme.scaleMetric(28)
    property real moveHandleTopMargin: 0
    property real moveHandleLeftMargin: 0
    property real moveHandleRightMargin: 0
    property var moveExclusionItems: []
    property bool resizeHandlesEnabled: true
    property int resizeEdges: Qt.LeftEdge | Qt.TopEdge | Qt.RightEdge | Qt.BottomEdge
    property real resizeBorderThickness: Theme.scaleMetric(6)
    property real resizeCornerSize: Theme.scaleMetric(12)

    readonly property alias moveHandleItem: moveHandle
    readonly property real effectiveResizeBorderThickness: Math.max(0,
                                                                    Math.min(resizeBorderThickness,
                                                                             width * 0.5,
                                                                             height * 0.5))
    readonly property real effectiveResizeCornerSize: Math.max(effectiveResizeBorderThickness,
                                                                Math.min(resizeCornerSize,
                                                                         width * 0.5,
                                                                         height * 0.5))
    readonly property bool moveVisibilityEnabled: targetWindow !== null
        && targetWindow.visible
        && targetWindow.visibility !== Window.FullScreen
        && targetWindow.visibility !== Window.Minimized
    readonly property bool resizeVisibilityEnabled: targetWindow !== null
        && targetWindow.visible
        && targetWindow.visibility === Window.Windowed

    signal moveAttempted(bool started)
    signal resizeAttempted(int edges, bool started)

    objectName: "windowChromeInteractionLayer"

    function isResizeEdgeEnabled(edge) {
        return (resizeEdges & edge) === edge
    }

    function movePointIsExcluded(x, y) {
        const exclusions = moveExclusionItems || []
        for (let index = 0; index < exclusions.length; ++index) {
            const item = exclusions[index]
            if (!item || !item.visible || item.width <= 0 || item.height <= 0)
                continue

            const localPoint = item.mapFromItem(root, x, y)
            if (localPoint.x >= 0 && localPoint.y >= 0
                    && localPoint.x < item.width && localPoint.y < item.height)
                return true
        }
        return false
    }

    function requestMove() {
        if (!interactionEnabled) {
            moveAttempted(false)
            return false
        }

        const started = NativeWindowInteraction.requestSystemMove(targetWindow)
        moveAttempted(started)
        return started
    }

    function requestResize(edges, globalPosition) {
        if (!interactionEnabled) {
            resizeAttempted(edges, false)
            return false
        }

        const started = NativeWindowInteraction.requestSystemResizeAt(targetWindow,
                                                                       edges,
                                                                       globalPosition)
        resizeAttempted(edges, started)
        return started
    }

    function handleMovePress(mouse) {
        if (mouse.button !== Qt.LeftButton)
            return

        const rootPoint = moveHandle.mapToItem(root, mouse.x, mouse.y)
        if (movePointIsExcluded(rootPoint.x, rootPoint.y) || !requestMove())
            mouse.accepted = false
    }

    function handleResizePress(mouse, edges, resizeHandle) {
        if (mouse.button !== Qt.LeftButton)
            return
        const globalPosition = resizeHandle.mapToGlobal(mouse.x, mouse.y)
        if (!requestResize(edges, globalPosition))
            mouse.accepted = false
    }

    MouseArea {
        id: moveHandle

        objectName: "windowDragHandle"
        x: root.moveHandleLeftMargin
        y: root.moveHandleTopMargin
        width: Math.max(0, root.width - root.moveHandleLeftMargin - root.moveHandleRightMargin)
        height: Math.max(0, root.moveHandleHeight)
        visible: root.interactionEnabled && root.moveHandleEnabled && root.moveVisibilityEnabled
        enabled: visible
        acceptedButtons: Qt.LeftButton

        onPressed: function (mouse) {
            root.handleMovePress(mouse)
        }
    }

    MouseArea {
        id: leftResizeHandle

        objectName: "windowResizeLeftHandle"
        z: 1
        x: 0
        y: root.effectiveResizeCornerSize
        width: root.effectiveResizeBorderThickness
        height: Math.max(0, root.height - (2 * root.effectiveResizeCornerSize))
        visible: root.interactionEnabled && root.resizeHandlesEnabled
            && root.resizeVisibilityEnabled && root.isResizeEdgeEnabled(Qt.LeftEdge)
        enabled: visible
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeHorCursor

        onPressed: function (mouse) {
            root.handleResizePress(mouse, Qt.LeftEdge, leftResizeHandle)
        }
    }

    MouseArea {
        id: topResizeHandle

        objectName: "windowResizeTopHandle"
        z: 1
        x: root.effectiveResizeCornerSize
        y: 0
        width: Math.max(0, root.width - (2 * root.effectiveResizeCornerSize))
        height: root.effectiveResizeBorderThickness
        visible: root.interactionEnabled && root.resizeHandlesEnabled
            && root.resizeVisibilityEnabled && root.isResizeEdgeEnabled(Qt.TopEdge)
        enabled: visible
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeVerCursor

        onPressed: function (mouse) {
            root.handleResizePress(mouse, Qt.TopEdge, topResizeHandle)
        }
    }

    MouseArea {
        id: rightResizeHandle

        objectName: "windowResizeRightHandle"
        z: 1
        x: Math.max(0, root.width - width)
        y: root.effectiveResizeCornerSize
        width: root.effectiveResizeBorderThickness
        height: Math.max(0, root.height - (2 * root.effectiveResizeCornerSize))
        visible: root.interactionEnabled && root.resizeHandlesEnabled
            && root.resizeVisibilityEnabled && root.isResizeEdgeEnabled(Qt.RightEdge)
        enabled: visible
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeHorCursor

        onPressed: function (mouse) {
            root.handleResizePress(mouse, Qt.RightEdge, rightResizeHandle)
        }
    }

    MouseArea {
        id: bottomResizeHandle

        objectName: "windowResizeBottomHandle"
        z: 1
        x: root.effectiveResizeCornerSize
        y: Math.max(0, root.height - height)
        width: Math.max(0, root.width - (2 * root.effectiveResizeCornerSize))
        height: root.effectiveResizeBorderThickness
        visible: root.interactionEnabled && root.resizeHandlesEnabled
            && root.resizeVisibilityEnabled && root.isResizeEdgeEnabled(Qt.BottomEdge)
        enabled: visible
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeVerCursor

        onPressed: function (mouse) {
            root.handleResizePress(mouse, Qt.BottomEdge, bottomResizeHandle)
        }
    }

    MouseArea {
        id: topLeftResizeHandle

        objectName: "windowResizeTopLeftHandle"
        z: 2
        x: 0
        y: 0
        width: root.effectiveResizeCornerSize
        height: root.effectiveResizeCornerSize
        visible: root.interactionEnabled && root.resizeHandlesEnabled
            && root.resizeVisibilityEnabled && root.isResizeEdgeEnabled(Qt.LeftEdge)
            && root.isResizeEdgeEnabled(Qt.TopEdge)
        enabled: visible
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeFDiagCursor

        onPressed: function (mouse) {
            root.handleResizePress(mouse,
                                   Qt.LeftEdge | Qt.TopEdge,
                                   topLeftResizeHandle)
        }
    }

    MouseArea {
        id: topRightResizeHandle

        objectName: "windowResizeTopRightHandle"
        z: 2
        x: Math.max(0, root.width - width)
        y: 0
        width: root.effectiveResizeCornerSize
        height: root.effectiveResizeCornerSize
        visible: root.interactionEnabled && root.resizeHandlesEnabled
            && root.resizeVisibilityEnabled && root.isResizeEdgeEnabled(Qt.RightEdge)
            && root.isResizeEdgeEnabled(Qt.TopEdge)
        enabled: visible
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeBDiagCursor

        onPressed: function (mouse) {
            root.handleResizePress(mouse,
                                   Qt.RightEdge | Qt.TopEdge,
                                   topRightResizeHandle)
        }
    }

    MouseArea {
        id: bottomLeftResizeHandle

        objectName: "windowResizeBottomLeftHandle"
        z: 2
        x: 0
        y: Math.max(0, root.height - height)
        width: root.effectiveResizeCornerSize
        height: root.effectiveResizeCornerSize
        visible: root.interactionEnabled && root.resizeHandlesEnabled
            && root.resizeVisibilityEnabled && root.isResizeEdgeEnabled(Qt.LeftEdge)
            && root.isResizeEdgeEnabled(Qt.BottomEdge)
        enabled: visible
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeBDiagCursor

        onPressed: function (mouse) {
            root.handleResizePress(mouse,
                                   Qt.LeftEdge | Qt.BottomEdge,
                                   bottomLeftResizeHandle)
        }
    }

    MouseArea {
        id: bottomRightResizeHandle

        objectName: "windowResizeBottomRightHandle"
        z: 2
        x: Math.max(0, root.width - width)
        y: Math.max(0, root.height - height)
        width: root.effectiveResizeCornerSize
        height: root.effectiveResizeCornerSize
        visible: root.interactionEnabled && root.resizeHandlesEnabled
            && root.resizeVisibilityEnabled && root.isResizeEdgeEnabled(Qt.RightEdge)
            && root.isResizeEdgeEnabled(Qt.BottomEdge)
        enabled: visible
        acceptedButtons: Qt.LeftButton
        cursorShape: Qt.SizeFDiagCursor

        onPressed: function (mouse) {
            root.handleResizePress(mouse,
                                   Qt.RightEdge | Qt.BottomEdge,
                                   bottomRightResizeHandle)
        }
    }
}

// API usage (external):
// WindowChromeInteraction { anchors.fill: parent; targetWindow: appWindow }
