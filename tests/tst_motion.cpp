#include <QtTest>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include "test_utils.h"

class MotionTests : public QObject
{
    Q_OBJECT
private slots:
    void button_press_rebound_and_retarget();
    void preferences_settle_in_flight_motion();
    void keyboard_disabled_and_layout();
    void slider_drag_is_direct_and_release_settles();
    void button_families_data();
    void button_families();
    void slider_programmatic_travel_stays_bounded();
    void touch_keeps_the_input_target_fixed();
};

static QObject *fixture(QQmlEngine &engine)
{
    engine.addImportPath(TestUtils::qmlImportBase());
    return TestUtils::createFromQml(engine, R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    visible: true; width: 500; height: 260
    function reduceMotion() { LV.Motion.reducedMotion = true }
    function restoreMotion() { LV.Motion.reducedMotion = false; LV.Motion.enabled = true }
    LV.LabelButton { objectName: "button"; x: 60; y: 40; text: "Press"; hoverEnabled: false }
    LV.Slider { objectName: "slider"; x: 60; y: 140; width: 300; value: 0.25 }
})");
}

void MotionTests::button_press_rebound_and_retarget()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    QVERIFY(QTest::qWaitForWindowExposed(window));
    auto *motion = button->findChild<QObject *>("interactionMotion");
    QVERIFY(motion);
    const auto center = button->mapToScene(button->boundingRect().center()).toPoint();
    QSignalSpy clicks(button, SIGNAL(clicked()));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
    QTRY_VERIFY(motion->property("pressProgress").toReal() > 0.95);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, center);
    bool rebounded = false;
    for (int i = 0; i < 35; ++i) {
        QTest::qWait(12);
        rebounded |= motion->property("pressProgress").toReal() < -0.01;
    }
    QVERIFY(rebounded);
    QTRY_COMPARE(motion->property("pressProgress").toReal(), 0.0);
    QCOMPARE(clicks.count(), 1);
    for (int i = 0; i < 4; ++i) {
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
        QTest::qWait(24);
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, center);
        QTest::qWait(24);
    }
    QTRY_COMPARE(motion->property("pressProgress").toReal(), 0.0);
    QCOMPARE(clicks.count(), 5);
}

void MotionTests::preferences_settle_in_flight_motion()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    QVERIFY(root);
    auto *button = root->findChild<QQuickItem *>("button");
    auto *motion = button->findChild<QObject *>("interactionMotion");
    QVERIFY(motion);
    button->setProperty("down", true);
    QTest::qWait(40);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "reduceMotion"));
    QTRY_COMPARE(motion->property("pressProgress").toReal(), 0.0);
    QCOMPARE(motion->property("xScale").toReal(), 1.0);
    QCOMPARE(motion->property("yScale").toReal(), 1.0);
    button->setProperty("down", false);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "restoreMotion"));
    button->setProperty("motionEnabled", false);
    button->setProperty("down", true);
    QCOMPARE(motion->property("pressProgress").toReal(), 0.0);
}

void MotionTests::keyboard_disabled_and_layout()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *motion = button->findChild<QObject *>("interactionMotion");
    QVERIFY(motion);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    const QRectF geometry(button->position(), button->size());
    QSignalSpy clicks(button, SIGNAL(clicked()));
    button->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyPress(window, Qt::Key_Space);
    QTRY_VERIFY(motion->property("pressProgress").toReal() > 0.5);
    QCOMPARE(QRectF(button->position(), button->size()), geometry);
    QTest::keyRelease(window, Qt::Key_Space);
    QCOMPARE(clicks.count(), 1);
    button->setEnabled(false);
    QTRY_COMPARE(motion->property("pressProgress").toReal(), 0.0);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, geometry.center().toPoint());
    QCOMPARE(clicks.count(), 1);
}

void MotionTests::slider_drag_is_direct_and_release_settles()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *slider = root->findChild<QQuickItem *>("slider");
    auto *handle = slider->findChild<QQuickItem *>("slider_handle");
    QVERIFY(handle);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    const auto start = handle->mapToScene(handle->boundingRect().center()).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove(window, start + QPoint(80, 0), 24);
    QVERIFY(slider->property("value").toReal() > 0.4);
    const qreal expected = slider->property("leftPadding").toReal()
        + slider->property("visualPosition").toReal() * slider->property("rangeWidth").toReal();
    QVERIFY(qAbs(handle->x() - expected) < 0.1);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, start + QPoint(80, 0));
    QTRY_VERIFY(!slider->property("pressed").toBool());
}

void MotionTests::button_families_data()
{
    QTest::addColumn<QString>("type");
    for (const char *type : {"AbstractButton", "PushButton", "DropdownButton", "LabelButton", "IconButton",
                            "LabelMenuButton", "IconMenuButton", "CheckBox", "RadioButton", "Card",
                            "AlertButton", "Link", "ToolbarButton", "HierarchyItem", "ListItem", "MenuItem",
                            "Stepper", "ComboBox"})
        QTest::newRow(type) << QString::fromLatin1(type);
}

void MotionTests::button_families()
{
    QFETCH(QString, type);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    const QByteArray qml = QString(R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    visible: true; width: 440; height: 400
    LV.%1 { objectName: "control"; x: 60; y: 60 }
})").arg(type).toUtf8();
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *control = root->findChild<QQuickItem *>("control");
    QVERIFY(control);
    auto *motion = control->findChild<QObject *>("interactionMotion");
    QVERIFY(motion);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    const QRectF bounds = control->mapRectToScene(control->boundingRect());
    const QPoint point = bounds.center().toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, point);
    QTRY_VERIFY_WITH_TIMEOUT(motion->property("pressProgress").toReal() > 0.7, 500);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, point);
    QTRY_COMPARE(motion->property("pressProgress").toReal(), 0.0);
    control->setEnabled(false);
    QTRY_COMPARE(motion->property("xScale").toReal(), 1.0);
    QTRY_COMPARE(motion->property("yScale").toReal(), 1.0);
}

void MotionTests::slider_programmatic_travel_stays_bounded()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    QVERIFY(root);
    auto *slider = root->findChild<QQuickItem *>("slider");
    auto *handle = slider->findChild<QQuickItem *>("slider_handle");
    const qreal left = slider->property("leftPadding").toReal();
    const qreal range = slider->property("rangeWidth").toReal();
    for (qreal value : {1.0, 0.0, 0.75}) {
        QVERIFY(slider->setProperty("value", value));
        QCOMPARE(slider->property("value").toReal(), value);
        for (int i = 0; i < 35; ++i) {
            QTest::qWait(12);
            QVERIFY(handle->x() >= left && handle->x() <= left + range);
        }
        QTRY_VERIFY(qAbs(handle->x() - left - value * range) < 0.01);
    }
}

void MotionTests::touch_keeps_the_input_target_fixed()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    QVERIFY(QTest::qWaitForWindowExposed(window));
    const QRectF bounds = button->mapRectToScene(button->boundingRect());
    QSignalSpy clicks(button, SIGNAL(clicked()));
    auto *device = QTest::createTouchDevice();
    const QPoint point = bounds.center().toPoint();
    QTest::touchEvent(window, device).press(0, point, window);
    QTest::qWait(110);
    QCOMPARE(button->mapRectToScene(button->boundingRect()), bounds);
    QTest::touchEvent(window, device).release(0, point, window);
    QTRY_COMPARE(clicks.count(), 1);
    QTest::qWait(400);
    QCOMPARE(button->mapRectToScene(button->boundingRect()), bounds);
}

QTEST_MAIN(MotionTests)
#include "tst_motion.moc"
