#include "backend/io/backend.h"

#include "backend/runtime/runtimeevents.h"

#include <algorithm>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRunnable>
#include <QSaveFile>
#include <QStandardPaths>
#include <QThread>

#include <cmath>
#include <functional>
#include <limits>
#include <numeric>

namespace {

constexpr int kAsyncThreadExpiryTimeoutMs = 5000;
constexpr int kIoTaskPriority = 1;
constexpr int kUtilityTaskPriority = 0;
constexpr int kRenderTaskPriority = 2;
constexpr int kAsyncQueueDepthLimitMin = 1;
constexpr int kAsyncQueueDepthLimitMax = 16384;
constexpr int kReadTextCacheMinTtlMs = 100;
constexpr int kReadTextCacheMaxTtlMs = 60 * 60 * 1000;
constexpr qint64 kReadTextCacheCapacityMinBytes = 64;
constexpr qint64 kReadTextCacheCapacityMaxBytes = 512LL * 1024LL * 1024LL;
constexpr qint64 kReadTextLargePayloadThresholdBytes = 4 * 1024;
constexpr int kIoBufferPoolSlots = 6;
constexpr qsizetype kIoBufferPoolMaxReusableBytes = 2 * 1024 * 1024;

struct ThreadLocalIoBufferPool {
    std::array<QByteArray, kIoBufferPoolSlots> buffers;
    int cursor = 0;
};

thread_local ThreadLocalIoBufferPool g_threadLocalIoBufferPool;

QByteArray acquireThreadLocalIoBuffer(qsizetype minCapacity)
{
    ThreadLocalIoBufferPool &pool = g_threadLocalIoBufferPool;
    for (int i = 0; i < kIoBufferPoolSlots; ++i) {
        QByteArray &slotBuffer = pool.buffers.at(i);
        if (slotBuffer.capacity() >= minCapacity && !slotBuffer.isNull()) {
            QByteArray reusable;
            reusable.swap(slotBuffer);
            reusable.clear();
            return reusable;
        }
    }

    QByteArray fresh;
    if (minCapacity > 0)
        fresh.reserve(minCapacity);
    return fresh;
}

void releaseThreadLocalIoBuffer(QByteArray &&buffer)
{
    if (buffer.capacity() <= 0 || buffer.capacity() > kIoBufferPoolMaxReusableBytes)
        return;

    buffer.clear();
    ThreadLocalIoBufferPool &pool = g_threadLocalIoBufferPool;
    pool.buffers[pool.cursor] = std::move(buffer);
    pool.cursor = (pool.cursor + 1) % kIoBufferPoolSlots;
}

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

    const qint64 estimatedBytes = static_cast<qint64>(text.size()) * 3;
    QByteArray data = estimatedBytes >= kReadTextLargePayloadThresholdBytes
        ? acquireThreadLocalIoBuffer(static_cast<qsizetype>(estimatedBytes))
        : QByteArray();
    data = text.toUtf8();

    if (file.write(data) != data.size()) {
        outcome.error = file.errorString();
        file.cancelWriting();
        releaseThreadLocalIoBuffer(std::move(data));
        return outcome;
    }

    if (!file.commit()) {
        outcome.error = file.errorString();
        releaseThreadLocalIoBuffer(std::move(data));
        return outcome;
    }

    releaseThreadLocalIoBuffer(std::move(data));
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

    const qint64 size = file.size();
    if (size > std::numeric_limits<int>::max()) {
        outcome.error = QStringLiteral("File too large");
        return outcome;
    }

    if (size >= kReadTextLargePayloadThresholdBytes) {
        QByteArray buffer = acquireThreadLocalIoBuffer(static_cast<qsizetype>(size));
        buffer.resize(static_cast<qsizetype>(size));

        qint64 totalRead = 0;
        while (totalRead < size) {
            const qint64 readNow = file.read(buffer.data() + totalRead, size - totalRead);
            if (readNow <= 0)
                break;
            totalRead += readNow;
        }

        if (totalRead < 0) {
            outcome.error = file.errorString();
            releaseThreadLocalIoBuffer(std::move(buffer));
            return outcome;
        }

        buffer.truncate(static_cast<int>(qMax<qint64>(0, totalRead)));
        outcome.text = QString::fromUtf8(buffer.constData(), buffer.size());
        releaseThreadLocalIoBuffer(std::move(buffer));
    } else {
        outcome.text = QString::fromUtf8(file.readAll());
    }

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
    m_ioThreadPool.setExpiryTimeout(kAsyncThreadExpiryTimeoutMs);
    m_utilityThreadPool.setExpiryTimeout(kAsyncThreadExpiryTimeoutMs);
    m_renderThreadPool.setExpiryTimeout(kAsyncThreadExpiryTimeoutMs);
    applyAsyncPoolLayout();
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
    const QString operation = QStringLiteral("saveTextFile");
    const AsyncLane lane = AsyncLane::Io;
    const qulonglong requestId = beginAsyncRequest(operation, path, lane);
    const QString pathCopy = path;
    const QString textCopy = text;
    const qint64 byteCount = text.toUtf8().size();
    const QSharedPointer<QAtomicInt> cancelToken = m_asyncCancelTokenByRequest.value(requestId);

    if (path.trimmed().isEmpty()) {
        finishAsyncRequest(requestId,
                           operation,
                           pathCopy,
                           false,
                           QVariantMap(),
                           QStringLiteral("Empty path"),
                           0);
        return requestId;
    }

    if (laneQueueSaturated(lane)) {
        m_asyncBackpressureDropCount += 1;
        finishAsyncRequest(requestId,
                           operation,
                           pathCopy,
                           false,
                           QVariantMap(),
                           QStringLiteral("Backpressure queue limit exceeded"),
                           0);
        return requestId;
    }

