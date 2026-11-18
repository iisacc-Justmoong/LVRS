#include "QtEventWait.h"
#include "QtGuiTouch.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QTimer>
#include <type_traits>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QKeyEvent>
#include "test_utils.h"

class MotionSignalRecorder : public QObject
{
    Q_OBJECT
public:
    int count = 0;
public slots:
    void recordSignal() { ++count; }
};

namespace {
template <typename Actual, typename Expected>
bool motionEqual(const Actual &actual, const Expected &expected)
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
Qt::MouseButtons pressedButtons = Qt::NoButton;
void sendMouse(QQuickWindow *window, QEvent::Type type, const QPoint &point, Qt::MouseButton button, ulong delay = 16)
{
    if (type == QEvent::MouseButtonPress) pressedButtons |= button;
    if (type == QEvent::MouseButtonRelease) pressedButtons &= ~Qt::MouseButtons(button);
    static ulong timestamp = 10000;
    const QPointF local(point), global(window->mapToGlobal(point));
    QMouseEvent event(type, local, local, global, button, pressedButtons, Qt::NoModifier);
    event.setTimestamp(timestamp += delay);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}
void mouseMove(QQuickWindow *window, const QPoint &point, ulong delay = 16) { sendMouse(window, QEvent::MouseMove, point, Qt::NoButton, delay); }
void mousePress(QQuickWindow *window, const QPoint &point) { sendMouse(window, QEvent::MouseButtonPress, point, Qt::LeftButton); }
void mouseRelease(QQuickWindow *window, const QPoint &point) { sendMouse(window, QEvent::MouseButtonRelease, point, Qt::LeftButton); }
void mouseClick(QQuickWindow *window, const QPoint &point) { mouseMove(window, point); mousePress(window, point); mouseRelease(window, point); }
void sendKey(QQuickWindow *window, QEvent::Type type, Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    const QString text = key == Qt::Key_Space ? QStringLiteral(" ") : QString();
    QKeyEvent event(type, key, modifiers, text);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}
void keyClick(QQuickWindow *window, Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    sendKey(window, QEvent::KeyPress, key, modifiers);
    sendKey(window, QEvent::KeyRelease, key, modifiers);
}
}
#define MOTION_COMPARE(actual, expected) CTEST_CHECK(motionEqual((actual), (expected)))
#define MOTION_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(motionEqual((actual), (expected)))

class MotionTests
{
public:
    void button_press_rebound_and_retarget();
    void preferences_settle_in_flight_motion();
    void keyboard_disabled_and_layout();
    void slider_drag_is_direct_and_release_settles();
    void button_families(const QString &type);
    void slider_programmatic_travel_stays_bounded();
    void touch_keeps_the_input_target_fixed();
    void compact_button_release(const QString &type, bool keyboard);
    void compact_button_cancel_and_opt_out(const QString &type);
    void compact_button_tones_and_keyboard_focus(const QString &type, int tone);
    void row_interaction_states(const QString &type, const QString &configuration, bool inactive, bool selected);
    void enter_activates_on_release(const QString &type);
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    auto *motion = button->findChild<QObject *>("interactionMotion");
    CTEST_CHECK(motion);
    const auto center = button->mapToScene(button->boundingRect().center()).toPoint();
    MotionSignalRecorder clicks;
    const auto clicksConnection = QObject::connect(button, SIGNAL(clicked()), &clicks, SLOT(recordSignal()));
    CTEST_CHECK(clicksConnection);
    mousePress(window, center);
    CTEST_WAIT_DEFAULT(motion->property("pressProgress").toReal() > 0.95);
    mouseRelease(window, center);
    bool rebounded = false;
    for (int i = 0; i < 35; ++i) {
        processFor(12);
        rebounded |= motion->property("deformationProgress").toReal() < -0.01;
    }
    CTEST_CHECK(rebounded);
    MOTION_WAIT_COMPARE(motion->property("deformationProgress").toReal(), 0.0);
    MOTION_WAIT_COMPARE(motion->property("pressProgress").toReal(), 0.0);
    MOTION_COMPARE(clicks.count, 1);
    for (int i = 0; i < 4; ++i) {
        mousePress(window, center);
        processFor(24);
        mouseRelease(window, center);
        processFor(24);
    }
    MOTION_WAIT_COMPARE(motion->property("pressProgress").toReal(), 0.0);
    MOTION_COMPARE(clicks.count, 5);
}

