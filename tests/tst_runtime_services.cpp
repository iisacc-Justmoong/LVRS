#include "QtEventWait.h"
#include <QEventLoop>
#include <QGuiApplication>
#include <QScopeGuard>
#include <QTimer>
#include <type_traits>

#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QEvent>
#include <QEventPoint>
#include <QInputDevice>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPointingDevice>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTouchEvent>
#include <QtPlugin>

#include "backend/runtime/debuglogger.h"
#include "backend/runtime/gestureevents.h"
#include "backend/runtime/renderingmonitor.h"
#include "backend/runtime/runtimeevents.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

namespace {

const QPointingDevice *testTouchDevice()
{
    static const QPointingDevice *device = new QPointingDevice(
        QStringLiteral("LVRS Test Touchscreen"),
        1,
        QInputDevice::DeviceType::TouchScreen,
        QPointingDevice::PointerType::Finger,
        QInputDevice::Capability::Position
            | QInputDevice::Capability::Area
            | QInputDevice::Capability::Pressure,
        10,
        1);
    return device;
}

} // namespace

namespace {
template <typename Actual, typename Expected>
bool runtimeEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual) + 1.0, qreal(expected) + 1.0);
    else
        return actual == expected;
}
void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}
}
#define RUNTIME_COMPARE(actual, expected) CTEST_CHECK(runtimeEqual((actual), (expected)))
#define RUNTIME_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(runtimeEqual((actual), (expected)))

class RuntimeServicesTests
{
public:
    void runtime_events_measurement_boundaries();
    void runtime_events_daemon_health_and_recent_log();
    void runtime_events_idle_and_reset_signal_contract();
    void runtime_events_context_menu_signal_contract();
    void runtime_events_single_input_event_is_counted_once();
    void runtime_events_touch_input_normalizes_to_pointer_contract();
    void runtime_events_touch_payload_exposes_native_point_details();
    void runtime_events_hit_test_exposes_ui_hierarchy_metadata();
    void gesture_events_recognize_hold_drag_and_swipe();
    void gesture_events_classify_mobile_press_scroll_and_multitouch();
    void runtime_events_window_replacement_survives_old_window_destruction();
    void render_monitor_counts_frames_when_swapped();
    void render_monitor_active_signal_and_destroy_path();
    void debug_logger_enabled_signal_and_message_format();
};

void RuntimeServicesTests::runtime_events_measurement_boundaries()
{
    RuntimeEvents events;
    QQuickWindow window;
    window.setWidth(800);
    window.setHeight(500);
    events.attachWindow(&window);
    CTEST_WAIT_DEFAULT(events.running());
    RUNTIME_COMPARE(events.uiCreatedCount(), quint64(0));
    RUNTIME_COMPARE(events.captureProfile(), static_cast<int>(RuntimeEvents::LowLatencyCapture));
    CTEST_CHECK(!events.pointerHitTestingEnabled());
    CTEST_CHECK(!events.uiTrackingEnabled());

    events.setIdleTimeoutMs(-10);
    RUNTIME_COMPARE(events.idleTimeoutMs(), 250);
    events.setIdleTimeoutMs(24 * 60 * 60 * 1000 + 999);
    RUNTIME_COMPARE(events.idleTimeoutMs(), 24 * 60 * 60 * 1000);

    events.setOsSampleIntervalMs(-10);
    RUNTIME_COMPARE(events.osSampleIntervalMs(), 250);
    events.setOsSampleIntervalMs(70 * 1000);
    RUNTIME_COMPARE(events.osSampleIntervalMs(), 60 * 1000);

    events.setCaptureProfile(RuntimeEvents::LowLatencyCapture);
    RUNTIME_COMPARE(events.captureProfile(), static_cast<int>(RuntimeEvents::LowLatencyCapture));
    CTEST_CHECK(!events.pointerHitTestingEnabled());
    CTEST_CHECK(!events.uiTrackingEnabled());

    events.setCaptureProfile(RuntimeEvents::FullCapture);
    RUNTIME_COMPARE(events.captureProfile(), static_cast<int>(RuntimeEvents::FullCapture));
    CTEST_CHECK(events.pointerHitTestingEnabled());
    CTEST_CHECK(events.uiTrackingEnabled());
    CTEST_CHECK(events.uiCreatedCount() > 0);
    events.setPointerHitTestMinIntervalMs(48);
    RUNTIME_COMPARE(events.pointerHitTestMinIntervalMs(), 48);

    const qint64 previousActivityEpoch = events.lastActivityEpochMs();
    processFor(2);
    events.markActivity();
    CTEST_CHECK(events.lastActivityEpochMs() >= previousActivityEpoch);

    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_K, Qt::ControlModifier, QStringLiteral("k"));
    QCoreApplication::sendEvent(&window, &keyPress);
    CTEST_CHECK(events.isKeyPressed(Qt::Key_K));
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_K, Qt::ControlModifier, QStringLiteral("k"));
    QCoreApplication::sendEvent(&window, &keyRelease);
    CTEST_CHECK(!events.isKeyPressed(Qt::Key_K));
    CTEST_CHECK(events.keyPressCount() >= 1);
    CTEST_CHECK(events.keyReleaseCount() >= 1);

    const QPointF p(24.0, 18.0);
    QMouseEvent mouseMove(QEvent::MouseMove, p, p, p, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QMouseEvent mousePress(QEvent::MouseButtonPress, p, p, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent mouseRelease(QEvent::MouseButtonRelease, p, p, p, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &mouseMove);
    QCoreApplication::sendEvent(&window, &mousePress);
    QCoreApplication::sendEvent(&window, &mouseRelease);
    CTEST_CHECK(events.mouseMoveCount() >= 1);
    CTEST_CHECK(events.mousePressCount() >= 1);
    CTEST_CHECK(events.mouseReleaseCount() >= 1);

    const quint64 createdBefore = events.uiCreatedCount();
    auto *dynamicItem = new QQuickItem(window.contentItem());
    dynamicItem->setVisible(true);
    QCoreApplication::processEvents();
    CTEST_CHECK(events.uiCreatedCount() > createdBefore);
    delete dynamicItem;
    QCoreApplication::processEvents();
    CTEST_CHECK(events.uiDestroyedCount() >= 1);

    const QVariantMap snapshot = events.snapshot();
    CTEST_CHECK(snapshot.contains(QStringLiteral("running")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("keyPressCount")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("mouseMoveCount")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("uiCreatedCount")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("idle")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("pid")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("rssBytes")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("uptimeMs")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("daemonBootEpochMs")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("eventSequence")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("recentEventCount")));
    CTEST_CHECK(snapshot.value(QStringLiteral("pid")).toLongLong() > 0);

    events.stop();
    CTEST_CHECK(!events.running());
}

void RuntimeServicesTests::runtime_events_daemon_health_and_recent_log()
{
    RuntimeEvents events;
    QQuickWindow window;
    window.setWidth(640);
    window.setHeight(360);
    events.attachWindow(&window);
    CTEST_WAIT_DEFAULT(events.running());

    CTEST_CHECK(events.daemonBootEpochMs() > 0);
    const QVariantMap health = events.daemonHealth();
    CTEST_CHECK(health.value(QStringLiteral("running")).toBool());
    CTEST_CHECK(health.value(QStringLiteral("attachedWindow")).toBool());
    CTEST_CHECK(health.contains(QStringLiteral("eventSequence")));
    CTEST_CHECK(health.contains(QStringLiteral("recentEventCount")));

    const quint64 seqBefore = events.eventSequence();

    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_D, Qt::NoModifier, QStringLiteral("d"));
    QCoreApplication::sendEvent(&window, &keyPress);
    CTEST_WAIT_DEFAULT(events.eventSequence() > seqBefore);

    const QVariantMap lastEvent = events.lastEvent();
    CTEST_CHECK(!lastEvent.isEmpty());
    CTEST_CHECK(lastEvent.contains(QStringLiteral("type")));
    CTEST_CHECK(lastEvent.contains(QStringLiteral("sequence")));
    CTEST_CHECK(events.recentEventCount() > 0);
    CTEST_CHECK(!events.recentEvents().isEmpty());

    events.setRecentEventCapacity(16);
    RUNTIME_COMPARE(events.recentEventCapacity(), 16);
    events.clearRecentEvents();
    RUNTIME_COMPARE(events.recentEventCount(), 0);
}

