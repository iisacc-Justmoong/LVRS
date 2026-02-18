#include <QtTest>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtPlugin>

#include "backend/io/backend.h"
#include "backend/runtime/runtimeevents.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class BackendIoTests : public QObject
{
    Q_OBJECT

private slots:
    void backend_file_roundtrip_and_errors();
    void backend_error_signal_and_directory_idempotence();
    void backend_event_hook_receives_runtime_events();
    void backend_async_concurrency_and_io_contract();
    void backend_async_delay_distribution_and_read_cache_contract();
};

void BackendIoTests::backend_file_roundtrip_and_errors()
{
    Backend backend;
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString nestedDir = tempDir.path() + "/io/a/b";
    QVERIFY(backend.ensureDir(nestedDir));

    const QString filePath = nestedDir + "/sample.txt";
    QVERIFY(backend.saveTextFile(filePath, QStringLiteral("metrics-check")));
    QCOMPARE(backend.readTextFile(filePath), QStringLiteral("metrics-check"));

    QVERIFY(!backend.saveTextFile(QString(), QStringLiteral("x")));
    QVERIFY(!backend.lastError().isEmpty());

    const QString missingPath = nestedDir + "/missing.txt";
    QCOMPARE(backend.readTextFile(missingPath), QString());
    QVERIFY(!backend.lastError().isEmpty());

    const QString tempLocation = backend.writableLocation(static_cast<int>(QStandardPaths::TempLocation));
    QVERIFY(!tempLocation.isEmpty());
}

void BackendIoTests::backend_error_signal_and_directory_idempotence()
{
    Backend backend;
    QSignalSpy errorSpy(&backend, &Backend::lastErrorChanged);
    QVERIFY(errorSpy.isValid());

    QVERIFY(!backend.ensureDir(QStringLiteral("   ")));
    QCOMPARE(backend.lastError(), QStringLiteral("Empty path"));

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString targetDir = tempDir.path() + "/x/y/z";
    QVERIFY(backend.ensureDir(targetDir));
    QCOMPARE(backend.lastError(), QString());
    QVERIFY(backend.ensureDir(targetDir));
    QCOMPARE(backend.lastError(), QString());

    QVERIFY(!backend.readTextFile(QStringLiteral(" ")).size());
    QCOMPARE(backend.lastError(), QStringLiteral("Empty path"));

    const QString filePath = targetDir + "/coverage.txt";
    QVERIFY(backend.saveTextFile(filePath, QStringLiteral("ok")));
    QCOMPARE(backend.lastError(), QString());
    QCOMPARE(backend.readTextFile(filePath), QStringLiteral("ok"));
    QCOMPARE(backend.lastError(), QString());

    QVERIFY(errorSpy.count() >= 4);
}

void BackendIoTests::backend_event_hook_receives_runtime_events()
{
    RuntimeEvents runtime;
    runtime.start();

    QQuickWindow window;
    window.setWidth(320);
    window.setHeight(180);
    runtime.attachWindow(&window);

    Backend backend;
    QVERIFY(backend.hookUserEvents());
    QVERIFY(backend.userEventHooked());

    backend.clearHookedUserEvents();
    QCOMPARE(backend.hookedEventCount(), 0);

    QSignalSpy hookedSpy(&backend, &Backend::hookedEventsChanged);
    QVERIFY(hookedSpy.isValid());

    const QPointF point(28.0, 20.0);
    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, QStringLiteral("a"));
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier, QStringLiteral("a"));
    QMouseEvent mousePress(QEvent::MouseButtonPress,
                           point,
                           point,
                           point,
                           Qt::LeftButton,
                           Qt::LeftButton,
                           Qt::NoModifier);
    QMouseEvent mouseRelease(QEvent::MouseButtonRelease,
                             point,
                             point,
                             point,
                             Qt::LeftButton,
                             Qt::NoButton,
                             Qt::NoModifier);

    QCoreApplication::sendEvent(&window, &keyPress);
    QCoreApplication::sendEvent(&window, &mousePress);
    QCoreApplication::sendEvent(&window, &mouseRelease);
    QCoreApplication::sendEvent(&window, &keyRelease);

    QTRY_VERIFY(backend.hookedEventCount() >= 4);
    QVERIFY(hookedSpy.count() >= 1);

    const QVariantMap last = backend.lastHookedEvent();
    QVERIFY(!last.isEmpty());
    QVERIFY(!last.value(QStringLiteral("type")).toString().isEmpty());
    QVERIFY(last.contains(QStringLiteral("payload")));

    const QVariantMap summary = backend.hookedUserEventSummary();
    QVERIFY(summary.value(QStringLiteral("hooked")).toBool());
    QVERIFY(summary.value(QStringLiteral("eventCount")).toInt() >= 4);
    const QVariantMap typeCounts = summary.value(QStringLiteral("typeCounts")).toMap();
    QVERIFY(!typeCounts.isEmpty());

    const QVariantMap input = backend.currentUserInputState();
    QVERIFY(input.contains(QStringLiteral("anyKeyPressed")));
    QVERIFY(input.contains(QStringLiteral("pointerUi")));
    const QVariantMap pointerUi = input.value(QStringLiteral("pointerUi")).toMap();
    QVERIFY(pointerUi.contains(QStringLiteral("objectName")));

    backend.unhookUserEvents();
    QVERIFY(!backend.userEventHooked());
}

