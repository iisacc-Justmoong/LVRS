# WindowChromeInteraction

Location: `qml/WindowChromeInteraction.qml`

`WindowChromeInteraction` is the reusable pointer layer behind `LV.Window` and `LV.ApplicationWindow`. Most applications should configure the properties exposed by those root types instead of instantiating this component directly.

## API

- target: `targetWindow`
- master switch: `interactionEnabled`
- move region: `moveHandleEnabled`, `moveHandleHeight`, `moveHandleTopMargin`, `moveHandleLeftMargin`, `moveHandleRightMargin`
- move exclusions: `moveExclusionItems`
- resize regions: `resizeHandlesEnabled`, `resizeEdges`, `resizeBorderThickness`, `resizeCornerSize`
- calculated geometry: `effectiveResizeBorderThickness`, `effectiveResizeCornerSize`
- move handle alias: `moveHandleItem`
- methods: `requestMove()`, `requestResize(edges)`, `isResizeEdgeEnabled(edge)`
- signals: `moveAttempted(started)`, `resizeAttempted(edges, started)`

## Behavior

- The move handle is disabled for minimized and fullscreen windows.
- Resize handles are enabled only for a visible, windowed target.
- Corner handles sit above edge handles, and all resize handles sit above the move handle.
- A resize press maps its `MouseArea` position to global coordinates before requesting the operation. This keeps the macOS fallback anchored to the event that started it even when a remote, tablet, or synthesized input source has not synchronized `QCursor::pos()`.
- A resize press stays accepted when either native resize or the macOS manual fallback starts. Requests rejected by both paths release the `MouseArea` press so an item underneath can still process it.
- Exclusion coordinates are mapped across the target window item tree, so controls may live in scaled or nested content.

## Direct use

```qml
import QtQuick
import QtQuick.Controls
import LVRS as LV

ApplicationWindow {
    id: root
    visible: true

    LV.WindowChromeInteraction {
        anchors.fill: parent
        targetWindow: root
        moveExclusionItems: [menuButton]
    }

    Button {
        id: menuButton
        text: "Menu"
    }
}
```
