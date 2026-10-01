#include <QtTest>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QKeyEvent>
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
    void compact_button_release_data();
    void compact_button_release();
    void compact_button_cancel_and_opt_out_data();
    void compact_button_cancel_and_opt_out();
    void compact_button_tones_and_keyboard_focus_data();
    void compact_button_tones_and_keyboard_focus();
    void row_interaction_states_data();
    void row_interaction_states();
    void enter_activates_on_release_data();
    void enter_activates_on_release();
    void hierarchy_mobile_release_and_cancel();
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
        rebounded |= motion->property("deformationProgress").toReal() < -0.01;
    }
    QVERIFY(rebounded);
    QTRY_COMPARE(motion->property("deformationProgress").toReal(), 0.0);
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
                            "AlertButton", "Link", "ToolbarButton", "HierarchyItem", "ListItem", "MenuItem", "ContextMenuItem",
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

static QObject *compactFixture(QQmlEngine &engine, const QString &type, int tone = 0)
{
    engine.addImportPath(TestUtils::qmlImportBase());
    return TestUtils::createFromQml(engine, QString(R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    visible: true; width: 500; height: 260
    function reduceMotion() { LV.Motion.reducedMotion = true }
    function restoreMotion() { LV.Motion.reducedMotion = false }
    LV.%1 { objectName: "button"; x: 60; y: 40; text: "Action"; tone: %2 }
    LV.PushButton { objectName: "next"; x: 220; y: 40; text: "Next" }
})").arg(type).arg(tone).toUtf8());
}

void MotionTests::compact_button_release_data()
{
    QTest::addColumn<QString>("type");
    QTest::addColumn<bool>("keyboard");
    for (const char *type : {"PushButton", "DropdownButton", "IconButton", "IconMenuButton",
                            "MenuItem", "ContextMenuItem", "ListItem", "HierarchyItem"}) {
        for (bool keyboard : {false, true}) {
            const QByteArray row = QByteArray(type) + (keyboard ? "-keyboard" : "-pointer");
            QTest::newRow(row.constData()) << QString::fromLatin1(type) << keyboard;
        }
    }
}

void MotionTests::compact_button_release()
{
    QFETCH(QString, type);
    QFETCH(bool, keyboard);
    QQmlEngine engine;
    QScopedPointer<QObject> root(compactFixture(engine, type));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *motion = button->findChild<QObject *>("interactionMotion");
    QVERIFY(motion);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    const QRectF geometry = button->mapRectToScene(button->boundingRect());
    const QPoint center = geometry.center().toPoint();
    QSignalSpy clicks(button, SIGNAL(clicked()));
    button->forceActiveFocus(Qt::TabFocusReason);

    // A tap can finish before the 90ms compression has reached its first frame.
    for (int holdMs : {0, 110}) {
        if (keyboard)
            QTest::keyPress(window, Qt::Key_Space);
        else
            QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
        if (holdMs)
            QTest::qWait(holdMs);
        if (keyboard)
            QTest::keyRelease(window, Qt::Key_Space);
        else
            QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, center);
        QCOMPARE(clicks.count(), holdMs ? 2 : 1);
        bool expanded = false;
        QElapsedTimer elapsed;
        elapsed.start();
        while (elapsed.elapsed() < 240) {
            QTest::qWait(8);
            expanded |= motion->property("deformationProgress").toReal() < -0.01;
            QCOMPARE(button->mapRectToScene(button->boundingRect()), geometry);
        }
        QVERIFY2(expanded, "Both immediate taps and held releases must show one elastic return");
        QCOMPARE(motion->property("deformationProgress").toReal(), 0.0);
        auto *background = button->property("background").value<QQuickItem *>();
        QCOMPARE(background->property("color"), button->property(button->property("hovered").toBool()
            ? "backgroundColorHover" : "backgroundColor"));
    }
}

void MotionTests::compact_button_cancel_and_opt_out_data()
{
    QTest::addColumn<QString>("type");
    for (const char *type : {"PushButton", "MenuItem", "ContextMenuItem", "ListItem", "HierarchyItem"})
        QTest::newRow(type) << QString::fromLatin1(type);
}

