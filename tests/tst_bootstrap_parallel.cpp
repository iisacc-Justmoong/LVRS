#include "QtEventWait.h"
#include <QGuiApplication>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>
#include <QVariantMap>
#include <QWaitCondition>

#include "backend/runtime/bootstrapparallel.h"

class BootstrapParallelTests : public QObject
{
public:
    void parallel_tasks_run_concurrently_and_apply_on_receiver_thread();
    void parallel_runner_reports_load_and_apply_failures();
};

void BootstrapParallelTests::parallel_tasks_run_concurrently_and_apply_on_receiver_thread()
{
    QMutex barrierMutex;
    QWaitCondition barrierCondition;
    int startedCount = 0;

    QStringList applyOrder;
    QList<bool> workerThreadFlags;
    bool applyThreadOk = true;

    auto makeTask = [&](const QString &name, int priority) {
        lvrs::BootstrapParallelTask task;
        task.name = name;
        task.priority = priority;
        task.metadata = {{QStringLiteral("domain"), name}};
        task.load = [&](const lvrs::BootstrapParallelTaskContext &context,
                        QVariant *value,
                        QString *errorMessage) {
            {
                QMutexLocker locker(&barrierMutex);
                startedCount += 1;
                barrierCondition.wakeAll();

                QElapsedTimer waitTimer;
                waitTimer.start();
                while (startedCount < 2 && waitTimer.elapsed() < 1000)
                    barrierCondition.wait(&barrierMutex, 50);

                if (startedCount < 2) {
                    if (errorMessage)
                        *errorMessage = QStringLiteral("parallel barrier was not reached");
                    return false;
                }
            }

            QThread::msleep(25);
            QVariantMap payload;
            payload.insert(QStringLiteral("name"), context.name);
            payload.insert(QStringLiteral("workerThread"), QThread::currentThread() != qApp->thread());
            payload.insert(QStringLiteral("domain"), context.metadata.value(QStringLiteral("domain")));
            if (value)
                *value = payload;
            return true;
        };
        task.apply = [&](const lvrs::BootstrapParallelTaskResult &result, QString *) {
            applyThreadOk = applyThreadOk && QThread::currentThread() == qApp->thread();
            applyOrder.append(result.name);
            workerThreadFlags.append(result.value.toMap().value(QStringLiteral("workerThread")).toBool());
            return true;
        };
        return task;
    };

    lvrs::BootstrapParallelRunOptions options;
    options.applyReceiver = qApp;
    options.maxThreadCount = 2;
    options.logDiagnostics = false;

    const lvrs::BootstrapParallelRunResult result = lvrs::runBootstrapParallelTasks({
        makeTask(QStringLiteral("second"), 20),
        makeTask(QStringLiteral("first"), 10)
    }, options);

    CTEST_CHECK_MESSAGE(result.ok, qPrintable(result.errorMessage()));
    CTEST_CHECK(!result.fatalFailure());
    CTEST_COMPARE(result.taskResults.size(), 2);
    CTEST_COMPARE(startedCount, 2);
    CTEST_CHECK(applyThreadOk);
    CTEST_COMPARE(applyOrder, (QStringList {QStringLiteral("first"), QStringLiteral("second")}));
    CTEST_COMPARE(workerThreadFlags, (QList<bool> {true, true}));
    CTEST_COMPARE(result.taskResults.at(0).metadata.value(QStringLiteral("domain")).toString(),
             QStringLiteral("first"));
    CTEST_CHECK(result.taskResults.at(0).loadElapsedMs >= 0);
    CTEST_CHECK(result.taskResults.at(0).applied);
    CTEST_CHECK(result.diagnostics().size() == 2);
}

void BootstrapParallelTests::parallel_runner_reports_load_and_apply_failures()
{
    bool skippedApplyCalled = false;

    lvrs::BootstrapParallelTask missingLoad;
    missingLoad.name = QStringLiteral("missing-load");
    missingLoad.priority = -10;
    missingLoad.fatal = true;

    lvrs::BootstrapParallelTask loadFailure;
    loadFailure.name = QStringLiteral("load-failure");
    loadFailure.priority = 10;
    loadFailure.load = [](const lvrs::BootstrapParallelTaskContext &, QVariant *, QString *errorMessage) {
        if (errorMessage)
            *errorMessage = QStringLiteral("domain load failed");
        return false;
    };
    loadFailure.apply = [&](const lvrs::BootstrapParallelTaskResult &, QString *) {
        skippedApplyCalled = true;
        return true;
    };

    lvrs::BootstrapParallelTask applyFailure;
    applyFailure.name = QStringLiteral("apply-failure");
    applyFailure.priority = 30;
    applyFailure.load = [](const lvrs::BootstrapParallelTaskContext &, QVariant *value, QString *) {
        if (value)
            *value = QStringLiteral("loaded");
        return true;
    };
    applyFailure.apply = [](const lvrs::BootstrapParallelTaskResult &, QString *errorMessage) {
        if (errorMessage)
            *errorMessage = QStringLiteral("main-thread apply failed");
        return false;
    };

    lvrs::BootstrapParallelRunOptions options;
    options.applyReceiver = qApp;
    options.maxThreadCount = 2;
    options.logDiagnostics = false;

    const lvrs::BootstrapParallelRunResult result = lvrs::runBootstrapParallelTasks({
        applyFailure,
        loadFailure,
        missingLoad
    }, options);

    CTEST_CHECK(!result.ok);
    CTEST_CHECK(result.fatalFailure());
    CTEST_COMPARE(result.taskResults.size(), 3);
    CTEST_CHECK(!skippedApplyCalled);
    CTEST_CHECK(result.errorMessage().contains(QStringLiteral("missing-load")));
    CTEST_CHECK(result.errorMessage().contains(QStringLiteral("domain load failed")));
    CTEST_CHECK(result.errorMessage().contains(QStringLiteral("main-thread apply failed")));
    CTEST_CHECK(!result.taskResults.at(0).ok);
    CTEST_CHECK(result.taskResults.at(0).fatal);
    CTEST_CHECK(!result.taskResults.at(1).applied);
    CTEST_CHECK(result.taskResults.at(2).applied);
    CTEST_CHECK(!result.taskResults.at(2).applyOk);
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    BootstrapParallelTests tests;
    runner.run("parallel_tasks_run_concurrently_and_apply_on_receiver_thread", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.parallel_tasks_run_concurrently_and_apply_on_receiver_thread(); });
    runner.run("parallel_runner_reports_load_and_apply_failures", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.parallel_runner_reports_load_and_apply_failures(); });
    return runner.result();
}
