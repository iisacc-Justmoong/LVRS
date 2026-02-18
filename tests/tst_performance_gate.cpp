#include <QtTest>

#include <QSignalSpy>
#include <QVariantMap>
#include <QtPlugin>

#include <algorithm>

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

qint64 medianValue(QVector<qint64> values)
{
    if (values.isEmpty())
        return 0;

    std::sort(values.begin(), values.end());
    const int mid = values.size() / 2;
    if ((values.size() % 2) == 0)
        return (values.at(mid - 1) + values.at(mid)) / 2;
    return values.at(mid);
}

qint64 operationPercentileMs(const Backend &backend, const QString &operationKey, const QString &percentileKey)
{
    const QVariantMap metrics = backend.performanceMetrics();
    const QVariantMap byOperation = metrics.value(QStringLiteral("asyncLatencyByOperation")).toMap();
    const QVariantMap summary = byOperation.value(operationKey).toMap();
    return summary.value(percentileKey).toLongLong();
}

} // namespace

class PerformanceGateTests : public QObject
{
    Q_OBJECT

private slots:
    void backend_dispatch_latency_regression_gate();
};

void PerformanceGateTests::backend_dispatch_latency_regression_gate()
{
    const int rounds = envInt("LVRS_PERF_GATE_ROUNDS", 5, 3, 15);
    const int tasksPerRound = envInt("LVRS_PERF_GATE_TASKS", 48, 16, 512);
    const int workMs = envInt("LVRS_PERF_GATE_WORK_MS", 2, 1, 20);
    const int waitTimeoutMs = envInt("LVRS_PERF_GATE_TIMEOUT_MS", 15000, 5000, 120000);
    const qint64 p95LimitMs = static_cast<qint64>(envInt("LVRS_PERF_GATE_P95_MS", 120, 1, 2000));
    const qint64 p99LimitMs = static_cast<qint64>(envInt("LVRS_PERF_GATE_P99_MS", 180, 1, 3000));

    QVector<qint64> p95Series;
    QVector<qint64> p99Series;
    p95Series.reserve(rounds);
    p99Series.reserve(rounds);

    for (int round = 0; round < rounds; ++round) {
        Backend backend;
        backend.setAsyncMaxConcurrency(12);
        backend.setAsyncQueueDepthLimit(4096);

        QSignalSpy finishedSpy(&backend, &Backend::asyncRequestFinished);
        QVERIFY(finishedSpy.isValid());

        for (int i = 0; i < tasksPerRound; ++i) {
            const QVariantMap payload {
                { QStringLiteral("coalesce"), false },
                { QStringLiteral("lane"), QStringLiteral("utility") },
                { QStringLiteral("workMs"), workMs },
                { QStringLiteral("round"), round },
                { QStringLiteral("index"), i }
            };
            const qulonglong requestId = backend.dispatchAsyncTask(QStringLiteral("perf-gate-task"), payload, 0);
            QVERIFY(requestId > 0);
        }

        QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), tasksPerRound, waitTimeoutMs);

        const qint64 p95Ms = operationPercentileMs(backend,
                                                   QStringLiteral("dispatchTask"),
                                                   QStringLiteral("p95Ms"));
        const qint64 p99Ms = operationPercentileMs(backend,
                                                   QStringLiteral("dispatchTask"),
                                                   QStringLiteral("p99Ms"));
        p95Series.append(p95Ms);
        p99Series.append(p99Ms);
    }

    const qint64 medianP95Ms = medianValue(p95Series);
    const qint64 medianP99Ms = medianValue(p99Series);

    qInfo().noquote() << "LVRS perf gate median p95=" << medianP95Ms
                      << "median p99=" << medianP99Ms
                      << "limits=" << p95LimitMs << "/" << p99LimitMs
                      << "samples=" << rounds;

    QVERIFY2(medianP95Ms <= p95LimitMs,
             qPrintable(QStringLiteral("p95 gate failed: %1 > %2").arg(medianP95Ms).arg(p95LimitMs)));
    QVERIFY2(medianP99Ms <= p99LimitMs,
             qPrintable(QStringLiteral("p99 gate failed: %1 > %2").arg(medianP99Ms).arg(p99LimitMs)));
}

QTEST_MAIN(PerformanceGateTests)
#include "tst_performance_gate.moc"
