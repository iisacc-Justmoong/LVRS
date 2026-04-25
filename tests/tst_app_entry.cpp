#include <QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QMap>
#include <QQmlApplicationEngine>
#include <QTemporaryDir>
#include <QWindow>

#include <memory>

#include "backend/runtime/appbootstrap.h"
#include "backend/runtime/appentry.h"

namespace {

class MessageCapture
{
public:
    MessageCapture()
        : m_previousHandler(qInstallMessageHandler(MessageCapture::handler))
    {
        s_activeCapture = this;
    }

    ~MessageCapture()
    {
        s_activeCapture = nullptr;
        qInstallMessageHandler(m_previousHandler);
    }

    QString joinedMessages() const
    {
        return m_messages.join(QLatin1Char('\n'));
    }

private:
    static void handler(QtMsgType type, const QMessageLogContext &, const QString &message)
    {
        if (!s_activeCapture)
            return;
        if (type == QtInfoMsg || type == QtWarningMsg || type == QtCriticalMsg)
            s_activeCapture->m_messages.append(message);
    }

    static MessageCapture *s_activeCapture;

    QtMessageHandler m_previousHandler = nullptr;
    QStringList m_messages;
};

MessageCapture *MessageCapture::s_activeCapture = nullptr;

bool writeTextFile(const QString &path, const QString &content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    file.write(content.toUtf8());
    return file.error() == QFileDevice::NoError;
}

QString createTestQmlModule(const QString &rootPath,
                            const QString &moduleUri,
                            const QMap<QString, QString> &qmlFiles)
{
    const QString modulePath = QDir(rootPath).filePath(moduleUri);
    if (!QDir().mkpath(modulePath))
        return QString();

    QString qmldir = QStringLiteral("module %1\n").arg(moduleUri);
    for (auto it = qmlFiles.cbegin(); it != qmlFiles.cend(); ++it)
        qmldir += QStringLiteral("%1 1.0 %1.qml\n").arg(it.key());

    if (!writeTextFile(QDir(modulePath).filePath(QStringLiteral("qmldir")), qmldir))
        return QString();

    for (auto it = qmlFiles.cbegin(); it != qmlFiles.cend(); ++it) {
        if (!writeTextFile(QDir(modulePath).filePath(it.key() + QStringLiteral(".qml")), it.value()))
            return QString();
    }

    return modulePath;
}

} // namespace

class AppEntryTests : public QObject
{
    Q_OBJECT

private slots:
    void default_runtime_qml_import_paths_include_staged_bundle_dirs();
    void default_runtime_qml_import_paths_include_snapshot_platform_dirs();
    void bootstrap_diagnostics_are_emitted();
    void qml_root_loader_loads_multiple_roots_with_initial_properties();
    void qml_root_loader_reports_missing_root_object();
    void qml_root_loader_applies_window_activation_policy();
    void lifecycle_stage_runs_hook_and_priority_tasks();
    void lifecycle_queue_reports_task_failures_and_fatality();
    void lifecycle_stage_can_be_scheduled_for_first_idle();
};

void AppEntryTests::default_runtime_qml_import_paths_include_staged_bundle_dirs()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path();
    const QString appDir = QDir(rootDir).filePath(QStringLiteral("bin"));
    const QString bundledQmlDir = QDir(appDir).filePath(QStringLiteral("lvrs-runtime/qml/LVRS"));
    const QString installedQmlDir = QDir(appDir).filePath(QStringLiteral("../lib/qt6/qml/LVRS"));

    QVERIFY(QDir().mkpath(bundledQmlDir));
    QVERIFY(QDir().mkpath(installedQmlDir));

    const QStringList importPaths = lvrs::defaultRuntimeQmlImportPaths(appDir);
    QCOMPARE(importPaths,
             QStringList({
                 QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("lvrs-runtime/qml"))),
                 QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../lib/qt6/qml")))
             }));
}

