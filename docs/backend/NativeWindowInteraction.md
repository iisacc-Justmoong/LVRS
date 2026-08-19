# NativeWindowInteraction

Location: `backend/platform/nativewindowinteraction.h/.cpp`

`NativeWindowInteraction` is a QML singleton that owns native-first interactive move and resize requests for LVRS windows.

## Methods

- `requestSystemMove(windowObject)` casts the object to `QWindow` and calls `QWindow::startSystemMove()`.
- `requestSystemResize(windowObject, edges)` validates the edge mask and first calls `QWindow::startSystemResize()`.
- `requestSystemResizeAt(windowObject, edges, globalPosition)` performs the same native-first request while preserving the exact global press position for a manual fallback. Built-in LVRS resize handles use this form so remote, tablet, and synthesized pointer input does not depend on a separately sampled cursor position.
- `isValidResizeEdges(edges)` accepts one edge or two adjacent edges only.

The bridge keeps the LVRS QML contract available on the framework minimum Qt 6.5. The equivalent methods on the QML `Window` type were introduced in Qt 6.8, while the underlying `QWindow` APIs are available to the supported C++ baseline.

Native resize remains the preferred path because it preserves compositor snapping, tiling, and animations. Qt 6.8's Cocoa platform implements `startSystemMove()` but not `startSystemResize()`, so a rejected resize request on macOS starts an LVRS-owned pointer session instead. That session updates `QWindow::geometry()` from global pointer deltas, constrains the moving edge against the inherited minimum and maximum sizes, and ends on left-button release, mouse ungrab, application deactivation, or window teardown. The explicit-position entry point anchors that delta to the actual press event; the two-argument compatibility entry point samples `QCursor::pos()`. Both return `true` only when either the native operation or this macOS fallback actually starts.

The macOS fallback deliberately does not synthesize mouse events or use private AppKit APIs. Its tradeoff is that resize geometry works consistently for solid custom chrome but does not gain native snapping or tiling behavior unavailable through Qt's Cocoa platform API.

References:

- [Qt Window QML type](https://doc.qt.io/qt-6/qml-qtquick-window.html)
- [QWindow class](https://doc.qt.io/qt-6/qwindow.html)
