#include "backend/io/backend.h"

#include "backend/runtime/runtimeevents.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRunnable>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThread>

#include <functional>

namespace {

constexpr int kAsyncThreadExpiryTimeoutMs = 5000;
constexpr int kIoTaskPriority = 1;
constexpr int kUtilityTaskPriority = 0;
constexpr int kReadTextCacheMinTtlMs = 100;
constexpr int kReadTextCacheMaxTtlMs = 60 * 60 * 1000;

struct SaveTextOutcome {
    bool ok = false;
    QString error;
};

struct ReadTextOutcome {
    bool ok = false;
    QString text;
    QString error;
};

struct EnsureDirOutcome {
    bool ok = false;
    QString error;
};

struct FileState {
    bool ok = false;
    qint64 size = -1;
    qint64 lastModifiedMs = -1;
};

SaveTextOutcome saveTextFileSync(const QString &path, const QString &text)
{
    SaveTextOutcome outcome;
    if (path.trimmed().isEmpty()) {
        outcome.error = QStringLiteral("Empty path");
        return outcome;
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        outcome.error = file.errorString();
        return outcome;
    }

    const QByteArray data = text.toUtf8();
    if (file.write(data) != data.size()) {
        outcome.error = file.errorString();
        file.cancelWriting();
        return outcome;
    }

    if (!file.commit()) {
        outcome.error = file.errorString();
        return outcome;
    }

    outcome.ok = true;
    return outcome;
}

ReadTextOutcome readTextFileSync(const QString &path)
{
    ReadTextOutcome outcome;
    if (path.trimmed().isEmpty()) {
        outcome.error = QStringLiteral("Empty path");
        return outcome;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        outcome.error = file.errorString();
        return outcome;
    }

    outcome.text = QString::fromUtf8(file.readAll());
    outcome.ok = true;
    return outcome;
}

EnsureDirOutcome ensureDirSync(const QString &path)
{
    EnsureDirOutcome outcome;
    if (path.trimmed().isEmpty()) {
        outcome.error = QStringLiteral("Empty path");
        return outcome;
    }

    QDir dir(path);
    if (dir.exists()) {
        outcome.ok = true;
        return outcome;
    }

    if (!dir.mkpath(QStringLiteral("."))) {
        outcome.error = QStringLiteral("Failed to create directory");
        return outcome;
    }

    outcome.ok = true;
    return outcome;
}

FileState inspectFileState(const QString &path)
{
    FileState state;
    QFileInfo info(path);
    if (!info.exists() || !info.isFile())
        return state;

    state.ok = true;
    state.size = info.size();
    state.lastModifiedMs = info.lastModified().toMSecsSinceEpoch();
    return state;
}

class LambdaRunnable final : public QRunnable
{
public:
    explicit LambdaRunnable(std::function<void()> fn)
        : m_fn(std::move(fn))
    {
        setAutoDelete(true);
    }

    void run() override
    {
        if (m_fn)
            m_fn();
    }

private:
    std::function<void()> m_fn;
};

} // namespace

Backend::Backend(QObject *parent)
    : QObject(parent)
{
    m_asyncMaxConcurrency = qMax(1, QThread::idealThreadCount());
    m_asyncThreadPool.setMaxThreadCount(m_asyncMaxConcurrency);
    m_asyncThreadPool.setExpiryTimeout(kAsyncThreadExpiryTimeoutMs);
}

bool Backend::saveTextFile(const QString &path, const QString &text)
{
    setLastError(QString());
    const SaveTextOutcome outcome = saveTextFileSync(path, text);
    if (!outcome.ok) {
        invalidateReadTextCache(path);
        setLastError(outcome.error);
    } else {
        updateReadTextCache(path, text);
    }
    return outcome.ok;
}

QString Backend::readTextFile(const QString &path)
{
    setLastError(QString());
    QString cachedText;
    if (tryReadTextCache(path, &cachedText))
        return cachedText;

    const ReadTextOutcome outcome = readTextFileSync(path);
    if (!outcome.ok) {
        invalidateReadTextCache(path);
        setLastError(outcome.error);
        return QString();
    }
    updateReadTextCache(path, outcome.text);
    return outcome.text;
}

