#pragma once

#include <QObject>
#include <QAtomicInt>
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QSet>
#include <QSharedPointer>
#include <QThreadPool>
#include <QList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

#include <array>

class RuntimeEvents;

class Backend : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Backend)
    QML_SINGLETON

    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool userEventHooked READ userEventHooked NOTIFY userEventHookedChanged)
    Q_PROPERTY(int hookedEventCount READ hookedEventCount NOTIFY hookedEventsChanged)
    Q_PROPERTY(int hookedEventCapacity READ hookedEventCapacity WRITE setHookedEventCapacity NOTIFY hookedEventCapacityChanged)
    Q_PROPERTY(QVariantMap lastHookedEvent READ lastHookedEvent NOTIFY hookedEventsChanged)
    Q_PROPERTY(QVariantMap lastHookedInputState READ lastHookedInputState NOTIFY hookedEventsChanged)
    Q_PROPERTY(int asyncJobsInFlight READ asyncJobsInFlight NOTIFY asyncJobsInFlightChanged)
    Q_PROPERTY(int asyncMaxConcurrency READ asyncMaxConcurrency WRITE setAsyncMaxConcurrency NOTIFY asyncMaxConcurrencyChanged)
    Q_PROPERTY(int asyncIoMaxConcurrency READ asyncIoMaxConcurrency NOTIFY asyncPoolLayoutChanged)
    Q_PROPERTY(int asyncUtilityMaxConcurrency READ asyncUtilityMaxConcurrency NOTIFY asyncPoolLayoutChanged)
    Q_PROPERTY(int asyncRenderMaxConcurrency READ asyncRenderMaxConcurrency NOTIFY asyncPoolLayoutChanged)
    Q_PROPERTY(int asyncQueueDepth READ asyncQueueDepth NOTIFY asyncQueueDepthChanged)
    Q_PROPERTY(int asyncQueuePeakDepth READ asyncQueuePeakDepth NOTIFY asyncQueuePeakDepthChanged)
    Q_PROPERTY(int asyncQueueDepthLimit READ asyncQueueDepthLimit WRITE setAsyncQueueDepthLimit NOTIFY asyncQueueDepthLimitChanged)
    Q_PROPERTY(qulonglong asyncBackpressureDropCount READ asyncBackpressureDropCount NOTIFY performanceMetricsChanged)
    Q_PROPERTY(qulonglong asyncMergedRequestCount READ asyncMergedRequestCount NOTIFY performanceMetricsChanged)
    Q_PROPERTY(qulonglong asyncCanceledRequestCount READ asyncCanceledRequestCount NOTIFY performanceMetricsChanged)
    Q_PROPERTY(QVariantMap performanceMetrics READ performanceMetrics NOTIFY performanceMetricsChanged)
    Q_PROPERTY(int performanceTraceCapacity READ performanceTraceCapacity WRITE setPerformanceTraceCapacity NOTIFY performanceTraceCapacityChanged)
    Q_PROPERTY(int performanceTraceCount READ performanceTraceCount NOTIFY performanceTraceChanged)
    Q_PROPERTY(int readTextCacheTtlMs READ readTextCacheTtlMs WRITE setReadTextCacheTtlMs NOTIFY readTextCacheTtlMsChanged)
    Q_PROPERTY(qint64 readTextCacheCapacityBytes READ readTextCacheCapacityBytes WRITE setReadTextCacheCapacityBytes NOTIFY readTextCacheCapacityBytesChanged)
    Q_PROPERTY(qint64 readTextCacheBytes READ readTextCacheBytes NOTIFY readTextCacheBytesChanged)
    Q_PROPERTY(int readTextCacheEntryCount READ readTextCacheEntryCount NOTIFY readTextCacheBytesChanged)