void RuntimeServicesTests::runtime_events_idle_and_reset_signal_contract()
{
    RuntimeEvents events;
    QQuickWindow window;
    events.attachWindow(&window);
    CTEST_WAIT_DEFAULT(events.running());

    QList<QVariantList> idleChangedSpyEvents;
    const auto idleChangedSpyConnection = QObject::connect(&events, &RuntimeEvents::idleChanged,
        [&idleChangedSpyEvents]() { idleChangedSpyEvents.append(QVariantList{}); });
    const auto idleChangedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(idleChangedSpyConnection); });
    QList<QVariantList> idleEnteredSpyEvents;
    const auto idleEnteredSpyConnection = QObject::connect(&events, &RuntimeEvents::idleEntered,
        [&idleEnteredSpyEvents]() { idleEnteredSpyEvents.append(QVariantList{}); });
    const auto idleEnteredSpyDisconnect = qScopeGuard([&] { QObject::disconnect(idleEnteredSpyConnection); });
    QList<QVariantList> idleExitedSpyEvents;
    const auto idleExitedSpyConnection = QObject::connect(&events, &RuntimeEvents::idleExited,
        [&idleExitedSpyEvents]() { idleExitedSpyEvents.append(QVariantList{}); });
    const auto idleExitedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(idleExitedSpyConnection); });
    QList<QVariantList> runningSpyEvents;
    const auto runningSpyConnection = QObject::connect(&events, &RuntimeEvents::runningChanged,
        [&runningSpyEvents]() { runningSpyEvents.append(QVariantList{}); });
    const auto runningSpyDisconnect = qScopeGuard([&] { QObject::disconnect(runningSpyConnection); });
    CTEST_CHECK(bool(idleChangedSpyConnection));
    CTEST_CHECK(bool(idleEnteredSpyConnection));
    CTEST_CHECK(bool(idleExitedSpyConnection));
    CTEST_CHECK(bool(runningSpyConnection));

    events.setIdleTimeoutMs(250);
    events.markActivity();
    CTEST_WAIT_VERIFY(events.idle(), 1200);
    CTEST_CHECK(idleChangedSpyEvents.size() >= 1);
    CTEST_CHECK(idleEnteredSpyEvents.size() >= 1);

    events.markActivity();
    CTEST_WAIT_DEFAULT(!events.idle());
    CTEST_CHECK(idleExitedSpyEvents.size() >= 1);

    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_T, Qt::NoModifier, QStringLiteral("t"));
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_T, Qt::NoModifier, QStringLiteral("t"));
    QCoreApplication::sendEvent(&window, &keyPress);
    CTEST_CHECK(events.anyKeyPressed());
    QCoreApplication::sendEvent(&window, &keyRelease);
    CTEST_CHECK(!events.anyKeyPressed());

    const QPointF p(12.0, 14.0);
    QMouseEvent mousePress(QEvent::MouseButtonPress, p, p, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent mouseRelease(QEvent::MouseButtonRelease, p, p, p, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &mousePress);
    CTEST_CHECK(events.mouseButtonPressed());
    QCoreApplication::sendEvent(&window, &mouseRelease);
    CTEST_CHECK(!events.mouseButtonPressed());

    events.stop();
    CTEST_CHECK(!events.running());

    events.resetCounters();
    RUNTIME_COMPARE(events.keyPressCount(), 0u);
    RUNTIME_COMPARE(events.keyReleaseCount(), 0u);
    RUNTIME_COMPARE(events.mouseMoveCount(), 0u);
    RUNTIME_COMPARE(events.mousePressCount(), 0u);
    RUNTIME_COMPARE(events.mouseReleaseCount(), 0u);
    RUNTIME_COMPARE(events.uiCreatedCount(), 0u);
    RUNTIME_COMPARE(events.uiShownCount(), 0u);
    RUNTIME_COMPARE(events.uiHiddenCount(), 0u);
    RUNTIME_COMPARE(events.uiDestroyedCount(), 0u);
    CTEST_CHECK(!events.anyKeyPressed());
    CTEST_CHECK(!events.mouseButtonPressed());
    RUNTIME_COMPARE(runningSpyEvents.size(), 1);
}

void RuntimeServicesTests::runtime_events_context_menu_signal_contract()
{
    RuntimeEvents events;
    QQuickWindow window;
    events.attachWindow(&window);
    CTEST_WAIT_DEFAULT(events.running());

    QList<QVariantList> contextSpyEvents;
    const auto contextSpyConnection = QObject::connect(&events, &RuntimeEvents::contextRequested,
        [&contextSpyEvents](qreal x, qreal y, int modifiers, int reason) { contextSpyEvents.append(QVariantList{x, y, modifiers, reason}); });
    const auto contextSpyDisconnect = qScopeGuard([&] { QObject::disconnect(contextSpyConnection); });
    CTEST_CHECK(bool(contextSpyConnection));

    const QPoint local(22, 16);
    const QPoint global(320, 240);
    QContextMenuEvent contextEvent(QContextMenuEvent::Mouse,
                                   local,
                                   global,
                                   Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &contextEvent);

    CTEST_WAIT_DEFAULT(contextSpyEvents.size() >= 1);
    const QList<QVariant> args = contextSpyEvents.takeLast();
    CTEST_CHECK(args.size() >= 4);
    RUNTIME_COMPARE(args.at(2).toInt(), static_cast<int>(Qt::NoModifier));
    RUNTIME_COMPARE(args.at(3).toInt(), static_cast<int>(QContextMenuEvent::Mouse));
    RUNTIME_COMPARE(events.lastMouseButtons(), static_cast<int>(Qt::RightButton));
    RUNTIME_COMPARE(events.lastMouseX(), static_cast<qreal>(global.x()));
    RUNTIME_COMPARE(events.lastMouseY(), static_cast<qreal>(global.y()));
}

