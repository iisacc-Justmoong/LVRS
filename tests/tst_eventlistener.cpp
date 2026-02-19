#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QMouseEvent>
#include <QQuickWindow>
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
};

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

QTEST_MAIN(EventListenerTests)
#include "tst_eventlistener.moc"
