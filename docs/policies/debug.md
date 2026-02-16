# Debug Policy

Goal: provide centralized diagnostics without changing runtime behavior or degrading UX.

## Rule 1: Route application logs through `Debug`

Preferred API:

- `LV.Debug.log(component, event, data?)`
- `LV.Debug.warn(component, event, data?)`
- `LV.Debug.error(component, event, data?)`

## Rule 2: Logging is opt-in

`Debug.enabled` defaults to `false`. Production flows must remain silent unless explicitly configured.

## Rule 3: Runtime echo is rate-controlled

When runtime echo is enabled, respect:

- minimum echo interval (`runtimeEchoMinIntervalMs`)
- exclude type list (`runtimeEchoExcludeTypes`)
- stdout threshold (`stdoutMinimumLevel`)

This prevents flood from high-frequency events.

## Rule 4: Logging must never block UI paths

Logging must not introduce synchronous blocking work in interactive event handlers.

## Rule 5: Filtering belongs in logger, not call sites

Use logger filters (`levelFilter`, `componentFilter`, `textFilter`) instead of scattering conditional logging logic in UI code.

## Rule 6: Schema stability is mandatory

If output field names or entry schema changes, update `docs/backend/DebugOutput.md` in the same change set.

## Production Debug Strategy

Recommended environment split:

- local/dev: `Debug.enabled=true`, selective runtime capture on
- staging: capture on with tighter filters and rate limits
- production: default disabled, enable only for incident windows

## Incident Triage Workflow

When investigating runtime incidents:

1. enable logger with strict filters (`WARN/ERROR` first),
2. attach runtime events,
3. capture bounded entries and summary snapshot,
4. export data with timestamps and sequence ids,
5. disable logger after capture window closes.

## Enforcement Note

Any feature that requires logs for correctness is violating policy.
Logging is diagnostic, not functional dependency.

## Audit Checklist

- all runtime/UI diagnostic logs are emitted through `Debug`,
- no feature flow depends on log side effects,
- rate limiting and filters are configured in high-volume environments.