void RuntimeServicesTests::runtime_events_single_input_event_is_counted_once()
{
    RuntimeEvents events;
    QQuickWindow window;
    events.attachWindow(&window);
    CTEST_WAIT_DEFAULT(events.running());

    events.resetCounters();

    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_G, Qt::NoModifier, QStringLiteral("g"));
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_G, Qt::NoModifier, QStringLiteral("g"));
    QCoreApplication::sendEvent(&window, &keyPress);
    QCoreApplication::sendEvent(&window, &keyRelease);

    const QPointF p(30.0, 22.0);
    QMouseEvent mouseMove(QEvent::MouseMove, p, p, p, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QMouseEvent mousePress(QEvent::MouseButtonPress, p, p, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent mouseRelease(QEvent::MouseButtonRelease, p, p, p, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &mouseMove);
    QCoreApplication::sendEvent(&window, &mousePress);
    QCoreApplication::sendEvent(&window, &mouseRelease);

    RUNTIME_COMPARE(events.keyPressCount(), 1u);
    RUNTIME_COMPARE(events.keyReleaseCount(), 1u);
    RUNTIME_COMPARE(events.mouseMoveCount(), 1u);
    RUNTIME_COMPARE(events.mousePressCount(), 1u);
    RUNTIME_COMPARE(events.mouseReleaseCount(), 1u);
    RUNTIME_COMPARE(events.lastMouseX(), 30.0);
    RUNTIME_COMPARE(events.lastMouseY(), 22.0);
}

void RuntimeServicesTests::runtime_events_touch_input_normalizes_to_pointer_contract()
{
    RuntimeEvents events;
    QQuickWindow window;
    window.setWidth(480);
    window.setHeight(320);
    events.attachWindow(&window);
    CTEST_WAIT_DEFAULT(events.running());

    QList<QVariantList> mousePressedSpyEvents;
    const auto mousePressedSpyConnection = QObject::connect(&events, &RuntimeEvents::mousePressed,
        [&mousePressedSpyEvents](qreal x, qreal y, int buttons, int modifiers) { mousePressedSpyEvents.append(QVariantList{x, y, buttons, modifiers}); });
    const auto mousePressedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(mousePressedSpyConnection); });
    QList<QVariantList> mouseMovedSpyEvents;
    const auto mouseMovedSpyConnection = QObject::connect(&events, &RuntimeEvents::mouseMoved,
        [&mouseMovedSpyEvents](qreal x, qreal y, int buttons, int modifiers) { mouseMovedSpyEvents.append(QVariantList{x, y, buttons, modifiers}); });
    const auto mouseMovedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(mouseMovedSpyConnection); });
    QList<QVariantList> mouseReleasedSpyEvents;
    const auto mouseReleasedSpyConnection = QObject::connect(&events, &RuntimeEvents::mouseReleased,
        [&mouseReleasedSpyEvents](qreal x, qreal y, int buttons, int modifiers) { mouseReleasedSpyEvents.append(QVariantList{x, y, buttons, modifiers}); });
    const auto mouseReleasedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(mouseReleasedSpyConnection); });
    CTEST_CHECK(bool(mousePressedSpyConnection));
    CTEST_CHECK(bool(mouseMovedSpyConnection));
    CTEST_CHECK(bool(mouseReleasedSpyConnection));

    events.resetCounters();

    const QPointF beginPoint(40.0, 28.0);
    const QPointF updatePoint(52.0, 36.0);

    QTouchEvent touchBegin(QEvent::TouchBegin,
                           testTouchDevice(),
                           Qt::NoModifier,
                           {QEventPoint(0, QEventPoint::State::Pressed, beginPoint, beginPoint)});
    QCoreApplication::sendEvent(&window, &touchBegin);

    RUNTIME_COMPARE(events.mousePressCount(), 1u);
    RUNTIME_COMPARE(events.mouseMoveCount(), 0u);
    RUNTIME_COMPARE(events.mouseReleaseCount(), 0u);
    CTEST_CHECK(events.mouseButtonPressed());
    RUNTIME_COMPARE(events.lastMouseButtons(), static_cast<int>(Qt::LeftButton));
    RUNTIME_COMPARE(events.lastMouseX(), beginPoint.x());
    RUNTIME_COMPARE(events.lastMouseY(), beginPoint.y());
    CTEST_CHECK(events.lastMousePressEpochMs() > 0);
    RUNTIME_COMPARE(mousePressedSpyEvents.size(), 1);

    const QVariantMap beginState = events.inputState();
    RUNTIME_COMPARE(beginState.value(QStringLiteral("mouseButtons")).toInt(), static_cast<int>(Qt::LeftButton));
    CTEST_CHECK(beginState.value(QStringLiteral("mouseButtonPressed")).toBool());

    QTouchEvent touchUpdate(QEvent::TouchUpdate,
                            testTouchDevice(),
                            Qt::NoModifier,
                            {QEventPoint(0, QEventPoint::State::Updated, updatePoint, updatePoint)});
    QCoreApplication::sendEvent(&window, &touchUpdate);

    RUNTIME_COMPARE(events.mousePressCount(), 1u);
    RUNTIME_COMPARE(events.mouseMoveCount(), 1u);
    RUNTIME_COMPARE(events.mouseReleaseCount(), 0u);
    CTEST_CHECK(events.mouseButtonPressed());
    RUNTIME_COMPARE(events.lastMouseButtons(), static_cast<int>(Qt::LeftButton));
    RUNTIME_COMPARE(events.lastMouseX(), updatePoint.x());
    RUNTIME_COMPARE(events.lastMouseY(), updatePoint.y());
    RUNTIME_COMPARE(mouseMovedSpyEvents.size(), 1);

    QTouchEvent touchEnd(QEvent::TouchEnd,
                         testTouchDevice(),
                         Qt::NoModifier,
                         {QEventPoint(0, QEventPoint::State::Released, updatePoint, updatePoint)});
    QCoreApplication::sendEvent(&window, &touchEnd);

    RUNTIME_COMPARE(events.mousePressCount(), 1u);
    RUNTIME_COMPARE(events.mouseMoveCount(), 1u);
    RUNTIME_COMPARE(events.mouseReleaseCount(), 1u);
    CTEST_CHECK(!events.mouseButtonPressed());
    RUNTIME_COMPARE(events.lastMouseButtons(), static_cast<int>(Qt::NoButton));
    RUNTIME_COMPARE(events.lastMouseX(), updatePoint.x());
    RUNTIME_COMPARE(events.lastMouseY(), updatePoint.y());
    CTEST_CHECK(events.lastMouseReleaseEpochMs() > 0);
    RUNTIME_COMPARE(mouseReleasedSpyEvents.size(), 1);

    const QVariantMap lastEvent = events.lastEvent();
    RUNTIME_COMPARE(lastEvent.value(QStringLiteral("type")).toString(), QStringLiteral("touch-event"));
    const QVariantMap payload = lastEvent.value(QStringLiteral("payload")).toMap();
    RUNTIME_COMPARE(payload.value(QStringLiteral("phase")).toString(), QStringLiteral("end"));
    RUNTIME_COMPARE(payload.value(QStringLiteral("buttons")).toInt(), static_cast<int>(Qt::NoButton));
    CTEST_CHECK(!payload.value(QStringLiteral("mouseButtonPressed")).toBool());
}

