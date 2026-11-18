#include "QtEventWait.h"
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QGuiApplication>
#include <QTimer>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtPlugin>

#include "backend/io/backend.h"
#include "backend/runtime/runtimeevents.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class BackendObservations final
{
public:
    QList<QList<QVariant>> finished;
    int errorCount = 0;
    int hookedCount = 0;
    int queuedCount = 0;
    int inFlightCount = 0;
    int traceCount = 0;
    int metricsCount = 0;

    explicit BackendObservations(Backend &backend)
    {
        const auto errorConnection = QObject::connect(&backend, &Backend::lastErrorChanged,
                                                       &context_, [this] { ++errorCount; });
        const auto hookedConnection = QObject::connect(&backend, &Backend::hookedEventsChanged,
                                                        &context_, [this] { ++hookedCount; });
        const auto queuedConnection = QObject::connect(&backend, &Backend::asyncRequestQueued,
                                                        &context_, [this] { ++queuedCount; });
        const auto inFlightConnection = QObject::connect(&backend, &Backend::asyncJobsInFlightChanged,
                                                          &context_, [this] { ++inFlightCount; });
        const auto traceConnection = QObject::connect(&backend, &Backend::performanceTraceChanged,
                                                       &context_, [this] { ++traceCount; });
        const auto metricsConnection = QObject::connect(&backend, &Backend::performanceMetricsChanged,
                                                         &context_, [this] { ++metricsCount; });
        const auto finishedConnection = QObject::connect(&backend, &Backend::asyncRequestFinished,
            &context_, [this](qulonglong requestId, const QString &operation, const QString &subject,
                              bool ok, const QVariantMap &result, const QString &error, qint64 elapsedMs) {
                finished.append({QVariant::fromValue(requestId), operation, subject, ok, result,
                                 error, QVariant::fromValue(elapsedMs)});
            });
        CTEST_CHECK(errorConnection);
        CTEST_CHECK(hookedConnection);
        CTEST_CHECK(queuedConnection);
        CTEST_CHECK(inFlightConnection);
        CTEST_CHECK(traceConnection);
        CTEST_CHECK(metricsConnection);
        CTEST_CHECK(finishedConnection);
    }
private:
    QObject context_;
};

static void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(milliseconds);
    loop.exec();
}

