# ViewStateTracker

Location: `backend/navigation/viewstatetracker.h` / `backend/navigation/viewstatetracker.cpp`

`ViewStateTracker` tracks route/view stack state and computes active/inactive/disabled partitions.

## Purpose

- Represent current page stack as stateful records.
- Compute one active top-most enabled view.
- Apply explicit disable overrides independent of base route metadata.

## States

Enum `ViewState`:

- `Active`
- `Inactive`
- `Disabled`

String representation from API:

- `"Active"`
- `"Inactive"`
- `"Disabled"`

## Properties

- `stack: list`
- `loadedViews: stringList`
- `activeViews: stringList`
- `inactiveViews: stringList`
- `disabledViews: stringList`
- `currentActiveView: string`
- `loadedCount: int`

## Methods

Stack sync and overrides:

- `syncStack(entries)`
- `setViewDisabled(viewId, disabled)`
- `setViewEnabled(viewId, enabled)`

Lookup/snapshot:

- `isLoaded(viewId)`
- `stateOf(viewId)`
- `view(viewId)`
- `snapshot()`
- `clear()`

## Entry Parsing Rules

`syncStack(entries)` accepts entry maps with optional fields:

- `viewId`
- `path`
- `enabled` / `disabled`

If `viewId` is missing:

- use `path` when available,
- otherwise generate `_component_<index>`.

## State Resolution Rule

- Starting from stack tail, first effectively enabled entry becomes `Active`.
- Other effectively enabled entries become `Inactive`.
- Disabled entries become `Disabled`.

## Usage Example

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.ViewStateTracker.syncStack([
        { viewId: "overview", path: "/" },
        { viewId: "reports", path: "/reports", enabled: true }
    ])
}
```

## Extended Example: Temporary Disable Override

```qml
import LVRS 1.0 as LV

function suspendView(viewId) {
    LV.ViewStateTracker.setViewDisabled(viewId, true)
}

function resumeView(viewId) {
    LV.ViewStateTracker.setViewDisabled(viewId, false)
}
```

## Operational Notes

- Disable overrides are independent from route-provided `enabled` flags.
- `currentActiveView` always resolves to the top-most effectively enabled entry.
- `clear()` resets both stack records and disable overrides.

## FAQ

Q. Why is only one view marked Active?  
A. Tracker defines active state as top-most effectively enabled view in current stack.
