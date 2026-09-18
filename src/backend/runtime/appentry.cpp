#include "backend/runtime/appentry.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QVariantMap>
#include <QWindow>

#include <algorithm>
#include <utility>

namespace lvrs {

namespace {

void appendExistingPath(QStringList *paths, const QString &path)
{
    const QString cleanedPath = QDir::cleanPath(path.trimmed());
    if (cleanedPath.isEmpty())
        return;

    const QFileInfo pathInfo(cleanedPath);
    if (!pathInfo.isDir())
        return;
    if (paths->contains(cleanedPath))
        return;

    paths->append(cleanedPath);
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

QmlWindowActivationPolicy resolvedWindowActivationPolicy(QmlWindowActivationPolicy policy,
                                                         QmlWindowActivationPolicy fallback)
{
    if (policy == QmlWindowActivationPolicy::Inherit)
        return fallback == QmlWindowActivationPolicy::Inherit ? QmlWindowActivationPolicy::None : fallback;
    return policy;
}

QVariantMap rootSpecSummary(const QmlRootLoadSpec &root, QmlWindowActivationPolicy activationPolicy)
{
    QVariantMap payload;
    payload.insert(QStringLiteral("moduleUri"), root.moduleUri);
    payload.insert(QStringLiteral("rootObject"), root.rootObject);
    payload.insert(QStringLiteral("initialPropertyCount"), root.initialProperties.size());
    payload.insert(QStringLiteral("initialPropertyKeys"), root.initialProperties.keys());
    payload.insert(QStringLiteral("windowActivationPolicy"), qmlWindowActivationPolicyName(activationPolicy));
    return payload;
}

bool validateRootSpec(const QmlRootLoadSpec &root, QString *errorMessage)
{
    if (!root.moduleUri.trimmed().isEmpty() && !root.rootObject.trimmed().isEmpty())
        return true;

    if (errorMessage) {
        *errorMessage = QStringLiteral("QML root load requires non-empty module URI and root object.");
    }
    return false;
}

QList<QmlRootLoadSpec> resolvedAppRoots(const QmlAppLaunchSpec &spec, QString *errorMessage)
{
    QList<QmlRootLoadSpec> roots;

    if (spec.roots.isEmpty()) {
        QmlRootLoadSpec root;
        root.moduleUri = spec.moduleUri;
        root.rootObject = spec.rootObject;
        root.initialProperties = spec.initialProperties;
        root.windowActivationPolicy = spec.windowActivationPolicy;
        roots.append(root);
    } else {
        roots = spec.roots;
        for (QmlRootLoadSpec &root : roots) {
            if (root.moduleUri.trimmed().isEmpty())
                root.moduleUri = spec.moduleUri;
            root.windowActivationPolicy =
                resolvedWindowActivationPolicy(root.windowActivationPolicy, spec.windowActivationPolicy);
        }
    }

    for (const QmlRootLoadSpec &root : std::as_const(roots)) {
        if (!validateRootSpec(root, errorMessage))
            return {};
    }

    return roots;
}

bool hasLifecycleStageWork(const QmlAppLifecycleHooks &hooks, QmlAppLifecycleStage stage)
{
    switch (stage) {
    case QmlAppLifecycleStage::AfterRootLoaded:
        if (hooks.afterRootLoaded)
            return true;
        break;
    case QmlAppLifecycleStage::AfterWindowActivated:
        if (hooks.afterWindowActivated)
            return true;
        break;
    case QmlAppLifecycleStage::AfterFirstIdle:
        if (hooks.afterFirstIdle)
            return true;
        break;
    }

    for (const QmlBootstrapTask &task : hooks.tasks) {
        if (task.stage == stage)
            return true;
    }
    return false;
}

QmlAppLifecycleContext contextForStage(const QmlAppLifecycleContext &context, QmlAppLifecycleStage stage)
{
    QmlAppLifecycleContext stageContext = context;
    stageContext.stage = stage;
    return stageContext;
}

QmlAppLifecycleHook hookForStage(const QmlAppLifecycleHooks &hooks, QmlAppLifecycleStage stage)
{
    switch (stage) {
    case QmlAppLifecycleStage::AfterRootLoaded:
        return hooks.afterRootLoaded;
    case QmlAppLifecycleStage::AfterWindowActivated:
        return hooks.afterWindowActivated;
    case QmlAppLifecycleStage::AfterFirstIdle:
        return hooks.afterFirstIdle;
    }

    return {};
}

QVariantMap lifecycleStageSummary(const QmlAppLifecycleContext &context, QmlAppLifecycleStage stage)
{
    QVariantMap payload;
    payload.insert(QStringLiteral("stage"), qmlAppLifecycleStageName(stage));
    payload.insert(QStringLiteral("rootObjectCount"), context.rootLoadResult.rootObjects.size());
    payload.insert(QStringLiteral("windowCount"), context.rootLoadResult.windows.size());
    return payload;
}

} // namespace

QString QmlRootLoadResult::errorMessage() const
{
    return errors.join(QStringLiteral("; "));
}

bool QmlBootstrapQueueResult::fatalFailure() const
{
    for (const QmlBootstrapTaskResult &taskResult : taskResults) {
        if (!taskResult.ok && taskResult.fatal)
            return true;
    }
    return false;
}

QString QmlBootstrapQueueResult::errorMessage() const
{
    return errors.join(QStringLiteral("; "));
}

QString qmlAppLifecycleStageName(QmlAppLifecycleStage stage)
{
    switch (stage) {
    case QmlAppLifecycleStage::AfterRootLoaded:
        return QStringLiteral("after-root-loaded");
    case QmlAppLifecycleStage::AfterWindowActivated:
        return QStringLiteral("after-window-activated");
    case QmlAppLifecycleStage::AfterFirstIdle:
        return QStringLiteral("after-first-idle");
    }

    return QStringLiteral("unknown");
}

QString qmlWindowActivationPolicyName(QmlWindowActivationPolicy policy)
{
    switch (policy) {
    case QmlWindowActivationPolicy::Inherit:
        return QStringLiteral("inherit");
    case QmlWindowActivationPolicy::None:
        return QStringLiteral("none");
    case QmlWindowActivationPolicy::Show:
        return QStringLiteral("show");
    case QmlWindowActivationPolicy::ShowAndRaise:
        return QStringLiteral("show-and-raise");
    case QmlWindowActivationPolicy::ShowRaiseAndActivate:
        return QStringLiteral("show-raise-and-activate");
    }

    return QStringLiteral("unknown");
}

QWindow *qmlRootWindow(QObject *rootObject)
{
    return qobject_cast<QWindow *>(rootObject);
}

void applyQmlWindowActivationPolicy(QWindow *window, QmlWindowActivationPolicy policy)
{
    if (!window)
        return;

    switch (resolvedWindowActivationPolicy(policy, QmlWindowActivationPolicy::None)) {
    case QmlWindowActivationPolicy::Inherit:
    case QmlWindowActivationPolicy::None:
        return;
    case QmlWindowActivationPolicy::Show:
        window->show();
        return;
    case QmlWindowActivationPolicy::ShowAndRaise:
        window->show();
        window->raise();
        return;
    case QmlWindowActivationPolicy::ShowRaiseAndActivate:
        window->show();
        window->raise();
        window->requestActivate();
        return;
    }
}

QmlRootLoadResult loadQmlRootObjects(QQmlApplicationEngine &engine,
                                     const QList<QmlRootLoadSpec> &roots,
                                     const QmlRootLoadOptions &options)
{
    QmlRootLoadResult result;

    if (roots.isEmpty()) {
        result.errors.append(QStringLiteral("No QML root objects were requested."));
        return result;
    }

    for (int index = 0; index < roots.size(); ++index) {
        const QmlRootLoadSpec &root = roots.at(index);
        const QmlWindowActivationPolicy activationPolicy =
            resolvedWindowActivationPolicy(root.windowActivationPolicy, options.defaultWindowActivationPolicy);

        QString validationError;
        if (!validateRootSpec(root, &validationError)) {
            result.errors.append(validationError);
            continue;
        }

        if (options.logDiagnostics) {
            QVariantMap payload = rootSpecSummary(root, activationPolicy);
            payload.insert(QStringLiteral("index"), index);
            logBootstrapEvent(QStringLiteral("entry.root-load-request"), payload);
        }

        const int previousRootCount = engine.rootObjects().size();
        engine.setInitialProperties(root.initialProperties);
        engine.loadFromModule(root.moduleUri, root.rootObject);
        engine.setInitialProperties({});

        const QObjectList allRootObjects = engine.rootObjects();
        if (allRootObjects.size() <= previousRootCount) {
            const QString error =
                QStringLiteral("QML root '%1.%2' did not create a root object.")
                    .arg(root.moduleUri, root.rootObject);
            result.errors.append(error);
            if (options.logDiagnostics) {
                QVariantMap payload = rootSpecSummary(root, activationPolicy);
                payload.insert(QStringLiteral("index"), index);
                payload.insert(QStringLiteral("error"), error);
                logBootstrapEvent(QStringLiteral("entry.root-load-failed"), payload);
            }
            continue;
        }

        QObjectList createdRoots;
        for (int rootIndex = previousRootCount; rootIndex < allRootObjects.size(); ++rootIndex) {
            QObject *createdRoot = allRootObjects.at(rootIndex);
            if (!createdRoot)
                continue;

            createdRoots.append(createdRoot);
            result.rootObjects.append(createdRoot);

            QWindow *window = qmlRootWindow(createdRoot);
            if (!window)
                continue;

            result.windows.append(window);
            applyQmlWindowActivationPolicy(window, activationPolicy);
        }

        if (options.logDiagnostics) {
            QVariantMap payload = rootSpecSummary(root, activationPolicy);
            payload.insert(QStringLiteral("index"), index);
            payload.insert(QStringLiteral("createdRootCount"), createdRoots.size());
            payload.insert(QStringLiteral("windowCount"), result.windows.size());
            logBootstrapEvent(QStringLiteral("entry.root-loaded"), payload);
        }
    }

    result.ok = result.errors.isEmpty() && !result.rootObjects.isEmpty();
    return result;
}

QmlBootstrapQueueResult runQmlAppLifecycleStage(const QmlAppLifecycleContext &context,
                                                const QmlAppLifecycleHooks &hooks,
                                                QmlAppLifecycleStage stage,
                                                bool logDiagnostics)
{
    QmlBootstrapQueueResult result;
    if (!hasLifecycleStageWork(hooks, stage))
        return result;

    const QmlAppLifecycleContext stageContext = contextForStage(context, stage);
    if (logDiagnostics) {
        QVariantMap payload = lifecycleStageSummary(stageContext, stage);
        payload.insert(QStringLiteral("taskCount"), 0);
        logBootstrapEvent(QStringLiteral("lifecycle.stage-start"), payload);
    }

    if (const QmlAppLifecycleHook hook = hookForStage(hooks, stage))
        hook(stageContext);

    struct IndexedTask {
        int index = 0;
        QmlBootstrapTask task;
    };

    QList<IndexedTask> stageTasks;
    for (int index = 0; index < hooks.tasks.size(); ++index) {
        const QmlBootstrapTask &task = hooks.tasks.at(index);
        if (task.stage != stage)
            continue;
        stageTasks.append({index, task});
    }

    std::stable_sort(stageTasks.begin(), stageTasks.end(), [](const IndexedTask &left, const IndexedTask &right) {
        return left.task.priority < right.task.priority;
    });

    if (logDiagnostics) {
        QVariantMap payload = lifecycleStageSummary(stageContext, stage);
        payload.insert(QStringLiteral("taskCount"), stageTasks.size());
        logBootstrapEvent(QStringLiteral("lifecycle.queue-start"), payload);
    }

    for (const IndexedTask &indexedTask : stageTasks) {
        const QmlBootstrapTask &task = indexedTask.task;
        const QString taskName = task.name.trimmed().isEmpty()
            ? QStringLiteral("bootstrap-task-%1").arg(indexedTask.index)
            : task.name.trimmed();

        if (logDiagnostics) {
            QVariantMap payload;
            payload.insert(QStringLiteral("stage"), qmlAppLifecycleStageName(stage));
            payload.insert(QStringLiteral("name"), taskName);
            payload.insert(QStringLiteral("priority"), task.priority);
            payload.insert(QStringLiteral("fatal"), task.fatal);
            logBootstrapEvent(QStringLiteral("lifecycle.task-start"), payload);
        }

        QmlBootstrapTaskResult taskResult;
        taskResult.name = taskName;
        taskResult.stage = stage;
        taskResult.fatal = task.fatal;

        QString errorMessage;
        if (!task.run) {
            taskResult.ok = false;
            taskResult.errorMessage = QStringLiteral("Bootstrap task has no callback.");
        } else {
            taskResult.ok = task.run(stageContext, &errorMessage);
            if (!taskResult.ok) {
                taskResult.errorMessage = errorMessage.trimmed().isEmpty()
                    ? QStringLiteral("Bootstrap task failed.")
                    : errorMessage.trimmed();
            }
        }

        if (!taskResult.ok) {
            result.ok = false;
            result.errors.append(QStringLiteral("%1: %2").arg(taskName, taskResult.errorMessage));
        }
        result.taskResults.append(taskResult);

        if (logDiagnostics) {
            QVariantMap payload;
            payload.insert(QStringLiteral("stage"), qmlAppLifecycleStageName(stage));
            payload.insert(QStringLiteral("name"), taskName);
            payload.insert(QStringLiteral("priority"), task.priority);
            payload.insert(QStringLiteral("fatal"), task.fatal);
            payload.insert(QStringLiteral("ok"), taskResult.ok);
            if (!taskResult.errorMessage.isEmpty())
                payload.insert(QStringLiteral("error"), taskResult.errorMessage);
            logBootstrapEvent(taskResult.ok ? QStringLiteral("lifecycle.task-complete")
                                            : QStringLiteral("lifecycle.task-failed"),
                              payload,
                              taskResult.ok ? QtInfoMsg : QtWarningMsg);
        }
    }

    if (logDiagnostics) {
        QVariantMap payload = lifecycleStageSummary(stageContext, stage);
        payload.insert(QStringLiteral("ok"), result.ok);
        payload.insert(QStringLiteral("fatalFailure"), result.fatalFailure());
        payload.insert(QStringLiteral("taskCount"), stageTasks.size());
        if (!result.errors.isEmpty())
            payload.insert(QStringLiteral("errors"), result.errors);
        logBootstrapEvent(QStringLiteral("lifecycle.stage-complete"),
                          payload,
                          result.fatalFailure() ? QtCriticalMsg : QtInfoMsg);
    }

    return result;
}

bool scheduleQmlAppLifecycleStage(QObject *receiver,
                                  const QmlAppLifecycleContext &context,
                                  const QmlAppLifecycleHooks &hooks,
                                  QmlAppLifecycleStage stage,
                                  bool logDiagnostics)
{
    if (!hasLifecycleStageWork(hooks, stage))
        return true;

    if (!receiver)
        return false;

    QTimer::singleShot(0, receiver, [context, hooks, stage, logDiagnostics]() {
        const QmlBootstrapQueueResult result =
            runQmlAppLifecycleStage(context, hooks, stage, logDiagnostics);
        if (result.fatalFailure())
            QCoreApplication::exit(-1);
    });
    return true;
}

QStringList defaultRuntimeQmlImportPaths(const QString &applicationDirPath)
{
    QStringList importPaths;
    const QString appDir = QDir::cleanPath(applicationDirPath.trimmed());
    if (appDir.isEmpty())
        return importPaths;

    appendExistingPath(&importPaths, QDir(appDir).filePath(QStringLiteral("lvrs-runtime/qml")));
    appendExistingPath(&importPaths, QDir(appDir).filePath(QStringLiteral("../lib/qt6/qml")));
    appendExistingPath(&importPaths, QDir(appDir).filePath(QStringLiteral("../qml")));
    appendExistingPath(&importPaths,
                       QDir(appDir).filePath(QStringLiteral("../../../../../platforms/linux/lib/qt6/qml")));
    return importPaths;
}

int runBootstrappedQmlApp(int argc, char *argv[], const QmlAppLaunchSpec &spec)
{
    QString rootSpecError;
    const QList<QmlRootLoadSpec> roots = resolvedAppRoots(spec, &rootSpecError);
    if (roots.isEmpty()) {
        qCritical().noquote() << (rootSpecError.isEmpty()
                                      ? QStringLiteral("LVRS app entry requires at least one QML root object.")
                                      : rootSpecError);
        return -1;
    }

    const AppBootstrapState bootstrapState = preApplicationBootstrap(spec.bootstrap);
    if (!bootstrapState.ok) {
        qCritical().noquote() << bootstrapState.errorMessage;
        return -1;
    }

    QGuiApplication app(argc, argv);
    postApplicationBootstrap(app, spec.bootstrap);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    QStringList runtimeImportPaths;
    if (spec.includeDefaultRuntimeQmlImportPaths)
        runtimeImportPaths = defaultRuntimeQmlImportPaths(QCoreApplication::applicationDirPath());
    for (const QString &path : spec.qmlImportPaths)
        appendExistingPath(&runtimeImportPaths, path);
    for (const QString &path : runtimeImportPaths)
        engine.addImportPath(path);

    if (spec.bootstrap.logBootstrapDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("applicationDirPath"), QCoreApplication::applicationDirPath());
        payload.insert(QStringLiteral("includeDefaultRuntimeQmlImportPaths"),
                       spec.includeDefaultRuntimeQmlImportPaths);
        payload.insert(QStringLiteral("importPathCount"), runtimeImportPaths.size());
        payload.insert(QStringLiteral("importPaths"), runtimeImportPaths);
        logBootstrapEvent(QStringLiteral("entry.import-paths"), payload);
    }

