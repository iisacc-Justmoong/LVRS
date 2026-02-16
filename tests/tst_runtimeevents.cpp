#include <QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QtPlugin>

#include "backend/runtime/runtimeevents.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class RuntimeEventsTests : public QObject
{
    Q_OBJECT

private slots:
    void runtime_events_are_exposed_and_idle_transitions();
    void keyboard_and_mouse_events_are_captured();
    void daemon_contract_is_exposed_to_qml();
};

static QObject *createFromQml(QQmlEngine &engine, const QByteArray &qml)
{
    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QObject *obj = component.create();
    if (component.isError()) {
        const auto errors = component.errors();
        for (const auto &err : errors)
            qWarning() << err;
    }
    return obj;
}

static QScopedPointer<QObject> createRuntimeWindow(QQmlEngine &engine)
{
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 640
    height: 420
    autoAttachRuntimeEvents: true
    visible: false
    title: "RuntimeEventsTest"

    property bool running: LV.RuntimeEvents.running
    property int pid: LV.RuntimeEvents.pid
    property string osLabel: LV.RuntimeEvents.osName
    property bool idleState: LV.RuntimeEvents.idle
    property int keyPressCount: LV.RuntimeEvents.keyPressCount
    property int keyReleaseCount: LV.RuntimeEvents.keyReleaseCount
    property int mouseMoveCount: LV.RuntimeEvents.mouseMoveCount
    property int mousePressCount: LV.RuntimeEvents.mousePressCount
    property int mouseReleaseCount: LV.RuntimeEvents.mouseReleaseCount
    property int captureProfile: LV.RuntimeEvents.captureProfile
    property bool uiTrackingEnabled: LV.RuntimeEvents.uiTrackingEnabled
    property int uiCreatedCount: LV.RuntimeEvents.uiCreatedCount
    property int uiShownCount: LV.RuntimeEvents.uiShownCount
    property int uiHiddenCount: LV.RuntimeEvents.uiHiddenCount
    property double uptimeMs: LV.RuntimeEvents.uptimeMs
    property double rssBytes: LV.RuntimeEvents.rssBytes
    property double daemonBootEpochMs: LV.RuntimeEvents.daemonBootEpochMs
    property int eventSequence: LV.RuntimeEvents.eventSequence
    property int recentEventCount: LV.RuntimeEvents.recentEventCount

    function resetMonitor() {
        LV.RuntimeEvents.resetCounters()
        LV.RuntimeEvents.idleTimeoutMs = 150
        LV.RuntimeEvents.markActivity()
    }

    function pokeActivity() {
        LV.RuntimeEvents.markActivity()
    }

    function clearEventLog() {
        LV.RuntimeEvents.clearRecentEvents()
    }

    function daemonHealthRunning() {
        const health = LV.RuntimeEvents.daemonHealth()
        return !!health.running
    }

    function inputStateSnapshot() {
        return LV.RuntimeEvents.inputState()
    }
}
)";
    return QScopedPointer<QObject>(createFromQml(engine, qml));
}

void RuntimeEventsTests::runtime_events_are_exposed_and_idle_transitions()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);

    QScopedPointer<QObject> root = createRuntimeWindow(engine);
    QVERIFY(root);

    QTRY_VERIFY(root->property("running").toBool());
    QVERIFY(root->property("pid").toInt() > 0);
    QVERIFY(!root->property("osLabel").toString().isEmpty());
    QCOMPARE(root->property("captureProfile").toInt(), static_cast<int>(RuntimeEvents::LowLatencyCapture));
    QVERIFY(!root->property("uiTrackingEnabled").toBool());
    QCOMPARE(root->property("uiCreatedCount").toInt(), 0);
    QVERIFY(root->property("daemonBootEpochMs").toDouble() > 0.0);
    QVariant runningFromHealth;
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "daemonHealthRunning",
                                      Q_RETURN_ARG(QVariant, runningFromHealth)));
    QVERIFY(runningFromHealth.toBool());

    QVERIFY(QMetaObject::invokeMethod(root.data(), "resetMonitor"));
    QTRY_VERIFY(root->property("uptimeMs").toDouble() >= 0.0);
    QTRY_VERIFY(root->property("idleState").toBool());

    QVERIFY(QMetaObject::invokeMethod(root.data(), "pokeActivity"));
    QTRY_VERIFY(!root->property("idleState").toBool());
}

