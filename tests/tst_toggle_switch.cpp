#include "QtEventWait.h"
#include "QtGuiTouch.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QTimer>
#include <type_traits>
#include <QElapsedTimer>
#include <QFile>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include "test_utils.h"

class ToggleSignalRecorder : public QObject
{
    Q_OBJECT
public:
    int count = 0;
public slots:
    void recordToggled() { ++count; }
};

namespace {
template <typename Actual, typename Expected>
bool toggleEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual), qreal(expected));
    else
        return actual == expected;
}

void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(milliseconds);
    loop.exec();
}

void sendMouse(QQuickWindow *window, QEvent::Type type, const QPoint &point,
               Qt::MouseButton button, Qt::MouseButtons buttons, ulong delay = 16)
{
    static ulong timestamp = 10000;
    timestamp += delay;
    const QPointF local(point);
    QMouseEvent event(type, local, local, QPointF(window->mapToGlobal(point)),
                      button, buttons, Qt::NoModifier);
    event.setTimestamp(timestamp);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}

void mouseClick(QQuickWindow *window, const QPoint &point)
{
    sendMouse(window, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton);
    sendMouse(window, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
}

void sendKey(QQuickWindow *window, QEvent::Type type, Qt::Key key)
{
    QKeyEvent event(type, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}
}

#define TOGGLE_COMPARE(actual, expected) CTEST_CHECK(toggleEqual((actual), (expected)))
#define TOGGLE_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(toggleEqual((actual), (expected)))

class ToggleSwitchTests
{
public:
    void init();
    void cleanup();
    void press_and_rebound(bool initiallyChecked, const QString &dataTag);
    void quick_click_and_retarget();
    void keyboard_and_cancelled_press();
    void touch_press_and_rebound();
    void drag_and_mirroring();
    void immediate_and_disabled_states();

private:
    QPoint clickPoint() const;
    QRectF knobBounds() const;
    void setInitialState(bool checked);

    QScopedPointer<QQmlEngine> engine;
    QScopedPointer<QObject> root;
    QQuickWindow *window = nullptr;
    QQuickItem *control = nullptr;
    QQuickItem *knob = nullptr;
    QQuickItem *indicator = nullptr;
};

void ToggleSwitchTests::init()
{
    engine.reset(new QQmlEngine);
    engine->addImportPath(TestUtils::qmlImportBase());
    root.reset(TestUtils::createFromQml(*engine, R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV

Controls.ApplicationWindow {
    width: 240; height: 130; visible: true
    color: LV.Theme.window
    LV.ToggleSwitch {
        objectName: "toggle"; x: 80; y: 50
        hoverEnabled: false
    }
    LV.ToggleSwitch {
        objectName: "initialOn"; x: 140; y: 50
        checked: true
    }
})"));
    CTEST_CHECK(root);
    window = qobject_cast<QQuickWindow *>(root.data());
    control = root->findChild<QQuickItem *>("toggle");
    knob = root->findChild<QQuickItem *>("toggle_knob");
    indicator = root->findChild<QQuickItem *>("toggle_indicator");
    CTEST_CHECK(window && control && knob && indicator);
    CTEST_WAIT_DEFAULT(window->isExposed());
    TOGGLE_COMPARE(knob->x(), 2.0);
    auto *initialOnKnob = root->findChild<QQuickItem *>("initialOn_knob");
    CTEST_CHECK(initialOnKnob);
    TOGGLE_COMPARE(initialOnKnob->x(), 18.0);
}

void ToggleSwitchTests::cleanup()
{
    root.reset();
    engine.reset();
}

QPoint ToggleSwitchTests::clickPoint() const
{
    return indicator->mapToScene(indicator->boundingRect().center()).toPoint();
}

QRectF ToggleSwitchTests::knobBounds() const
{
    return knob->mapRectToItem(indicator, knob->boundingRect());
}

void ToggleSwitchTests::setInitialState(bool checked)
{
    const int duration = control->property("transitionDuration").toInt();
    control->setProperty("transitionDuration", 0);
    control->setProperty("checked", checked);
    TOGGLE_COMPARE(knob->x(), checked ? 18.0 : 2.0);
    control->setProperty("transitionDuration", duration);
}

void ToggleSwitchTests::press_and_rebound(bool initiallyChecked, const QString &dataTag)
{
    setInitialState(initiallyChecked);
    const QString captureDirectory = qEnvironmentVariable("LVRS_TOGGLE_CAPTURE_DIR");
    QFile captureTimeline;
    QElapsedTimer captureTimer;
    int frameIndex = 0;
    if (!captureDirectory.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(captureDirectory));
        window->resize(360, 200);
        control->setTransformOrigin(QQuickItem::TopLeft);
        control->setScale(4);
        control->setPosition(QPointF(104, 56));
        root->findChild<QQuickItem *>("initialOn")->setVisible(false);
        processFor(60);
        captureTimeline.setFileName(captureDirectory + "/" + dataTag + ".csv");
        CTEST_CHECK(captureTimeline.open(QIODevice::WriteOnly | QIODevice::Truncate));
        captureTimeline.write("frame,time_ms,x,width,height\n");
        captureTimer.start();
    }
    const auto captureFrame = [&] {
        if (captureDirectory.isEmpty())
            return;
        const QString name = QString("%1-%2.png").arg(dataTag)
            .arg(frameIndex++, 3, 10, QLatin1Char('0'));
        const QImage frame = window->grabWindow();
        CTEST_CHECK(!frame.isNull());
        CTEST_CHECK(frame.save(captureDirectory + "/" + name));
        captureTimeline.write(QString("%1,%2,%3,%4,%5\n").arg(name)
            .arg(captureTimer.elapsed()).arg(knob->x())
            .arg(knobBounds().width()).arg(knobBounds().height()).toUtf8());
    };
    captureFrame();
    ToggleSignalRecorder toggled;
    const auto toggledConnection = QObject::connect(control, SIGNAL(toggled()),
                                                      &toggled, SLOT(recordToggled()));
    CTEST_CHECK(toggledConnection);
    sendMouse(window, QEvent::MouseButtonPress, clickPoint(), Qt::LeftButton, Qt::LeftButton);
    if (!captureDirectory.isEmpty()) {
        for (int i = 0; i < 6; ++i) {
            processFor(16);
            captureFrame();
        }
    }
    CTEST_WAIT_DEFAULT(control->property("down").toBool());
    CTEST_WAIT_VERIFY(knobBounds().width() > 19.0, 300);
    CTEST_CHECK(knobBounds().height() < 17.0);
    TOGGLE_COMPARE(control->property("checked").toBool(), initiallyChecked);
    TOGGLE_COMPARE(control->implicitWidth(), 38.0);
    TOGGLE_COMPARE(control->implicitHeight(), 22.0);
    TOGGLE_COMPARE(knob->size(), QSizeF(18, 18));

    sendMouse(window, QEvent::MouseButtonRelease, clickPoint(), Qt::LeftButton, Qt::NoButton);
    TOGGLE_COMPARE(control->property("checked").toBool(), !initiallyChecked);
    TOGGLE_COMPARE(toggled.count, 1);

    const qreal destination = initiallyChecked ? 2.0 : 18.0;
    qreal furthestPosition = knob->x();
    qreal widestKnob = 0;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < control->property("transitionDuration").toInt() + 120) {
        processFor(8);
        furthestPosition = initiallyChecked ? qMin(furthestPosition, knob->x())
                                           : qMax(furthestPosition, knob->x());
        widestKnob = qMax(widestKnob, knobBounds().width());
        captureFrame();
        // The rebound remains inside the authored 38 x 22 track.
        CTEST_CHECK(knobBounds().left() >= -0.1);
        CTEST_CHECK(knobBounds().right() <= 38.1);
        CTEST_CHECK(knobBounds().top() >= -0.1);
        CTEST_CHECK(knobBounds().bottom() <= 22.1);
    }
    CTEST_CHECK(initiallyChecked ? furthestPosition < destination - 0.3
                             : furthestPosition > destination + 0.3);
    CTEST_CHECK(widestKnob > 19.0);
    TOGGLE_WAIT_COMPARE(knob->x(), destination);
    CTEST_WAIT_DEFAULT(qAbs(knobBounds().width() - 18.0) < 0.01);
    CTEST_WAIT_DEFAULT(qAbs(knobBounds().height() - 18.0) < 0.01);
    TOGGLE_COMPARE(toggled.count, 1);
}

