#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QMouseEvent>
#include <QInputDevice>
#include <QPointingDevice>
#include <QQuickWindow>
#include <QTouchEvent>
#include <QtPlugin>

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class EventListenerTests : public QObject
{
    Q_OBJECT

private slots:
    void click_trigger();
    void global_context_requested_trigger();
    void application_window_global_context_signal();
    void context_menu_dismisses_on_global_press_outside();
    void gesture_triggers_receive_touch_and_swipe();
    void mobile_press_and_scroll_triggers_are_distinct();
};

namespace {

const QPointingDevice *eventListenerTouchDevice()
{
    static const QPointingDevice *device = new QPointingDevice(
        QStringLiteral("LVRS EventListener Touchscreen"),
        2,
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

void EventListenerTests::click_trigger()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS 1.0 as LV

Item {
    id: root
    width: 100
    height: 40
    property int count: 0

    Rectangle {
        anchors.fill: parent
        LV.EventListener {
            objectName: "listener"
            trigger: "clicked"
            action: () => root.count++
        }
    }
}
)";

    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object);

    QObject *listener = object->findChild<QObject *>("listener");
    QVERIFY(listener);
    QVariant payload;
    QVERIFY(QMetaObject::invokeMethod(listener, "fire", Q_ARG(QVariant, payload)));
    QCOMPARE(object->property("count").toInt(), 1);
}

void EventListenerTests::global_context_requested_trigger()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root
    width: 240
    height: 140
    autoAttachRuntimeEvents: true
    visible: false
    title: "EventListenerGlobalContextTest"

    property int contextCount: 0
    property bool runtimeRunning: LV.RuntimeEvents.running
    property string lastSource: ""
    property int lastReason: -2
    property bool hasInputState: false
    property bool hasPointerState: false

    function resetMonitor() {
        contextCount = 0
        lastSource = ""
        lastReason = -2
        hasInputState = false
        hasPointerState = false
        LV.RuntimeEvents.resetCounters()
    }

    LV.EventListener {
        trigger: "globalContextRequested"
        enabled: true
        includeUiHit: true
        includeInputState: true
        action: function(mouse) {
            root.contextCount += 1
            root.lastSource = mouse.source || ""
            root.lastReason = mouse.reason === undefined ? -2 : mouse.reason
            root.hasInputState = !!(mouse.input && mouse.input.activeModifierNames !== undefined)
            root.hasPointerState = !!(mouse.input && mouse.input.pointerUi && mouse.input.pointerUi.objectName !== undefined)
        }
    }
}
)";

    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object);

    QTRY_VERIFY(object->property("runtimeRunning").toBool());
    QVERIFY(QMetaObject::invokeMethod(object.data(), "resetMonitor"));

    auto *window = qobject_cast<QQuickWindow *>(object.data());
    QVERIFY(window);

    const QPointF p(24.0, 18.0);
    QMouseEvent mousePress(QEvent::MouseButtonPress,
                           p,
                           p,
                           p,
                           Qt::RightButton,
                           Qt::RightButton,
                           Qt::NoModifier);
    QCoreApplication::sendEvent(window, &mousePress);
    QTRY_VERIFY(object->property("contextCount").toInt() >= 1);
    QCOMPARE(object->property("lastSource").toString(), QStringLiteral("mouse"));
    QVERIFY(object->property("hasInputState").toBool());
    QVERIFY(object->property("hasPointerState").toBool());

    const QPoint local(34, 22);
    const QPoint global(460, 360);
    QContextMenuEvent contextEvent(QContextMenuEvent::Keyboard, local, global, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &contextEvent);
    QTRY_VERIFY(object->property("contextCount").toInt() >= 2);
    QCOMPARE(object->property("lastSource").toString(), QStringLiteral("context"));
    QCOMPARE(object->property("lastReason").toInt(), static_cast<int>(QContextMenuEvent::Keyboard));
}

void EventListenerTests::application_window_global_context_signal()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root
    width: 240
    height: 140
    autoAttachRuntimeEvents: true
    globalEventListenersEnabled: true
    visible: false
    title: "ApplicationWindowGlobalSignalTest"

    property int contextCount: 0
    property bool hasUiPayload: false
    property real lastGlobalX: -1
    property real lastGlobalY: -1

    onGlobalContextEvent: function(eventData) {
        contextCount += 1
        hasUiPayload = !!(eventData && eventData.ui && eventData.ui.objectName !== undefined)
        lastGlobalX = eventData && eventData.globalX !== undefined ? eventData.globalX : -1
        lastGlobalY = eventData && eventData.globalY !== undefined ? eventData.globalY : -1
    }
}
)";

    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object);

    auto *window = qobject_cast<QQuickWindow *>(object.data());
    QVERIFY(window);

    const QPointF p(26.0, 19.0);
    QMouseEvent mousePress(QEvent::MouseButtonPress,
                           p,
                           p,
                           p,
                           Qt::RightButton,
                           Qt::RightButton,
                           Qt::NoModifier);
    QCoreApplication::sendEvent(window, &mousePress);

    QTRY_VERIFY(object->property("contextCount").toInt() >= 1);
    QVERIFY(object->property("hasUiPayload").toBool());
    QVERIFY(object->property("lastGlobalX").toReal() >= 0.0);
    QVERIFY(object->property("lastGlobalY").toReal() >= 0.0);
}

