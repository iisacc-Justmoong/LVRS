#include <QtTest>
#include <QElapsedTimer>
#include <QFile>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include "test_utils.h"

class ToggleSwitchTests : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void press_and_rebound_data();
    void press_and_rebound();
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
    QVERIFY(root);
    window = qobject_cast<QQuickWindow *>(root.data());
    control = root->findChild<QQuickItem *>("toggle");
    knob = root->findChild<QQuickItem *>("toggle_knob");
    indicator = root->findChild<QQuickItem *>("toggle_indicator");
    QVERIFY(window && control && knob && indicator);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QCOMPARE(knob->x(), 2.0);
    auto *initialOnKnob = root->findChild<QQuickItem *>("initialOn_knob");
    QVERIFY(initialOnKnob);
    QCOMPARE(initialOnKnob->x(), 18.0);
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
    QCOMPARE(knob->x(), checked ? 18.0 : 2.0);
    control->setProperty("transitionDuration", duration);
}

void ToggleSwitchTests::press_and_rebound_data()
{
    QTest::addColumn<bool>("initiallyChecked");
    QTest::newRow("off-to-on") << false;
    QTest::newRow("on-to-off") << true;
}

void ToggleSwitchTests::press_and_rebound()
{
    QFETCH(bool, initiallyChecked);
    setInitialState(initiallyChecked);
    const QString captureDirectory = qEnvironmentVariable("LVRS_TOGGLE_CAPTURE_DIR");
    QFile captureTimeline;
    QElapsedTimer captureTimer;
    int frameIndex = 0;
    if (!captureDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(captureDirectory));
        window->resize(360, 200);
        control->setTransformOrigin(QQuickItem::TopLeft);
        control->setScale(4);
        control->setPosition(QPointF(104, 56));
        root->findChild<QQuickItem *>("initialOn")->setVisible(false);
        QTest::qWait(60);
        captureTimeline.setFileName(captureDirectory + "/" + QTest::currentDataTag() + ".csv");
        QVERIFY(captureTimeline.open(QIODevice::WriteOnly | QIODevice::Truncate));
        captureTimeline.write("frame,time_ms,x,width,height\n");
        captureTimer.start();
    }
    const auto captureFrame = [&] {
        if (captureDirectory.isEmpty())
            return;
        const QString name = QString("%1-%2.png").arg(QTest::currentDataTag())
            .arg(frameIndex++, 3, 10, QLatin1Char('0'));
        const QImage frame = window->grabWindow();
        QVERIFY(!frame.isNull());
        QVERIFY(frame.save(captureDirectory + "/" + name));
        captureTimeline.write(QString("%1,%2,%3,%4,%5\n").arg(name)
            .arg(captureTimer.elapsed()).arg(knob->x())
            .arg(knobBounds().width()).arg(knobBounds().height()).toUtf8());
    };
    captureFrame();
    QSignalSpy toggled(control, SIGNAL(toggled()));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, clickPoint());
    if (!captureDirectory.isEmpty()) {
        for (int i = 0; i < 6; ++i) {
            QTest::qWait(16);
            captureFrame();
        }
    }
    QTRY_VERIFY(control->property("down").toBool());
    QTRY_VERIFY_WITH_TIMEOUT(knobBounds().width() > 19.0, 300);
    QVERIFY(knobBounds().height() < 17.0);
    QCOMPARE(control->property("checked").toBool(), initiallyChecked);
    QCOMPARE(control->implicitWidth(), 38.0);
    QCOMPARE(control->implicitHeight(), 22.0);
    QCOMPARE(knob->size(), QSizeF(18, 18));

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, clickPoint());
    QCOMPARE(control->property("checked").toBool(), !initiallyChecked);
    QCOMPARE(toggled.count(), 1);

    const qreal destination = initiallyChecked ? 2.0 : 18.0;
    qreal furthestPosition = knob->x();
    qreal widestKnob = 0;
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < control->property("transitionDuration").toInt() + 120) {
        QTest::qWait(8);
        furthestPosition = initiallyChecked ? qMin(furthestPosition, knob->x())
                                           : qMax(furthestPosition, knob->x());
        widestKnob = qMax(widestKnob, knobBounds().width());
        captureFrame();
        // The rebound remains inside the authored 38 x 22 track.
        QVERIFY(knobBounds().left() >= -0.1);
        QVERIFY(knobBounds().right() <= 38.1);
        QVERIFY(knobBounds().top() >= -0.1);
        QVERIFY(knobBounds().bottom() <= 22.1);
    }
    QVERIFY(initiallyChecked ? furthestPosition < destination - 0.3
                             : furthestPosition > destination + 0.3);
    QVERIFY(widestKnob > 19.0);
    QTRY_COMPARE(knob->x(), destination);
    QTRY_VERIFY(qAbs(knobBounds().width() - 18.0) < 0.01);
    QTRY_VERIFY(qAbs(knobBounds().height() - 18.0) < 0.01);
    QCOMPARE(toggled.count(), 1);
}