    if (spec.configureEngine)
        spec.configureEngine(engine);

    if (spec.bootstrap.logBootstrapDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("moduleUri"), spec.moduleUri);
        payload.insert(QStringLiteral("rootObject"), spec.rootObject);
        payload.insert(QStringLiteral("rootCount"), roots.size());
        payload.insert(QStringLiteral("windowActivationPolicy"),
                       qmlWindowActivationPolicyName(spec.windowActivationPolicy));
        payload.insert(QStringLiteral("configureEngine"), spec.configureEngine != nullptr);
        payload.insert(QStringLiteral("initialPropertyCount"), spec.initialProperties.size());
        payload.insert(QStringLiteral("initialPropertyKeys"), spec.initialProperties.keys());
        logBootstrapEvent(QStringLiteral("entry.load-request"), payload);
    }

    QmlRootLoadOptions loadOptions;
    loadOptions.logDiagnostics = spec.bootstrap.logBootstrapDiagnostics;
    loadOptions.defaultWindowActivationPolicy = spec.windowActivationPolicy;
    const QmlRootLoadResult loadResult = loadQmlRootObjects(engine, roots, loadOptions);
    if (!loadResult.ok) {
        qCritical().noquote() << "LVRS app entry failed to load QML roots:" << loadResult.errorMessage();
        return -1;
    }