void RuntimeServicesTests::runtime_events_touch_payload_exposes_native_point_details()
{
    RuntimeEvents events;
    QQuickWindow window;
    window.setWidth(480);
    window.setHeight(320);
    events.attachWindow(&window);
    CTEST_WAIT_DEFAULT(events.running());

    events.resetCounters();

    const QPointF firstBegin(40.0, 28.0);
    const QPointF secondBegin(74.0, 36.0);
    QTouchEvent touchBegin(QEvent::TouchBegin,
                           testTouchDevice(),
                           Qt::NoModifier,
                           {
                               QEventPoint(0, QEventPoint::State::Pressed, firstBegin, firstBegin),
                               QEventPoint(1, QEventPoint::State::Pressed, secondBegin, secondBegin),
                           });
    QCoreApplication::sendEvent(&window, &touchBegin);

    const QVariantMap beginEvent = events.lastEvent();
    RUNTIME_COMPARE(beginEvent.value(QStringLiteral("type")).toString(), QStringLiteral("touch-event"));
    const QVariantMap beginPayload = beginEvent.value(QStringLiteral("payload")).toMap();
    RUNTIME_COMPARE(beginPayload.value(QStringLiteral("phase")).toString(), QStringLiteral("begin"));
    RUNTIME_COMPARE(beginPayload.value(QStringLiteral("pointCount")).toInt(), 2);
    RUNTIME_COMPARE(beginPayload.value(QStringLiteral("fingerCount")).toInt(), 2);
    RUNTIME_COMPARE(beginPayload.value(QStringLiteral("activeFingerCount")).toInt(), 2);
    RUNTIME_COMPARE(beginPayload.value(QStringLiteral("pressedFingerCount")).toInt(), 2);
    RUNTIME_COMPARE(beginPayload.value(QStringLiteral("releasedFingerCount")).toInt(), 0);
    RUNTIME_COMPARE(beginPayload.value(QStringLiteral("primaryPointId")).toInt(), 0);
    CTEST_CHECK(beginPayload.value(QStringLiteral("multiTouch")).toBool());
    CTEST_CHECK(!beginPayload.value(QStringLiteral("released")).toBool());
    CTEST_CHECK(!beginPayload.value(QStringLiteral("cancelled")).toBool());
    RUNTIME_COMPARE(beginPayload.value(QStringLiteral("touchDeviceName")).toString(),
             QStringLiteral("LVRS Test Touchscreen"));
    RUNTIME_COMPARE(beginPayload.value(QStringLiteral("maximumTouchPoints")).toInt(), 10);
    CTEST_CHECK(beginPayload.contains(QStringLiteral("nativeTimestamp")));

    const QVariantList beginPoints = beginPayload.value(QStringLiteral("points")).toList();
    RUNTIME_COMPARE(beginPoints.size(), 2);
    const QVariantMap firstPoint = beginPoints.constFirst().toMap();
    RUNTIME_COMPARE(firstPoint.value(QStringLiteral("id")).toInt(), 0);
    RUNTIME_COMPARE(firstPoint.value(QStringLiteral("stateName")).toString(), QStringLiteral("pressed"));
    CTEST_CHECK(firstPoint.contains(QStringLiteral("positionX")));
    RUNTIME_COMPARE(firstPoint.value(QStringLiteral("scenePositionX")).toReal(), firstBegin.x());
    RUNTIME_COMPARE(firstPoint.value(QStringLiteral("globalX")).toReal(), firstBegin.x());
    CTEST_CHECK(firstPoint.contains(QStringLiteral("pressPositionX")));
    CTEST_CHECK(firstPoint.contains(QStringLiteral("globalPressPositionX")));
    CTEST_CHECK(firstPoint.contains(QStringLiteral("lastPositionX")));
    CTEST_CHECK(firstPoint.contains(QStringLiteral("timestamp")));
    CTEST_CHECK(firstPoint.contains(QStringLiteral("pressTimestamp")));
    CTEST_CHECK(firstPoint.contains(QStringLiteral("timeHeld")));
    CTEST_CHECK(firstPoint.contains(QStringLiteral("velocityX")));
    CTEST_CHECK(firstPoint.contains(QStringLiteral("pressure")));

    processFor(2);

    const QPointF firstEnd(48.0, 44.0);
    const QPointF secondEnd(82.0, 52.0);
    QTouchEvent touchEnd(QEvent::TouchEnd,
                         testTouchDevice(),
                         Qt::NoModifier,
                         {
                             QEventPoint(0, QEventPoint::State::Released, firstEnd, firstEnd),
                             QEventPoint(1, QEventPoint::State::Released, secondEnd, secondEnd),
                         });
    QCoreApplication::sendEvent(&window, &touchEnd);

    const QVariantMap endEvent = events.lastEvent();
    RUNTIME_COMPARE(endEvent.value(QStringLiteral("type")).toString(), QStringLiteral("touch-event"));
    const QVariantMap endPayload = endEvent.value(QStringLiteral("payload")).toMap();
    RUNTIME_COMPARE(endPayload.value(QStringLiteral("phase")).toString(), QStringLiteral("end"));
    RUNTIME_COMPARE(endPayload.value(QStringLiteral("fingerCount")).toInt(), 2);
    RUNTIME_COMPARE(endPayload.value(QStringLiteral("activeFingerCount")).toInt(), 0);
    RUNTIME_COMPARE(endPayload.value(QStringLiteral("releasedFingerCount")).toInt(), 2);
    CTEST_CHECK(endPayload.value(QStringLiteral("released")).toBool());
    CTEST_CHECK(!endPayload.value(QStringLiteral("cancelled")).toBool());
    CTEST_CHECK(endPayload.value(QStringLiteral("releaseEpochMs")).toLongLong() > 0);
    CTEST_CHECK(endPayload.value(QStringLiteral("pressDurationMs")).toLongLong() >= 0);
    RUNTIME_COMPARE(endPayload.value(QStringLiteral("primaryPointId")).toInt(), 0);
}

void RuntimeServicesTests::runtime_events_hit_test_exposes_ui_hierarchy_metadata()
{
    RuntimeEvents events;
    QQuickWindow window;
    window.setObjectName(QStringLiteral("runtimeWindow"));
    window.setWidth(240);
    window.setHeight(160);

    QQuickItem *content = window.contentItem();
    CTEST_CHECK(content);
    content->setObjectName(QStringLiteral("contentRoot"));

    auto *panel = new QQuickItem(content);
    panel->setObjectName(QStringLiteral("panel"));
    panel->setX(10.0);
    panel->setY(10.0);
    panel->setWidth(140.0);
    panel->setHeight(100.0);
    panel->setZ(1.0);

    auto *button = new QQuickItem(panel);
    button->setObjectName(QStringLiteral("button"));
    button->setX(8.0);
    button->setY(6.0);
    button->setWidth(56.0);
    button->setHeight(32.0);
    button->setZ(3.0);

    events.attachWindow(&window);
    CTEST_WAIT_DEFAULT(events.running());

    const QVariantMap hit = events.hitTestUiAt(20.0, 20.0);
    RUNTIME_COMPARE(hit.value(QStringLiteral("objectName")).toString(), QStringLiteral("button"));
    RUNTIME_COMPARE(hit.value(QStringLiteral("componentName")).toString(), QStringLiteral("button"));
    RUNTIME_COMPARE(hit.value(QStringLiteral("layerKind")).toString(), QStringLiteral("content"));
    RUNTIME_COMPARE(hit.value(QStringLiteral("windowObjectName")).toString(), QStringLiteral("runtimeWindow"));
    RUNTIME_COMPARE(hit.value(QStringLiteral("rootObjectName")).toString(), QStringLiteral("contentRoot"));
    RUNTIME_COMPARE(hit.value(QStringLiteral("z")).toReal(), 3.0);
    CTEST_CHECK(hit.value(QStringLiteral("path")).toString().contains(QStringLiteral("contentRoot")));
    CTEST_CHECK(hit.value(QStringLiteral("path")).toString().contains(QStringLiteral("button")));

    const QVariantList hierarchy = hit.value(QStringLiteral("hierarchy")).toList();
    RUNTIME_COMPARE(hierarchy.size(), 3);
    RUNTIME_COMPARE(hierarchy.constFirst().toMap().value(QStringLiteral("objectName")).toString(),
             QStringLiteral("contentRoot"));
    RUNTIME_COMPARE(hierarchy.constLast().toMap().value(QStringLiteral("objectName")).toString(),
             QStringLiteral("button"));
    RUNTIME_COMPARE(hit.value(QStringLiteral("depth")).toInt(), hierarchy.size() - 1);
}