    QPointer<QCoreApplication> appGuard(QCoreApplication::instance());
    QPointer<Backend> backendGuard(this);
    threadPoolForLane(lane).start(new LambdaRunnable(
        [appGuard, backendGuard, cancelToken, requestId, pathCopy, textCopy, byteCount, operation]() {
            if (cancelToken && cancelToken->loadAcquire() != 0) {
                if (!appGuard)
                    return;
                QMetaObject::invokeMethod(appGuard.data(),
                                          [backendGuard, requestId, operation, pathCopy]() {
                                              if (!backendGuard)
                                                  return;
                                              QVariantMap canceledResult;
                                              canceledResult.insert(QStringLiteral("canceled"), true);
                                              backendGuard->finishAsyncRequest(requestId,
                                                                               operation,
                                                                               pathCopy,
                                                                               false,
                                                                               canceledResult,
                                                                               QStringLiteral("Canceled by request"),
                                                                               0);
                                          },
                                          Qt::QueuedConnection);
                return;
            }

            const qint64 startedMs = QDateTime::currentMSecsSinceEpoch();
            if (backendGuard) {
                QMetaObject::invokeMethod(backendGuard.data(),
                                          [backendGuard, requestId, operation, pathCopy, startedMs]() {
                                              if (!backendGuard)
                                                  return;
                                              backendGuard->markAsyncRequestStarted(requestId,
                                                                                    operation,
                                                                                    pathCopy,
                                                                                    startedMs);
                                          },
                                          Qt::QueuedConnection);
            }

            if (cancelToken && cancelToken->loadAcquire() != 0) {
                if (!appGuard)
                    return;
                QMetaObject::invokeMethod(appGuard.data(),
                                          [backendGuard, requestId, operation, pathCopy]() {
                                              if (!backendGuard)
                                                  return;
                                              QVariantMap canceledResult;
                                              canceledResult.insert(QStringLiteral("canceled"), true);
                                              backendGuard->finishAsyncRequest(requestId,
                                                                               operation,
                                                                               pathCopy,
                                                                               false,
                                                                               canceledResult,
                                                                               QStringLiteral("Canceled by request"),
                                                                               0);
                                          },
                                          Qt::QueuedConnection);
                return;
            }

            const SaveTextOutcome outcome = saveTextFileSync(pathCopy, textCopy);
            const qint64 elapsedMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startedMs);

            QVariantMap result;
            result.insert(QStringLiteral("bytes"), byteCount);

            if (!appGuard)
                return;
            QMetaObject::invokeMethod(appGuard.data(),
                                      [backendGuard, requestId, operation, pathCopy, textCopy, outcome, result, elapsedMs]() {
                                          if (!backendGuard)
                                              return;
                                          if (outcome.ok)
                                              backendGuard->updateReadTextCache(pathCopy, textCopy);
                                          else
                                              backendGuard->invalidateReadTextCache(pathCopy);
                                          backendGuard->finishAsyncRequest(requestId,
                                                                           operation,
                                                                           pathCopy,
                                                                           outcome.ok,
                                                                           result,
                                                                           outcome.error,
                                                                           elapsedMs);
                                      },
                                      Qt::QueuedConnection);
        }),
                          taskPriorityForLane(lane));
    return requestId;
}

qulonglong Backend::readTextFileAsync(const QString &path)
{
    setLastError(QString());
    const QString operation = QStringLiteral("readTextFile");
    const AsyncLane lane = AsyncLane::Io;
    const qulonglong requestId = beginAsyncRequest(operation, path, lane);
    const QString pathCopy = path;
    const QSharedPointer<QAtomicInt> cancelToken = m_asyncCancelTokenByRequest.value(requestId);

    if (path.trimmed().isEmpty()) {
        finishAsyncRequest(requestId,
                           operation,
                           pathCopy,
                           false,
                           QVariantMap(),
                           QStringLiteral("Empty path"),
                           0);
        return requestId;
    }

    if (laneQueueSaturated(lane)) {
        m_asyncBackpressureDropCount += 1;
        finishAsyncRequest(requestId,
                           operation,
                           pathCopy,
                           false,
                           QVariantMap(),
                           QStringLiteral("Backpressure queue limit exceeded"),
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
                           operation,
                           pathCopy,
                           true,
                           result,
                           QString(),
                           0);
        return requestId;
    }

    QPointer<QCoreApplication> appGuard(QCoreApplication::instance());
    QPointer<Backend> backendGuard(this);
    threadPoolForLane(lane).start(new LambdaRunnable(
        [appGuard, backendGuard, cancelToken, requestId, pathCopy, operation]() {
            if (cancelToken && cancelToken->loadAcquire() != 0) {
                if (!appGuard)
                    return;
                QMetaObject::invokeMethod(appGuard.data(),
                                          [backendGuard, requestId, operation, pathCopy]() {
                                              if (!backendGuard)
                                                  return;
                                              QVariantMap canceledResult;
                                              canceledResult.insert(QStringLiteral("canceled"), true);
                                              backendGuard->finishAsyncRequest(requestId,
                                                                               operation,
                                                                               pathCopy,
                                                                               false,
                                                                               canceledResult,
                                                                               QStringLiteral("Canceled by request"),
                                                                               0);
                                          },
                                          Qt::QueuedConnection);
                return;
            }

            const qint64 startedMs = QDateTime::currentMSecsSinceEpoch();
            if (backendGuard) {
                QMetaObject::invokeMethod(backendGuard.data(),
                                          [backendGuard, requestId, operation, pathCopy, startedMs]() {
                                              if (!backendGuard)
                                                  return;
                                              backendGuard->markAsyncRequestStarted(requestId,
                                                                                    operation,
                                                                                    pathCopy,
                                                                                    startedMs);
                                          },
                                          Qt::QueuedConnection);
            }
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
                                      [backendGuard, requestId, operation, pathCopy, outcome, result, elapsedMs]() {
                                          if (!backendGuard)
                                              return;
                                          if (outcome.ok)
                                              backendGuard->updateReadTextCache(pathCopy, outcome.text);
                                          else
                                              backendGuard->invalidateReadTextCache(pathCopy);
                                          backendGuard->finishAsyncRequest(requestId,
                                                                           operation,
                                                                           pathCopy,
                                                                           outcome.ok,
                                                                           result,
                                                                           outcome.error,
                                                                           elapsedMs);
                                      },
                                      Qt::QueuedConnection);
        }),
                          taskPriorityForLane(lane));
    return requestId;
}

