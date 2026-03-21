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
    void backend_writable_location_contract_respects_platform_defaults();
    void backend_error_signal_and_directory_idempotence();
    void backend_event_hook_receives_runtime_events();
    void backend_async_concurrency_and_io_contract();
    void backend_async_delay_distribution_and_read_cache_contract();
    void backend_performance_metrics_and_trace_contract();
    void backend_p1_scheduler_backpressure_cancel_and_cache_capacity_contract();
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

void BackendIoTests::backend_writable_location_contract_respects_platform_defaults()
{
    Backend backend;

    const QString tempLocation = backend.writableLocation(static_cast<int>(QStandardPaths::TempLocation));
    QVERIFY(!tempLocation.isEmpty());
    QCOMPARE(tempLocation, QStandardPaths::writableLocation(QStandardPaths::TempLocation));

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    const QList<QStandardPaths::StandardLocation> mobileFallbackLocations = {
        QStandardPaths::DesktopLocation,
        QStandardPaths::ApplicationsLocation,
        QStandardPaths::DownloadLocation
    };

    for (QStandardPaths::StandardLocation location : mobileFallbackLocations)
        QVERIFY(!backend.writableLocation(static_cast<int>(location)).isEmpty());
#endif
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
    backend.setReadTextCacheTtlMs(150);
    QCOMPARE(backend.readTextCacheTtlMs(), 150);

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
    QVERIFY(delayedArgs.at(6).toLongLong() < 220);
    QCOMPARE(delayedArgs.at(4).toMap().value(QStringLiteral("delayMs")).toInt(), 0);
    QCOMPARE(delayedArgs.at(4).toMap().value(QStringLiteral("requestedDelayMs")).toInt(), 320);

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

    QTest::qWait(220);
    const qulonglong expiredReadId = backend.readTextFileAsync(filePath);
    QTRY_VERIFY(hasFinishedRequest(expiredReadId));
    const QList<QVariant> expiredReadArgs = finishedArgsFor(expiredReadId);
    QVERIFY(expiredReadArgs.size() >= 7);
    QCOMPARE(expiredReadArgs.at(1).toString(), QStringLiteral("readTextFile"));
    QVERIFY(expiredReadArgs.at(3).toBool());
    QCOMPARE(expiredReadArgs.at(4).toMap().value(QStringLiteral("cached")).toBool(), false);

    QTRY_COMPARE(backend.asyncJobsInFlight(), 0);
}

void BackendIoTests::backend_performance_metrics_and_trace_contract()
{
    Backend backend;
    backend.setAsyncMaxConcurrency(1);
    backend.setPerformanceTraceCapacity(512);
    QCOMPARE(backend.performanceTraceCapacity(), 512);

    QSignalSpy traceSpy(&backend, &Backend::performanceTraceChanged);
    QSignalSpy metricsSpy(&backend, &Backend::performanceMetricsChanged);
    QSignalSpy finishedSpy(&backend, &Backend::asyncRequestFinished);
    QVERIFY(traceSpy.isValid());
    QVERIFY(metricsSpy.isValid());
    QVERIFY(finishedSpy.isValid());

    auto hasFinishedRequest = [&finishedSpy](qulonglong requestId) {
        for (const QList<QVariant> &args : finishedSpy) {
            if (args.size() >= 7 && args.at(0).toULongLong() == requestId)
                return true;
        }
        return false;
    };

    const qulonglong taskA = backend.dispatchAsyncTask(QStringLiteral("perf-task-a"),
                                                       QVariantMap { { QStringLiteral("kind"), QStringLiteral("a") } },
                                                       0);
    const qulonglong taskB = backend.dispatchAsyncTask(QStringLiteral("perf-task-b"),
                                                       QVariantMap { { QStringLiteral("kind"), QStringLiteral("b") } },
                                                       0);
    QTRY_VERIFY(hasFinishedRequest(taskA));
    QTRY_VERIFY(hasFinishedRequest(taskB));
    QTRY_COMPARE(backend.asyncJobsInFlight(), 0);

    const QVariantMap metrics = backend.performanceMetrics();
    QCOMPARE(metrics.value(QStringLiteral("schema")).toString(), QStringLiteral("lvrs.performance.v1"));
    QCOMPARE(metrics.value(QStringLiteral("component")).toString(), QStringLiteral("Backend"));
    QCOMPARE(metrics.value(QStringLiteral("asyncMaxConcurrency")).toInt(), 1);
    QVERIFY(metrics.contains(QStringLiteral("asyncQueueDepth")));
    QVERIFY(metrics.contains(QStringLiteral("asyncQueuePeakDepth")));
    QVERIFY(metrics.contains(QStringLiteral("performanceTraceCount")));

    const QVariantMap byOperation = metrics.value(QStringLiteral("asyncLatencyByOperation")).toMap();
    QVERIFY(byOperation.contains(QStringLiteral("dispatchTask")));
    const QVariantMap dispatchLatency = byOperation.value(QStringLiteral("dispatchTask")).toMap();
    QVERIFY(dispatchLatency.value(QStringLiteral("count")).toULongLong() >= 2);
    QVERIFY(dispatchLatency.contains(QStringLiteral("avgMs")));
    QVERIFY(dispatchLatency.contains(QStringLiteral("p95Ms")));
    QVERIFY(dispatchLatency.contains(QStringLiteral("p99Ms")));

    const QVariantList trace = backend.recentPerformanceTrace();
    QVERIFY(trace.size() >= 6);

    bool hasQueued = false;
    bool hasStarted = false;
    bool hasFinished = false;
    qulonglong prevSequence = 0;
    for (const QVariant &entryValue : trace) {
        const QVariantMap entry = entryValue.toMap();
        const qulonglong sequence = entry.value(QStringLiteral("sequence")).toULongLong();
        QVERIFY(sequence > prevSequence);
        prevSequence = sequence;
        const QString phase = entry.value(QStringLiteral("phase")).toString();
        if (phase == QStringLiteral("queued"))
            hasQueued = true;
        if (phase == QStringLiteral("started"))
            hasStarted = true;
        if (phase == QStringLiteral("finished"))
            hasFinished = true;
    }
    QVERIFY(hasQueued);
    QVERIFY(hasStarted);
    QVERIFY(hasFinished);
    QVERIFY(traceSpy.count() >= 6);
    QVERIFY(metricsSpy.count() >= 1);

    backend.clearPerformanceTrace();
    QCOMPARE(backend.performanceTraceCount(), 0);
}