void RuntimeServicesTests::gesture_events_recognize_hold_drag_and_swipe()
{
    RuntimeEvents events;
    GestureEvents gestures;
    QQuickWindow window;
    window.setWidth(480);
    window.setHeight(320);
    events.attachWindow(&window);
    CTEST_WAIT_DEFAULT(events.running());
    CTEST_CHECK(gestures.attachRuntime(&events));
    CTEST_CHECK(gestures.runtimeAttached());

    gestures.setHoldThresholdMs(60);
    gestures.setDragThresholdPx(10.0);
    gestures.setSwipeThresholdPx(30.0);
    gestures.setSwipeMaxDurationMs(500);
    gestures.resetState();

    QList<QVariantList> touchStartedSpyEvents;
    const auto touchStartedSpyConnection = QObject::connect(&gestures, &GestureEvents::touchStarted,
        [&touchStartedSpyEvents](const QVariantMap &eventData) { touchStartedSpyEvents.append(QVariantList{eventData}); });
    const auto touchStartedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(touchStartedSpyConnection); });
    QList<QVariantList> holdStartedSpyEvents;
    const auto holdStartedSpyConnection = QObject::connect(&gestures, &GestureEvents::holdStarted,
        [&holdStartedSpyEvents](const QVariantMap &eventData) { holdStartedSpyEvents.append(QVariantList{eventData}); });
    const auto holdStartedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(holdStartedSpyConnection); });
    QList<QVariantList> touchEndedSpyEvents;
    const auto touchEndedSpyConnection = QObject::connect(&gestures, &GestureEvents::touchEnded,
        [&touchEndedSpyEvents](const QVariantMap &eventData) { touchEndedSpyEvents.append(QVariantList{eventData}); });
    const auto touchEndedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(touchEndedSpyConnection); });
    QList<QVariantList> dragStartedSpyEvents;
    const auto dragStartedSpyConnection = QObject::connect(&gestures, &GestureEvents::dragStarted,
        [&dragStartedSpyEvents](const QVariantMap &eventData) { dragStartedSpyEvents.append(QVariantList{eventData}); });
    const auto dragStartedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(dragStartedSpyConnection); });
    QList<QVariantList> dragUpdatedSpyEvents;
    const auto dragUpdatedSpyConnection = QObject::connect(&gestures, &GestureEvents::dragUpdated,
        [&dragUpdatedSpyEvents](const QVariantMap &eventData) { dragUpdatedSpyEvents.append(QVariantList{eventData}); });
    const auto dragUpdatedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(dragUpdatedSpyConnection); });
    QList<QVariantList> dragEndedSpyEvents;
    const auto dragEndedSpyConnection = QObject::connect(&gestures, &GestureEvents::dragEnded,
        [&dragEndedSpyEvents](const QVariantMap &eventData) { dragEndedSpyEvents.append(QVariantList{eventData}); });
    const auto dragEndedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(dragEndedSpyConnection); });
    QList<QVariantList> swipeDetectedSpyEvents;
    const auto swipeDetectedSpyConnection = QObject::connect(&gestures, &GestureEvents::swipeDetected,
        [&swipeDetectedSpyEvents](const QVariantMap &eventData) { swipeDetectedSpyEvents.append(QVariantList{eventData}); });
    const auto swipeDetectedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(swipeDetectedSpyConnection); });
    CTEST_CHECK(bool(touchStartedSpyConnection));
    CTEST_CHECK(bool(holdStartedSpyConnection));
    CTEST_CHECK(bool(touchEndedSpyConnection));
    CTEST_CHECK(bool(dragStartedSpyConnection));
    CTEST_CHECK(bool(dragUpdatedSpyConnection));
    CTEST_CHECK(bool(dragEndedSpyConnection));
    CTEST_CHECK(bool(swipeDetectedSpyConnection));

    const QPointF holdPoint(40.0, 28.0);
    QTouchEvent holdBegin(QEvent::TouchBegin,
                          testTouchDevice(),
                          Qt::NoModifier,
                          {QEventPoint(0, QEventPoint::State::Pressed, holdPoint, holdPoint)});
    QCoreApplication::sendEvent(&window, &holdBegin);

    RUNTIME_WAIT_COMPARE(touchStartedSpyEvents.size(), 1);
    const QVariantMap holdStartTouchPayload = touchStartedSpyEvents.constFirst().constFirst().toMap();
    RUNTIME_COMPARE(holdStartTouchPayload.value(QStringLiteral("gestureType")).toString(), QStringLiteral("touchStarted"));
    RUNTIME_COMPARE(holdStartTouchPayload.value(QStringLiteral("sessionId")).toULongLong(), quint64(1));
    RUNTIME_COMPARE(holdStartTouchPayload.value(QStringLiteral("globalX")).toReal(), holdPoint.x());
    RUNTIME_COMPARE(holdStartTouchPayload.value(QStringLiteral("globalY")).toReal(), holdPoint.y());
    CTEST_CHECK(holdStartTouchPayload.contains(QStringLiteral("ui")));
    const QVariantMap holdStartUi = holdStartTouchPayload.value(QStringLiteral("ui")).toMap();
    RUNTIME_COMPARE(holdStartUi.value(QStringLiteral("layerKind")).toString(), QStringLiteral("content"));
    CTEST_CHECK(holdStartUi.value(QStringLiteral("hierarchy")).toList().size() >= 1);
    CTEST_CHECK(!holdStartUi.value(QStringLiteral("componentName")).toString().isEmpty());

    CTEST_WAIT_VERIFY(runtimeEqual(holdStartedSpyEvents.size(), 1), 400);
    const QVariantMap holdPayload = holdStartedSpyEvents.constFirst().constFirst().toMap();
    RUNTIME_COMPARE(holdPayload.value(QStringLiteral("gestureType")).toString(), QStringLiteral("holdStarted"));
    RUNTIME_COMPARE(holdPayload.value(QStringLiteral("interactionKind")).toString(), QStringLiteral("hold"));
    CTEST_CHECK_MESSAGE(holdPayload.value(QStringLiteral("durationMs")).toLongLong() >= gestures.holdThresholdMs(),
             "A hold must not start before the configured duration has elapsed.");
    CTEST_CHECK(holdPayload.value(QStringLiteral("holdActive")).toBool());

    QTouchEvent holdEnd(QEvent::TouchEnd,
                        testTouchDevice(),
                        Qt::NoModifier,
                        {QEventPoint(0, QEventPoint::State::Released, holdPoint, holdPoint)});
    QCoreApplication::sendEvent(&window, &holdEnd);

    RUNTIME_WAIT_COMPARE(touchEndedSpyEvents.size(), 1);
    const QVariantMap holdEndPayload = touchEndedSpyEvents.constFirst().constFirst().toMap();
    RUNTIME_COMPARE(holdEndPayload.value(QStringLiteral("interactionKind")).toString(), QStringLiteral("hold"));
    RUNTIME_COMPARE(holdEndPayload.value(QStringLiteral("gestureType")).toString(), QStringLiteral("touchEnded"));

    const QPointF swipeBeginPoint(52.0, 36.0);
    const QPointF swipeUpdatePoint(94.0, 40.0);
    const QPointF swipeEndPoint(116.0, 42.0);

    QTouchEvent swipeBegin(QEvent::TouchBegin,
                           testTouchDevice(),
                           Qt::NoModifier,
                           {QEventPoint(0, QEventPoint::State::Pressed, swipeBeginPoint, swipeBeginPoint)});
    QCoreApplication::sendEvent(&window, &swipeBegin);
    RUNTIME_WAIT_COMPARE(touchStartedSpyEvents.size(), 2);

    QTouchEvent swipeUpdate(QEvent::TouchUpdate,
                            testTouchDevice(),
                            Qt::NoModifier,
                            {QEventPoint(0, QEventPoint::State::Updated, swipeUpdatePoint, swipeUpdatePoint)});
    QCoreApplication::sendEvent(&window, &swipeUpdate);

    RUNTIME_WAIT_COMPARE(dragStartedSpyEvents.size(), 1);
    CTEST_WAIT_DEFAULT(dragUpdatedSpyEvents.size() >= 1);
    const QVariantMap dragStartPayload = dragStartedSpyEvents.constFirst().constFirst().toMap();
    RUNTIME_COMPARE(dragStartPayload.value(QStringLiteral("gestureType")).toString(), QStringLiteral("dragStarted"));
    RUNTIME_COMPARE(dragStartPayload.value(QStringLiteral("sessionId")).toULongLong(), quint64(2));
    RUNTIME_COMPARE(dragStartPayload.value(QStringLiteral("dominantAxis")).toString(), QStringLiteral("x"));
    CTEST_CHECK(dragStartPayload.value(QStringLiteral("totalDeltaX")).toReal() > 0.0);
    CTEST_CHECK(qAbs(dragStartPayload.value(QStringLiteral("totalDeltaY")).toReal()) < 10.0);

    QTouchEvent swipeEnd(QEvent::TouchEnd,
                         testTouchDevice(),
                         Qt::NoModifier,
                         {QEventPoint(0, QEventPoint::State::Released, swipeEndPoint, swipeEndPoint)});
    QCoreApplication::sendEvent(&window, &swipeEnd);

    RUNTIME_WAIT_COMPARE(dragEndedSpyEvents.size(), 1);
    RUNTIME_WAIT_COMPARE(swipeDetectedSpyEvents.size(), 1);
    RUNTIME_WAIT_COMPARE(touchEndedSpyEvents.size(), 2);

    const QVariantMap swipePayload = swipeDetectedSpyEvents.constFirst().constFirst().toMap();
    RUNTIME_COMPARE(swipePayload.value(QStringLiteral("gestureType")).toString(), QStringLiteral("swipeDetected"));
    RUNTIME_COMPARE(swipePayload.value(QStringLiteral("interactionKind")).toString(), QStringLiteral("swipe"));
    RUNTIME_COMPARE(swipePayload.value(QStringLiteral("swipeDirection")).toString(), QStringLiteral("leftToRight"));
    CTEST_CHECK(swipePayload.value(QStringLiteral("totalDeltaX")).toReal() >= 60.0);
    CTEST_CHECK(qAbs(swipePayload.value(QStringLiteral("totalDeltaY")).toReal()) < 10.0);
    CTEST_CHECK(swipePayload.value(QStringLiteral("velocityX")).toReal() > 0.0);

    const QVariantMap swipeEndPayload = touchEndedSpyEvents.at(1).at(0).toMap();
    RUNTIME_COMPARE(swipeEndPayload.value(QStringLiteral("interactionKind")).toString(), QStringLiteral("swipe"));
    RUNTIME_COMPARE(swipeEndPayload.value(QStringLiteral("directionX")).toString(), QStringLiteral("positive"));
    RUNTIME_COMPARE(swipeEndPayload.value(QStringLiteral("dominantAxis")).toString(), QStringLiteral("x"));
}