void AppEntryTests::default_runtime_qml_import_paths_include_snapshot_platform_dirs()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString installRoot = QDir(tempDir.path()).filePath(QStringLiteral("prefix"));
    const QString appDir = QDir(installRoot).filePath(QStringLiteral("src/LVRS/example/VisualCatalog/bin"));
    const QString snapshotQmlDir =
        QDir(installRoot).filePath(QStringLiteral("platforms/linux/lib/qt6/qml/LVRS"));

    QVERIFY(QDir().mkpath(appDir));
    QVERIFY(QDir().mkpath(snapshotQmlDir));

    const QStringList importPaths = lvrs::defaultRuntimeQmlImportPaths(appDir);
    QCOMPARE(importPaths,
             QStringList({
                 QDir::cleanPath(
                     QDir(appDir).filePath(QStringLiteral("../../../../../platforms/linux/lib/qt6/qml")))
             }));
}

void AppEntryTests::bootstrap_diagnostics_are_emitted()
{
    const QString previousApplicationName = qGuiApp ? qGuiApp->applicationName() : QString();

    lvrs::AppBootstrapOptions options;
    options.applicationName = QStringLiteral("BootstrapDiagnosticsTest");
    options.bootstrapGraphicsBackend = false;
    options.installBundledFonts = false;
    options.installPretendardFallbacks = false;
    options.enforcePretendardFallback = false;

    MessageCapture capture;

    const lvrs::AppBootstrapState state = lvrs::preApplicationBootstrap(options);
    QVERIFY(state.ok);
    lvrs::postApplicationBootstrap(*qGuiApp, options);

    const QString logs = capture.joinedMessages();
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.pre.options")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.pre.render-quality")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.pre.graphics-backend")));
    QVERIFY(logs.contains(QStringLiteral("\"bootstrapGraphicsBackend\":false")));
    QVERIFY(logs.contains(QStringLiteral("\"QSG_RHI_PIPELINE_CACHE_LOAD\"")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.post.application")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.post.font-policy")));
    QVERIFY(logs.contains(QStringLiteral("\"pretendardFallbackEnforced\":true")));

    if (qGuiApp)
        qGuiApp->setApplicationName(previousApplicationName);
}

void AppEntryTests::qml_root_loader_loads_multiple_roots_with_initial_properties()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString moduleUri = QStringLiteral("LVRSAppEntryMultiRootTest");
    QVERIFY(!createTestQmlModule(tempDir.path(),
                                 moduleUri,
                                 {
                                     {QStringLiteral("FirstRoot"),
                                      QStringLiteral("import QtQml\nQtObject { property string marker: \"\" }")},
                                     {QStringLiteral("SecondRoot"),
                                      QStringLiteral("import QtQml\nQtObject { property string marker: \"\" }")}
                                 })
                 .isEmpty());

    QQmlApplicationEngine engine;
    engine.addImportPath(tempDir.path());

    lvrs::QmlRootLoadSpec firstRoot;
    firstRoot.moduleUri = moduleUri;
    firstRoot.rootObject = QStringLiteral("FirstRoot");
    firstRoot.initialProperties.insert(QStringLiteral("marker"), QStringLiteral("alpha"));

    lvrs::QmlRootLoadSpec secondRoot;
    secondRoot.moduleUri = moduleUri;
    secondRoot.rootObject = QStringLiteral("SecondRoot");
    secondRoot.initialProperties.insert(QStringLiteral("marker"), QStringLiteral("beta"));

    MessageCapture capture;
    const lvrs::QmlRootLoadResult result = lvrs::loadQmlRootObjects(engine, {firstRoot, secondRoot});

    QVERIFY2(result.ok, qPrintable(result.errorMessage()));
    QCOMPARE(result.rootObjects.size(), 2);
    QCOMPARE(result.windows.size(), 0);
    QCOMPARE(result.rootObjects.at(0)->property("marker").toString(), QStringLiteral("alpha"));
    QCOMPARE(result.rootObjects.at(1)->property("marker").toString(), QStringLiteral("beta"));

    const QString logs = capture.joinedMessages();
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.entry.root-load-request")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.entry.root-loaded")));
    QVERIFY(logs.contains(QStringLiteral("\"initialPropertyKeys\":[\"marker\"]")));
}

