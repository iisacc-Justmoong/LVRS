#include "backend/runtime/gestureevents.h"

#include "backend/runtime/runtimeevents.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QLineF>

namespace {

qint64 eventTimestampEpochMs(const QVariantMap &eventData)
{
    return eventData.value(QStringLiteral("timestampEpochMs")).toLongLong();
}

QVariantMap runtimePayload(const QVariantMap &eventData)
{
    return eventData.value(QStringLiteral("payload")).toMap();
}

int rawInt(const QVariantMap &payload, const QString &key, int fallback = 0)
{
    const QVariant value = payload.value(key);
    return value.isValid() ? value.toInt() : fallback;
}

int rawFingerCount(const QVariantMap &payload)
{
    return rawInt(payload,
                  QStringLiteral("fingerCount"),
                  rawInt(payload, QStringLiteral("pointCount"), 0));
}

int rawActiveFingerCount(const QVariantMap &payload)
{
    return rawInt(payload, QStringLiteral("activeFingerCount"), rawFingerCount(payload));
}

QVariantMap pointerUiFallbackMap(const QVariantMap &rawPayload)
{
    QVariantMap ui = rawPayload.value(QStringLiteral("pointerUi")).toMap();
    if (!ui.isEmpty())
        return ui;

    QVariantMap fallback;
    if (rawPayload.contains(QStringLiteral("pointerObjectName")))
        fallback.insert(QStringLiteral("objectName"), rawPayload.value(QStringLiteral("pointerObjectName")));
    if (rawPayload.contains(QStringLiteral("pointerClassName")))
        fallback.insert(QStringLiteral("className"), rawPayload.value(QStringLiteral("pointerClassName")));
    if (rawPayload.contains(QStringLiteral("pointerPath")))
        fallback.insert(QStringLiteral("path"), rawPayload.value(QStringLiteral("pointerPath")));
    return fallback;
}

QString signedDirectionToken(const qreal delta)
{
    if (delta > 0.0)
        return QStringLiteral("positive");
    if (delta < 0.0)
        return QStringLiteral("negative");
    return QStringLiteral("none");
}

QString dominantAxisForDeltas(const qreal deltaX, const qreal deltaY, const qreal axisDominanceRatio)
{
    const qreal absX = qAbs(deltaX);
    const qreal absY = qAbs(deltaY);
    if (absX < 0.01 && absY < 0.01)
        return QStringLiteral("none");
    if (absX >= absY * axisDominanceRatio)
        return QStringLiteral("x");
    if (absY >= absX * axisDominanceRatio)
        return QStringLiteral("y");
    return QStringLiteral("diagonal");
}

QString swipeDirectionForDeltas(const qreal deltaX, const qreal deltaY, const qreal axisDominanceRatio)
{
    const QString dominantAxis = dominantAxisForDeltas(deltaX, deltaY, axisDominanceRatio);
    if (dominantAxis == QStringLiteral("x"))
        return deltaX >= 0.0 ? QStringLiteral("leftToRight") : QStringLiteral("rightToLeft");
    if (dominantAxis == QStringLiteral("y"))
        return deltaY >= 0.0 ? QStringLiteral("topToBottom") : QStringLiteral("bottomToTop");
    if (deltaX >= 0.0 && deltaY >= 0.0)
        return QStringLiteral("topLeftToBottomRight");
    if (deltaX >= 0.0 && deltaY < 0.0)
        return QStringLiteral("bottomLeftToTopRight");
    if (deltaX < 0.0 && deltaY >= 0.0)
        return QStringLiteral("topRightToBottomLeft");
    return QStringLiteral("bottomRightToTopLeft");
}

QString classificationForInteraction(const QString &gestureType, const QString &interactionKind)
{
    if (gestureType == QStringLiteral("pressStarted")
        || gestureType == QStringLiteral("pressEnded")) {
        return QStringLiteral("press");
    }
    if (interactionKind == QStringLiteral("nativeGesture"))
        return QStringLiteral("gesture");
    if (interactionKind.isEmpty())
        return QStringLiteral("touch");
    return interactionKind;
}

qreal distanceBetween(const QPointF &a, const QPointF &b)
{
    return QLineF(a, b).length();
}

} // namespace

GestureEvents::GestureEvents(QObject *parent)
    : QObject(parent)
{
    m_holdTimer.setSingleShot(true);
    connect(&m_holdTimer, &QTimer::timeout, this, &GestureEvents::emitHoldStarted);
}

