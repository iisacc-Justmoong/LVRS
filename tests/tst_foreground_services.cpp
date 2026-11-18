#include "QtEventWait.h"
#include <QGuiApplication>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QWindow>

#include "backend/runtime/foregroundservices.h"

class ForegroundServicesTests : public QObject
{
public:
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
    CTEST_CHECK(!notReady.ok);
    CTEST_CHECK(!notReady.started);
    CTEST_CHECK(!notReady.alreadyStarted);
    CTEST_CHECK(!notReady.visibleWorkspace);
    CTEST_COMPARE(gate.started(), false);
    CTEST_COMPARE(gate.startAttemptCount(), 0);
    CTEST_CHECK(notReady.errorMessage().contains(QStringLiteral("visible workspace")));

    window.show();
    CTEST_WAIT_DEFAULT(window.isVisible());

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

    CTEST_CHECK_MESSAGE(started.ok, qPrintable(started.errorMessage()));
    CTEST_CHECK(started.started);
    CTEST_CHECK(!started.alreadyStarted);
    CTEST_CHECK(started.visibleWorkspace);
    CTEST_COMPARE(started.visibleWindowCount, 1);
    CTEST_COMPARE(started.taskResults.size(), 2);
    CTEST_COMPARE(calls,
             (QStringList {
                 QStringLiteral("permission-bootstrap"),
                 QStringLiteral("scheduler")
             }));
    CTEST_CHECK(gate.started());
    CTEST_COMPARE(gate.startAttemptCount(), 1);
    CTEST_COMPARE(started.taskResults.at(0).metadata.value(QStringLiteral("service")).toString(),
             QStringLiteral("permission-bootstrap"));
    CTEST_COMPARE(started.diagnostics().size(), 2);

    const lvrs::ForegroundServiceStartResult duplicate =
        gate.startOnceWhenWorkspaceVisible(context, {makeTask(QStringLiteral("duplicate"), 0)}, options);
    CTEST_CHECK(duplicate.ok);
    CTEST_CHECK(!duplicate.started);
    CTEST_CHECK(duplicate.alreadyStarted);
    CTEST_COMPARE(duplicate.taskResults.size(), 0);
    CTEST_COMPARE(gate.startAttemptCount(), 1);
    CTEST_COMPARE(calls.size(), 2);

    window.close();
}

void ForegroundServicesTests::gate_reports_failures_and_can_be_reset()
{
    QWindow window;
    window.show();
    CTEST_WAIT_DEFAULT(window.isVisible());

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

    CTEST_CHECK(!failed.ok);
    CTEST_CHECK(failed.started);
    CTEST_CHECK(failed.fatalFailure());
    CTEST_COMPARE(failed.taskResults.size(), 2);
    CTEST_COMPARE(failed.taskResults.at(0).name, QStringLiteral("recoverable-service"));
    CTEST_CHECK(!failed.taskResults.at(0).fatal);
    CTEST_COMPARE(failed.taskResults.at(1).name, QStringLiteral("fatal-service"));
    CTEST_CHECK(failed.taskResults.at(1).fatal);
    CTEST_CHECK(failed.errorMessage().contains(QStringLiteral("recoverable startup failed")));
    CTEST_CHECK(failed.errorMessage().contains(QStringLiteral("fatal startup failed")));
    CTEST_CHECK(gate.started());
    CTEST_COMPARE(gate.startAttemptCount(), 1);

    const lvrs::ForegroundServiceStartResult duplicate =
        gate.startOnceWhenWorkspaceVisible(context, {recoverable}, options);
    CTEST_CHECK(duplicate.ok);
    CTEST_CHECK(duplicate.alreadyStarted);

    gate.reset();
    CTEST_CHECK(!gate.started());
    CTEST_COMPARE(gate.startAttemptCount(), 0);

    bool restarted = false;
    lvrs::ForegroundServiceTask success;
    success.name = QStringLiteral("restart-success");
    success.start = [&](const lvrs::ForegroundServiceStartContext &, QString *) {
        restarted = true;
        return true;
    };

    const lvrs::ForegroundServiceStartResult afterReset =
        gate.startOnceWhenWorkspaceVisible(context, {success}, options);
    CTEST_CHECK(afterReset.ok);
    CTEST_CHECK(afterReset.started);
    CTEST_CHECK(restarted);
    CTEST_COMPARE(gate.startAttemptCount(), 1);

    window.close();
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    ForegroundServicesTests tests;
    runner.run("gate_requires_visible_workspace_and_starts_services_once", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.gate_requires_visible_workspace_and_starts_services_once(); });
    runner.run("gate_reports_failures_and_can_be_reset", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.gate_reports_failures_and_can_be_reset(); });
    return runner.result();
}
