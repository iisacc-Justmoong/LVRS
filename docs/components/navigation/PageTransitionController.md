# PageTransitionController

위치: `src/qml/components/navigation/PageTransitionController.qml`

`PageTransitionController`는 라우터 내부에 커밋된 경로 상태를 유지하면서 `PageRouter` 대화형 전환을 구동하는 비시각적 프록시입니다.

<a id="purpose"></a>

## 목적

- `PageRouter` 위에 컨트롤러 모양의 표면을 제공합니다.
- 특정 라우터에 바인딩하거나 `Navigator.router`를 따릅니다.
- 전환 수명 주기 신호를 전용 객체로 전달합니다.

<a id="properties"></a>

## 속성

- `router`
- `active`
- `progress`
- `direction`
- `operation`
- `fromPath`, `toPath`
- `fromParams`, `toParams`
- `canCommit`

<a id="signals"></a>

## 신호

- `started(state)`
- `updated(state)`
- `committed(state)`
- `cancelled(state)`
- `rejected(reason, state)`

<a id="methods"></a>

## 방법

- `canControl()`
- `begin(spec)`
- `beginBack(meta?)`
- `beginPush(path, params?, meta?)`
- `beginReplace(path, params?, meta?)`
- `beginSetRoot(path, params?, meta?)`
- `beginPushComponent(component, params?, meta?)`
- `beginReplaceComponent(component, params?, meta?)`
- `beginSetRootComponent(component, params?, meta?)`
- `update(progress, details?)`
- `finish(commit?)`
- `cancel()`
- `shouldCommit(progress?, velocityX?, velocityY?)`

<a id="behavior-contract"></a>

## 행동 계약

- 컨트롤러는 `PageRouter.path`를 직접적으로 변경하지 않습니다. 커밋과 취소는 여전히 라우터 내부에서 발생합니다.
- `router`가 설정되지 않은 경우 기본 바인딩은 `Navigator.router`를 따릅니다.
- 명시적인 부울이 없는 `finish()`는 라우터의 커밋 휴리스틱을 사용합니다.

<a id="usage"></a>

## 사용법

```qml
import LVRS 1.0 as LV

LV.PageTransitionController {
    id: transitions
    router: router

    function driveBackSwipe(progress, velocityX) {
        if (!active)
            beginBack({ source: "edge-pan" })
        update(progress, { velocityX: velocityX })
    }
}
```

<a id="how-it-works"></a>

## 동작 원리

- 읽기 전용 상태는 바인딩된 라우터의 대화형 전환 속성에서 미러링됩니다.
- 수명주기 신호는 `Connections`를 통해 전달됩니다.
- 이렇게 하면 동작 정책과 전환 정책이 커밋된 라우팅 상태에서 분리된 상태로 유지됩니다.