void ToggleSwitchTests::quick_click_and_retarget()
{
    ToggleSignalRecorder toggled;
    const auto toggledConnection = QObject::connect(control, SIGNAL(toggled()),
                                                      &toggled, SLOT(recordToggled()));
    CTEST_CHECK(toggledConnection);
    mouseClick(window, clickPoint());
    CTEST_WAIT_VERIFY(knobBounds().width() > 19.0, 200);
    QList<qreal> retargetPositions;
    const auto connection = QObject::connect(knob, &QQuickItem::xChanged, knob, [&] {
        retargetPositions.append(knob->x());
    });
    CTEST_CHECK(connection);
    const auto disconnectPosition = qScopeGuard([&] { QObject::disconnect(connection); });
    for (int i = 0; i < 4; ++i) {
        mouseClick(window, clickPoint());
        processFor(35);
    }
    QObject::disconnect(connection);
    CTEST_CHECK(!retargetPositions.isEmpty());
    // These reversals happen during travel: even an intermediate frame must
    // not snap to an endpoint before starting the next animation.
    for (qreal position : retargetPositions)
        CTEST_CHECK(position > 2.0 && position < 18.0);
    TOGGLE_COMPARE(toggled.count, 5);
    CTEST_CHECK(control->property("checked").toBool());
    TOGGLE_WAIT_COMPARE(knob->x(), 18.0);
    CTEST_WAIT_DEFAULT(qAbs(knobBounds().width() - 18.0) < 0.01);

    // Application-driven changes animate without emitting an input signal.
    control->setProperty("checked", false);
    CTEST_WAIT_DEFAULT(knob->x() < 1.7);
    TOGGLE_WAIT_COMPARE(knob->x(), 2.0);
    TOGGLE_COMPARE(toggled.count, 5);
    TOGGLE_COMPARE(control->property("state").toBool(), false);
}

