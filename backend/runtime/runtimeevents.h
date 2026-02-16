#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QPointF>
#include <QSet>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

class QQuickWindow;
class QQuickItem;

class RuntimeEvents : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RuntimeEvents)
    QML_SINGLETON

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

    Q_PROPERTY(quint64 keyPressCount READ keyPressCount NOTIFY keyboardChanged)
    Q_PROPERTY(quint64 keyReleaseCount READ keyReleaseCount NOTIFY keyboardChanged)
    Q_PROPERTY(int lastKey READ lastKey NOTIFY keyboardChanged)
    Q_PROPERTY(QString lastKeyText READ lastKeyText NOTIFY keyboardChanged)
    Q_PROPERTY(int lastKeyModifiers READ lastKeyModifiers NOTIFY keyboardChanged)
    Q_PROPERTY(bool anyKeyPressed READ anyKeyPressed NOTIFY keyboardChanged)
    Q_PROPERTY(QStringList pressedKeys READ pressedKeys NOTIFY keyboardChanged)
    Q_PROPERTY(QVariantList pressedKeyCodes READ pressedKeyCodes NOTIFY keyboardChanged)

    Q_PROPERTY(quint64 mouseMoveCount READ mouseMoveCount NOTIFY mouseChanged)
    Q_PROPERTY(quint64 mousePressCount READ mousePressCount NOTIFY mouseChanged)
    Q_PROPERTY(quint64 mouseReleaseCount READ mouseReleaseCount NOTIFY mouseChanged)
    Q_PROPERTY(qreal lastMouseX READ lastMouseX NOTIFY mouseChanged)
    Q_PROPERTY(qreal lastMouseY READ lastMouseY NOTIFY mouseChanged)
    Q_PROPERTY(int lastMouseButtons READ lastMouseButtons NOTIFY mouseChanged)
    Q_PROPERTY(int lastMouseModifiers READ lastMouseModifiers NOTIFY mouseChanged)
    Q_PROPERTY(bool mouseButtonPressed READ mouseButtonPressed NOTIFY mouseChanged)
    Q_PROPERTY(QVariantMap pointerUi READ pointerUi NOTIFY mouseChanged)
    Q_PROPERTY(qint64 lastMousePressEpochMs READ lastMousePressEpochMs NOTIFY mouseChanged)
    Q_PROPERTY(qint64 lastMouseReleaseEpochMs READ lastMouseReleaseEpochMs NOTIFY mouseChanged)
    Q_PROPERTY(qint64 mousePressElapsedMs READ mousePressElapsedMs NOTIFY daemonStateChanged)
    Q_PROPERTY(qint64 mouseReleaseElapsedMs READ mouseReleaseElapsedMs NOTIFY daemonStateChanged)
    Q_PROPERTY(qint64 activePressDurationMs READ activePressDurationMs NOTIFY daemonStateChanged)
    Q_PROPERTY(QStringList pressedMouseButtonNames READ pressedMouseButtonNames NOTIFY mouseChanged)
    Q_PROPERTY(int activeModifiers READ activeModifiers NOTIFY daemonStateChanged)
    Q_PROPERTY(QStringList activeModifierNames READ activeModifierNames NOTIFY daemonStateChanged)
    Q_PROPERTY(QVariantMap inputStateMap READ inputState NOTIFY daemonStateChanged)

    Q_PROPERTY(quint64 uiCreatedCount READ uiCreatedCount NOTIFY uiChanged)
    Q_PROPERTY(quint64 uiShownCount READ uiShownCount NOTIFY uiChanged)
    Q_PROPERTY(quint64 uiHiddenCount READ uiHiddenCount NOTIFY uiChanged)
    Q_PROPERTY(quint64 uiDestroyedCount READ uiDestroyedCount NOTIFY uiChanged)
    Q_PROPERTY(QString lastUiEvent READ lastUiEvent NOTIFY uiChanged)
    Q_PROPERTY(QString lastUiObjectName READ lastUiObjectName NOTIFY uiChanged)
    Q_PROPERTY(QString lastUiClassName READ lastUiClassName NOTIFY uiChanged)

    Q_PROPERTY(bool idle READ idle NOTIFY idleChanged)
    Q_PROPERTY(int idleTimeoutMs READ idleTimeoutMs WRITE setIdleTimeoutMs NOTIFY idleTimeoutMsChanged)
    Q_PROPERTY(qint64 idleForMs READ idleForMs NOTIFY idleForMsChanged)
    Q_PROPERTY(qint64 lastActivityEpochMs READ lastActivityEpochMs NOTIFY lastActivityChanged)

    Q_PROPERTY(qint64 pid READ pid CONSTANT)
    Q_PROPERTY(QString processName READ processName CONSTANT)
    Q_PROPERTY(QString osName READ osName CONSTANT)
    Q_PROPERTY(qint64 daemonBootEpochMs READ daemonBootEpochMs CONSTANT)
    Q_PROPERTY(quint64 eventSequence READ eventSequence NOTIFY daemonStateChanged)
    Q_PROPERTY(QVariantMap lastEvent READ lastEvent NOTIFY daemonStateChanged)
    Q_PROPERTY(int recentEventCapacity READ recentEventCapacity WRITE setRecentEventCapacity NOTIFY recentEventCapacityChanged)
    Q_PROPERTY(int recentEventCount READ recentEventCount NOTIFY eventLogChanged)
    Q_PROPERTY(bool applicationActive READ applicationActive NOTIFY osChanged)
    Q_PROPERTY(int osSampleIntervalMs READ osSampleIntervalMs WRITE setOsSampleIntervalMs NOTIFY osSampleIntervalMsChanged)
    Q_PROPERTY(qint64 uptimeMs READ uptimeMs NOTIFY osStatsChanged)
    Q_PROPERTY(qint64 rssBytes READ rssBytes NOTIFY osStatsChanged)
    Q_PROPERTY(int captureProfile READ captureProfile WRITE setCaptureProfile NOTIFY captureProfileChanged)
    Q_PROPERTY(bool pointerHitTestingEnabled READ pointerHitTestingEnabled WRITE setPointerHitTestingEnabled NOTIFY pointerHitTestingEnabledChanged)
    Q_PROPERTY(int pointerHitTestMinIntervalMs READ pointerHitTestMinIntervalMs WRITE setPointerHitTestMinIntervalMs NOTIFY pointerHitTestMinIntervalMsChanged)
    Q_PROPERTY(bool uiTrackingEnabled READ uiTrackingEnabled WRITE setUiTrackingEnabled NOTIFY uiTrackingEnabledChanged)

