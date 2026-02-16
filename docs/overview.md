# Overview

LVRS is a Qt 6.5+ QML framework centered on deterministic UI behavior, explicit runtime observability, and strict integration boundaries between view, navigation, and model ownership.

## Design Goals

- Stable component contracts across control, layout, navigation, and surfaces.
- Backend-first runtime observability exposed as QML-friendly APIs.
- Deterministic rendering bootstrap by explicit platform/backend policy.
- Safe MVVM write ownership to prevent accidental shared-state races.

## Runtime Architecture

LVRS runtime is split into two cooperating layers.

1. Event capture daemon (`RuntimeEvents`)  
   Captures keyboard, pointer, context, touch/tablet/gesture, UI lifecycle, and process telemetry.
2. Backend cache/bridge (`Backend`)  
   Mirrors runtime event stream into a bounded cache and provides stable snapshots for QML consumers.

This architecture exists so UI-layer logic can consume coherent snapshots under event bursts instead of racing mutable real-time streams.

## UI Architecture

QML modules are grouped by concern.

- `control`: inputs, display controls, selection controls, and behavior guards.
- `layout`: stack primitives and app header.
- `navigation`: router, global navigator, hierarchy, context menu.
- `surfaces`: card/dialog style containers.
- `app`: root window shell with adaptive navigation bridge.

## Startup Sequence

A production startup path should follow this order.

1. `lvrs::preApplicationBootstrap(options)` before `QGuiApplication`.
2. Construct `QGuiApplication`.
3. `lvrs::postApplicationBootstrap(app, options)`.
4. Load QML root (`LV.ApplicationWindow`).
5. Optionally enable runtime bridges:
   - `RuntimeEvents.start() + attachWindow(window)`
   - `Backend.hookUserEvents()`

## Key Runtime Guarantees

- Route transitions update `PageRouter.path`, current route state, and optional `ViewStateTracker` sync.
- Global context/click signals can be consumed at app root through `ApplicationWindow` event bridge.
- Nested wheel behavior can be isolated by `WheelScrollGuard`.
- IME composition integrity can be enforced by `InputMethodGuard`.

## Recommended Entry Documents by Use Case

- Runtime event integration: `docs/backend/RuntimeEvents.md`, `docs/components/control/EventListener.md`.
- Navigation and route model binding: `docs/components/navigation/PageRouter.md`, `docs/mvvm.md`.
- Platform/backend behavior: `docs/architecture/rendering-backend.md`, `docs/backend/Platform.md`.
- Logging and diagnostics: `docs/backend/Debug.md`, `docs/backend/DebugOutput.md`.

## End-to-End Integration Scenario

A typical production flow uses all runtime layers in sequence:

1. Bootstrap app and rendering policy (`AppBootstrap`).
2. Start and attach runtime daemon (`RuntimeEvents`).
3. Hook backend event mirror (`Backend.hookUserEvents()`).
4. Mount `ApplicationWindow` with `PageRouter` routes.
5. Bind route metadata to `ViewModels` ownership.
6. Handle global context/pressed events via `EventListener`.

This chain provides deterministic behavior from startup through UI interaction.

## Common Integration Mistakes

- Starting `RuntimeEvents` without attaching a window, then expecting UI hit-test data.
- Writing model properties from views that never claimed ownership in `ViewModels`.
- Using nested `Flickable` surfaces without `WheelScrollGuard`.
- Building context-menu close logic only from local click handlers instead of global coordinates.

## Verification Checklist

After integrating LVRS in a new app, validate:

- `RuntimeEvents.running == true`
- `Backend.userEventHooked == true` when backend-first listeners are used
- route transitions update `PageRouter.currentPath` and `ViewStateTracker.snapshot()` as expected
- debug output includes expected event domains without uncontrolled flood
