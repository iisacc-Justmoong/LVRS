# AppBootstrapWindow

위치: `src/qml/AppBootstrapWindow.qml`

`AppBootstrapWindow`는 `ApplicationWindow`에 대한 호환성 래퍼입니다.

<a id="purpose"></a>

## 목적

- 하위 소비 측 앱에 대한 이전 부트스트랩 루트 가져오기 경로를 유지합니다.
- 상속된 `ApplicationWindow` 부트스트랩 계약 위에 표시 가능한 루트 편의 기본값을 제공합니다.

<a id="inherited-base"></a>

## 상속된 기반

- `ApplicationWindow`를 상속합니다.
- `initialRoutePath`, 페이지 스택 호스팅 및 런타임 프로필 기반 기본값을 포함하여 모든 `ApplicationWindow` 속성, 별칭, 메서드 및 신호를 계속 사용할 수 있습니다.

<a id="added-default"></a>

## 기본값이 추가되었습니다.

- `visible: true`

<a id="usage"></a>

## 사용법

```qml
import QtQuick
import LVRS 1.0 as LV

LV.AppBootstrapWindow {
    width: 900
    height: 620
    title: "MyApp"

    pageRoutes: [
        { path: "/", component: homePage }
    ]

    Component {
        id: homePage
        Item {}
    }
}
```

<a id="recommendation"></a>

## 추천

- 새로운 소비자 앱 루트에는 `ApplicationWindow`를 직접 사용하세요.
- 기존 코드베이스가 레거시 유형 이름의 이점을 얻거나 QML 루트에 사전 구성된 `visible: true`를 원하는 경우에만 `AppBootstrapWindow`를 유지하십시오.
- 상속된 렌더링 경로는 `ApplicationWindow`와 동일한 런타임-direct `RenderQuality` 정책입니다. 자동 장치 계층 사전 설정은 선택 상태로 유지됩니다.
- 데스크톱 대상에서는 상속된 기본 프로필이 여전히 `RuntimeEvents`를 자동 연결합니다. iOS/Android에서는 앱이 선택하지 않는 한 꺼진 상태로 유지됩니다.

<a id="shared-motion"></a>

## 공유 모션

호환성 루트는 ApplicationWindow의 서랍 및 페이지 모션을 상속합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
