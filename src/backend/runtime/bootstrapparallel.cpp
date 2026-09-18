#include "backend/runtime/bootstrapparallel.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>
#include <QVariantMap>
#include <QWaitCondition>

#include <algorithm>
#include <exception>
#include <utility>

namespace {

class LambdaRunnable final : public QRunnable
{
public:
    explicit LambdaRunnable(std::function<void()> fn)
        : m_fn(std::move(fn))
    {
        setAutoDelete(true);
    }

    void run() override
    {
        if (m_fn)
            m_fn();
    }

private:
    std::function<void()> m_fn;
};

struct IndexedTask {
    int index = 0;
    lvrs::BootstrapParallelTask task;
};

QString normalizedTaskName(const lvrs::BootstrapParallelTask &task, int index)
{
    const QString name = task.name.trimmed();
    if (!name.isEmpty())
        return name;
    return QStringLiteral("bootstrap-parallel-task-%1").arg(index);
}

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

QVariantMap taskSummary(const lvrs::BootstrapParallelTaskResult &result)
{
    QVariantMap payload;
    payload.insert(QStringLiteral("name"), result.name);
    payload.insert(QStringLiteral("index"), result.index);
    payload.insert(QStringLiteral("priority"), result.priority);
    payload.insert(QStringLiteral("fatal"), result.fatal);
    payload.insert(QStringLiteral("ok"), result.ok);
    payload.insert(QStringLiteral("loadOk"), result.loadOk);
    payload.insert(QStringLiteral("applied"), result.applied);
    payload.insert(QStringLiteral("applyOk"), result.applyOk);
    payload.insert(QStringLiteral("loadElapsedMs"), result.loadElapsedMs);
    payload.insert(QStringLiteral("applyElapsedMs"), result.applyElapsedMs);
    if (!result.errorMessage.isEmpty())
        payload.insert(QStringLiteral("error"), result.errorMessage);
    if (!result.applyErrorMessage.isEmpty())
        payload.insert(QStringLiteral("applyError"), result.applyErrorMessage);
    if (!result.metadata.isEmpty())
        payload.insert(QStringLiteral("metadata"), result.metadata);
    return payload;
}

lvrs::BootstrapParallelTaskResult runLoadTask(const IndexedTask &indexedTask)
{
    const lvrs::BootstrapParallelTask &task = indexedTask.task;

    lvrs::BootstrapParallelTaskResult result;
    result.name = normalizedTaskName(task, indexedTask.index);
    result.index = indexedTask.index;
    result.priority = task.priority;
    result.fatal = task.fatal;
    result.metadata = task.metadata;

    lvrs::BootstrapParallelTaskContext context;
    context.name = result.name;
    context.index = result.index;
    context.priority = result.priority;
    context.metadata = task.metadata;

    QElapsedTimer timer;
    timer.start();

    if (!task.load) {
        result.loadOk = false;
        result.ok = false;
        result.errorMessage = QStringLiteral("Bootstrap parallel task has no load callback.");
        result.loadElapsedMs = timer.elapsed();
        return result;
    }

    QString errorMessage;
    QVariant value;
    try {
        result.loadOk = task.load(context, &value, &errorMessage);
    } catch (const std::exception &exception) {
        result.loadOk = false;
        errorMessage = QString::fromUtf8(exception.what());
    } catch (...) {
        result.loadOk = false;
        errorMessage = QStringLiteral("Bootstrap parallel task threw an unknown exception.");
    }

    result.value = value;
    result.ok = result.loadOk;
    if (!result.loadOk) {
        result.errorMessage = errorMessage.trimmed().isEmpty()
            ? QStringLiteral("Bootstrap parallel task failed.")
            : errorMessage.trimmed();
    }
    result.loadElapsedMs = timer.elapsed();
    return result;
}

bool invokeOnReceiverThread(QObject *receiver, const std::function<void()> &fn)
{
    if (!fn)
        return true;

    if (!receiver || receiver->thread() == QThread::currentThread()) {
        fn();
        return true;
    }

    bool ran = false;
    const bool invoked = QMetaObject::invokeMethod(receiver,
                                                  [&fn, &ran]() {
                                                      fn();
                                                      ran = true;
                                                  },
                                                  Qt::BlockingQueuedConnection);
    return invoked && ran;
}

void appendResultErrors(lvrs::BootstrapParallelRunResult *runResult,
                        const lvrs::BootstrapParallelTaskResult &taskResult)
{
    if (!runResult || taskResult.ok)
        return;

    runResult->ok = false;
    if (!taskResult.errorMessage.isEmpty()) {
        runResult->errors.append(QStringLiteral("%1: %2").arg(taskResult.name, taskResult.errorMessage));
    }
    if (!taskResult.applyErrorMessage.isEmpty()) {
        runResult->errors.append(QStringLiteral("%1: %2").arg(taskResult.name, taskResult.applyErrorMessage));
    }
}

} // namespace

