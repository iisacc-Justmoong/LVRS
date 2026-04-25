# BootstrapParallel

Location: `backend/runtime/bootstrapparallel.h` / `backend/runtime/bootstrapparallel.cpp`

`BootstrapParallel` is a small runtime executor for loading independent startup domains in parallel,
then applying the collected results on a chosen QObject thread.

It is intended for app bootstrap phases where the expensive work is independent, but the final mutation
must happen on the main/UI thread.

## Purpose

- Fan out independent load tasks onto a bounded worker pool.
- Collect typed `QVariant` payloads and per-task diagnostics.
- Apply successful results in deterministic priority order.
- Route apply callbacks through a receiver thread, usually `QGuiApplication` or `QQmlApplicationEngine`.
- Distinguish normal failure from fatal bootstrap failure.

## API

- `lvrs::runBootstrapParallelTasks(tasks, options) -> BootstrapParallelRunResult`

Core structs:

- `BootstrapParallelTaskContext`
- `BootstrapParallelTask`
- `BootstrapParallelTaskResult`
- `BootstrapParallelRunOptions`
- `BootstrapParallelRunResult`

## Task Contract

`BootstrapParallelTask` fields:

- `name`
- `priority`
- `fatal`
- `metadata`
- `load(context, value, errorMessage)`
- `apply(result, errorMessage)`

`load` runs on a worker thread. It should avoid touching QML objects, QObject trees owned by the UI thread,
or ViewModels directly. Return domain data through `value`.

`apply` runs after all loads complete. If `BootstrapParallelRunOptions::applyReceiver` is set and the caller is
not already on that receiver thread, LVRS invokes the callback with `Qt::BlockingQueuedConnection`.

By default, `apply` is skipped for failed loads.

## Options

`BootstrapParallelRunOptions` fields:

- `applyReceiver`: QObject whose thread owns apply callbacks. `nullptr` means caller thread.
- `maxThreadCount`: bounded worker count. `0` uses `QThread::idealThreadCount()`.
- `skipApplyOnLoadFailure`: default `true`.
- `logDiagnostics`: default `true`, emits `LVRS bootstrap.parallel.*` lines.

## Result

`BootstrapParallelRunResult` contains:

- `ok`
- `taskResults`
- `errors`
- `elapsedMs`
- `fatalFailure()`
- `errorMessage()`
- `diagnostics()`

Each task result includes:

- `ok`, `loadOk`, `applied`, `applyOk`
- `errorMessage`, `applyErrorMessage`
- `value`
- `metadata`
- `loadElapsedMs`, `applyElapsedMs`

## Usage

```cpp
lvrs::BootstrapParallelTask libraryTask;
libraryTask.name = QStringLiteral("library");
libraryTask.priority = 10;
libraryTask.metadata = {{QStringLiteral("domain"), QStringLiteral("library")}};
libraryTask.load = [](const lvrs::BootstrapParallelTaskContext &, QVariant *value, QString *errorMessage) {
    QVariantMap snapshot = loadLibrarySnapshot();
    if (snapshot.isEmpty()) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Library snapshot is empty");
        return false;
    }
    if (value)
        *value = snapshot;
    return true;
};
libraryTask.apply = [libraryViewModel](const lvrs::BootstrapParallelTaskResult &result, QString *) {
    libraryViewModel->applySnapshot(result.value.toMap());
    return true;
};

lvrs::BootstrapParallelRunOptions options;
options.applyReceiver = qGuiApp;
options.maxThreadCount = 4;

const lvrs::BootstrapParallelRunResult result =
    lvrs::runBootstrapParallelTasks({libraryTask}, options);
if (result.fatalFailure())
    return false;
```

Lifecycle hook example:

```cpp
lvrs::QmlBootstrapTask loadDomains;
loadDomains.name = QStringLiteral("load-domains");
loadDomains.stage = lvrs::QmlAppLifecycleStage::AfterFirstIdle;
loadDomains.run = [](const lvrs::QmlAppLifecycleContext &context, QString *errorMessage) {
    lvrs::BootstrapParallelRunOptions options;
    options.applyReceiver = context.application;

    const lvrs::BootstrapParallelRunResult result =
        lvrs::runBootstrapParallelTasks(buildDomainTasks(), options);
    if (!result.ok && errorMessage)
        *errorMessage = result.errorMessage();
    return result.ok;
};
```

## Responsibility Boundary

LVRS owns:

- task fan-out,
- bounded worker execution,
- deterministic result/apply ordering,
- apply-thread invocation,
- timing and failure diagnostics.

The app owns:

- domain selection,
- file/database/network parsing,
- payload schema,
- ViewModel mutation rules,
- whether a failed domain is fatal.
