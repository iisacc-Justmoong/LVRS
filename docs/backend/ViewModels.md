# ViewModels

Location:

- `backend/state/viewmodel.h` / `backend/state/viewmodel.cpp`
- `backend/state/viewmodelregistry.h` / `backend/state/viewmodelregistry.cpp`

`ViewModel` is the C++ base type for dedicated ViewModel objects.
`ViewModels` is the singleton registry and ownership gate for MVVM model objects.

## Purpose

- Give C++ ViewModels a consistent diagnostic surface.
- Register model objects by key.
- Bind view ids to keys.
- Enforce single-writer ownership semantics.
- Expose descriptors that developer tooling can inspect without knowing domain classes.

## C++ ViewModel Base

Apps should prefer C++ ViewModel classes that derive from `ViewModel` when they need a stable MVVM contract.
Domain-specific state and commands still live in the app subclass.

Base properties:

- `key: string`
- `displayName: string`
- `busy: bool`
- `error: string`
- `hasError: bool`
- `metadata: map`

Base methods:

- `clearError()`
- `snapshot()`

The base type is registered to QML as an uncreatable C++ type. QML consumes concrete instances registered by the app bootstrap; it should not instantiate the base type.

## Properties

- `keys: stringList`
- `views: stringList`
- `bindings: map`
- `owners: map`
- `descriptors: map`
- `lastError: string`

## Registration APIs

- `set(key, object)`
- `registerViewModel(object, fallbackKey = "")`
- `get(key)`
- `remove(key)`
- `clear()`

`registerViewModel()` is the preferred C++ bootstrap entrypoint for ViewModel objects. It resolves the key from the explicit fallback first, then from `ViewModel::key`, stores the object, and starts descriptor observation.

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

## Descriptor APIs

- `descriptor(key)`
- `descriptors`

Descriptor maps include:

- `key`
- `className`
- `owner`
- `views`
- `viewModel`
- `viewModelKey`, `displayName`, `busy`, `error`, `hasError`, `metadata` for objects derived from `ViewModel`

`descriptorsChanged` is emitted when registered ViewModel diagnostics change, when keys are added/removed, or when view bindings/ownership change.

## Write Guard Semantics

Write updates fail when:

- view id is empty,
- view has no binding,
- view is not owner of target key,
- key/object/property is invalid.

On failure, API returns `false` and updates `lastError`.

## Usage Example

```cpp
class DashboardViewModel : public ViewModel
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    // Domain state and commands omitted.
};

auto *vm = new DashboardViewModel(&engine);
vm->setKey(QStringLiteral("Dashboard"));
vm->setDisplayName(QStringLiteral("Dashboard"));
vm->setMetadata({{QStringLiteral("domain"), QStringLiteral("dashboard")}});

auto *registry = engine.singletonInstance<ViewModelRegistry *>(QStringLiteral("LVRS"),
                                                               QStringLiteral("ViewModels"));
registry->registerViewModel(vm);
registry->bindView(QStringLiteral("DashboardPage"), QStringLiteral("Dashboard"), true);
```

QML should consume the object through the registry:

```qml
import LVRS 1.0 as LV

property var vm: LV.ViewModels.getForView("DashboardPage")
```

## Internal Behavior Notes

- Tokens are trim-normalized.
- Stale bindings/owners are pruned when keys vanish.
- Objects parented by registry may be auto-disposed when no key references remain.
- Descriptor observation is disconnected when an object is no longer referenced by a registry key.

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
