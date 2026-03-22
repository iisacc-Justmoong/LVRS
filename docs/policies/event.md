# Event Policy

Goal: preserve deterministic interaction behavior by routing high-level reactions through a unified runtime event contract.

## Rule 1: Prefer `EventListener` over ad-hoc handlers

Cross-component interaction logic should use `EventListener` trigger semantics rather than duplicating raw `MouseArea`/`Keys` plumbing in each component.

Reason:
- central payload shape,
- consistent backend/runtime fallback,
- shared dedup behavior.

## Rule 2: Use runtime/gesture triggers for cross-surface behavior

For overlay dismissal, global context menu control, app-level interaction hooks, and mobile touch semantics that must survive nested view boundaries, use:

- `globalPressed`
- `globalContextRequested`
- `touchStarted`, `holdStarted`, `dragStarted`, `dragUpdated`, `dragEnded`
- `swipeDetected`, `nativeGestureDetected`

These triggers are resilient to nested local event boundaries and share the framework-managed payload contract.

## Rule 3: Incident payload is default; backend/input enrichment is opt-in

`EventListener` must stay incident-centric by default.
Enable `includeInputState` / `preferBackendState` only when the callback truly requires coherent input snapshots.

## Rule 4: Outside-dismiss must be coordinate based

Popup/dialog dismissal must use global-to-local coordinate mapping and geometry checks, not local-only click assumptions.

## Rule 5: Nested wheel isolation is mandatory

Any component that owns an internal scroll viewport inside a larger scrollable page must install wheel isolation (`WheelScrollGuard`) to avoid dual scrolling.

## Rule 6: Text composition safety is mandatory

All text entry surfaces must include `InputMethodGuard` so IME composition is committed safely on visibility/focus transitions.

## Rule 7: Dedup windows must remain explicit

Global press/context dedup thresholds (time and distance) are contract parameters. Any change must be documented and tested against context-menu behavior.

## Concrete Implementation Pattern

For overlays/popups, preferred pattern is:

1. local action trigger for open,
2. global trigger for outside dismiss,
3. coordinate-based geometry check,
4. explicit event consume strategy for nested scroll areas.

## Review Checklist

During code review, reject changes that:

- duplicate global event plumbing per page without `EventListener`,
- rely on local-only coordinates for global dismiss behavior,
- remove `WheelScrollGuard` from nested `Flickable` surfaces,
- disable IME composition guard on editable text controls.

## Enforcement Note

Event behavior must remain deterministic under high-frequency input.
If behavior changes with click speed or scroll burst, pipeline design is incomplete.

## Audit Checklist

- global behaviors use global triggers, not local click hacks,
- nested scroll surfaces include explicit wheel isolation,
- text-input components include IME guard integration.