bool GestureEvents::runtimeAttached() const
{
    return !m_runtimeEvents.isNull();
}

int GestureEvents::holdThresholdMs() const
{
    return m_holdThresholdMs;
}

void GestureEvents::setHoldThresholdMs(int value)
{
    const int bounded = qMax(0, value);
    if (m_holdThresholdMs == bounded)
        return;
    m_holdThresholdMs = bounded;
    if (m_touchActive && !m_holdActive) {
        m_holdTimer.stop();
        if (m_holdThresholdMs > 0)
            m_holdTimer.start(m_holdThresholdMs);
    }
    emit holdThresholdMsChanged();
}

qreal GestureEvents::dragThresholdPx() const
{
    return m_dragThresholdPx;
}

void GestureEvents::setDragThresholdPx(qreal value)
{
    const qreal bounded = qMax<qreal>(0.0, value);
    if (qFuzzyCompare(m_dragThresholdPx, bounded))
        return;
    m_dragThresholdPx = bounded;
    emit dragThresholdPxChanged();
}

qreal GestureEvents::scrollThresholdPx() const
{
    return m_scrollThresholdPx;
}

void GestureEvents::setScrollThresholdPx(qreal value)
{
    const qreal bounded = qMax<qreal>(0.0, value);
    if (qFuzzyCompare(m_scrollThresholdPx, bounded))
        return;
    m_scrollThresholdPx = bounded;
    emit scrollThresholdPxChanged();
}

qreal GestureEvents::swipeThresholdPx() const
{
    return m_swipeThresholdPx;
}

void GestureEvents::setSwipeThresholdPx(qreal value)
{
    const qreal bounded = qMax<qreal>(0.0, value);
    if (qFuzzyCompare(m_swipeThresholdPx, bounded))
        return;
    m_swipeThresholdPx = bounded;
    emit swipeThresholdPxChanged();
}

int GestureEvents::swipeMaxDurationMs() const
{
    return m_swipeMaxDurationMs;
}

void GestureEvents::setSwipeMaxDurationMs(int value)
{
    const int bounded = qMax(0, value);
    if (m_swipeMaxDurationMs == bounded)
        return;
    m_swipeMaxDurationMs = bounded;
    emit swipeMaxDurationMsChanged();
}

qreal GestureEvents::axisDominanceRatio() const
{
    return m_axisDominanceRatio;
}

void GestureEvents::setAxisDominanceRatio(qreal value)
{
    const qreal bounded = qMax<qreal>(1.0, value);
    if (qFuzzyCompare(m_axisDominanceRatio, bounded))
        return;
    m_axisDominanceRatio = bounded;
    emit axisDominanceRatioChanged();
}

quint64 GestureEvents::gestureSequence() const
{
    return m_gestureSequence;
}

QVariantMap GestureEvents::lastGesture() const
{
    return m_lastGesture;
}

bool GestureEvents::attachRuntime(QObject *runtimeObject)
{
    bindRuntime(resolveRuntime(runtimeObject));
    return runtimeAttached();
}

void GestureEvents::detachRuntime()
{
    if (!m_runtimeEvents.isNull()) {
        disconnect(m_runtimeRecordedConnection);
        disconnect(m_runtimeDestroyedConnection);
        m_runtimeEvents.clear();
    }
    resetState();
    emit runtimeAttachedChanged();
}

void GestureEvents::resetState()
{
    m_holdTimer.stop();
    resetTouchSessionInternal(false);
}

RuntimeEvents *GestureEvents::resolveRuntime(QObject *runtimeObject) const
{
    if (auto *runtime = qobject_cast<RuntimeEvents *>(runtimeObject))
        return runtime;
    if (RuntimeEvents::instance())
        return RuntimeEvents::instance();
    if (!qApp)
        return nullptr;
    const QList<RuntimeEvents *> runtimes = qApp->findChildren<RuntimeEvents *>();
    return runtimes.isEmpty() ? nullptr : runtimes.constFirst();
}

void GestureEvents::bindRuntime(RuntimeEvents *runtime)
{
    if (m_runtimeEvents == runtime)
        return;

    if (!m_runtimeEvents.isNull()) {
        disconnect(m_runtimeRecordedConnection);
        disconnect(m_runtimeDestroyedConnection);
        m_runtimeEvents.clear();
    }

    m_runtimeEvents = runtime;
    if (m_runtimeEvents.isNull()) {
        emit runtimeAttachedChanged();
        return;
    }

    m_runtimeRecordedConnection = connect(m_runtimeEvents,
                                          &RuntimeEvents::eventRecorded,
                                          this,
                                          &GestureEvents::handleRuntimeEventRecorded);
    m_runtimeDestroyedConnection = connect(m_runtimeEvents,
                                           &QObject::destroyed,
                                           this,
                                           [this]() {
                                               detachRuntime();
                                           });
    emit runtimeAttachedChanged();
}