void RuntimeServicesTests::gesture_events_classify_mobile_press_scroll_and_multitouch()
{
    RuntimeEvents events;
    GestureEvents gestures;
    QQuickWindow window;
    window.setWidth(480);
    window.setHeight(320);
    events.attachWindow(&window);
    CTEST_WAIT_DEFAULT(events.running());
    CTEST_CHECK(gestures.attachRuntime(&events));
    CTEST_CHECK(gestures.runtimeAttached());

    gestures.setHoldThresholdMs(500);
    gestures.setDragThresholdPx(10.0);
    gestures.setSwipeThresholdPx(96.0);
    CTEST_CHECK(gestures.setProperty("scrollThresholdPx", 10.0));
    gestures.resetState();

    QList<QVariantList> pressStartedSpyEvents;
    const auto pressStartedSpyConnection = QObject::connect(&gestures, &GestureEvents::pressStarted,
        [&pressStartedSpyEvents](const QVariantMap &eventData) { pressStartedSpyEvents.append(QVariantList{eventData}); });
    const auto pressStartedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(pressStartedSpyConnection); });
    QList<QVariantList> pressEndedSpyEvents;
    const auto pressEndedSpyConnection = QObject::connect(&gestures, &GestureEvents::pressEnded,
        [&pressEndedSpyEvents](const QVariantMap &eventData) { pressEndedSpyEvents.append(QVariantList{eventData}); });
    const auto pressEndedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(pressEndedSpyConnection); });
    QList<QVariantList> scrollStartedSpyEvents;
    const auto scrollStartedSpyConnection = QObject::connect(&gestures, &GestureEvents::scrollStarted,
        [&scrollStartedSpyEvents](const QVariantMap &eventData) { scrollStartedSpyEvents.append(QVariantList{eventData}); });
    const auto scrollStartedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(scrollStartedSpyConnection); });
    QList<QVariantList> scrollUpdatedSpyEvents;
    const auto scrollUpdatedSpyConnection = QObject::connect(&gestures, &GestureEvents::scrollUpdated,
        [&scrollUpdatedSpyEvents](const QVariantMap &eventData) { scrollUpdatedSpyEvents.append(QVariantList{eventData}); });
    const auto scrollUpdatedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(scrollUpdatedSpyConnection); });
    QList<QVariantList> scrollEndedSpyEvents;
    const auto scrollEndedSpyConnection = QObject::connect(&gestures, &GestureEvents::scrollEnded,
        [&scrollEndedSpyEvents](const QVariantMap &eventData) { scrollEndedSpyEvents.append(QVariantList{eventData}); });
    const auto scrollEndedSpyDisconnect = qScopeGuard([&] { QObject::disconnect(scrollEndedSpyConnection); });
    CTEST_CHECK(bool(pressStartedSpyConnection));
    CTEST_CHECK(bool(pressEndedSpyConnection));
    CTEST_CHECK(bool(scrollStartedSpyConnection));
    CTEST_CHECK(bool(scrollUpdatedSpyConnection));
    CTEST_CHECK(bool(scrollEndedSpyConnection));

    const QPointF pressPoint(40.0, 28.0);
    QTouchEvent pressBegin(QEvent::TouchBegin,
                           testTouchDevice(),
                           Qt::NoModifier,
                           {QEventPoint(0, QEventPoint::State::Pressed, pressPoint, pressPoint)});
    QCoreApplication::sendEvent(&window, &pressBegin);
    RUNTIME_WAIT_COMPARE(pressStartedSpyEvents.size(), 1);

    processFor(2);

    QTouchEvent pressEnd(QEvent::TouchEnd,
                         testTouchDevice(),
                         Qt::NoModifier,
                         {QEventPoint(0, QEventPoint::State::Released, pressPoint, pressPoint)});
    QCoreApplication::sendEvent(&window, &pressEnd);
    RUNTIME_WAIT_COMPARE(pressEndedSpyEvents.size(), 1);

    const QVariantMap pressEnded = pressEndedSpyEvents.constFirst().constFirst().toMap();
    RUNTIME_COMPARE(pressEnded.value(QStringLiteral("gestureType")).toString(), QStringLiteral("pressEnded"));
    RUNTIME_COMPARE(pressEnded.value(QStringLiteral("classification")).toString(), QStringLiteral("press"));
    RUNTIME_COMPARE(pressEnded.value(QStringLiteral("interactionKind")).toString(), QStringLiteral("press"));
    CTEST_CHECK(pressEnded.value(QStringLiteral("released")).toBool());
    CTEST_CHECK(pressEnded.value(QStringLiteral("releaseEpochMs")).toLongLong() > 0);
    CTEST_CHECK(pressEnded.value(QStringLiteral("pressDurationMs")).toLongLong() >= 0);
    RUNTIME_COMPARE(pressEnded.value(QStringLiteral("fingerCount")).toInt(), 1);
    RUNTIME_COMPARE(pressEnded.value(QStringLiteral("maximumFingerCount")).toInt(), 1);

    const QPointF scrollBeginPoint(64.0, 34.0);
    const QPointF scrollUpdatePoint(67.0, 78.0);
    const QPointF scrollEndPoint(69.0, 104.0);
    QTouchEvent scrollBeginEvent(QEvent::TouchBegin,
                                 testTouchDevice(),
                                 Qt::NoModifier,
                                 {QEventPoint(0, QEventPoint::State::Pressed, scrollBeginPoint, scrollBeginPoint)});
    QCoreApplication::sendEvent(&window, &scrollBeginEvent);
    RUNTIME_WAIT_COMPARE(pressStartedSpyEvents.size(), 2);

    QTouchEvent scrollUpdateEvent(QEvent::TouchUpdate,
                                  testTouchDevice(),
                                  Qt::NoModifier,
                                  {QEventPoint(0, QEventPoint::State::Updated, scrollUpdatePoint, scrollUpdatePoint)});
    QCoreApplication::sendEvent(&window, &scrollUpdateEvent);
    RUNTIME_WAIT_COMPARE(scrollStartedSpyEvents.size(), 1);
    CTEST_WAIT_DEFAULT(scrollUpdatedSpyEvents.size() >= 1);

    const QVariantMap scrollStarted = scrollStartedSpyEvents.constFirst().constFirst().toMap();
    RUNTIME_COMPARE(scrollStarted.value(QStringLiteral("gestureType")).toString(), QStringLiteral("scrollStarted"));
    RUNTIME_COMPARE(scrollStarted.value(QStringLiteral("classification")).toString(), QStringLiteral("scroll"));
    RUNTIME_COMPARE(scrollStarted.value(QStringLiteral("interactionKind")).toString(), QStringLiteral("scroll"));
    RUNTIME_COMPARE(scrollStarted.value(QStringLiteral("scrollAxis")).toString(), QStringLiteral("y"));
    RUNTIME_COMPARE(scrollStarted.value(QStringLiteral("scrollDirection")).toString(), QStringLiteral("topToBottom"));
    RUNTIME_COMPARE(scrollStarted.value(QStringLiteral("fingerCount")).toInt(), 1);
    RUNTIME_COMPARE(scrollStarted.value(QStringLiteral("maximumFingerCount")).toInt(), 1);

    QTouchEvent scrollEndEvent(QEvent::TouchEnd,
                               testTouchDevice(),
                               Qt::NoModifier,
                               {QEventPoint(0, QEventPoint::State::Released, scrollEndPoint, scrollEndPoint)});
    QCoreApplication::sendEvent(&window, &scrollEndEvent);
    RUNTIME_WAIT_COMPARE(scrollEndedSpyEvents.size(), 1);

    const QVariantMap scrollEnded = scrollEndedSpyEvents.constFirst().constFirst().toMap();
    RUNTIME_COMPARE(scrollEnded.value(QStringLiteral("gestureType")).toString(), QStringLiteral("scrollEnded"));
    RUNTIME_COMPARE(scrollEnded.value(QStringLiteral("classification")).toString(), QStringLiteral("scroll"));
    RUNTIME_COMPARE(scrollEnded.value(QStringLiteral("scrollAxis")).toString(), QStringLiteral("y"));
    CTEST_CHECK(scrollEnded.value(QStringLiteral("released")).toBool());
    CTEST_CHECK(scrollEnded.value(QStringLiteral("pressDurationMs")).toLongLong() >= 0);

    const QPointF firstPoint(110.0, 40.0);
    const QPointF secondPoint(146.0, 42.0);
    QTouchEvent multiBegin(QEvent::TouchBegin,
                           testTouchDevice(),
                           Qt::NoModifier,
                           {
                               QEventPoint(0, QEventPoint::State::Pressed, firstPoint, firstPoint),
                               QEventPoint(1, QEventPoint::State::Pressed, secondPoint, secondPoint),
                           });
    QCoreApplication::sendEvent(&window, &multiBegin);
    RUNTIME_WAIT_COMPARE(pressStartedSpyEvents.size(), 3);

    const QVariantMap multiPress = pressStartedSpyEvents.at(2).at(0).toMap();
    RUNTIME_COMPARE(multiPress.value(QStringLiteral("fingerCount")).toInt(), 2);
    RUNTIME_COMPARE(multiPress.value(QStringLiteral("activeFingerCount")).toInt(), 2);
    RUNTIME_COMPARE(multiPress.value(QStringLiteral("maximumFingerCount")).toInt(), 2);
    CTEST_CHECK(multiPress.value(QStringLiteral("multiTouch")).toBool());
}