public:
    explicit Backend(QObject *parent = nullptr);

    Q_INVOKABLE bool saveTextFile(const QString &path, const QString &text);
    Q_INVOKABLE QString readTextFile(const QString &path);
    Q_INVOKABLE bool ensureDir(const QString &path);
    Q_INVOKABLE qulonglong saveTextFileAsync(const QString &path, const QString &text);
    Q_INVOKABLE qulonglong readTextFileAsync(const QString &path);
    Q_INVOKABLE qulonglong ensureDirAsync(const QString &path);
    Q_INVOKABLE qulonglong dispatchAsyncTask(const QString &taskName,
                                             const QVariantMap &payload = QVariantMap(),
                                             int delayMs = 0);
    Q_INVOKABLE QString writableLocation(int location) const;
    Q_INVOKABLE bool hookUserEvents();
    Q_INVOKABLE void unhookUserEvents();
    Q_INVOKABLE void clearHookedUserEvents();
    Q_INVOKABLE QVariantList hookedUserEvents(int limit = -1) const;
    Q_INVOKABLE QVariantMap hookedUserEventSummary() const;
    Q_INVOKABLE QVariantMap currentUserInputState() const;
    Q_INVOKABLE QVariantList recentPerformanceTrace(int limit = -1) const;
    Q_INVOKABLE void clearPerformanceTrace();
    Q_INVOKABLE bool cancelAsyncRequest(qulonglong requestId,
                                        const QString &reason = QStringLiteral("Canceled by request"));

    QString lastError() const;
    bool userEventHooked() const;
    int hookedEventCount() const;
    int hookedEventCapacity() const;
    void setHookedEventCapacity(int value);
    QVariantMap lastHookedEvent() const;
    QVariantMap lastHookedInputState() const;
    int asyncJobsInFlight() const;
    int asyncMaxConcurrency() const;
    void setAsyncMaxConcurrency(int value);
    int asyncIoMaxConcurrency() const;
    int asyncUtilityMaxConcurrency() const;
    int asyncRenderMaxConcurrency() const;
    int asyncQueueDepth() const;
    int asyncQueuePeakDepth() const;
    int asyncQueueDepthLimit() const;
    void setAsyncQueueDepthLimit(int value);
    qulonglong asyncBackpressureDropCount() const;
    qulonglong asyncMergedRequestCount() const;
    qulonglong asyncCanceledRequestCount() const;
    QVariantMap performanceMetrics() const;
    int performanceTraceCapacity() const;
    void setPerformanceTraceCapacity(int value);
    int performanceTraceCount() const;
    int readTextCacheTtlMs() const;
    void setReadTextCacheTtlMs(int value);
    qint64 readTextCacheCapacityBytes() const;
    void setReadTextCacheCapacityBytes(qint64 value);
    qint64 readTextCacheBytes() const;
    int readTextCacheEntryCount() const;

signals:
    void lastErrorChanged();
    void userEventHookedChanged();
    void hookedEventsChanged();
    void hookedEventCapacityChanged();
    void asyncJobsInFlightChanged();
    void asyncMaxConcurrencyChanged();
    void asyncPoolLayoutChanged();
    void asyncQueueDepthChanged();
    void asyncQueuePeakDepthChanged();
    void asyncQueueDepthLimitChanged();
    void performanceMetricsChanged();
    void performanceTraceChanged();
    void performanceTraceCapacityChanged();
    void readTextCacheTtlMsChanged();
    void readTextCacheCapacityBytesChanged();
    void readTextCacheBytesChanged();
    void asyncRequestQueued(qulonglong requestId,
                            const QString &operation,
                            const QString &subject);
    void asyncRequestFinished(qulonglong requestId,
                              const QString &operation,
                              const QString &subject,
                              bool ok,
                              const QVariantMap &result,
                              const QString &error,
                              qint64 elapsedMs);
    void asyncRequestCanceled(qulonglong requestId,
                              const QString &operation,
                              const QString &subject,
                              const QString &reason);

