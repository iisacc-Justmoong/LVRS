#pragma once

#include "backend/runtime/appbootstrap.h"

#include <functional>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QVariantMap>

class QQmlApplicationEngine;
class QGuiApplication;
class QWindow;

namespace lvrs {

enum class QmlWindowActivationPolicy {
    Inherit,
    None,
    Show,
    ShowAndRaise,
    ShowRaiseAndActivate
};

struct QmlRootLoadSpec {
    QString moduleUri;
    QString rootObject = QStringLiteral("Main");
    QVariantMap initialProperties;
    QmlWindowActivationPolicy windowActivationPolicy = QmlWindowActivationPolicy::Inherit;
};

struct QmlRootLoadOptions {
    bool logDiagnostics = true;
    QmlWindowActivationPolicy defaultWindowActivationPolicy = QmlWindowActivationPolicy::None;
};

struct QmlRootLoadResult {
    bool ok = false;
    QStringList errors;
    QObjectList rootObjects;
    QList<QWindow *> windows;

    QString errorMessage() const;
};

enum class QmlAppLifecycleStage {
    AfterRootLoaded,
    AfterWindowActivated,
    AfterFirstIdle
};

struct QmlAppLifecycleContext {
    QGuiApplication *application = nullptr;
    QQmlApplicationEngine *engine = nullptr;
    QmlRootLoadResult rootLoadResult;
    QmlAppLifecycleStage stage = QmlAppLifecycleStage::AfterRootLoaded;
};

using QmlAppLifecycleHook = std::function<void(const QmlAppLifecycleContext &context)>;
using QmlBootstrapTaskCallback = std::function<bool(const QmlAppLifecycleContext &context,
                                                    QString *errorMessage)>;

struct QmlBootstrapTask {
    QString name;
    QmlAppLifecycleStage stage = QmlAppLifecycleStage::AfterRootLoaded;
    int priority = 0;
    bool fatal = false;
    QmlBootstrapTaskCallback run;
};

struct QmlBootstrapTaskResult {
    QString name;
    QmlAppLifecycleStage stage = QmlAppLifecycleStage::AfterRootLoaded;
    bool ok = false;
    bool fatal = false;
    QString errorMessage;
};

struct QmlBootstrapQueueResult {
    bool ok = true;
    QList<QmlBootstrapTaskResult> taskResults;
    QStringList errors;

    bool fatalFailure() const;
    QString errorMessage() const;
};

struct QmlAppLifecycleHooks {
    QmlAppLifecycleHook afterRootLoaded;
    QmlAppLifecycleHook afterWindowActivated;
    QmlAppLifecycleHook afterFirstIdle;
    QList<QmlBootstrapTask> tasks;
};

struct QmlAppLaunchSpec {
    AppBootstrapOptions bootstrap;
    QString moduleUri;
    QString rootObject = QStringLiteral("Main");
    QVariantMap initialProperties;
    QList<QmlRootLoadSpec> roots;
    QmlWindowActivationPolicy windowActivationPolicy = QmlWindowActivationPolicy::None;
    QStringList qmlImportPaths;
    bool includeDefaultRuntimeQmlImportPaths = true;
    std::function<void(QQmlApplicationEngine &engine)> configureEngine;
    QmlAppLifecycleHooks lifecycle;
};

QString qmlAppLifecycleStageName(QmlAppLifecycleStage stage);
QString qmlWindowActivationPolicyName(QmlWindowActivationPolicy policy);
QWindow *qmlRootWindow(QObject *rootObject);
void applyQmlWindowActivationPolicy(QWindow *window, QmlWindowActivationPolicy policy);
QmlRootLoadResult loadQmlRootObjects(QQmlApplicationEngine &engine,
                                     const QList<QmlRootLoadSpec> &roots,
                                     const QmlRootLoadOptions &options = {});
QmlBootstrapQueueResult runQmlAppLifecycleStage(const QmlAppLifecycleContext &context,
                                                const QmlAppLifecycleHooks &hooks,
                                                QmlAppLifecycleStage stage,
                                                bool logDiagnostics = true);
bool scheduleQmlAppLifecycleStage(QObject *receiver,
                                  const QmlAppLifecycleContext &context,
                                  const QmlAppLifecycleHooks &hooks,
                                  QmlAppLifecycleStage stage,
                                  bool logDiagnostics = true);
QStringList defaultRuntimeQmlImportPaths(const QString &applicationDirPath);
int runBootstrappedQmlApp(int argc, char *argv[], const QmlAppLaunchSpec &spec);

} // namespace lvrs
