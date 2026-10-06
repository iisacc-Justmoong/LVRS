# WindowChromeInteraction

위치: `src/qml/WindowChromeInteraction.qml`

`WindowChromeInteraction`는 `LV.Window` 및 `LV.ApplicationWindow` 뒤에 있는 재사용 가능한 포인터 레이어입니다. 대부분의 애플리케이션은 이 구성 요소를 직접 인스턴스화하는 대신 해당 루트 유형에 의해 노출되는 속성을 구성해야 합니다.

## API

- 대상: `targetWindow`
- 마스터 스위치: `interactionEnabled`
- 지역 이동: `moveHandleEnabled`, `moveHandleHeight`, `moveHandleTopMargin`, `moveHandleLeftMargin`, `moveHandleRightMargin`
- 이동 제외: `moveExclusionItems`
- 영역 크기 조정: `resizeHandlesEnabled`, `resizeEdges`, `resizeBorderThickness`, `resizeCornerSize`
- 계산된 형상: `effectiveResizeBorderThickness`, `effectiveResizeCornerSize`
- 이동 핸들 별칭: `moveHandleItem`
- 방법: `requestMove()`, `requestResize(edges)`, `isResizeEdgeEnabled(edge)`
- 신호: `moveAttempted(started)`, `resizeAttempted(edges, started)`

<a id="behavior"></a>

## 행동

- 최소화된 창과 전체 화면 창에서는 이동 핸들이 비활성화됩니다.
- 이동 히트 영역은 `moveHandleTopMargin + moveHandleHeight`에서 끝납니다. 솔리드 macOS 크롬은 AppKit 배경 이동을 비활성화하므로 이 영역 외부의 드래그는 LVRS 핸들러 또는 해당 제외를 우회할 수 없습니다.
- 크기 조정 핸들은 보이는 창 대상에 대해서만 활성화됩니다.
- 모서리 핸들은 가장자리 핸들 위에 있고 모든 크기 조정 핸들은 이동 핸들 위에 있습니다.
- 크기 조정 프레스는 작업을 요청하기 전에 `MouseArea` 위치를 전역 좌표에 매핑합니다. 이렇게 하면 원격, 태블릿 또는 합성 입력 소스가 `QCursor::pos()`를 동기화하지 않은 경우에도 macOS 대체 경로를 시작한 이벤트에 고정된 상태로 유지됩니다.
- 네이티브 크기 조정 또는 macOS 매뉴얼 대체 경로가 시작되면 크기 조정 누르기가 계속 허용됩니다. 두 경로 모두에서 요청이 거부되면 `MouseArea` 프레스가 해제되어 아래 항목이 계속 처리될 수 있습니다.
- 제외 좌표는 대상 창 항목 트리 전체에 매핑되므로 컨트롤은 크기가 조정되거나 중첩된 콘텐츠에 있을 수 있습니다.

<a id="direct-use"></a>

## 직접 사용

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