private:
    enum class AsyncLane {
        Io = 0,
        Utility = 1,
        Render = 2
    };

    struct ReadCacheEntry {
        QString text;
        qint64 size = -1;
        qint64 lastModifiedMs = -1;
        qint64 cachedAtMs = -1;
        qint64 lastAccessedMs = -1;
        qint64 byteSize = 0;
    };

    qulonglong beginAsyncRequest(const QString &operation,
                                 const QString &subject,
                                 AsyncLane lane);
    void finishAsyncRequest(qulonglong requestId,
                            const QString &operation,
                            const QString &subject,
                            bool ok,
                            const QVariantMap &result,
                            const QString &error,
                            qint64 elapsedMs);
    static const char *laneName(AsyncLane lane);
    static int laneIndex(AsyncLane lane);
    AsyncLane resolveDispatchLane(const QString &taskName, const QVariantMap &payload) const;
    int laneQueueDepth(AsyncLane lane) const;
    int laneRunningCount(AsyncLane lane) const;
    int laneMaxConcurrency(AsyncLane lane) const;
    bool laneQueueSaturated(AsyncLane lane) const;
    void applyAsyncPoolLayout();
    QThreadPool &threadPoolForLane(AsyncLane lane);
    int taskPriorityForLane(AsyncLane lane) const;
    void setAsyncJobsInFlight(int value);
    void updateAsyncQueueDepth();
    void markAsyncRequestStarted(qulonglong requestId,
                                 const QString &operation,
                                 const QString &subject,
                                 qint64 startedEpochMs);
    bool isCancellationRequested(qulonglong requestId) const;
    QString takeCancellationReason(qulonglong requestId);
    void cleanupRequestState(qulonglong requestId);
    void recordAsyncLatencySample(const QString &operation, qint64 totalLatencyMs, bool ok);
    void appendPerformanceTrace(const QString &phase,
                                qulonglong requestId,
                                const QString &operation,
                                const QString &subject,
                                const QVariantMap &detail = QVariantMap());
    QVariantMap buildLatencySummary(const QString &operation) const;
    static qint64 percentileValue(QList<qint64> values, double percentile);
    QString normalizePathForCache(const QString &path) const;
    bool isReadCacheEntryExpired(const ReadCacheEntry &entry, qint64 nowMs) const;
    void pruneExpiredReadTextCache(qint64 nowMs);
    void pruneReadTextCacheByCapacity();
    void eraseReadTextCacheEntry(const QString &key);
    bool tryReadTextCache(const QString &path, QString *text);
    void updateReadTextCache(const QString &path, const QString &text);
    void invalidateReadTextCache(const QString &path);
    RuntimeEvents *resolveRuntimeEvents() const;
    void appendHookedEvent(const QVariantMap &eventData);
    void setLastError(const QString &message);

    QString m_lastError;
    bool m_userEventHooked = false;
    int m_hookedEventCapacity = 2048;
    QVariantList m_hookedEvents;
    QVariantMap m_lastHookedEvent;
    QVariantMap m_lastHookedInputState;
    QHash<QString, int> m_hookedTypeCounts;
    QPointer<RuntimeEvents> m_runtimeEvents;
    QMetaObject::Connection m_runtimeEventConnection;
    QMetaObject::Connection m_runtimeDestroyedConnection;
    QThreadPool m_ioThreadPool;
    QThreadPool m_utilityThreadPool;
    QThreadPool m_renderThreadPool;
    QHash<QString, ReadCacheEntry> m_readTextCache;
    qint64 m_readTextCacheCapacityBytes = 8 * 1024 * 1024;
    qint64 m_readTextCacheBytes = 0;
    QHash<qulonglong, qint64> m_asyncQueuedEpochMs;
    QHash<qulonglong, qint64> m_asyncStartedEpochMs;
    QHash<qulonglong, AsyncLane> m_asyncLaneByRequest;
    QHash<qulonglong, QString> m_asyncOperationByRequest;
    QHash<qulonglong, QString> m_asyncSubjectByRequest;
    QHash<qulonglong, QSharedPointer<QAtomicInt>> m_asyncCancelTokenByRequest;
    QHash<qulonglong, QString> m_asyncCancelReasonByRequest;
    QSet<qulonglong> m_asyncActiveRequestIds;
    QHash<QString, qulonglong> m_dispatchCoalescedLeaderByKey;
    QHash<qulonglong, QString> m_dispatchCoalesceKeyByRequest;
    QHash<QString, QList<qint64>> m_asyncLatencySamplesByOperation;
    QHash<QString, quint64> m_asyncOperationCountByOperation;
    QHash<QString, quint64> m_asyncOperationFailureCountByOperation;
    QVariantList m_performanceTrace;
    int m_performanceTraceCapacity = 2048;
    int m_asyncQueueDepth = 0;
    int m_asyncQueuePeakDepth = 0;
    int m_asyncQueueDepthLimit = 256;
    std::array<int, 3> m_asyncQueuedByLane { 0, 0, 0 };
    std::array<int, 3> m_asyncRunningByLane { 0, 0, 0 };
    std::array<int, 3> m_asyncQueuePeakByLane { 0, 0, 0 };
    std::array<int, 3> m_asyncMaxConcurrencyByLane { 1, 1, 1 };
    qulonglong m_asyncBackpressureDropCount = 0;
    qulonglong m_asyncMergedRequestCount = 0;
    qulonglong m_asyncCanceledRequestCount = 0;
    quint64 m_asyncTraceSequence = 0;
    int m_asyncLatencySampleCapacity = 256;
    int m_readTextCacheTtlMs = 15000;
    int m_asyncJobsInFlight = 0;
    int m_asyncMaxConcurrency = 0;
    qulonglong m_asyncNextRequestId = 0;
};
