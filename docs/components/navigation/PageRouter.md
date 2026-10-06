# PageRouter

위치: `src/qml/components/navigation/PageRouter.qml`

`PageRouter`는 `StackView` + `RouteResolver`를 기반으로 구축된 LVRS 스택 탐색 엔진입니다.

<a id="purpose"></a>

## 목적

- 페이지 대상(`component`/`source`)에 대한 경로 경로를 확인합니다.
- 경로 스택 메타데이터를 시각적 스택 작업과 동기화합니다.
- 옵션 ViewModel/ViewState 추적 후크를 통합합니다.

<a id="core-api"></a>

## 코어 API

경로 구성:

- `routes`
- `initialPath`
- `notFoundComponent`, `notFoundSource`
- `routeResolveCacheCapacity`

스택 및 경로 상태:

- `path`(SwiftUI와 유사한 스택 항목)
- `currentPath`, `currentParams`
- `depth`, `canGoBack`, `currentPageItem`
- 대화형 전환 상태:
  - `interactiveTransitionActive`
  - `interactiveTransitionProgress`
  - `interactiveTransitionDirection`
  - `interactiveTransitionOperation`
  - `interactiveTransitionFromPath`, `interactiveTransitionToPath`
  - `interactiveTransitionFromParams`, `interactiveTransitionToParams`
  - `interactiveTransitionVelocityX`, `interactiveTransitionVelocityY`
  - `interactiveTransitionMeta`
  - `interactiveTransitionPreviewItem`
  - `interactiveTransitionCanCommit`

프리젠테이션 플래그:

- `enforcePageViewport`
- `isolateInactivePages`
- `retainInactivePageCount`
- `interactiveTransitionsEnabled`
- `interactiveTransitionSettleDuration`(기본값: `0`)
- `interactiveTransitionCommitProgress`
- `interactiveTransitionVelocityThreshold`
- `interactiveTransitionOutgoingParallaxFactor`
- `interactiveTransitionIncomingPreviewFactor`

글로벌 등록:

- `registerAsGlobalNavigator`

신호:

- `navigated(path, params)`
- `navigationFailed(path)`
- `componentNavigated(component)`
- `interactiveTransitionStarted(state)`
- `interactiveTransitionUpdated(state)`
- `interactiveTransitionCommitted(state)`
- `interactiveTransitionCancelled(state)`
- `interactiveTransitionRejected(reason, state)`

탐색 방법:

- 경로 기반: `go`, `push`, `replace`, `setRoot`, `back`, `pop`, `popToRoot`
- 구성요소 기반: `goTo`, `replaceWith`, `setRootComponent`
- 대화형:
  - `beginInteractiveTransition(spec)`
  - `beginInteractiveBack(meta?)`
  - `beginInteractivePush(path, params?, meta?)`
  - `beginInteractiveReplace(path, params?, meta?)`
  - `beginInteractiveSetRoot(path, params?, meta?)`
  - `beginInteractivePushComponent(component, params?, meta?)`
  - `beginInteractiveReplaceComponent(component, params?, meta?)`
  - `beginInteractiveSetRootComponent(component, params?, meta?)`
  - `updateInteractiveTransition(progress, details?)`
  - `shouldCommitInteractiveTransition(progress?, velocityX?, velocityY?)`
  - `finishInteractiveTransition(commit?)`
  - `cancelInteractiveTransition()`
  - `interactiveTransitionState()`

<a id="route-grammar"></a>

## 경로 문법

- 정적: `/reports`
- 매개변수: `/runs/[id]`
- 나머지: `/logs/[...path]`

<a id="behavior-contract"></a>

## 행동 계약

- 커밋된 경로 스택 돌연변이는 C++ `NavigationStackModel`에 위임됩니다.
- 경로 확인은 내부 `RouteResolver`와 `routeResolveCacheCapacity`의 캐시 용량을 사용합니다.
- `navigate(...)`는 확인된 매개변수를 호출자 매개변수와 병합합니다.
- 누락된 경로는 구성 시 찾을 수 없는 대상으로 대체됩니다. 그렇지 않으면 `navigationFailed`를 내보냅니다.
- `initialPath`는 초기 `path` 스택이 없거나 기존 스택 항목이 이미 있는 경우에만 사용됩니다. `path: [...]`가 있는 라우터는 `Component.onCompleted` 중에 기본 `initialPath: "/"`로 재설정되지 않습니다.
- `applyPageViewportContract` 및 `applySingleChildViewportContract`는 뷰포트 안전 페이지 크기 조정을 적용합니다.
- `isolateInactivePages == true`인 경우 보유되지 않은 스택 항목은 숨겨지거나 비활성화되거나 불투명도가0입니다.
- 라우터는 가능한 경우 `ViewModels`와 바인딩을 동기화하고 `ViewStateTracker`와 스냅샷을 동기화할 수 있습니다.
- 대화형 전환 오케스트레이션은 내부 드라이버 개체에 위임되므로 `PageRouter`는 커밋된 스택 정보의 소스로 유지됩니다.
- 대화형 전환은 `finishInteractiveTransition(true)`가 커밋될 때까지 `path` 또는 `currentPath`를 변경하지 않습니다.
- 역방향 대화형 전환은 이전 스택 항목을 미리보기 화면으로 재사용합니다.
- 정방향 대화형 전환은 스택 위에 실시간 미리 보기 항목을 인스턴스화합니다. 따라서 미리보기 페이지는 커밋하기 전에 일반 QML 라이프사이클 코드를 실행합니다.
- 대화형 전환 커밋은 내장된 `StackView` 푸시/팝 애니메이션을 억제하고 스택 변형을 즉시 적용하므로 사용자 중심 드래그가 눈에 보이는 유일한 동작으로 유지됩니다.
- `interactiveTransitionSettleDuration`의 기본값은 `0`이므로 대화형 제스처를 놓으면 호스트가 명시적으로 선택하지 않는 한 추가 릴리스 후 정착 애니메이션이 재생되지 않습니다.
- 경쟁하는 비대화형 탐색 호출은 활성 대화형 전환을 먼저 중단합니다.

<a id="backend-model"></a>

## 백엔드 모델

`NavigationStackModel`는 정규화된 경로 항목, 구성 요소 스택 항목, 푸시/교체/루트 설정/팝 스택 결과, `currentPath`, `currentParams` 및 뷰 추적 설명자를 소유합니다. `PageRouter.qml`는 계속해서 경로 확인, `StackView` 항목 작업 및 대화형 전환 제어를 담당합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.PageRouter {
    id: router
    routes: [
        { path: "/", component: homePage },
        { path: "/runs/[id]", component: runPage }
    ]
    initialPath: "/"
}
```

대화형 뒤로 스와이프:

```qml
if (!router.interactiveTransitionActive)
    router.beginInteractiveBack({ source: "edge-pan" })

router.updateInteractiveTransition(progress, { velocityX: velocityX })
router.finishInteractiveTransition()
```

<a id="shared-motion"></a>

## 공유 모션

새 페이지는 적당한 규모 변화로 안정됩니다. 대화형 제스처는 직접 추적하고 릴리스를 결정합니다. 전역 속도, 모션 감소, 로컬 오버라이드 및 구성요소별 VisualCatalog 레시피는 [모션 정책](../../motion.md)를 참조하세요.