class BackendIoTests
{
public:
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
    QTemporaryDir tempDir(QDir::cleanPath(
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources/ctest-backend-XXXXXX")));
    CTEST_CHECK(tempDir.isValid());
    qInfo().noquote() << "backend I/O workspace:" << tempDir.path();
    Backend backend;

    const QString nestedDir = tempDir.path() + "/io/a/b";
    CTEST_CHECK(backend.ensureDir(nestedDir));

    const QString filePath = nestedDir + "/sample.txt";
    CTEST_CHECK(backend.saveTextFile(filePath, QStringLiteral("metrics-check")));
    CTEST_COMPARE(backend.readTextFile(filePath), QStringLiteral("metrics-check"));

    CTEST_CHECK(!backend.saveTextFile(QString(), QStringLiteral("x")));
    CTEST_CHECK(!backend.lastError().isEmpty());

    const QString missingPath = nestedDir + "/missing.txt";
    CTEST_COMPARE(backend.readTextFile(missingPath), QString());
    CTEST_CHECK(!backend.lastError().isEmpty());

    const QString tempLocation = backend.writableLocation(static_cast<int>(QStandardPaths::TempLocation));
    CTEST_CHECK(!tempLocation.isEmpty());
}

void BackendIoTests::backend_writable_location_contract_respects_platform_defaults()
{
    Backend backend;

    const QString tempLocation = backend.writableLocation(static_cast<int>(QStandardPaths::TempLocation));
    CTEST_CHECK(!tempLocation.isEmpty());
    CTEST_COMPARE(tempLocation, QStandardPaths::writableLocation(QStandardPaths::TempLocation));

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    const QList<QStandardPaths::StandardLocation> mobileFallbackLocations = {
        QStandardPaths::DesktopLocation,
        QStandardPaths::ApplicationsLocation,
        QStandardPaths::DownloadLocation
    };

    for (QStandardPaths::StandardLocation location : mobileFallbackLocations)
        CTEST_CHECK(!backend.writableLocation(static_cast<int>(location)).isEmpty());
#endif
}

void BackendIoTests::backend_error_signal_and_directory_idempotence()
{
    QTemporaryDir tempDir(QDir::cleanPath(
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources/ctest-backend-XXXXXX")));
    CTEST_CHECK(tempDir.isValid());
    qInfo().noquote() << "backend I/O workspace:" << tempDir.path();
    Backend backend;
    BackendObservations observations(backend);

    CTEST_CHECK(!backend.ensureDir(QStringLiteral("   ")));
    CTEST_COMPARE(backend.lastError(), QStringLiteral("Empty path"));

    const QString targetDir = tempDir.path() + "/x/y/z";
    CTEST_CHECK(backend.ensureDir(targetDir));
    CTEST_COMPARE(backend.lastError(), QString());
    CTEST_CHECK(backend.ensureDir(targetDir));
    CTEST_COMPARE(backend.lastError(), QString());

    CTEST_CHECK(!backend.readTextFile(QStringLiteral(" ")).size());
    CTEST_COMPARE(backend.lastError(), QStringLiteral("Empty path"));

    const QString filePath = targetDir + "/coverage.txt";
    CTEST_CHECK(backend.saveTextFile(filePath, QStringLiteral("ok")));
    CTEST_COMPARE(backend.lastError(), QString());
    CTEST_COMPARE(backend.readTextFile(filePath), QStringLiteral("ok"));
    CTEST_COMPARE(backend.lastError(), QString());

    CTEST_CHECK(observations.errorCount >= 4);
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
    CTEST_CHECK(backend.hookUserEvents());
    CTEST_CHECK(backend.userEventHooked());

    backend.clearHookedUserEvents();
    CTEST_COMPARE(backend.hookedEventCount(), 0);

    BackendObservations observations(backend);

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

    CTEST_WAIT_DEFAULT(backend.hookedEventCount() >= 4);
    CTEST_CHECK(observations.hookedCount >= 1);

    const QVariantMap last = backend.lastHookedEvent();
    CTEST_CHECK(!last.isEmpty());
    CTEST_CHECK(!last.value(QStringLiteral("type")).toString().isEmpty());
    CTEST_CHECK(last.contains(QStringLiteral("payload")));

    const QVariantMap summary = backend.hookedUserEventSummary();
    CTEST_CHECK(summary.value(QStringLiteral("hooked")).toBool());
    CTEST_CHECK(summary.value(QStringLiteral("eventCount")).toInt() >= 4);
    const QVariantMap typeCounts = summary.value(QStringLiteral("typeCounts")).toMap();
    CTEST_CHECK(!typeCounts.isEmpty());

    const QVariantMap input = backend.currentUserInputState();
    CTEST_CHECK(input.contains(QStringLiteral("anyKeyPressed")));
    CTEST_CHECK(input.contains(QStringLiteral("pointerUi")));
    const QVariantMap pointerUi = input.value(QStringLiteral("pointerUi")).toMap();
    CTEST_CHECK(pointerUi.contains(QStringLiteral("objectName")));

    backend.unhookUserEvents();
    CTEST_CHECK(!backend.userEventHooked());
}

void BackendIoTests::backend_async_concurrency_and_io_contract()
{
    QTemporaryDir tempDir(QDir::cleanPath(
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources/ctest-backend-XXXXXX")));
    CTEST_CHECK(tempDir.isValid());
    qInfo().noquote() << "backend I/O workspace:" << tempDir.path();
    Backend backend;
    backend.setAsyncMaxConcurrency(2);
    CTEST_COMPARE(backend.asyncMaxConcurrency(), 2);

    BackendObservations observations(backend);

    auto hasFinishedRequest = [&observations](qulonglong requestId) {
        for (const QList<QVariant> &args : observations.finished) {
            if (args.size() >= 7 && args.at(0).toULongLong() == requestId)
                return true;
        }
        return false;
    };

    auto finishedArgsFor = [&observations](qulonglong requestId) {
        for (const QList<QVariant> &args : observations.finished) {
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
    CTEST_WAIT_DEFAULT(backend.asyncJobsInFlight() > 0);
    CTEST_WAIT_DEFAULT(hasFinishedRequest(taskA));
    CTEST_WAIT_DEFAULT(hasFinishedRequest(taskB));

    const qint64 elapsedMs = timer.elapsed();
    CTEST_CHECK_MESSAGE(elapsedMs < 430,
             qPrintable(QStringLiteral("Expected parallel execution (<430ms), elapsed=%1ms").arg(elapsedMs)));

    const QList<QVariant> taskAArgs = finishedArgsFor(taskA);
    const QList<QVariant> taskBArgs = finishedArgsFor(taskB);
    CTEST_CHECK(taskAArgs.size() >= 7);
    CTEST_CHECK(taskBArgs.size() >= 7);
    CTEST_COMPARE(taskAArgs.at(1).toString(), QStringLiteral("dispatchTask"));
    CTEST_COMPARE(taskBArgs.at(1).toString(), QStringLiteral("dispatchTask"));
    CTEST_CHECK(taskAArgs.at(3).toBool());
    CTEST_CHECK(taskBArgs.at(3).toBool());
    CTEST_COMPARE(taskAArgs.at(4).toMap().value(QStringLiteral("key")).toString(), QStringLiteral("a"));
    CTEST_COMPARE(taskBArgs.at(4).toMap().value(QStringLiteral("key")).toString(), QStringLiteral("b"));

    const QString nestedDir = tempDir.path() + "/async/contract";
    const QString filePath = nestedDir + "/sample.txt";

    const qulonglong ensureId = backend.ensureDirAsync(nestedDir);
    CTEST_WAIT_DEFAULT(hasFinishedRequest(ensureId));
    const QList<QVariant> ensureArgs = finishedArgsFor(ensureId);
    CTEST_CHECK(ensureArgs.size() >= 7);
    CTEST_COMPARE(ensureArgs.at(1).toString(), QStringLiteral("ensureDir"));
    CTEST_CHECK(ensureArgs.at(3).toBool());
    CTEST_CHECK(ensureArgs.at(4).toMap().value(QStringLiteral("ensured")).toBool());

    const qulonglong saveId = backend.saveTextFileAsync(filePath, QStringLiteral("async-metrics"));
    CTEST_WAIT_DEFAULT(hasFinishedRequest(saveId));
    const QList<QVariant> saveArgs = finishedArgsFor(saveId);
    CTEST_CHECK(saveArgs.size() >= 7);
    CTEST_COMPARE(saveArgs.at(1).toString(), QStringLiteral("saveTextFile"));
    CTEST_CHECK(saveArgs.at(3).toBool());
    CTEST_CHECK(saveArgs.at(4).toMap().value(QStringLiteral("bytes")).toLongLong() > 0);

    const qulonglong readId = backend.readTextFileAsync(filePath);
    CTEST_WAIT_DEFAULT(hasFinishedRequest(readId));
    const QList<QVariant> readArgs = finishedArgsFor(readId);
    CTEST_CHECK(readArgs.size() >= 7);
    CTEST_COMPARE(readArgs.at(1).toString(), QStringLiteral("readTextFile"));
    CTEST_CHECK(readArgs.at(3).toBool());
    CTEST_COMPARE(readArgs.at(4).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("async-metrics"));
    CTEST_COMPARE(readArgs.at(4).toMap().value(QStringLiteral("length")).toInt(),
             QStringLiteral("async-metrics").length());

    const qulonglong badReadId = backend.readTextFileAsync(QStringLiteral(" "));
    CTEST_WAIT_DEFAULT(hasFinishedRequest(badReadId));
    const QList<QVariant> badReadArgs = finishedArgsFor(badReadId);
    CTEST_CHECK(badReadArgs.size() >= 7);
    CTEST_COMPARE(badReadArgs.at(1).toString(), QStringLiteral("readTextFile"));
    CTEST_CHECK(!badReadArgs.at(3).toBool());
    CTEST_COMPARE(badReadArgs.at(5).toString(), QStringLiteral("Empty path"));
    CTEST_COMPARE(backend.lastError(), QStringLiteral("Empty path"));

    CTEST_WAIT_COMPARE(backend.asyncJobsInFlight(), 0);
    CTEST_CHECK(observations.queuedCount >= 6);
    CTEST_CHECK(observations.finished.count() >= 6);
}

void BackendIoTests::backend_async_delay_distribution_and_read_cache_contract()
{
    QTemporaryDir tempDir(QDir::cleanPath(
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources/ctest-backend-XXXXXX")));
    CTEST_CHECK(tempDir.isValid());
    qInfo().noquote() << "backend I/O workspace:" << tempDir.path();
    Backend backend;
    backend.setAsyncMaxConcurrency(1);
    CTEST_COMPARE(backend.asyncMaxConcurrency(), 1);
    backend.setReadTextCacheTtlMs(150);
    CTEST_COMPARE(backend.readTextCacheTtlMs(), 150);

    BackendObservations observations(backend);

    auto hasFinishedRequest = [&observations](qulonglong requestId) {
        for (const QList<QVariant> &args : observations.finished) {
            if (args.size() >= 7 && args.at(0).toULongLong() == requestId)
                return true;
        }
        return false;
    };

    auto finishedArgsFor = [&observations](qulonglong requestId) {
        for (const QList<QVariant> &args : observations.finished) {
            if (args.size() >= 7 && args.at(0).toULongLong() == requestId)
                return args;
        }
        return QList<QVariant>();
    };


    const QString filePath = tempDir.path() + "/distribution.txt";
    QFile seedFile(filePath);
    CTEST_CHECK(seedFile.open(QIODevice::WriteOnly | QIODevice::Text));
    CTEST_CHECK(seedFile.write("delay-distribution-check") > 0);
    seedFile.close();

    QElapsedTimer timer;
    timer.start();
    const qulonglong delayedId = backend.dispatchAsyncTask(QStringLiteral("delayed-task"),
                                                           QVariantMap(),
                                                           320);
    const qulonglong readId = backend.readTextFileAsync(filePath);

    CTEST_WAIT_DEFAULT(hasFinishedRequest(readId));
    const qint64 firstReadElapsedMs = timer.elapsed();
    CTEST_CHECK_MESSAGE(firstReadElapsedMs < 260,
             qPrintable(QStringLiteral("Expected read to bypass delayed-task queue blocking (<260ms), elapsed=%1ms")
                            .arg(firstReadElapsedMs)));

    const QList<QVariant> firstReadArgs = finishedArgsFor(readId);
    CTEST_CHECK(firstReadArgs.size() >= 7);
    CTEST_COMPARE(firstReadArgs.at(1).toString(), QStringLiteral("readTextFile"));
    CTEST_CHECK(firstReadArgs.at(3).toBool());
    CTEST_COMPARE(firstReadArgs.at(4).toMap().value(QStringLiteral("text")).toString(),
             QStringLiteral("delay-distribution-check"));
    CTEST_COMPARE(firstReadArgs.at(4).toMap().value(QStringLiteral("cached")).toBool(), false);

    CTEST_WAIT_DEFAULT(hasFinishedRequest(delayedId));
    const QList<QVariant> delayedArgs = finishedArgsFor(delayedId);
    CTEST_CHECK(delayedArgs.size() >= 7);
    CTEST_COMPARE(delayedArgs.at(1).toString(), QStringLiteral("dispatchTask"));
    CTEST_CHECK(delayedArgs.at(3).toBool());
    CTEST_CHECK(delayedArgs.at(6).toLongLong() < 220);
    CTEST_COMPARE(delayedArgs.at(4).toMap().value(QStringLiteral("delayMs")).toInt(), 0);
    CTEST_COMPARE(delayedArgs.at(4).toMap().value(QStringLiteral("requestedDelayMs")).toInt(), 320);

    QElapsedTimer cacheTimer;
    cacheTimer.start();
    const qulonglong cachedReadId = backend.readTextFileAsync(filePath);
    CTEST_WAIT_DEFAULT(hasFinishedRequest(cachedReadId));
    const qint64 cachedReadElapsedMs = cacheTimer.elapsed();
    CTEST_CHECK_MESSAGE(cachedReadElapsedMs < 120,
             qPrintable(QStringLiteral("Expected cache hit to complete quickly (<120ms), elapsed=%1ms")
                            .arg(cachedReadElapsedMs)));

    const QList<QVariant> cachedReadArgs = finishedArgsFor(cachedReadId);
    CTEST_CHECK(cachedReadArgs.size() >= 7);
    CTEST_COMPARE(cachedReadArgs.at(1).toString(), QStringLiteral("readTextFile"));
    CTEST_CHECK(cachedReadArgs.at(3).toBool());
    CTEST_COMPARE(cachedReadArgs.at(4).toMap().value(QStringLiteral("cached")).toBool(), true);

    processFor(220);
    const qulonglong expiredReadId = backend.readTextFileAsync(filePath);
    CTEST_WAIT_DEFAULT(hasFinishedRequest(expiredReadId));
    const QList<QVariant> expiredReadArgs = finishedArgsFor(expiredReadId);
    CTEST_CHECK(expiredReadArgs.size() >= 7);
    CTEST_COMPARE(expiredReadArgs.at(1).toString(), QStringLiteral("readTextFile"));
    CTEST_CHECK(expiredReadArgs.at(3).toBool());
    CTEST_COMPARE(expiredReadArgs.at(4).toMap().value(QStringLiteral("cached")).toBool(), false);

    CTEST_WAIT_COMPARE(backend.asyncJobsInFlight(), 0);
}

void BackendIoTests::backend_performance_metrics_and_trace_contract()
{
    Backend backend;
    backend.setAsyncMaxConcurrency(1);
    backend.setPerformanceTraceCapacity(512);
    CTEST_COMPARE(backend.performanceTraceCapacity(), 512);

    BackendObservations observations(backend);

    auto hasFinishedRequest = [&observations](qulonglong requestId) {
        for (const QList<QVariant> &args : observations.finished) {
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
    CTEST_WAIT_DEFAULT(hasFinishedRequest(taskA));
    CTEST_WAIT_DEFAULT(hasFinishedRequest(taskB));
    CTEST_WAIT_COMPARE(backend.asyncJobsInFlight(), 0);

    const QVariantMap metrics = backend.performanceMetrics();
    CTEST_COMPARE(metrics.value(QStringLiteral("schema")).toString(), QStringLiteral("lvrs.performance.v1"));
    CTEST_COMPARE(metrics.value(QStringLiteral("component")).toString(), QStringLiteral("Backend"));
    CTEST_COMPARE(metrics.value(QStringLiteral("asyncMaxConcurrency")).toInt(), 1);
    CTEST_CHECK(metrics.contains(QStringLiteral("asyncQueueDepth")));
    CTEST_CHECK(metrics.contains(QStringLiteral("asyncQueuePeakDepth")));
    CTEST_CHECK(metrics.contains(QStringLiteral("performanceTraceCount")));

    const QVariantMap byOperation = metrics.value(QStringLiteral("asyncLatencyByOperation")).toMap();
    CTEST_CHECK(byOperation.contains(QStringLiteral("dispatchTask")));
    const QVariantMap dispatchLatency = byOperation.value(QStringLiteral("dispatchTask")).toMap();
    CTEST_CHECK(dispatchLatency.value(QStringLiteral("count")).toULongLong() >= 2);
    CTEST_CHECK(dispatchLatency.contains(QStringLiteral("avgMs")));
    CTEST_CHECK(dispatchLatency.contains(QStringLiteral("p95Ms")));
    CTEST_CHECK(dispatchLatency.contains(QStringLiteral("p99Ms")));

    const QVariantList trace = backend.recentPerformanceTrace();
    CTEST_CHECK(trace.size() >= 6);

    bool hasQueued = false;
    bool hasStarted = false;
    bool hasFinished = false;
    qulonglong prevSequence = 0;
    for (const QVariant &entryValue : trace) {
        const QVariantMap entry = entryValue.toMap();
        const qulonglong sequence = entry.value(QStringLiteral("sequence")).toULongLong();
        CTEST_CHECK(sequence > prevSequence);
        prevSequence = sequence;
        const QString phase = entry.value(QStringLiteral("phase")).toString();
        if (phase == QStringLiteral("queued"))
            hasQueued = true;
        if (phase == QStringLiteral("started"))
            hasStarted = true;
        if (phase == QStringLiteral("finished"))
            hasFinished = true;
    }
    CTEST_CHECK(hasQueued);
    CTEST_CHECK(hasStarted);
    CTEST_CHECK(hasFinished);
    CTEST_CHECK(observations.traceCount >= 6);
    CTEST_CHECK(observations.metricsCount >= 1);

    backend.clearPerformanceTrace();
    CTEST_COMPARE(backend.performanceTraceCount(), 0);
}

void BackendIoTests::backend_p1_scheduler_backpressure_cancel_and_cache_capacity_contract()
{
    QTemporaryDir tempDir(QDir::cleanPath(
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources/ctest-backend-XXXXXX")));
    CTEST_CHECK(tempDir.isValid());
    qInfo().noquote() << "backend I/O workspace:" << tempDir.path();
    Backend backend;
    backend.setAsyncMaxConcurrency(3);
    backend.setAsyncQueueDepthLimit(1);
    backend.setReadTextCacheTtlMs(10000);
    backend.setReadTextCacheCapacityBytes(64);
    CTEST_COMPARE(backend.asyncQueueDepthLimit(), 1);
    CTEST_COMPARE(backend.readTextCacheCapacityBytes(), 64);
    CTEST_CHECK(backend.asyncIoMaxConcurrency() >= 1);
    CTEST_CHECK(backend.asyncUtilityMaxConcurrency() >= 1);
    CTEST_CHECK(backend.asyncRenderMaxConcurrency() >= 1);

    BackendObservations observations(backend);

    auto hasFinishedRequest = [&observations](qulonglong requestId) {
        for (const QList<QVariant> &args : observations.finished) {
            if (args.size() >= 7 && args.at(0).toULongLong() == requestId)
                return true;
        }
        return false;
    };

    auto finishedArgsFor = [&observations](qulonglong requestId) {
        for (const QList<QVariant> &args : observations.finished) {
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

    CTEST_WAIT_DEFAULT(hasFinishedRequest(coalesceFollowerId));
    const QList<QVariant> coalesceFollowerArgs = finishedArgsFor(coalesceFollowerId);
    CTEST_CHECK(coalesceFollowerArgs.size() >= 7);
    CTEST_CHECK(coalesceFollowerArgs.at(3).toBool());
    CTEST_CHECK(coalesceFollowerArgs.at(4).toMap().value(QStringLiteral("coalesced")).toBool());
    CTEST_COMPARE(coalesceFollowerArgs.at(4).toMap().value(QStringLiteral("mergedIntoRequestId")).toULongLong(),
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

    CTEST_WAIT_DEFAULT(hasFinishedRequest(droppedUtilityId));
    const QList<QVariant> droppedArgs = finishedArgsFor(droppedUtilityId);
    CTEST_CHECK(droppedArgs.size() >= 7);
    CTEST_CHECK(!droppedArgs.at(3).toBool());
    CTEST_COMPARE(droppedArgs.at(5).toString(), QStringLiteral("Backpressure queue limit exceeded"));

    const qulonglong renderTaskId = backend.dispatchAsyncTask(
        QStringLiteral("render:frame-prep"),
        QVariantMap {
            { QStringLiteral("lane"), QStringLiteral("render") },
            { QStringLiteral("workMs"), 50 }
        });
    CTEST_WAIT_DEFAULT(hasFinishedRequest(renderTaskId));
    const QList<QVariant> renderArgs = finishedArgsFor(renderTaskId);
    CTEST_CHECK(renderArgs.size() >= 7);
    CTEST_CHECK(renderArgs.at(3).toBool());
    CTEST_COMPARE(renderArgs.at(4).toMap().value(QStringLiteral("lane")).toString(), QStringLiteral("render"));

    CTEST_WAIT_DEFAULT(hasFinishedRequest(coalesceLeaderId));
    CTEST_WAIT_DEFAULT(hasFinishedRequest(queuedUtilityId));

    const qulonglong cancellableTaskId = backend.dispatchAsyncTask(
        QStringLiteral("utility:cancellable"),
        QVariantMap {
            { QStringLiteral("lane"), QStringLiteral("utility") },
            { QStringLiteral("workMs"), 800 },
            { QStringLiteral("coalesce"), false }
        });
    CTEST_CHECK(backend.cancelAsyncRequest(cancellableTaskId, QStringLiteral("test-cancel")));

    CTEST_WAIT_DEFAULT(hasFinishedRequest(cancellableTaskId));
    const QList<QVariant> canceledArgs = finishedArgsFor(cancellableTaskId);
    CTEST_CHECK(canceledArgs.size() >= 7);
    CTEST_CHECK(!canceledArgs.at(3).toBool());
    CTEST_COMPARE(canceledArgs.at(5).toString(), QStringLiteral("test-cancel"));
    CTEST_CHECK(canceledArgs.at(4).toMap().value(QStringLiteral("canceled")).toBool());

    const QString fileA = tempDir.path() + "/cache-a.txt";
    const QString fileB = tempDir.path() + "/cache-b.txt";
    const QString textA = QStringLiteral("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    const QString textB = QStringLiteral("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBB");
    CTEST_CHECK(backend.saveTextFile(fileA, textA));
    CTEST_CHECK(backend.saveTextFile(fileB, textB));
    processFor(5);
    QFile mutateA(fileA);
    CTEST_CHECK(mutateA.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
    CTEST_CHECK(mutateA.write((textA + QStringLiteral("!")).toUtf8()) > 0);
    mutateA.close();
    QFile mutateB(fileB);
    CTEST_CHECK(mutateB.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate));
    CTEST_CHECK(mutateB.write((textB + QStringLiteral("!")).toUtf8()) > 0);
    mutateB.close();

    const qulonglong readA1 = backend.readTextFileAsync(fileA);
    CTEST_WAIT_DEFAULT(hasFinishedRequest(readA1));
    const QList<QVariant> readA1Args = finishedArgsFor(readA1);
    CTEST_CHECK(readA1Args.size() >= 7);
    CTEST_CHECK(readA1Args.at(3).toBool());
    CTEST_COMPARE(readA1Args.at(4).toMap().value(QStringLiteral("cached")).toBool(), false);

    const qulonglong readB1 = backend.readTextFileAsync(fileB);
    CTEST_WAIT_DEFAULT(hasFinishedRequest(readB1));
    const QList<QVariant> readB1Args = finishedArgsFor(readB1);
    CTEST_CHECK(readB1Args.size() >= 7);
    CTEST_CHECK(readB1Args.at(3).toBool());
    CTEST_COMPARE(readB1Args.at(4).toMap().value(QStringLiteral("cached")).toBool(), false);

    CTEST_CHECK(backend.readTextCacheEntryCount() <= 1);
    CTEST_CHECK(backend.readTextCacheBytes() <= backend.readTextCacheCapacityBytes());

    const qulonglong readA2 = backend.readTextFileAsync(fileA);
    CTEST_WAIT_DEFAULT(hasFinishedRequest(readA2));
    const QList<QVariant> readA2Args = finishedArgsFor(readA2);
    CTEST_CHECK(readA2Args.size() >= 7);
    CTEST_CHECK(readA2Args.at(3).toBool());
    CTEST_COMPARE(readA2Args.at(4).toMap().value(QStringLiteral("cached")).toBool(), false);

    const QVariantMap metrics = backend.performanceMetrics();
    CTEST_COMPARE(metrics.value(QStringLiteral("asyncQueueDepthLimit")).toInt(), 1);
    CTEST_CHECK(metrics.value(QStringLiteral("asyncBackpressureDropCount")).toULongLong() >= 1);
    CTEST_CHECK(metrics.value(QStringLiteral("asyncMergedRequestCount")).toULongLong() >= 1);
    CTEST_CHECK(metrics.value(QStringLiteral("asyncCanceledRequestCount")).toULongLong() >= 1);
    CTEST_COMPARE(metrics.value(QStringLiteral("readTextCacheCapacityBytes")).toLongLong(),
             backend.readTextCacheCapacityBytes());
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    BackendIoTests tests;
    runner.run("backend_file_roundtrip_and_errors", [&] { tests.backend_file_roundtrip_and_errors(); });
    runner.run("backend_writable_location_contract_respects_platform_defaults", [&] { tests.backend_writable_location_contract_respects_platform_defaults(); });
    runner.run("backend_error_signal_and_directory_idempotence", [&] { tests.backend_error_signal_and_directory_idempotence(); });
    runner.run("backend_event_hook_receives_runtime_events", [&] { tests.backend_event_hook_receives_runtime_events(); });
    runner.run("backend_async_concurrency_and_io_contract", [&] { tests.backend_async_concurrency_and_io_contract(); });
    runner.run("backend_async_delay_distribution_and_read_cache_contract", [&] { tests.backend_async_delay_distribution_and_read_cache_contract(); });
    runner.run("backend_performance_metrics_and_trace_contract", [&] { tests.backend_performance_metrics_and_trace_contract(); });
    runner.run("backend_p1_scheduler_backpressure_cancel_and_cache_capacity_contract", [&] { tests.backend_p1_scheduler_backpressure_cancel_and_cache_capacity_contract(); });
    return runner.result();
}
