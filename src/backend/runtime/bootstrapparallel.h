#pragma once

#include <functional>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

namespace lvrs {

struct BootstrapParallelTaskContext {
    QString name;
    int index = 0;
    int priority = 0;
    QVariantMap metadata;
};

struct BootstrapParallelTaskResult {
    QString name;
    int index = 0;
    int priority = 0;
    bool fatal = false;
    bool ok = false;
    bool loadOk = false;
    bool applied = false;
    bool applyOk = true;
    QString errorMessage;
    QString applyErrorMessage;
    QVariant value;
    QVariantMap metadata;
    qint64 loadElapsedMs = 0;
    qint64 applyElapsedMs = 0;

    QVariantMap toVariantMap() const;
};

using BootstrapParallelLoadCallback = std::function<bool(const BootstrapParallelTaskContext &context,
                                                         QVariant *value,
                                                         QString *errorMessage)>;
using BootstrapParallelApplyCallback = std::function<bool(const BootstrapParallelTaskResult &result,
                                                          QString *errorMessage)>;

struct BootstrapParallelTask {
    QString name;
    int priority = 0;
    bool fatal = false;
    QVariantMap metadata;
    BootstrapParallelLoadCallback load;
    BootstrapParallelApplyCallback apply;
};

struct BootstrapParallelRunOptions {
    QObject *applyReceiver = nullptr;
    int maxThreadCount = 0;
    bool skipApplyOnLoadFailure = true;
    bool logDiagnostics = true;
};

struct BootstrapParallelRunResult {
    bool ok = true;
    QList<BootstrapParallelTaskResult> taskResults;
    QStringList errors;
    qint64 elapsedMs = 0;

    bool fatalFailure() const;
    QString errorMessage() const;
    QVariantList diagnostics() const;
};

BootstrapParallelRunResult runBootstrapParallelTasks(const QList<BootstrapParallelTask> &tasks,
                                                     const BootstrapParallelRunOptions &options = {});

} // namespace lvrs
