#include <QtTest>

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
    Q_OBJECT

private slots:
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

    QVERIFY2(result.ok, qPrintable(result.errorMessage()));
    QVERIFY(!result.fatalFailure());
    QCOMPARE(result.taskResults.size(), 2);
    QCOMPARE(startedCount, 2);
    QVERIFY(applyThreadOk);
    QCOMPARE(applyOrder, (QStringList {QStringLiteral("first"), QStringLiteral("second")}));
    QCOMPARE(workerThreadFlags, (QList<bool> {true, true}));
    QCOMPARE(result.taskResults.at(0).metadata.value(QStringLiteral("domain")).toString(),
             QStringLiteral("first"));
    QVERIFY(result.taskResults.at(0).loadElapsedMs >= 0);
    QVERIFY(result.taskResults.at(0).applied);
    QVERIFY(result.diagnostics().size() == 2);
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

    QVERIFY(!result.ok);
    QVERIFY(result.fatalFailure());
    QCOMPARE(result.taskResults.size(), 3);
    QVERIFY(!skippedApplyCalled);
    QVERIFY(result.errorMessage().contains(QStringLiteral("missing-load")));
    QVERIFY(result.errorMessage().contains(QStringLiteral("domain load failed")));
    QVERIFY(result.errorMessage().contains(QStringLiteral("main-thread apply failed")));
    QVERIFY(!result.taskResults.at(0).ok);
    QVERIFY(result.taskResults.at(0).fatal);
    QVERIFY(!result.taskResults.at(1).applied);
    QVERIFY(result.taskResults.at(2).applied);
    QVERIFY(!result.taskResults.at(2).applyOk);
}

QTEST_MAIN(BootstrapParallelTests)
#include "tst_bootstrap_parallel.moc"