void AppEntryTests::qml_root_loader_reports_missing_root_object()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString moduleUri = QStringLiteral("LVRSAppEntryMissingRootTest");
    QVERIFY(!createTestQmlModule(tempDir.path(),
                                 moduleUri,
                                 {
                                     {QStringLiteral("ExistingRoot"),
                                      QStringLiteral("import QtQml\nQtObject { property string marker: \"ok\" }")}
                                 })
                 .isEmpty());

    QQmlApplicationEngine engine;
    engine.addImportPath(tempDir.path());

    lvrs::QmlRootLoadSpec missingRoot;
    missingRoot.moduleUri = moduleUri;
    missingRoot.rootObject = QStringLiteral("MissingRoot");

    MessageCapture capture;
    const lvrs::QmlRootLoadResult result = lvrs::loadQmlRootObjects(engine, {missingRoot});

    QVERIFY(!result.ok);
    QVERIFY(result.rootObjects.isEmpty());
    QVERIFY(result.errorMessage().contains(QStringLiteral("did not create a root object")));

    const QString logs = capture.joinedMessages();
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.entry.root-load-failed")));
}

void AppEntryTests::qml_root_loader_applies_window_activation_policy()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString moduleUri = QStringLiteral("LVRSAppEntryWindowRootTest");
    QVERIFY(!createTestQmlModule(tempDir.path(),
                                 moduleUri,
                                 {
                                     {QStringLiteral("WindowRoot"),
                                      QStringLiteral("import QtQuick\nWindow { width: 160; height: 120; visible: false }")}
                                 })
                 .isEmpty());

    QQmlApplicationEngine engine;
    engine.addImportPath(tempDir.path());

    lvrs::QmlRootLoadSpec windowRoot;
    windowRoot.moduleUri = moduleUri;
    windowRoot.rootObject = QStringLiteral("WindowRoot");
    windowRoot.windowActivationPolicy = lvrs::QmlWindowActivationPolicy::ShowRaiseAndActivate;

    const lvrs::QmlRootLoadResult result = lvrs::loadQmlRootObjects(engine, {windowRoot});

    QVERIFY2(result.ok, qPrintable(result.errorMessage()));
    QCOMPARE(result.rootObjects.size(), 1);
    QCOMPARE(result.windows.size(), 1);

    QWindow *window = result.windows.first();
    QVERIFY(window);
    QCoreApplication::processEvents();
    QVERIFY(window->isVisible());
    window->close();
}

void AppEntryTests::lifecycle_stage_runs_hook_and_priority_tasks()
{
    QQmlApplicationEngine engine;
    lvrs::QmlAppLifecycleContext context;
    context.application = qGuiApp;
    context.engine = &engine;

    auto calls = std::make_shared<QStringList>();

    lvrs::QmlAppLifecycleHooks hooks;
    hooks.afterRootLoaded = [calls](const lvrs::QmlAppLifecycleContext &stageContext) {
        calls->append(QStringLiteral("hook:%1").arg(lvrs::qmlAppLifecycleStageName(stageContext.stage)));
    };

    lvrs::QmlBootstrapTask lateTask;
    lateTask.name = QStringLiteral("late-root-task");
    lateTask.stage = lvrs::QmlAppLifecycleStage::AfterRootLoaded;
    lateTask.priority = 20;
    lateTask.run = [calls](const lvrs::QmlAppLifecycleContext &, QString *) {
        calls->append(QStringLiteral("task:late"));
        return true;
    };

    lvrs::QmlBootstrapTask earlyTask;
    earlyTask.name = QStringLiteral("early-root-task");
    earlyTask.stage = lvrs::QmlAppLifecycleStage::AfterRootLoaded;
    earlyTask.priority = 10;
    earlyTask.run = [calls](const lvrs::QmlAppLifecycleContext &, QString *) {
        calls->append(QStringLiteral("task:early"));
        return true;
    };

    hooks.tasks = {lateTask, earlyTask};

    MessageCapture capture;
    const lvrs::QmlBootstrapQueueResult result =
        lvrs::runQmlAppLifecycleStage(context, hooks, lvrs::QmlAppLifecycleStage::AfterRootLoaded);

    QVERIFY(result.ok);
    QVERIFY(!result.fatalFailure());
    QCOMPARE(result.taskResults.size(), 2);
    QCOMPARE(*calls,
             QStringList({
                 QStringLiteral("hook:after-root-loaded"),
                 QStringLiteral("task:early"),
                 QStringLiteral("task:late")
             }));

    const QString logs = capture.joinedMessages();
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.lifecycle.stage-start")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.lifecycle.queue-start")));
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.lifecycle.task-complete")));
    QVERIFY(logs.contains(QStringLiteral("\"stage\":\"after-root-loaded\"")));
}