public:
    enum CaptureProfile {
        FullCapture = 0,
        BalancedCapture = 1,
        LowLatencyCapture = 2
    };
    Q_ENUM(CaptureProfile)

    explicit RuntimeEvents(QObject *parent = nullptr);
    ~RuntimeEvents() override;
    static RuntimeEvents *instance();

    bool running() const;

    quint64 keyPressCount() const;
    quint64 keyReleaseCount() const;
    int lastKey() const;
    QString lastKeyText() const;
    int lastKeyModifiers() const;
    bool anyKeyPressed() const;
    QStringList pressedKeys() const;
    QVariantList pressedKeyCodes() const;
    Q_INVOKABLE bool isKeyPressed(int key) const;

    quint64 mouseMoveCount() const;
    quint64 mousePressCount() const;
    quint64 mouseReleaseCount() const;
    qreal lastMouseX() const;
    qreal lastMouseY() const;
    int lastMouseButtons() const;
    int lastMouseModifiers() const;
    bool mouseButtonPressed() const;
    QVariantMap pointerUi() const;
    qint64 lastMousePressEpochMs() const;
    qint64 lastMouseReleaseEpochMs() const;
    qint64 mousePressElapsedMs() const;
    qint64 mouseReleaseElapsedMs() const;
    qint64 activePressDurationMs() const;
    QStringList pressedMouseButtonNames() const;
    int activeModifiers() const;
    QStringList activeModifierNames() const;
    Q_INVOKABLE QVariantMap inputState() const;

    quint64 uiCreatedCount() const;
    quint64 uiShownCount() const;
    quint64 uiHiddenCount() const;
    quint64 uiDestroyedCount() const;
    QString lastUiEvent() const;
    QString lastUiObjectName() const;
    QString lastUiClassName() const;

    bool idle() const;
    int idleTimeoutMs() const;
    void setIdleTimeoutMs(int value);
    qint64 idleForMs() const;
    qint64 lastActivityEpochMs() const;

    qint64 pid() const;
    QString processName() const;
    QString osName() const;
    qint64 daemonBootEpochMs() const;
    quint64 eventSequence() const;
    QVariantMap lastEvent() const;
    int recentEventCapacity() const;
    void setRecentEventCapacity(int value);
    int recentEventCount() const;
    bool applicationActive() const;
    int osSampleIntervalMs() const;
    void setOsSampleIntervalMs(int value);
    qint64 uptimeMs() const;
    qint64 rssBytes() const;
    int captureProfile() const;
    void setCaptureProfile(int value);
    bool pointerHitTestingEnabled() const;
    void setPointerHitTestingEnabled(bool value);
    int pointerHitTestMinIntervalMs() const;
    void setPointerHitTestMinIntervalMs(int value);
    bool uiTrackingEnabled() const;
    void setUiTrackingEnabled(bool value);

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void attachWindow(QObject *window);
    Q_INVOKABLE void markActivity();
    Q_INVOKABLE void resetCounters();
    Q_INVOKABLE QVariantMap snapshot() const;
    Q_INVOKABLE QVariantMap daemonHealth() const;
    Q_INVOKABLE QVariantList recentEvents() const;
    Q_INVOKABLE void clearRecentEvents();
    Q_INVOKABLE QVariantMap hitTestUiAt(qreal globalX, qreal globalY) const;