qulonglong Backend::ensureDirAsync(const QString &path)
{
    setLastError(QString());
    const QString operation = QStringLiteral("ensureDir");
    const AsyncLane lane = AsyncLane::Io;
    const qulonglong requestId = beginAsyncRequest(operation, path, lane);
    const QString pathCopy = path;
    const QSharedPointer<QAtomicInt> cancelToken = m_asyncCancelTokenByRequest.value(requestId);

    if (path.trimmed().isEmpty()) {
        finishAsyncRequest(requestId,
                           operation,
                           pathCopy,
                           false,
                           QVariantMap(),
                           QStringLiteral("Empty path"),
                           0);
        return requestId;
    }

    if (laneQueueSaturated(lane)) {
        m_asyncBackpressureDropCount += 1;
        finishAsyncRequest(requestId,
                           operation,
                           pathCopy,
                           false,
                           QVariantMap(),
                           QStringLiteral("Backpressure queue limit exceeded"),
                           0);
        return requestId;
    }

    QPointer<QCoreApplication> appGuard(QCoreApplication::instance());
    QPointer<Backend> backendGuard(this);
    threadPoolForLane(lane).start(new LambdaRunnable(
        [appGuard, backendGuard, cancelToken, requestId, pathCopy, operation]() {
            if (cancelToken && cancelToken->loadAcquire() != 0) {
                if (!appGuard)
                    return;
                QMetaObject::invokeMethod(appGuard.data(),
                                          [backendGuard, requestId, operation, pathCopy]() {
                                              if (!backendGuard)
                                                  return;
                                              QVariantMap canceledResult;
                                              canceledResult.insert(QStringLiteral("canceled"), true);
                                              backendGuard->finishAsyncRequest(requestId,
                                                                               operation,
                                                                               pathCopy,
                                                                               false,
                                                                               canceledResult,
                                                                               QStringLiteral("Canceled by request"),
                                                                               0);
                                          },
                                          Qt::QueuedConnection);
                return;
            }

            const qint64 startedMs = QDateTime::currentMSecsSinceEpoch();
            if (backendGuard) {
                QMetaObject::invokeMethod(backendGuard.data(),
                                          [backendGuard, requestId, operation, pathCopy, startedMs]() {
                                              if (!backendGuard)
                                                  return;
                                              backendGuard->markAsyncRequestStarted(requestId,
                                                                                    operation,
                                                                                    pathCopy,
                                                                                    startedMs);
                                          },
                                          Qt::QueuedConnection);
            }
            const EnsureDirOutcome outcome = ensureDirSync(pathCopy);
            const qint64 elapsedMs = qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - startedMs);

            QVariantMap result;
            result.insert(QStringLiteral("ensured"), outcome.ok);

            if (!appGuard)
                return;
            QMetaObject::invokeMethod(appGuard.data(),
                                      [backendGuard, requestId, operation, pathCopy, outcome, result, elapsedMs]() {
                                          if (!backendGuard)
                                              return;
                                          backendGuard->finishAsyncRequest(requestId,
                                                                           operation,
                                                                           pathCopy,
                                                                           outcome.ok,
                                                                           result,
                                                                           outcome.error,
                                                                           elapsedMs);
                                      },
                                      Qt::QueuedConnection);
        }),
                          taskPriorityForLane(lane));
    return requestId;
}

