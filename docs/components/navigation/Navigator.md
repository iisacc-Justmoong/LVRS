<a id="navigator"></a>

# 네비게이터

위치: `src/qml/components/navigation/Navigator.qml`

`Navigator`는 활성 `PageRouter` 탐색을 위한 글로벌 싱글턴 대표입니다.

<a id="purpose"></a>

## 목적

- 한 줄의 앱 전체 탐색 호출을 허용합니다.
- 라우터 등록 스택을 추적하고 활성 라우터를 선택합니다.
- 프록시 경로/구성 요소 탐색 및 백 스택 작업.

<a id="properties"></a>

## 속성

- `router`
- `routerStack`
- `hasRouter`
- `currentPath`
- `depth`
- `interactiveTransitionActive`
- `interactiveTransitionProgress`
- `interactiveTransitionDirection`
- `interactiveTransitionOperation`

<a id="router-lifecycle-methods"></a>

## 라우터 수명주기 방법

- `registerRouter(targetRouter)`
- `unregisterRouter(targetRouter?)`

등록 순서는 라우터가 상위 항목을 공유할 때 형제 순서를 고려한 다음 추가로 대체됩니다.

<a id="navigation-methods"></a>

## 탐색 방법

경로 기반:

- `go(path, params)`
- `replace(path, params)`
- `setRoot(path, params)`
- `back()`
- `popToRoot()`

구성 요소 기반:

- `goTo(component, params)`
- `replaceWith(component, params)`
- `setRootComponent(component, params)`

대화형:

- `beginInteractiveTransition(spec)`
- `beginInteractiveBack(meta?)`
- `beginInteractivePush(path, params?, meta?)`
- `beginInteractiveReplace(path, params?, meta?)`
- `beginInteractiveSetRoot(path, params?, meta?)`
- `updateInteractiveTransition(progress, details?)`
- `finishInteractiveTransition(commit?)`
- `cancelInteractiveTransition()`
- `shouldCommitInteractiveTransition(progress?, velocityX?, velocityY?)`

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.LabelButton {
    text: "Open Reports"
    onClicked: LV.Navigator.go("/reports")
}
```

<a id="how-it-works"></a>

## 동작 원리

- 활성 라우터는 `routerStack`의 꼬리입니다.
- 가능한 라우터가 없으면 모든 탐색 방법은 `false`를 반환합니다.
- 라우터 활성화 변경 시 지원되는 경우 추적기 동기화 후크(`syncViewStateTracker`)가 호출됩니다.
- 대화형 전환 도우미는 얇은 프록시로 유지됩니다. 미리보기 생성 및 스택 커밋은 여전히 ​​라우터의 책임입니다.

<a id="nested-router-strategy"></a>

## 중첩 라우터 전략

중첩된 라우터가 있는 애플리케이션에서는 등록/등록 취소 순서에 따라 활성 라우터 확인이 결정됩니다. 중첩된 탐색 영역을 탑재/마운트 해제할 때 명시적인 등록 경계를 사용하세요.

<a id="debug-tip"></a>

## 디버그 팁

경로 오류를 진단하기 전에 디버그 패널에서 `Navigator.currentPath` 및 `Navigator.depth`를 확인하여 전역 대상 라우터 상태를 확인하세요.

<a id="recipe-safe-back-action"></a>

## 레시피: 세이프 백 액션

```qml
import LVRS 1.0 as LV

function safeBack() {
    if (LV.Navigator.depth > 1)
        LV.Navigator.back()
}
```

활성 라우터를 통한 대화형 가장자리 스와이프:

```qml
import LVRS 1.0 as LV

function driveEdgePan(progress, velocityX) {
    if (!LV.Navigator.interactiveTransitionActive)
        LV.Navigator.beginInteractiveBack({ source: "edge-pan" })
    LV.Navigator.updateInteractiveTransition(progress, { velocityX: velocityX })
}
```

<a id="failure-pattern"></a>

## 실패 패턴

단일 항목 스택에서 깊이 가드 없이 `back()`를 호출하면 작동하지 않으며 상위 공급 측 흐름 설계에서 탐색 버그를 숨길 수 있습니다.

<a id="shared-motion"></a>

## 공유 모션

네비게이터는 보이는 페이지 전환을 소유한 활성 PageRouter로 전달됩니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
