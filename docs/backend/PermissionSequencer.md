# PermissionSequencer

Location: `backend/runtime/permissionsequencer.h` / `backend/runtime/permissionsequencer.cpp`

`PermissionSequencer` runs app-defined permission request steps sequentially and stores request history.
It is intentionally generic: LVRS does not know about full-disk access, photo-library access, document folders,
or platform bridge APIs.

## Purpose

- Execute permission request steps in deterministic priority order.
- Stop on required failures by default.
- Keep an in-memory request history for diagnostics.
- Aggregate granted/denied/skipped/unavailable/failed counts.
- Provide structured result maps for developer tooling and logs.

## API

- `PermissionRequestSequencer::run(steps, options) -> PermissionRequestRunResult`
- `PermissionRequestSequencer::history()`
- `PermissionRequestSequencer::clearHistory()`

Core structs:

- `PermissionRequestStepContext`
- `PermissionRequestStep`
- `PermissionRequestStepResult`
- `PermissionRequestRunOptions`
- `PermissionRequestRunResult`

Status enum:

- `Granted`
- `Denied`
- `Skipped`
- `Unavailable`
- `Failed`

## Step Contract

`PermissionRequestStep` fields:

- `name`
- `priority`
- `required`
- `metadata`
- `request(context, details, errorMessage)`

The callback owns the platform/domain behavior. It can call native APIs, show app UI, inspect app state, or skip
a permission when it is not relevant. LVRS only records the returned status and details.

Required steps fail the run when they return `Denied`, `Unavailable`, or `Failed`. Optional steps with those
statuses are recorded but do not fail the run.

## Options

`PermissionRequestRunOptions` fields:

- `stopOnRequiredFailure`: default `true`.
- `appendHistory`: default `true`.
- `logDiagnostics`: default `true`, emits `LVRS bootstrap.permission.*` lines.
- `metadata`: copied into every step context.

## Result

`PermissionRequestRunResult` contains:

- `ok`
- `runId`
- `stoppedEarly`
- `stepResults`
- `errors`
- `elapsedMs`
- `completedCount`
- `grantedCount`
- `deniedCount`
- `skippedCount`
- `unavailableCount`
- `failedCount`
- `requiredFailure()`
- `errorMessage()`
- `diagnostics()`

Each step result includes:

- `name`, `index`, `priority`, `runId`
- `required`, `ok`, `status`
- `granted`, `terminalFailure`
- `errorMessage`
- `metadata`
- `details`
- `elapsedMs`

## Usage

```cpp
lvrs::PermissionRequestSequencer sequencer;

lvrs::PermissionRequestStep fullDisk;
fullDisk.name = QStringLiteral("full-disk-access");
fullDisk.priority = 10;
fullDisk.required = true;
fullDisk.request = [](const lvrs::PermissionRequestStepContext &, QVariantMap *details, QString *errorMessage) {
    const auto status = requestFullDiskAccessThroughAppBridge();
    if (details)
        details->insert(QStringLiteral("bridge"), QStringLiteral("macos"));
    if (status == AppPermissionStatus::Granted)
        return lvrs::PermissionRequestStatus::Granted;
    if (errorMessage)
        *errorMessage = QStringLiteral("Full disk access was not granted");
    return lvrs::PermissionRequestStatus::Denied;
};

lvrs::PermissionRequestStep photos;
photos.name = QStringLiteral("photo-library");
photos.priority = 20;
photos.required = false;
photos.request = [](const lvrs::PermissionRequestStepContext &, QVariantMap *, QString *) {
    return lvrs::PermissionRequestStatus::Skipped;
};

const lvrs::PermissionRequestRunResult result = sequencer.run({fullDisk, photos});
if (!result.ok)
    qWarning().noquote() << result.errorMessage();
```

Foreground service example:

```cpp
lvrs::ForegroundServiceTask permissionBootstrap;
permissionBootstrap.name = QStringLiteral("permission-bootstrap");
permissionBootstrap.start = [&sequencer](const lvrs::ForegroundServiceStartContext &, QString *errorMessage) {
    const lvrs::PermissionRequestRunResult result =
        sequencer.run(buildPermissionStepsForCurrentPlatform());
    if (!result.ok && errorMessage)
        *errorMessage = result.errorMessage();
    return result.ok;
};
```

## Responsibility Boundary

LVRS owns:

- step ordering,
- sequential execution,
- history storage,
- status/count aggregation,
- generic diagnostics.

The app owns:

- permission names,
- native permission calls,
- prompt UI,
- platform bridge functions,
- whether an app-specific permission is required.
