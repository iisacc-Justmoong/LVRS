#include <QtTest>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QSignalSpy>
#include "test_utils.h"

class TooltipTests : public QObject
{
    Q_OBJECT
private slots:
    void placement_and_tail_data();
    void placement_and_tail();
    void constrained_viewport();
    void content_replacement_and_scrolling();
    void transformed_target_and_resize();
    void hover_delay_timeout_and_escape();
    void touch_hold_and_nonmodal_input();
    void gallery_loads();
};

static QByteArray fixture(const QByteArray &body = {}, bool defaultContent = true)
{
    const QByteArray content = defaultContent ? R"(
        contentComponent: Component {
            Rectangle { objectName: "providedView"; implicitWidth: 180; implicitHeight: 80; color: "#3A4050" }
        }
)" : QByteArray();
    return R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    width: 640; height: 480; visible: true; color: LV.Theme.window
    property int clicks: 0
    LV.LabelButton { id: trigger; objectName: "trigger"; x: 280; y: 220; text: "Help"; onClicked: clicks++ }
    LV.Tooltip {
        id: tip; objectName: "tip"; target: trigger
        automatic: false; delay: 0; timeout: -1; animationDuration: 0
)" + content + body + R"(
    }
})";
}

void TooltipTests::placement_and_tail_data()
{
    QTest::addColumn<QPointF>("point");
    QTest::addColumn<int>("preferred");
    QTest::addColumn<int>("expected");
    QTest::newRow("above") << QPointF(320, 240) << 1 << 1;
    QTest::newRow("below") << QPointF(320, 240) << 2 << 2;
    QTest::newRow("left") << QPointF(320, 240) << 3 << 3;
    QTest::newRow("right") << QPointF(320, 240) << 4 << 4;
    QTest::newRow("top-left") << QPointF(3, 3) << 0 << 2;
    QTest::newRow("top-right") << QPointF(637, 3) << 0 << 2;
    QTest::newRow("bottom-left") << QPointF(3, 477) << 0 << 1;
    QTest::newRow("bottom-right") << QPointF(637, 477) << 0 << 1;
    QTest::newRow("flip-preference") << QPointF(320, 5) << 1 << 2;
}

void TooltipTests::placement_and_tail()
{
    QFETCH(QPointF, point); QFETCH(int, preferred); QFETCH(int, expected);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture()));
    QVERIFY(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    QVERIFY(window && tip);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    tip->setProperty("target", QVariant::fromValue<QQuickItem *>(nullptr));
    tip->setProperty("anchorPoint", point);
    tip->setProperty("preferredPlacement", preferred);
    QVERIFY(QMetaObject::invokeMethod(tip, "open"));
    QTRY_VERIFY(tip->property("opened").toBool());
    QTRY_COMPARE(tip->property("resolvedPlacement").toInt(), expected);
    const QRectF body = tip->property("bodyRect").toRectF();
    const QPointF origin(tip->property("x").toReal(), tip->property("y").toReal());
    const QPointF tail = tip->property("tailPoint").toPointF();
    QVERIFY(QLineF(origin + tail, point).length() < 0.01);
    QVERIFY(!body.contains(tail));
    const QRectF globalBody = body.translated(origin);
    QVERIFY(QRectF(8, 8, 624, 464).contains(globalBody));
    QCOMPARE(body.size(), QSizeF(204, 104));
    auto *view = tip->findChild<QQuickItem *>("providedView");
    QVERIFY(view);
    QCOMPARE(view->width(), 180.0);
    QCOMPARE(view->height(), 80.0);

    // Inspect the actual filled outline, including its tail and transparent corner.
    auto *surface = tip->findChild<QQuickItem *>("tooltip_surface");
    QVERIFY(surface);
    auto capture = surface->grabToImage();
    QVERIFY(capture);
    QTRY_VERIFY(!capture->image().isNull());
    const QImage image = capture->image();
    const qreal sx = image.width() / surface->width();
    const qreal sy = image.height() / surface->height();
    const QPoint center(qRound(body.center().x() * sx), qRound(body.center().y() * sy));
    QCOMPARE(surface->property("tintOpacity").toReal(), 0.25);
    QCOMPARE(surface->property("blurRadius").toReal(), 16.0);
    QVERIFY(image.pixelColor(center).alpha() >= 50);
    int tailPixels = 0;
    const QRectF bodyAndStroke = body.adjusted(-1, -1, 1, 1);
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (!bodyAndStroke.contains(QPointF((x + 0.5) / sx, (y + 0.5) / sy))
                    && image.pixelColor(x, y).alpha() > 35)
                ++tailPixels;
    QVERIFY(tailPixels > 4);
}