bool Backend::ensureDir(const QString &path)
{
    setLastError(QString());
    const EnsureDirOutcome outcome = ensureDirSync(path);
    if (!outcome.ok)
        setLastError(outcome.error);
    return outcome.ok;
}

qulonglong Backend::saveTextFileAsync(const QString &path, const QString &text)
{
    setLastError(QString());
    const qulonglong requestId = beginAsyncRequest(QStringLiteral("saveTextFile"), path);
    const QString pathCopy = path;
    const QString textCopy = text;
    const qint64 byteCount = text.toUtf8().size();

    if (path.trimmed().isEmpty()) {
        finishAsyncRequest(requestId,
                           QStringLiteral("saveTextFile"),
                           pathCopy,
                           false,
                           QVariantMap(),
                           QStringLiteral("Empty path"),
                           0);
        return requestId;
    }

    QPointer<QCoreApplication> appGuard(QCoreApplication::instance());
    QPointer<Backend> backendGuard(this);
    m_asyncThreadPool.start(new LambdaRunnable(
        [appGuard, backendGuard, requestId, pathCopy, textCopy, byteCount]() {
            const qint64 startedMs = QDateTime::currentMSecsSinceEpoch();
            const SaveTextOutcome outcome = saveTextFileSync(pathCopy, textCopy);
            const qint64 elapsedMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startedMs);

            QVariantMap result;
            result.insert(QStringLiteral("bytes"), byteCount);

            if (!appGuard)
                return;
            QMetaObject::invokeMethod(appGuard.data(),
                                      [backendGuard, requestId, pathCopy, textCopy, outcome, result, elapsedMs]() {
                                          if (!backendGuard)
                                              return;
                                          if (outcome.ok)
                                              backendGuard->updateReadTextCache(pathCopy, textCopy);
                                          else
                                              backendGuard->invalidateReadTextCache(pathCopy);
                                          backendGuard->finishAsyncRequest(requestId,
                                                                           QStringLiteral("saveTextFile"),
                                                                           pathCopy,
                                                                           outcome.ok,
                                                                           result,
                                                                           outcome.error,
                                                                           elapsedMs);
                                      },
                                      Qt::QueuedConnection);
        }),
                          kIoTaskPriority);
    return requestId;
}

qulonglong Backend::readTextFileAsync(const QString &path)
{
    setLastError(QString());
    const qulonglong requestId = beginAsyncRequest(QStringLiteral("readTextFile"), path);
    const QString pathCopy = path;

    if (path.trimmed().isEmpty()) {
        finishAsyncRequest(requestId,
                           QStringLiteral("readTextFile"),
                           pathCopy,
                           false,
                           QVariantMap(),
                           QStringLiteral("Empty path"),
                           0);
        return requestId;
    }

    QString cachedText;
    if (tryReadTextCache(pathCopy, &cachedText)) {
        QVariantMap result;
        result.insert(QStringLiteral("text"), cachedText);
        result.insert(QStringLiteral("length"), cachedText.length());
        result.insert(QStringLiteral("cached"), true);
        finishAsyncRequest(requestId,
                           QStringLiteral("readTextFile"),
                           pathCopy,
                           true,
                           result,
                           QString(),
                           0);
        return requestId;
    }

    QPointer<QCoreApplication> appGuard(QCoreApplication::instance());
    QPointer<Backend> backendGuard(this);
    m_asyncThreadPool.start(new LambdaRunnable(
        [appGuard, backendGuard, requestId, pathCopy]() {
            const qint64 startedMs = QDateTime::currentMSecsSinceEpoch();
            const ReadTextOutcome outcome = readTextFileSync(pathCopy);
            const qint64 elapsedMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startedMs);

            QVariantMap result;
            if (outcome.ok) {
                result.insert(QStringLiteral("text"), outcome.text);
                result.insert(QStringLiteral("length"), outcome.text.length());
                result.insert(QStringLiteral("cached"), false);
            }

            if (!appGuard)
                return;
            QMetaObject::invokeMethod(appGuard.data(),
                                      [backendGuard, requestId, pathCopy, outcome, result, elapsedMs]() {
                                          if (!backendGuard)
                                              return;
                                          if (outcome.ok)
                                              backendGuard->updateReadTextCache(pathCopy, outcome.text);
                                          else
                                              backendGuard->invalidateReadTextCache(pathCopy);
                                          backendGuard->finishAsyncRequest(requestId,
                                                                           QStringLiteral("readTextFile"),
                                                                           pathCopy,
                                                                           outcome.ok,
                                                                           result,
                                                                           outcome.error,
                                                                           elapsedMs);
                                      },
                                      Qt::QueuedConnection);
        }),
                          kIoTaskPriority);
    return requestId;
}

