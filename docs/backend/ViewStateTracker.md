# ViewStateTracker

위치: `src/backend/navigation/viewstatetracker.h` / `src/backend/navigation/viewstatetracker.cpp`

`ViewStateTracker`는 경로/뷰 스택 상태를 추적하고 활성/비활성/비활성화 파티션을 계산합니다.

<a id="purpose"></a>

## 목적

- 현재 페이지 스택을 상태 저장 레코드로 나타냅니다.
- 활성화된 최상위 활성 뷰 하나를 계산합니다.
- 기본 경로 메타데이터와 관계없이 명시적인 비활성화 재정의를 적용합니다.

<a id="states"></a>

## 상태

열거형 `ViewState`:

- `Active`
- `Inactive`
- `Disabled`

API의 문자열 표현:

- `"Active"`
- `"Inactive"`
- `"Disabled"`

<a id="properties"></a>

## 속성

- `stack: list`
- `loadedViews: stringList`
- `activeViews: stringList`
- `inactiveViews: stringList`
- `disabledViews: stringList`
- `currentActiveView: string`
- `loadedCount: int`

<a id="methods"></a>

## 방법

스택 동기화 및 재정의:

- `syncStack(entries)`
- `setViewDisabled(viewId, disabled)`
- `setViewEnabled(viewId, enabled)`

조회/스냅샷:

- `isLoaded(viewId)`
- `stateOf(viewId)`
- `view(viewId)`
- `snapshot()`
- `clear()`

<a id="entry-parsing-rules"></a>

## 항목 구문 분석 규칙

`syncStack(entries)`는 선택적 필드가 있는 항목 맵을 허용합니다.

- `viewId`
- `path`
- `enabled` / `disabled`

`viewId`가 누락된 경우:

- 가능한 경우 `path`를 사용하세요.
- 그렇지 않으면 `_component_<index>`를 생성합니다.

<a id="state-resolution-rule"></a>

## 상태 해결 규칙

- 스택 테일에서 시작하여 처음으로 효과적으로 활성화된 항목은 `Active`가 됩니다.
- 효과적으로 활성화된 다른 항목은 `Inactive`가 됩니다.
- 비활성화된 항목은 `Disabled`가 됩니다.

<a id="usage-example"></a>

## 사용예

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.ViewStateTracker.syncStack([
        { viewId: "overview", path: "/" },
        { viewId: "reports", path: "/reports", enabled: true }
    ])
}
```

<a id="extended-example-temporary-disable-override"></a>

## 확장된 예: 임시 비활성화 재정의

```qml
import LVRS 1.0 as LV

function suspendView(viewId) {
    LV.ViewStateTracker.setViewDisabled(viewId, true)
}

function resumeView(viewId) {
    LV.ViewStateTracker.setViewDisabled(viewId, false)
}
```

<a id="operational-notes"></a>

## 운영 참고 사항

- 비활성화 재정의는 경로 제공 `enabled` 플래그와 별개입니다.
- `currentActiveView`는 항상 가장 효과적으로 활성화된 항목을 확인합니다.
- `clear()`는 스택 레코드를 모두 재설정하고 재정의를 비활성화합니다.

## FAQ

Q. 하나의 보기에만 활성으로 표시된 이유는 무엇입니까?   A. 추적기는 활성 상태를 현재 스택에서 가장 효과적으로 활성화된 보기로 정의합니다.
