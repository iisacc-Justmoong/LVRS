# ViewModels

Location: `backend/state/viewmodelregistry.h` / `backend/state/viewmodelregistry.cpp`

`ViewModels` is the singleton registry and ownership gate for MVVM model objects.

## Purpose

- Register model objects by key.
- Bind view ids to keys.
- Enforce single-writer ownership semantics.

## Properties

- `keys: stringList`
- `views: stringList`
- `bindings: map`
- `owners: map`
- `lastError: string`

## Registration APIs

- `set(key, object)`
- `get(key)`
- `remove(key)`
- `clear()`

## Binding and Ownership APIs

- `bindView(viewId, key, writable = false)`
- `unbindView(viewId)`
- `getForView(viewId)`
- `keyForView(viewId)`
- `claimOwnership(viewId, key)`
- `releaseOwnership(viewId, key?)`
- `canWrite(viewId, key?)`
- `ownerOf(key)`

## Property Access APIs

- `updateProperty(viewId, property, value)`
- `updatePropertyByKey(viewId, key, property, value)`
- `readProperty(viewId, property)`

## Write Guard Semantics

Write updates fail when:

- view id is empty,
- view has no binding,
- view is not owner of target key,
- key/object/property is invalid.

On failure, API returns `false` and updates `lastError`.

## Usage Example

```qml
import LVRS 1.0 as LV

Component.onCompleted: {
    LV.ViewModels.set("Dashboard", dashboardVm)
    LV.ViewModels.bindView("DashboardPage", "Dashboard", true)
}

function setStatus(nextStatus) {
    if (!LV.ViewModels.updateProperty("DashboardPage", "status", nextStatus))
        console.warn(LV.ViewModels.lastError)
}
```

## Internal Behavior Notes

- Tokens are trim-normalized.
- Stale bindings/owners are pruned when keys vanish.
- Objects parented by registry may be auto-disposed when no key references remain.

## Extended Example: Explicit Ownership Transfer

```qml
import LVRS 1.0 as LV

function transferOwner(fromView, toView, key) {
    LV.ViewModels.releaseOwnership(fromView, key)
    if (!LV.ViewModels.claimOwnership(toView, key))
        console.warn(LV.ViewModels.lastError)
}
```

## Review Checklist

- every writable view must have explicit ownership claim,
- read-only views should bind with `writable=false`,
- `lastError` must be surfaced in developer tooling for failed writes.

## FAQ

Q. Can two views write to one key simultaneously?  
A. No. Ownership model is single-writer by contract.