void BackendIoTests::backend_async_concurrency_and_io_contract()
{
    Backend backend;
    backend.setAsyncMaxConcurrency(2);
    QCOMPARE(backend.asyncMaxConcurrency(), 2);

    QSignalSpy queuedSpy(&backend, &Backend::asyncRequestQueued);
    QSignalSpy finishedSpy(&backend, &Backend::asyncRequestFinished);
    QSignalSpy inFlightSpy(&backend, &Backend::asyncJobsInFlightChanged);
    QVERIFY(queuedSpy.isValid());
    QVERIFY(finishedSpy.isValid());
    QVERIFY(inFlightSpy.isValid());

    auto hasFinishedRequest = [&finishedSpy](qulonglong requestId) {
        for (const QList<QVariant> &args : finishedSpy) {
            if (args.size() >= 7 && args.at(0).toULongLong() == requestId)
                return true;
        }
        return false;
    };

    auto finishedArgsFor = [&finishedSpy](qulonglong requestId) {
        for (const QList<QVariant> &args : finishedSpy) {
            if (args.size() >= 7 && args.at(0).toULongLong() == requestId)
                return args;
        }
        return QList<QVariant>();
    };

    QElapsedTimer timer;
    timer.start();
    const qulonglong taskA = backend.dispatchAsyncTask(QStringLiteral("ui-task-a"),
                                                       QVariantMap { { QStringLiteral("key"), QStringLiteral("a") } },
                                                       260);
    const qulonglong taskB = backend.dispatchAsyncTask(QStringLiteral("ui-task-b"),
                                                       QVariantMap { { QStringLiteral("key"), QStringLiteral("b") } },
                                                       260);
    QTRY_VERIFY(backend.asyncJobsInFlight() > 0);
    QTRY_VERIFY(hasFinishedRequest(taskA));
    QTRY_VERIFY(hasFinishedRequest(taskB));

    const qint64 elapsedMs = timer.elapsed();
    QVERIFY2(elapsedMs < 430,
             qPrintable(QStringLiteral("Expected parallel execution (<430ms), elapsed=%1ms").arg(elapsedMs)));

    const QList<QVariant> taskAArgs = finishedArgsFor(taskA);
    const QList<QVariant> taskBArgs = finishedArgsFor(taskB);
    QVERIFY(taskAArgs.size() >= 7);
    QVERIFY(taskBArgs.size() >= 7);
    QCOMPARE(taskAArgs.at(1).toString(), QStringLiteral("dispatchTask"));
    QCOMPARE(taskBArgs.at(1).toString(), QStringLiteral("dispatchTask"));
    QVERIFY(taskAArgs.at(3).toBool());
    QVERIFY(taskBArgs.at(3).toBool());
    QCOMPARE(taskAArgs.at(4).toMap().value(QStringLiteral("key")).toString(), QStringLiteral("a"));
    QCOMPARE(taskBArgs.at(4).toMap().value(QStringLiteral("key")).toString(), QStringLiteral("b"));

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString nestedDir = tempDir.path() + "/async/contract";
    const QString filePath = nestedDir + "/sample.txt";

    const qulonglong ensureId = backend.ensureDirAsync(nestedDir);
    QTRY_VERIFY(hasFinishedRequest(ensureId));
    const QList<QVariant> ensureArgs = finishedArgsFor(ensureId);
    QVERIFY(ensureArgs.size() >= 7);
    QCOMPARE(ensureArgs.at(1).toString(), QStringLiteral("ensureDir"));
    QVERIFY(ensureArgs.at(3).toBool());
    QVERIFY(ensureArgs.at(4).toMap().value(QStringLiteral("ensured")).toBool());

    const qulonglong saveId = backend.saveTextFileAsync(filePath, QStringLiteral("async-metrics"));
    QTRY_VERIFY(hasFinishedRequest(saveId));
    const QList<QVariant> saveArgs = finishedArgsFor(saveId);
    QVERIFY(saveArgs.size() >= 7);
    QCOMPARE(saveArgs.at(1).toString(), QStringLiteral("saveTextFile"));
    QVERIFY(saveArgs.at(3).toBool());
    QVERIFY(saveArgs.at(4).toMap().value(QStringLiteral("bytes")).toLongLong() > 0);

    const qulonglong readId = backend.readTextFileAsync(filePath);
    QTRY_VERIFY(hasFinishedRequest(readId));
    const QList<QVariant> readArgs = finishedArgsFor(readId);
    QVERIFY(readArgs.size() >= 7);
    QCOMPARE(readArgs.at(1).toString(), QStringLiteral("readTextFile"));
    QVERIFY(readArgs.at(3).toBool());
    QCOMPARE(readArgs.at(4).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("async-metrics"));
    QCOMPARE(readArgs.at(4).toMap().value(QStringLiteral("length")).toInt(),
             QStringLiteral("async-metrics").length());

    const qulonglong badReadId = backend.readTextFileAsync(QStringLiteral(" "));
    QTRY_VERIFY(hasFinishedRequest(badReadId));
    const QList<QVariant> badReadArgs = finishedArgsFor(badReadId);
    QVERIFY(badReadArgs.size() >= 7);
    QCOMPARE(badReadArgs.at(1).toString(), QStringLiteral("readTextFile"));
    QVERIFY(!badReadArgs.at(3).toBool());
    QCOMPARE(badReadArgs.at(5).toString(), QStringLiteral("Empty path"));
    QCOMPARE(backend.lastError(), QStringLiteral("Empty path"));

    QTRY_COMPARE(backend.asyncJobsInFlight(), 0);
    QVERIFY(queuedSpy.count() >= 6);
    QVERIFY(finishedSpy.count() >= 6);
}