qulonglong Backend::ensureDirAsync(const QString &path)
{
    setLastError(QString());
    const qulonglong requestId = beginAsyncRequest(QStringLiteral("ensureDir"), path);
    const QString pathCopy = path;

    if (path.trimmed().isEmpty()) {
        finishAsyncRequest(requestId,
                           QStringLiteral("ensureDir"),
                           pathCopy,
                           false,
                           QVariantMap(),
                           QStringLiteral("Empty path"),
                           0);
        return requestId;
    }

    QPointer<QCoreApplication> appGuard(QCoreApplication::instance());
    QPointer<Backend> backendGuard(this);
    m_asyncThreadPool.start(new LambdaRunnable(
        [appGuard, backendGuard, requestId, pathCopy]() {
            const qint64 startedMs = QDateTime::currentMSecsSinceEpoch();
            const EnsureDirOutcome outcome = ensureDirSync(pathCopy);
            const qint64 elapsedMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startedMs);

            QVariantMap result;
            result.insert(QStringLiteral("ensured"), outcome.ok);

            if (!appGuard)
                return;
            QMetaObject::invokeMethod(appGuard.data(),
                                      [backendGuard, requestId, pathCopy, outcome, result, elapsedMs]() {
                                          if (!backendGuard)
                                              return;
                                          backendGuard->finishAsyncRequest(requestId,
                                                                           QStringLiteral("ensureDir"),
                                                                           pathCopy,
                                                                           outcome.ok,
                                                                           result,
                                                                           outcome.error,
                                                                           elapsedMs);
                                      },
                                      Qt::QueuedConnection);
        }),
                          kIoTaskPriority);
    return requestId;
}

qulonglong Backend::dispatchAsyncTask(const QString &taskName, const QVariantMap &payload, int delayMs)
{
    setLastError(QString());
    const QString normalizedTaskName = taskName.trimmed().isEmpty()
        ? QStringLiteral("task")
        : taskName.trimmed();
    const QString operation = QStringLiteral("dispatchTask");
    const qulonglong requestId = beginAsyncRequest(operation, normalizedTaskName);
    const QVariantMap payloadCopy = payload;
    const int requestedDelayMs = qBound(0, delayMs, 60 * 60 * 1000);

    QPointer<QCoreApplication> appGuard(QCoreApplication::instance());
    QPointer<Backend> backendGuard(this);
    m_asyncThreadPool.start(new LambdaRunnable(
        [appGuard, backendGuard, requestId, normalizedTaskName, payloadCopy, requestedDelayMs, operation]() {
            const qint64 startedMs = QDateTime::currentMSecsSinceEpoch();
            QVariantMap result = payloadCopy;
            result.insert(QStringLiteral("taskName"), normalizedTaskName);
            result.insert(QStringLiteral("delayMs"), 0);
            result.insert(QStringLiteral("requestedDelayMs"), requestedDelayMs);

            const qint64 elapsedMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startedMs);
            if (!appGuard)
                return;

            QMetaObject::invokeMethod(appGuard.data(),
                                      [backendGuard, requestId, operation, normalizedTaskName, result, elapsedMs]() {
                                          if (!backendGuard)
                                              return;
                                          backendGuard->finishAsyncRequest(requestId,
                                                                           operation,
                                                                           normalizedTaskName,
                                                                           true,
                                                                           result,
                                                                           QString(),
                                                                           elapsedMs);
                                      },
                                      Qt::QueuedConnection);
        }),
                          kUtilityTaskPriority);
    return requestId;
}