qulonglong Backend::dispatchAsyncTask(const QString &taskName, const QVariantMap &payload, int delayMs)
{
    setLastError(QString());
    const QString normalizedTaskName = taskName.trimmed().isEmpty()
        ? QStringLiteral("task")
        : taskName.trimmed();
    const QString operation = QStringLiteral("dispatchTask");
    const AsyncLane lane = resolveDispatchLane(normalizedTaskName, payload);
    const QVariantMap payloadCopy = payload;
    const int requestedDelayMs = qBound(0, delayMs, 60 * 60 * 1000);
    const int requestedWorkMs = qBound(0, payloadCopy.value(QStringLiteral("workMs")).toInt(), 120000);
    const bool allowCoalescing = payloadCopy.value(QStringLiteral("coalesce"), true).toBool();
    const QString coalesceKey = allowCoalescing
        ? (payloadCopy.value(QStringLiteral("coalesceKey")).toString().trimmed().isEmpty()
            ? normalizedTaskName
            : payloadCopy.value(QStringLiteral("coalesceKey")).toString().trimmed())
        : QString();

    auto finishAsMerged = [&](qulonglong mergedIntoRequestId) -> qulonglong {
        const qulonglong mergedRequestId = beginAsyncRequest(operation, normalizedTaskName, lane);
        QVariantMap mergedResult = payloadCopy;
        mergedResult.insert(QStringLiteral("taskName"), normalizedTaskName);
        mergedResult.insert(QStringLiteral("delayMs"), 0);
        mergedResult.insert(QStringLiteral("requestedDelayMs"), requestedDelayMs);
        mergedResult.insert(QStringLiteral("lane"), QString::fromLatin1(laneName(lane)));
        mergedResult.insert(QStringLiteral("coalesced"), true);
        mergedResult.insert(QStringLiteral("mergedIntoRequestId"), QVariant::fromValue(mergedIntoRequestId));
        mergedResult.insert(QStringLiteral("coalesceKey"), coalesceKey);
        m_asyncMergedRequestCount += 1;
        finishAsyncRequest(mergedRequestId,
                           operation,
                           normalizedTaskName,
                           true,
                           mergedResult,
                           QString(),
                           0);
        return mergedRequestId;
    };

    if (!coalesceKey.isEmpty()) {
        const qulonglong leaderId = m_dispatchCoalescedLeaderByKey.value(coalesceKey, 0);
        if (leaderId > 0 && m_asyncActiveRequestIds.contains(leaderId))
            return finishAsMerged(leaderId);
    }

    const qulonglong requestId = beginAsyncRequest(operation, normalizedTaskName, lane);
    const QSharedPointer<QAtomicInt> cancelToken = m_asyncCancelTokenByRequest.value(requestId);

    if (laneQueueSaturated(lane)) {
        m_asyncBackpressureDropCount += 1;
        finishAsyncRequest(requestId,
                           operation,
                           normalizedTaskName,
                           false,
                           QVariantMap(),
                           QStringLiteral("Backpressure queue limit exceeded"),
                           0);
        return requestId;
    }

    if (!coalesceKey.isEmpty()) {
        m_dispatchCoalescedLeaderByKey.insert(coalesceKey, requestId);
        m_dispatchCoalesceKeyByRequest.insert(requestId, coalesceKey);
    }

    QPointer<QCoreApplication> appGuard(QCoreApplication::instance());
    QPointer<Backend> backendGuard(this);
    threadPoolForLane(lane).start(new LambdaRunnable(
        [appGuard,
         backendGuard,
         cancelToken,
         requestId,
         normalizedTaskName,
         payloadCopy,
         requestedDelayMs,
         requestedWorkMs,
         lane,
         operation,
         coalesceKey]() {
            if (cancelToken && cancelToken->loadAcquire() != 0) {
                if (!appGuard)
                    return;
                QMetaObject::invokeMethod(appGuard.data(),
                                          [backendGuard, requestId, operation, normalizedTaskName]() {
                                              if (!backendGuard)
                                                  return;
                                              QVariantMap canceledResult;
                                              canceledResult.insert(QStringLiteral("canceled"), true);
                                              backendGuard->finishAsyncRequest(requestId,
                                                                               operation,
                                                                               normalizedTaskName,
                                                                               false,
                                                                               canceledResult,
                                                                               QStringLiteral("Canceled by request"),
                                                                               0);
                                          },
                                          Qt::QueuedConnection);
                return;
            }

            const qint64 startedMs = QDateTime::currentMSecsSinceEpoch();
            if (backendGuard) {
                QMetaObject::invokeMethod(backendGuard.data(),
                                          [backendGuard, requestId, operation, normalizedTaskName, startedMs]() {
                                              if (!backendGuard)
                                                  return;
                                              backendGuard->markAsyncRequestStarted(requestId,
                                                                                    operation,
                                                                                    normalizedTaskName,
                                                                                    startedMs);
                                          },
                                          Qt::QueuedConnection);
            }

            if (requestedWorkMs > 0)
                QThread::msleep(static_cast<unsigned long>(requestedWorkMs));

            QVariantMap result = payloadCopy;
            result.insert(QStringLiteral("taskName"), normalizedTaskName);
            result.insert(QStringLiteral("delayMs"), 0);
            result.insert(QStringLiteral("requestedDelayMs"), requestedDelayMs);
            result.insert(QStringLiteral("lane"), QString::fromLatin1(laneName(lane)));
            result.insert(QStringLiteral("workMs"), requestedWorkMs);
            result.insert(QStringLiteral("coalesced"), false);
            if (!coalesceKey.isEmpty())
                result.insert(QStringLiteral("coalesceKey"), coalesceKey);

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
                          taskPriorityForLane(lane));
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

QVariantList Backend::recentPerformanceTrace(int limit) const
{
    if (limit <= 0 || limit >= m_performanceTrace.size())
        return m_performanceTrace;

    QVariantList subset;
    const int start = m_performanceTrace.size() - limit;
    subset.reserve(limit);
    for (int i = start; i < m_performanceTrace.size(); ++i)
        subset.append(m_performanceTrace.at(i));
    return subset;
}

void Backend::clearPerformanceTrace()
{
    if (m_performanceTrace.isEmpty())
        return;
    m_performanceTrace.clear();
    emit performanceTraceChanged();
    emit performanceMetricsChanged();
}

bool Backend::cancelAsyncRequest(qulonglong requestId, const QString &reason)
{
    if (!m_asyncActiveRequestIds.contains(requestId))
        return false;

    const QSharedPointer<QAtomicInt> cancelToken = m_asyncCancelTokenByRequest.value(requestId);
    if (!cancelToken)
        return false;

    const QString normalizedReason = reason.trimmed().isEmpty()
        ? QStringLiteral("Canceled by request")
        : reason.trimmed();
    cancelToken->storeRelease(1);
    m_asyncCancelReasonByRequest.insert(requestId, normalizedReason);

    const QString operation = m_asyncOperationByRequest.value(requestId);
    const QString subject = m_asyncSubjectByRequest.value(requestId);
    emit asyncRequestCanceled(requestId, operation, subject, normalizedReason);
    appendPerformanceTrace(QStringLiteral("canceled"),
                           requestId,
                           operation,
                           subject,
                           QVariantMap {
                               { QStringLiteral("reason"), normalizedReason }
                           });

    if (!m_asyncStartedEpochMs.contains(requestId)) {
        QVariantMap canceledResult;
        canceledResult.insert(QStringLiteral("canceled"), true);
        canceledResult.insert(QStringLiteral("cancelReason"), normalizedReason);
        finishAsyncRequest(requestId,
                           operation,
                           subject,
                           false,
                           canceledResult,
                           normalizedReason,
                           0);
    }

    return true;
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

int Backend::asyncIoMaxConcurrency() const
{
    return m_asyncMaxConcurrencyByLane.at(laneIndex(AsyncLane::Io));
}

int Backend::asyncUtilityMaxConcurrency() const
{
    return m_asyncMaxConcurrencyByLane.at(laneIndex(AsyncLane::Utility));
}

int Backend::asyncRenderMaxConcurrency() const
{
    return m_asyncMaxConcurrencyByLane.at(laneIndex(AsyncLane::Render));
}

int Backend::asyncQueueDepth() const
{
    return m_asyncQueueDepth;
}

int Backend::asyncQueuePeakDepth() const
{
    return m_asyncQueuePeakDepth;
}

int Backend::asyncQueueDepthLimit() const
{
    return m_asyncQueueDepthLimit;
}

void Backend::setAsyncQueueDepthLimit(int value)
{
    const int next = qBound(kAsyncQueueDepthLimitMin, value, kAsyncQueueDepthLimitMax);
    if (m_asyncQueueDepthLimit == next)
        return;
    m_asyncQueueDepthLimit = next;
    emit asyncQueueDepthLimitChanged();
    emit performanceMetricsChanged();
}

qulonglong Backend::asyncBackpressureDropCount() const
{
    return m_asyncBackpressureDropCount;
}

qulonglong Backend::asyncMergedRequestCount() const
{
    return m_asyncMergedRequestCount;
}

qulonglong Backend::asyncCanceledRequestCount() const
{
    return m_asyncCanceledRequestCount;
}

QVariantMap Backend::performanceMetrics() const
{
    QVariantMap metrics;
    metrics.insert(QStringLiteral("schema"), QStringLiteral("lvrs.performance.v1"));
    metrics.insert(QStringLiteral("component"), QStringLiteral("Backend"));
    metrics.insert(QStringLiteral("epochMs"), QVariant::fromValue(QDateTime::currentMSecsSinceEpoch()));
    metrics.insert(QStringLiteral("asyncJobsInFlight"), m_asyncJobsInFlight);
    metrics.insert(QStringLiteral("asyncMaxConcurrency"), m_asyncMaxConcurrency);
    metrics.insert(QStringLiteral("asyncQueueDepth"), m_asyncQueueDepth);
    metrics.insert(QStringLiteral("asyncQueuePeakDepth"), m_asyncQueuePeakDepth);
    metrics.insert(QStringLiteral("asyncQueueDepthLimit"), m_asyncQueueDepthLimit);
    metrics.insert(QStringLiteral("asyncIoMaxConcurrency"), asyncIoMaxConcurrency());
    metrics.insert(QStringLiteral("asyncUtilityMaxConcurrency"), asyncUtilityMaxConcurrency());
    metrics.insert(QStringLiteral("asyncRenderMaxConcurrency"), asyncRenderMaxConcurrency());
    metrics.insert(QStringLiteral("asyncBackpressureDropCount"), QVariant::fromValue(m_asyncBackpressureDropCount));
    metrics.insert(QStringLiteral("asyncMergedRequestCount"), QVariant::fromValue(m_asyncMergedRequestCount));
    metrics.insert(QStringLiteral("asyncCanceledRequestCount"), QVariant::fromValue(m_asyncCanceledRequestCount));
    metrics.insert(QStringLiteral("performanceTraceCount"), m_performanceTrace.size());
    metrics.insert(QStringLiteral("performanceTraceCapacity"), m_performanceTraceCapacity);
    metrics.insert(QStringLiteral("readTextCacheTtlMs"), m_readTextCacheTtlMs);
    metrics.insert(QStringLiteral("readTextCacheCapacityBytes"), QVariant::fromValue(m_readTextCacheCapacityBytes));
    metrics.insert(QStringLiteral("readTextCacheBytes"), QVariant::fromValue(m_readTextCacheBytes));
    metrics.insert(QStringLiteral("readTextCacheEntryCount"), m_readTextCache.size());

    QVariantMap laneMetrics;
    for (const AsyncLane lane : { AsyncLane::Io, AsyncLane::Utility, AsyncLane::Render }) {
        QVariantMap laneMetric;
        laneMetric.insert(QStringLiteral("queued"), laneQueueDepth(lane));
        laneMetric.insert(QStringLiteral("running"), laneRunningCount(lane));
        laneMetric.insert(QStringLiteral("maxConcurrency"), laneMaxConcurrency(lane));
        laneMetric.insert(QStringLiteral("queuePeak"), m_asyncQueuePeakByLane.at(laneIndex(lane)));
        laneMetrics.insert(QString::fromLatin1(laneName(lane)), laneMetric);
    }
    metrics.insert(QStringLiteral("asyncLaneMetrics"), laneMetrics);

    QVariantMap byOperation;
    for (auto it = m_asyncOperationCountByOperation.constBegin();
         it != m_asyncOperationCountByOperation.constEnd();
         ++it) {
        byOperation.insert(it.key(), buildLatencySummary(it.key()));
    }
    metrics.insert(QStringLiteral("asyncLatencyByOperation"), byOperation);
    return metrics;
}

int Backend::performanceTraceCapacity() const
{
    return m_performanceTraceCapacity;
}

void Backend::setPerformanceTraceCapacity(int value)
{
    const int next = qBound(128, value, 16384);
    if (m_performanceTraceCapacity == next)
        return;
    m_performanceTraceCapacity = next;
    while (m_performanceTrace.size() > m_performanceTraceCapacity)
        m_performanceTrace.removeFirst();
    emit performanceTraceCapacityChanged();
    emit performanceTraceChanged();
    emit performanceMetricsChanged();
}

int Backend::performanceTraceCount() const
{
    return m_performanceTrace.size();
}

void Backend::setAsyncMaxConcurrency(int value)
{
    const int next = qBound(1, value, 64);
    if (m_asyncMaxConcurrency == next)
        return;

    m_asyncMaxConcurrency = next;
    applyAsyncPoolLayout();
    updateAsyncQueueDepth();
    emit asyncMaxConcurrencyChanged();
    emit asyncPoolLayoutChanged();
    emit performanceMetricsChanged();
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
    const qint64 beforeBytes = m_readTextCacheBytes;
    pruneExpiredReadTextCache(QDateTime::currentMSecsSinceEpoch());
    if (beforeBytes != m_readTextCacheBytes)
        emit readTextCacheBytesChanged();
    emit readTextCacheTtlMsChanged();
}

qint64 Backend::readTextCacheCapacityBytes() const
{
    return m_readTextCacheCapacityBytes;
}

void Backend::setReadTextCacheCapacityBytes(qint64 value)
{
    const qint64 next = qBound(kReadTextCacheCapacityMinBytes,
                               value,
                               kReadTextCacheCapacityMaxBytes);
    if (m_readTextCacheCapacityBytes == next)
        return;

    m_readTextCacheCapacityBytes = next;
    const qint64 beforeBytes = m_readTextCacheBytes;
    pruneReadTextCacheByCapacity();
    if (beforeBytes != m_readTextCacheBytes)
        emit readTextCacheBytesChanged();
    emit readTextCacheCapacityBytesChanged();
    emit performanceMetricsChanged();
}

qint64 Backend::readTextCacheBytes() const
{
    return m_readTextCacheBytes;
}

int Backend::readTextCacheEntryCount() const
{
    return m_readTextCache.size();
}

qulonglong Backend::beginAsyncRequest(const QString &operation,
                                      const QString &subject,
                                      AsyncLane lane)
{
    const qulonglong requestId = ++m_asyncNextRequestId;
    const qint64 queuedEpochMs = QDateTime::currentMSecsSinceEpoch();
    m_asyncActiveRequestIds.insert(requestId);
    m_asyncQueuedEpochMs.insert(requestId, queuedEpochMs);
    m_asyncLaneByRequest.insert(requestId, lane);
    m_asyncOperationByRequest.insert(requestId, operation);
    m_asyncSubjectByRequest.insert(requestId, subject);
    m_asyncCancelTokenByRequest.insert(requestId, QSharedPointer<QAtomicInt>::create(0));
    m_asyncQueuedByLane[laneIndex(lane)] += 1;
    setAsyncJobsInFlight(m_asyncJobsInFlight + 1);
    emit asyncRequestQueued(requestId, operation, subject);
    appendPerformanceTrace(QStringLiteral("queued"),
                           requestId,
                           operation,
                           subject,
                           QVariantMap {
                               { QStringLiteral("lane"), QString::fromLatin1(laneName(lane)) },
                               { QStringLiteral("queuedEpochMs"), QVariant::fromValue(queuedEpochMs) },
                               { QStringLiteral("queueDepth"), m_asyncQueueDepth },
                               { QStringLiteral("laneQueueDepth"), laneQueueDepth(lane) }
                           });
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
    if (!m_asyncActiveRequestIds.remove(requestId))
        return;

    const AsyncLane lane = m_asyncLaneByRequest.value(requestId, AsyncLane::Utility);
    const int laneIdx = laneIndex(lane);
    const qint64 finishedEpochMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 queuedEpochMs = m_asyncQueuedEpochMs.take(requestId);
    const bool started = m_asyncStartedEpochMs.contains(requestId);
    const qint64 startedEpochMs = m_asyncStartedEpochMs.take(requestId);
    if (started) {
        if (m_asyncRunningByLane.at(laneIdx) > 0)
            m_asyncRunningByLane[laneIdx] -= 1;
    } else {
        if (m_asyncQueuedByLane.at(laneIdx) > 0)
            m_asyncQueuedByLane[laneIdx] -= 1;
    }

    const bool canceled = isCancellationRequested(requestId);
    const QString canceledReason = canceled
        ? takeCancellationReason(requestId)
        : QString();
    QVariantMap finalResult = result;
    QString finalError = canceled ? canceledReason : error;
    bool finalOk = ok;
    if (canceled) {
        finalOk = false;
        finalResult.insert(QStringLiteral("canceled"), true);
        if (!canceledReason.isEmpty())
            finalResult.insert(QStringLiteral("cancelReason"), canceledReason);
        m_asyncCanceledRequestCount += 1;
    }

    const qint64 queueWaitMs = (queuedEpochMs > 0 && startedEpochMs > 0)
        ? qMax<qint64>(0, startedEpochMs - queuedEpochMs)
        : 0;
    const qint64 totalLatencyMs = queuedEpochMs > 0
        ? qMax<qint64>(0, finishedEpochMs - queuedEpochMs)
        : qMax<qint64>(0, elapsedMs);

    if (!finalOk)
        setLastError(finalError);
    recordAsyncLatencySample(operation, totalLatencyMs, finalOk);
    appendPerformanceTrace(QStringLiteral("finished"),
                           requestId,
                           operation,
                           subject,
                           QVariantMap {
                               { QStringLiteral("lane"), QString::fromLatin1(laneName(lane)) },
                               { QStringLiteral("ok"), finalOk },
                               { QStringLiteral("canceled"), canceled },
                               { QStringLiteral("queueWaitMs"), QVariant::fromValue(queueWaitMs) },
                               { QStringLiteral("runMs"), QVariant::fromValue(qMax<qint64>(0, elapsedMs)) },
                               { QStringLiteral("totalLatencyMs"), QVariant::fromValue(totalLatencyMs) },
                               { QStringLiteral("error"), finalError }
                           });
    setAsyncJobsInFlight(qMax(0, m_asyncJobsInFlight - 1));
    cleanupRequestState(requestId);
    emit asyncRequestFinished(requestId,
                              operation,
                              subject,
                              finalOk,
                              finalResult,
                              finalError,
                              qMax<qint64>(0, elapsedMs));
    emit performanceMetricsChanged();
}

void Backend::setAsyncJobsInFlight(int value)
{
    const int next = qMax(0, value);
    if (m_asyncJobsInFlight == next)
        return;
    m_asyncJobsInFlight = next;
    updateAsyncQueueDepth();
    emit asyncJobsInFlightChanged();
    emit performanceMetricsChanged();
}

void Backend::updateAsyncQueueDepth()
{
    const int nextDepth = m_asyncQueuedByLane.at(0)
        + m_asyncQueuedByLane.at(1)
        + m_asyncQueuedByLane.at(2);
    if (m_asyncQueueDepth != nextDepth) {
        m_asyncQueueDepth = nextDepth;
        emit asyncQueueDepthChanged();
    }

    if (nextDepth > m_asyncQueuePeakDepth) {
        m_asyncQueuePeakDepth = nextDepth;
        emit asyncQueuePeakDepthChanged();
    }

    for (const AsyncLane lane : { AsyncLane::Io, AsyncLane::Utility, AsyncLane::Render }) {
        const int idx = laneIndex(lane);
        if (m_asyncQueuedByLane.at(idx) > m_asyncQueuePeakByLane.at(idx))
            m_asyncQueuePeakByLane[idx] = m_asyncQueuedByLane.at(idx);
    }
}

void Backend::markAsyncRequestStarted(qulonglong requestId,
                                      const QString &operation,
                                      const QString &subject,
                                      qint64 startedEpochMs)
{
    if (startedEpochMs <= 0 || !m_asyncActiveRequestIds.contains(requestId))
        return;

    if (m_asyncStartedEpochMs.contains(requestId))
        return;

    const AsyncLane lane = m_asyncLaneByRequest.value(requestId, AsyncLane::Utility);
    const int laneIdx = laneIndex(lane);
    if (m_asyncQueuedByLane.at(laneIdx) > 0)
        m_asyncQueuedByLane[laneIdx] -= 1;
    m_asyncRunningByLane[laneIdx] += 1;
    m_asyncStartedEpochMs.insert(requestId, startedEpochMs);
    const qint64 queuedEpochMs = m_asyncQueuedEpochMs.value(requestId, -1);
    const qint64 queueWaitMs = queuedEpochMs > 0
        ? qMax<qint64>(0, startedEpochMs - queuedEpochMs)
        : 0;

    updateAsyncQueueDepth();
    appendPerformanceTrace(QStringLiteral("started"),
                           requestId,
                           operation,
                           subject,
                           QVariantMap {
                               { QStringLiteral("lane"), QString::fromLatin1(laneName(lane)) },
                               { QStringLiteral("startedEpochMs"), QVariant::fromValue(startedEpochMs) },
                               { QStringLiteral("queueWaitMs"), QVariant::fromValue(queueWaitMs) },
                               { QStringLiteral("queueDepth"), m_asyncQueueDepth },
                               { QStringLiteral("laneQueueDepth"), laneQueueDepth(lane) }
                           });
    emit performanceMetricsChanged();
}

void Backend::recordAsyncLatencySample(const QString &operation, qint64 totalLatencyMs, bool ok)
{
    const QString key = operation.trimmed().isEmpty()
        ? QStringLiteral("unknown")
        : operation.trimmed();
    QList<qint64> &samples = m_asyncLatencySamplesByOperation[key];
    samples.append(qMax<qint64>(0, totalLatencyMs));
    while (samples.size() > m_asyncLatencySampleCapacity)
        samples.removeFirst();

    m_asyncOperationCountByOperation.insert(key, m_asyncOperationCountByOperation.value(key, 0) + 1);
    if (!ok)
        m_asyncOperationFailureCountByOperation.insert(key, m_asyncOperationFailureCountByOperation.value(key, 0) + 1);
}

void Backend::appendPerformanceTrace(const QString &phase,
                                     qulonglong requestId,
                                     const QString &operation,
                                     const QString &subject,
                                     const QVariantMap &detail)
{
    QVariantMap entry;
    entry.insert(QStringLiteral("schema"), QStringLiteral("lvrs.performance.trace.v1"));
    entry.insert(QStringLiteral("sequence"), QVariant::fromValue(++m_asyncTraceSequence));
    entry.insert(QStringLiteral("epochMs"), QVariant::fromValue(QDateTime::currentMSecsSinceEpoch()));
    entry.insert(QStringLiteral("phase"), phase);
    entry.insert(QStringLiteral("requestId"), QVariant::fromValue(requestId));
    entry.insert(QStringLiteral("operation"), operation);
    entry.insert(QStringLiteral("subject"), subject);
    entry.insert(QStringLiteral("detail"), detail);
    m_performanceTrace.append(entry);

    while (m_performanceTrace.size() > m_performanceTraceCapacity)
        m_performanceTrace.removeFirst();

    emit performanceTraceChanged();
}

QVariantMap Backend::buildLatencySummary(const QString &operation) const
{
    QVariantMap summary;
    const QList<qint64> samples = m_asyncLatencySamplesByOperation.value(operation);
    if (samples.isEmpty()) {
        summary.insert(QStringLiteral("count"), QVariant::fromValue(m_asyncOperationCountByOperation.value(operation, 0)));
        summary.insert(QStringLiteral("failureCount"), QVariant::fromValue(m_asyncOperationFailureCountByOperation.value(operation, 0)));
        summary.insert(QStringLiteral("avgMs"), 0.0);
        summary.insert(QStringLiteral("p50Ms"), 0.0);
        summary.insert(QStringLiteral("p95Ms"), 0.0);
        summary.insert(QStringLiteral("p99Ms"), 0.0);
        summary.insert(QStringLiteral("maxMs"), 0.0);
        summary.insert(QStringLiteral("sampleSize"), 0);
        return summary;
    }

    const double total = std::accumulate(samples.constBegin(), samples.constEnd(), 0.0);
    const qint64 p50 = percentileValue(samples, 50.0);
    const qint64 p95 = percentileValue(samples, 95.0);
    const qint64 p99 = percentileValue(samples, 99.0);
    const qint64 maxValue = *std::max_element(samples.constBegin(), samples.constEnd());
    const quint64 count = m_asyncOperationCountByOperation.value(operation, samples.size());
    const quint64 failureCount = m_asyncOperationFailureCountByOperation.value(operation, 0);
    const double failureRate = count > 0
        ? (static_cast<double>(failureCount) / static_cast<double>(count))
        : 0.0;

    summary.insert(QStringLiteral("count"), QVariant::fromValue(count));
    summary.insert(QStringLiteral("failureCount"), QVariant::fromValue(failureCount));
    summary.insert(QStringLiteral("failureRate"), failureRate);
    summary.insert(QStringLiteral("sampleSize"), samples.size());
    summary.insert(QStringLiteral("avgMs"), total / static_cast<double>(samples.size()));
    summary.insert(QStringLiteral("p50Ms"), QVariant::fromValue(p50));
    summary.insert(QStringLiteral("p95Ms"), QVariant::fromValue(p95));
    summary.insert(QStringLiteral("p99Ms"), QVariant::fromValue(p99));
    summary.insert(QStringLiteral("maxMs"), QVariant::fromValue(maxValue));
    return summary;
}

qint64 Backend::percentileValue(QList<qint64> values, double percentile)
{
    if (values.isEmpty())
        return 0;

    std::sort(values.begin(), values.end());
    const double clamped = qBound(0.0, percentile, 100.0);
    const int index = qBound(0,
                             static_cast<int>(std::ceil((clamped / 100.0) * values.size())) - 1,
                             values.size() - 1);
    return values.at(index);
}

const char *Backend::laneName(AsyncLane lane)
{
    switch (lane) {
    case AsyncLane::Io:
        return "io";
    case AsyncLane::Utility:
        return "utility";
    case AsyncLane::Render:
        return "render";
    }
    return "utility";
}

int Backend::laneIndex(AsyncLane lane)
{
    return static_cast<int>(lane);
}

Backend::AsyncLane Backend::resolveDispatchLane(const QString &taskName, const QVariantMap &payload) const
{
    const QString payloadLane = payload.value(QStringLiteral("lane")).toString().trimmed().toLower();
    if (payloadLane == QStringLiteral("io"))
        return AsyncLane::Io;
    if (payloadLane == QStringLiteral("render"))
        return AsyncLane::Render;
    if (payloadLane == QStringLiteral("utility"))
        return AsyncLane::Utility;

    const QString normalizedName = taskName.trimmed().toLower();
    if (normalizedName.startsWith(QStringLiteral("io:")))
        return AsyncLane::Io;
    if (normalizedName.startsWith(QStringLiteral("render:")))
        return AsyncLane::Render;
    return AsyncLane::Utility;
}

int Backend::laneQueueDepth(AsyncLane lane) const
{
    return m_asyncQueuedByLane.at(laneIndex(lane));
}

int Backend::laneRunningCount(AsyncLane lane) const
{
    return m_asyncRunningByLane.at(laneIndex(lane));
}

int Backend::laneMaxConcurrency(AsyncLane lane) const
{
    return m_asyncMaxConcurrencyByLane.at(laneIndex(lane));
}

bool Backend::laneQueueSaturated(AsyncLane lane) const
{
    return laneQueueDepth(lane) > m_asyncQueueDepthLimit;
}

void Backend::applyAsyncPoolLayout()
{
    const int total = qBound(1, m_asyncMaxConcurrency, 64);

    int ioConcurrency = 1;
    int utilityConcurrency = 1;
    int renderConcurrency = 1;
    if (total >= 3) {
        ioConcurrency = qMax(1, (total * 2) / 4);
        utilityConcurrency = qMax(1, total / 4);
        renderConcurrency = qMax(1, total - ioConcurrency - utilityConcurrency);
    }

    m_asyncMaxConcurrencyByLane[laneIndex(AsyncLane::Io)] = ioConcurrency;
    m_asyncMaxConcurrencyByLane[laneIndex(AsyncLane::Utility)] = utilityConcurrency;
    m_asyncMaxConcurrencyByLane[laneIndex(AsyncLane::Render)] = renderConcurrency;

    m_ioThreadPool.setMaxThreadCount(ioConcurrency);
    m_utilityThreadPool.setMaxThreadCount(utilityConcurrency);
    m_renderThreadPool.setMaxThreadCount(renderConcurrency);
}

QThreadPool &Backend::threadPoolForLane(AsyncLane lane)
{
    switch (lane) {
    case AsyncLane::Io:
        return m_ioThreadPool;
    case AsyncLane::Utility:
        return m_utilityThreadPool;
    case AsyncLane::Render:
        return m_renderThreadPool;
    }
    return m_utilityThreadPool;
}

int Backend::taskPriorityForLane(AsyncLane lane) const
{
    switch (lane) {
    case AsyncLane::Io:
        return kIoTaskPriority;
    case AsyncLane::Render:
        return kRenderTaskPriority;
    case AsyncLane::Utility:
        return kUtilityTaskPriority;
    }
    return kUtilityTaskPriority;
}

bool Backend::isCancellationRequested(qulonglong requestId) const
{
    const QSharedPointer<QAtomicInt> token = m_asyncCancelTokenByRequest.value(requestId);
    if (token && token->loadAcquire() != 0)
        return true;
    return m_asyncCancelReasonByRequest.contains(requestId);
}

QString Backend::takeCancellationReason(qulonglong requestId)
{
    QString reason = m_asyncCancelReasonByRequest.take(requestId);
    if (reason.trimmed().isEmpty() && isCancellationRequested(requestId))
        reason = QStringLiteral("Canceled by request");
    return reason;
}

void Backend::cleanupRequestState(qulonglong requestId)
{
    m_asyncQueuedEpochMs.remove(requestId);
    m_asyncStartedEpochMs.remove(requestId);
    m_asyncLaneByRequest.remove(requestId);
    m_asyncOperationByRequest.remove(requestId);
    m_asyncSubjectByRequest.remove(requestId);
    m_asyncCancelTokenByRequest.remove(requestId);
    m_asyncCancelReasonByRequest.remove(requestId);

    const QString coalesceKey = m_dispatchCoalesceKeyByRequest.take(requestId);
    if (!coalesceKey.isEmpty()) {
        const qulonglong leader = m_dispatchCoalescedLeaderByKey.value(coalesceKey, 0);
        if (leader == requestId)
            m_dispatchCoalescedLeaderByKey.remove(coalesceKey);
    }
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
        if (isReadCacheEntryExpired(it.value(), nowMs)) {
            m_readTextCacheBytes = qMax<qint64>(0, m_readTextCacheBytes - qMax<qint64>(0, it.value().byteSize));
            it = m_readTextCache.erase(it);
        } else {
            ++it;
        }
    }
}

void Backend::pruneReadTextCacheByCapacity()
{
    if (m_readTextCacheCapacityBytes <= 0) {
        m_readTextCache.clear();
        m_readTextCacheBytes = 0;
        return;
    }

    while (m_readTextCacheBytes > m_readTextCacheCapacityBytes && !m_readTextCache.isEmpty()) {
        auto oldestIt = m_readTextCache.begin();
        qint64 oldestAccessMs = oldestIt.value().lastAccessedMs;
        if (oldestAccessMs < 0)
            oldestAccessMs = oldestIt.value().cachedAtMs;

        for (auto it = m_readTextCache.begin(); it != m_readTextCache.end(); ++it) {
            qint64 candidateMs = it.value().lastAccessedMs;
            if (candidateMs < 0)
                candidateMs = it.value().cachedAtMs;
            if (candidateMs < oldestAccessMs) {
                oldestIt = it;
                oldestAccessMs = candidateMs;
            }
        }

        m_readTextCacheBytes = qMax<qint64>(0, m_readTextCacheBytes - qMax<qint64>(0, oldestIt.value().byteSize));
        m_readTextCache.erase(oldestIt);
    }
}

void Backend::eraseReadTextCacheEntry(const QString &key)
{
    auto it = m_readTextCache.find(key);
    if (it == m_readTextCache.end())
        return;
    m_readTextCacheBytes = qMax<qint64>(0, m_readTextCacheBytes - qMax<qint64>(0, it.value().byteSize));
    m_readTextCache.erase(it);
}

bool Backend::tryReadTextCache(const QString &path, QString *text)
{
    const QString key = normalizePathForCache(path);
    if (key.isEmpty())
        return false;

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 beforeBytes = m_readTextCacheBytes;
    pruneExpiredReadTextCache(nowMs);
    if (beforeBytes != m_readTextCacheBytes) {
        emit readTextCacheBytesChanged();
        emit performanceMetricsChanged();
    }

    auto it = m_readTextCache.find(key);
    if (it == m_readTextCache.end())
        return false;

    if (isReadCacheEntryExpired(it.value(), nowMs)) {
        const qint64 beforeEraseBytes = m_readTextCacheBytes;
        eraseReadTextCacheEntry(key);
        if (beforeEraseBytes != m_readTextCacheBytes) {
            emit readTextCacheBytesChanged();
            emit performanceMetricsChanged();
        }
        return false;
    }

    const FileState state = inspectFileState(key);
    if (!state.ok
        || state.size != it.value().size
        || state.lastModifiedMs != it.value().lastModifiedMs) {
        const qint64 beforeEraseBytes = m_readTextCacheBytes;
        eraseReadTextCacheEntry(key);
        if (beforeEraseBytes != m_readTextCacheBytes) {
            emit readTextCacheBytesChanged();
            emit performanceMetricsChanged();
        }
        return false;
    }

    it.value().lastAccessedMs = nowMs;
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
    const qint64 beforeBytes = m_readTextCacheBytes;
    pruneExpiredReadTextCache(nowMs);

    ReadCacheEntry entry;
    entry.text = text;
    entry.size = state.size;
    entry.lastModifiedMs = state.lastModifiedMs;
    entry.cachedAtMs = nowMs;
    entry.lastAccessedMs = nowMs;
    entry.byteSize = qMax<qint64>(0, static_cast<qint64>(text.size()) * static_cast<qint64>(sizeof(QChar)));

    eraseReadTextCacheEntry(key);
    m_readTextCache.insert(key, entry);
    m_readTextCacheBytes += entry.byteSize;
    pruneReadTextCacheByCapacity();

    if (beforeBytes != m_readTextCacheBytes)
        emit readTextCacheBytesChanged();
    emit performanceMetricsChanged();
}

void Backend::invalidateReadTextCache(const QString &path)
{
    const QString key = normalizePathForCache(path);
    if (key.isEmpty())
        return;
    const qint64 beforeBytes = m_readTextCacheBytes;
    eraseReadTextCacheEntry(key);
    if (beforeBytes != m_readTextCacheBytes)
        emit readTextCacheBytesChanged();
    emit performanceMetricsChanged();
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