void EventListenerTests::context_menu_dismisses_on_global_press_outside()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root
    width: 260
    height: 170
    autoAttachRuntimeEvents: true
    visible: true
    title: "ContextMenuDismissTest"

    property bool menuOpened: demoMenu.opened
    property int closeCount: 0

    LV.ContextMenu {
        id: demoMenu
        objectName: "demoMenu"
        items: [{ label: "Inspect", showChevron: false }]
        onClosed: root.closeCount += 1
    }

    Component.onCompleted: Qt.callLater(function() {
        demoMenu.openAt(24, 20)
    })
}
)";

    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object);

    auto *window = qobject_cast<QQuickWindow *>(object.data());
    QVERIFY(window);
    QTRY_VERIFY(object->property("menuOpened").toBool());

    const QPointF outsidePoint(230.0, 150.0);
    QMouseEvent outsidePress(QEvent::MouseButtonPress,
                             outsidePoint,
                             outsidePoint,
                             outsidePoint,
                             Qt::LeftButton,
                             Qt::LeftButton,
                             Qt::NoModifier);
    QCoreApplication::sendEvent(window, &outsidePress);

    QTRY_VERIFY(!object->property("menuOpened").toBool());
    QVERIFY(object->property("closeCount").toInt() >= 1);
}

void EventListenerTests::gesture_triggers_receive_touch_and_swipe()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root
    width: 260
    height: 180
    autoAttachRuntimeEvents: true
    visible: false
    title: "EventListenerGestureTest"

    property int touchStartCount: 0
    property int swipeCount: 0
    property real lastStartX: -1
    property real lastStartY: -1
    property string lastSwipeDirection: ""
    property real lastSwipeDx: 0
    property real lastSwipeDy: 0
    property string lastUiObjectName: ""
    property string lastUiLayerKind: ""
    property string lastUiComponentName: ""
    property int lastUiHierarchyDepth: -1
    property string lastUiHitPath: ""

    Rectangle {
        id: gestureSurface
        objectName: "gestureSurfaceObject"
        anchors.fill: parent
        color: "transparent"
        z: 1
    }

    LV.EventListener {
        trigger: "touchStarted"
        action: function(eventData) {
            root.touchStartCount += 1
            root.lastStartX = eventData.globalX
            root.lastStartY = eventData.globalY
            const ui = eventData.ui || ({})
            root.lastUiObjectName = ui.objectName || ""
            root.lastUiLayerKind = ui.layerKind || ""
            root.lastUiComponentName = ui.componentName || ""
            root.lastUiHierarchyDepth = ui.depth === undefined ? -1 : ui.depth
            root.lastUiHitPath = ui.hitPath || ""
        }
    }

    LV.EventListener {
        trigger: "swipeDetected"
        action: function(eventData) {
            root.swipeCount += 1
            root.lastSwipeDirection = eventData.swipeDirection || ""
            root.lastSwipeDx = eventData.totalDeltaX || 0
            root.lastSwipeDy = eventData.totalDeltaY || 0
        }
    }
}
)";

    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object);

    auto *window = qobject_cast<QQuickWindow *>(object.data());
    QVERIFY(window);

    const QPointF beginPoint(42.0, 30.0);
    const QPointF updatePoint(88.0, 34.0);
    const QPointF endPoint(112.0, 36.0);

    QTouchEvent touchBegin(QEvent::TouchBegin,
                           eventListenerTouchDevice(),
                           Qt::NoModifier,
                           {QEventPoint(0, QEventPoint::State::Pressed, beginPoint, beginPoint)});
    QCoreApplication::sendEvent(window, &touchBegin);

    QTRY_COMPARE(object->property("touchStartCount").toInt(), 1);
    QCOMPARE(object->property("lastStartX").toReal(), beginPoint.x());
    QCOMPARE(object->property("lastStartY").toReal(), beginPoint.y());
    QVERIFY(!object->property("lastUiObjectName").toString().isEmpty());
    QVERIFY(object->property("lastUiObjectName").toString() != QStringLiteral("unknown"));
    QCOMPARE(object->property("lastUiLayerKind").toString(), QStringLiteral("content"));
    QVERIFY(!object->property("lastUiComponentName").toString().isEmpty());
    QVERIFY(object->property("lastUiHierarchyDepth").toInt() >= 1);
    QVERIFY(!object->property("lastUiHitPath").toString().isEmpty());

    QTouchEvent touchUpdate(QEvent::TouchUpdate,
                            eventListenerTouchDevice(),
                            Qt::NoModifier,
                            {QEventPoint(0, QEventPoint::State::Updated, updatePoint, updatePoint)});
    QCoreApplication::sendEvent(window, &touchUpdate);

    QTouchEvent touchEnd(QEvent::TouchEnd,
                         eventListenerTouchDevice(),
                         Qt::NoModifier,
                         {QEventPoint(0, QEventPoint::State::Released, endPoint, endPoint)});
    QCoreApplication::sendEvent(window, &touchEnd);

    QTRY_COMPARE(object->property("swipeCount").toInt(), 1);
    QCOMPARE(object->property("lastSwipeDirection").toString(), QStringLiteral("leftToRight"));
    QVERIFY(object->property("lastSwipeDx").toReal() > 0.0);
    QVERIFY(qAbs(object->property("lastSwipeDy").toReal()) < 12.0);
}