void GestureEvents::handleRuntimeEventRecorded(const QVariantMap &eventData)
{
    const QString type = eventData.value(QStringLiteral("type")).toString();
    if (type == QStringLiteral("touch-event")) {
        handleTouchRuntimeEvent(eventData);
        return;
    }
    if (type == QStringLiteral("native-gesture"))
        handleNativeGestureRuntimeEvent(eventData);
}

void GestureEvents::handleTouchRuntimeEvent(const QVariantMap &eventData)
{
    const QVariantMap rawPayload = runtimePayload(eventData);
    const QString phase = rawPayload.value(QStringLiteral("phase")).toString();
    if (phase == QStringLiteral("begin")) {
        startTouchSession(eventData, rawPayload);
        return;
    }
    if (phase == QStringLiteral("update")) {
        updateTouchSession(eventData, rawPayload);
        return;
    }
    if (phase == QStringLiteral("end")) {
        endTouchSession(eventData, rawPayload, false);
        return;
    }
    if (phase == QStringLiteral("cancel"))
        endTouchSession(eventData, rawPayload, true);
}

void GestureEvents::handleNativeGestureRuntimeEvent(const QVariantMap &eventData)
{
    const QVariantMap payload = publishGesture(buildNativeGesturePayload(eventData));
    emit nativeGestureDetected(payload);
}

void GestureEvents::startTouchSession(const QVariantMap &eventData, const QVariantMap &rawPayload)
{
    if (m_touchActive)
        endTouchSession(eventData, rawPayload, true);

    const qint64 timestampEpochMs = eventTimestampEpochMs(eventData);
    const QPointF currentPos(rawPayload.value(QStringLiteral("x")).toReal(),
                             rawPayload.value(QStringLiteral("y")).toReal());

    m_touchActive = true;
    m_holdActive = false;
    m_dragActive = false;
    m_activeSessionId = m_nextSessionId++;
    m_touchStartEpochMs = timestampEpochMs;
    m_lastTouchEpochMs = timestampEpochMs;
    m_touchStartPos = currentPos;
    m_lastTouchPos = currentPos;
    m_touchStartUi = resolvedPointerUi(rawPayload);
    m_lastTouchPayload = rawPayload;
    m_maximumFingerCount = qMax(rawFingerCount(rawPayload), rawActiveFingerCount(rawPayload));

    const QVariantMap payload = publishGesture(buildTouchPayload(QStringLiteral("touchStarted"),
                                                                 rawPayload,
                                                                 timestampEpochMs,
                                                                 QStringLiteral("touch"),
                                                                 currentPos,
                                                                 currentPos));
    emit touchStarted(payload);

    const QVariantMap pressPayload = publishGesture(buildTouchPayload(QStringLiteral("pressStarted"),
                                                                      rawPayload,
                                                                      timestampEpochMs,
                                                                      QStringLiteral("press"),
                                                                      currentPos,
                                                                      currentPos));
    emit pressStarted(pressPayload);

    if (m_holdThresholdMs > 0)
        m_holdTimer.start(m_holdThresholdMs);
}

