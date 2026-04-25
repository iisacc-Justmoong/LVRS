#include "backend/runtime/permissionsequencer.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QJsonDocument>

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

QString normalizedStepName(const lvrs::PermissionRequestStep &step, int index)
{
    const QString name = step.name.trimmed();
    if (!name.isEmpty())
        return name;
    return QStringLiteral("permission-step-%1").arg(index);
}

bool statusAllowedByPolicy(lvrs::PermissionRequestStatus status, bool required)
{
    switch (status) {
    case lvrs::PermissionRequestStatus::Granted:
    case lvrs::PermissionRequestStatus::Skipped:
        return true;
    case lvrs::PermissionRequestStatus::Denied:
    case lvrs::PermissionRequestStatus::Unavailable:
    case lvrs::PermissionRequestStatus::Failed:
        return !required;
    }
    return !required;
}

QString defaultErrorForStatus(lvrs::PermissionRequestStatus status)
{
    switch (status) {
    case lvrs::PermissionRequestStatus::Denied:
        return QStringLiteral("Permission was denied.");
    case lvrs::PermissionRequestStatus::Unavailable:
        return QStringLiteral("Permission is unavailable.");
    case lvrs::PermissionRequestStatus::Failed:
        return QStringLiteral("Permission request failed.");
    case lvrs::PermissionRequestStatus::Granted:
    case lvrs::PermissionRequestStatus::Skipped:
        return QString();
    }
    return QStringLiteral("Permission request failed.");
}

void countResult(lvrs::PermissionRequestRunResult *runResult,
                 const lvrs::PermissionRequestStepResult &stepResult)
{
    if (!runResult)
        return;

    runResult->completedCount += 1;
    switch (stepResult.status) {
    case lvrs::PermissionRequestStatus::Granted:
        runResult->grantedCount += 1;
        break;
    case lvrs::PermissionRequestStatus::Denied:
        runResult->deniedCount += 1;
        break;
    case lvrs::PermissionRequestStatus::Skipped:
        runResult->skippedCount += 1;
        break;
    case lvrs::PermissionRequestStatus::Unavailable:
        runResult->unavailableCount += 1;
        break;
    case lvrs::PermissionRequestStatus::Failed:
        runResult->failedCount += 1;
        break;
    }
}

void appendRunError(lvrs::PermissionRequestRunResult *runResult,
                    const lvrs::PermissionRequestStepResult &stepResult)
{
    if (!runResult || stepResult.ok)
        return;

    runResult->ok = false;
    const QString error = stepResult.errorMessage.trimmed().isEmpty()
        ? QStringLiteral("Permission request step failed.")
        : stepResult.errorMessage.trimmed();
    runResult->errors.append(QStringLiteral("%1: %2").arg(stepResult.name, error));
}

QVariantMap stepSummary(const lvrs::PermissionRequestStepResult &stepResult)
{
    QVariantMap payload;
    payload.insert(QStringLiteral("name"), stepResult.name);
    payload.insert(QStringLiteral("index"), stepResult.index);
    payload.insert(QStringLiteral("priority"), stepResult.priority);
    payload.insert(QStringLiteral("runId"), stepResult.runId);
    payload.insert(QStringLiteral("required"), stepResult.required);
    payload.insert(QStringLiteral("ok"), stepResult.ok);
    payload.insert(QStringLiteral("status"), lvrs::permissionRequestStatusName(stepResult.status));
    payload.insert(QStringLiteral("elapsedMs"), stepResult.elapsedMs);
    if (!stepResult.errorMessage.isEmpty())
        payload.insert(QStringLiteral("error"), stepResult.errorMessage);
    if (!stepResult.metadata.isEmpty())
        payload.insert(QStringLiteral("metadata"), stepResult.metadata);
    if (!stepResult.details.isEmpty())
        payload.insert(QStringLiteral("details"), stepResult.details);
    return payload;
}

} // namespace