void MotionTests::preferences_settle_in_flight_motion()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    CTEST_CHECK(root);
    auto *button = root->findChild<QQuickItem *>("button");
    auto *motion = button->findChild<QObject *>("interactionMotion");
    CTEST_CHECK(motion);
    button->setProperty("down", true);
    processFor(40);
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "reduceMotion"));
    MOTION_WAIT_COMPARE(motion->property("pressProgress").toReal(), 0.0);
    MOTION_COMPARE(motion->property("xScale").toReal(), 1.0);
    MOTION_COMPARE(motion->property("yScale").toReal(), 1.0);
    button->setProperty("down", false);
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "restoreMotion"));
    button->setProperty("motionEnabled", false);
    button->setProperty("down", true);
    MOTION_COMPARE(motion->property("pressProgress").toReal(), 0.0);
}

void MotionTests::keyboard_disabled_and_layout()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *motion = button->findChild<QObject *>("interactionMotion");
    CTEST_CHECK(motion);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    const QRectF geometry(button->position(), button->size());
    MotionSignalRecorder clicks;
    const auto clicksConnection = QObject::connect(button, SIGNAL(clicked()), &clicks, SLOT(recordSignal()));
    CTEST_CHECK(clicksConnection);
    button->forceActiveFocus(Qt::TabFocusReason);
    sendKey(window, QEvent::KeyPress, Qt::Key_Space);
    CTEST_WAIT_DEFAULT(motion->property("pressProgress").toReal() > 0.5);
    MOTION_COMPARE(QRectF(button->position(), button->size()), geometry);
    sendKey(window, QEvent::KeyRelease, Qt::Key_Space);
    MOTION_COMPARE(clicks.count, 1);
    button->setEnabled(false);
    MOTION_WAIT_COMPARE(motion->property("pressProgress").toReal(), 0.0);
    mouseClick(window, geometry.center().toPoint());
    MOTION_COMPARE(clicks.count, 1);
}

void MotionTests::slider_drag_is_direct_and_release_settles()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *slider = root->findChild<QQuickItem *>("slider");
    auto *handle = slider->findChild<QQuickItem *>("slider_handle");
    CTEST_CHECK(handle);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    const auto start = handle->mapToScene(handle->boundingRect().center()).toPoint();
    mousePress(window, start);
    mouseMove(window, start + QPoint(80, 0), 24);
    CTEST_CHECK(slider->property("value").toReal() > 0.4);
    const qreal expected = slider->property("leftPadding").toReal()
        + slider->property("visualPosition").toReal() * slider->property("rangeWidth").toReal();
    CTEST_CHECK(qAbs(handle->x() - expected) < 0.1);
    mouseRelease(window, start + QPoint(80, 0));
    CTEST_WAIT_DEFAULT(!slider->property("pressed").toBool());
}

void MotionTests::button_families(const QString &type)
{
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *control = root->findChild<QQuickItem *>("control");
    CTEST_CHECK(control);
    auto *motion = control->findChild<QObject *>("interactionMotion");
    CTEST_CHECK(motion);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    window->requestActivate();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isActive(); }, 5000));
    const QRectF bounds = control->mapRectToScene(control->boundingRect());
    const QPoint point = bounds.center().toPoint();
    mousePress(window, point);
    CTEST_WAIT_VERIFY(motion->property("pressProgress").toReal() > 0.7, 500);
    mouseRelease(window, point);
    MOTION_WAIT_COMPARE(motion->property("pressProgress").toReal(), 0.0);
    control->setEnabled(false);
    MOTION_WAIT_COMPARE(motion->property("xScale").toReal(), 1.0);
    MOTION_WAIT_COMPARE(motion->property("yScale").toReal(), 1.0);
}

