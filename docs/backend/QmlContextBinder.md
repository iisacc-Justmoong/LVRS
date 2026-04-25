# QmlContextBinder

Location: `backend/runtime/qmlcontextbinder.h` / `backend/runtime/qmlcontextbinder.cpp`

`QmlContextBinder` applies a declared C++ object exposure plan to a `QQmlApplicationEngine`.
It covers two bootstrap jobs that downstream apps often repeat by hand:

- set C++ objects as QML context properties,
- register C++ ViewModels into `LV.ViewModels` and optionally expose them as context properties.

## API

- `lvrs::applyQmlContextBindPlan(engine, plan) -> QmlContextBindResult`

Plan structures:

- `QmlContextObjectBinding`
- `QmlViewModelBinding`
- `QmlContextBindPlan`
- `QmlContextBindResult`

## Context Object Binding

`QmlContextObjectBinding` fields:

- `contextName`
- `object`
- `required` (default `true`)

Required null objects are reported as errors. Optional null objects are skipped.

## ViewModel Binding

`QmlViewModelBinding` fields:

- `key`
- `object`
- `contextName`
- `displayName`
- `metadata`
- `viewId`
- `writable`
- `required` (default `true`)

If the object derives from `ViewModel`, the binder uses the object's `key` when `key` is omitted, and applies `displayName` / `metadata` before registration.

If `viewId` is set, the binder also calls:

```cpp
ViewModels.bindView(viewId, key, writable)
```

This keeps ownership policy in the C++ bootstrap plan instead of scattering it across QML startup code.

## Result

`QmlContextBindResult` reports:

- `ok`
- `errors`
- `contextNames`
- `viewModelKeys`
- `errorMessage()`

## Usage

```cpp
lvrs::QmlContextBindPlan plan;

lvrs::QmlContextObjectBinding services;
services.contextName = QStringLiteral("workspaceServices");
services.object = workspaceServices;
plan.contextObjects.append(services);

lvrs::QmlViewModelBinding libraryVm;
libraryVm.key = QStringLiteral("Library");
libraryVm.object = libraryViewModel;
libraryVm.contextName = QStringLiteral("libraryViewModel");
libraryVm.displayName = QStringLiteral("Library");
libraryVm.viewId = QStringLiteral("LibraryView");
libraryVm.writable = true;
plan.viewModels.append(libraryVm);

const lvrs::QmlContextBindResult result =
    lvrs::applyQmlContextBindPlan(engine, plan);
if (!result.ok)
    qWarning().noquote() << result.errorMessage();
```

## Responsibility Boundary

LVRS owns:

- validation,
- context property assignment,
- `ViewModels` singleton registration,
- optional view binding and ownership claim,
- error aggregation.

The app owns:

- object construction,
- domain keys,
- domain-specific ownership policy,
- data loading and mutation semantics.
