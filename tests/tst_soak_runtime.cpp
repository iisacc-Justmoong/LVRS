#include "QtEventWait.h"
#include <QGuiApplication>

#include <QDir>
#include <QTemporaryDir>
#include <QVariantMap>
#include <QtPlugin>

#include "backend/io/backend.h"
#include "millisecond_timer.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

namespace {

int envInt(const char *name, int fallback, int minimum, int maximum)
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    if (!ok)
        return qBound(minimum, fallback, maximum);
    return qBound(minimum, value, maximum);
}

qint64 operationStat(const Backend &backend, const QString &operation, const QString &key)
{
    const QVariantMap metrics = backend.performanceMetrics();
    const QVariantMap byOperation = metrics.value(QStringLiteral("asyncLatencyByOperation")).toMap();
    const QVariantMap summary = byOperation.value(operation).toMap();
    return summary.value(key).toLongLong();
}

} // namespace

class SoakRuntimeTests : public QObject
{
public:
    void backend_io_dispatch_soak_contract();
};

void SoakRuntimeTests::backend_io_dispatch_soak_contract()
{
    MillisecondTimerResolution timerResolution;
    CTEST_CHECK(timerResolution.available());
    const int iterations = envInt("LVRS_SOAK_ITERATIONS", 300, 64, 200000);
    const int workMs = envInt("LVRS_SOAK_WORK_MS", 1, 0, 20);
    const int waitTimeoutMs = envInt("LVRS_SOAK_TIMEOUT_MS", 120000, 10000, 900000);

    QTemporaryDir tempDir(QDir(QStringLiteral(LVRS_TEST_SOURCE_DIR)).filePath(QStringLiteral("../resources/ctest-soak-XXXXXX")));
    CTEST_CHECK(tempDir.isValid());
    const QString testFilePath = tempDir.filePath(QStringLiteral("soak-runtime.txt"));

    Backend backend;
    backend.setAsyncMaxConcurrency(8);
    backend.setAsyncQueueDepthLimit(8192);
    backend.setReadTextCacheTtlMs(30000);
    backend.setReadTextCacheCapacityBytes(2 * 1024 * 1024);

    QObject observer;
    int finishedCount = 0;
    const auto finishedConnection = QObject::connect(&backend, &Backend::asyncRequestFinished, &observer, [&] { ++finishedCount; });
    CTEST_CHECK(finishedConnection);

    for (int i = 0; i < iterations; ++i) {
        const QString text = QStringLiteral("iteration=%1 payload=%2")
                                 .arg(i)
                                 .arg(QString(i % 101, QLatin1Char('x')));
        CTEST_CHECK_MESSAGE(backend.saveTextFile(testFilePath, text), qPrintable(backend.lastError()));
        const QString loaded = backend.readTextFile(testFilePath);
        CTEST_COMPARE(loaded, text);

        const QVariantMap payload {
            { QStringLiteral("coalesce"), false },
            { QStringLiteral("lane"), QStringLiteral("utility") },
            { QStringLiteral("workMs"), workMs },
            { QStringLiteral("iteration"), i }
        };
        const qulonglong requestId = backend.dispatchAsyncTask(QStringLiteral("soak-runtime-task"), payload, 0);
        CTEST_CHECK(requestId > 0);
        // Keep servicing worker completions while the fixture performs 300
        // synchronous filesystem writes, as the application event loop does.
        QCoreApplication::processEvents();
    }

    CTEST_WAIT_COMPARE_TIMEOUT(finishedCount, iterations, waitTimeoutMs);

    const QVariantMap metrics = backend.performanceMetrics();
    const QVariantMap byOperation = metrics.value(QStringLiteral("asyncLatencyByOperation")).toMap();
    const QVariantMap dispatchSummary = byOperation.value(QStringLiteral("dispatchTask")).toMap();

    const qint64 failureCount = dispatchSummary.value(QStringLiteral("failureCount")).toLongLong();
    const qint64 p99Ms = operationStat(backend, QStringLiteral("dispatchTask"), QStringLiteral("p99Ms"));
    const qint64 p99LimitMs = static_cast<qint64>(envInt("LVRS_SOAK_P99_LIMIT_MS", 500, 10, 5000));

    CTEST_CHECK_MESSAGE(failureCount == 0,
             qPrintable(QStringLiteral("Soak detected failed async tasks: %1").arg(failureCount)));
    CTEST_CHECK_MESSAGE(backend.asyncQueueDepth() == 0,
             qPrintable(QStringLiteral("Queue depth not drained after soak: %1").arg(backend.asyncQueueDepth())));
    CTEST_CHECK_MESSAGE(backend.asyncBackpressureDropCount() == 0,
             qPrintable(QStringLiteral("Backpressure drops detected: %1").arg(backend.asyncBackpressureDropCount())));
    CTEST_CHECK_MESSAGE(backend.readTextCacheBytes() <= backend.readTextCacheCapacityBytes(),
             qPrintable(QStringLiteral("Read cache capacity exceeded: %1 > %2")
                            .arg(backend.readTextCacheBytes())
                            .arg(backend.readTextCacheCapacityBytes())));
    CTEST_CHECK_MESSAGE(backend.performanceTraceCount() <= backend.performanceTraceCapacity(),
             qPrintable(QStringLiteral("Trace capacity exceeded: %1 > %2")
                            .arg(backend.performanceTraceCount())
                            .arg(backend.performanceTraceCapacity())));
    CTEST_CHECK_MESSAGE(p99Ms <= p99LimitMs,
             qPrintable(QStringLiteral("Soak p99 limit exceeded: %1 > %2").arg(p99Ms).arg(p99LimitMs)));

    qInfo().noquote() << "LVRS soak summary iterations=" << iterations
                      << "p99Ms=" << p99Ms
                      << "cacheBytes=" << backend.readTextCacheBytes()
                      << "traceCount=" << backend.performanceTraceCount();
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    SoakRuntimeTests tests;
    runner.run("backend_io_dispatch_soak_contract", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.backend_io_dispatch_soak_contract(); });
    return runner.result();
}