void ToggleSwitchTests::quick_click_and_retarget()
{
    QSignalSpy toggled(control, SIGNAL(toggled()));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, clickPoint());
    QTRY_VERIFY_WITH_TIMEOUT(knobBounds().width() > 19.0, 200);
    QList<qreal> retargetPositions;
    const auto connection = connect(knob, &QQuickItem::xChanged, this, [&] {
        retargetPositions.append(knob->x());
    });
    for (int i = 0; i < 4; ++i) {
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, clickPoint());
        QTest::qWait(35);
    }
    disconnect(connection);
    QVERIFY(!retargetPositions.isEmpty());
    // These reversals happen during travel: even an intermediate frame must
    // not snap to an endpoint before starting the next animation.
    for (qreal position : retargetPositions)
        QVERIFY(position > 2.0 && position < 18.0);
    QCOMPARE(toggled.count(), 5);
    QVERIFY(control->property("checked").toBool());
    QTRY_COMPARE(knob->x(), 18.0);
    QTRY_VERIFY(qAbs(knobBounds().width() - 18.0) < 0.01);

    // Application-driven changes animate without emitting an input signal.
    control->setProperty("checked", false);
    QTRY_VERIFY(knob->x() < 1.7);
    QTRY_COMPARE(knob->x(), 2.0);
    QCOMPARE(toggled.count(), 5);
    QCOMPARE(control->property("state").toBool(), false);
}

void ToggleSwitchTests::keyboard_and_cancelled_press()
{
    control->forceActiveFocus();
    QTRY_VERIFY(control->hasActiveFocus());
    QSignalSpy toggled(control, SIGNAL(toggled()));
    QTest::keyPress(window, Qt::Key_Space);
    QTRY_VERIFY(knobBounds().width() > 19.0);
    QTest::keyRelease(window, Qt::Key_Space);
    QCOMPARE(toggled.count(), 1);
    QTRY_VERIFY(knob->x() > 18.3);
    QTRY_COMPARE(knob->x(), 18.0);

    // Losing the mouse grab cancels input, as when a parent steals a gesture.
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, clickPoint());
    QTRY_VERIFY(knobBounds().width() > 19.0);
    control->ungrabMouse();
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, clickPoint());
    QTRY_VERIFY(!control->property("down").toBool());
    QTRY_VERIFY(qAbs(knobBounds().width() - 18.0) < 0.01);
    QCOMPARE(toggled.count(), 1);
    QVERIFY(control->property("checked").toBool());
}

void ToggleSwitchTests::touch_press_and_rebound()
{
    auto *device = QTest::createTouchDevice();
    QSignalSpy toggled(control, SIGNAL(toggled()));
    QTest::touchEvent(window, device).press(0, clickPoint(), window);
    QTRY_VERIFY(control->property("down").toBool());
    QTRY_VERIFY(knobBounds().width() > 19.0);
    QTest::touchEvent(window, device).release(0, clickPoint(), window);
    QTRY_COMPARE(toggled.count(), 1);
    QTRY_VERIFY(knob->x() > 18.3);
    QTRY_COMPARE(knob->x(), 18.0);
    QTRY_VERIFY(qAbs(knobBounds().size().width() - 18.0) < 0.01);
}

void ToggleSwitchTests::drag_and_mirroring()
{
    // Native Switch drag and RTL position must not fight the visual animation.
    const QPoint start = indicator->mapToScene(QPointF(4, 11)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, start);
    QTest::mouseMove(window, start + QPoint(23, 0), 30);
    QTRY_VERIFY(control->property("position").toReal() > 0.5);
    const qreal visualPosition = control->property("visualPosition").toReal();
    QVERIFY(qAbs(knob->x() - (2.0 + visualPosition * 16.0)) < 0.1);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, start + QPoint(23, 0));
    QTRY_VERIFY(control->property("checked").toBool());
    QTRY_COMPARE(knob->x(), 18.0);

    control->setProperty("transitionDuration", 0);
    // Set the QML attached property through a binding evaluated in its context.
    QQmlExpression mirror(qmlContext(control), control,
                          "LayoutMirroring.enabled = true");
    mirror.evaluate();
    QVERIFY2(!mirror.hasError(), qPrintable(mirror.error().toString()));
    QTRY_VERIFY(control->property("mirrored").toBool());
    QCOMPARE(knob->x(), 2.0);
    control->setProperty("checked", false);
    QCOMPARE(knob->x(), 18.0);
}

void ToggleSwitchTests::immediate_and_disabled_states()
{
    control->setProperty("transitionDuration", 0);
    QSignalSpy toggled(control, SIGNAL(toggled()));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, clickPoint());
    QTest::qWait(100);
    QCOMPARE(knobBounds().size(), QSizeF(18, 18));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, clickPoint());
    QCOMPARE(knob->x(), 18.0);
    QCOMPARE(toggled.count(), 1);
    control->setProperty("state", false);
    QCOMPARE(knob->x(), 2.0);

    control->setProperty("transitionDuration", 320);
    control->setEnabled(false);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, clickPoint());
    QTest::keyClick(window, Qt::Key_Space);
    QCOMPARE(toggled.count(), 1);
    QVERIFY(!control->property("checked").toBool());
    QCOMPARE(knobBounds().size(), QSizeF(18, 18));
    control->setProperty("checked", true);
    QCOMPARE(knob->x(), 18.0);
}

QTEST_MAIN(ToggleSwitchTests)
#include "tst_toggle_switch.moc"