void BackendIoTests::backend_async_delay_distribution_and_read_cache_contract()
{
    Backend backend;
    backend.setAsyncMaxConcurrency(1);
    QCOMPARE(backend.asyncMaxConcurrency(), 1);

    QSignalSpy finishedSpy(&backend, &Backend::asyncRequestFinished);
    QVERIFY(finishedSpy.isValid());

    auto hasFinishedRequest = [&finishedSpy](qulonglong requestId) {
        for (const QList<QVariant> &args : finishedSpy) {
            if (args.size() >= 7 && args.at(0).toULongLong() == requestId)
                return true;
        }
        return false;
    };

    auto finishedArgsFor = [&finishedSpy](qulonglong requestId) {
        for (const QList<QVariant> &args : finishedSpy) {
            if (args.size() >= 7 && args.at(0).toULongLong() == requestId)
                return args;
        }
        return QList<QVariant>();
    };

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString filePath = tempDir.path() + "/distribution.txt";
    QFile seedFile(filePath);
    QVERIFY(seedFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(seedFile.write("delay-distribution-check") > 0);
    seedFile.close();

    QElapsedTimer timer;
    timer.start();
    const qulonglong delayedId = backend.dispatchAsyncTask(QStringLiteral("delayed-task"),
                                                           QVariantMap(),
                                                           320);
    const qulonglong readId = backend.readTextFileAsync(filePath);

    QTRY_VERIFY(hasFinishedRequest(readId));
    const qint64 firstReadElapsedMs = timer.elapsed();
    QVERIFY2(firstReadElapsedMs < 260,
             qPrintable(QStringLiteral("Expected read to bypass delayed-task queue blocking (<260ms), elapsed=%1ms")
                            .arg(firstReadElapsedMs)));

    const QList<QVariant> firstReadArgs = finishedArgsFor(readId);
    QVERIFY(firstReadArgs.size() >= 7);
    QCOMPARE(firstReadArgs.at(1).toString(), QStringLiteral("readTextFile"));
    QVERIFY(firstReadArgs.at(3).toBool());
    QCOMPARE(firstReadArgs.at(4).toMap().value(QStringLiteral("text")).toString(),
             QStringLiteral("delay-distribution-check"));
    QCOMPARE(firstReadArgs.at(4).toMap().value(QStringLiteral("cached")).toBool(), false);

    QTRY_VERIFY(hasFinishedRequest(delayedId));
    const QList<QVariant> delayedArgs = finishedArgsFor(delayedId);
    QVERIFY(delayedArgs.size() >= 7);
    QCOMPARE(delayedArgs.at(1).toString(), QStringLiteral("dispatchTask"));
    QVERIFY(delayedArgs.at(3).toBool());
    QVERIFY(delayedArgs.at(6).toLongLong() >= 300);

    QElapsedTimer cacheTimer;
    cacheTimer.start();
    const qulonglong cachedReadId = backend.readTextFileAsync(filePath);
    QTRY_VERIFY(hasFinishedRequest(cachedReadId));
    const qint64 cachedReadElapsedMs = cacheTimer.elapsed();
    QVERIFY2(cachedReadElapsedMs < 120,
             qPrintable(QStringLiteral("Expected cache hit to complete quickly (<120ms), elapsed=%1ms")
                            .arg(cachedReadElapsedMs)));

    const QList<QVariant> cachedReadArgs = finishedArgsFor(cachedReadId);
    QVERIFY(cachedReadArgs.size() >= 7);
    QCOMPARE(cachedReadArgs.at(1).toString(), QStringLiteral("readTextFile"));
    QVERIFY(cachedReadArgs.at(3).toBool());
    QCOMPARE(cachedReadArgs.at(4).toMap().value(QStringLiteral("cached")).toBool(), true);

    QTRY_COMPARE(backend.asyncJobsInFlight(), 0);
}

QTEST_MAIN(BackendIoTests)
#include "tst_backend_io.moc"
