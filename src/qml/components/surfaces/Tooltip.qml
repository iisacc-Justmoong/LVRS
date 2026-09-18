import QtQuick
import QtQuick.Controls as Controls
import LVRS 1.0

Controls.ToolTip {
    id: root
    property bool motionEnabled: true

    // Avoid Popup's inherited transform-origin constants named Left and Right.
    enum Placement { Automatic, Above, Below, LeftSide, RightSide }

    property Item target: null
    // Target-local coordinates, or overlay coordinates when target is null.
    property point anchorPoint: target ? Qt.point(target.width / 2, target.height / 2) : Qt.point(0, 0)
    property Component contentComponent: null
    default property alias content: inlineContent.data
    readonly property alias loadedContent: contentLoader.item
    property bool automatic: true
    property bool scrollContent: true
    property int preferredPlacement: Tooltip.Automatic
    property real maximumWidth: 320
    property real maximumHeight: 320
    property real contentPadding: Theme.gap12
    property real edgeMargin: Theme.gap8
    property real cornerRadius: Theme.radiusLg
    property real tailLength: 10
    property real tailWidth: 16
    property int hideDelay: 120
    property int animationDuration: Motion.surfaceDuration
    property color surfaceColor: Theme.materialTint
    property real surfaceOpacity: Theme.materialGlassOpacity
    property color borderColor: Theme.materialGlassEdge
    property color primaryColor: Theme.primary
    property Item backdropSource: Controls.ApplicationWindow.contentItem
    property Item backdropBackground: Controls.ApplicationWindow.window
        ? Controls.ApplicationWindow.window.background : null
    property rect availableRect: Qt.rect(safeArea.leftInset, safeArea.topInset,
        Math.max(0, (parent ? parent.width : 0) - safeArea.leftInset - safeArea.rightInset),
        Math.max(0, (parent ? parent.height : 0) - safeArea.topInset - safeArea.bottomInset))

    readonly property int resolvedPlacement: _layout.side
    readonly property point resolvedAnchorPoint: _anchor
    readonly property rect bodyRect: Qt.rect(_layout.bx - x, _layout.by - y, _layout.bw, _layout.bh)
    readonly property point tailPoint: Qt.point(_anchor.x - x, _anchor.y - y)
    readonly property real availableContentWidth: viewport.width
    readonly property real availableContentHeight: viewport.height

    readonly property WindowSafeAreaObserver safeArea: WindowSafeAreaObserver {
        window: root.parent ? root.parent.Window.window : null
    }
    property point _anchor: Qt.point(0, 0)
    property rect _bounds: availableRect
    property bool _autoLatched: false
    readonly property bool _targetReady: target && target.visible && target.enabled && target.Window.window
    readonly property bool _focusWanted: _targetReady
        && (target.visualFocus !== undefined ? target.visualFocus : target.activeFocus)
    readonly property bool _autoWanted: _targetReady
        && (triggerHover.hovered || triggerPress.pressed || _focusWanted)
    readonly property real _naturalWidth: contentComponent !== null
        ? (loadedContent ? nonnegative(loadedContent.implicitWidth) : 0)
        : (inlineContent.children.length > 0 ? inlineContent.implicitWidth : textLabel.implicitWidth)
    readonly property real _naturalHeight: contentComponent !== null
        ? (loadedContent ? nonnegative(loadedContent.implicitHeight) : 0)
        : (inlineContent.children.length > 0 ? inlineContent.implicitHeight : textLabel.implicitHeight)
    property var _layout: ({ side: Tooltip.Above, bx: 0, by: 0, bw: 0, bh: 0, x: 0, y: 0, width: 0, height: 0 })

    function nonnegative(value) { return isFinite(value) ? Math.max(0, value) : 0 }
    function clamp(value, low, high) { return Math.max(low, Math.min(high, value)) }

    function refreshAnchor() {
        if (!parent)
            return;
        const point = target ? target.mapToItem(parent, anchorPoint.x, anchorPoint.y) : anchorPoint;
        if (isFinite(point.x) && isFinite(point.y))
            _anchor = point;
        let bounds = availableRect;
        // The overlay is also clipped to its current physical display when a
        // desktop window is partly off-screen. All coordinates remain logical.
        const window = parent.Window.window;
        const screen = window ? window.screen : null;
        if (screen && isFinite(screen.virtualX) && isFinite(screen.virtualY)
                && screen.width > 0 && screen.height > 0) {
            const topLeft = parent.mapFromGlobal(screen.virtualX, screen.virtualY);
            const bottomRight = parent.mapFromGlobal(screen.virtualX + screen.width, screen.virtualY + screen.height);
            const left = Math.max(bounds.x, topLeft.x);
            const top = Math.max(bounds.y, topLeft.y);
            bounds = Qt.rect(left, top, Math.max(0, Math.min(bounds.x + bounds.width, bottomRight.x) - left),
                             Math.max(0, Math.min(bounds.y + bounds.height, bottomRight.y) - top));
        }
        _bounds = bounds;
        let clipped = false;
        for (let ancestor = target; ancestor; ancestor = ancestor.parent) {
            if (!ancestor.clip)
                continue;
            const local = target.mapToItem(ancestor, anchorPoint.x, anchorPoint.y);
            if (local.x < 0 || local.y < 0 || local.x > ancestor.width || local.y > ancestor.height) {
                clipped = true;
                break;
            }
        }
        if (visible && (clipped || (!isFinite(point.x) || !isFinite(point.y)) || (target && !_targetReady)
                || point.x < bounds.x || point.y < bounds.y
                || point.x > bounds.x + bounds.width || point.y > bounds.y + bounds.height))
            close();
        updatePlacement();
    }

    function updatePlacement() {
        const next = placeBubble();
        for (const key in next) {
            if (next[key] !== _layout[key]) {
                _layout = next;
                break;
            }
        }
    }

    function placeBubble() {
        const margin = nonnegative(edgeMargin);
        const left = _bounds.x + margin, top = _bounds.y + margin;
        const right = Math.max(left, _bounds.x + _bounds.width - margin);
        const bottom = Math.max(top, _bounds.y + _bounds.height - margin);
        const padding = nonnegative(contentPadding);
        const desiredWidth = Math.min(nonnegative(maximumWidth), Math.max(1, _naturalWidth + padding * 2));
        const desiredHeight = Math.min(nonnegative(maximumHeight), Math.max(1, _naturalHeight + padding * 2));
        const length = nonnegative(tailLength);
        const order = [Tooltip.Above, Tooltip.Below, Tooltip.RightSide, Tooltip.LeftSide];
        if (preferredPlacement >= Tooltip.Above && preferredPlacement <= Tooltip.RightSide) {
            order.splice(order.indexOf(preferredPlacement), 1);
            order.unshift(preferredPlacement);
        }
        let best = null, bestArea = -1;
        for (let i = 0; i < order.length; ++i) {
            const side = order[i];
            let l = left, t = top, r = right, b = bottom;
            if (side === Tooltip.Above) b = Math.min(b, _anchor.y - length);
            if (side === Tooltip.Below) t = Math.max(t, _anchor.y + length);
            if (side === Tooltip.LeftSide) r = Math.min(r, _anchor.x - length);
            if (side === Tooltip.RightSide) l = Math.max(l, _anchor.x + length);
            const w = Math.max(0, Math.min(desiredWidth, r - l));
            const h = Math.max(0, Math.min(desiredHeight, b - t));
            const bx = side === Tooltip.LeftSide ? r - w : side === Tooltip.RightSide ? l : clamp(_anchor.x - w / 2, l, r - w);
            const by = side === Tooltip.Above ? b - h : side === Tooltip.Below ? t : clamp(_anchor.y - h / 2, t, b - h);
            const candidate = { side: side, bx: bx, by: by, bw: w, bh: h };
            if (w >= desiredWidth && h >= desiredHeight) { best = candidate; break; }
            if (w * h > bestArea) { best = candidate; bestArea = w * h; }
        }
        best.x = Math.min(best.bx, _anchor.x);
        best.y = Math.min(best.by, _anchor.y);
        best.width = Math.max(best.bx + best.bw, _anchor.x) - best.x;
        best.height = Math.max(best.by + best.bh, _anchor.y) - best.y;
        return best;
    }

    function bubbleOutline() {
        const b = bodyRect, p = tailPoint;
        const l = b.x, t = b.y, r = l + b.width, d = t + b.height;
        const radius = Math.min(nonnegative(cornerRadius), b.width / 2, b.height / 2);
        const hx = Math.min(nonnegative(tailWidth) / 2, Math.max(0, b.width / 2 - radius));
        const hy = Math.min(nonnegative(tailWidth) / 2, Math.max(0, b.height / 2 - radius));
        const cx = clamp(p.x, l + radius + hx, r - radius - hx);
        const cy = clamp(p.y, t + radius + hy, d - radius - hy);
        let path = "M " + (l + radius) + " " + t;
        if (resolvedPlacement === Tooltip.Below)
            path += " L " + (cx - hx) + " " + t + " L " + p.x + " " + p.y + " L " + (cx + hx) + " " + t;
        path += " L " + (r - radius) + " " + t + " Q " + r + " " + t + " " + r + " " + (t + radius);
        if (resolvedPlacement === Tooltip.LeftSide)
            path += " L " + r + " " + (cy - hy) + " L " + p.x + " " + p.y + " L " + r + " " + (cy + hy);
        path += " L " + r + " " + (d - radius) + " Q " + r + " " + d + " " + (r - radius) + " " + d;
        if (resolvedPlacement === Tooltip.Above)
            path += " L " + (cx + hx) + " " + d + " L " + p.x + " " + p.y + " L " + (cx - hx) + " " + d;
        path += " L " + (l + radius) + " " + d + " Q " + l + " " + d + " " + l + " " + (d - radius);
        if (resolvedPlacement === Tooltip.RightSide)
            path += " L " + l + " " + (cy + hy) + " L " + p.x + " " + p.y + " L " + l + " " + (cy - hy);
        return path + " L " + l + " " + (t + radius) + " Q " + l + " " + t + " " + (l + radius) + " " + t + " Z";
    }

    function syncAutomatic() {
        if (!automatic)
            return;
        if (_autoWanted || (visible && bubbleHover.hovered)) {
            hideTimer.stop();
            if (_autoWanted && !_autoLatched) {
                _autoLatched = true;
                refreshAnchor();
                open();
            }
        } else {
            _autoLatched = false;
            if (!visible) close(); // Cancel a pending ToolTip delay immediately.
            else hideTimer.restart();
        }
    }

    function openAt(item, point, component) {
        target = item;
        anchorPoint = point;
        if (component !== undefined)
            contentComponent = component;
        refreshAnchor();
        open();
    }

    // Resolve through the target so a lazily loaded gallery/host can acquire its
    // window without a parent <-> Overlay attached-property binding cycle.
    parent: target && target.Window.window
        ? (target.Controls.ApplicationWindow.window
            ? target.Controls.Overlay.overlay : target.Window.window.contentItem)
        : Controls.Overlay.overlay
    popupType: Controls.Popup.Item
    modal: false
    dim: false
    focus: false
    padding: 0
    margins: -1
    delay: 500
    timeout: 5000
    transformOrigin: resolvedPlacement === Tooltip.Above ? Controls.Popup.Bottom
        : resolvedPlacement === Tooltip.Below ? Controls.Popup.Top
        : resolvedPlacement === Tooltip.LeftSide ? Controls.Popup.Right : Controls.Popup.Left
    closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutside
    x: _layout.x
    y: _layout.y
    width: _layout.width
    height: _layout.height

    onAboutToShow: { refreshAnchor(); viewport.contentY = 0; }
    onAnchorPointChanged: refreshAnchor()
    onAvailableRectChanged: refreshAnchor()
    // Wrapped content depends on the assigned width. Resolve its new height
    // after layout, rather than feeding it back into the same QML binding.
    on_NaturalWidthChanged: Qt.callLater(root.refreshAnchor)
    on_NaturalHeightChanged: Qt.callLater(root.refreshAnchor)
    onTargetChanged: { close(); _autoLatched = false; refreshAnchor(); syncAutomatic(); }
    on_AutoWantedChanged: syncAutomatic()
    onAutomaticChanged: { _autoLatched = false; if (!automatic) close(); else syncAutomatic(); }

    readonly property FrameAnimation anchorTracker: FrameAnimation {
        running: root.visible
        onTriggered: root.refreshAnchor()
    }
    readonly property Timer hideTimer: Timer {
        interval: Math.max(0, root.hideDelay)
        onTriggered: root.close()
    }
    readonly property Item triggerSurface: Item {
        parent: root.target
        anchors.fill: parent
        visible: root.automatic && root._targetReady
        HoverHandler { id: triggerHover; onHoveredChanged: root.syncAutomatic() }
        TapHandler {
            id: triggerPress
            gesturePolicy: TapHandler.DragThreshold
            onPressedChanged: {
                if (pressed && root.visible) root.close();
                else root.syncAutomatic();
            }
        }
    }
    readonly property Shortcut escapeShortcut: Shortcut {
        sequence: "Escape"
        enabled: root.visible
        context: Qt.WindowShortcut
        onActivated: root.close()
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: root.motionEnabled ? Motion.duration(Math.min(root.animationDuration, Motion.hoverDuration)) : 0 }
            NumberAnimation {
                property: "scale"; from: Motion.animated && root.motionEnabled && root.animationDuration > 0 ? 0.94 : 1; to: 1
                duration: root.motionEnabled ? Motion.duration(root.animationDuration) : 0
                easing.type: Easing.OutBack; easing.overshoot: Motion.overshoot
            }
        }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; to: 0; duration: root.motionEnabled ? Motion.duration(Math.min(root.animationDuration, Motion.exitDuration)) : 0 }
    }
    background: PanelMaterial {
        objectName: "tooltip_surface"
        color: root.surfaceColor
        tintOpacity: root.surfaceOpacity
        primaryColor: root.primaryColor
        borderColor: root.borderColor
        radius: root.cornerRadius
        outlinePath: root.bubbleOutline()
        backdropSource: root.backdropSource
        backdropBackground: root.backdropBackground
    }
    contentItem: Item {
        Accessible.role: Accessible.ToolTip
        Accessible.name: root.text
        HoverHandler { id: bubbleHover; onHoveredChanged: root.syncAutomatic() }
        Flickable {
            id: viewport
            objectName: "tooltip_viewport"
            x: root.bodyRect.x + root.nonnegative(root.contentPadding)
            y: root.bodyRect.y + root.nonnegative(root.contentPadding)
            width: Math.max(0, root.bodyRect.width - root.nonnegative(root.contentPadding) * 2)
            height: Math.max(0, root.bodyRect.height - root.nonnegative(root.contentPadding) * 2)
            clip: true
            contentWidth: width
            contentHeight: root.scrollContent ? root._naturalHeight : height
            interactive: root.scrollContent && contentHeight > height
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.VerticalFlick
            onContentHeightChanged: contentY = Math.max(0, Math.min(contentY, contentHeight - height))
            onHeightChanged: contentY = Math.max(0, Math.min(contentY, contentHeight - height))
            Controls.ScrollBar.vertical: Controls.ScrollBar { }
            Loader {
                id: contentLoader
                objectName: "tooltip_contentLoader"
                sourceComponent: root.contentComponent
                active: root.contentComponent !== null
                visible: active
                width: viewport.width
                height: root.scrollContent ? root._naturalHeight : viewport.height
            }
            Item {
                id: inlineContent
                width: viewport.width
                height: root.scrollContent ? implicitHeight : viewport.height
                visible: root.contentComponent === null
                implicitWidth: {
                    let result = 0;
                    for (let i = 0; i < children.length; ++i)
                        if (children[i].visible) result = Math.max(result, children[i].x + children[i].implicitWidth);
                    return result;
                }
                implicitHeight: {
                    let result = 0;
                    for (let i = 0; i < children.length; ++i)
                        if (children[i].visible) result = Math.max(result, children[i].y + (children[i].implicitHeight || children[i].height));
                    return result;
                }
            }
            Label {
                id: textLabel
                width: viewport.width
                style: body
                text: root.text
                visible: root.contentComponent === null && inlineContent.children.length === 0
                wrapMode: Text.WordWrap
                sizeToContentHeight: true
            }
        }
    }
}

// API usage (external):
// import LVRS 1.0 as LV
// LV.Tooltip { target: helpButton; contentComponent: helpView }
// tooltip.openAt(canvas, Qt.point(pointerX, pointerY), previewView)
