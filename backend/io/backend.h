#pragma once

#include <QObject>
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QThreadPool>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

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
    Q_PROPERTY(int readTextCacheTtlMs READ readTextCacheTtlMs WRITE setReadTextCacheTtlMs NOTIFY readTextCacheTtlMsChanged)

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
    int readTextCacheTtlMs() const;
    void setReadTextCacheTtlMs(int value);

signals:
    void lastErrorChanged();
    void userEventHookedChanged();
    void hookedEventsChanged();
    void hookedEventCapacityChanged();
    void asyncJobsInFlightChanged();
    void asyncMaxConcurrencyChanged();
    void readTextCacheTtlMsChanged();
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

private:
    struct ReadCacheEntry {
        QString text;
        qint64 size = -1;
        qint64 lastModifiedMs = -1;
        qint64 cachedAtMs = -1;
    };

    qulonglong beginAsyncRequest(const QString &operation, const QString &subject);
    void finishAsyncRequest(qulonglong requestId,
                            const QString &operation,
                            const QString &subject,
                            bool ok,
                            const QVariantMap &result,
                            const QString &error,
                            qint64 elapsedMs);
    void setAsyncJobsInFlight(int value);
    QString normalizePathForCache(const QString &path) const;
    bool isReadCacheEntryExpired(const ReadCacheEntry &entry, qint64 nowMs) const;
    void pruneExpiredReadTextCache(qint64 nowMs);
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
    QThreadPool m_asyncThreadPool;
    QHash<QString, ReadCacheEntry> m_readTextCache;
    int m_readTextCacheTtlMs = 15000;
    int m_asyncJobsInFlight = 0;
    int m_asyncMaxConcurrency = 0;
    qulonglong m_asyncNextRequestId = 0;
};
