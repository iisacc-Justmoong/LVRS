#include <QtTest>

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QVariantMap>
#include <QtPlugin>

#include "backend/io/backend.h"

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
    Q_OBJECT

private slots:
    void backend_io_dispatch_soak_contract();
};

void SoakRuntimeTests::backend_io_dispatch_soak_contract()
{
    const int iterations = envInt("LVRS_SOAK_ITERATIONS", 300, 64, 200000);
    const int workMs = envInt("LVRS_SOAK_WORK_MS", 1, 0, 20);
    const int waitTimeoutMs = envInt("LVRS_SOAK_TIMEOUT_MS", 120000, 10000, 900000);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString testFilePath = tempDir.filePath(QStringLiteral("soak-runtime.txt"));

    Backend backend;
    backend.setAsyncMaxConcurrency(8);
    backend.setAsyncQueueDepthLimit(8192);
    backend.setReadTextCacheTtlMs(30000);
    backend.setReadTextCacheCapacityBytes(2 * 1024 * 1024);

    QSignalSpy finishedSpy(&backend, &Backend::asyncRequestFinished);
    QVERIFY(finishedSpy.isValid());

    for (int i = 0; i < iterations; ++i) {
        const QString text = QStringLiteral("iteration=%1 payload=%2")
                                 .arg(i)
                                 .arg(QString(i % 101, QLatin1Char('x')));
        QVERIFY2(backend.saveTextFile(testFilePath, text), qPrintable(backend.lastError()));
        const QString loaded = backend.readTextFile(testFilePath);
        QCOMPARE(loaded, text);

        const QVariantMap payload {
            { QStringLiteral("coalesce"), false },
            { QStringLiteral("lane"), QStringLiteral("utility") },
            { QStringLiteral("workMs"), workMs },
            { QStringLiteral("iteration"), i }
        };
        const qulonglong requestId = backend.dispatchAsyncTask(QStringLiteral("soak-runtime-task"), payload, 0);
        QVERIFY(requestId > 0);
    }

    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), iterations, waitTimeoutMs);

    const QVariantMap metrics = backend.performanceMetrics();
    const QVariantMap byOperation = metrics.value(QStringLiteral("asyncLatencyByOperation")).toMap();
    const QVariantMap dispatchSummary = byOperation.value(QStringLiteral("dispatchTask")).toMap();

    const qint64 failureCount = dispatchSummary.value(QStringLiteral("failureCount")).toLongLong();
    const qint64 p99Ms = operationStat(backend, QStringLiteral("dispatchTask"), QStringLiteral("p99Ms"));
    const qint64 p99LimitMs = static_cast<qint64>(envInt("LVRS_SOAK_P99_LIMIT_MS", 500, 10, 5000));

    QVERIFY2(failureCount == 0,
             qPrintable(QStringLiteral("Soak detected failed async tasks: %1").arg(failureCount)));
    QVERIFY2(backend.asyncQueueDepth() == 0,
             qPrintable(QStringLiteral("Queue depth not drained after soak: %1").arg(backend.asyncQueueDepth())));
    QVERIFY2(backend.asyncBackpressureDropCount() == 0,
             qPrintable(QStringLiteral("Backpressure drops detected: %1").arg(backend.asyncBackpressureDropCount())));
    QVERIFY2(backend.readTextCacheBytes() <= backend.readTextCacheCapacityBytes(),
             qPrintable(QStringLiteral("Read cache capacity exceeded: %1 > %2")
                            .arg(backend.readTextCacheBytes())
                            .arg(backend.readTextCacheCapacityBytes())));
    QVERIFY2(backend.performanceTraceCount() <= backend.performanceTraceCapacity(),
             qPrintable(QStringLiteral("Trace capacity exceeded: %1 > %2")
                            .arg(backend.performanceTraceCount())
                            .arg(backend.performanceTraceCapacity())));
    QVERIFY2(p99Ms <= p99LimitMs,
             qPrintable(QStringLiteral("Soak p99 limit exceeded: %1 > %2").arg(p99Ms).arg(p99LimitMs)));

    qInfo().noquote() << "LVRS soak summary iterations=" << iterations
                      << "p99Ms=" << p99Ms
                      << "cacheBytes=" << backend.readTextCacheBytes()
                      << "traceCount=" << backend.performanceTraceCount();
}

QTEST_MAIN(SoakRuntimeTests)
#include "tst_soak_runtime.moc"
