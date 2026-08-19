# NativeWindowInteraction

Location: `backend/platform/nativewindowinteraction.h/.cpp`

`NativeWindowInteraction` is a QML singleton that forwards interactive move and resize requests to `QWindow`.

## Methods

- `requestSystemMove(windowObject)` casts the object to `QWindow` and calls `QWindow::startSystemMove()`.
- `requestSystemResize(windowObject, edges)` validates the edge mask and calls `QWindow::startSystemResize()`.
- `isValidResizeEdges(edges)` accepts one edge or two adjacent edges only.

The bridge keeps the LVRS QML contract available on the framework minimum Qt 6.5. The equivalent methods on the QML `Window` type were introduced in Qt 6.8, while the underlying `QWindow` APIs are available to the supported C++ baseline.

No manual `x/y/width/height` fallback is used. Returning the platform result preserves native snapping, tiling, resize constraints, and compositor behavior.

References:

- [Qt Window QML type](https://doc.qt.io/qt-6/qml-qtquick-window.html)
- [QWindow class](https://doc.qt.io/qt-6/qwindow.html)
