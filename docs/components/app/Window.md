<a id="window"></a>

# 창

위치: `src/qml/Window.qml`

`Window`는 경량 LVRS 최상위 창입니다. `ApplicationWindow`가 제공하는 적응형 스캐폴드 없이 플랫폼 및 크기 클래스 메타데이터, 렌더링 품질 배선, 네이티브 솔리드 크롬, 네이티브 우선 이동/크기 조정 상호 작용을 제공합니다.

<a id="core-properties"></a>

## 핵심 속성

<a id="platform-and-layout"></a>

### 플랫폼 및 레이아웃

- `platform`, `backendRuntimeProfile`, `canonicalPlatform`
- `isMobilePlatform`, `isDesktopPlatform`, `backendMobilePlatform`
- `widthClass`, `heightClass`, `isCompact`, `isExpanded`
- `desktopMinWidth/Height`, `mobileMinWidth/Height`
- `usePlatformSafeMargin`, `safeMargin`
- `renderSurfaceBounds`, `layoutSafeAreaBounds`

<a id="native-chrome"></a>

### 네이티브 크롬

- `windowColor`
- `forceNativeDarkTitleBar`
- `solidChrome`
- `windowChromeInteractionsEnabled`: 기본값은 `solidChrome && isDesktopPlatform`입니다.

`solidChrome`가 비활성화되면 네이티브 제목 표시줄이 이동 및 크기 조정 동작을 담당하며 LVRS 상호작용 계층은 기본적으로 비활성화된다. 다른 프레임 없는 창 전략을 사용하는 애플리케이션은 `windowChromeInteractionsEnabled: true`를 명시적으로 설정할 수 있다.

<a id="move-handle"></a>

### 핸들 이동

- `windowDragHandleEnabled`
- `windowDragHandleHeight`
- `windowDragHandleTopMargin`
- `windowDragHandleLeftMargin`
- `windowDragHandleRightMargin`
- `windowDragExclusionItems`
- `windowDragHandleItem`(읽기 전용 별칭)

스톡 핸들은 왼쪽 버튼을 눌러 `requestWindowMove()`를 호출합니다. `windowDragExclusionItems`에 나열된 항목은 대화형 상태를 유지하며 드래그 직사각형과 겹쳐도 이동 작업을 시작하지 않습니다.

<a id="resize-handles"></a>

### 핸들 크기 조정

- `windowResizeHandlesEnabled`
- `windowResizeEdges`
- `windowResizeBorderThickness`
- `windowResizeCornerSize`

스톡 레이어는 4 개의 엣지 히트 영역과 4 개의 코너 히트 영역을 제공합니다. `windowResizeEdges` 는 `Qt.LeftEdge` , `Qt.TopEdge` , `Qt.RightEdge` , 및 `Qt.BottomEdge` 의 마스크입니다. 코너는 인접한 두 엣지가 모두 활성화될 때만 활성화됩니다. 핸들은 창이 `Window.Windowed` 가시성 상태인 동안만 활성화되므로, 최대화 및 풀스크린 콘텐츠는 바깥쪽 히트 영역을 잃지 않습니다.

<a id="interaction-layer"></a>

### 상호작용 레이어

- `windowChromeInteractionZ`
- `windowChromeInteractionLayer`(읽기 전용 별칭)
- `requestWindowMove()`
- `requestWindowResize(edges)`
- `windowMoveAttempted(started)`
- `windowResizeAttempted(edges, started)`

요청 메서드는 상호작용이 실제로 시작되었는지를 반환합니다. Resize 는 하나의 엣지 또는 2 개의 인접한 엣지만을 허용합니다. 네이티브 플랫폼 리사이즈는 여전히 선호되며, macOS 에서 Qt 6.8 Cocoa 가 `startSystemResize()` 를 거부하는 경우, LVRS 는 포인터를 추적하고 릴리스까지 제한된 창 기하학을 적용합니다.

<a id="custom-title-bar-controls"></a>

## 사용자 정의 제목 표시줄 컨트롤

스톡 드래그 영역을 유지하고 대화형 컨트롤을 제외합니다.

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

또는 재고 이동 직사각형만 비활성화하고 애플리케이션 소유 제목 표시줄에서 동일한 시스템 API를 호출합니다.

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

애플리케이션이 자체 엣지 히트 영역을 제공하는 경우 `windowResizeHandlesEnabled: false`를 설정합니다. 이러한 사용자 정의 영역은 코너에 대해 `requestWindowResize(Qt.LeftEdge | Qt.TopEdge)`를 호출할 수 있습니다.

<a id="notes"></a>

## 메모

- `minimumWidth/Height`는 계속해서 LVRS 데스크톱/모바일 최소 속성을 따릅니다. 애플리케이션은 상속된 `maximumWidth/Height` 속성을 정상적으로 사용할 수 있습니다.
- 고정 크기 또는 지원되지 않는 작업은 `false`를 반환합니다. macOS 크기 조정 대체 경로는 상속된 최소 및 최대 치수를 준수하지만 합성기 스냅 또는 타일링을 제공할 수 없습니다.
- `WindowChromeInteraction`에는 재사용 가능한 포인터 레이어가 포함되어 있으며, `NativeWindowInteraction`는 Qt 6.5 이상의 `QWindow`에 요청을 연결합니다.