void MotionTests::compact_button_cancel_and_opt_out()
{
    QFETCH(QString, type);
    QQmlEngine engine;
    QScopedPointer<QObject> root(compactFixture(engine, type));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *motion = button->findChild<QObject *>("interactionMotion");
    QVERIFY(QTest::qWaitForWindowExposed(window));
    const QPoint center = button->mapToScene(button->boundingRect().center()).toPoint();
    const QPoint outside(450, 220);
    QSignalSpy clicks(button, SIGNAL(clicked()));
    QSignalSpy cancelled(button, SIGNAL(canceled()));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
    QTest::qWait(110);
    QTest::mouseMove(window, outside);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, outside);
    QCOMPARE(clicks.count(), 0);
    QCOMPARE(cancelled.count(), 1);
    for (int i = 0; i < 25; ++i) {
        QTest::qWait(8);
        QVERIFY(motion->property("deformationProgress").toReal() >= 0);
    }

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(clicks.count(), 1);
    QTest::qWait(24);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "reduceMotion"));
    QCOMPARE(motion->property("deformationProgress").toReal(), 0.0);
    QCOMPARE(motion->property("xScale").toReal(), 1.0);
    QCOMPARE(motion->property("yScale").toReal(), 1.0);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "restoreMotion"));
    button->setProperty("motionEnabled", false);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(clicks.count(), 2);
    QCOMPARE(motion->property("deformationProgress").toReal(), 0.0);
    button->setProperty("motionEnabled", true);
    for (int i = 0; i < 4; ++i) {
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
        QTest::qWait(24);
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, center);
        QTest::qWait(24);
    }
    QCOMPARE(clicks.count(), 6);
    QTest::qWait(220);
    QCOMPARE(motion->property("deformationProgress").toReal(), 0.0);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
    button->setEnabled(false);
    QCOMPARE(motion->property("deformationProgress").toReal(), 0.0);
    QCOMPARE(motion->property("xScale").toReal(), 1.0);
    QCOMPARE(motion->property("yScale").toReal(), 1.0);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(clicks.count(), 7);
}

void MotionTests::compact_button_tones_and_keyboard_focus_data()
{
    QTest::addColumn<QString>("type");
    QTest::addColumn<int>("tone");
    for (const char *type : {"PushButton", "DropdownButton", "IconButton", "IconMenuButton"}) {
        for (int tone = 0; tone < 5; ++tone) {
            const QByteArray row = QByteArray(type) + '-' + QByteArray::number(tone);
            QTest::newRow(row.constData()) << QString::fromLatin1(type) << tone;
        }
    }
}