QString Backend::writableLocation(int location) const
{
    return QStandardPaths::writableLocation(
        static_cast<QStandardPaths::StandardLocation>(location));
}

bool Backend::hookUserEvents()
{
    setLastError(QString());

    RuntimeEvents *runtime = resolveRuntimeEvents();
    if (!runtime) {
        setLastError(QStringLiteral("RuntimeEvents singleton unavailable"));
        return false;
    }

    runtime->start();

    if (m_userEventHooked && m_runtimeEvents == runtime)
        return true;

    unhookUserEvents();

    m_runtimeEvents = runtime;
    m_runtimeEventConnection = connect(runtime,
                                       &RuntimeEvents::eventRecorded,
                                       this,
                                       [this](const QVariantMap &eventData) {
                                           appendHookedEvent(eventData);
                                       });
    m_runtimeDestroyedConnection = connect(runtime,
                                           &QObject::destroyed,
                                           this,
                                           [this]() {
                                               m_runtimeEvents.clear();
                                               if (m_userEventHooked) {
                                                   m_userEventHooked = false;
                                                   emit userEventHookedChanged();
                                               }
                                           });

    const QVariantList cachedEvents = runtime->recentEvents();
    for (const QVariant &entry : cachedEvents)
        appendHookedEvent(entry.toMap());

    m_userEventHooked = true;
    emit userEventHookedChanged();
    return true;
}

void Backend::unhookUserEvents()
{
    if (m_runtimeEventConnection)
        QObject::disconnect(m_runtimeEventConnection);
    if (m_runtimeDestroyedConnection)
        QObject::disconnect(m_runtimeDestroyedConnection);

    m_runtimeEventConnection = QMetaObject::Connection();
    m_runtimeDestroyedConnection = QMetaObject::Connection();
    m_runtimeEvents.clear();

    if (m_userEventHooked) {
        m_userEventHooked = false;
        emit userEventHookedChanged();
    }
}

void Backend::clearHookedUserEvents()
{
    if (m_hookedEvents.isEmpty() && m_lastHookedEvent.isEmpty() && m_lastHookedInputState.isEmpty())
        return;

    m_hookedEvents.clear();
    m_lastHookedEvent.clear();
    m_lastHookedInputState.clear();
    m_hookedTypeCounts.clear();
    emit hookedEventsChanged();
}

QVariantList Backend::hookedUserEvents(int limit) const
{
    if (limit <= 0 || limit >= m_hookedEvents.size())
        return m_hookedEvents;

    QVariantList subset;
    const int start = m_hookedEvents.size() - limit;
    subset.reserve(limit);
    for (int i = start; i < m_hookedEvents.size(); ++i)
        subset.append(m_hookedEvents.at(i));
    return subset;
}

QVariantMap Backend::hookedUserEventSummary() const
{
    QVariantMap summary;
    summary.insert(QStringLiteral("hooked"), m_userEventHooked);
    summary.insert(QStringLiteral("eventCount"), m_hookedEvents.size());
    summary.insert(QStringLiteral("capacity"), m_hookedEventCapacity);
    summary.insert(QStringLiteral("lastEvent"), m_lastHookedEvent);
    summary.insert(QStringLiteral("input"), currentUserInputState());

    QVariantMap typeCounts;
    for (auto it = m_hookedTypeCounts.constBegin(); it != m_hookedTypeCounts.constEnd(); ++it)
        typeCounts.insert(it.key(), it.value());
    summary.insert(QStringLiteral("typeCounts"), typeCounts);

    if (m_runtimeEvents)
        summary.insert(QStringLiteral("runtimeEventSequence"), QVariant::fromValue(m_runtimeEvents->eventSequence()));

    return summary;
}

QVariantMap Backend::currentUserInputState() const
{
    if (m_runtimeEvents)
        return m_runtimeEvents->inputState();
    return m_lastHookedInputState;
}