namespace lvrs {

QVariantMap BootstrapParallelTaskResult::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("name"), name);
    map.insert(QStringLiteral("index"), index);
    map.insert(QStringLiteral("priority"), priority);
    map.insert(QStringLiteral("fatal"), fatal);
    map.insert(QStringLiteral("ok"), ok);
    map.insert(QStringLiteral("loadOk"), loadOk);
    map.insert(QStringLiteral("applied"), applied);
    map.insert(QStringLiteral("applyOk"), applyOk);
    map.insert(QStringLiteral("errorMessage"), errorMessage);
    map.insert(QStringLiteral("applyErrorMessage"), applyErrorMessage);
    map.insert(QStringLiteral("value"), value);
    map.insert(QStringLiteral("metadata"), metadata);
    map.insert(QStringLiteral("loadElapsedMs"), loadElapsedMs);
    map.insert(QStringLiteral("applyElapsedMs"), applyElapsedMs);
    return map;
}

bool BootstrapParallelRunResult::fatalFailure() const
{
    for (const BootstrapParallelTaskResult &result : taskResults) {
        if (result.fatal && !result.ok)
            return true;
    }
    return false;
}

QString BootstrapParallelRunResult::errorMessage() const
{
    return errors.join(QStringLiteral("; "));
}

QVariantList BootstrapParallelRunResult::diagnostics() const
{
    QVariantList list;
    list.reserve(taskResults.size());
    for (const BootstrapParallelTaskResult &result : taskResults)
        list.append(result.toVariantMap());
    return list;
}

