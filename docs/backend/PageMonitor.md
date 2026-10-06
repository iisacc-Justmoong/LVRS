# PageMonitor

위치: `src/backend/navigation/pagemonitor.h` / `src/backend/navigation/pagemonitor.cpp`

`PageMonitor`는 간단한 실행 취소 의미 체계에 대한 경로 기록을 추적합니다.

<a id="purpose"></a>

## 목적

- 주문된 경로 기록을 유지합니다.
- 최신 항목을 제거하여 `undo()` 동작을 제공합니다.
- 현재 경로를 노출하고 가용성을 실행 취소합니다.

<a id="properties"></a>

## 속성

- `history: stringList`
- `count: int`
- `current: string`
- `canUndo: bool`

<a id="methods"></a>

## 방법

- `record(path): void`
- `undo(): string`
- `clear(): void`

<a id="behavior"></a>

## 행동

`record(path)`:

- 들어오는 경로를 다듬고,
- 빈 값을 무시합니다.
- 현재 꼬리와 다른 경우에만 추가됩니다.

`undo()`:

- 기록 크기 <= 1인 경우 변형 없이 현재를 반환합니다.
- 그렇지 않으면 마지막 항목을 제거하고 새 현재 항목을 반환합니다.

`clear()`:

- 이미 비어 있으면 작동하지 않습니다.
- 그렇지 않으면 기록을 지우고 변경 사항을 내보냅니다.

<a id="signal"></a>

## 신호

- `historyChanged()`

<a id="usage-example"></a>

## 사용예

```qml
import LVRS 1.0 as LV

function onRouteChanged(path) {
    LV.PageMonitor.record(path)
}

function goBackIfPossible() {
    if (!LV.PageMonitor.canUndo)
        return
    const target = LV.PageMonitor.undo()
    LV.Navigator.replace(target)
}
```

<a id="integration-pattern-with-router"></a>

## 라우터와의 통합 패턴

일반적인 패턴은 `PageRouter.navigated`를 `PageMonitor.record`로 미러링하는 것입니다. 이는 UI 스택 외부에 경로 기록을 유지하고 독립적인 실행 취소 UI 제어를 가능하게 합니다.

<a id="edge-cases"></a>

## 엣지 케이스

- 중복된 연속 경로는 설계상 무시됩니다.
- 비어 있거나 공백만 있는 입력은 무시됩니다.
- 단일 항목 기록의 `undo()`는 멱등성을 갖습니다.

## FAQ

Q. `PageMonitor`는 `PageRouter`와 자동으로 동기화됩니까?   A. 아니요. 동기화는 명시적입니다. 경로 변경은 통합 레이어에서 `record()`를 호출해야 합니다.
