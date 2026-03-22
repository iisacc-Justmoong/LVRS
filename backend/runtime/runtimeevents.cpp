#include "backend/runtime/runtimeevents.h"

#include <algorithm>
#include <QChildEvent>
#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QEvent>
#include <QFile>
#include <QGuiApplication>
#include <QHoverEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTabletEvent>
#include <QTouchEvent>
#include <QWheelEvent>
#include <QStringList>
#include <QSysInfo>

#ifdef Q_OS_MAC
#include <mach/mach.h>
#endif

#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

namespace {

constexpr int kPrimaryTouchButtonMask = static_cast<int>(Qt::LeftButton);

QString touchPhaseLabel(QEvent::Type type)
{
    switch (type) {
    case QEvent::TouchBegin:
        return QStringLiteral("begin");
    case QEvent::TouchEnd:
        return QStringLiteral("end");
    case QEvent::TouchCancel:
        return QStringLiteral("cancel");
    default:
        return QStringLiteral("update");
    }
}

} // namespace

RuntimeEvents *RuntimeEvents::s_instance = nullptr;

RuntimeEvents::RuntimeEvents(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
    m_idleTimer.setInterval(250);
    m_idleTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_idleTimer, &QTimer::timeout, this, &RuntimeEvents::handleIdleTick);

    m_osTimer.setInterval(m_osSampleIntervalMs);
    m_osTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_osTimer, &QTimer::timeout, this, &RuntimeEvents::handleOsTick);

    m_stateSignalTimer.setSingleShot(true);
    m_stateSignalTimer.setInterval(m_runtimeStateSignalMinIntervalMs);
    m_stateSignalTimer.setTimerType(Qt::CoarseTimer);
    connect(&m_stateSignalTimer,
            &QTimer::timeout,
            this,
            [this]() {
                if (!m_runtimeStateSignalDirty)
                    return;
                m_runtimeStateSignalDirty = false;
                emit daemonStateChanged();
                emit eventLogChanged();
            });

    m_uptimeTimer.start();
    m_daemonBootEpochMs = nowEpochMs();
    m_lastActivityMonotonicMs = 0;
    m_lastActivityEpochMs = m_daemonBootEpochMs;

    if (qGuiApp) {
        m_applicationActive = qGuiApp->applicationState() == Qt::ApplicationActive;
        connect(qGuiApp,
                &QGuiApplication::applicationStateChanged,
                this,
                [this](Qt::ApplicationState state) {
                    const bool nextActive = state == Qt::ApplicationActive;
                    if (m_applicationActive != nextActive) {
                        m_applicationActive = nextActive;
                        emit osChanged();
                    }
                    emit osApplicationStateChanged(static_cast<int>(state));
                });
    }
}

RuntimeEvents::~RuntimeEvents()
{
    stop();
    if (s_instance == this)
        s_instance = nullptr;
}

RuntimeEvents *RuntimeEvents::instance()
{
    return s_instance;
}

bool RuntimeEvents::running() const
{
    return m_running;
}

quint64 RuntimeEvents::keyPressCount() const
{
    return m_keyPressCount;
}

quint64 RuntimeEvents::keyReleaseCount() const
{
    return m_keyReleaseCount;
}

int RuntimeEvents::lastKey() const
{
    return m_lastKey;
}

QString RuntimeEvents::lastKeyText() const
{
    return m_lastKeyText;
}

int RuntimeEvents::lastKeyModifiers() const
{
    return m_lastKeyModifiers;
}

bool RuntimeEvents::anyKeyPressed() const
{
    return !m_pressedKeys.isEmpty();
}

QStringList RuntimeEvents::pressedKeys() const
{
    QList<int> keys = m_pressedKeys.values();
    std::sort(keys.begin(), keys.end());
    QStringList names;
    names.reserve(keys.size());
    for (int key : keys)
        names.append(keyLabelForCode(key));
    return names;
}

QVariantList RuntimeEvents::pressedKeyCodes() const
{
    QList<int> keys = m_pressedKeys.values();
    std::sort(keys.begin(), keys.end());
    QVariantList codes;
    codes.reserve(keys.size());
    for (int key : keys)
        codes.append(key);
    return codes;
}

bool RuntimeEvents::isKeyPressed(int key) const
{
    return m_pressedKeys.contains(key);
}

quint64 RuntimeEvents::mouseMoveCount() const
{
    return m_mouseMoveCount;
}

quint64 RuntimeEvents::mousePressCount() const
{
    return m_mousePressCount;
}

quint64 RuntimeEvents::mouseReleaseCount() const
{
    return m_mouseReleaseCount;
}

qreal RuntimeEvents::lastMouseX() const
{
    return m_lastMouseX;
}

qreal RuntimeEvents::lastMouseY() const
{
    return m_lastMouseY;
}

int RuntimeEvents::lastMouseButtons() const
{
    return m_lastMouseButtons;
}

int RuntimeEvents::lastMouseModifiers() const
{
    return m_lastMouseModifiers;
}

bool RuntimeEvents::mouseButtonPressed() const
{
    return m_mouseButtonPressed;
}

QVariantMap RuntimeEvents::pointerUi() const
{
    if (m_pointerUi.isEmpty())
        return fallbackUiAt(m_lastMouseX, m_lastMouseY);
    return m_pointerUi;
}

qint64 RuntimeEvents::lastMousePressEpochMs() const
{
    return m_lastMousePressEpochMs;
}

qint64 RuntimeEvents::lastMouseReleaseEpochMs() const
{
    return m_lastMouseReleaseEpochMs;
}

qint64 RuntimeEvents::mousePressElapsedMs() const
{
    return elapsedSinceEpoch(m_lastMousePressEpochMs);
}

qint64 RuntimeEvents::mouseReleaseElapsedMs() const
{
    return elapsedSinceEpoch(m_lastMouseReleaseEpochMs);
}

qint64 RuntimeEvents::activePressDurationMs() const
{
    if (!m_mouseButtonPressed || m_lastMousePressEpochMs < 0)
        return 0;
    return elapsedSinceEpoch(m_lastMousePressEpochMs);
}

QStringList RuntimeEvents::pressedMouseButtonNames() const
{
    return mouseButtonNames(m_lastMouseButtons);
}

int RuntimeEvents::activeModifiers() const
{
    int modifiers = m_lastMouseModifiers | m_lastKeyModifiers;
    if (m_pressedKeys.contains(Qt::Key_Shift))
        modifiers |= Qt::ShiftModifier;
    if (m_pressedKeys.contains(Qt::Key_Control))
        modifiers |= Qt::ControlModifier;
    if (m_pressedKeys.contains(Qt::Key_Alt) || m_pressedKeys.contains(Qt::Key_AltGr))
        modifiers |= Qt::AltModifier;
    if (m_pressedKeys.contains(Qt::Key_Meta))
        modifiers |= Qt::MetaModifier;
    return modifiers;
}

QStringList RuntimeEvents::activeModifierNames() const
{
    return modifierNames(activeModifiers());
}

QVariantMap RuntimeEvents::inputState() const
{
    QVariantMap state;
    state.insert(QStringLiteral("pointerGlobalX"), m_lastMouseX);
    state.insert(QStringLiteral("pointerGlobalY"), m_lastMouseY);
    state.insert(QStringLiteral("mouseButtons"), m_lastMouseButtons);
    state.insert(QStringLiteral("mouseButtonPressed"), m_mouseButtonPressed);
    state.insert(QStringLiteral("pressedMouseButtons"), mouseButtonNames(m_lastMouseButtons));
    state.insert(QStringLiteral("lastMousePressEpochMs"), QVariant::fromValue(m_lastMousePressEpochMs));
    state.insert(QStringLiteral("lastMouseReleaseEpochMs"), QVariant::fromValue(m_lastMouseReleaseEpochMs));
    state.insert(QStringLiteral("mousePressElapsedMs"), QVariant::fromValue(mousePressElapsedMs()));
    state.insert(QStringLiteral("mouseReleaseElapsedMs"), QVariant::fromValue(mouseReleaseElapsedMs()));
    state.insert(QStringLiteral("activePressDurationMs"), QVariant::fromValue(activePressDurationMs()));
    state.insert(QStringLiteral("pointerUi"), pointerUi());
    state.insert(QStringLiteral("anyKeyPressed"), anyKeyPressed());
    state.insert(QStringLiteral("pressedKeys"), pressedKeys());
    state.insert(QStringLiteral("pressedKeyCodes"), pressedKeyCodes());
    state.insert(QStringLiteral("activeModifiers"), activeModifiers());
    state.insert(QStringLiteral("activeModifierNames"), activeModifierNames());
    state.insert(QStringLiteral("lastKey"), m_lastKey);
    state.insert(QStringLiteral("lastKeyText"), m_lastKeyText);
    state.insert(QStringLiteral("lastKeyModifiers"), m_lastKeyModifiers);
    return state;
}

quint64 RuntimeEvents::uiCreatedCount() const
{
    return m_uiCreatedCount;
}

quint64 RuntimeEvents::uiShownCount() const
{
    return m_uiShownCount;
}

quint64 RuntimeEvents::uiHiddenCount() const
{
    return m_uiHiddenCount;
}

quint64 RuntimeEvents::uiDestroyedCount() const
{
    return m_uiDestroyedCount;
}

QString RuntimeEvents::lastUiEvent() const
{
    return m_lastUiEvent;
}

QString RuntimeEvents::lastUiObjectName() const
{
    return m_lastUiObjectName;
}

QString RuntimeEvents::lastUiClassName() const
{
    return m_lastUiClassName;
}

bool RuntimeEvents::idle() const
{
    return m_idle;
}