QString Backend::lastError() const
{
    return m_lastError;
}

bool Backend::userEventHooked() const
{
    return m_userEventHooked;
}

int Backend::hookedEventCount() const
{
    return m_hookedEvents.size();
}

int Backend::hookedEventCapacity() const
{
    return m_hookedEventCapacity;
}

void Backend::setHookedEventCapacity(int value)
{
    const int next = qBound(64, value, 32768);
    if (m_hookedEventCapacity == next)
        return;

    m_hookedEventCapacity = next;
    emit hookedEventCapacityChanged();

    bool dropped = false;
    while (m_hookedEvents.size() > m_hookedEventCapacity) {
        const QVariantMap droppedEvent = m_hookedEvents.takeFirst().toMap();
        const QString droppedType = droppedEvent.value(QStringLiteral("type")).toString();
        if (!droppedType.isEmpty()) {
            const int current = m_hookedTypeCounts.value(droppedType, 0);
            if (current <= 1)
                m_hookedTypeCounts.remove(droppedType);
            else
                m_hookedTypeCounts.insert(droppedType, current - 1);
        }
        dropped = true;
    }

    if (dropped)
        emit hookedEventsChanged();
}

QVariantMap Backend::lastHookedEvent() const
{
    return m_lastHookedEvent;
}

QVariantMap Backend::lastHookedInputState() const
{
    return m_lastHookedInputState;
}

int Backend::asyncJobsInFlight() const
{
    return m_asyncJobsInFlight;
}

int Backend::asyncMaxConcurrency() const
{
    return m_asyncMaxConcurrency;
}

void Backend::setAsyncMaxConcurrency(int value)
{
    const int next = qBound(1, value, 64);
    if (m_asyncMaxConcurrency == next)
        return;

    m_asyncMaxConcurrency = next;
    m_asyncThreadPool.setMaxThreadCount(m_asyncMaxConcurrency);
    emit asyncMaxConcurrencyChanged();
}

int Backend::readTextCacheTtlMs() const
{
    return m_readTextCacheTtlMs;
}

void Backend::setReadTextCacheTtlMs(int value)
{
    const int next = qBound(kReadTextCacheMinTtlMs, value, kReadTextCacheMaxTtlMs);
    if (m_readTextCacheTtlMs == next)
        return;

    m_readTextCacheTtlMs = next;
    pruneExpiredReadTextCache(QDateTime::currentMSecsSinceEpoch());
    emit readTextCacheTtlMsChanged();
}

qulonglong Backend::beginAsyncRequest(const QString &operation, const QString &subject)
{
    const qulonglong requestId = ++m_asyncNextRequestId;
    setAsyncJobsInFlight(m_asyncJobsInFlight + 1);
    emit asyncRequestQueued(requestId, operation, subject);
    return requestId;
}

void Backend::finishAsyncRequest(qulonglong requestId,
                                 const QString &operation,
                                 const QString &subject,
                                 bool ok,
                                 const QVariantMap &result,
                                 const QString &error,
                                 qint64 elapsedMs)
{
    if (!ok)
        setLastError(error);
    setAsyncJobsInFlight(qMax(0, m_asyncJobsInFlight - 1));
    emit asyncRequestFinished(requestId,
                              operation,
                              subject,
                              ok,
                              result,
                              error,
                              qMax<qint64>(0, elapsedMs));
}

void Backend::setAsyncJobsInFlight(int value)
{
    const int next = qMax(0, value);
    if (m_asyncJobsInFlight == next)
        return;
    m_asyncJobsInFlight = next;
    emit asyncJobsInFlightChanged();
}

QString Backend::normalizePathForCache(const QString &path) const
{
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty())
        return QString();
    return QDir::cleanPath(trimmed);
}

bool Backend::isReadCacheEntryExpired(const ReadCacheEntry &entry, qint64 nowMs) const
{
    if (entry.cachedAtMs < 0)
        return true;
    if (m_readTextCacheTtlMs <= 0)
        return true;
    return (nowMs - entry.cachedAtMs) >= m_readTextCacheTtlMs;
}