void TooltipTests::constrained_viewport()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture()));
    QVERIFY(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    QVERIFY(window && tip);
    window->resize(130, 110);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    tip->setProperty("target", QVariant::fromValue<QQuickItem *>(nullptr));
    tip->setProperty("anchorPoint", QPointF(65, 55));
    QVERIFY(QMetaObject::invokeMethod(tip, "open"));
    QTRY_VERIFY(tip->property("opened").toBool());
    // No side fits 204 x 104. A side placement exposes more content than above/below.
    QCOMPARE(tip->property("resolvedPlacement").toInt(), 4);
    const QRectF body = tip->property("bodyRect").toRectF();
    QCOMPARE(body.size(), QSizeF(47, 94));
    QVERIFY(tip->property("availableContentWidth").toReal() > 0);
    QVERIFY(tip->property("availableContentHeight").toReal() > 0);
    auto *viewport = tip->findChild<QQuickItem *>("tooltip_viewport");
    QVERIFY(viewport && viewport->property("contentHeight").toReal() > viewport->height());
    tip->setProperty("anchorPoint", QPointF(-20, 55));
    QTRY_VERIFY(!tip->property("visible").toBool());
}

void TooltipTests::content_replacement_and_scrolling()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        maximumWidth: 260; maximumHeight: 180
        property Component replacement: Component {
            Item {
                objectName: "longView"; implicitWidth: 400; implicitHeight: column.implicitHeight
                Column {
                    id: column; width: parent.width; spacing: 8
                    Repeater { model: 20; LV.Label { text: "Detail " + index; style: body } }
                }
            }
        }
        function replaceContent() { contentComponent = replacement }
    )")));
    QVERIFY(object);
    auto *tip = object->findChild<QObject *>("tip");
    QVERIFY(tip);
    QVERIFY(QTest::qWaitForWindowExposed(qobject_cast<QQuickWindow *>(object.data())));
    QVERIFY(QMetaObject::invokeMethod(tip, "open"));
    QTRY_VERIFY(tip->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(tip, "replaceContent"));
    QTRY_VERIFY(tip->findChild<QQuickItem *>("longView"));
    auto *viewport = tip->findChild<QQuickItem *>("tooltip_viewport");
    QVERIFY(viewport);
    QTRY_VERIFY(viewport->property("contentHeight").toReal() > viewport->height());
    QVERIFY(tip->property("bodyRect").toRectF().width() <= 260);
    QVERIFY(tip->property("bodyRect").toRectF().height() <= 180);
    viewport->setProperty("contentY", 90);
    QVERIFY(viewport->property("contentY").toReal() > 0);
    QVERIFY(QMetaObject::invokeMethod(tip, "close"));
    QTRY_VERIFY(!tip->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(tip, "open"));
    QTRY_VERIFY(tip->property("opened").toBool());
    QCOMPARE(viewport->property("contentY").toReal(), 0.0);

    QScopedPointer<QObject> inlineObject(TestUtils::createFromQml(engine, fixture(R"(
        Rectangle { objectName: "inlineView"; width: parent.width; implicitWidth: 140; implicitHeight: 60; color: "red" }
    )", false)));
    QVERIFY(inlineObject);
    auto *inlineTip = inlineObject->findChild<QObject *>("tip");
    QVERIFY(QTest::qWaitForWindowExposed(qobject_cast<QQuickWindow *>(inlineObject.data())));
    QVERIFY(QMetaObject::invokeMethod(inlineTip, "open"));
    QTRY_VERIFY(inlineTip->property("opened").toBool());
    QCOMPARE(inlineTip->property("bodyRect").toRectF().size(), QSizeF(164, 84));
}

void TooltipTests::transformed_target_and_resize()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture()));
    QVERIFY(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    auto *target = object->findChild<QQuickItem *>("trigger");
    QVERIFY(window && tip && target);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(QMetaObject::invokeMethod(tip, "open"));
    QTRY_VERIFY(tip->property("opened").toBool());
    QQuickItem container(window->contentItem());
    container.setSize(QSizeF(400, 300));
    target->setParentItem(&container);
    target->setPosition(QPointF(25, 30));
    container.setPosition(QPointF(70, 50));
    container.setScale(0.8);
    container.setRotation(12);
    const auto expectedPoint = [&] { return target->mapToScene(target->boundingRect().center()); };
    QTRY_VERIFY(QLineF(tip->property("resolvedAnchorPoint").toPointF(), expectedPoint()).length() < 0.01);
    container.setX(110);
    QTRY_VERIFY(QLineF(tip->property("resolvedAnchorPoint").toPointF(), expectedPoint()).length() < 0.01);
    window->resize(280, 230);
    QTRY_VERIFY(tip->property("x").toReal() + tip->property("width").toReal() <= 280.1);
    QTRY_VERIFY(tip->property("y").toReal() + tip->property("height").toReal() <= 230.1);
    target->setVisible(false);
    QTRY_VERIFY(!tip->property("visible").toBool());
    target->setVisible(true);
    QVERIFY(QMetaObject::invokeMethod(tip, "open"));
    QTRY_VERIFY(tip->property("opened").toBool());
    container.setClip(true);
    container.setSize(QSizeF(20, 20));
    QTRY_VERIFY(!tip->property("visible").toBool());
    target->setParentItem(window->contentItem());
    QVERIFY(QMetaObject::invokeMethod(tip, "open"));
    QTRY_VERIFY(tip->property("opened").toBool());
    target->deleteLater();
    QTRY_VERIFY(!tip->property("visible").toBool());
}