void RuntimeEventsTests::keyboard_and_mouse_events_are_captured()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);

    QScopedPointer<QObject> root = createRuntimeWindow(engine);
    QVERIFY(root);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "resetMonitor"));
    QTRY_VERIFY(root->property("running").toBool());

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);

    const int keyPressBefore = root->property("keyPressCount").toInt();
    const int keyReleaseBefore = root->property("keyReleaseCount").toInt();
    const int mouseMoveBefore = root->property("mouseMoveCount").toInt();
    const int mousePressBefore = root->property("mousePressCount").toInt();
    const int mouseReleaseBefore = root->property("mouseReleaseCount").toInt();
    const int seqBefore = root->property("eventSequence").toInt();

    QKeyEvent keyPressShift(QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier, QString());
    QKeyEvent keyPress(QEvent::KeyPress, Qt::Key_A, Qt::ShiftModifier, QStringLiteral("A"));
    QKeyEvent keyRelease(QEvent::KeyRelease, Qt::Key_A, Qt::NoModifier, QStringLiteral("a"));
    QKeyEvent keyReleaseShift(QEvent::KeyRelease, Qt::Key_Shift, Qt::NoModifier, QString());
    QCoreApplication::sendEvent(window, &keyPressShift);
    QCoreApplication::sendEvent(window, &keyPress);

    QMouseEvent mouseMove(QEvent::MouseMove,
                          QPointF(24.0, 20.0),
                          QPointF(24.0, 20.0),
                          QPointF(24.0, 20.0),
                          Qt::NoButton,
                          Qt::NoButton,
                          Qt::NoModifier);
    QMouseEvent mousePress(QEvent::MouseButtonPress,
                           QPointF(24.0, 20.0),
                           QPointF(24.0, 20.0),
                           QPointF(24.0, 20.0),
                           Qt::LeftButton,
                           Qt::LeftButton,
                           Qt::NoModifier);
    QMouseEvent mouseRelease(QEvent::MouseButtonRelease,
                             QPointF(24.0, 20.0),
                             QPointF(24.0, 20.0),
                             QPointF(24.0, 20.0),
                           Qt::LeftButton,
                           Qt::NoButton,
                           Qt::NoModifier);
    QCoreApplication::sendEvent(window, &mouseMove);
    QCoreApplication::sendEvent(window, &mousePress);

    QVariant inputStateValue;
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "inputStateSnapshot",
                                      Q_RETURN_ARG(QVariant, inputStateValue)));
    const QVariantMap inputState = inputStateValue.toMap();
    QVERIFY(!inputState.isEmpty());
    QVERIFY(inputState.value(QStringLiteral("anyKeyPressed")).toBool());
    QVERIFY(inputState.value(QStringLiteral("mouseButtonPressed")).toBool());
    QVERIFY(inputState.value(QStringLiteral("activePressDurationMs")).toLongLong() >= 0);
    const QVariantMap pointerUi = inputState.value(QStringLiteral("pointerUi")).toMap();
    QVERIFY(pointerUi.contains(QStringLiteral("objectName")));
    const QVariantList pressedKeyCodes = inputState.value(QStringLiteral("pressedKeyCodes")).toList();
    QVERIFY(pressedKeyCodes.contains(QVariant::fromValue(static_cast<int>(Qt::Key_A))));

    QCoreApplication::sendEvent(window, &keyRelease);
    QCoreApplication::sendEvent(window, &keyReleaseShift);
    QCoreApplication::sendEvent(window, &mouseRelease);

    QTRY_VERIFY(root->property("keyPressCount").toInt() > keyPressBefore);
    QTRY_VERIFY(root->property("keyReleaseCount").toInt() > keyReleaseBefore);
    QTRY_VERIFY(root->property("mouseMoveCount").toInt() > mouseMoveBefore);
    QTRY_VERIFY(root->property("mousePressCount").toInt() > mousePressBefore);
    QTRY_VERIFY(root->property("mouseReleaseCount").toInt() > mouseReleaseBefore);
    QTRY_VERIFY(root->property("eventSequence").toInt() > seqBefore);
    QTRY_VERIFY(root->property("recentEventCount").toInt() > 0);
}

void RuntimeEventsTests::daemon_contract_is_exposed_to_qml()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);

    QScopedPointer<QObject> root = createRuntimeWindow(engine);
    QVERIFY(root);

    QTRY_VERIFY(root->property("running").toBool());
    QVERIFY(root->property("daemonBootEpochMs").toDouble() > 0.0);
    QVariant runningFromHealth;
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "daemonHealthRunning",
                                      Q_RETURN_ARG(QVariant, runningFromHealth)));
    QVERIFY(runningFromHealth.toBool());

    QVERIFY(QMetaObject::invokeMethod(root.data(), "clearEventLog"));
    QTRY_COMPARE(root->property("recentEventCount").toInt(), 0);
}

QTEST_MAIN(RuntimeEventsTests)
#include "tst_runtimeevents.moc"