void MotionTests::compact_button_tones_and_keyboard_focus()
{
    QFETCH(QString, type);
    QFETCH(int, tone);
    QQmlEngine engine;
    QScopedPointer<QObject> root(compactFixture(engine, type, tone));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *next = root->findChild<QQuickItem *>("next");
    auto *background = button->property("background").value<QQuickItem *>();
    QVERIFY(background);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    // Turning motion off makes colors immediate without changing state semantics.
    button->setProperty("motionEnabled", false);
    QTest::mouseMove(window, QPoint(450, 220));
    QCOMPARE(background->property("color"), button->property(tone == 4
        ? "backgroundColorDisabled" : "backgroundColor"));
    QSignalSpy clicks(button, SIGNAL(clicked()));
    const QPoint center = button->mapToScene(button->boundingRect().center()).toPoint();
    QTest::mouseMove(window, center);
    if (tone == 4) {
        QVERIFY(!button->property("activeFocusOnTab").toBool());
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, center);
        QCOMPARE(clicks.count(), 0);
        QCOMPARE(background->property("color"), button->property("backgroundColorDisabled"));
        return;
    }
    QTRY_VERIFY(button->property("hovered").toBool());
    QCOMPARE(background->property("color"), button->property("backgroundColorHover"));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(background->property("color"), button->property("backgroundColorPressed"));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(clicks.count(), 1);
    QCOMPARE(background->property("color"), button->property("backgroundColorHover"));
    next->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyClick(window, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(button->property("visualFocus").toBool());
    QTest::keyPress(window, Qt::Key_Space);
    QVERIFY(button->property("visualFocus").toBool());
    QTest::keyRelease(window, Qt::Key_Space);
    QVERIFY(button->property("visualFocus").toBool());
    QCOMPARE(clicks.count(), 2);
    QTest::keyClick(window, Qt::Key_Tab);
    QVERIFY(next->hasActiveFocus());
    QTest::keyClick(window, Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(button->hasActiveFocus());
}

void MotionTests::row_interaction_states_data()
{
    QTest::addColumn<QString>("type");
    QTest::addColumn<QString>("configuration");
    QTest::addColumn<bool>("inactive");
    QTest::addColumn<bool>("selected");
    for (const char *type : {"MenuItem", "ContextMenuItem", "HierarchyItem"}) {
        for (int state = 0; state < 3; ++state) {
            const QString configuration = QString::fromLatin1(type) == "HierarchyItem"
                ? (state == 1 ? "selected: true" : state == 2 ? "activatable: false" : "")
                : QStringLiteral("state: %1").arg(state);
            const QByteArray row = QByteArray(type) + '-' + QByteArray::number(state);
            QTest::newRow(row.constData()) << QString::fromLatin1(type) << configuration << (state == 2) << (state == 1);
        }
    }
    for (int variant = 0; variant < 17; ++variant) {
        const QByteArray row = "ListItem-" + QByteArray::number(variant);
        QTest::newRow(row.constData()) << QStringLiteral("ListItem") << QStringLiteral("type: %1").arg(variant) << false << false;
    }
    QTest::newRow("ListItem-selected") << QStringLiteral("ListItem") << QStringLiteral("selected: true") << false << true;
    QTest::newRow("ListItem-disabled") << QStringLiteral("ListItem") << QStringLiteral("enabled: false") << true << false;
}

void MotionTests::row_interaction_states()
{
    QFETCH(QString, type);
    QFETCH(QString, configuration);
    QFETCH(bool, inactive);
    QFETCH(bool, selected);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, QString(R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    visible: true; width: 700; height: 400
    LV.%1 { objectName: "row"; x: 40; y: 40; motionEnabled: false; %2 }
    LV.PushButton { objectName: "next"; x: 500; y: 280; text: "Next" }
})").arg(type, configuration).toUtf8()));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *row = root->findChild<QQuickItem *>("row");
    auto *next = root->findChild<QQuickItem *>("next");
    auto *motion = row->findChild<QObject *>("interactionMotion");
    auto *ring = row->findChild<QQuickItem *>("buttonFocusRing");
    auto *background = row->property("background").value<QQuickItem *>();
    QVERIFY(motion);
    QVERIFY(ring);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    const QRectF geometry = row->mapRectToScene(row->boundingRect());
    // Leading edge belongs to the row; embedded accessory controls keep their own events.
    const QPoint point = row->mapToScene(QPointF(2, row->height() / 2)).toPoint();
    QSignalSpy clicks(row, SIGNAL(clicked()));
    QTest::mouseMove(window, QPoint(650, 350));
    QCOMPARE(background->property("color"), row->property(inactive ? "backgroundColorDisabled" : "backgroundColor"));
    const QVariant originalFill = background->property("color");
    QTest::mouseMove(window, point);
    if (inactive) {
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point);
        QVERIFY(!row->property("activeFocusOnTab").toBool());
        QVERIFY(!ring->property("active").toBool());
        QCOMPARE(background->property("color"), originalFill);
        QCOMPARE(motion->property("xScale").toReal(), 1.0);
        QCOMPARE(motion->property("yScale").toReal(), 1.0);
        if (type != "HierarchyItem")
            QCOMPARE(clicks.count(), 0);
        return;
    }
    QTRY_VERIFY(row->property("hovered").toBool());
    QCOMPARE(background->property("color"), row->property("backgroundColorHover"));
    if (selected)
        QCOMPARE(background->property("color"), originalFill);
    else
        QVERIFY(background->property("color") != originalFill);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(background->property("color"), row->property("backgroundColorPressed"));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, point);
    QCOMPARE(clicks.count(), 1);
    QCOMPARE(background->property("color"), row->property("backgroundColorHover"));
    next->forceActiveFocus(Qt::TabFocusReason);
    // Complex rows contain independently focusable controls; explicitly focus the row itself.
    row->forceActiveFocus(Qt::TabFocusReason);
    QVERIFY(ring->property("active").toBool());
    QCOMPARE(ring->x(), -3.0);
    QCOMPARE(ring->width(), row->width() + 6.0);
    QTest::keyPress(window, Qt::Key_Space);
    QVERIFY(ring->property("active").toBool());
    QTest::keyRelease(window, Qt::Key_Space);
    QCOMPARE(clicks.count(), 2);
    QVERIFY(ring->property("active").toBool());
    QCOMPARE(row->mapRectToScene(row->boundingRect()), geometry);
}