void RuntimeServicesTests::runtime_events_window_replacement_survives_old_window_destruction()
{
    RuntimeEvents events;

    auto *firstWindow = new QQuickWindow;
    firstWindow->setWidth(640);
    firstWindow->setHeight(360);
    events.attachWindow(firstWindow);
    CTEST_WAIT_DEFAULT(events.running());

    auto *secondWindow = new QQuickWindow;
    secondWindow->setWidth(640);
    secondWindow->setHeight(360);
    events.attachWindow(secondWindow);
    CTEST_WAIT_DEFAULT(events.daemonHealth().value(QStringLiteral("attachedWindow")).toBool());

    delete firstWindow;
    QCoreApplication::processEvents();
    CTEST_WAIT_DEFAULT(events.daemonHealth().value(QStringLiteral("attachedWindow")).toBool());

    const quint64 sequenceBefore = events.eventSequence();
    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_R, Qt::NoModifier, QStringLiteral("r"));
    QCoreApplication::sendEvent(secondWindow, &keyPress);
    CTEST_WAIT_DEFAULT(events.eventSequence() > sequenceBefore);

    delete secondWindow;
    QCoreApplication::processEvents();
    CTEST_WAIT_DEFAULT(!events.daemonHealth().value(QStringLiteral("attachedWindow")).toBool());
}

void RuntimeServicesTests::render_monitor_counts_frames_when_swapped()
{
    RenderingMonitor monitor;
    QQuickWindow window;
    monitor.setFrameSampleCapacity(64);
    monitor.setDroppedFrameThresholdMs(1.0);
    monitor.attachWindow(&window);
    CTEST_CHECK(monitor.active());
    RUNTIME_COMPARE(monitor.frameCount(), 0u);

    CTEST_CHECK(QMetaObject::invokeMethod(&monitor, "handleFrameSwapped", Qt::DirectConnection));
    processFor(5);
    CTEST_CHECK(QMetaObject::invokeMethod(&monitor, "handleFrameSwapped", Qt::DirectConnection));
    CTEST_CHECK(monitor.frameCount() >= 2);
    CTEST_CHECK(monitor.lastFrameMs() >= 0.0);
    CTEST_CHECK(monitor.fps() >= 0.0);
    CTEST_CHECK(monitor.avgFrameMs() >= 0.0);
    CTEST_CHECK(monitor.p95FrameMs() >= 0.0);
    CTEST_CHECK(monitor.p99FrameMs() >= 0.0);
    CTEST_CHECK(monitor.recentSampleCount() >= 1);
    CTEST_CHECK(monitor.droppedFrameCount() >= 1);

    const QVariantMap snapshot = monitor.performanceSnapshot();
    RUNTIME_COMPARE(snapshot.value(QStringLiteral("schema")).toString(), QStringLiteral("lvrs.performance.v1"));
    RUNTIME_COMPARE(snapshot.value(QStringLiteral("component")).toString(), QStringLiteral("RenderMonitor"));
    CTEST_CHECK(snapshot.contains(QStringLiteral("p95FrameMs")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("p99FrameMs")));
    CTEST_CHECK(snapshot.contains(QStringLiteral("droppedFrameCount")));

    monitor.reset();
    RUNTIME_COMPARE(monitor.frameCount(), 0u);
    RUNTIME_COMPARE(monitor.lastFrameMs(), 0.0);
    RUNTIME_COMPARE(monitor.avgFrameMs(), 0.0);
    RUNTIME_COMPARE(monitor.recentSampleCount(), 0);

    monitor.stop();
    CTEST_CHECK(!monitor.active());
    monitor.start();
    CTEST_CHECK(monitor.active());
    monitor.attachWindow(static_cast<QObject *>(nullptr));
    CTEST_CHECK(!monitor.active());
}

