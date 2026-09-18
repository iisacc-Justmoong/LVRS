#include "backend/runtime/foregroundservices.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QWindow>

#include <algorithm>
#include <exception>

namespace {

QString compactJson(const QVariant &value)
{
    const QByteArray json = QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
    if (!json.isEmpty())
        return QString::fromUtf8(json);
    return value.toString();
}

void logBootstrapEvent(const QString &event, const QVariantMap &payload, QtMsgType type = QtInfoMsg)
{
    const QString line = QStringLiteral("LVRS bootstrap.%1 %2").arg(event, compactJson(payload));
    switch (type) {
    case QtWarningMsg:
        qWarning().noquote() << line;
        break;
    case QtCriticalMsg:
        qCritical().noquote() << line;
        break;
    default:
        qInfo().noquote() << line;
        break;
    }
}

QString normalizedTaskName(const lvrs::ForegroundServiceTask &task, int index)
{
    const QString name = task.name.trimmed();
    if (!name.isEmpty())
        return name;
    return QStringLiteral("foreground-service-%1").arg(index);
}

QVariantMap serviceTaskSummary(const lvrs::ForegroundServiceTaskResult &result)
{
    QVariantMap payload;
    payload.insert(QStringLiteral("name"), result.name);
    payload.insert(QStringLiteral("index"), result.index);
    payload.insert(QStringLiteral("priority"), result.priority);
    payload.insert(QStringLiteral("fatal"), result.fatal);
    payload.insert(QStringLiteral("ok"), result.ok);
    payload.insert(QStringLiteral("elapsedMs"), result.elapsedMs);
    if (!result.errorMessage.isEmpty())
        payload.insert(QStringLiteral("error"), result.errorMessage);
    if (!result.metadata.isEmpty())
        payload.insert(QStringLiteral("metadata"), result.metadata);
    return payload;
}

void appendError(lvrs::ForegroundServiceStartResult *startResult,
                 const lvrs::ForegroundServiceTaskResult &taskResult)
{
    if (!startResult || taskResult.ok)
        return;

    startResult->ok = false;
    startResult->errors.append(QStringLiteral("%1: %2").arg(taskResult.name, taskResult.errorMessage));
}

} // namespace

namespace lvrs {

QVariantMap ForegroundServiceTaskResult::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("name"), name);
    map.insert(QStringLiteral("index"), index);
    map.insert(QStringLiteral("priority"), priority);
    map.insert(QStringLiteral("fatal"), fatal);
    map.insert(QStringLiteral("ok"), ok);
    map.insert(QStringLiteral("errorMessage"), errorMessage);
    map.insert(QStringLiteral("metadata"), metadata);
    map.insert(QStringLiteral("elapsedMs"), elapsedMs);
    return map;
}

bool ForegroundServiceStartResult::fatalFailure() const
{
    for (const ForegroundServiceTaskResult &taskResult : taskResults) {
        if (!taskResult.ok && taskResult.fatal)
            return true;
    }
    return false;
}

QString ForegroundServiceStartResult::errorMessage() const
{
    return errors.join(QStringLiteral("; "));
}

QVariantList ForegroundServiceStartResult::diagnostics() const
{
    QVariantList list;
    list.reserve(taskResults.size());
    for (const ForegroundServiceTaskResult &taskResult : taskResults)
        list.append(taskResult.toVariantMap());
    return list;
}

QList<QWindow *> visibleWorkspaceWindows(const QmlRootLoadResult &rootLoadResult)
{
    QList<QWindow *> windows;
    for (QWindow *window : rootLoadResult.windows) {
        if (window && window->isVisible())
            windows.append(window);
    }
    return windows;
}

bool hasVisibleWorkspace(const QmlRootLoadResult &rootLoadResult)
{
    return !visibleWorkspaceWindows(rootLoadResult).isEmpty();
}

ForegroundServiceGate::ForegroundServiceGate(QObject *parent)
    : QObject(parent)
{
}

bool ForegroundServiceGate::started() const
{
    return m_started;
}

int ForegroundServiceGate::startAttemptCount() const
{
    return m_startAttemptCount;
}

void ForegroundServiceGate::reset()
{
    m_started = false;
    m_startAttemptCount = 0;
}