void MotionTests::enter_activates_on_release_data()
{
    QTest::addColumn<QString>("type");
    for (const char *type : {"PushButton", "DropdownButton", "MenuItem", "ContextMenuItem", "ListItem", "HierarchyItem"})
        QTest::newRow(type) << QString::fromLatin1(type);
}

void MotionTests::enter_activates_on_release()
{
    QFETCH(QString, type);
    QQmlEngine engine;
    QScopedPointer<QObject> root(compactFixture(engine, type));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *next = root->findChild<QQuickItem *>("next");
    auto *motion = button->findChild<QObject *>("interactionMotion");
    QVERIFY(QTest::qWaitForWindowExposed(window));
    button->forceActiveFocus(Qt::TabFocusReason);
    QSignalSpy clicks(button, SIGNAL(clicked()));
    for (Qt::Key key : {Qt::Key_Return, Qt::Key_Enter}) {
        const int before = clicks.count();
        QTest::keyPress(window, key);
        QTRY_VERIFY(motion->property("pressProgress").toReal() > 0.5);
        QCOMPARE(clicks.count(), before);
        QKeyEvent repeat(QEvent::KeyPress, key, Qt::NoModifier, QString(), true);
        QCoreApplication::sendEvent(window, &repeat);
        QCOMPARE(clicks.count(), before);
        QTest::keyRelease(window, key);
        QCOMPARE(clicks.count(), before + 1);
        QVERIFY(button->property("visualFocus").toBool());
        // The first animation frame may already have advanced on a busy row.
        QVERIFY(motion->property("releasing").toBool());
        QTest::qWait(220);
    }
    QTest::keyPress(window, Qt::Key_Return);
    next->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyRelease(window, Qt::Key_Return);
    QCOMPARE(clicks.count(), 2);
    QTRY_VERIFY(!button->property("down").toBool());
    QCOMPARE(motion->property("releaseProgress").toReal(), 0.0);
    if (type == "PushButton") {
        button->setProperty("checkable", true);
        button->setProperty("checked", false);
        button->forceActiveFocus(Qt::TabFocusReason);
        QTest::keyPress(window, Qt::Key_Return);
        QVERIFY(!button->property("checked").toBool());
        QTest::keyRelease(window, Qt::Key_Return);
        QVERIFY(button->property("checked").toBool());
        QCOMPARE(clicks.count(), 3);
    }
}

void MotionTests::hierarchy_mobile_release_and_cancel()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    visible: true; width: 500; height: 260
    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""
    QtObject {
        id: owner
        property bool editableEnabled: true
        property int _dragTargetIndex: -1
        property int _dragTargetDepth: -1
    }
    LV.HierarchyItem {
        objectName: "row"; x: 40; y: 40; width: 300
        hierarchyList: owner; generatedByTreeModel: true; showChevron: false
    }
})"));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *row = root->findChild<QQuickItem *>("row");
    auto *motion = row->findChild<QObject *>("interactionMotion");
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(row->property("pointerDragRequiresLongPress").toBool());
    const QPoint center = row->mapToScene(row->boundingRect().center()).toPoint();
    QSignalSpy clicks(row, SIGNAL(clicked()));
    const QRectF geometry = row->mapRectToScene(row->boundingRect());
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
    QTRY_VERIFY_WITH_TIMEOUT(motion->property("pressProgress").toReal() > 0.7, 500);
    QVERIFY(!row->property("down").toBool());
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(clicks.count(), 1);
    QVERIFY(motion->property("releaseProgress").toReal() >= 0.35);
    QTest::qWait(220);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
    QTest::qWait(100);
    QTest::mouseMove(window, QPoint(450, 230));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(450, 230));
    QCOMPARE(clicks.count(), 1);
    QCOMPARE(motion->property("releaseProgress").toReal(), 0.0);
    row->setProperty("dragPreviewActive", true);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, center);
    QTest::qWait(100);
    QCOMPARE(motion->property("xScale").toReal(), 1.0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, center);
    QCOMPARE(motion->property("releaseProgress").toReal(), 0.0);
    QCOMPARE(row->mapRectToScene(row->boundingRect()), geometry);
}

QTEST_MAIN(MotionTests)
#include "tst_motion.moc"
