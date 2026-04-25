#pragma once

#include "backend/runtime/appentry.h"

#include <functional>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class QWindow;

namespace lvrs {

struct ForegroundServiceStartContext {
    QGuiApplication *application = nullptr;
    QQmlApplicationEngine *engine = nullptr;
    QmlRootLoadResult rootLoadResult;
    QList<QWindow *> visibleWindows;
    QVariantMap metadata;
};

struct ForegroundServiceTaskResult {
    QString name;
    int index = 0;
    int priority = 0;
    bool fatal = false;
    bool ok = false;
    QString errorMessage;
    QVariantMap metadata;
    qint64 elapsedMs = 0;

    QVariantMap toVariantMap() const;
};

using ForegroundServiceStartCallback = std::function<bool(const ForegroundServiceStartContext &context,
                                                          QString *errorMessage)>;

struct ForegroundServiceTask {
    QString name;
    int priority = 0;
    bool fatal = false;
    QVariantMap metadata;
    ForegroundServiceStartCallback start;
};

struct ForegroundServiceStartOptions {
    bool requireVisibleWorkspace = true;
    bool logDiagnostics = true;
    QVariantMap metadata;
};

struct ForegroundServiceStartResult {
    bool ok = true;
    bool started = false;
    bool alreadyStarted = false;
    bool visibleWorkspace = false;
    int visibleWindowCount = 0;
    QList<ForegroundServiceTaskResult> taskResults;
    QStringList errors;
    qint64 elapsedMs = 0;

    bool fatalFailure() const;
    QString errorMessage() const;
    QVariantList diagnostics() const;
};

QList<QWindow *> visibleWorkspaceWindows(const QmlRootLoadResult &rootLoadResult);
bool hasVisibleWorkspace(const QmlRootLoadResult &rootLoadResult);

class ForegroundServiceGate : public QObject
{
public:
    explicit ForegroundServiceGate(QObject *parent = nullptr);

    bool started() const;
    int startAttemptCount() const;
    void reset();

    ForegroundServiceStartResult startOnceWhenWorkspaceVisible(
        const QmlAppLifecycleContext &lifecycleContext,
        const QList<ForegroundServiceTask> &tasks,
        const ForegroundServiceStartOptions &options = {});

private:
    bool m_started = false;
    int m_startAttemptCount = 0;
};

} // namespace lvrs