BootstrapParallelRunResult runBootstrapParallelTasks(const QList<BootstrapParallelTask> &tasks,
                                                     const BootstrapParallelRunOptions &options)
{
    BootstrapParallelRunResult runResult;

    QElapsedTimer totalTimer;
    totalTimer.start();

    if (tasks.isEmpty()) {
        runResult.elapsedMs = 0;
        return runResult;
    }

    QList<IndexedTask> indexedTasks;
    indexedTasks.reserve(tasks.size());
    for (int index = 0; index < tasks.size(); ++index)
        indexedTasks.append({index, tasks.at(index)});

    std::stable_sort(indexedTasks.begin(), indexedTasks.end(), [](const IndexedTask &left, const IndexedTask &right) {
        return left.task.priority < right.task.priority;
    });

    const int maxThreadCount = options.maxThreadCount > 0
        ? options.maxThreadCount
        : qMax(1, QThread::idealThreadCount());

    if (options.logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("taskCount"), indexedTasks.size());
        payload.insert(QStringLiteral("maxThreadCount"), maxThreadCount);
        payload.insert(QStringLiteral("skipApplyOnLoadFailure"), options.skipApplyOnLoadFailure);
        payload.insert(QStringLiteral("hasApplyReceiver"), options.applyReceiver != nullptr);
        logBootstrapEvent(QStringLiteral("parallel.queue-start"), payload);
    }

    QList<BootstrapParallelTaskResult> loadResults;
    loadResults.resize(indexedTasks.size());

    QMutex mutex;
    QWaitCondition condition;
    int remaining = indexedTasks.size();

    QThreadPool pool;
    pool.setMaxThreadCount(maxThreadCount);

    for (int taskIndex = 0; taskIndex < indexedTasks.size(); ++taskIndex) {
        const IndexedTask indexedTask = indexedTasks.at(taskIndex);
        pool.start(new LambdaRunnable([taskIndex, indexedTask, &loadResults, &mutex, &condition, &remaining]() {
            BootstrapParallelTaskResult taskResult = runLoadTask(indexedTask);

            QMutexLocker locker(&mutex);
            loadResults[taskIndex] = taskResult;
            remaining -= 1;
            condition.wakeAll();
        }));
    }

    {
        QMutexLocker locker(&mutex);
        while (remaining > 0)
            condition.wait(&mutex);
    }
    pool.waitForDone();

    runResult.taskResults = loadResults;

    for (int index = 0; index < runResult.taskResults.size(); ++index) {
        BootstrapParallelTaskResult taskResult = runResult.taskResults.at(index);
        const BootstrapParallelTask &task = indexedTasks.at(index).task;

        if (options.logDiagnostics) {
            logBootstrapEvent(taskResult.loadOk ? QStringLiteral("parallel.load-complete")
                                                : QStringLiteral("parallel.load-failed"),
                              taskSummary(taskResult),
                              taskResult.loadOk ? QtInfoMsg : QtWarningMsg);
        }

        if (task.apply && (!options.skipApplyOnLoadFailure || taskResult.loadOk)) {
            QElapsedTimer applyTimer;
            applyTimer.start();

            QString applyErrorMessage;
            bool applyResult = false;
            bool invoked = false;
            try {
                invoked = invokeOnReceiverThread(options.applyReceiver, [&]() {
                    applyResult = task.apply(taskResult, &applyErrorMessage);
                });
            } catch (const std::exception &exception) {
                invoked = true;
                applyResult = false;
                applyErrorMessage = QString::fromUtf8(exception.what());
            } catch (...) {
                invoked = true;
                applyResult = false;
                applyErrorMessage = QStringLiteral("Bootstrap parallel apply callback threw an unknown exception.");
            }

            taskResult.applied = invoked;
            taskResult.applyOk = invoked && applyResult;
            taskResult.applyElapsedMs = applyTimer.elapsed();
            if (!invoked) {
                taskResult.applyErrorMessage =
                    QStringLiteral("Failed to invoke bootstrap parallel apply callback.");
            } else if (!applyResult) {
                taskResult.applyErrorMessage = applyErrorMessage.trimmed().isEmpty()
                    ? QStringLiteral("Bootstrap parallel apply callback failed.")
                    : applyErrorMessage.trimmed();
            }
            taskResult.ok = taskResult.loadOk && taskResult.applyOk;

            if (options.logDiagnostics) {
                logBootstrapEvent(taskResult.applyOk ? QStringLiteral("parallel.apply-complete")
                                                     : QStringLiteral("parallel.apply-failed"),
                                  taskSummary(taskResult),
                                  taskResult.applyOk ? QtInfoMsg : QtWarningMsg);
            }
        }

        runResult.taskResults[index] = taskResult;
        appendResultErrors(&runResult, taskResult);
    }

    runResult.elapsedMs = totalTimer.elapsed();

    if (options.logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("ok"), runResult.ok);
        payload.insert(QStringLiteral("fatalFailure"), runResult.fatalFailure());
        payload.insert(QStringLiteral("taskCount"), runResult.taskResults.size());
        payload.insert(QStringLiteral("elapsedMs"), runResult.elapsedMs);
        if (!runResult.errors.isEmpty())
            payload.insert(QStringLiteral("errors"), runResult.errors);
        logBootstrapEvent(QStringLiteral("parallel.queue-complete"),
                          payload,
                          runResult.fatalFailure() ? QtCriticalMsg : QtInfoMsg);
    }

    return runResult;
}

} // namespace lvrs