int RuntimeEvents::idleTimeoutMs() const
{
    return m_idleTimeoutMs;
}

void RuntimeEvents::setIdleTimeoutMs(int value)
{
    const int next = qBound(250, value, 24 * 60 * 60 * 1000);
    if (m_idleTimeoutMs == next)
        return;
    m_idleTimeoutMs = next;
    emit idleTimeoutMsChanged();
}

qint64 RuntimeEvents::idleForMs() const
{
    return m_idleForMs;
}

qint64 RuntimeEvents::lastActivityEpochMs() const
{
    return m_lastActivityEpochMs;
}

qint64 RuntimeEvents::pid() const
{
    return static_cast<qint64>(QCoreApplication::applicationPid());
}

QString RuntimeEvents::processName() const
{
    const QString name = QCoreApplication::applicationName();
    if (!name.isEmpty())
        return name;
    return QStringLiteral("LVRSApp");
}

QString RuntimeEvents::osName() const
{
    return QSysInfo::prettyProductName();
}

qint64 RuntimeEvents::daemonBootEpochMs() const
{
    return m_daemonBootEpochMs;
}

quint64 RuntimeEvents::eventSequence() const
{
    return m_eventSequence;
}

QVariantMap RuntimeEvents::lastEvent() const
{
    return m_lastEvent;
}

int RuntimeEvents::recentEventCapacity() const
{
    return m_recentEventCapacity;
}

void RuntimeEvents::setRecentEventCapacity(int value)
{
    const int next = qBound(16, value, 4096);
    if (m_recentEventCapacity == next)
        return;
    m_recentEventCapacity = next;
    emit recentEventCapacityChanged();
    while (m_recentEvents.size() > static_cast<std::size_t>(m_recentEventCapacity))
        m_recentEvents.pop_front();
    emit eventLogChanged();
}

int RuntimeEvents::recentEventCount() const
{
    return static_cast<int>(m_recentEvents.size());
}

bool RuntimeEvents::applicationActive() const
{
    return m_applicationActive;
}

int RuntimeEvents::osSampleIntervalMs() const
{
    return m_osSampleIntervalMs;
}

void RuntimeEvents::setOsSampleIntervalMs(int value)
{
    const int next = qBound(250, value, 60 * 1000);
    if (m_osSampleIntervalMs == next)
        return;
    m_osSampleIntervalMs = next;
    if (m_osTimer.interval() != next)
        m_osTimer.setInterval(next);
    emit osSampleIntervalMsChanged();
}

qint64 RuntimeEvents::uptimeMs() const
{
    return m_uptimeMs;
}

qint64 RuntimeEvents::rssBytes() const
{
    return m_rssBytes;
}

int RuntimeEvents::captureProfile() const
{
    return m_captureProfile;
}

void RuntimeEvents::setCaptureProfile(int value)
{
    const int next = qBound(static_cast<int>(FullCapture),
                            value,
                            static_cast<int>(LowLatencyCapture));
    if (m_captureProfile == next)
        return;

    m_captureProfile = next;

    int nextHighFrequencyInterval = 16;
    int nextRuntimeSignalInterval = 16;
    bool nextPointerHitTestingEnabled = true;
    int nextPointerHitTestMinInterval = 0;
    bool nextUiTrackingEnabled = true;

    switch (static_cast<CaptureProfile>(m_captureProfile)) {
    case FullCapture:
        break;
    case BalancedCapture:
        nextHighFrequencyInterval = 24;
        nextRuntimeSignalInterval = 24;
        nextPointerHitTestMinInterval = 16;
        break;
    case LowLatencyCapture:
        nextHighFrequencyInterval = 33;
        nextRuntimeSignalInterval = 33;
        nextPointerHitTestingEnabled = false;
        nextUiTrackingEnabled = false;
        break;
    default:
        break;
    }

    setPointerHitTestingEnabled(nextPointerHitTestingEnabled);
    setPointerHitTestMinIntervalMs(nextPointerHitTestMinInterval);
    setUiTrackingEnabled(nextUiTrackingEnabled);

    m_highFrequencyEventMinIntervalMs = nextHighFrequencyInterval;
    m_runtimeStateSignalMinIntervalMs = nextRuntimeSignalInterval;
    if (m_stateSignalTimer.interval() != m_runtimeStateSignalMinIntervalMs)
        m_stateSignalTimer.setInterval(m_runtimeStateSignalMinIntervalMs);

    emit captureProfileChanged();
}

bool RuntimeEvents::pointerHitTestingEnabled() const
{
    return m_pointerHitTestingEnabled;
}

void RuntimeEvents::setPointerHitTestingEnabled(bool value)
{
    if (m_pointerHitTestingEnabled == value)
        return;
    m_pointerHitTestingEnabled = value;
    m_lastPointerHitTestEpochMs = -1;
    updatePointerUiSnapshot(true);
    emit pointerHitTestingEnabledChanged();
    emit mouseChanged();
}

int RuntimeEvents::pointerHitTestMinIntervalMs() const
{
    return m_pointerHitTestMinIntervalMs;
}

void RuntimeEvents::setPointerHitTestMinIntervalMs(int value)
{
    const int next = qBound(0, value, 1000);
    if (m_pointerHitTestMinIntervalMs == next)
        return;
    m_pointerHitTestMinIntervalMs = next;
    emit pointerHitTestMinIntervalMsChanged();
}

bool RuntimeEvents::uiTrackingEnabled() const
{
    return m_uiTrackingEnabled;
}

void RuntimeEvents::setUiTrackingEnabled(bool value)
{
    if (m_uiTrackingEnabled == value)
        return;
    m_uiTrackingEnabled = value;
    if (!m_uiTrackingEnabled) {
        detachTrackedObjects();
    } else if (m_window) {
        trackUiObjectRecursive(m_window);
    }
    emit uiTrackingEnabledChanged();
}

void RuntimeEvents::start()
{
    if (m_running)
        return;
    if (!qApp)
        return;

    qApp->installEventFilter(this);
    m_idleTimer.start();
    m_osTimer.start();
    m_running = true;
    handleOsTick();
    recordRuntimeEvent(QStringLiteral("daemon-started"));
    emit daemonStateChanged();
    emit runningChanged();
}

void RuntimeEvents::stop()
{
    if (!m_running)
        return;

    if (qApp)
        qApp->removeEventFilter(this);

    if (m_windowDestroyedConnection) {
        disconnect(m_windowDestroyedConnection);
        m_windowDestroyedConnection = QMetaObject::Connection();
    }
    m_window.clear();

    detachTrackedObjects();
    m_pointerUi = fallbackUiAt(m_lastMouseX, m_lastMouseY);

    m_idleTimer.stop();
    m_osTimer.stop();
    m_running = false;
    recordRuntimeEvent(QStringLiteral("daemon-stopped"));
    emit daemonStateChanged();
    emit runningChanged();
}

void RuntimeEvents::attachWindow(QObject *window)
{
    auto *quickWindow = qobject_cast<QQuickWindow *>(window);
    if (!quickWindow)
        return;

    start();

    if (m_window == quickWindow)
        return;

    if (m_windowDestroyedConnection) {
        disconnect(m_windowDestroyedConnection);
        m_windowDestroyedConnection = QMetaObject::Connection();
    }
    detachTrackedObjects();

    m_window = quickWindow;
    {
        QVariantMap payload;
        payload.insert(QStringLiteral("width"), quickWindow->width());
        payload.insert(QStringLiteral("height"), quickWindow->height());
        payload.insert(QStringLiteral("title"), quickWindow->title());
        recordRuntimeEvent(QStringLiteral("window-attached"), payload);
    }
    emit daemonStateChanged();
    m_windowDestroyedConnection = connect(m_window,
                                          &QObject::destroyed,
                                          this,
                                          [this]() {
                                              m_windowDestroyedConnection = QMetaObject::Connection();
                                              m_window.clear();
                                              detachTrackedObjects();
                                              m_pointerUi = fallbackUiAt(m_lastMouseX, m_lastMouseY);
                                              recordRuntimeEvent(QStringLiteral("window-detached"));
                                              emit daemonStateChanged();
                                          });

    if (m_uiTrackingEnabled)
        trackUiObjectRecursive(m_window);
    updatePointerUiSnapshot(true);
}

void RuntimeEvents::markActivity()
{
    const qint64 nowMonotonic = m_uptimeTimer.isValid() ? m_uptimeTimer.elapsed() : 0;
    m_lastActivityMonotonicMs = nowMonotonic;
    m_lastActivityEpochMs = nowEpochMs();
    emit lastActivityChanged();

    if (m_idle)
        updateIdleState(false);
}

void RuntimeEvents::resetCounters()
{
    m_keyPressCount = 0;
    m_keyReleaseCount = 0;
    m_lastKey = 0;
    m_lastKeyText.clear();
    m_lastKeyModifiers = 0;
    m_pressedKeys.clear();
    emit keyboardChanged();

    m_mouseMoveCount = 0;
    m_mousePressCount = 0;
    m_mouseReleaseCount = 0;
    m_lastMouseX = 0.0;
    m_lastMouseY = 0.0;
    m_lastMouseButtons = 0;
    m_lastMouseModifiers = 0;
    m_mouseButtonPressed = false;
    m_lastMousePressEpochMs = -1;
    m_lastMouseReleaseEpochMs = -1;
    m_lastPointerHitTestEpochMs = -1;
    m_pointerUi = fallbackUiAt(m_lastMouseX, m_lastMouseY);
    emit mouseChanged();

    m_uiCreatedCount = 0;
    m_uiShownCount = 0;
    m_uiHiddenCount = 0;
    m_uiDestroyedCount = 0;
    m_lastUiEvent.clear();
    m_lastUiObjectName.clear();
    m_lastUiClassName.clear();
    emit uiChanged();

    m_lastEventRecordedEpochByType.clear();

    m_idleForMs = 0;
    emit idleForMsChanged();
    markActivity();
    recordRuntimeEvent(QStringLiteral("counters-reset"));
}