void MotionTests::slider_programmatic_travel_stays_bounded()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    CTEST_CHECK(root);
    auto *slider = root->findChild<QQuickItem *>("slider");
    auto *handle = slider->findChild<QQuickItem *>("slider_handle");
    const qreal left = slider->property("leftPadding").toReal();
    const qreal range = slider->property("rangeWidth").toReal();
    for (qreal value : {1.0, 0.0, 0.75}) {
        CTEST_CHECK(slider->setProperty("value", value));
        MOTION_COMPARE(slider->property("value").toReal(), value);
        for (int i = 0; i < 35; ++i) {
            processFor(12);
            CTEST_CHECK(handle->x() >= left && handle->x() <= left + range);
        }
        CTEST_WAIT_DEFAULT(qAbs(handle->x() - left - value * range) < 0.01);
    }
}

void MotionTests::touch_keeps_the_input_target_fixed()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(fixture(engine));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    const QRectF bounds = button->mapRectToScene(button->boundingRect());
    MotionSignalRecorder clicks;
    const auto clicksConnection = QObject::connect(button, SIGNAL(clicked()), &clicks, SLOT(recordSignal()));
    CTEST_CHECK(clicksConnection);
    auto *device = lvrs_ctest::createTouchDevice();
    const QPoint point = bounds.center().toPoint();
    lvrs_ctest::TouchSequence(window, device).press(0, point, window);
    processFor(110);
    MOTION_COMPARE(button->mapRectToScene(button->boundingRect()), bounds);
    lvrs_ctest::TouchSequence(window, device).release(0, point, window);
    MOTION_WAIT_COMPARE(clicks.count, 1);
    processFor(400);
    MOTION_COMPARE(button->mapRectToScene(button->boundingRect()), bounds);
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

void MotionTests::compact_button_release(const QString &type, bool keyboard)
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(compactFixture(engine, type));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *motion = button->findChild<QObject *>("interactionMotion");
    CTEST_CHECK(motion);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    const QRectF geometry = button->mapRectToScene(button->boundingRect());
    const QPoint center = geometry.center().toPoint();
    MotionSignalRecorder clicks;
    const auto clicksConnection = QObject::connect(button, SIGNAL(clicked()), &clicks, SLOT(recordSignal()));
    CTEST_CHECK(clicksConnection);
    button->forceActiveFocus(Qt::TabFocusReason);

    // A tap can finish before the 90ms compression has reached its first frame.
    for (int holdMs : {0, 110}) {
        if (keyboard)
            sendKey(window, QEvent::KeyPress, Qt::Key_Space);
        else
            mousePress(window, center);
        if (holdMs)
            processFor(holdMs);
        if (keyboard)
            sendKey(window, QEvent::KeyRelease, Qt::Key_Space);
        else
            mouseRelease(window, center);
        MOTION_COMPARE(clicks.count, holdMs ? 2 : 1);
        bool expanded = false;
        QElapsedTimer elapsed;
        elapsed.start();
        while (elapsed.elapsed() < 240) {
            processFor(8);
            expanded |= motion->property("deformationProgress").toReal() < -0.01;
            MOTION_COMPARE(button->mapRectToScene(button->boundingRect()), geometry);
        }
        CTEST_CHECK_MESSAGE(expanded, "Both immediate taps and held releases must show one elastic return");
        MOTION_COMPARE(motion->property("deformationProgress").toReal(), 0.0);
        auto *background = button->property("background").value<QQuickItem *>();
        MOTION_COMPARE(background->property("color"), button->property(button->property("hovered").toBool()
            ? "backgroundColorHover" : "backgroundColor"));
    }
}

