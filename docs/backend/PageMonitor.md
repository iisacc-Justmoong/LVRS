# PageMonitor

Location: `backend/navigation/pagemonitor.h` / `backend/navigation/pagemonitor.cpp`

`PageMonitor` tracks path history for simple undo semantics.

## Purpose

- Keep ordered route history.
- Provide `undo()` behavior by removing latest entry.
- Expose current path and undo availability.

## Properties

- `history: stringList`
- `count: int`
- `current: string`
- `canUndo: bool`

## Methods

- `record(path): void`
- `undo(): string`
- `clear(): void`

## Behavior

`record(path)`:

- trims incoming path,
- ignores empty value,
- appends only when different from current tail.

`undo()`:

- if history size <= 1, returns current without mutation,
- otherwise removes last entry and returns new current.

`clear()`:

- no-op when already empty,
- otherwise clears history and emits change.

## Signal

- `historyChanged()`

## Usage Example

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

## Integration Pattern with Router

A common pattern is to mirror `PageRouter.navigated` into `PageMonitor.record`.
This keeps route history external to UI stack and enables independent undo UI controls.

## Edge Cases

- Duplicate consecutive routes are ignored by design.
- Empty or whitespace-only input is ignored.
- `undo()` on a single-entry history is idempotent.

## FAQ

Q. Does `PageMonitor` automatically sync with `PageRouter`?  
A. No. Sync is explicit; route changes must call `record()` from integration layer.