void GestureEvents::updateTouchSession(const QVariantMap &eventData, const QVariantMap &rawPayload)
{
    if (!m_touchActive) {
        startTouchSession(eventData, rawPayload);
        return;
    }

    const qint64 timestampEpochMs = eventTimestampEpochMs(eventData);
    const QPointF currentPos(rawPayload.value(QStringLiteral("x")).toReal(),
                             rawPayload.value(QStringLiteral("y")).toReal());
    const QPointF previousPos = m_lastTouchPos;
    const qreal totalDistance = distanceBetween(m_touchStartPos, currentPos);
    m_maximumFingerCount = qMax(m_maximumFingerCount,
                                qMax(rawFingerCount(rawPayload), rawActiveFingerCount(rawPayload)));

    if (!m_holdActive && totalDistance >= qMin(m_dragThresholdPx, m_scrollThresholdPx))
        m_holdTimer.stop();

    const QString dominantAxis = dominantAxisForDeltas(currentPos.x() - m_touchStartPos.x(),
                                                       currentPos.y() - m_touchStartPos.y(),
                                                       m_axisDominanceRatio);
    const bool scrollCandidate = !rawPayload.value(QStringLiteral("multiTouch")).toBool()
        && (dominantAxis == QStringLiteral("x") || dominantAxis == QStringLiteral("y"))
        && totalDistance >= m_scrollThresholdPx;
    if (!m_scrollActive && scrollCandidate) {
        m_scrollActive = true;
        const QVariantMap payload = publishGesture(buildTouchPayload(QStringLiteral("scrollStarted"),
                                                                     rawPayload,
                                                                     timestampEpochMs,
                                                                     QStringLiteral("scroll"),
                                                                     currentPos,
                                                                     previousPos));
        emit scrollStarted(payload);
    }

    if (!m_dragActive && totalDistance >= m_dragThresholdPx) {
        m_dragActive = true;
        const QVariantMap payload = publishGesture(buildTouchPayload(QStringLiteral("dragStarted"),
                                                                     rawPayload,
                                                                     timestampEpochMs,
                                                                     QStringLiteral("drag"),
                                                                     currentPos,
                                                                     previousPos));
        emit dragStarted(payload);
    }

    const QVariantMap touchPayload = publishGesture(buildTouchPayload(QStringLiteral("touchUpdated"),
                                                                      rawPayload,
                                                                      timestampEpochMs,
                                                                      m_scrollActive
                                                                      ? QStringLiteral("scroll")
                                                                      : m_dragActive
                                                                      ? QStringLiteral("drag")
                                                                      : (m_holdActive ? QStringLiteral("hold")
                                                                                      : QStringLiteral("touch")),
                                                                      currentPos,
                                                                      previousPos));
    emit touchUpdated(touchPayload);

    if (m_dragActive) {
        const QVariantMap dragPayload = publishGesture(buildTouchPayload(QStringLiteral("dragUpdated"),
                                                                         rawPayload,
                                                                         timestampEpochMs,
                                                                         QStringLiteral("drag"),
                                                                         currentPos,
                                                                         previousPos));
        emit dragUpdated(dragPayload);
    }

    if (m_scrollActive) {
        const QVariantMap scrollPayload = publishGesture(buildTouchPayload(QStringLiteral("scrollUpdated"),
                                                                           rawPayload,
                                                                           timestampEpochMs,
                                                                           QStringLiteral("scroll"),
                                                                           currentPos,
                                                                           previousPos));
        emit scrollUpdated(scrollPayload);
    }

    m_lastTouchEpochMs = timestampEpochMs;
    m_lastTouchPos = currentPos;
    m_lastTouchPayload = rawPayload;
}