void MotionTests::compact_button_cancel_and_opt_out(const QString &type)
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(compactFixture(engine, type));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *motion = button->findChild<QObject *>("interactionMotion");
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    const QPoint center = button->mapToScene(button->boundingRect().center()).toPoint();
    const QPoint outside(450, 220);
    MotionSignalRecorder clicks;
    const auto clicksConnection = QObject::connect(button, SIGNAL(clicked()), &clicks, SLOT(recordSignal()));
    CTEST_CHECK(clicksConnection);
    MotionSignalRecorder cancelled;
    const auto cancelledConnection = QObject::connect(button, SIGNAL(canceled()), &cancelled, SLOT(recordSignal()));
    CTEST_CHECK(cancelledConnection);
    mousePress(window, center);
    processFor(110);
    mouseMove(window, outside);
    mouseRelease(window, outside);
    MOTION_COMPARE(clicks.count, 0);
    MOTION_COMPARE(cancelled.count, 1);
    for (int i = 0; i < 25; ++i) {
        processFor(8);
        CTEST_CHECK(motion->property("deformationProgress").toReal() >= 0);
    }

    mouseClick(window, center);
    MOTION_COMPARE(clicks.count, 1);
    processFor(24);
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "reduceMotion"));
    MOTION_COMPARE(motion->property("deformationProgress").toReal(), 0.0);
    MOTION_COMPARE(motion->property("xScale").toReal(), 1.0);
    MOTION_COMPARE(motion->property("yScale").toReal(), 1.0);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "restoreMotion"));
    button->setProperty("motionEnabled", false);
    mouseClick(window, center);
    MOTION_COMPARE(clicks.count, 2);
    MOTION_COMPARE(motion->property("deformationProgress").toReal(), 0.0);
    button->setProperty("motionEnabled", true);
    for (int i = 0; i < 4; ++i) {
        mousePress(window, center);
        processFor(24);
        mouseRelease(window, center);
        processFor(24);
    }
    MOTION_COMPARE(clicks.count, 6);
    processFor(220);
    MOTION_COMPARE(motion->property("deformationProgress").toReal(), 0.0);
    mouseClick(window, center);
    button->setEnabled(false);
    MOTION_COMPARE(motion->property("deformationProgress").toReal(), 0.0);
    MOTION_COMPARE(motion->property("xScale").toReal(), 1.0);
    MOTION_COMPARE(motion->property("yScale").toReal(), 1.0);
    mouseClick(window, center);
    MOTION_COMPARE(clicks.count, 7);
}

void MotionTests::compact_button_tones_and_keyboard_focus(const QString &type, int tone)
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(compactFixture(engine, type, tone));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *next = root->findChild<QQuickItem *>("next");
    auto *background = button->property("background").value<QQuickItem *>();
    CTEST_CHECK(background);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    // Turning motion off makes colors immediate without changing state semantics.
    button->setProperty("motionEnabled", false);
    mouseMove(window, QPoint(450, 220));
    MOTION_COMPARE(background->property("color"), button->property(tone == 4
        ? "backgroundColorDisabled" : "backgroundColor"));
    MotionSignalRecorder clicks;
    const auto clicksConnection = QObject::connect(button, SIGNAL(clicked()), &clicks, SLOT(recordSignal()));
    CTEST_CHECK(clicksConnection);
    const QPoint center = button->mapToScene(button->boundingRect().center()).toPoint();
    mouseMove(window, center);
    if (tone == 4) {
        CTEST_CHECK(!button->property("activeFocusOnTab").toBool());
        mouseClick(window, center);
        MOTION_COMPARE(clicks.count, 0);
        MOTION_COMPARE(background->property("color"), button->property("backgroundColorDisabled"));
        return;
    }
    CTEST_WAIT_DEFAULT(button->property("hovered").toBool());
    MOTION_COMPARE(background->property("color"), button->property("backgroundColorHover"));
    mousePress(window, center);
    MOTION_COMPARE(background->property("color"), button->property("backgroundColorPressed"));
    mouseRelease(window, center);
    MOTION_COMPARE(clicks.count, 1);
    MOTION_COMPARE(background->property("color"), button->property("backgroundColorHover"));
    next->forceActiveFocus(Qt::TabFocusReason);
    keyClick(window, Qt::Key_Tab, Qt::ShiftModifier);
    CTEST_CHECK(button->property("visualFocus").toBool());
    sendKey(window, QEvent::KeyPress, Qt::Key_Space);
    CTEST_CHECK(button->property("visualFocus").toBool());
    sendKey(window, QEvent::KeyRelease, Qt::Key_Space);
    CTEST_CHECK(button->property("visualFocus").toBool());
    MOTION_COMPARE(clicks.count, 2);
    keyClick(window, Qt::Key_Tab);
    CTEST_CHECK(next->hasActiveFocus());
    keyClick(window, Qt::Key_Tab, Qt::ShiftModifier);
    CTEST_CHECK(button->hasActiveFocus());
}

