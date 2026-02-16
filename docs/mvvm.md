# MVVM

LVRS MVVM model is implemented by `ViewModels` (`ViewModelRegistry`) and route/view binding metadata emitted from `PageRouter`.

## Purpose

- Register view-model instances by logical key.
- Bind a concrete view identity (`viewId`) to a model key.
- Control write ownership explicitly so only one view can mutate one model key at a time.

## Core API (`ViewModels`)

Location: `backend/state/viewmodelregistry.h` / `backend/state/viewmodelregistry.cpp`

Registration lifecycle:

- `set(key, object)`
- `get(key)`
- `remove(key)`
- `clear()`

Binding and ownership:

- `bindView(viewId, key, writable)`
- `unbindView(viewId)`
- `claimOwnership(viewId, key)`
- `releaseOwnership(viewId, key?)`
- `canWrite(viewId, key?)`
- `ownerOf(key)`

Data access helpers:

- `updateProperty(viewId, property, value)`
- `updatePropertyByKey(viewId, key, property, value)`
- `readProperty(viewId, property)`

State/diagnostics:

- `keys`, `views`, `bindings`, `owners`, `lastError`

## Binding from Router Metadata

`PageRouter` can bind views automatically when route metadata includes:

- `viewModelKey` or `modelKey`
- `viewId` (optional; defaults to route path or generated id)
- `writable` or `modelWritable`

This enables route-level ownership policy without per-page boilerplate.

## Write-Ownership Rule

Write access is allowed only when both conditions are true.

1. The view is bound to a model key.
2. The view is current owner of that key.

If ownership is missing or conflicting, write APIs return `false` and set `lastError`.

## Usage Pattern

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.ViewModels.set("Session", sessionViewModel)
    if (!LV.ViewModels.bindView("SessionPage", "Session", true))
        console.warn(LV.ViewModels.lastError)
}

function renameSession(nextName) {
    if (!LV.ViewModels.updateProperty("SessionPage", "name", nextName))
        console.warn(LV.ViewModels.lastError)
}
```

## How It Works

- Keys and view ids are normalized by trim-based token normalization.
- Ownership map (`key -> viewId`) is pruned when keys are removed.
- Orphaned model objects parented by `ViewModels` are auto-disposed when no key references remain.
- Binding updates are signal-driven (`viewsChanged`, `ownershipChanged`) so QML observers can react.

## Failure Modes to Handle

- Empty `viewId` or key.
- Binding to unknown key.
- Attempted writable bind while key is owned by another view.
- Property writes to unbound view or unknown property name.

## Advanced Scenario: Read-Only Detail + Writable Editor

A common pattern is two views bound to the same model key with different permissions.

- Detail page: `bindView("Detail", "RunVM", false)`
- Editor page: `bindView("Editor", "RunVM", true)`

This ensures read-only components can display state while only one editor surface can mutate model properties.

## Ownership Handover Pattern

When switching editable surfaces:

1. `releaseOwnership(previousViewId, key)`
2. `claimOwnership(nextViewId, key)`
3. execute write updates from next view only

This explicit handover avoids silent write denial and keeps ownership auditability.

## Debugging Write Failures

When `updateProperty*` returns `false`, inspect in order:

- `ViewModels.lastError`
- `ViewModels.bindings`
- `ViewModels.owners`
- whether target property exists on QObject

Most runtime failures come from missing ownership or typo in property name.