void GestureEvents::endTouchSession(const QVariantMap &eventData,
                                    const QVariantMap &rawPayload,
                                    bool cancelled)
{
    if (!m_touchActive) {
        resetTouchSessionInternal(false);
        return;
    }

    m_holdTimer.stop();

    const qint64 timestampEpochMs = eventTimestampEpochMs(eventData);
    const QPointF currentPos(rawPayload.value(QStringLiteral("x")).toReal(),
                             rawPayload.value(QStringLiteral("y")).toReal());
    const QPointF previousPos = m_lastTouchPos;
    const qreal totalDeltaX = currentPos.x() - m_touchStartPos.x();
    const qreal totalDeltaY = currentPos.y() - m_touchStartPos.y();
    const qreal totalDistance = distanceBetween(m_touchStartPos, currentPos);
    const qint64 durationMs = m_touchStartEpochMs >= 0
        ? qMax<qint64>(0, timestampEpochMs - m_touchStartEpochMs)
        : 0;
    m_maximumFingerCount = qMax(m_maximumFingerCount,
                                qMax(rawFingerCount(rawPayload), rawActiveFingerCount(rawPayload)));
    const bool swipeQualified = !cancelled
        && durationMs <= m_swipeMaxDurationMs
        && totalDistance >= m_swipeThresholdPx;

    if (m_dragActive) {
        const QVariantMap dragPayload = publishGesture(buildTouchPayload(QStringLiteral("dragEnded"),
                                                                         rawPayload,
                                                                         timestampEpochMs,
                                                                         QStringLiteral("drag"),
                                                                         currentPos,
                                                                         previousPos));
        emit dragEnded(dragPayload);
    }

    if (m_scrollActive) {
        const QVariantMap scrollPayload = publishGesture(buildTouchPayload(QStringLiteral("scrollEnded"),
                                                                           rawPayload,
                                                                           timestampEpochMs,
                                                                           QStringLiteral("scroll"),
                                                                           currentPos,
                                                                           previousPos));
        emit scrollEnded(scrollPayload);
    }

    if (swipeQualified) {
        const QVariantMap swipePayload = publishGesture(buildSwipePayload(rawPayload,
                                                                         timestampEpochMs,
                                                                         currentPos));
        emit swipeDetected(swipePayload);
    }

    const QString interactionKind = cancelled
        ? QStringLiteral("cancel")
        : swipeQualified
            ? QStringLiteral("swipe")
            : m_scrollActive
                ? QStringLiteral("scroll")
                : m_dragActive
                    ? QStringLiteral("drag")
                    : m_holdActive
                        ? QStringLiteral("hold")
                        : QStringLiteral("tap");
    const QString gestureType = cancelled
        ? QStringLiteral("touchCancelled")
        : QStringLiteral("touchEnded");
    const QVariantMap touchPayload = publishGesture(buildTouchPayload(gestureType,
                                                                      rawPayload,
                                                                      timestampEpochMs,
                                                                      interactionKind,
                                                                      currentPos,
                                                                      previousPos));
    if (cancelled)
        emit touchCancelled(touchPayload);
    else
        emit touchEnded(touchPayload);

    QVariantMap pressPayload = buildTouchPayload(QStringLiteral("pressEnded"),
                                                 rawPayload,
                                                 timestampEpochMs,
                                                 QStringLiteral("press"),
                                                 currentPos,
                                                 previousPos);
    pressPayload.insert(QStringLiteral("finalInteractionKind"), interactionKind);
    pressPayload = publishGesture(pressPayload);
    emit pressEnded(pressPayload);

    resetTouchSessionInternal(true);
}

void GestureEvents::emitHoldStarted()
{
    if (!m_touchActive || m_holdActive || m_dragActive)
        return;

    m_holdActive = true;
    const qint64 timestampEpochMs = QDateTime::currentMSecsSinceEpoch();
    const QVariantMap payload = publishGesture(buildTouchPayload(QStringLiteral("holdStarted"),
                                                                 m_lastTouchPayload,
                                                                 timestampEpochMs,
                                                                 QStringLiteral("hold"),
                                                                 m_lastTouchPos,
                                                                 m_lastTouchPos));
    emit holdStarted(payload);
}

QVariantMap GestureEvents::resolvedPointerUi(const QVariantMap &rawPayload) const
{
    QVariantMap ui = pointerUiFallbackMap(rawPayload);
    const bool needsResolution = ui.isEmpty()
        || ui.value(QStringLiteral("objectName")).toString() == QStringLiteral("unknown")
        || ui.value(QStringLiteral("layerKind")).toString() == QStringLiteral("outsideWindow");
    if (!needsResolution || m_runtimeEvents.isNull())
        return ui;

    const qreal globalX = rawPayload.value(QStringLiteral("x")).toReal();
    const qreal globalY = rawPayload.value(QStringLiteral("y")).toReal();
    const QVariantMap resolved = m_runtimeEvents->hitTestUiAt(globalX, globalY);
    return resolved.isEmpty() ? ui : resolved;
}