namespace lvrs {

QString permissionRequestStatusName(PermissionRequestStatus status)
{
    switch (status) {
    case PermissionRequestStatus::Granted:
        return QStringLiteral("granted");
    case PermissionRequestStatus::Denied:
        return QStringLiteral("denied");
    case PermissionRequestStatus::Skipped:
        return QStringLiteral("skipped");
    case PermissionRequestStatus::Unavailable:
        return QStringLiteral("unavailable");
    case PermissionRequestStatus::Failed:
        return QStringLiteral("failed");
    }
    return QStringLiteral("failed");
}

bool PermissionRequestStepResult::granted() const
{
    return status == PermissionRequestStatus::Granted;
}

bool PermissionRequestStepResult::terminalFailure() const
{
    return required && !ok;
}

QVariantMap PermissionRequestStepResult::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("name"), name);
    map.insert(QStringLiteral("index"), index);
    map.insert(QStringLiteral("priority"), priority);
    map.insert(QStringLiteral("runId"), runId);
    map.insert(QStringLiteral("required"), required);
    map.insert(QStringLiteral("ok"), ok);
    map.insert(QStringLiteral("status"), permissionRequestStatusName(status));
    map.insert(QStringLiteral("granted"), granted());
    map.insert(QStringLiteral("terminalFailure"), terminalFailure());
    map.insert(QStringLiteral("errorMessage"), errorMessage);
    map.insert(QStringLiteral("metadata"), metadata);
    map.insert(QStringLiteral("details"), details);
    map.insert(QStringLiteral("elapsedMs"), elapsedMs);
    return map;
}

bool PermissionRequestRunResult::requiredFailure() const
{
    for (const PermissionRequestStepResult &stepResult : stepResults) {
        if (stepResult.terminalFailure())
            return true;
    }
    return false;
}

QString PermissionRequestRunResult::errorMessage() const
{
    return errors.join(QStringLiteral("; "));
}

QVariantList PermissionRequestRunResult::diagnostics() const
{
    QVariantList list;
    list.reserve(stepResults.size());
    for (const PermissionRequestStepResult &stepResult : stepResults)
        list.append(stepResult.toVariantMap());
    return list;
}

PermissionRequestSequencer::PermissionRequestSequencer(QObject *parent)
    : QObject(parent)
{
}

QVariantList PermissionRequestSequencer::history() const
{
    return m_history;
}

int PermissionRequestSequencer::runCount() const
{
    return m_runCount;
}

void PermissionRequestSequencer::clearHistory()
{
    if (m_history.isEmpty() && m_runCount == 0 && m_nextRunId == 1)
        return;

    m_history.clear();
    m_nextRunId = 1;
    m_runCount = 0;
    emit historyChanged();
}

