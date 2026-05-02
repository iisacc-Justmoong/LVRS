# StateModel

Location: `backend/state/statemodel.h` / `backend/state/statemodel.cpp`

`StateModel` is the first concrete C++ state container for backend-driven LVRS components. It derives from `ViewModel`, so it can be registered through `ViewModels` and bound to views with the same ownership rules.

## Purpose

- Move component state out of QML into a C++ QObject.
- Provide a small key-value state surface for incremental component migration.
- Expose deterministic change notification through `revision`, `valuesChanged`, and `valueChanged`.
- Keep state objects compatible with `ViewModels` descriptors and ownership gates.

## API

Properties:

- `values: map`
- `stateKeys: stringList`
- `revision: int`
- `empty: bool`

Methods:

- `value(key, fallbackValue = undefined)`
- `valueOr(key, fallbackValue)`
- `hasValue(key)`
- `setValue(key, value)`
- `removeValue(key)`
- `applyPatch(patch)`
- `clearValues()`
- `stateSnapshot()`

Signals:

- `valuesChanged()`
- `stateKeysChanged()`
- `revisionChanged()`
- `valueChanged(key, value, previousValue)`

## Behavior Contract

- Keys are trim-normalized.
- Empty keys are rejected and set `error` to `Empty state key`.
- `setValue()` changes one key and increments `revision` only when the value actually changes.
- `applyPatch()` merges non-empty patch keys into current state.
- `clearValues()` removes all state and emits per-key `valueChanged` notifications.
- `stateSnapshot()` includes the base `ViewModel` snapshot plus `values`, `stateKeys`, `revision`, and `empty`.
- `ViewModels.descriptor(key)` marks registered instances with `stateModel=true` and includes current state fields.

## Usage

```qml
import LVRS 1.0 as LV

LV.StateModel {
    id: progressState
    values: ({
        minimumValue: 0,
        maximumValue: 100,
        startValue: 0,
        currentValue: 64
    })
}

LV.ProgressBar {
    stateModel: progressState
}
```

## Migration Policy

Use `StateModel` for component state that does not yet deserve a domain-specific `ViewModel` subclass. Once behavior becomes domain-specific, move from key-value state to a typed C++ `ViewModel` with explicit `Q_PROPERTY` fields and commands.
