# NavigationStackModel

Location: `backend/navigation/navigationstackmodel.h`, `backend/navigation/navigationstackmodel.cpp`

`NavigationStackModel` owns path-stack mutation and view-tracking derivation for `PageRouter.qml`.

## Purpose

- Normalize path entries before they enter the page stack.
- Build stack results for push, replace, set-root, pop, and pop-to-root operations.
- Preserve component-based stack entries alongside path-based entries.
- Derive current path/params and view-tracking metadata from the committed stack.

## API

Input:

- `path`

Readonly:

- `currentPath`
- `currentParams`
- `viewTrackingEntries`
- `trackedViewIds`
- `depth`

Methods:

- `normalizePath(pathValue)`
- `createPathEntry(pathValue, params)`
- `createComponentPathEntry(component, params)`
- `stackAfterPathOperation(pathValue, params, mode)`
- `stackAfterComponentOperation(component, params, mode)`
- `stackAfterPop()`
- `stackAfterPopToRoot()`
- `applyPathOperation(pathValue, params, mode)`
- `applyComponentOperation(component, params, mode)`
- `pop()`
- `popToRoot()`
- `currentEntryDescriptor()`
- `createViewTrackingEntry(entry, index)`
- `buildViewTrackingEntries(pathValue?)`
- `updateTrackedViewIds(entries)`

## Stack Entry Contract

Path entry:

- `path`
- `params`

Component entry:

- `path` as an empty string
- `params`
- `component`

View-tracking entry:

- `viewId`
- `path`
- `enabled`

## How It Works

- Path normalization delegates to `RouteMatcher`.
- `mode == "set"` replaces the entire stack with one entry.
- `mode == "replace"` replaces the current entry.
- Any other mode appends a new entry.
- `currentPath` and `currentParams` are derived from the last stack entry.
- Component entries receive generated view ids in the form `_component_<index>` unless a `viewId` is provided.
- Entries with `enabled: false`, `disabled: true`, or `params.disabled: true` are marked disabled for view tracking.
- `updateTrackedViewIds` returns ids that disappeared from the latest tracking set so QML can release snapshots.

## QML Boundary

`PageRouter.qml` still owns `StackView` operations, route resolution, and interactive transition orchestration. `NavigationStackModel` owns committed stack math, normalized entry creation, current entry derivation, and view-tracking entry generation.