ForegroundServiceStartResult ForegroundServiceGate::startOnceWhenWorkspaceVisible(
    const QmlAppLifecycleContext &lifecycleContext,
    const QList<ForegroundServiceTask> &tasks,
    const ForegroundServiceStartOptions &options)
{
    ForegroundServiceStartResult result;

    QElapsedTimer totalTimer;
    totalTimer.start();

    const QList<QWindow *> visibleWindows = visibleWorkspaceWindows(lifecycleContext.rootLoadResult);
    result.visibleWorkspace = !visibleWindows.isEmpty();
    result.visibleWindowCount = visibleWindows.size();

    if (m_started) {
        result.started = false;
        result.alreadyStarted = true;
        result.elapsedMs = totalTimer.elapsed();
        if (options.logDiagnostics) {
            QVariantMap payload;
            payload.insert(QStringLiteral("alreadyStarted"), true);
            payload.insert(QStringLiteral("visibleWorkspace"), result.visibleWorkspace);
            payload.insert(QStringLiteral("visibleWindowCount"), result.visibleWindowCount);
            logBootstrapEvent(QStringLiteral("foreground.skipped"), payload);
        }
        return result;
    }

    if (options.requireVisibleWorkspace && !result.visibleWorkspace) {
        result.ok = false;
        result.started = false;
        result.errors.append(QStringLiteral("Foreground services require a visible workspace window."));
        result.elapsedMs = totalTimer.elapsed();
        if (options.logDiagnostics) {
            QVariantMap payload;
            payload.insert(QStringLiteral("visibleWorkspace"), false);
            payload.insert(QStringLiteral("visibleWindowCount"), 0);
            payload.insert(QStringLiteral("error"), result.errorMessage());
            logBootstrapEvent(QStringLiteral("foreground.not-ready"), payload, QtWarningMsg);
        }
        return result;
    }

    m_started = true;
    m_startAttemptCount += 1;
    result.started = true;

    ForegroundServiceStartContext context;
    context.application = lifecycleContext.application;
    context.engine = lifecycleContext.engine;
    context.rootLoadResult = lifecycleContext.rootLoadResult;
    context.visibleWindows = visibleWindows;
    context.metadata = options.metadata;

    struct IndexedTask {
        int index = 0;
        ForegroundServiceTask task;
    };

    QList<IndexedTask> indexedTasks;
    indexedTasks.reserve(tasks.size());
    for (int index = 0; index < tasks.size(); ++index)
        indexedTasks.append({index, tasks.at(index)});

    std::stable_sort(indexedTasks.begin(), indexedTasks.end(), [](const IndexedTask &left, const IndexedTask &right) {
        return left.task.priority < right.task.priority;
    });

    if (options.logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("taskCount"), indexedTasks.size());
        payload.insert(QStringLiteral("visibleWorkspace"), result.visibleWorkspace);
        payload.insert(QStringLiteral("visibleWindowCount"), result.visibleWindowCount);
        payload.insert(QStringLiteral("startAttemptCount"), m_startAttemptCount);
        if (!options.metadata.isEmpty())
            payload.insert(QStringLiteral("metadata"), options.metadata);
        logBootstrapEvent(QStringLiteral("foreground.queue-start"), payload);
    }

    for (const IndexedTask &indexedTask : indexedTasks) {
        const ForegroundServiceTask &task = indexedTask.task;
        ForegroundServiceTaskResult taskResult;
        taskResult.name = normalizedTaskName(task, indexedTask.index);
        taskResult.index = indexedTask.index;
        taskResult.priority = task.priority;
        taskResult.fatal = task.fatal;
        taskResult.metadata = task.metadata;

        QElapsedTimer taskTimer;
        taskTimer.start();

        if (!task.start) {
            taskResult.ok = false;
            taskResult.errorMessage = QStringLiteral("Foreground service task has no start callback.");
        } else {
            QString errorMessage;
            try {
                taskResult.ok = task.start(context, &errorMessage);
            } catch (const std::exception &exception) {
                taskResult.ok = false;
                errorMessage = QString::fromUtf8(exception.what());
            } catch (...) {
                taskResult.ok = false;
                errorMessage = QStringLiteral("Foreground service task threw an unknown exception.");
            }

            if (!taskResult.ok) {
                taskResult.errorMessage = errorMessage.trimmed().isEmpty()
                    ? QStringLiteral("Foreground service task failed.")
                    : errorMessage.trimmed();
            }
        }

        taskResult.elapsedMs = taskTimer.elapsed();
        appendError(&result, taskResult);
        result.taskResults.append(taskResult);

        if (options.logDiagnostics) {
            logBootstrapEvent(taskResult.ok ? QStringLiteral("foreground.task-complete")
                                            : QStringLiteral("foreground.task-failed"),
                              serviceTaskSummary(taskResult),
                              taskResult.ok ? QtInfoMsg : QtWarningMsg);
        }
    }

    result.elapsedMs = totalTimer.elapsed();

    if (options.logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("ok"), result.ok);
        payload.insert(QStringLiteral("fatalFailure"), result.fatalFailure());
        payload.insert(QStringLiteral("taskCount"), result.taskResults.size());
        payload.insert(QStringLiteral("visibleWorkspace"), result.visibleWorkspace);
        payload.insert(QStringLiteral("visibleWindowCount"), result.visibleWindowCount);
        payload.insert(QStringLiteral("elapsedMs"), result.elapsedMs);
        if (!result.errors.isEmpty())
            payload.insert(QStringLiteral("errors"), result.errors);
        logBootstrapEvent(QStringLiteral("foreground.queue-complete"),
                          payload,
                          result.fatalFailure() ? QtCriticalMsg : QtInfoMsg);
    }

    return result;
}

} // namespace lvrs