PermissionRequestRunResult PermissionRequestSequencer::run(const QList<PermissionRequestStep> &steps,
                                                           const PermissionRequestRunOptions &options)
{
    PermissionRequestRunResult runResult;
    runResult.runId = m_nextRunId++;
    m_runCount += 1;

    QElapsedTimer totalTimer;
    totalTimer.start();

    struct IndexedStep {
        int index = 0;
        PermissionRequestStep step;
    };

    QList<IndexedStep> indexedSteps;
    indexedSteps.reserve(steps.size());
    for (int index = 0; index < steps.size(); ++index)
        indexedSteps.append({index, steps.at(index)});

    std::stable_sort(indexedSteps.begin(), indexedSteps.end(), [](const IndexedStep &left, const IndexedStep &right) {
        return left.step.priority < right.step.priority;
    });

    if (options.logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("runId"), runResult.runId);
        payload.insert(QStringLiteral("stepCount"), indexedSteps.size());
        payload.insert(QStringLiteral("stopOnRequiredFailure"), options.stopOnRequiredFailure);
        if (!options.metadata.isEmpty())
            payload.insert(QStringLiteral("metadata"), options.metadata);
        logBootstrapEvent(QStringLiteral("permission.queue-start"), payload);
    }

    QVariantList runHistory;

    for (const IndexedStep &indexedStep : indexedSteps) {
        const PermissionRequestStep &step = indexedStep.step;

        PermissionRequestStepResult stepResult;
        stepResult.name = normalizedStepName(step, indexedStep.index);
        stepResult.index = indexedStep.index;
        stepResult.priority = step.priority;
        stepResult.runId = runResult.runId;
        stepResult.required = step.required;
        stepResult.metadata = step.metadata;

        PermissionRequestStepContext context;
        context.name = stepResult.name;
        context.index = stepResult.index;
        context.priority = stepResult.priority;
        context.runId = stepResult.runId;
        context.required = stepResult.required;
        context.metadata = step.metadata;
        context.runMetadata = options.metadata;
        context.previousResults = runHistory;

        QElapsedTimer stepTimer;
        stepTimer.start();

        QString errorMessage;
        QVariantMap details;
        if (!step.request) {
            stepResult.status = PermissionRequestStatus::Failed;
            errorMessage = QStringLiteral("Permission request step has no callback.");
        } else {
            try {
                stepResult.status = step.request(context, &details, &errorMessage);
            } catch (const std::exception &exception) {
                stepResult.status = PermissionRequestStatus::Failed;
                errorMessage = QString::fromUtf8(exception.what());
            } catch (...) {
                stepResult.status = PermissionRequestStatus::Failed;
                errorMessage = QStringLiteral("Permission request step threw an unknown exception.");
            }
        }

        stepResult.details = details;
        stepResult.ok = statusAllowedByPolicy(stepResult.status, stepResult.required);
        if (!stepResult.ok || stepResult.status == PermissionRequestStatus::Failed) {
            stepResult.errorMessage = errorMessage.trimmed().isEmpty()
                ? defaultErrorForStatus(stepResult.status)
                : errorMessage.trimmed();
        }
        stepResult.elapsedMs = stepTimer.elapsed();

        countResult(&runResult, stepResult);
        appendRunError(&runResult, stepResult);
        runResult.stepResults.append(stepResult);

        const QVariantMap stepMap = stepResult.toVariantMap();
        runHistory.append(stepMap);
        if (options.appendHistory)
            m_history.append(stepMap);

        if (options.logDiagnostics) {
            logBootstrapEvent(stepResult.ok ? QStringLiteral("permission.step-complete")
                                            : QStringLiteral("permission.step-failed"),
                              stepSummary(stepResult),
                              stepResult.ok ? QtInfoMsg : QtWarningMsg);
        }

        if (options.stopOnRequiredFailure && stepResult.terminalFailure()) {
            runResult.stoppedEarly = true;
            break;
        }
    }

    runResult.elapsedMs = totalTimer.elapsed();

    if (options.logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("runId"), runResult.runId);
        payload.insert(QStringLiteral("ok"), runResult.ok);
        payload.insert(QStringLiteral("requiredFailure"), runResult.requiredFailure());
        payload.insert(QStringLiteral("stoppedEarly"), runResult.stoppedEarly);
        payload.insert(QStringLiteral("completedCount"), runResult.completedCount);
        payload.insert(QStringLiteral("grantedCount"), runResult.grantedCount);
        payload.insert(QStringLiteral("deniedCount"), runResult.deniedCount);
        payload.insert(QStringLiteral("skippedCount"), runResult.skippedCount);
        payload.insert(QStringLiteral("unavailableCount"), runResult.unavailableCount);
        payload.insert(QStringLiteral("failedCount"), runResult.failedCount);
        payload.insert(QStringLiteral("elapsedMs"), runResult.elapsedMs);
        if (!runResult.errors.isEmpty())
            payload.insert(QStringLiteral("errors"), runResult.errors);
        logBootstrapEvent(QStringLiteral("permission.queue-complete"),
                          payload,
                          runResult.requiredFailure() ? QtWarningMsg : QtInfoMsg);
    }

    emit historyChanged();

    return runResult;
}

} // namespace lvrs