void RuntimeServicesTests::render_monitor_active_signal_and_destroy_path()
{
    RenderingMonitor monitor;
    QList<QVariantList> activeSpyEvents;
    const auto activeSpyConnection = QObject::connect(&monitor, &RenderingMonitor::activeChanged,
        [&activeSpyEvents]() { activeSpyEvents.append(QVariantList{}); });
    const auto activeSpyDisconnect = qScopeGuard([&] { QObject::disconnect(activeSpyConnection); });
    QList<QVariantList> statsSpyEvents;
    const auto statsSpyConnection = QObject::connect(&monitor, &RenderingMonitor::statsChanged,
        [&statsSpyEvents]() { statsSpyEvents.append(QVariantList{}); });
    const auto statsSpyDisconnect = qScopeGuard([&] { QObject::disconnect(statsSpyConnection); });
    QList<QVariantList> thresholdSpyEvents;
    const auto thresholdSpyConnection = QObject::connect(&monitor, &RenderingMonitor::droppedFrameThresholdMsChanged,
        [&thresholdSpyEvents]() { thresholdSpyEvents.append(QVariantList{}); });
    const auto thresholdSpyDisconnect = qScopeGuard([&] { QObject::disconnect(thresholdSpyConnection); });
    QList<QVariantList> capacitySpyEvents;
    const auto capacitySpyConnection = QObject::connect(&monitor, &RenderingMonitor::frameSampleCapacityChanged,
        [&capacitySpyEvents]() { capacitySpyEvents.append(QVariantList{}); });
    const auto capacitySpyDisconnect = qScopeGuard([&] { QObject::disconnect(capacitySpyConnection); });
    CTEST_CHECK(bool(activeSpyConnection));
    CTEST_CHECK(bool(statsSpyConnection));
    CTEST_CHECK(bool(thresholdSpyConnection));
    CTEST_CHECK(bool(capacitySpyConnection));

    monitor.setDroppedFrameThresholdMs(2.0);
    monitor.setFrameSampleCapacity(32);
    RUNTIME_COMPARE(thresholdSpyEvents.size(), 1);
    RUNTIME_COMPARE(capacitySpyEvents.size(), 1);

    auto *window = new QQuickWindow;
    monitor.attachWindow(window);
    CTEST_CHECK(monitor.active());
    RUNTIME_COMPARE(activeSpyEvents.size(), 1);
    CTEST_CHECK(statsSpyEvents.size() >= 1);

    CTEST_CHECK(QMetaObject::invokeMethod(&monitor, "handleFrameSwapped", Qt::DirectConnection));
    const quint64 frameCountBeforeStop = monitor.frameCount();
    CTEST_CHECK(frameCountBeforeStop >= 1);

    monitor.stop();
    RUNTIME_COMPARE(activeSpyEvents.size(), 2);
    CTEST_CHECK(!monitor.active());
    CTEST_CHECK(QMetaObject::invokeMethod(&monitor, "handleFrameSwapped", Qt::DirectConnection));
    RUNTIME_COMPARE(monitor.frameCount(), frameCountBeforeStop);

    monitor.start();
    RUNTIME_COMPARE(activeSpyEvents.size(), 3);
    CTEST_CHECK(monitor.active());

    delete window;
    QCoreApplication::processEvents();
    CTEST_WAIT_DEFAULT(!monitor.active());
    RUNTIME_COMPARE(activeSpyEvents.size(), 4);
}

void RuntimeServicesTests::debug_logger_enabled_signal_and_message_format()
{
    DebugLogger logger;
    QList<QVariantList> enabledSpyEvents;
    const auto enabledSpyConnection = QObject::connect(&logger, &DebugLogger::enabledChanged,
        [&enabledSpyEvents]() { enabledSpyEvents.append(QVariantList{}); });
    const auto enabledSpyDisconnect = qScopeGuard([&] { QObject::disconnect(enabledSpyConnection); });
    CTEST_CHECK(bool(enabledSpyConnection));

    logger.setEnabled(false);
    RUNTIME_COMPARE(enabledSpyEvents.size(), 0);
    logger.setEnabled(true);
    RUNTIME_COMPARE(enabledSpyEvents.size(), 1);
    logger.setEnabled(true);
    RUNTIME_COMPARE(enabledSpyEvents.size(), 1);

    logger.log(QString(), QString(), 42);
    logger.warn(QStringLiteral("Runtime"), QStringLiteral("warning"));
    logger.error(QString(), QStringLiteral("failure"));

    const QVariantList entries = logger.entries();
    RUNTIME_COMPARE(entries.size(), 3);

    const QVariantMap logEntry = entries.at(0).toMap();
    RUNTIME_COMPARE(logEntry.value(QStringLiteral("level")).toString(), QStringLiteral("LOG"));
    RUNTIME_COMPARE(logEntry.value(QStringLiteral("component")).toString(), QStringLiteral("Unknown"));
    RUNTIME_COMPARE(logEntry.value(QStringLiteral("event")).toString(), QStringLiteral("event"));
    CTEST_CHECK(logEntry.value(QStringLiteral("message")).toString().contains(QStringLiteral("Unknown.event")));

    const QVariantMap warnEntry = entries.at(1).toMap();
    RUNTIME_COMPARE(warnEntry.value(QStringLiteral("level")).toString(), QStringLiteral("WARN"));
    RUNTIME_COMPARE(warnEntry.value(QStringLiteral("component")).toString(), QStringLiteral("Runtime"));
    RUNTIME_COMPARE(warnEntry.value(QStringLiteral("event")).toString(), QStringLiteral("warning"));

    const QVariantMap errorEntry = entries.at(2).toMap();
    RUNTIME_COMPARE(errorEntry.value(QStringLiteral("level")).toString(), QStringLiteral("ERROR"));
    RUNTIME_COMPARE(errorEntry.value(QStringLiteral("component")).toString(), QStringLiteral("Unknown"));
    RUNTIME_COMPARE(errorEntry.value(QStringLiteral("event")).toString(), QStringLiteral("failure"));

    logger.setEnabled(false);
    RUNTIME_COMPARE(enabledSpyEvents.size(), 2);
    logger.log(QStringLiteral("PostDisable"), QStringLiteral("ignored"));
    RUNTIME_COMPARE(logger.entries().size(), 3);
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    RuntimeServicesTests tests;
    runner.run("runtime_events_measurement_boundaries", [&] { tests.runtime_events_measurement_boundaries(); });
    runner.run("runtime_events_daemon_health_and_recent_log", [&] { tests.runtime_events_daemon_health_and_recent_log(); });
    runner.run("runtime_events_idle_and_reset_signal_contract", [&] { tests.runtime_events_idle_and_reset_signal_contract(); });
    runner.run("runtime_events_context_menu_signal_contract", [&] { tests.runtime_events_context_menu_signal_contract(); });
    runner.run("runtime_events_single_input_event_is_counted_once", [&] { tests.runtime_events_single_input_event_is_counted_once(); });
    runner.run("runtime_events_touch_input_normalizes_to_pointer_contract", [&] { tests.runtime_events_touch_input_normalizes_to_pointer_contract(); });
    runner.run("runtime_events_touch_payload_exposes_native_point_details", [&] { tests.runtime_events_touch_payload_exposes_native_point_details(); });
    runner.run("runtime_events_hit_test_exposes_ui_hierarchy_metadata", [&] { tests.runtime_events_hit_test_exposes_ui_hierarchy_metadata(); });
    runner.run("gesture_events_recognize_hold_drag_and_swipe", [&] { tests.gesture_events_recognize_hold_drag_and_swipe(); });
    runner.run("gesture_events_classify_mobile_press_scroll_and_multitouch", [&] { tests.gesture_events_classify_mobile_press_scroll_and_multitouch(); });
    runner.run("runtime_events_window_replacement_survives_old_window_destruction", [&] { tests.runtime_events_window_replacement_survives_old_window_destruction(); });
    runner.run("render_monitor_counts_frames_when_swapped", [&] { tests.render_monitor_counts_frames_when_swapped(); });
    runner.run("render_monitor_active_signal_and_destroy_path", [&] { tests.render_monitor_active_signal_and_destroy_path(); });
    runner.run("debug_logger_enabled_signal_and_message_format", [&] { tests.debug_logger_enabled_signal_and_message_format(); });
    return runner.result();
}
