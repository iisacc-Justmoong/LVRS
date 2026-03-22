# Overview

LVRS is a Qt 6.5+ QML framework centered on deterministic UI behavior, explicit runtime observability, and strict integration boundaries between view, navigation, and model ownership.

## Design Goals

- Stable component contracts across control, layout, navigation, and surfaces.
- Backend-first runtime observability exposed as QML-friendly APIs.
- Deterministic rendering bootstrap by explicit platform/backend policy.
- Safe MVVM write ownership to prevent accidental shared-state races.

## Runtime Architecture

LVRS runtime is split into three cooperating layers.

1. Event capture daemon (`RuntimeEvents`)  
   Captures keyboard, pointer, context, touch/tablet/native-gesture, UI lifecycle, and process telemetry.
2. High-level gesture recognizer (`GestureEvents`)
   Classifies the raw touch/native stream into `hold`, `drag`, `swipe`, and normalized touch-session payloads.
3. Backend cache/bridge (`Backend`)
   Mirrors the raw runtime event stream into a bounded cache and provides stable snapshots for QML consumers.

This architecture exists so UI-layer logic can consume either low-level runtime telemetry or high-level gesture semantics without collapsing both responsibilities into one mutable stream.

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
- Interactive page transitions can preview forward/backward movement while deferring route commit until finish/cancel.
- Global context/click signals can be consumed at app root through `ApplicationWindow` event bridge.
- Nested wheel behavior can be isolated by `WheelScrollGuard`.
- IME composition integrity can be enforced by `InputMethodGuard`.
- High-level touch/gesture semantics can be consumed through `GestureEvents` or `EventListener` gesture triggers.

## Recommended Entry Documents by Use Case

- Runtime event integration: `docs/backend/RuntimeEvents.md`, `docs/backend/GestureEvents.md`, `docs/components/control/EventListener.md`.
- Navigation and route model binding: `docs/components/navigation/PageRouter.md`, `docs/mvvm.md`.
- Interactive navigation driving: `docs/components/navigation/PageRouter.md`, `docs/components/navigation/PageTransitionController.md`.
- Platform/backend behavior: `docs/architecture/rendering-backend.md`, `docs/backend/Platform.md`.
- Logging and diagnostics: `docs/backend/Debug.md`, `docs/backend/DebugOutput.md`.

## End-to-End Integration Scenario

A typical production flow uses all runtime layers in sequence:

1. Bootstrap app and rendering policy (`AppBootstrap`).
2. Start and attach runtime daemon (`RuntimeEvents`).
3. Attach `GestureEvents` when the feature requires high-level touch semantics.
4. Hook backend event mirror (`Backend.hookUserEvents()`).
5. Mount `ApplicationWindow` with `PageRouter` routes.
6. Bind route metadata to `ViewModels` ownership.
7. Handle global context/pressed/gesture events via `EventListener`.

This chain provides deterministic behavior from startup through UI interaction.

## Common Integration Mistakes

- Starting `RuntimeEvents` without attaching a window, then expecting UI hit-test data.
- Expecting raw `RuntimeEvents` touch records to already distinguish hold/drag/swipe semantics.
- Writing model properties from views that never claimed ownership in `ViewModels`.
- Using nested `Flickable` surfaces without `WheelScrollGuard`.
- Building context-menu close logic only from local click handlers instead of global coordinates.

## Verification Checklist

After integrating LVRS in a new app, validate:

- `RuntimeEvents.running == true`
- `GestureEvents.runtimeAttached == true` when gesture triggers are used outside `EventListener`
- `Backend.userEventHooked == true` when backend-first listeners are used
- route transitions update `PageRouter.currentPath` and `ViewStateTracker.snapshot()` as expected
- debug output includes expected event domains without uncontrolled flood