void ToggleSwitchTests::keyboard_and_cancelled_press()
{
    control->forceActiveFocus();
    CTEST_WAIT_DEFAULT(control->hasActiveFocus());
    ToggleSignalRecorder toggled;
    const auto toggledConnection = QObject::connect(control, SIGNAL(toggled()),
                                                      &toggled, SLOT(recordToggled()));
    CTEST_CHECK(toggledConnection);
    sendKey(window, QEvent::KeyPress, Qt::Key_Space);
    CTEST_WAIT_DEFAULT(knobBounds().width() > 19.0);
    sendKey(window, QEvent::KeyRelease, Qt::Key_Space);
    TOGGLE_COMPARE(toggled.count, 1);
    CTEST_WAIT_DEFAULT(knob->x() > 18.3);
    TOGGLE_WAIT_COMPARE(knob->x(), 18.0);

    // Losing the mouse grab cancels input, as when a parent steals a gesture.
    sendMouse(window, QEvent::MouseButtonPress, clickPoint(), Qt::LeftButton, Qt::LeftButton);
    CTEST_WAIT_DEFAULT(knobBounds().width() > 19.0);
    control->ungrabMouse();
    sendMouse(window, QEvent::MouseButtonRelease, clickPoint(), Qt::LeftButton, Qt::NoButton);
    CTEST_WAIT_DEFAULT(!control->property("down").toBool());
    CTEST_WAIT_DEFAULT(qAbs(knobBounds().width() - 18.0) < 0.01);
    TOGGLE_COMPARE(toggled.count, 1);
    CTEST_CHECK(control->property("checked").toBool());
}

void ToggleSwitchTests::touch_press_and_rebound()
{
    auto *device = lvrs_ctest::createTouchDevice();
    ToggleSignalRecorder toggled;
    const auto toggledConnection = QObject::connect(control, SIGNAL(toggled()),
                                                      &toggled, SLOT(recordToggled()));
    CTEST_CHECK(toggledConnection);
    lvrs_ctest::TouchSequence(window, device).press(0, clickPoint(), window);
    CTEST_WAIT_DEFAULT(control->property("down").toBool());
    CTEST_WAIT_DEFAULT(knobBounds().width() > 19.0);
    lvrs_ctest::TouchSequence(window, device).release(0, clickPoint(), window);
    TOGGLE_WAIT_COMPARE(toggled.count, 1);
    CTEST_WAIT_DEFAULT(knob->x() > 18.3);
    TOGGLE_WAIT_COMPARE(knob->x(), 18.0);
    CTEST_WAIT_DEFAULT(qAbs(knobBounds().size().width() - 18.0) < 0.01);
}