void AppEntryTests::lifecycle_queue_reports_task_failures_and_fatality()
{
    QQmlApplicationEngine engine;
    lvrs::QmlAppLifecycleContext context;
    context.application = qGuiApp;
    context.engine = &engine;

    lvrs::QmlBootstrapTask nonFatalTask;
    nonFatalTask.name = QStringLiteral("recoverable");
    nonFatalTask.stage = lvrs::QmlAppLifecycleStage::AfterWindowActivated;
    nonFatalTask.priority = 1;
    nonFatalTask.run = [](const lvrs::QmlAppLifecycleContext &, QString *errorMessage) {
        if (errorMessage)
            *errorMessage = QStringLiteral("recoverable failure");
        return false;
    };

    lvrs::QmlBootstrapTask fatalTask;
    fatalTask.name = QStringLiteral("fatal");
    fatalTask.stage = lvrs::QmlAppLifecycleStage::AfterWindowActivated;
    fatalTask.priority = 2;
    fatalTask.fatal = true;
    fatalTask.run = [](const lvrs::QmlAppLifecycleContext &, QString *errorMessage) {
        if (errorMessage)
            *errorMessage = QStringLiteral("fatal failure");
        return false;
    };

    lvrs::QmlAppLifecycleHooks hooks;
    hooks.tasks = {nonFatalTask, fatalTask};

    MessageCapture capture;
    const lvrs::QmlBootstrapQueueResult result =
        lvrs::runQmlAppLifecycleStage(context, hooks, lvrs::QmlAppLifecycleStage::AfterWindowActivated);

    QVERIFY(!result.ok);
    QVERIFY(result.fatalFailure());
    QCOMPARE(result.taskResults.size(), 2);
    QCOMPARE(result.taskResults.at(0).name, QStringLiteral("recoverable"));
    QVERIFY(!result.taskResults.at(0).fatal);
    QCOMPARE(result.taskResults.at(0).errorMessage, QStringLiteral("recoverable failure"));
    QCOMPARE(result.taskResults.at(1).name, QStringLiteral("fatal"));
    QVERIFY(result.taskResults.at(1).fatal);
    QCOMPARE(result.taskResults.at(1).errorMessage, QStringLiteral("fatal failure"));
    QVERIFY(result.errorMessage().contains(QStringLiteral("recoverable failure")));
    QVERIFY(result.errorMessage().contains(QStringLiteral("fatal failure")));

    const QString logs = capture.joinedMessages();
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.lifecycle.task-failed")));
    QVERIFY(logs.contains(QStringLiteral("\"fatalFailure\":true")));
}

void AppEntryTests::lifecycle_stage_can_be_scheduled_for_first_idle()
{
    QQmlApplicationEngine engine;
    lvrs::QmlAppLifecycleContext context;
    context.application = qGuiApp;
    context.engine = &engine;

    auto calls = std::make_shared<QStringList>();

    lvrs::QmlAppLifecycleHooks hooks;
    hooks.afterFirstIdle = [calls](const lvrs::QmlAppLifecycleContext &stageContext) {
        calls->append(QStringLiteral("hook:%1").arg(lvrs::qmlAppLifecycleStageName(stageContext.stage)));
    };

    lvrs::QmlBootstrapTask task;
    task.name = QStringLiteral("idle-task");
    task.stage = lvrs::QmlAppLifecycleStage::AfterFirstIdle;
    task.run = [calls](const lvrs::QmlAppLifecycleContext &, QString *) {
        calls->append(QStringLiteral("task:idle"));
        return true;
    };
    hooks.tasks = {task};

    MessageCapture capture;
    QVERIFY(lvrs::scheduleQmlAppLifecycleStage(qGuiApp,
                                               context,
                                               hooks,
                                               lvrs::QmlAppLifecycleStage::AfterFirstIdle));

    QTRY_VERIFY(calls->size() == 2);
    QCOMPARE(*calls,
             QStringList({
                 QStringLiteral("hook:after-first-idle"),
                 QStringLiteral("task:idle")
             }));

    const QString logs = capture.joinedMessages();
    QVERIFY(logs.contains(QStringLiteral("LVRS bootstrap.lifecycle.stage-start")));
    QVERIFY(logs.contains(QStringLiteral("\"stage\":\"after-first-idle\"")));
}

QTEST_MAIN(AppEntryTests)
#include "tst_app_entry.moc"