void Backend::pruneExpiredReadTextCache(qint64 nowMs)
{
    for (auto it = m_readTextCache.begin(); it != m_readTextCache.end(); ) {
        if (isReadCacheEntryExpired(it.value(), nowMs))
            it = m_readTextCache.erase(it);
        else
            ++it;
    }
}

bool Backend::tryReadTextCache(const QString &path, QString *text)
{
    const QString key = normalizePathForCache(path);
    if (key.isEmpty())
        return false;

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    pruneExpiredReadTextCache(nowMs);

    auto it = m_readTextCache.constFind(key);
    if (it == m_readTextCache.constEnd())
        return false;

    if (isReadCacheEntryExpired(it.value(), nowMs)) {
        m_readTextCache.remove(key);
        return false;
    }

    const FileState state = inspectFileState(key);
    if (!state.ok
        || state.size != it.value().size
        || state.lastModifiedMs != it.value().lastModifiedMs) {
        m_readTextCache.remove(key);
        return false;
    }

    if (text)
        *text = it.value().text;
    return true;
}

void Backend::updateReadTextCache(const QString &path, const QString &text)
{
    const QString key = normalizePathForCache(path);
    if (key.isEmpty())
        return;

    const FileState state = inspectFileState(key);
    if (!state.ok) {
        invalidateReadTextCache(key);
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    pruneExpiredReadTextCache(nowMs);

    ReadCacheEntry entry;
    entry.text = text;
    entry.size = state.size;
    entry.lastModifiedMs = state.lastModifiedMs;
    entry.cachedAtMs = nowMs;

    m_readTextCache.insert(key, entry);
}

void Backend::invalidateReadTextCache(const QString &path)
{
    const QString key = normalizePathForCache(path);
    if (key.isEmpty())
        return;
    m_readTextCache.remove(key);
}

RuntimeEvents *Backend::resolveRuntimeEvents() const
{
    if (m_runtimeEvents)
        return m_runtimeEvents.data();

    if (RuntimeEvents::instance())
        return RuntimeEvents::instance();

    if (!qApp)
        return nullptr;

    const QList<RuntimeEvents *> runtimes = qApp->findChildren<RuntimeEvents *>();
    if (runtimes.isEmpty())
        return nullptr;
    return runtimes.constLast();
}

void Backend::appendHookedEvent(const QVariantMap &eventData)
{
    if (eventData.isEmpty())
        return;

    QVariantMap hookedEvent = eventData;
    hookedEvent.insert(QStringLiteral("hookEpochMs"), QVariant::fromValue(QDateTime::currentMSecsSinceEpoch()));

    const QString eventType = hookedEvent.value(QStringLiteral("type")).toString();
    if (!eventType.isEmpty())
        m_hookedTypeCounts.insert(eventType, m_hookedTypeCounts.value(eventType, 0) + 1);

    const QVariantMap payload = hookedEvent.value(QStringLiteral("payload")).toMap();
    const QVariantMap payloadInput = payload.value(QStringLiteral("input")).toMap();
    if (!payloadInput.isEmpty())
        m_lastHookedInputState = payloadInput;
    else if (m_runtimeEvents)
        m_lastHookedInputState = m_runtimeEvents->inputState();

    m_lastHookedEvent = hookedEvent;
    m_hookedEvents.append(hookedEvent);

    while (m_hookedEvents.size() > m_hookedEventCapacity) {
        const QVariantMap droppedEvent = m_hookedEvents.takeFirst().toMap();
        const QString droppedType = droppedEvent.value(QStringLiteral("type")).toString();
        if (!droppedType.isEmpty()) {
            const int current = m_hookedTypeCounts.value(droppedType, 0);
            if (current <= 1)
                m_hookedTypeCounts.remove(droppedType);
            else
                m_hookedTypeCounts.insert(droppedType, current - 1);
        }
    }

    emit hookedEventsChanged();
}

void Backend::setLastError(const QString &message)
{
    if (m_lastError == message)
        return;
    m_lastError = message;
    emit lastErrorChanged();
}