void BackendIoTests::backend_p1_scheduler_backpressure_cancel_and_cache_capacity_contract()
{
    Backend backend;
    backend.setAsyncMaxConcurrency(3);
    backend.setAsyncQueueDepthLimit(1);
    backend.setReadTextCacheTtlMs(10000);
    backend.setReadTextCacheCapacityBytes(64);
    QCOMPARE(backend.asyncQueueDepthLimit(), 1);
    QCOMPARE(backend.readTextCacheCapacityBytes(), 64);
    QVERIFY(backend.asyncIoMaxConcurrency() >= 1);
    QVERIFY(backend.asyncUtilityMaxConcurrency() >= 1);
    QVERIFY(backend.asyncRenderMaxConcurrency() >= 1);

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

    const qulonglong coalesceLeaderId = backend.dispatchAsyncTask(
        QStringLiteral("utility:coalesce-leader"),
        QVariantMap {
            { QStringLiteral("lane"), QStringLiteral("utility") },
            { QStringLiteral("workMs"), 300 },
            { QStringLiteral("coalesce"), true },
            { QStringLiteral("coalesceKey"), QStringLiteral("group-a") }
        });
    const qulonglong coalesceFollowerId = backend.dispatchAsyncTask(
        QStringLiteral("utility:coalesce-follower"),
        QVariantMap {
            { QStringLiteral("lane"), QStringLiteral("utility") },
            { QStringLiteral("coalesce"), true },
            { QStringLiteral("coalesceKey"), QStringLiteral("group-a") }
        });

    QTRY_VERIFY(hasFinishedRequest(coalesceFollowerId));
    const QList<QVariant> coalesceFollowerArgs = finishedArgsFor(coalesceFollowerId);
    QVERIFY(coalesceFollowerArgs.size() >= 7);
    QVERIFY(coalesceFollowerArgs.at(3).toBool());
    QVERIFY(coalesceFollowerArgs.at(4).toMap().value(QStringLiteral("coalesced")).toBool());
    QCOMPARE(coalesceFollowerArgs.at(4).toMap().value(QStringLiteral("mergedIntoRequestId")).toULongLong(),
             coalesceLeaderId);

    const qulonglong queuedUtilityId = backend.dispatchAsyncTask(
        QStringLiteral("utility:queued"),
        QVariantMap {
            { QStringLiteral("lane"), QStringLiteral("utility") },
            { QStringLiteral("workMs"), 300 },
            { QStringLiteral("coalesce"), false }
        });
    const qulonglong droppedUtilityId = backend.dispatchAsyncTask(
        QStringLiteral("utility:dropped"),
        QVariantMap {
            { QStringLiteral("lane"), QStringLiteral("utility") },
            { QStringLiteral("workMs"), 50 },
            { QStringLiteral("coalesce"), false }
        });

    QTRY_VERIFY(hasFinishedRequest(droppedUtilityId));
    const QList<QVariant> droppedArgs = finishedArgsFor(droppedUtilityId);
    QVERIFY(droppedArgs.size() >= 7);
    QVERIFY(!droppedArgs.at(3).toBool());
    QCOMPARE(droppedArgs.at(5).toString(), QStringLiteral("Backpressure queue limit exceeded"));

    const qulonglong renderTaskId = backend.dispatchAsyncTask(
        QStringLiteral("render:frame-prep"),
        QVariantMap {
            { QStringLiteral("lane"), QStringLiteral("render") },
            { QStringLiteral("workMs"), 50 }
        });
    QTRY_VERIFY(hasFinishedRequest(renderTaskId));
    const QList<QVariant> renderArgs = finishedArgsFor(renderTaskId);
    QVERIFY(renderArgs.size() >= 7);
    QVERIFY(renderArgs.at(3).toBool());
    QCOMPARE(renderArgs.at(4).toMap().value(QStringLiteral("lane")).toString(), QStringLiteral("render"));

    QTRY_VERIFY(hasFinishedRequest(coalesceLeaderId));
    QTRY_VERIFY(hasFinishedRequest(queuedUtilityId));

    const qulonglong cancellableTaskId = backend.dispatchAsyncTask(
        QStringLiteral("utility:cancellable"),
        QVariantMap {
            { QStringLiteral("lane"), QStringLiteral("utility") },
            { QStringLiteral("workMs"), 800 },
            { QStringLiteral("coalesce"), false }
        });
    QVERIFY(backend.cancelAsyncRequest(cancellableTaskId, QStringLiteral("test-cancel")));

    QTRY_VERIFY(hasFinishedRequest(cancellableTaskId));
    const QList<QVariant> canceledArgs = finishedArgsFor(cancellableTaskId);
    QVERIFY(canceledArgs.size() >= 7);
    QVERIFY(!canceledArgs.at(3).toBool());
    QCOMPARE(canceledArgs.at(5).toString(), QStringLiteral("test-cancel"));
    QVERIFY(canceledArgs.at(4).toMap().value(QStringLiteral("canceled")).toBool());

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString fileA = tempDir.path() + "/cache-a.txt";
    const QString fileB = tempDir.path() + "/cache-b.txt";
    const QString textA = QStringLiteral("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    const QString textB = QStringLiteral("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
    QVERIFY(backend.saveTextFile(fileA, textA));
    QVERIFY(backend.saveTextFile(fileB, textB));
    QTest::qWait(5);
    QFile mutateA(fileA);
    QVERIFY(mutateA.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
    QVERIFY(mutateA.write((textA + QStringLiteral("!")).toUtf8()) > 0);
    mutateA.close();
    QFile mutateB(fileB);
    QVERIFY(mutateB.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
    QVERIFY(mutateB.write((textB + QStringLiteral("!")).toUtf8()) > 0);
    mutateB.close();

    const qulonglong readA1 = backend.readTextFileAsync(fileA);
    QTRY_VERIFY(hasFinishedRequest(readA1));
    const QList<QVariant> readA1Args = finishedArgsFor(readA1);
    QVERIFY(readA1Args.size() >= 7);
    QVERIFY(readA1Args.at(3).toBool());
    QCOMPARE(readA1Args.at(4).toMap().value(QStringLiteral("cached")).toBool(), false);

    const qulonglong readB1 = backend.readTextFileAsync(fileB);
    QTRY_VERIFY(hasFinishedRequest(readB1));
    const QList<QVariant> readB1Args = finishedArgsFor(readB1);
    QVERIFY(readB1Args.size() >= 7);
    QVERIFY(readB1Args.at(3).toBool());
    QCOMPARE(readB1Args.at(4).toMap().value(QStringLiteral("cached")).toBool(), false);

    QVERIFY(backend.readTextCacheEntryCount() <= 1);
    QVERIFY(backend.readTextCacheBytes() <= backend.readTextCacheCapacityBytes());

    const qulonglong readA2 = backend.readTextFileAsync(fileA);
    QTRY_VERIFY(hasFinishedRequest(readA2));
    const QList<QVariant> readA2Args = finishedArgsFor(readA2);
    QVERIFY(readA2Args.size() >= 7);
    QVERIFY(readA2Args.at(3).toBool());
    QCOMPARE(readA2Args.at(4).toMap().value(QStringLiteral("cached")).toBool(), false);

    const QVariantMap metrics = backend.performanceMetrics();
    QCOMPARE(metrics.value(QStringLiteral("asyncQueueDepthLimit")).toInt(), 1);
    QVERIFY(metrics.value(QStringLiteral("asyncBackpressureDropCount")).toULongLong() >= 1);
    QVERIFY(metrics.value(QStringLiteral("asyncMergedRequestCount")).toULongLong() >= 1);
    QVERIFY(metrics.value(QStringLiteral("asyncCanceledRequestCount")).toULongLong() >= 1);
    QCOMPARE(metrics.value(QStringLiteral("readTextCacheCapacityBytes")).toLongLong(),
             backend.readTextCacheCapacityBytes());
}

QTEST_MAIN(BackendIoTests)
#include "tst_backend_io.moc"