void MotionTests::row_interaction_states(const QString &type, const QString &configuration, bool inactive, bool selected)
{
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *row = root->findChild<QQuickItem *>("row");
    auto *next = root->findChild<QQuickItem *>("next");
    auto *motion = row->findChild<QObject *>("interactionMotion");
    auto *ring = row->findChild<QQuickItem *>("buttonFocusRing");
    auto *background = row->property("background").value<QQuickItem *>();
    CTEST_CHECK(motion);
    CTEST_CHECK(ring);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    const QRectF geometry = row->mapRectToScene(row->boundingRect());
    // Leading edge belongs to the row; embedded accessory controls keep their own events.
    const QPoint point = row->mapToScene(QPointF(2, row->height() / 2)).toPoint();
    MotionSignalRecorder clicks;
    const auto clicksConnection = QObject::connect(row, SIGNAL(clicked()), &clicks, SLOT(recordSignal()));
    CTEST_CHECK(clicksConnection);
    mouseMove(window, QPoint(650, 350));
    MOTION_COMPARE(background->property("color"), row->property(inactive ? "backgroundColorDisabled" : "backgroundColor"));
    const QVariant originalFill = background->property("color");
    mouseMove(window, point);
    if (inactive) {
        mouseClick(window, point);
        CTEST_CHECK(!row->property("activeFocusOnTab").toBool());
        CTEST_CHECK(!ring->property("active").toBool());
        MOTION_COMPARE(background->property("color"), originalFill);
        MOTION_COMPARE(motion->property("xScale").toReal(), 1.0);
        MOTION_COMPARE(motion->property("yScale").toReal(), 1.0);
        if (type != "HierarchyItem")
            MOTION_COMPARE(clicks.count, 0);
        return;
    }
    CTEST_WAIT_DEFAULT(row->property("hovered").toBool());
    MOTION_COMPARE(background->property("color"), row->property("backgroundColorHover"));
    if (selected)
        MOTION_COMPARE(background->property("color"), originalFill);
    else
        CTEST_CHECK(background->property("color") != originalFill);
    mousePress(window, point);
    MOTION_COMPARE(background->property("color"), row->property("backgroundColorPressed"));
    mouseRelease(window, point);
    MOTION_COMPARE(clicks.count, 1);
    MOTION_COMPARE(background->property("color"), row->property("backgroundColorHover"));
    next->forceActiveFocus(Qt::TabFocusReason);
    // Complex rows contain independently focusable controls; explicitly focus the row itself.
    row->forceActiveFocus(Qt::TabFocusReason);
    CTEST_CHECK(ring->property("active").toBool());
    MOTION_COMPARE(ring->x(), -3.0);
    MOTION_COMPARE(ring->width(), row->width() + 6.0);
    sendKey(window, QEvent::KeyPress, Qt::Key_Space);
    CTEST_CHECK(ring->property("active").toBool());
    sendKey(window, QEvent::KeyRelease, Qt::Key_Space);
    MOTION_COMPARE(clicks.count, 2);
    CTEST_CHECK(ring->property("active").toBool());
    MOTION_COMPARE(row->mapRectToScene(row->boundingRect()), geometry);
}

