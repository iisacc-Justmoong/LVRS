# NativeWindowInteraction

위치: `src/backend/platform/nativewindowinteraction.h/.cpp`

`NativeWindowInteraction`는 LVRS 창에 대한 네이티브 우선 대화형 이동 및 크기 조정 요청을 소유하는 QML 싱글턴입니다.

<a id="methods"></a>

## 방법

- `requestSystemMove(windowObject)`는 개체를 `QWindow`로 캐스팅하고 `QWindow::startSystemMove()`를 호출합니다.
- `requestSystemResize(windowObject, edges)`는 에지 마스크를 검증하고 먼저 `QWindow::startSystemResize()`를 호출합니다.
- `requestSystemResizeAt(windowObject, edges, globalPosition)`는 수동 대체 경로에 대해 정확한 글로벌 프레스 위치를 유지하면서 동일한 네이티브 우선 요청을 수행합니다. 내장된 LVRS 크기 조정 핸들은 이 형식을 사용하므로 원격, 태블릿 및 합성 포인터 입력은 별도로 샘플링된 커서 위치에 의존하지 않습니다.
- `isValidResizeEdges(edges)` 는 한 개의 엣지 또는 2 개의 인접한 엣지만을 허용합니다.

브리지는 프레임워크 최소 Qt 6.5에서 사용할 수 있는 LVRS QML 계약을 유지합니다. QML `Window` 유형에 대한 동등한 메서드는 Qt 6.8에 도입되었으며, 기본 `QWindow` API는 지원되는 C++ 기준선에서 사용할 수 있습니다.

네이티브 리사이즈는 컴포저터 스냅, 타일링, 애니메이션을 보존하기 때문에 선호되는 경로로 남습니다. Qt 6.8 의 Cocoa 플랫폼은  `startSystemMove()` 를 구현하지만  `startSystemResize()` 를 구현하지 않으므로,  macOS 에서 거부된 리사이즈 요청은  LVRS 가 소유한 포인터 세션을 시작합니다. 그 세션은  `QWindow::geometry()` 를 글로벌 포인터 델타에서 업데이트하고, 이동하는 에지를 상속된 최소 및 최대 크기에 제한하며, 왼쪽 버튼 릴리스, 마우스 언그랩, 애플리케이션 비활성화 또는 윈도우 해체 시 종료됩니다. 명시 위치 진입점은 해당 델타를 실제 프레스 이벤트에 고정하고,  2-인수 호환성 진입점은  `QCursor::pos()` 를 샘플링합니다. 두 경우 모두  `true` 를 반환하지만, 이는 네이티브 작업 또는 이  macOS   대체 경로 가 실제로 시작했을 때만 해당합니다.

macOS   대체 경로 는 의도적으로 마우스 이벤트를 합성하거나 비공개  AppKit API 를 사용하지 않습니다. 그 대가는 리지오메트리가 솔리드 커스텀 크롬에 대해 일관되게 작동하지만,  Qt 의 Cocoa 플랫폼  API 를 통해 이용 불가능한 네이티브 스냅 또는 타일링 동작을 얻지 못한다는 점입니다.

참고자료:

- [Qt 창 QML 유형](https://doc.qt.io/qt-6/qml-qtquick-window.html)
- [QWindow 클래스](https://doc.qt.io/qt-6/qwindow.html)