void TooltipTests::hover_delay_timeout_and_escape()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture()));
    QVERIFY(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    auto *target = object->findChild<QQuickItem *>("trigger");
    QVERIFY(window && tip && target);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    tip->setProperty("delay", 100);
    tip->setProperty("timeout", 400);
    tip->setProperty("automatic", true);
    const QPoint point = target->mapToScene(target->boundingRect().center()).toPoint();
    QTest::mouseMove(window, point);
    QVERIFY(!tip->property("visible").toBool());
    QTRY_VERIFY(tip->property("opened").toBool());
    QVERIFY(!tip->property("modal").toBool());
    QVERIFY(!tip->property("focus").toBool());
    QTRY_VERIFY(!tip->property("visible").toBool());
    QTest::qWait(180);
    QVERIFY(!tip->property("visible").toBool());
    QTest::mouseMove(window, QPoint(20, 20));
    QTest::mouseMove(window, point);
    QTRY_VERIFY(tip->property("opened").toBool());
    QTest::keyClick(window, Qt::Key_Escape);
    QTRY_VERIFY(!tip->property("visible").toBool());
    QTest::mouseMove(window, QPoint(20, 20));
    QTest::mouseMove(window, point);
    QTest::mouseMove(window, QPoint(20, 20));
    QTest::qWait(250);
    QVERIFY(!tip->property("visible").toBool());
    target->forceActiveFocus(Qt::TabFocusReason);
    QTRY_VERIFY(tip->property("opened").toBool());
    QCOMPARE(window->activeFocusItem(), target);
    QTest::keyClick(window, Qt::Key_Escape);
    QTRY_VERIFY(!tip->property("visible").toBool());
    target->setFocus(false);
    tip->setProperty("timeout", -1);
    QTest::mouseMove(window, QPoint(20, 20));
    QTest::mouseMove(window, point);
    QTRY_VERIFY(tip->property("opened").toBool());
    const QPointF bodyCenter = tip->property("bodyRect").toRectF().center()
        + QPointF(tip->property("x").toReal(), tip->property("y").toReal());
    QTest::mouseMove(window, bodyCenter.toPoint());
    QTest::qWait(250);
    QVERIFY(tip->property("visible").toBool());
    QTest::mouseMove(window, QPoint(20, 20));
    QTRY_VERIFY(!tip->property("visible").toBool());
}