void EventListenerTests::mobile_press_and_scroll_triggers_are_distinct()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS 1.0 as LV

LV.ApplicationWindow {
    id: root
    width: 260
    height: 180
    autoAttachRuntimeEvents: true
    visible: false
    title: "EventListenerMobileClassificationTest"

    Component.onCompleted: {
        LV.GestureEvents.holdThresholdMs = 500
        LV.GestureEvents.dragThresholdPx = 10
        LV.GestureEvents.scrollThresholdPx = 10
        LV.GestureEvents.swipeThresholdPx = 96
        LV.GestureEvents.resetState()
    }

    property int pressStartCount: 0
    property int pressEndCount: 0
    property int scrollStartCount: 0
    property int scrollEndCount: 0
    property string lastPressKind: ""
    property string lastScrollAxis: ""
    property string lastScrollDirection: ""
    property int lastFingerCount: 0
    property int lastMaximumFingerCount: 0
    property bool lastReleased: false
    property real lastPressDurationMs: -1

    LV.EventListener {
        trigger: "pressStarted"
        action: function(eventData) {
            root.pressStartCount += 1
            root.lastFingerCount = eventData.fingerCount || 0
            root.lastMaximumFingerCount = eventData.maximumFingerCount || 0
        }
    }

    LV.EventListener {
        trigger: "pressEnded"
        action: function(eventData) {
            root.pressEndCount += 1
            root.lastPressKind = eventData.classification || ""
            root.lastReleased = !!eventData.released
            root.lastPressDurationMs = eventData.pressDurationMs === undefined ? -1 : eventData.pressDurationMs
        }
    }

    LV.EventListener {
        trigger: "scrollStarted"
        action: function(eventData) {
            root.scrollStartCount += 1
            root.lastScrollAxis = eventData.scrollAxis || ""
            root.lastScrollDirection = eventData.scrollDirection || ""
            root.lastFingerCount = eventData.fingerCount || 0
            root.lastMaximumFingerCount = eventData.maximumFingerCount || 0
        }
    }

    LV.EventListener {
        trigger: "scrollEnded"
        action: function(eventData) {
            root.scrollEndCount += 1
            root.lastReleased = !!eventData.released
            root.lastPressDurationMs = eventData.pressDurationMs === undefined ? -1 : eventData.pressDurationMs
        }
    }
}
)";

    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object);

    auto *window = qobject_cast<QQuickWindow *>(object.data());
    QVERIFY(window);

    const QPointF beginPoint(52.0, 34.0);
    const QPointF updatePoint(55.0, 84.0);
    const QPointF endPoint(56.0, 112.0);

    QTouchEvent touchBegin(QEvent::TouchBegin,
                           eventListenerTouchDevice(),
                           Qt::NoModifier,
                           {QEventPoint(0, QEventPoint::State::Pressed, beginPoint, beginPoint)});
    QCoreApplication::sendEvent(window, &touchBegin);
    QTRY_COMPARE(object->property("pressStartCount").toInt(), 1);
    QCOMPARE(object->property("lastFingerCount").toInt(), 1);
    QCOMPARE(object->property("lastMaximumFingerCount").toInt(), 1);

    QTouchEvent touchUpdate(QEvent::TouchUpdate,
                            eventListenerTouchDevice(),
                            Qt::NoModifier,
                            {QEventPoint(0, QEventPoint::State::Updated, updatePoint, updatePoint)});
    QCoreApplication::sendEvent(window, &touchUpdate);
    QTRY_COMPARE(object->property("scrollStartCount").toInt(), 1);
    QCOMPARE(object->property("lastScrollAxis").toString(), QStringLiteral("y"));
    QCOMPARE(object->property("lastScrollDirection").toString(), QStringLiteral("topToBottom"));

    QTouchEvent touchEnd(QEvent::TouchEnd,
                         eventListenerTouchDevice(),
                         Qt::NoModifier,
                         {QEventPoint(0, QEventPoint::State::Released, endPoint, endPoint)});
    QCoreApplication::sendEvent(window, &touchEnd);
    QTRY_COMPARE(object->property("scrollEndCount").toInt(), 1);
    QTRY_COMPARE(object->property("pressEndCount").toInt(), 1);
    QVERIFY(object->property("lastReleased").toBool());
    QVERIFY(object->property("lastPressDurationMs").toReal() >= 0.0);
}

QTEST_MAIN(EventListenerTests)
#include "tst_eventlistener.moc"