void MotionTests::enter_activates_on_release(const QString &type)
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(compactFixture(engine, type));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *next = root->findChild<QQuickItem *>("next");
    auto *motion = button->findChild<QObject *>("interactionMotion");
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    button->forceActiveFocus(Qt::TabFocusReason);
    MotionSignalRecorder clicks;
    const auto clicksConnection = QObject::connect(button, SIGNAL(clicked()), &clicks, SLOT(recordSignal()));
    CTEST_CHECK(clicksConnection);
    for (Qt::Key key : {Qt::Key_Return, Qt::Key_Enter}) {
        const int before = clicks.count;
        sendKey(window, QEvent::KeyPress, key);
        CTEST_WAIT_DEFAULT(motion->property("pressProgress").toReal() > 0.5);
        MOTION_COMPARE(clicks.count, before);
        QKeyEvent repeat(QEvent::KeyPress, key, Qt::NoModifier, QString(), true);
        QCoreApplication::sendEvent(window, &repeat);
        MOTION_COMPARE(clicks.count, before);
        sendKey(window, QEvent::KeyRelease, key);
        MOTION_COMPARE(clicks.count, before + 1);
        CTEST_CHECK(button->property("visualFocus").toBool());
        // The first animation frame may already have advanced on a busy row.
        CTEST_CHECK(motion->property("releasing").toBool());
        processFor(220);
    }
    sendKey(window, QEvent::KeyPress, Qt::Key_Return);
    next->forceActiveFocus(Qt::TabFocusReason);
    sendKey(window, QEvent::KeyRelease, Qt::Key_Return);
    MOTION_COMPARE(clicks.count, 2);
    CTEST_WAIT_DEFAULT(!button->property("down").toBool());
    MOTION_COMPARE(motion->property("releaseProgress").toReal(), 0.0);
    if (type == "PushButton") {
        button->setProperty("checkable", true);
        button->setProperty("checked", false);
        button->forceActiveFocus(Qt::TabFocusReason);
        sendKey(window, QEvent::KeyPress, Qt::Key_Return);
        CTEST_CHECK(!button->property("checked").toBool());
        sendKey(window, QEvent::KeyRelease, Qt::Key_Return);
        CTEST_CHECK(button->property("checked").toBool());
        MOTION_COMPARE(clicks.count, 3);
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *row = root->findChild<QQuickItem *>("row");
    auto *motion = row->findChild<QObject *>("interactionMotion");
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    CTEST_CHECK(row->property("pointerDragRequiresLongPress").toBool());
    const QPoint center = row->mapToScene(row->boundingRect().center()).toPoint();
    MotionSignalRecorder clicks;
    const auto clicksConnection = QObject::connect(row, SIGNAL(clicked()), &clicks, SLOT(recordSignal()));
    CTEST_CHECK(clicksConnection);
    const QRectF geometry = row->mapRectToScene(row->boundingRect());
    mousePress(window, center);
    CTEST_WAIT_VERIFY(motion->property("pressProgress").toReal() > 0.7, 500);
    CTEST_CHECK(!row->property("down").toBool());
    mouseRelease(window, center);
    MOTION_COMPARE(clicks.count, 1);
    CTEST_CHECK(motion->property("releaseProgress").toReal() >= 0.35);
    processFor(220);
    mousePress(window, center);
    processFor(100);
    mouseMove(window, QPoint(450, 230));
    mouseRelease(window, QPoint(450, 230));
    MOTION_COMPARE(clicks.count, 1);
    MOTION_COMPARE(motion->property("releaseProgress").toReal(), 0.0);
    row->setProperty("dragPreviewActive", true);
    mousePress(window, center);
    processFor(100);
    MOTION_COMPARE(motion->property("xScale").toReal(), 1.0);
    mouseRelease(window, center);
    MOTION_COMPARE(motion->property("releaseProgress").toReal(), 0.0);
    MOTION_COMPARE(row->mapRectToScene(row->boundingRect()), geometry);
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    MotionTests tests;
    const auto runRow = [&](const QString &name, auto function) {
        runner.run(name.toStdString(), [&] { pressedButtons = Qt::NoButton; function(); });
    };
    for (const char *value : {"AbstractButton", "PushButton", "DropdownButton", "LabelButton", "IconButton", "LabelMenuButton", "IconMenuButton", "CheckBox", "RadioButton", "Card", "AlertButton", "Link", "ToolbarButton", "HierarchyItem", "ListItem", "MenuItem", "ContextMenuItem", "Stepper", "ComboBox"}) {
        const auto type = QString::fromLatin1(value);
        runRow(QStringLiteral("button_families:") + type, [&] { tests.button_families(type); });
    }
    for (const char *value : {"PushButton", "DropdownButton", "IconButton", "IconMenuButton", "MenuItem", "ContextMenuItem", "ListItem", "HierarchyItem"}) {
        const auto type = QString::fromLatin1(value);
        for (bool keyboard : {false, true})
            runRow(QStringLiteral("compact_button_release:") + type + (keyboard ? "-keyboard" : "-pointer"), [&] { tests.compact_button_release(type, keyboard); });
    }
    for (const char *value : {"PushButton", "MenuItem", "ContextMenuItem", "ListItem", "HierarchyItem"}) {
        const auto type = QString::fromLatin1(value);
        runRow(QStringLiteral("compact_button_cancel_and_opt_out:") + type, [&] { tests.compact_button_cancel_and_opt_out(type); });
    }
    for (const char *value : {"PushButton", "DropdownButton", "IconButton", "IconMenuButton"}) {
        const auto type = QString::fromLatin1(value);
        for (int tone = 0; tone < 5; ++tone)
            runRow(QStringLiteral("compact_button_tones_and_keyboard_focus:") + type + '-' + QString::number(tone), [&] { tests.compact_button_tones_and_keyboard_focus(type, tone); });
    }
    for (const char *value : {"MenuItem", "ContextMenuItem", "HierarchyItem"}) {
        const auto type = QString::fromLatin1(value);
        for (int state = 0; state < 3; ++state) {
            const auto configuration = type == "HierarchyItem"
                ? (state == 1 ? QStringLiteral("selected: true") : state == 2 ? QStringLiteral("activatable: false") : QString())
                : QStringLiteral("state: %1").arg(state);
            runRow(QStringLiteral("row_interaction_states:") + type + '-' + QString::number(state), [&] { tests.row_interaction_states(type, configuration, state == 2, state == 1); });
        }
    }
    for (int variant = 0; variant < 17; ++variant)
        runRow(QStringLiteral("row_interaction_states:ListItem-") + QString::number(variant), [&] { tests.row_interaction_states(QStringLiteral("ListItem"), QStringLiteral("type: %1").arg(variant), false, false); });
    runRow(QStringLiteral("row_interaction_states:ListItem-selected"), [&] { tests.row_interaction_states(QStringLiteral("ListItem"), QStringLiteral("selected: true"), false, true); });
    runRow(QStringLiteral("row_interaction_states:ListItem-disabled"), [&] { tests.row_interaction_states(QStringLiteral("ListItem"), QStringLiteral("enabled: false"), true, false); });
    for (const char *value : {"PushButton", "DropdownButton", "MenuItem", "ContextMenuItem", "ListItem", "HierarchyItem"}) {
        const auto type = QString::fromLatin1(value);
        runRow(QStringLiteral("enter_activates_on_release:") + type, [&] { tests.enter_activates_on_release(type); });
    }
    runner.run("button_press_rebound_and_retarget", [&] { pressedButtons = Qt::NoButton; tests.button_press_rebound_and_retarget(); });
    runner.run("preferences_settle_in_flight_motion", [&] { pressedButtons = Qt::NoButton; tests.preferences_settle_in_flight_motion(); });
    runner.run("keyboard_disabled_and_layout", [&] { pressedButtons = Qt::NoButton; tests.keyboard_disabled_and_layout(); });
    runner.run("slider_drag_is_direct_and_release_settles", [&] { pressedButtons = Qt::NoButton; tests.slider_drag_is_direct_and_release_settles(); });
    runner.run("slider_programmatic_travel_stays_bounded", [&] { pressedButtons = Qt::NoButton; tests.slider_programmatic_travel_stays_bounded(); });
    runner.run("touch_keeps_the_input_target_fixed", [&] { pressedButtons = Qt::NoButton; tests.touch_keeps_the_input_target_fixed(); });
    runner.run("hierarchy_mobile_release_and_cancel", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_mobile_release_and_cancel(); });
    return runner.result();
}
#include "tst_motion.moc"