signals:
    void daemonStateChanged();
    void runningChanged();
    void keyboardChanged();
    void keyPressed(int key, int modifiers, bool autoRepeat, const QString &text);
    void keyReleased(int key, int modifiers, bool autoRepeat, const QString &text);
    void mouseChanged();
    void mouseMoved(qreal x, qreal y, int buttons, int modifiers);
    void mousePressed(qreal x, qreal y, int buttons, int modifiers);
    void mouseReleased(qreal x, qreal y, int buttons, int modifiers);
    void contextRequested(qreal x, qreal y, int modifiers, int reason);
    void uiChanged();
    void uiEvent(const QString &eventType, const QString &objectName, const QString &className, bool visible);
    void idleChanged();
    void idleTimeoutMsChanged();
    void idleForMsChanged();
    void lastActivityChanged();
    void idleEntered();
    void idleExited();
    void osChanged();
    void osSampleIntervalMsChanged();
    void osStatsChanged();
    void osApplicationStateChanged(int state);
    void eventRecorded(const QVariantMap &eventData);
    void eventLogChanged();
    void recentEventCapacityChanged();
    void daemonHeartbeat(qint64 epochMs, qint64 uptimeMs, quint64 eventSequence);
    void captureProfileChanged();
    void pointerHitTestingEnabledChanged();
    void pointerHitTestMinIntervalMsChanged();
    void uiTrackingEnabledChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    struct UiObjectInfo {
        QString objectName;
        QString className;
        bool visible = false;
    };

    void trackUiObjectRecursive(QObject *object);
    void detachTrackedObjects();
    void handleTrackedDestroyed(const QObject *object);
    void recordUiEvent(const QString &eventType, const QObject *object, bool visible);
    void updateMouseFromEvent(qreal x, qreal y, int buttons, int modifiers, bool refreshPointerUi = true);
    void updatePointerUiSnapshot(bool forceRefresh = false);
    void handleIdleTick();
    void handleOsTick();
    void updateIdleState(bool nextIdle);
    qint64 nowEpochMs() const;
    qint64 elapsedSinceEpoch(qint64 epochMs) const;
    qint64 sampleResidentSetBytes() const;
    bool isHighFrequencyEventType(const QString &eventType) const;
    bool shouldSampleHighFrequencyPayload(const QString &eventType, qint64 epochMs) const;
    bool shouldSkipHighFrequencyRecord(const QString &eventType, qint64 epochMs);
    void scheduleRuntimeStateSignals();
    void recordRuntimeEvent(const QString &eventType, const QVariantMap &payload = QVariantMap());
    void pushRecentEvent(const QVariantMap &eventData);
    QVariantMap describeQuickItemAtGlobal(qreal globalX, qreal globalY) const;
    QVariantMap fallbackUiAt(qreal globalX, qreal globalY) const;
    QString keyLabelForCode(int key) const;
    QStringList mouseButtonNames(int buttons) const;
    QStringList modifierNames(int modifiers) const;
    QQuickItem *deepestVisibleChildAt(QQuickItem *item, const QPointF &scenePos) const;
    QString quickItemPath(const QQuickItem *item, const QQuickItem *rootItem) const;

    QPointer<QQuickWindow> m_window;
    QMetaObject::Connection m_windowDestroyedConnection;
    QSet<QObject *> m_trackedObjects;
    QHash<const QObject *, UiObjectInfo> m_trackedInfo;
    QSet<int> m_pressedKeys;

    QTimer m_idleTimer;
    QTimer m_osTimer;
    QTimer m_stateSignalTimer;
    QElapsedTimer m_uptimeTimer;
    qint64 m_lastActivityMonotonicMs = 0;

    bool m_running = false;

    quint64 m_keyPressCount = 0;
    quint64 m_keyReleaseCount = 0;
    int m_lastKey = 0;
    QString m_lastKeyText;
    int m_lastKeyModifiers = 0;

    quint64 m_mouseMoveCount = 0;
    quint64 m_mousePressCount = 0;
    quint64 m_mouseReleaseCount = 0;
    qreal m_lastMouseX = 0.0;
    qreal m_lastMouseY = 0.0;
    int m_lastMouseButtons = 0;
    int m_lastMouseModifiers = 0;
    bool m_mouseButtonPressed = false;
    qint64 m_lastMousePressEpochMs = -1;
    qint64 m_lastMouseReleaseEpochMs = -1;
    QVariantMap m_pointerUi;

    quint64 m_uiCreatedCount = 0;
    quint64 m_uiShownCount = 0;
    quint64 m_uiHiddenCount = 0;
    quint64 m_uiDestroyedCount = 0;
    QString m_lastUiEvent;
    QString m_lastUiObjectName;
    QString m_lastUiClassName;

    bool m_idle = false;
    int m_idleTimeoutMs = 60000;
    qint64 m_idleForMs = 0;
    qint64 m_lastActivityEpochMs = 0;

    bool m_applicationActive = true;
    int m_osSampleIntervalMs = 1000;
    qint64 m_uptimeMs = 0;
    qint64 m_rssBytes = -1;
    qint64 m_daemonBootEpochMs = 0;
    quint64 m_eventSequence = 0;
    QVariantMap m_lastEvent;
    QVariantList m_recentEvents;
    int m_recentEventCapacity = 256;
    int m_highFrequencyEventMinIntervalMs = 16;
    int m_runtimeStateSignalMinIntervalMs = 16;
    int m_captureProfile = FullCapture;
    bool m_pointerHitTestingEnabled = true;
    int m_pointerHitTestMinIntervalMs = 0;
    bool m_uiTrackingEnabled = true;
    qint64 m_lastPointerHitTestEpochMs = -1;
    QHash<QString, qint64> m_lastEventRecordedEpochByType;
    bool m_runtimeStateSignalDirty = false;

    static RuntimeEvents *s_instance;
};
