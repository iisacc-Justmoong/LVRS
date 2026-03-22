#pragma once

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QPointF>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

class RuntimeEvents;

class GestureEvents : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(GestureEvents)
    QML_SINGLETON

    Q_PROPERTY(bool runtimeAttached READ runtimeAttached NOTIFY runtimeAttachedChanged)
    Q_PROPERTY(int holdThresholdMs READ holdThresholdMs WRITE setHoldThresholdMs NOTIFY holdThresholdMsChanged)
    Q_PROPERTY(qreal dragThresholdPx READ dragThresholdPx WRITE setDragThresholdPx NOTIFY dragThresholdPxChanged)
    Q_PROPERTY(qreal swipeThresholdPx READ swipeThresholdPx WRITE setSwipeThresholdPx NOTIFY swipeThresholdPxChanged)
    Q_PROPERTY(int swipeMaxDurationMs READ swipeMaxDurationMs WRITE setSwipeMaxDurationMs NOTIFY swipeMaxDurationMsChanged)
    Q_PROPERTY(qreal axisDominanceRatio READ axisDominanceRatio WRITE setAxisDominanceRatio NOTIFY axisDominanceRatioChanged)
    Q_PROPERTY(quint64 gestureSequence READ gestureSequence NOTIFY gestureChanged)
    Q_PROPERTY(QVariantMap lastGesture READ lastGesture NOTIFY gestureChanged)

public:
    explicit GestureEvents(QObject *parent = nullptr);

    bool runtimeAttached() const;

    int holdThresholdMs() const;
    void setHoldThresholdMs(int value);

    qreal dragThresholdPx() const;
    void setDragThresholdPx(qreal value);

    qreal swipeThresholdPx() const;
    void setSwipeThresholdPx(qreal value);

    int swipeMaxDurationMs() const;
    void setSwipeMaxDurationMs(int value);

    qreal axisDominanceRatio() const;
    void setAxisDominanceRatio(qreal value);

    quint64 gestureSequence() const;
    QVariantMap lastGesture() const;

    Q_INVOKABLE bool attachRuntime(QObject *runtimeObject = nullptr);
    Q_INVOKABLE void detachRuntime();
    Q_INVOKABLE void resetState();

signals:
    void runtimeAttachedChanged();
    void holdThresholdMsChanged();
    void dragThresholdPxChanged();
    void swipeThresholdPxChanged();
    void swipeMaxDurationMsChanged();
    void axisDominanceRatioChanged();
    void gestureChanged();

    void gestureRecognized(const QVariantMap &eventData);
    void touchStarted(const QVariantMap &eventData);
    void touchUpdated(const QVariantMap &eventData);
    void touchEnded(const QVariantMap &eventData);
    void touchCancelled(const QVariantMap &eventData);
    void holdStarted(const QVariantMap &eventData);
    void dragStarted(const QVariantMap &eventData);
    void dragUpdated(const QVariantMap &eventData);
    void dragEnded(const QVariantMap &eventData);
    void swipeDetected(const QVariantMap &eventData);
    void nativeGestureDetected(const QVariantMap &eventData);

private:
    RuntimeEvents *resolveRuntime(QObject *runtimeObject) const;
    void bindRuntime(RuntimeEvents *runtime);
    void handleRuntimeEventRecorded(const QVariantMap &eventData);
    void handleTouchRuntimeEvent(const QVariantMap &eventData);
    void handleNativeGestureRuntimeEvent(const QVariantMap &eventData);
    void startTouchSession(const QVariantMap &eventData, const QVariantMap &rawPayload);
    void updateTouchSession(const QVariantMap &eventData, const QVariantMap &rawPayload);
    void endTouchSession(const QVariantMap &eventData, const QVariantMap &rawPayload, bool cancelled);
    void emitHoldStarted();
    QVariantMap resolvedPointerUi(const QVariantMap &rawPayload) const;
    QVariantMap buildTouchPayload(const QString &gestureType,
                                  const QVariantMap &rawPayload,
                                  qint64 timestampEpochMs,
                                  const QString &interactionKind,
                                  const QPointF &currentPos,
                                  const QPointF &previousPos) const;
    QVariantMap buildSwipePayload(const QVariantMap &rawPayload,
                                  qint64 timestampEpochMs,
                                  const QPointF &currentPos) const;
    QVariantMap buildNativeGesturePayload(const QVariantMap &eventData) const;
    QVariantMap publishGesture(const QVariantMap &eventData);
    void resetTouchSessionInternal(bool keepLastGesture);

    QPointer<RuntimeEvents> m_runtimeEvents;
    QMetaObject::Connection m_runtimeRecordedConnection;
    QMetaObject::Connection m_runtimeDestroyedConnection;

    QTimer m_holdTimer;

    bool m_touchActive = false;
    bool m_holdActive = false;
    bool m_dragActive = false;
    quint64 m_activeSessionId = 0;
    quint64 m_nextSessionId = 1;
    qint64 m_touchStartEpochMs = -1;
    qint64 m_lastTouchEpochMs = -1;
    QPointF m_touchStartPos;
    QPointF m_lastTouchPos;
    QVariantMap m_touchStartUi;
    QVariantMap m_lastTouchPayload;

    int m_holdThresholdMs = 450;
    qreal m_dragThresholdPx = 12.0;
    qreal m_swipeThresholdPx = 48.0;
    int m_swipeMaxDurationMs = 700;
    qreal m_axisDominanceRatio = 1.35;

    quint64 m_gestureSequence = 0;
    QVariantMap m_lastGesture;
};