QVariantMap GestureEvents::buildTouchPayload(const QString &gestureType,
                                             const QVariantMap &rawPayload,
                                             qint64 timestampEpochMs,
                                             const QString &interactionKind,
                                             const QPointF &currentPos,
                                             const QPointF &previousPos) const
{
    QVariantMap payload;
    const qreal deltaX = currentPos.x() - previousPos.x();
    const qreal deltaY = currentPos.y() - previousPos.y();
    const qreal totalDeltaX = currentPos.x() - m_touchStartPos.x();
    const qreal totalDeltaY = currentPos.y() - m_touchStartPos.y();
    const QString dominantAxis = dominantAxisForDeltas(totalDeltaX, totalDeltaY, m_axisDominanceRatio);
    const qint64 durationMs = m_touchStartEpochMs >= 0
        ? qMax<qint64>(0, timestampEpochMs - m_touchStartEpochMs)
        : 0;
    const QVariantMap currentUi = resolvedPointerUi(rawPayload);

    payload.insert(QStringLiteral("gestureType"), gestureType);
    payload.insert(QStringLiteral("interactionKind"), interactionKind);
    payload.insert(QStringLiteral("classification"), classificationForInteraction(gestureType, interactionKind));
    payload.insert(QStringLiteral("source"), QStringLiteral("touch"));
    payload.insert(QStringLiteral("sessionId"), QVariant::fromValue(m_activeSessionId));
    payload.insert(QStringLiteral("timestampEpochMs"), QVariant::fromValue(timestampEpochMs));
    payload.insert(QStringLiteral("x"), currentPos.x());
    payload.insert(QStringLiteral("y"), currentPos.y());
    payload.insert(QStringLiteral("globalX"), currentPos.x());
    payload.insert(QStringLiteral("globalY"), currentPos.y());
    payload.insert(QStringLiteral("previousX"), previousPos.x());
    payload.insert(QStringLiteral("previousY"), previousPos.y());
    payload.insert(QStringLiteral("startX"), m_touchStartPos.x());
    payload.insert(QStringLiteral("startY"), m_touchStartPos.y());
    payload.insert(QStringLiteral("startGlobalX"), m_touchStartPos.x());
    payload.insert(QStringLiteral("startGlobalY"), m_touchStartPos.y());
    payload.insert(QStringLiteral("deltaX"), deltaX);
    payload.insert(QStringLiteral("deltaY"), deltaY);
    payload.insert(QStringLiteral("totalDeltaX"), totalDeltaX);
    payload.insert(QStringLiteral("totalDeltaY"), totalDeltaY);
    payload.insert(QStringLiteral("absoluteDeltaX"), qAbs(totalDeltaX));
    payload.insert(QStringLiteral("absoluteDeltaY"), qAbs(totalDeltaY));
    payload.insert(QStringLiteral("distance"), distanceBetween(m_touchStartPos, currentPos));
    payload.insert(QStringLiteral("durationMs"), QVariant::fromValue(durationMs));
    payload.insert(QStringLiteral("pressDurationMs"),
                   rawPayload.contains(QStringLiteral("pressDurationMs"))
                   ? rawPayload.value(QStringLiteral("pressDurationMs"))
                   : QVariant::fromValue(durationMs));
    payload.insert(QStringLiteral("directionX"), signedDirectionToken(totalDeltaX));
    payload.insert(QStringLiteral("directionY"), signedDirectionToken(totalDeltaY));
    payload.insert(QStringLiteral("dominantAxis"), dominantAxis);
    payload.insert(QStringLiteral("holdActive"), m_holdActive);
    payload.insert(QStringLiteral("dragActive"), m_dragActive);
    payload.insert(QStringLiteral("scrollActive"), m_scrollActive);
    payload.insert(QStringLiteral("holdThresholdMs"), m_holdThresholdMs);
    payload.insert(QStringLiteral("dragThresholdPx"), m_dragThresholdPx);
    payload.insert(QStringLiteral("scrollThresholdPx"), m_scrollThresholdPx);
    payload.insert(QStringLiteral("swipeThresholdPx"), m_swipeThresholdPx);
    payload.insert(QStringLiteral("swipeMaxDurationMs"), m_swipeMaxDurationMs);
    payload.insert(QStringLiteral("fingerCount"), rawFingerCount(rawPayload));
    payload.insert(QStringLiteral("activeFingerCount"), rawActiveFingerCount(rawPayload));
    payload.insert(QStringLiteral("maximumFingerCount"), m_maximumFingerCount);
    payload.insert(QStringLiteral("multiTouch"), rawPayload.value(QStringLiteral("multiTouch")).toBool());

    if (rawPayload.contains(QStringLiteral("phase")))
        payload.insert(QStringLiteral("phase"), rawPayload.value(QStringLiteral("phase")));
    if (rawPayload.contains(QStringLiteral("pointCount")))
        payload.insert(QStringLiteral("pointCount"), rawPayload.value(QStringLiteral("pointCount")));
    if (rawPayload.contains(QStringLiteral("pressedFingerCount")))
        payload.insert(QStringLiteral("pressedFingerCount"), rawPayload.value(QStringLiteral("pressedFingerCount")));
    if (rawPayload.contains(QStringLiteral("updatedFingerCount")))
        payload.insert(QStringLiteral("updatedFingerCount"), rawPayload.value(QStringLiteral("updatedFingerCount")));
    if (rawPayload.contains(QStringLiteral("stationaryFingerCount")))
        payload.insert(QStringLiteral("stationaryFingerCount"), rawPayload.value(QStringLiteral("stationaryFingerCount")));
    if (rawPayload.contains(QStringLiteral("releasedFingerCount")))
        payload.insert(QStringLiteral("releasedFingerCount"), rawPayload.value(QStringLiteral("releasedFingerCount")));
    if (rawPayload.contains(QStringLiteral("primaryPointId")))
        payload.insert(QStringLiteral("primaryPointId"), rawPayload.value(QStringLiteral("primaryPointId")));
    if (rawPayload.contains(QStringLiteral("points")))
        payload.insert(QStringLiteral("points"), rawPayload.value(QStringLiteral("points")));
    if (rawPayload.contains(QStringLiteral("buttons")))
        payload.insert(QStringLiteral("buttons"), rawPayload.value(QStringLiteral("buttons")));
    if (rawPayload.contains(QStringLiteral("pressedMouseButtons")))
        payload.insert(QStringLiteral("pressedMouseButtons"), rawPayload.value(QStringLiteral("pressedMouseButtons")));
    if (rawPayload.contains(QStringLiteral("modifiers")))
        payload.insert(QStringLiteral("modifiers"), rawPayload.value(QStringLiteral("modifiers")));
    if (rawPayload.contains(QStringLiteral("mouseButtonPressed")))
        payload.insert(QStringLiteral("mouseButtonPressed"), rawPayload.value(QStringLiteral("mouseButtonPressed")));
    if (rawPayload.contains(QStringLiteral("released")))
        payload.insert(QStringLiteral("released"), rawPayload.value(QStringLiteral("released")));
    if (rawPayload.contains(QStringLiteral("cancelled")))
        payload.insert(QStringLiteral("cancelled"), rawPayload.value(QStringLiteral("cancelled")));
    if (rawPayload.contains(QStringLiteral("releaseEpochMs")))
        payload.insert(QStringLiteral("releaseEpochMs"), rawPayload.value(QStringLiteral("releaseEpochMs")));
    if (rawPayload.contains(QStringLiteral("lastMousePressEpochMs")))
        payload.insert(QStringLiteral("lastMousePressEpochMs"), rawPayload.value(QStringLiteral("lastMousePressEpochMs")));
    if (rawPayload.contains(QStringLiteral("lastMouseReleaseEpochMs")))
        payload.insert(QStringLiteral("lastMouseReleaseEpochMs"), rawPayload.value(QStringLiteral("lastMouseReleaseEpochMs")));
    if (rawPayload.contains(QStringLiteral("nativeTimestamp")))
        payload.insert(QStringLiteral("nativeTimestamp"), rawPayload.value(QStringLiteral("nativeTimestamp")));
    if (rawPayload.contains(QStringLiteral("touchDeviceName")))
        payload.insert(QStringLiteral("touchDeviceName"), rawPayload.value(QStringLiteral("touchDeviceName")));
    if (rawPayload.contains(QStringLiteral("touchDeviceType")))
        payload.insert(QStringLiteral("touchDeviceType"), rawPayload.value(QStringLiteral("touchDeviceType")));
    if (rawPayload.contains(QStringLiteral("pointerType")))
        payload.insert(QStringLiteral("pointerType"), rawPayload.value(QStringLiteral("pointerType")));
    if (rawPayload.contains(QStringLiteral("maximumTouchPoints")))
        payload.insert(QStringLiteral("maximumTouchPoints"), rawPayload.value(QStringLiteral("maximumTouchPoints")));

    if (interactionKind == QStringLiteral("scroll") || m_scrollActive) {
        payload.insert(QStringLiteral("scrollAxis"), dominantAxis);
        payload.insert(QStringLiteral("scrollDirection"),
                       swipeDirectionForDeltas(totalDeltaX, totalDeltaY, m_axisDominanceRatio));
        payload.insert(QStringLiteral("scrollDeltaX"), totalDeltaX);
        payload.insert(QStringLiteral("scrollDeltaY"), totalDeltaY);
    }

    payload.insert(QStringLiteral("ui"), currentUi);
    payload.insert(QStringLiteral("originUi"), m_touchStartUi);
    return payload;
}