    QmlAppLifecycleContext lifecycleContext;
    lifecycleContext.application = &app;
    lifecycleContext.engine = &engine;
    lifecycleContext.rootLoadResult = loadResult;

    const QmlBootstrapQueueResult rootStageResult =
        runQmlAppLifecycleStage(lifecycleContext,
                                spec.lifecycle,
                                QmlAppLifecycleStage::AfterRootLoaded,
                                spec.bootstrap.logBootstrapDiagnostics);
    if (rootStageResult.fatalFailure())
        return -1;

    const QmlBootstrapQueueResult windowStageResult =
        runQmlAppLifecycleStage(lifecycleContext,
                                spec.lifecycle,
                                QmlAppLifecycleStage::AfterWindowActivated,
                                spec.bootstrap.logBootstrapDiagnostics);
    if (windowStageResult.fatalFailure())
        return -1;

    if (!scheduleQmlAppLifecycleStage(&app,
                                      lifecycleContext,
                                      spec.lifecycle,
                                      QmlAppLifecycleStage::AfterFirstIdle,
                                      spec.bootstrap.logBootstrapDiagnostics)) {
        qCritical().noquote() << "LVRS app entry failed to schedule first-idle lifecycle stage.";
        return -1;
    }

    return app.exec();
}

} // namespace lvrs