void TooltipTests::touch_hold_and_nonmodal_input()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture()));
    QVERIFY(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    auto *target = object->findChild<QQuickItem *>("trigger");
    QVERIFY(window && tip && target);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    tip->setProperty("delay", 100);
    tip->setProperty("automatic", true);
    const QPoint point = target->mapToScene(target->boundingRect().center()).toPoint();
    auto *device = QTest::createTouchDevice();
    QTest::touchEvent(window, device).press(0, point, window);
    QVERIFY(!tip->property("visible").toBool());
    QTRY_VERIFY(tip->property("opened").toBool());
    QTest::touchEvent(window, device).release(0, point, window);
    QTRY_VERIFY(!tip->property("visible").toBool());
    tip->setProperty("automatic", false);
    QVERIFY(QMetaObject::invokeMethod(tip, "open"));
    QTRY_VERIFY(tip->property("opened").toBool());
    const int previousClicks = object->property("clicks").toInt();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point);
    QTRY_COMPARE(object->property("clicks").toInt(), previousClicks + 1);
    QTRY_VERIFY(!tip->property("visible").toBool());
}

void TooltipTests::gallery_loads()
{
    QQmlEngine engine;
    connect(&engine, &QQmlEngine::warnings, this, [](const QList<QQmlError> &warnings) {
        for (const QQmlError &warning : warnings)
            QVERIFY2(!warning.description().contains("Binding loop"), qPrintable(warning.toString()));
    });
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(900, 500);
    window.setColor(QColor("#141414"));
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine, QFINDTESTDATA("../example/VisualCatalog/qml/TooltipGallery.qml")));
    QVERIFY(object);
    auto *gallery = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(gallery);
    gallery->setParentItem(window.contentItem());
    gallery->setWidth(900);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QVERIFY(QMetaObject::invokeMethod(gallery, "showExample", Q_ARG(QVariant, 0)));
    auto *tip = gallery->findChild<QObject *>("galleryTooltip");
    QVERIFY(tip);
    QTRY_VERIFY(tip->property("opened").toBool());
    const QString captureDirectory = qEnvironmentVariable("LVRS_TOOLTIP_CAPTURE_DIR");
    if (!captureDirectory.isEmpty()) {
        QVERIFY(QDir().mkpath(captureDirectory));
        QTest::qWait(160);
        QVERIFY(window.grabWindow().save(captureDirectory + "/tooltip-gallery.png"));
    }
    QVERIFY(QMetaObject::invokeMethod(gallery, "showExample", Q_ARG(QVariant, 2)));
    QTRY_VERIFY(tip->property("opened").toBool());
    auto *action = tip->findChild<QQuickItem *>("tooltipAction");
    QVERIFY(action);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier,
                      action->mapToScene(action->boundingRect().center()).toPoint());
    QTRY_VERIFY(!tip->property("visible").toBool());
}

QTEST_MAIN(TooltipTests)
#include "tst_tooltip.moc"