QVariantMap RuntimeEvents::snapshot() const
{
    QVariantMap map;
    map.insert(QStringLiteral("running"), m_running);
    map.insert(QStringLiteral("keyPressCount"), QVariant::fromValue(m_keyPressCount));
    map.insert(QStringLiteral("keyReleaseCount"), QVariant::fromValue(m_keyReleaseCount));
    map.insert(QStringLiteral("mouseMoveCount"), QVariant::fromValue(m_mouseMoveCount));
    map.insert(QStringLiteral("mousePressCount"), QVariant::fromValue(m_mousePressCount));
    map.insert(QStringLiteral("mouseReleaseCount"), QVariant::fromValue(m_mouseReleaseCount));
    map.insert(QStringLiteral("uiCreatedCount"), QVariant::fromValue(m_uiCreatedCount));
    map.insert(QStringLiteral("uiShownCount"), QVariant::fromValue(m_uiShownCount));
    map.insert(QStringLiteral("uiHiddenCount"), QVariant::fromValue(m_uiHiddenCount));
    map.insert(QStringLiteral("uiDestroyedCount"), QVariant::fromValue(m_uiDestroyedCount));
    map.insert(QStringLiteral("idle"), m_idle);
    map.insert(QStringLiteral("idleForMs"), QVariant::fromValue(m_idleForMs));
    map.insert(QStringLiteral("pid"), pid());
    map.insert(QStringLiteral("rssBytes"), QVariant::fromValue(m_rssBytes));
    map.insert(QStringLiteral("uptimeMs"), QVariant::fromValue(m_uptimeMs));
    map.insert(QStringLiteral("daemonBootEpochMs"), QVariant::fromValue(m_daemonBootEpochMs));
    map.insert(QStringLiteral("eventSequence"), QVariant::fromValue(m_eventSequence));
    map.insert(QStringLiteral("recentEventCount"), QVariant::fromValue(m_recentEvents.size()));
    map.insert(QStringLiteral("lastEvent"), m_lastEvent);
    map.insert(QStringLiteral("input"), inputState());
    return map;
}

QVariantMap RuntimeEvents::daemonHealth() const
{
    QVariantMap map;
    map.insert(QStringLiteral("running"), m_running);
    map.insert(QStringLiteral("attachedWindow"), !m_window.isNull());
    map.insert(QStringLiteral("bootEpochMs"), QVariant::fromValue(m_daemonBootEpochMs));
    map.insert(QStringLiteral("eventSequence"), QVariant::fromValue(m_eventSequence));
    map.insert(QStringLiteral("recentEventCount"), QVariant::fromValue(m_recentEvents.size()));
    map.insert(QStringLiteral("recentEventCapacity"), QVariant::fromValue(m_recentEventCapacity));
    map.insert(QStringLiteral("idle"), m_idle);
    map.insert(QStringLiteral("idleForMs"), QVariant::fromValue(m_idleForMs));
    map.insert(QStringLiteral("pid"), pid());
    map.insert(QStringLiteral("lastEvent"), m_lastEvent);
    map.insert(QStringLiteral("input"), inputState());
    return map;
}

QVariantList RuntimeEvents::recentEvents() const
{
    QVariantList events;
    events.reserve(static_cast<int>(m_recentEvents.size()));
    for (const QVariantMap &entry : m_recentEvents)
        events.append(entry);
    return events;
}

void RuntimeEvents::clearRecentEvents()
{
    if (m_recentEvents.empty())
        return;
    m_recentEvents.clear();
    emit eventLogChanged();
}

QVariantMap RuntimeEvents::hitTestUiAt(qreal globalX, qreal globalY) const
{
    QVariantMap hit = describeQuickItemAtGlobal(globalX, globalY);
    if (!hit.isEmpty())
        return hit;
    return fallbackUiAt(globalX, globalY);
}