QVariantMap GestureEvents::buildSwipePayload(const QVariantMap &rawPayload,
                                             qint64 timestampEpochMs,
                                             const QPointF &currentPos) const
{
    QVariantMap payload = buildTouchPayload(QStringLiteral("swipeDetected"),
                                            rawPayload,
                                            timestampEpochMs,
                                            QStringLiteral("swipe"),
                                            currentPos,
                                            m_lastTouchPos);
    const qreal totalDeltaX = payload.value(QStringLiteral("totalDeltaX")).toReal();
    const qreal totalDeltaY = payload.value(QStringLiteral("totalDeltaY")).toReal();
    const qint64 durationMs = qMax<qint64>(1, payload.value(QStringLiteral("durationMs")).toLongLong());

    payload.insert(QStringLiteral("swipeDirection"),
                   swipeDirectionForDeltas(totalDeltaX, totalDeltaY, m_axisDominanceRatio));
    payload.insert(QStringLiteral("velocityX"), totalDeltaX / durationMs);
    payload.insert(QStringLiteral("velocityY"), totalDeltaY / durationMs);
    payload.insert(QStringLiteral("speed"), payload.value(QStringLiteral("distance")).toReal() / durationMs);
    return payload;
}

QVariantMap GestureEvents::buildNativeGesturePayload(const QVariantMap &eventData) const
{
    const QVariantMap rawPayload = runtimePayload(eventData);
    QVariantMap payload;
    payload.insert(QStringLiteral("gestureType"), QStringLiteral("nativeGestureDetected"));
    payload.insert(QStringLiteral("interactionKind"), QStringLiteral("nativeGesture"));
    payload.insert(QStringLiteral("classification"), QStringLiteral("gesture"));
    payload.insert(QStringLiteral("source"), QStringLiteral("nativeGesture"));
    payload.insert(QStringLiteral("timestampEpochMs"), QVariant::fromValue(eventTimestampEpochMs(eventData)));
    payload.insert(QStringLiteral("x"), rawPayload.value(QStringLiteral("x")));
    payload.insert(QStringLiteral("y"), rawPayload.value(QStringLiteral("y")));
    payload.insert(QStringLiteral("globalX"), rawPayload.value(QStringLiteral("x")));
    payload.insert(QStringLiteral("globalY"), rawPayload.value(QStringLiteral("y")));
    payload.insert(QStringLiteral("buttons"), rawPayload.value(QStringLiteral("buttons")));
    payload.insert(QStringLiteral("pressedMouseButtons"), rawPayload.value(QStringLiteral("pressedMouseButtons")));
    payload.insert(QStringLiteral("modifiers"), rawPayload.value(QStringLiteral("modifiers")));
    payload.insert(QStringLiteral("nativeGestureType"), rawPayload.value(QStringLiteral("gestureType")));
    payload.insert(QStringLiteral("fingerCount"), rawPayload.value(QStringLiteral("fingerCount")));
    payload.insert(QStringLiteral("value"), rawPayload.value(QStringLiteral("value")));
    payload.insert(QStringLiteral("deltaX"), rawPayload.value(QStringLiteral("deltaX")));
    payload.insert(QStringLiteral("deltaY"), rawPayload.value(QStringLiteral("deltaY")));
    if (rawPayload.contains(QStringLiteral("deviceName")))
        payload.insert(QStringLiteral("deviceName"), rawPayload.value(QStringLiteral("deviceName")));
    if (rawPayload.contains(QStringLiteral("deviceType")))
        payload.insert(QStringLiteral("deviceType"), rawPayload.value(QStringLiteral("deviceType")));
    if (rawPayload.contains(QStringLiteral("pointerType")))
        payload.insert(QStringLiteral("pointerType"), rawPayload.value(QStringLiteral("pointerType")));
    payload.insert(QStringLiteral("ui"), resolvedPointerUi(rawPayload));
    return payload;
}

QVariantMap GestureEvents::publishGesture(const QVariantMap &eventData)
{
    m_gestureSequence += 1;
    m_lastGesture = eventData;
    m_lastGesture.insert(QStringLiteral("sequence"), QVariant::fromValue(m_gestureSequence));
    emit gestureChanged();
    emit gestureRecognized(m_lastGesture);
    return m_lastGesture;
}

void GestureEvents::resetTouchSessionInternal(bool keepLastGesture)
{
    m_touchActive = false;
    m_holdActive = false;
    m_dragActive = false;
    m_scrollActive = false;
    m_activeSessionId = 0;
    m_touchStartEpochMs = -1;
    m_lastTouchEpochMs = -1;
    m_touchStartPos = QPointF();
    m_lastTouchPos = QPointF();
    m_touchStartUi.clear();
    m_lastTouchPayload.clear();
    m_maximumFingerCount = 0;
    if (!keepLastGesture) {
        m_lastGesture.clear();
        emit gestureChanged();
    }
}
