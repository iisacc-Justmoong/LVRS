# Window

Location: `qml/Window.qml`

`Window` is the lightweight LVRS top-level window. It provides platform and size-class metadata, render-quality wiring, native solid chrome, and native-first move/resize interactions without the adaptive scaffold carried by `ApplicationWindow`.

## Core properties

### Platform and layout

- `platform`, `backendRuntimeProfile`, `canonicalPlatform`
- `isMobilePlatform`, `isDesktopPlatform`, `backendMobilePlatform`
- `widthClass`, `heightClass`, `isCompact`, `isExpanded`
- `desktopMinWidth/Height`, `mobileMinWidth/Height`
- `usePlatformSafeMargin`, `safeMargin`
- `renderSurfaceBounds`, `layoutSafeAreaBounds`

### Native chrome

- `windowColor`
- `forceNativeDarkTitleBar`
- `solidChrome`
- `windowChromeInteractionsEnabled`: defaults to `solidChrome && isDesktopPlatform`

When `solidChrome` is disabled, the native title bar owns move and resize behavior and the LVRS interaction layer is disabled by default. An application using a different frameless strategy can explicitly set `windowChromeInteractionsEnabled: true`.

### Move handle

- `windowDragHandleEnabled`
- `windowDragHandleHeight`
- `windowDragHandleTopMargin`
- `windowDragHandleLeftMargin`
- `windowDragHandleRightMargin`
- `windowDragExclusionItems`
- `windowDragHandleItem` (read-only alias)

The stock handle calls `requestWindowMove()` from the left-button press. Items listed in `windowDragExclusionItems` remain interactive and do not start a move operation, even when they overlap the drag rectangle.

### Resize handles

- `windowResizeHandlesEnabled`
- `windowResizeEdges`
- `windowResizeBorderThickness`
- `windowResizeCornerSize`

The stock layer provides four edge and four corner hit regions. `windowResizeEdges` is a mask of `Qt.LeftEdge`, `Qt.TopEdge`, `Qt.RightEdge`, and `Qt.BottomEdge`. A corner is active only when both adjacent edges are enabled. Handles are active only while the window is in `Window.Windowed` visibility, so maximized and fullscreen content does not lose its outer hit region.

### Interaction layer

- `windowChromeInteractionZ`
- `windowChromeInteractionLayer` (read-only alias)
- `requestWindowMove()`
- `requestWindowResize(edges)`
- `windowMoveAttempted(started)`
- `windowResizeAttempted(edges, started)`

The request methods return whether an interaction actually started. Resize accepts one edge or two adjacent edges only. Native platform resize remains preferred; on macOS, where Qt 6.8 Cocoa rejects `startSystemResize()`, LVRS tracks the pointer and applies constrained window geometry until release.

## Custom title-bar controls

Keep the stock drag region and exclude interactive controls:

```qml
import QtQuick
import LVRS as LV

LV.Window {
    id: settingsWindow
    width: 480
    height: 360
    visible: true

    windowDragExclusionItems: [closeButton]

    LV.IconButton {
        id: closeButton
        anchors.top: parent.top
        anchors.right: parent.right
        onClicked: settingsWindow.close()
    }
}
```

Or disable only the stock move rectangle and call the same system API from an application-owned title bar:

```qml
LV.Window {
    id: settingsWindow
    windowDragHandleEnabled: false

    Rectangle {
        width: parent.width
        height: 40

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton
            onPressed: function (mouse) {
                if (!settingsWindow.requestWindowMove())
                    mouse.accepted = false
            }
        }
    }
}
```

Set `windowResizeHandlesEnabled: false` when the application supplies its own edge hit regions. Those custom regions can call `requestWindowResize(Qt.LeftEdge | Qt.TopEdge)` for a corner.

## Notes

- `minimumWidth/Height` continue to follow the LVRS desktop/mobile minimum properties. Applications can use the inherited `maximumWidth/Height` properties normally.
- A fixed-size or unsupported operation returns `false`. The macOS resize fallback honors inherited minimum and maximum dimensions but cannot provide compositor snapping or tiling.
- `WindowChromeInteraction` contains the reusable pointer layer, while `NativeWindowInteraction` bridges requests to `QWindow` on Qt 6.5 and newer.
