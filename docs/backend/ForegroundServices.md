# ForegroundServices

Location: `backend/runtime/foregroundservices.h` / `backend/runtime/foregroundservices.cpp`

`ForegroundServices` provides a one-shot gate for starting app services after a visible workspace window exists.
It is intended for startup work such as schedulers, monitors, or permission bootstrap entrypoints that must not
start repeatedly while QML roots reload or lifecycle hooks run more than once.

## Purpose

- Detect whether loaded QML roots include at least one visible `QWindow`.
- Run foreground service start callbacks once.
- Preserve deterministic task ordering through `priority`.
- Aggregate service start diagnostics.
- Distinguish recoverable failures from fatal startup failures.

## API

- `visibleWorkspaceWindows(rootLoadResult)`
- `hasVisibleWorkspace(rootLoadResult)`
- `ForegroundServiceGate::startOnceWhenWorkspaceVisible(context, tasks, options)`
- `ForegroundServiceGate::reset()`

Core structs:

- `ForegroundServiceStartContext`
- `ForegroundServiceTask`
- `ForegroundServiceTaskResult`
- `ForegroundServiceStartOptions`
- `ForegroundServiceStartResult`

## Task Contract

`ForegroundServiceTask` fields:

- `name`
- `priority`
- `fatal`
- `metadata`
- `start(context, errorMessage)`

Tasks run in ascending priority order. The gate marks itself started before running callbacks, so repeated calls
do not duplicate foreground services even if one service reports failure. Use `reset()` only when a caller has an
explicit retry policy.

`ForegroundServiceStartContext` exposes:

- `application`
- `engine`
- `rootLoadResult`
- `visibleWindows`
- `metadata`

## Options

`ForegroundServiceStartOptions` fields:

- `requireVisibleWorkspace`: default `true`; if no visible window exists, start is rejected and the gate remains retryable.
- `logDiagnostics`: default `true`; emits `LVRS bootstrap.foreground.*` lines.
- `metadata`: copied into the start context.

## Result

`ForegroundServiceStartResult` contains:

- `ok`
- `started`
- `alreadyStarted`
- `visibleWorkspace`
- `visibleWindowCount`
- `taskResults`
- `errors`
- `elapsedMs`
- `fatalFailure()`
- `errorMessage()`
- `diagnostics()`

## Usage

```cpp
auto foregroundGate = std::make_shared<lvrs::ForegroundServiceGate>();

lvrs::ForegroundServiceTask schedulerStart;
schedulerStart.name = QStringLiteral("async-scheduler");
schedulerStart.priority = 10;
schedulerStart.fatal = true;
schedulerStart.start = [scheduler](const lvrs::ForegroundServiceStartContext &, QString *errorMessage) {
    if (scheduler->start())
        return true;
    if (errorMessage)
        *errorMessage = QStringLiteral("Scheduler failed to start");
    return false;
};

lvrs::QmlBootstrapTask foregroundServices;
foregroundServices.name = QStringLiteral("foreground-services");
foregroundServices.stage = lvrs::QmlAppLifecycleStage::AfterWindowActivated;
foregroundServices.fatal = true;
foregroundServices.run = [foregroundGate, schedulerStart](const lvrs::QmlAppLifecycleContext &context,
                                                          QString *errorMessage) {
    const lvrs::ForegroundServiceStartResult result =
        foregroundGate->startOnceWhenWorkspaceVisible(context, {schedulerStart});
    if (!result.ok && errorMessage)
        *errorMessage = result.errorMessage();
    return result.ok;
};
```

## Responsibility Boundary

LVRS owns:

- visible-window detection,
- one-shot gate state,
- deterministic service start ordering,
- failure/fatal diagnostics,
- bootstrap log lines.

The app owns:

- which services exist,
- when services are considered fatal,
- scheduler internals,
- permission prompts,
- platform/domain-specific startup policy.