bool RuntimeEvents::eventFilter(QObject *watched, QEvent *event)
{
    if (!event)
        return QObject::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::KeyPress: {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        m_keyPressCount += 1;
        m_lastKey = keyEvent->key();
        m_lastKeyText = keyEvent->text();
        m_lastKeyModifiers = static_cast<int>(keyEvent->modifiers());
        m_pressedKeys.insert(keyEvent->key());
        emit keyboardChanged();
        emit keyPressed(keyEvent->key(),
                        static_cast<int>(keyEvent->modifiers()),
                        keyEvent->isAutoRepeat(),
                        keyEvent->text());
        {
            QVariantMap payload;
            payload.insert(QStringLiteral("key"), keyEvent->key());
            payload.insert(QStringLiteral("keyName"), keyLabelForCode(keyEvent->key()));
            payload.insert(QStringLiteral("modifiers"), static_cast<int>(keyEvent->modifiers()));
            payload.insert(QStringLiteral("autoRepeat"), keyEvent->isAutoRepeat());
            payload.insert(QStringLiteral("text"), keyEvent->text());
            payload.insert(QStringLiteral("anyKeyPressed"), anyKeyPressed());
            payload.insert(QStringLiteral("pressedKeys"), pressedKeys());
            payload.insert(QStringLiteral("pressedKeyCodes"), pressedKeyCodes());
            payload.insert(QStringLiteral("activeModifiers"), activeModifiers());
            payload.insert(QStringLiteral("activeModifierNames"), activeModifierNames());
            recordRuntimeEvent(QStringLiteral("key-press"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::KeyRelease: {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        m_keyReleaseCount += 1;
        m_lastKey = keyEvent->key();
        m_lastKeyText = keyEvent->text();
        m_lastKeyModifiers = static_cast<int>(keyEvent->modifiers());
        m_pressedKeys.remove(keyEvent->key());
        emit keyboardChanged();
        emit keyReleased(keyEvent->key(),
                         static_cast<int>(keyEvent->modifiers()),
                         keyEvent->isAutoRepeat(),
                         keyEvent->text());
        {
            QVariantMap payload;
            payload.insert(QStringLiteral("key"), keyEvent->key());
            payload.insert(QStringLiteral("keyName"), keyLabelForCode(keyEvent->key()));
            payload.insert(QStringLiteral("modifiers"), static_cast<int>(keyEvent->modifiers()));
            payload.insert(QStringLiteral("autoRepeat"), keyEvent->isAutoRepeat());
            payload.insert(QStringLiteral("text"), keyEvent->text());
            payload.insert(QStringLiteral("anyKeyPressed"), anyKeyPressed());
            payload.insert(QStringLiteral("pressedKeys"), pressedKeys());
            payload.insert(QStringLiteral("pressedKeyCodes"), pressedKeyCodes());
            payload.insert(QStringLiteral("activeModifiers"), activeModifiers());
            payload.insert(QStringLiteral("activeModifierNames"), activeModifierNames());
            recordRuntimeEvent(QStringLiteral("key-release"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::MouseMove: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const qint64 epochMs = nowEpochMs();
        const bool shouldRecordPayload = shouldSampleHighFrequencyPayload(QStringLiteral("mouse-move"), epochMs);
        m_mouseMoveCount += 1;
        updateMouseFromEvent(mouseEvent->globalPosition().x(),
                             mouseEvent->globalPosition().y(),
                             static_cast<int>(mouseEvent->buttons()),
                             static_cast<int>(mouseEvent->modifiers()),
                             shouldRecordPayload);
        emit mouseChanged();
        emit mouseMoved(m_lastMouseX, m_lastMouseY, m_lastMouseButtons, m_lastMouseModifiers);
        if (shouldRecordPayload) {
            QVariantMap payload;
            payload.insert(QStringLiteral("x"), m_lastMouseX);
            payload.insert(QStringLiteral("y"), m_lastMouseY);
            payload.insert(QStringLiteral("buttons"), m_lastMouseButtons);
            payload.insert(QStringLiteral("pressedMouseButtons"), pressedMouseButtonNames());
            payload.insert(QStringLiteral("modifiers"), m_lastMouseModifiers);
            payload.insert(QStringLiteral("activeModifiers"), activeModifiers());
            payload.insert(QStringLiteral("activeModifierNames"), activeModifierNames());
            payload.insert(QStringLiteral("mouseButtonPressed"), m_mouseButtonPressed);
            const QVariantMap pointer = pointerUi();
            payload.insert(QStringLiteral("pointerUi"), pointer);
            payload.insert(QStringLiteral("pointerObjectName"), pointer.value(QStringLiteral("objectName")));
            payload.insert(QStringLiteral("pointerClassName"), pointer.value(QStringLiteral("className")));
            payload.insert(QStringLiteral("pointerPath"), pointer.value(QStringLiteral("path")));
            recordRuntimeEvent(QStringLiteral("mouse-move"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::MouseButtonPress: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        m_lastMousePressEpochMs = nowEpochMs();
        m_mousePressCount += 1;
        m_mouseButtonPressed = true;
        updateMouseFromEvent(mouseEvent->globalPosition().x(),
                             mouseEvent->globalPosition().y(),
                             static_cast<int>(mouseEvent->buttons()),
                             static_cast<int>(mouseEvent->modifiers()));
        emit mouseChanged();
        emit mousePressed(m_lastMouseX, m_lastMouseY, m_lastMouseButtons, m_lastMouseModifiers);
        {
            QVariantMap payload;
            payload.insert(QStringLiteral("x"), m_lastMouseX);
            payload.insert(QStringLiteral("y"), m_lastMouseY);
            payload.insert(QStringLiteral("buttons"), m_lastMouseButtons);
            payload.insert(QStringLiteral("pressedMouseButtons"), pressedMouseButtonNames());
            payload.insert(QStringLiteral("modifiers"), m_lastMouseModifiers);
            payload.insert(QStringLiteral("button"), static_cast<int>(mouseEvent->button()));
            payload.insert(QStringLiteral("mouseButtonPressed"), m_mouseButtonPressed);
            payload.insert(QStringLiteral("lastMousePressEpochMs"), QVariant::fromValue(m_lastMousePressEpochMs));
            payload.insert(QStringLiteral("mousePressElapsedMs"), QVariant::fromValue(mousePressElapsedMs()));
            payload.insert(QStringLiteral("activePressDurationMs"), QVariant::fromValue(activePressDurationMs()));
            const QVariantMap pointer = pointerUi();
            payload.insert(QStringLiteral("pointerUi"), pointer);
            payload.insert(QStringLiteral("pointerObjectName"), pointer.value(QStringLiteral("objectName")));
            payload.insert(QStringLiteral("pointerClassName"), pointer.value(QStringLiteral("className")));
            payload.insert(QStringLiteral("pointerPath"), pointer.value(QStringLiteral("path")));
            recordRuntimeEvent(QStringLiteral("mouse-press"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::MouseButtonRelease: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        m_lastMouseReleaseEpochMs = nowEpochMs();
        m_mouseReleaseCount += 1;
        m_mouseButtonPressed = false;
        updateMouseFromEvent(mouseEvent->globalPosition().x(),
                             mouseEvent->globalPosition().y(),
                             static_cast<int>(mouseEvent->buttons()),
                             static_cast<int>(mouseEvent->modifiers()));
        emit mouseChanged();
        emit mouseReleased(m_lastMouseX, m_lastMouseY, m_lastMouseButtons, m_lastMouseModifiers);
        {
            QVariantMap payload;
            payload.insert(QStringLiteral("x"), m_lastMouseX);
            payload.insert(QStringLiteral("y"), m_lastMouseY);
            payload.insert(QStringLiteral("buttons"), m_lastMouseButtons);
            payload.insert(QStringLiteral("pressedMouseButtons"), pressedMouseButtonNames());
            payload.insert(QStringLiteral("modifiers"), m_lastMouseModifiers);
            payload.insert(QStringLiteral("button"), static_cast<int>(mouseEvent->button()));
            payload.insert(QStringLiteral("mouseButtonPressed"), m_mouseButtonPressed);
            payload.insert(QStringLiteral("lastMouseReleaseEpochMs"), QVariant::fromValue(m_lastMouseReleaseEpochMs));
            payload.insert(QStringLiteral("mouseReleaseElapsedMs"), QVariant::fromValue(mouseReleaseElapsedMs()));
            const QVariantMap pointer = pointerUi();
            payload.insert(QStringLiteral("pointerUi"), pointer);
            payload.insert(QStringLiteral("pointerObjectName"), pointer.value(QStringLiteral("objectName")));
            payload.insert(QStringLiteral("pointerClassName"), pointer.value(QStringLiteral("className")));
            payload.insert(QStringLiteral("pointerPath"), pointer.value(QStringLiteral("path")));
            recordRuntimeEvent(QStringLiteral("mouse-release"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::MouseButtonDblClick: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        m_lastMousePressEpochMs = nowEpochMs();
        m_mouseButtonPressed = true;
        updateMouseFromEvent(mouseEvent->globalPosition().x(),
                             mouseEvent->globalPosition().y(),
                             static_cast<int>(mouseEvent->buttons()),
                             static_cast<int>(mouseEvent->modifiers()));
        emit mouseChanged();
        emit mousePressed(m_lastMouseX, m_lastMouseY, m_lastMouseButtons, m_lastMouseModifiers);
        {
            QVariantMap payload;
            payload.insert(QStringLiteral("x"), m_lastMouseX);
            payload.insert(QStringLiteral("y"), m_lastMouseY);
            payload.insert(QStringLiteral("buttons"), m_lastMouseButtons);
            payload.insert(QStringLiteral("pressedMouseButtons"), pressedMouseButtonNames());
            payload.insert(QStringLiteral("modifiers"), m_lastMouseModifiers);
            payload.insert(QStringLiteral("button"), static_cast<int>(mouseEvent->button()));
            payload.insert(QStringLiteral("mouseButtonPressed"), m_mouseButtonPressed);
            payload.insert(QStringLiteral("doubleClick"), true);
            payload.insert(QStringLiteral("lastMousePressEpochMs"), QVariant::fromValue(m_lastMousePressEpochMs));
            payload.insert(QStringLiteral("mousePressElapsedMs"), QVariant::fromValue(mousePressElapsedMs()));
            payload.insert(QStringLiteral("activePressDurationMs"), QVariant::fromValue(activePressDurationMs()));
            const QVariantMap pointer = pointerUi();
            payload.insert(QStringLiteral("pointerUi"), pointer);
            payload.insert(QStringLiteral("pointerObjectName"), pointer.value(QStringLiteral("objectName")));
            payload.insert(QStringLiteral("pointerClassName"), pointer.value(QStringLiteral("className")));
            payload.insert(QStringLiteral("pointerPath"), pointer.value(QStringLiteral("path")));
            recordRuntimeEvent(QStringLiteral("mouse-double-click"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::Wheel: {
        auto *wheelEvent = static_cast<QWheelEvent *>(event);
        updateMouseFromEvent(wheelEvent->globalPosition().x(),
                             wheelEvent->globalPosition().y(),
                             m_lastMouseButtons,
                             static_cast<int>(wheelEvent->modifiers()));
        emit mouseChanged();
        {
            QVariantMap payload;
            payload.insert(QStringLiteral("x"), m_lastMouseX);
            payload.insert(QStringLiteral("y"), m_lastMouseY);
            payload.insert(QStringLiteral("buttons"), m_lastMouseButtons);
            payload.insert(QStringLiteral("pressedMouseButtons"), pressedMouseButtonNames());
            payload.insert(QStringLiteral("modifiers"), m_lastMouseModifiers);
            payload.insert(QStringLiteral("angleDeltaX"), wheelEvent->angleDelta().x());
            payload.insert(QStringLiteral("angleDeltaY"), wheelEvent->angleDelta().y());
            payload.insert(QStringLiteral("pixelDeltaX"), wheelEvent->pixelDelta().x());
            payload.insert(QStringLiteral("pixelDeltaY"), wheelEvent->pixelDelta().y());
            payload.insert(QStringLiteral("phase"), static_cast<int>(wheelEvent->phase()));
            payload.insert(QStringLiteral("inverted"), wheelEvent->inverted());
            payload.insert(QStringLiteral("mouseButtonPressed"), m_mouseButtonPressed);
            const QVariantMap pointer = pointerUi();
            payload.insert(QStringLiteral("pointerUi"), pointer);
            payload.insert(QStringLiteral("pointerObjectName"), pointer.value(QStringLiteral("objectName")));
            payload.insert(QStringLiteral("pointerClassName"), pointer.value(QStringLiteral("className")));
            payload.insert(QStringLiteral("pointerPath"), pointer.value(QStringLiteral("path")));
            recordRuntimeEvent(QStringLiteral("mouse-wheel"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel: {
        auto *touchEvent = static_cast<QTouchEvent *>(event);
        const QEvent::Type touchType = event->type();
        QVariantList points;
        points.reserve(touchEvent->points().size());
        for (const QEventPoint &point : touchEvent->points()) {
            QVariantMap pointMap;
            pointMap.insert(QStringLiteral("id"), point.id());
            pointMap.insert(QStringLiteral("state"), static_cast<int>(point.state()));
            pointMap.insert(QStringLiteral("positionX"), point.position().x());
            pointMap.insert(QStringLiteral("positionY"), point.position().y());
            pointMap.insert(QStringLiteral("globalX"), point.globalPosition().x());
            pointMap.insert(QStringLiteral("globalY"), point.globalPosition().y());
            pointMap.insert(QStringLiteral("pressure"), point.pressure());
            points.append(pointMap);
        }

        const bool hadActivePress = m_mouseButtonPressed
                                    || m_lastMouseButtons != static_cast<int>(Qt::NoButton);
        const QEventPoint *trackedPoint = nullptr;
        for (const QEventPoint &point : touchEvent->points()) {
            if (point.state() != QEventPoint::Released) {
                trackedPoint = &point;
                break;
            }
        }
        if (!trackedPoint && !touchEvent->points().isEmpty())
            trackedPoint = &touchEvent->points().constFirst();

        const bool hasActiveTouchPoint = trackedPoint
                                         && touchType != QEvent::TouchEnd
                                         && touchType != QEvent::TouchCancel
                                         && trackedPoint->state() != QEventPoint::Released;
        const bool pressedTransition = hasActiveTouchPoint && !hadActivePress;
        const bool releasedTransition = !hasActiveTouchPoint && hadActivePress;
        const int normalizedButtons = hasActiveTouchPoint
                                      ? kPrimaryTouchButtonMask
                                      : static_cast<int>(Qt::NoButton);

        if (pressedTransition) {
            m_lastMousePressEpochMs = nowEpochMs();
            m_mousePressCount += 1;
            m_mouseButtonPressed = true;
        } else if (releasedTransition) {
            m_lastMouseReleaseEpochMs = nowEpochMs();
            m_mouseReleaseCount += 1;
            m_mouseButtonPressed = false;
        } else if (hasActiveTouchPoint) {
            m_mouseMoveCount += 1;
            m_mouseButtonPressed = true;
        } else {
            m_mouseButtonPressed = false;
        }

        if (trackedPoint) {
            updateMouseFromEvent(trackedPoint->globalPosition().x(),
                                 trackedPoint->globalPosition().y(),
                                 normalizedButtons,
                                 static_cast<int>(touchEvent->modifiers()));
        } else {
            updateMouseFromEvent(m_lastMouseX,
                                 m_lastMouseY,
                                 normalizedButtons,
                                 static_cast<int>(touchEvent->modifiers()));
        }
        emit mouseChanged();
        if (pressedTransition)
            emit mousePressed(m_lastMouseX, m_lastMouseY, m_lastMouseButtons, m_lastMouseModifiers);
        else if (releasedTransition)
            emit mouseReleased(m_lastMouseX, m_lastMouseY, m_lastMouseButtons, m_lastMouseModifiers);
        else if (hasActiveTouchPoint)
            emit mouseMoved(m_lastMouseX, m_lastMouseY, m_lastMouseButtons, m_lastMouseModifiers);

        {
            QVariantMap payload;
            payload.insert(QStringLiteral("phase"), touchPhaseLabel(touchType));
            payload.insert(QStringLiteral("pointCount"), touchEvent->points().size());
            payload.insert(QStringLiteral("x"), m_lastMouseX);
            payload.insert(QStringLiteral("y"), m_lastMouseY);
            payload.insert(QStringLiteral("buttons"), m_lastMouseButtons);
            payload.insert(QStringLiteral("pressedMouseButtons"), pressedMouseButtonNames());
            payload.insert(QStringLiteral("modifiers"), m_lastMouseModifiers);
            payload.insert(QStringLiteral("mouseButtonPressed"), m_mouseButtonPressed);
            payload.insert(QStringLiteral("lastMousePressEpochMs"), QVariant::fromValue(m_lastMousePressEpochMs));
            payload.insert(QStringLiteral("lastMouseReleaseEpochMs"), QVariant::fromValue(m_lastMouseReleaseEpochMs));
            payload.insert(QStringLiteral("mousePressElapsedMs"), QVariant::fromValue(mousePressElapsedMs()));
            payload.insert(QStringLiteral("mouseReleaseElapsedMs"), QVariant::fromValue(mouseReleaseElapsedMs()));
            payload.insert(QStringLiteral("activePressDurationMs"), QVariant::fromValue(activePressDurationMs()));
            payload.insert(QStringLiteral("points"), points);
            const QVariantMap pointer = pointerUi();
            payload.insert(QStringLiteral("pointerUi"), pointer);
            payload.insert(QStringLiteral("pointerObjectName"), pointer.value(QStringLiteral("objectName")));
            payload.insert(QStringLiteral("pointerClassName"), pointer.value(QStringLiteral("className")));
            payload.insert(QStringLiteral("pointerPath"), pointer.value(QStringLiteral("path")));
            recordRuntimeEvent(QStringLiteral("touch-event"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease: {
        auto *tabletEvent = static_cast<QTabletEvent *>(event);
        if (event->type() == QEvent::TabletPress) {
            m_lastMousePressEpochMs = nowEpochMs();
            m_mouseButtonPressed = true;
        } else if (event->type() == QEvent::TabletRelease) {
            m_lastMouseReleaseEpochMs = nowEpochMs();
            m_mouseButtonPressed = false;
        }
        updateMouseFromEvent(tabletEvent->globalPosition().x(),
                             tabletEvent->globalPosition().y(),
                             static_cast<int>(tabletEvent->buttons()),
                             static_cast<int>(tabletEvent->modifiers()));
        emit mouseChanged();
        {
            QString phase = QStringLiteral("move");
            if (event->type() == QEvent::TabletPress)
                phase = QStringLiteral("press");
            else if (event->type() == QEvent::TabletRelease)
                phase = QStringLiteral("release");

            QVariantMap payload;
            payload.insert(QStringLiteral("phase"), phase);
            payload.insert(QStringLiteral("x"), m_lastMouseX);
            payload.insert(QStringLiteral("y"), m_lastMouseY);
            payload.insert(QStringLiteral("buttons"), m_lastMouseButtons);
            payload.insert(QStringLiteral("pressedMouseButtons"), pressedMouseButtonNames());
            payload.insert(QStringLiteral("modifiers"), m_lastMouseModifiers);
            payload.insert(QStringLiteral("button"), static_cast<int>(tabletEvent->button()));
            payload.insert(QStringLiteral("pressure"), tabletEvent->pressure());
            payload.insert(QStringLiteral("tangentialPressure"), tabletEvent->tangentialPressure());
            payload.insert(QStringLiteral("rotation"), tabletEvent->rotation());
            payload.insert(QStringLiteral("xTilt"), tabletEvent->xTilt());
            payload.insert(QStringLiteral("yTilt"), tabletEvent->yTilt());
            payload.insert(QStringLiteral("z"), tabletEvent->z());
            payload.insert(QStringLiteral("pointerType"), static_cast<int>(tabletEvent->pointerType()));
            payload.insert(QStringLiteral("mouseButtonPressed"), m_mouseButtonPressed);
            payload.insert(QStringLiteral("lastMousePressEpochMs"), QVariant::fromValue(m_lastMousePressEpochMs));
            payload.insert(QStringLiteral("lastMouseReleaseEpochMs"), QVariant::fromValue(m_lastMouseReleaseEpochMs));
            const QVariantMap pointer = pointerUi();
            payload.insert(QStringLiteral("pointerUi"), pointer);
            payload.insert(QStringLiteral("pointerObjectName"), pointer.value(QStringLiteral("objectName")));
            payload.insert(QStringLiteral("pointerClassName"), pointer.value(QStringLiteral("className")));
            payload.insert(QStringLiteral("pointerPath"), pointer.value(QStringLiteral("path")));
            recordRuntimeEvent(QStringLiteral("tablet-event"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::TabletEnterProximity:
    case QEvent::TabletLeaveProximity: {
        QVariantMap payload;
        payload.insert(QStringLiteral("phase"),
                       event->type() == QEvent::TabletEnterProximity
                       ? QStringLiteral("enter-proximity")
                       : QStringLiteral("leave-proximity"));
        recordRuntimeEvent(QStringLiteral("tablet-proximity"), payload);
        markActivity();
        break;
    }
    case QEvent::NativeGesture: {
        auto *gestureEvent = static_cast<QNativeGestureEvent *>(event);
        updateMouseFromEvent(gestureEvent->globalPosition().x(),
                             gestureEvent->globalPosition().y(),
                             m_lastMouseButtons,
                             static_cast<int>(gestureEvent->modifiers()));
        emit mouseChanged();
        {
            QVariantMap payload;
            payload.insert(QStringLiteral("x"), m_lastMouseX);
            payload.insert(QStringLiteral("y"), m_lastMouseY);
            payload.insert(QStringLiteral("buttons"), m_lastMouseButtons);
            payload.insert(QStringLiteral("pressedMouseButtons"), pressedMouseButtonNames());
            payload.insert(QStringLiteral("modifiers"), m_lastMouseModifiers);
            payload.insert(QStringLiteral("gestureType"), static_cast<int>(gestureEvent->gestureType()));
            payload.insert(QStringLiteral("value"), gestureEvent->value());
            payload.insert(QStringLiteral("pointerUi"), pointerUi());
            recordRuntimeEvent(QStringLiteral("native-gesture"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::ContextMenu: {
        auto *contextEvent = static_cast<QContextMenuEvent *>(event);
        const int modifiers = static_cast<int>(contextEvent->modifiers());
        const int buttons = contextEvent->reason() == QContextMenuEvent::Mouse
            ? static_cast<int>(Qt::RightButton)
            : 0;
        updateMouseFromEvent(static_cast<qreal>(contextEvent->globalPos().x()),
                             static_cast<qreal>(contextEvent->globalPos().y()),
                             buttons,
                             modifiers);
        emit mouseChanged();
        emit contextRequested(m_lastMouseX,
                              m_lastMouseY,
                              m_lastMouseModifiers,
                              static_cast<int>(contextEvent->reason()));
        {
            QVariantMap payload;
            payload.insert(QStringLiteral("x"), m_lastMouseX);
            payload.insert(QStringLiteral("y"), m_lastMouseY);
            payload.insert(QStringLiteral("modifiers"), m_lastMouseModifiers);
            payload.insert(QStringLiteral("buttons"), buttons);
            payload.insert(QStringLiteral("pressedMouseButtons"), pressedMouseButtonNames());
            payload.insert(QStringLiteral("reason"), static_cast<int>(contextEvent->reason()));
            const QVariantMap pointer = pointerUi();
            payload.insert(QStringLiteral("pointerUi"), pointer);
            payload.insert(QStringLiteral("pointerObjectName"), pointer.value(QStringLiteral("objectName")));
            payload.insert(QStringLiteral("pointerClassName"), pointer.value(QStringLiteral("className")));
            payload.insert(QStringLiteral("pointerPath"), pointer.value(QStringLiteral("path")));
            recordRuntimeEvent(QStringLiteral("context-requested"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::HoverMove: {
        auto *hoverEvent = static_cast<QHoverEvent *>(event);
        const qint64 epochMs = nowEpochMs();
        const bool shouldRecordPayload = shouldSampleHighFrequencyPayload(QStringLiteral("hover-move"), epochMs);
        m_mouseMoveCount += 1;
        updateMouseFromEvent(hoverEvent->globalPosition().x(),
                             hoverEvent->globalPosition().y(),
                             m_lastMouseButtons,
                             m_lastMouseModifiers,
                             shouldRecordPayload);
        emit mouseChanged();
        emit mouseMoved(m_lastMouseX, m_lastMouseY, m_lastMouseButtons, m_lastMouseModifiers);
        if (shouldRecordPayload) {
            QVariantMap payload;
            payload.insert(QStringLiteral("x"), m_lastMouseX);
            payload.insert(QStringLiteral("y"), m_lastMouseY);
            payload.insert(QStringLiteral("buttons"), m_lastMouseButtons);
            payload.insert(QStringLiteral("pressedMouseButtons"), pressedMouseButtonNames());
            payload.insert(QStringLiteral("modifiers"), m_lastMouseModifiers);
            payload.insert(QStringLiteral("mouseButtonPressed"), m_mouseButtonPressed);
            const QVariantMap pointer = pointerUi();
            payload.insert(QStringLiteral("pointerUi"), pointer);
            payload.insert(QStringLiteral("pointerObjectName"), pointer.value(QStringLiteral("objectName")));
            payload.insert(QStringLiteral("pointerClassName"), pointer.value(QStringLiteral("className")));
            payload.insert(QStringLiteral("pointerPath"), pointer.value(QStringLiteral("path")));
            recordRuntimeEvent(QStringLiteral("hover-move"), payload);
        }
        markActivity();
        break;
    }
    case QEvent::ChildAdded: {
        auto *childEvent = static_cast<QChildEvent *>(event);
        const bool withinTrackedTree = watched == m_window || m_trackedObjects.contains(watched);
        if (m_uiTrackingEnabled && withinTrackedTree && childEvent->added() && childEvent->child())
            trackUiObjectRecursive(childEvent->child());
        break;
    }
    case QEvent::Show:
    case QEvent::ShowToParent:
        recordUiEvent(QStringLiteral("shown"), watched, true);
        break;
    case QEvent::Hide:
    case QEvent::HideToParent:
        recordUiEvent(QStringLiteral("hidden"), watched, false);
        break;
    default:
        break;
    }

    return QObject::eventFilter(watched, event);
}

void RuntimeEvents::trackUiObjectRecursive(QObject *object)
{
    if (!m_uiTrackingEnabled || !object || m_trackedObjects.contains(object))
        return;

    m_trackedObjects.insert(object);

    UiObjectInfo info;
    info.objectName = object->objectName();
    if (info.objectName.isEmpty())
        info.objectName = QStringLiteral("unnamed");
    info.className = QString::fromLatin1(object->metaObject()->className());
    const QVariant visibleProp = object->property("visible");
    info.visible = visibleProp.isValid() ? visibleProp.toBool() : true;
    m_trackedInfo.insert(object, info);

    connect(object,
            &QObject::destroyed,
            this,
            [this, object]() {
                handleTrackedDestroyed(object);
            });

    recordUiEvent(QStringLiteral("created"), object, info.visible);

    const auto children = object->children();
    for (QObject *child : children)
        trackUiObjectRecursive(child);
}

void RuntimeEvents::detachTrackedObjects()
{
    m_trackedObjects.clear();
    m_trackedInfo.clear();
}

void RuntimeEvents::handleTrackedDestroyed(const QObject *object)
{
    if (!m_uiTrackingEnabled || !m_trackedInfo.contains(object))
        return;

    const UiObjectInfo info = m_trackedInfo.take(object);
    m_trackedObjects.remove(const_cast<QObject *>(object));
    m_uiDestroyedCount += 1;
    m_lastUiEvent = QStringLiteral("destroyed");
    m_lastUiObjectName = info.objectName;
    m_lastUiClassName = info.className;
    emit uiChanged();
    emit uiEvent(m_lastUiEvent, m_lastUiObjectName, m_lastUiClassName, false);

    QVariantMap payload;
    payload.insert(QStringLiteral("eventType"), m_lastUiEvent);
    payload.insert(QStringLiteral("objectName"), m_lastUiObjectName);
    payload.insert(QStringLiteral("className"), m_lastUiClassName);
    payload.insert(QStringLiteral("visible"), false);
    recordRuntimeEvent(QStringLiteral("ui-event"), payload);
}

void RuntimeEvents::recordUiEvent(const QString &eventType, const QObject *object, bool visible)
{
    if (!m_uiTrackingEnabled || !object || !m_trackedInfo.contains(object))
        return;

    UiObjectInfo &info = m_trackedInfo[object];
    info.visible = visible;

    if (eventType == QLatin1String("created"))
        m_uiCreatedCount += 1;
    else if (eventType == QLatin1String("shown"))
        m_uiShownCount += 1;
    else if (eventType == QLatin1String("hidden"))
        m_uiHiddenCount += 1;

    m_lastUiEvent = eventType;
    m_lastUiObjectName = info.objectName;
    m_lastUiClassName = info.className;
    emit uiChanged();
    emit uiEvent(eventType, info.objectName, info.className, visible);

    QVariantMap payload;
    payload.insert(QStringLiteral("eventType"), eventType);
    payload.insert(QStringLiteral("objectName"), info.objectName);
    payload.insert(QStringLiteral("className"), info.className);
    payload.insert(QStringLiteral("visible"), visible);
    recordRuntimeEvent(QStringLiteral("ui-event"), payload);
}

void RuntimeEvents::updateMouseFromEvent(qreal x, qreal y, int buttons, int modifiers, bool refreshPointerUi)
{
    m_lastMouseX = x;
    m_lastMouseY = y;
    m_lastMouseButtons = buttons;
    m_lastMouseModifiers = modifiers;
    if (refreshPointerUi)
        updatePointerUiSnapshot();
}

void RuntimeEvents::updatePointerUiSnapshot(bool forceRefresh)
{
    if (!m_pointerHitTestingEnabled) {
        m_pointerUi = fallbackUiAt(m_lastMouseX, m_lastMouseY);
        return;
    }

    const qint64 epochMs = nowEpochMs();
    if (!forceRefresh
        && m_pointerHitTestMinIntervalMs > 0
        && m_lastPointerHitTestEpochMs >= 0
        && (epochMs - m_lastPointerHitTestEpochMs) < m_pointerHitTestMinIntervalMs) {
        return;
    }

    m_lastPointerHitTestEpochMs = epochMs;
    QVariantMap hit = describeQuickItemAtGlobal(m_lastMouseX, m_lastMouseY);
    m_pointerUi = hit.isEmpty() ? fallbackUiAt(m_lastMouseX, m_lastMouseY) : hit;
}

void RuntimeEvents::handleIdleTick()
{
    const qint64 nowMs = m_uptimeTimer.elapsed();
    const qint64 idleFor = qMax<qint64>(0, nowMs - m_lastActivityMonotonicMs);
    if (m_idleForMs != idleFor) {
        m_idleForMs = idleFor;
        emit idleForMsChanged();
    }

    if (!m_idle && m_idleForMs >= m_idleTimeoutMs)
        updateIdleState(true);
}

void RuntimeEvents::handleOsTick()
{
    const qint64 nextUptime = m_uptimeTimer.elapsed();
    const qint64 nextRss = sampleResidentSetBytes();

    bool changed = false;
    if (m_uptimeMs != nextUptime) {
        m_uptimeMs = nextUptime;
        changed = true;
    }
    if (m_rssBytes != nextRss) {
        m_rssBytes = nextRss;
        changed = true;
    }

    if (qGuiApp) {
        const bool nextActive = qGuiApp->applicationState() == Qt::ApplicationActive;
        if (m_applicationActive != nextActive) {
            m_applicationActive = nextActive;
            emit osChanged();
        }
    }

    if (changed)
        emit osStatsChanged();

    emit daemonHeartbeat(nowEpochMs(), m_uptimeMs, m_eventSequence);
}

void RuntimeEvents::updateIdleState(bool nextIdle)
{
    if (m_idle == nextIdle)
        return;
    m_idle = nextIdle;
    emit idleChanged();
    if (m_idle)
        emit idleEntered();
    else
        emit idleExited();
}

qint64 RuntimeEvents::nowEpochMs() const
{
    return QDateTime::currentMSecsSinceEpoch();
}

qint64 RuntimeEvents::elapsedSinceEpoch(qint64 epochMs) const
{
    if (epochMs < 0)
        return -1;
    return qMax<qint64>(0, nowEpochMs() - epochMs);
}

qint64 RuntimeEvents::sampleResidentSetBytes() const
{
#ifdef Q_OS_MAC
    mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
        return static_cast<qint64>(info.resident_size);
    return -1;
#elif defined(Q_OS_LINUX)
    QFile file(QStringLiteral("/proc/self/statm"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return -1;
    const QByteArray line = file.readLine().trimmed();
    const QList<QByteArray> parts = line.split(' ');
    if (parts.size() < 2)
        return -1;
    bool ok = false;
    const qint64 residentPages = parts.at(1).toLongLong(&ok);
    if (!ok)
        return -1;
    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0)
        return -1;
    return residentPages * static_cast<qint64>(pageSize);
#else
    return -1;
#endif
}

bool RuntimeEvents::isHighFrequencyEventType(const QString &eventType) const
{
    return eventType == QLatin1String("mouse-move")
        || eventType == QLatin1String("hover-move");
}

bool RuntimeEvents::shouldSampleHighFrequencyPayload(const QString &eventType, qint64 epochMs) const
{
    if (m_highFrequencyEventMinIntervalMs <= 0)
        return true;
    if (!isHighFrequencyEventType(eventType))
        return true;

    const qint64 lastEpochMs = m_lastEventRecordedEpochByType.value(eventType, -1);
    return !(lastEpochMs >= 0 && (epochMs - lastEpochMs) < m_highFrequencyEventMinIntervalMs);
}

bool RuntimeEvents::shouldSkipHighFrequencyRecord(const QString &eventType, qint64 epochMs)
{
    if (m_highFrequencyEventMinIntervalMs <= 0)
        return false;
    if (!isHighFrequencyEventType(eventType))
        return false;

    const qint64 lastEpochMs = m_lastEventRecordedEpochByType.value(eventType, -1);
    if (lastEpochMs >= 0 && (epochMs - lastEpochMs) < m_highFrequencyEventMinIntervalMs)
        return true;

    m_lastEventRecordedEpochByType.insert(eventType, epochMs);
    return false;
}

void RuntimeEvents::scheduleRuntimeStateSignals()
{
    m_runtimeStateSignalDirty = true;
    if (!m_stateSignalTimer.isActive())
        m_stateSignalTimer.start();
}

void RuntimeEvents::recordRuntimeEvent(const QString &eventType, const QVariantMap &payload)
{
    const qint64 epochMs = nowEpochMs();
    if (shouldSkipHighFrequencyRecord(eventType, epochMs))
        return;

    QVariantMap eventData;
    m_eventSequence += 1;
    eventData.insert(QStringLiteral("sequence"), QVariant::fromValue(m_eventSequence));
    eventData.insert(QStringLiteral("type"), eventType);
    eventData.insert(QStringLiteral("timestampEpochMs"), QVariant::fromValue(epochMs));
    eventData.insert(QStringLiteral("uptimeMs"),
                     QVariant::fromValue(m_uptimeTimer.isValid() ? m_uptimeTimer.elapsed() : 0));
    if (!payload.isEmpty())
        eventData.insert(QStringLiteral("payload"), payload);

    m_lastEvent = eventData;
    pushRecentEvent(eventData);
    emit eventRecorded(eventData);
    scheduleRuntimeStateSignals();
}

void RuntimeEvents::pushRecentEvent(const QVariantMap &eventData)
{
    if (m_recentEventCapacity <= 0)
        return;
    while (m_recentEvents.size() >= static_cast<std::size_t>(m_recentEventCapacity))
        m_recentEvents.pop_front();
    m_recentEvents.push_back(eventData);
}

QVariantMap RuntimeEvents::describeQuickItemAtGlobal(qreal globalX, qreal globalY) const
{
    QVariantMap map;
    if (!m_window)
        return map;

    QQuickWindow *window = m_window.data();
    if (!window)
        return map;

    QQuickItem *content = window->contentItem();
    if (!content)
        return map;

    const QPoint windowPoint = window->mapFromGlobal(QPoint(qRound(globalX), qRound(globalY)));
    QPointF scenePos(windowPoint.x(), windowPoint.y());
    bool insideWindow = scenePos.x() >= 0.0
        && scenePos.y() >= 0.0
        && scenePos.x() <= static_cast<qreal>(window->width())
        && scenePos.y() <= static_cast<qreal>(window->height());
    QString coordinateMode = QStringLiteral("globalMapped");
    if (!insideWindow && !window->isVisible()) {
        const QPointF localFallback(globalX, globalY);
        const bool fallbackInside = localFallback.x() >= 0.0
            && localFallback.y() >= 0.0
            && localFallback.x() <= static_cast<qreal>(window->width())
            && localFallback.y() <= static_cast<qreal>(window->height());
        if (fallbackInside) {
            scenePos = localFallback;
            insideWindow = true;
            coordinateMode = QStringLiteral("hiddenWindowLocalFallback");
        }
    }

    map.insert(QStringLiteral("globalX"), globalX);
    map.insert(QStringLiteral("globalY"), globalY);
    map.insert(QStringLiteral("windowX"), scenePos.x());
    map.insert(QStringLiteral("windowY"), scenePos.y());
    map.insert(QStringLiteral("insideWindow"), insideWindow);
    map.insert(QStringLiteral("coordinateMode"), coordinateMode);

    if (!insideWindow)
        return map;

    QQuickItem *hitItem = deepestVisibleChildAt(content, scenePos);
    if (!hitItem)
        hitItem = content;
    const QQuickItem *logicalItem = logicalEventTargetItem(hitItem, content);
    if (!logicalItem)
        logicalItem = hitItem;

    const QPointF localPos = logicalItem->mapFromScene(scenePos);
    map.insert(QStringLiteral("localX"), localPos.x());
    map.insert(QStringLiteral("localY"), localPos.y());
    const QPointF hitLocalPos = hitItem->mapFromScene(scenePos);
    map.insert(QStringLiteral("hitLocalX"), hitLocalPos.x());
    map.insert(QStringLiteral("hitLocalY"), hitLocalPos.y());

    QString objectName = logicalItem->objectName().trimmed();
    if (objectName.isEmpty())
        objectName = QStringLiteral("unnamed");
    map.insert(QStringLiteral("objectName"), objectName);
    map.insert(QStringLiteral("className"), QString::fromLatin1(logicalItem->metaObject()->className()));
    map.insert(QStringLiteral("path"), quickItemPath(logicalItem, content));
    map.insert(QStringLiteral("componentName"), componentNameForObject(logicalItem));
    map.insert(QStringLiteral("qmlId"), qmlObjectName(logicalItem));
    map.insert(QStringLiteral("qmlBaseUrl"), qmlBaseUrlString(logicalItem));
    map.insert(QStringLiteral("enabled"), logicalItem->isEnabled());
    map.insert(QStringLiteral("visible"), logicalItem->isVisible());
    map.insert(QStringLiteral("x"), logicalItem->x());
    map.insert(QStringLiteral("y"), logicalItem->y());
    map.insert(QStringLiteral("width"), logicalItem->width());
    map.insert(QStringLiteral("height"), logicalItem->height());
    map.insert(QStringLiteral("z"), logicalItem->z());
    map.insert(QStringLiteral("opacity"), logicalItem->opacity());
    map.insert(QStringLiteral("clip"), logicalItem->clip());
    const QVariantList hierarchy = quickItemHierarchy(logicalItem, content, window);
    map.insert(QStringLiteral("hierarchy"), hierarchy);
    map.insert(QStringLiteral("depth"), qMax(0, hierarchy.size() - 1));
    map.insert(QStringLiteral("layerKind"), quickItemLayerKind(logicalItem, content));
    map.insert(QStringLiteral("windowObjectName"), window->objectName().trimmed());
    map.insert(QStringLiteral("windowClassName"), QString::fromLatin1(window->metaObject()->className()));
    map.insert(QStringLiteral("windowTitle"), window->title());
    QString hitObjectName = hitItem->objectName().trimmed();
    if (hitObjectName.isEmpty())
        hitObjectName = QStringLiteral("unnamed");
    map.insert(QStringLiteral("hitObjectName"), hitObjectName);
    map.insert(QStringLiteral("hitClassName"), QString::fromLatin1(hitItem->metaObject()->className()));
    map.insert(QStringLiteral("hitPath"), quickItemPath(hitItem, content));
    map.insert(QStringLiteral("hitComponentName"), componentNameForObject(hitItem));
    map.insert(QStringLiteral("hitQmlId"), qmlObjectName(hitItem));
    map.insert(QStringLiteral("hitDepth"), qMax(0, quickItemHierarchy(hitItem, content, window).size() - 1));
    if (!hierarchy.isEmpty()) {
        const QVariantMap root = hierarchy.constFirst().toMap();
        map.insert(QStringLiteral("rootObjectName"), root.value(QStringLiteral("objectName")));
        map.insert(QStringLiteral("rootClassName"), root.value(QStringLiteral("className")));
        map.insert(QStringLiteral("rootComponentName"), root.value(QStringLiteral("componentName")));
    }

    const QVariant textProp = hitItem->property("text");
    if (textProp.isValid())
        map.insert(QStringLiteral("text"), textProp.toString());
    const QVariant labelProp = hitItem->property("label");
    if (labelProp.isValid())
        map.insert(QStringLiteral("label"), labelProp.toString());
    const QVariant titleProp = hitItem->property("title");
    if (titleProp.isValid())
        map.insert(QStringLiteral("title"), titleProp.toString());

    return map;
}

QVariantMap RuntimeEvents::fallbackUiAt(qreal globalX, qreal globalY) const
{
    QVariantMap fallback;
    QQuickWindow *window = m_window.data();
    fallback.insert(QStringLiteral("globalX"), globalX);
    fallback.insert(QStringLiteral("globalY"), globalY);
    fallback.insert(QStringLiteral("insideWindow"), false);
    fallback.insert(QStringLiteral("objectName"), QStringLiteral("unknown"));
    fallback.insert(QStringLiteral("className"), QStringLiteral("unknown"));
    fallback.insert(QStringLiteral("path"), QStringLiteral("unknown"));
    fallback.insert(QStringLiteral("componentName"), QStringLiteral("unknown"));
    fallback.insert(QStringLiteral("qmlId"), QString());
    fallback.insert(QStringLiteral("qmlBaseUrl"), QString());
    fallback.insert(QStringLiteral("x"), 0.0);
    fallback.insert(QStringLiteral("y"), 0.0);
    fallback.insert(QStringLiteral("width"), 0.0);
    fallback.insert(QStringLiteral("height"), 0.0);
    fallback.insert(QStringLiteral("z"), 0.0);
    fallback.insert(QStringLiteral("opacity"), 0.0);
    fallback.insert(QStringLiteral("clip"), false);
    fallback.insert(QStringLiteral("hierarchy"), QVariantList());
    fallback.insert(QStringLiteral("depth"), -1);
    fallback.insert(QStringLiteral("layerKind"), window ? QStringLiteral("outsideWindow") : QStringLiteral("unboundWindow"));
    fallback.insert(QStringLiteral("windowObjectName"), window ? window->objectName().trimmed() : QStringLiteral("unknown"));
    fallback.insert(QStringLiteral("windowClassName"),
                    window ? QString::fromLatin1(window->metaObject()->className()) : QStringLiteral("unknown"));
    fallback.insert(QStringLiteral("windowTitle"), window ? window->title() : QString());
    fallback.insert(QStringLiteral("rootObjectName"), QStringLiteral("unknown"));
    fallback.insert(QStringLiteral("rootClassName"), QStringLiteral("unknown"));
    fallback.insert(QStringLiteral("rootComponentName"), QStringLiteral("unknown"));
    return fallback;
}

QVariantMap RuntimeEvents::describeQuickItemIdentity(const QQuickItem *item,
                                                     const QQuickItem *rootItem,
                                                     const QQuickWindow *window,
                                                     int depth,
                                                     bool isHit) const
{
    QVariantMap map;
    if (!item)
        return map;

    QString objectName = item->objectName().trimmed();
    if (objectName.isEmpty())
        objectName = QStringLiteral("unnamed");

    map.insert(QStringLiteral("objectName"), objectName);
    map.insert(QStringLiteral("className"), QString::fromLatin1(item->metaObject()->className()));
    map.insert(QStringLiteral("componentName"), componentNameForObject(item));
    map.insert(QStringLiteral("qmlId"), qmlObjectName(item));
    map.insert(QStringLiteral("qmlBaseUrl"), qmlBaseUrlString(item));
    map.insert(QStringLiteral("path"), quickItemPath(item, rootItem));
    map.insert(QStringLiteral("enabled"), item->isEnabled());
    map.insert(QStringLiteral("visible"), item->isVisible());
    map.insert(QStringLiteral("x"), item->x());
    map.insert(QStringLiteral("y"), item->y());
    map.insert(QStringLiteral("width"), item->width());
    map.insert(QStringLiteral("height"), item->height());
    map.insert(QStringLiteral("z"), item->z());
    map.insert(QStringLiteral("opacity"), item->opacity());
    map.insert(QStringLiteral("clip"), item->clip());
    map.insert(QStringLiteral("depth"), depth);
    map.insert(QStringLiteral("isHit"), isHit);
    map.insert(QStringLiteral("isRoot"), item == rootItem);
    map.insert(QStringLiteral("layerKind"), quickItemLayerKind(item, rootItem));
    if (window) {
        map.insert(QStringLiteral("windowObjectName"), window->objectName().trimmed());
        map.insert(QStringLiteral("windowClassName"), QString::fromLatin1(window->metaObject()->className()));
    }
    return map;
}

QVariantList RuntimeEvents::quickItemHierarchy(const QQuickItem *item,
                                               const QQuickItem *rootItem,
                                               const QQuickWindow *window) const
{
    QVariantList hierarchy;
    if (!item)
        return hierarchy;

    QList<const QQuickItem *> chain;
    const QQuickItem *cursor = item;
    while (cursor) {
        chain.prepend(cursor);
        if (cursor == rootItem)
            break;
        cursor = cursor->parentItem();
    }

    hierarchy.reserve(chain.size());
    for (qsizetype index = 0; index < chain.size(); ++index) {
        hierarchy.append(describeQuickItemIdentity(chain.at(index),
                                                   rootItem,
                                                   window,
                                                   static_cast<int>(index),
                                                   chain.at(index) == item));
    }
    return hierarchy;
}

QString RuntimeEvents::keyLabelForCode(int key) const
{
    const QString named = QKeySequence(key).toString(QKeySequence::PortableText).trimmed();
    if (!named.isEmpty())
        return named;
    return QStringLiteral("Key(%1)").arg(key);
}

QStringList RuntimeEvents::mouseButtonNames(int buttons) const
{
    QStringList names;
    if (buttons & Qt::LeftButton)
        names.append(QStringLiteral("Left"));
    if (buttons & Qt::RightButton)
        names.append(QStringLiteral("Right"));
    if (buttons & Qt::MiddleButton)
        names.append(QStringLiteral("Middle"));
    if (buttons & Qt::BackButton)
        names.append(QStringLiteral("Back"));
    if (buttons & Qt::ForwardButton)
        names.append(QStringLiteral("Forward"));
    if (buttons & Qt::ExtraButton1)
        names.append(QStringLiteral("Extra1"));
    if (buttons & Qt::ExtraButton2)
        names.append(QStringLiteral("Extra2"));
    return names;
}

QStringList RuntimeEvents::modifierNames(int modifiers) const
{
    QStringList names;
    if (modifiers & Qt::ShiftModifier)
        names.append(QStringLiteral("Shift"));
    if (modifiers & Qt::ControlModifier)
        names.append(QStringLiteral("Ctrl"));
    if (modifiers & Qt::AltModifier)
        names.append(QStringLiteral("Alt"));
    if (modifiers & Qt::MetaModifier)
        names.append(QStringLiteral("Meta"));
    if (modifiers & Qt::KeypadModifier)
        names.append(QStringLiteral("Keypad"));
    return names;
}

QQuickItem *RuntimeEvents::deepestVisibleChildAt(QQuickItem *item, const QPointF &scenePos) const
{
    if (!item || !item->isVisible())
        return nullptr;

    QList<QQuickItem *> children = item->childItems();
    std::stable_sort(children.begin(),
                     children.end(),
                     [](QQuickItem *lhs, QQuickItem *rhs) {
                         if (!lhs || !rhs)
                             return lhs < rhs;
                         if (!qFuzzyCompare(lhs->z(), rhs->z()))
                             return lhs->z() < rhs->z();
                         return false;
                     });
    for (auto it = children.crbegin(); it != children.crend(); ++it) {
        QQuickItem *child = *it;
        if (!child || !child->isVisible())
            continue;

        const QPointF childPos = child->mapFromScene(scenePos);
        if (!child->contains(childPos))
            continue;

        if (QQuickItem *deepest = deepestVisibleChildAt(child, scenePos))
            return deepest;
    }

    const QPointF itemPos = item->mapFromScene(scenePos);
    if (!item->contains(itemPos))
        return nullptr;
    if (item->property("runtimeHitTransparent").toBool())
        return nullptr;
    return item;
}

const QQuickItem *RuntimeEvents::logicalEventTargetItem(const QQuickItem *item, const QQuickItem *rootItem) const
{
    if (!item)
        return nullptr;

    const QQuickItem *cursor = item;
    while (cursor) {
        if (quickItemHasStableIdentity(cursor))
            return cursor;
        if (cursor == rootItem)
            break;
        cursor = cursor->parentItem();
    }
    return item;
}

bool RuntimeEvents::quickItemHasStableIdentity(const QQuickItem *item) const
{
    if (!item)
        return false;
    if (!qmlObjectName(item).isEmpty())
        return true;
    return !item->objectName().trimmed().isEmpty();
}

QString RuntimeEvents::quickItemPath(const QQuickItem *item, const QQuickItem *rootItem) const
{
    if (!item)
        return QStringLiteral("unknown");

    QStringList parts;
    const QQuickItem *cursor = item;
    while (cursor) {
        QString name = cursor->objectName().trimmed();
        const QString className = QString::fromLatin1(cursor->metaObject()->className());
        parts.prepend(name.isEmpty() ? className : className + QLatin1Char(':') + name);
        if (cursor == rootItem)
            break;
        cursor = cursor->parentItem();
    }

    return parts.join(QStringLiteral(" > "));
}

QString RuntimeEvents::quickItemLayerKind(const QQuickItem *item, const QQuickItem *rootItem) const
{
    if (!item)
        return QStringLiteral("unknown");

    const QQuickItem *cursor = item;
    bool reachedRoot = false;
    while (cursor) {
        const QString objectName = cursor->objectName().trimmed().toLower();
        const QString className = QString::fromLatin1(cursor->metaObject()->className());
        if (className.contains(QStringLiteral("Overlay"), Qt::CaseInsensitive)
            || objectName.contains(QStringLiteral("overlay"))) {
            return QStringLiteral("overlay");
        }
        if (objectName.contains(QStringLiteral("safearea")))
            return QStringLiteral("safeArea");
        if (cursor == rootItem)
            reachedRoot = true;
        cursor = cursor->parentItem();
    }

    return reachedRoot ? QStringLiteral("content") : QStringLiteral("window");
}

QString RuntimeEvents::qmlObjectName(const QObject *object) const
{
    if (!object)
        return QString();

    if (QQmlContext *context = QQmlEngine::contextForObject(object)) {
        const QString name = context->nameForObject(object).trimmed();
        if (!name.isEmpty())
            return name;
    }
    if (QQmlContext *context = qmlContext(object)) {
        const QString name = context->nameForObject(object).trimmed();
        if (!name.isEmpty())
            return name;
    }
    return QString();
}

QString RuntimeEvents::qmlBaseUrlString(const QObject *object) const
{
    if (!object)
        return QString();

    if (QQmlContext *context = QQmlEngine::contextForObject(object))
        return context->baseUrl().toString();
    if (QQmlContext *context = qmlContext(object))
        return context->baseUrl().toString();
    return QString();
}

QString RuntimeEvents::componentNameForObject(const QObject *object) const
{
    if (!object)
        return QStringLiteral("unknown");

    const QString qmlName = qmlObjectName(object);
    if (!qmlName.isEmpty())
        return qmlName;

    const QString objectName = object->objectName().trimmed();
    if (!objectName.isEmpty())
        return objectName;

    return QString::fromLatin1(object->metaObject()->className());
}
