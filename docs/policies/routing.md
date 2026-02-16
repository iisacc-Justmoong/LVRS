# Routing Policy

Goal: ensure predictable stack navigation with explicit path semantics and reversible state.

## Rule 1: Route path grammar

Supported path shapes:

- static: `/reports`
- param: `/runs/[id]`
- rest: `/logs/[...path]`

Paths are normalized to leading slash and no trailing slash (except root).

## Rule 2: `PageRouter.path` is the source of stack truth

Navigation operations (`go`, `replace`, `setRoot`, `pop`, `popToRoot`) must update stack state and current route state consistently.

## Rule 3: Back semantics are stack-only

Undo navigation is represented by:

- `pop()`
- `popToRoot()`

Any custom "back" behavior must be modeled as explicit stack mutation.

## Rule 4: Component navigation is out-of-band unless encoded

`goTo(component)` and related component-target operations are allowed, but they do not carry path identity unless encoded into `path` entries.

## Rule 5: Route-level model metadata should be colocated

If a page requires model binding/ownership, prefer route metadata (`viewModelKey`, `viewId`, `writable`) over per-page ad-hoc binding.

## Rule 6: Not-found fallback must be explicit

When route resolution fails, only `notFoundComponent` or `notFoundSource` may produce fallback navigation. Otherwise `navigationFailed(path)` must fire.

## Rule 7: Global navigator ownership is explicit

Only the intended active router should register as global navigator. Nested routers must define registration behavior deliberately.

## Route Change Review Checklist

When modifying routes:

1. verify path normalization behavior is unchanged,
2. verify dynamic/rest params still resolve correctly,
3. verify not-found behavior remains explicit,
4. verify `path` stack sync on replace/set/pop flows,
5. verify route-level MVVM metadata still binds as intended.

## Recommended Route Definition Pattern

Prefer colocating route documentation near route objects, including:

- path intent,
- expected params,
- view model key,
- writable ownership requirement.

This significantly reduces routing regressions during feature growth.

## Enforcement Note

Route transitions must be reproducible from `path` state alone.
If additional hidden state is required to restore navigation, routing design is underspecified.

## Audit Checklist

- every route has clear ownership and purpose,
- dynamic/rest segment parsing is covered by integration tests,
- stack mutations (`push/replace/set/pop`) remain reversible and traceable.
