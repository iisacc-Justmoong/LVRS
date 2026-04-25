#pragma once

#include <functional>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace lvrs {

enum class PermissionRequestStatus {
    Granted,
    Denied,
    Skipped,
    Unavailable,
    Failed
};

QString permissionRequestStatusName(PermissionRequestStatus status);

struct PermissionRequestStepContext {
    QString name;
    int index = 0;
    int priority = 0;
    int runId = 0;
    bool required = true;
    QVariantMap metadata;
    QVariantMap runMetadata;
    QVariantList previousResults;
};

struct PermissionRequestStepResult {
    QString name;
    int index = 0;
    int priority = 0;
    int runId = 0;
    bool required = true;
    bool ok = false;
    PermissionRequestStatus status = PermissionRequestStatus::Failed;
    QString errorMessage;
    QVariantMap metadata;
    QVariantMap details;
    qint64 elapsedMs = 0;

    bool granted() const;
    bool terminalFailure() const;
    QVariantMap toVariantMap() const;
};

using PermissionRequestCallback = std::function<PermissionRequestStatus(const PermissionRequestStepContext &context,
                                                                        QVariantMap *details,
                                                                        QString *errorMessage)>;

struct PermissionRequestStep {
    QString name;
    int priority = 0;
    bool required = true;
    QVariantMap metadata;
    PermissionRequestCallback request;
};

struct PermissionRequestRunOptions {
    bool stopOnRequiredFailure = true;
    bool appendHistory = true;
    bool logDiagnostics = true;
    QVariantMap metadata;
};

struct PermissionRequestRunResult {
    bool ok = true;
    int runId = 0;
    bool stoppedEarly = false;
    QList<PermissionRequestStepResult> stepResults;
    QStringList errors;
    qint64 elapsedMs = 0;
    int completedCount = 0;
    int grantedCount = 0;
    int deniedCount = 0;
    int skippedCount = 0;
    int unavailableCount = 0;
    int failedCount = 0;

    bool requiredFailure() const;
    QString errorMessage() const;
    QVariantList diagnostics() const;
};

class PermissionRequestSequencer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)
    Q_PROPERTY(int runCount READ runCount NOTIFY historyChanged)

public:
    explicit PermissionRequestSequencer(QObject *parent = nullptr);

    QVariantList history() const;
    int runCount() const;

    void clearHistory();

    PermissionRequestRunResult run(const QList<PermissionRequestStep> &steps,
                                   const PermissionRequestRunOptions &options = {});

signals:
    void historyChanged();

private:
    QVariantList m_history;
    int m_nextRunId = 1;
    int m_runCount = 0;
};

} // namespace lvrs