void ToggleSwitchTests::drag_and_mirroring()
{
    // Native Switch drag and RTL position must not fight the visual animation.
    const QPoint start = indicator->mapToScene(QPointF(4, 11)).toPoint();
    sendMouse(window, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    sendMouse(window, QEvent::MouseMove, start + QPoint(23, 0), Qt::NoButton, Qt::LeftButton, 30);
    CTEST_WAIT_DEFAULT(control->property("position").toReal() > 0.5);
    const qreal visualPosition = control->property("visualPosition").toReal();
    CTEST_CHECK(qAbs(knob->x() - (2.0 + visualPosition * 16.0)) < 0.1);
    sendMouse(window, QEvent::MouseButtonRelease, start + QPoint(23, 0), Qt::LeftButton, Qt::NoButton);
    CTEST_WAIT_DEFAULT(control->property("checked").toBool());
    TOGGLE_WAIT_COMPARE(knob->x(), 18.0);

    control->setProperty("transitionDuration", 0);
    // Set the QML attached property through a binding evaluated in its context.
    QQmlExpression mirror(qmlContext(control), control,
                          "LayoutMirroring.enabled = true");
    mirror.evaluate();
    CTEST_CHECK_MESSAGE(!mirror.hasError(), qPrintable(mirror.error().toString()));
    CTEST_WAIT_DEFAULT(control->property("mirrored").toBool());
    TOGGLE_COMPARE(knob->x(), 2.0);
    control->setProperty("checked", false);
    TOGGLE_COMPARE(knob->x(), 18.0);
}

void ToggleSwitchTests::immediate_and_disabled_states()
{
    control->setProperty("transitionDuration", 0);
    ToggleSignalRecorder toggled;
    const auto toggledConnection = QObject::connect(control, SIGNAL(toggled()),
                                                      &toggled, SLOT(recordToggled()));
    CTEST_CHECK(toggledConnection);
    sendMouse(window, QEvent::MouseButtonPress, clickPoint(), Qt::LeftButton, Qt::LeftButton);
    processFor(100);
    TOGGLE_COMPARE(knobBounds().size(), QSizeF(18, 18));
    sendMouse(window, QEvent::MouseButtonRelease, clickPoint(), Qt::LeftButton, Qt::NoButton);
    TOGGLE_COMPARE(knob->x(), 18.0);
    TOGGLE_COMPARE(toggled.count, 1);
    control->setProperty("state", false);
    TOGGLE_COMPARE(knob->x(), 2.0);

    control->setProperty("transitionDuration", 320);
    control->setEnabled(false);
    mouseClick(window, clickPoint());
    sendKey(window, QEvent::KeyPress, Qt::Key_Space); sendKey(window, QEvent::KeyRelease, Qt::Key_Space);
    TOGGLE_COMPARE(toggled.count, 1);
    CTEST_CHECK(!control->property("checked").toBool());
    TOGGLE_COMPARE(knobBounds().size(), QSizeF(18, 18));
    control->setProperty("checked", true);
    TOGGLE_COMPARE(knob->x(), 18.0);
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    auto runCase = [&](const char *name, auto function) {
        runner.run(name, [&] {
            ToggleSwitchTests tests;
            tests.init();
            const auto cleanup = qScopeGuard([&] { tests.cleanup(); });
            function(tests);
        });
    };
    runCase("press_and_rebound:off-to-on", [](auto &tests) { tests.press_and_rebound(false, QStringLiteral("off-to-on")); });
    runCase("press_and_rebound:on-to-off", [](auto &tests) { tests.press_and_rebound(true, QStringLiteral("on-to-off")); });
    runCase("quick_click_and_retarget", [](auto &tests) { tests.quick_click_and_retarget(); });
    runCase("keyboard_and_cancelled_press", [](auto &tests) { tests.keyboard_and_cancelled_press(); });
    runCase("touch_press_and_rebound", [](auto &tests) { tests.touch_press_and_rebound(); });
    runCase("drag_and_mirroring", [](auto &tests) { tests.drag_and_mirroring(); });
    runCase("immediate_and_disabled_states", [](auto &tests) { tests.immediate_and_disabled_states(); });
    return runner.result();
}
#include "tst_toggle_switch.moc"
