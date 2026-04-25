#include <QtTest>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QWindow>

#include "backend/runtime/foregroundservices.h"

class ForegroundServicesTests : public QObject
{
    Q_OBJECT

private slots:
    void gate_requires_visible_workspace_and_starts_services_once();
    void gate_reports_failures_and_can_be_reset();
};

void ForegroundServicesTests::gate_requires_visible_workspace_and_starts_services_once()
{
    QWindow window;
    lvrs::QmlAppLifecycleContext context;
    context.application = qGuiApp;
    context.rootLoadResult.ok = true;
    context.rootLoadResult.windows.append(&window);

    lvrs::ForegroundServiceGate gate;

    lvrs::ForegroundServiceStartOptions options;
    options.logDiagnostics = false;
    options.metadata = {{QStringLiteral("phase"), QStringLiteral("workspace-visible")}};

    const lvrs::ForegroundServiceStartResult notReady =
        gate.startOnceWhenWorkspaceVisible(context, {}, options);
    QVERIFY(!notReady.ok);
    QVERIFY(!notReady.started);
    QVERIFY(!notReady.alreadyStarted);
    QVERIFY(!notReady.visibleWorkspace);
    QCOMPARE(gate.started(), false);
    QCOMPARE(gate.startAttemptCount(), 0);
    QVERIFY(notReady.errorMessage().contains(QStringLiteral("visible workspace")));

    window.show();
    QTRY_VERIFY(window.isVisible());

    QStringList calls;
    auto makeTask = [&](const QString &name, int priority) {
        lvrs::ForegroundServiceTask task;
        task.name = name;
        task.priority = priority;
        task.metadata = {{QStringLiteral("service"), name}};
        task.start = [&](const lvrs::ForegroundServiceStartContext &startContext, QString *) -> bool {
            if (startContext.application != qGuiApp)
                return false;
            if (startContext.visibleWindows.size() != 1)
                return false;
            if (startContext.visibleWindows.first() != &window)
                return false;
            if (startContext.metadata.value(QStringLiteral("phase")).toString()
                != QStringLiteral("workspace-visible")) {
                return false;
            }
            calls.append(name);
            return true;
        };
        return task;
    };

    const lvrs::ForegroundServiceStartResult started =
        gate.startOnceWhenWorkspaceVisible(context,
                                           {
                                               makeTask(QStringLiteral("scheduler"), 20),
                                               makeTask(QStringLiteral("permission-bootstrap"), 10)
                                           },
                                           options);

    QVERIFY2(started.ok, qPrintable(started.errorMessage()));
    QVERIFY(started.started);
    QVERIFY(!started.alreadyStarted);
    QVERIFY(started.visibleWorkspace);
    QCOMPARE(started.visibleWindowCount, 1);
    QCOMPARE(started.taskResults.size(), 2);
    QCOMPARE(calls,
             (QStringList {
                 QStringLiteral("permission-bootstrap"),
                 QStringLiteral("scheduler")
             }));
    QVERIFY(gate.started());
    QCOMPARE(gate.startAttemptCount(), 1);
    QCOMPARE(started.taskResults.at(0).metadata.value(QStringLiteral("service")).toString(),
             QStringLiteral("permission-bootstrap"));
    QCOMPARE(started.diagnostics().size(), 2);

    const lvrs::ForegroundServiceStartResult duplicate =
        gate.startOnceWhenWorkspaceVisible(context, {makeTask(QStringLiteral("duplicate"), 0)}, options);
    QVERIFY(duplicate.ok);
    QVERIFY(!duplicate.started);
    QVERIFY(duplicate.alreadyStarted);
    QCOMPARE(duplicate.taskResults.size(), 0);
    QCOMPARE(gate.startAttemptCount(), 1);
    QCOMPARE(calls.size(), 2);

    window.close();
}

void ForegroundServicesTests::gate_reports_failures_and_can_be_reset()
{
    QWindow window;
    window.show();
    QTRY_VERIFY(window.isVisible());

    lvrs::QmlAppLifecycleContext context;
    context.application = qGuiApp;
    context.rootLoadResult.ok = true;
    context.rootLoadResult.windows.append(&window);

    lvrs::ForegroundServiceTask recoverable;
    recoverable.name = QStringLiteral("recoverable-service");
    recoverable.priority = 10;
    recoverable.start = [](const lvrs::ForegroundServiceStartContext &, QString *errorMessage) {
        if (errorMessage)
            *errorMessage = QStringLiteral("recoverable startup failed");
        return false;
    };

    lvrs::ForegroundServiceTask fatal;
    fatal.name = QStringLiteral("fatal-service");
    fatal.priority = 20;
    fatal.fatal = true;
    fatal.start = [](const lvrs::ForegroundServiceStartContext &, QString *errorMessage) {
        if (errorMessage)
            *errorMessage = QStringLiteral("fatal startup failed");
        return false;
    };

    lvrs::ForegroundServiceStartOptions options;
    options.logDiagnostics = false;

    lvrs::ForegroundServiceGate gate;
    const lvrs::ForegroundServiceStartResult failed =
        gate.startOnceWhenWorkspaceVisible(context, {fatal, recoverable}, options);

    QVERIFY(!failed.ok);
    QVERIFY(failed.started);
    QVERIFY(failed.fatalFailure());
    QCOMPARE(failed.taskResults.size(), 2);
    QCOMPARE(failed.taskResults.at(0).name, QStringLiteral("recoverable-service"));
    QVERIFY(!failed.taskResults.at(0).fatal);
    QCOMPARE(failed.taskResults.at(1).name, QStringLiteral("fatal-service"));
    QVERIFY(failed.taskResults.at(1).fatal);
    QVERIFY(failed.errorMessage().contains(QStringLiteral("recoverable startup failed")));
    QVERIFY(failed.errorMessage().contains(QStringLiteral("fatal startup failed")));
    QVERIFY(gate.started());
    QCOMPARE(gate.startAttemptCount(), 1);

    const lvrs::ForegroundServiceStartResult duplicate =
        gate.startOnceWhenWorkspaceVisible(context, {recoverable}, options);
    QVERIFY(duplicate.ok);
    QVERIFY(duplicate.alreadyStarted);

    gate.reset();
    QVERIFY(!gate.started());
    QCOMPARE(gate.startAttemptCount(), 0);

    bool restarted = false;
    lvrs::ForegroundServiceTask success;
    success.name = QStringLiteral("restart-success");
    success.start = [&](const lvrs::ForegroundServiceStartContext &, QString *) {
        restarted = true;
        return true;
    };

    const lvrs::ForegroundServiceStartResult afterReset =
        gate.startOnceWhenWorkspaceVisible(context, {success}, options);
    QVERIFY(afterReset.ok);
    QVERIFY(afterReset.started);
    QVERIFY(restarted);
    QCOMPARE(gate.startAttemptCount(), 1);

    window.close();
}

QTEST_MAIN(ForegroundServicesTests)
#include "tst_foreground_services.moc"
