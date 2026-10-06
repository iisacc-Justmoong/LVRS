# WindowSafeAreaObserver

위치: `src/backend/platform/windowsafeareaobserver.h`

`WindowSafeAreaObserver`는 바인딩된 `QWindow`/`QQuickWindow`에 대한 플랫폼 창 안전 영역 삽입을 노출합니다.

<a id="purpose"></a>

## 목적

- 하위 소비 측 앱은 레이아웃을 안전 영역 컨테이너에 강제로 적용하지 않고 실제 시스템 안전 영역 삽입을 읽을 수 있습니다.
- 상태 표시줄, 노치 및 홈 표시기 영역이 의도적으로 앱으로 제어되는 풀블리드 루트를 지원합니다.
- 바인딩된 창이 표시되거나, 크기가 조정되거나, 이동되거나, 다시 화면이 표시되거나 플랫폼 표면이 변경되면 삽입 값을 새로 고칩니다.

## API

- `window`: `QWindow`/`QQuickWindow` 개체를 대상으로 합니다.
- `leftInset`
- `topInset`
- `rightInset`
- `bottomInset`
- `bottomCornerRadius` :  Android   12+ 화면 하단 모서리 반지름, Qt 로직 픽셀로 변환됨. 사용 불가능할 때 `-1` 를 반환하며, 플랫폼이 사각 모서리를 보고할 때 `0` 를 반환함. 화면 기하학으로 새로고침되며 분리 시 `-1` 로 초기화됨. `resolved` 는 패딩을 설명하며 모서리 반지름 사용 가능 여부를 설명하지 않음.
- `resolved` : 관찰자가 플랫폼 창을 가지게 되고 현재 안전 영역 마진이 조회된 후 한 번 `true` 합니다.
- `refresh()`: 강제로 다시 쿼리합니다.

<a id="usage"></a>

## 사용법

```qml
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root
    visible: true

    LV.WindowSafeAreaObserver {
        id: safeArea
        window: root
    }

    Rectangle {
        anchors.fill: parent
        color: "#111827"
    }

    Item {
        x: safeArea.leftInset
        y: safeArea.topInset
        width: parent.width - safeArea.leftInset - safeArea.rightInset
        height: 56
    }
}
```

<a id="how-it-works"></a>

## 동작 원리

- 내부적으로 관찰자는 `ApplicationWindow` 패딩에서 추론하는 대신 Qt의 플랫폼 창 안전 영역 여백을 쿼리합니다.
- LVRS는 의도적으로 루트 콘텐츠를 모바일에서 풀 블리드 상태로 유지하기 때문에 이는 중요합니다. 앱은 시스템 삽입을 언제 존중할지 결정합니다.
- 플랫폼 창이 존재하기 전에는 `resolved`가 `false`로 유지되고 삽입 값은 `0`로 유지됩니다.
- 안전 영역 여백을 보고하지 않는 플랫폼에서는 관찰자가 연결 후에도 성공적으로 확인하지만 모든 삽입 값은 `0`로 유지됩니다.

<a id="shared-motion"></a>

## 공유 모션

관찰자는 주변 창이 올바르게 배치될 수 있도록 네이티브 형상을 동기식으로 보고합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
