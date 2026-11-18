#include "QtEventWait.h"
#include "QtGuiTouch.h"
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QTimer>
#include <type_traits>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include "test_utils.h"

namespace {
struct PlacementVariant { const char *name; QPointF point; int preferred; int expected; };
QList<PlacementVariant> placementVariants()
{
    return {{"above", {320, 240}, 1, 1}, {"below", {320, 240}, 2, 2},
            {"left", {320, 240}, 3, 3}, {"right", {320, 240}, 4, 4},
            {"top-left", {3, 3}, 0, 2}, {"top-right", {637, 3}, 0, 2},
            {"bottom-left", {3, 477}, 0, 1}, {"bottom-right", {637, 477}, 0, 1},
            {"flip-preference", {320, 5}, 1, 2}};
}
struct HoverVariant { const char *name; bool lvrsWindow; int targetY; };
QList<HoverVariant> hoverVariants()
{
    return {{"controls-body", false, 220}, {"controls-chrome", false, 0},
            {"lvrs-body", true, 220}, {"lvrs-chrome", true, 0}};
}

template <typename Actual, typename Expected>
bool tooltipEqual(const Actual &actual, const Expected &expected)
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
void sendMouse(QQuickWindow *window, QEvent::Type type, const QPoint &point,
               Qt::MouseButton button, Qt::MouseButtons buttons)
{
    static ulong timestamp = 10000;
    const QPointF local(point), global(window->mapToGlobal(point));
    QMouseEvent event(type, local, local, global, button, buttons, Qt::NoModifier);
    event.setTimestamp(timestamp += 16);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}
void mouseMove(QQuickWindow *window, const QPoint &point)
{
    sendMouse(window, QEvent::MouseMove, point, Qt::NoButton, Qt::NoButton);
}
void mouseClick(QQuickWindow *window, const QPoint &point)
{
    sendMouse(window, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton);
    sendMouse(window, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
}
void keyClick(QQuickWindow *window, Qt::Key key)
{
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier), release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}
void waitForGrab(const QSharedPointer<QQuickItemGrabResult> &capture)
{
    CTEST_CHECK(capture);
    int readyCount = 0;
    const auto captureConnection = QObject::connect(capture.data(), &QQuickItemGrabResult::ready,
                                                     [&] { ++readyCount; });
    CTEST_CHECK(captureConnection);
    const auto disconnectCapture = qScopeGuard([&] { QObject::disconnect(captureConnection); });
    CTEST_WAIT_DEFAULT(readyCount > 0);
    CTEST_CHECK(!capture->image().isNull());
}
}
#define TOOLTIP_COMPARE(actual, expected) CTEST_CHECK(tooltipEqual((actual), (expected)))
#define TOOLTIP_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(tooltipEqual((actual), (expected)))
#define TOOLTIP_WAIT_MESSAGE(expression, message) CTEST_CHECK_MESSAGE(lvrs_ctest::waitFor([&] { return bool(expression); }, 5000), (message))

class TooltipTests
{
public:
    void placement_and_tail(const QPointF &point, int preferred, int expected);
    void constrained_viewport();
    void content_replacement_and_scrolling();
    void transformed_target_and_resize();
    void hover_delay_timeout_and_escape();
    void chrome_target_above_content_safe_area();
    void automatic_hover_does_not_consume_clicks(bool lvrsWindow, int targetY);
    void touch_hold_and_nonmodal_input();
    void gallery_loads();
};

static QByteArray fixture(const QByteArray &body = {}, bool defaultContent = true)
{
    const QByteArray content = defaultContent ? QByteArray(R"(
        contentComponent: Component {
            Rectangle { objectName: "providedView"; implicitWidth: 180; implicitHeight: 80; color: "#3A4050" }
        }
)") : QByteArray();
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

void TooltipTests::placement_and_tail(const QPointF &point, int preferred, int expected)
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture()));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    CTEST_CHECK(window && tip);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    tip->setProperty("target", QVariant::fromValue<QQuickItem *>(nullptr));
    tip->setProperty("anchorPoint", point);
    tip->setProperty("preferredPlacement", preferred);
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "open"));
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    TOOLTIP_WAIT_COMPARE(tip->property("resolvedPlacement").toInt(), expected);
    const QRectF body = tip->property("bodyRect").toRectF();
    const QPointF origin(tip->property("x").toReal(), tip->property("y").toReal());
    const QPointF tail = tip->property("tailPoint").toPointF();
    CTEST_CHECK(QLineF(origin + tail, point).length() < 0.01);
    CTEST_CHECK(!body.contains(tail));
    const QRectF globalBody = body.translated(origin);
    CTEST_CHECK(QRectF(8, 8, 624, 464).contains(globalBody));
    TOOLTIP_COMPARE(body.size(), QSizeF(204, 104));
    auto *view = tip->findChild<QQuickItem *>("providedView");
    CTEST_CHECK(view);
    TOOLTIP_COMPARE(view->width(), 180.0);
    TOOLTIP_COMPARE(view->height(), 80.0);

    // Inspect the actual filled outline, including its tail and transparent corner.
    auto *surface = tip->findChild<QQuickItem *>("tooltip_surface");
    CTEST_CHECK(surface);
    auto capture = surface->grabToImage();
    waitForGrab(capture);
    const QImage image = capture->image();
    const qreal sx = image.width() / surface->width();
    const qreal sy = image.height() / surface->height();
    const QPoint center(qRound(body.center().x() * sx), qRound(body.center().y() * sy));
    TOOLTIP_COMPARE(surface->property("tintOpacity").toReal(), 0.25);
    TOOLTIP_COMPARE(surface->property("blurRadius").toReal(), 16.0);
    CTEST_CHECK(image.pixelColor(center).alpha() >= 50);
    int tailPixels = 0;
    const QRectF bodyAndStroke = body.adjusted(-1, -1, 1, 1);
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (!bodyAndStroke.contains(QPointF((x + 0.5) / sx, (y + 0.5) / sy))
                    && image.pixelColor(x, y).alpha() > 35)
                ++tailPixels;
    CTEST_CHECK(tailPixels > 4);
}

void TooltipTests::constrained_viewport()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture()));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    CTEST_CHECK(window && tip);
    window->resize(130, 110);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    tip->setProperty("target", QVariant::fromValue<QQuickItem *>(nullptr));
    tip->setProperty("anchorPoint", QPointF(65, 55));
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "open"));
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    // No side fits 204 x 104. A side placement exposes more content than above/below.
    TOOLTIP_COMPARE(tip->property("resolvedPlacement").toInt(), 4);
    const QRectF body = tip->property("bodyRect").toRectF();
    TOOLTIP_COMPARE(body.size(), QSizeF(47, 94));
    CTEST_CHECK(tip->property("availableContentWidth").toReal() > 0);
    CTEST_CHECK(tip->property("availableContentHeight").toReal() > 0);
    auto *viewport = tip->findChild<QQuickItem *>("tooltip_viewport");
    CTEST_CHECK(viewport && viewport->property("contentHeight").toReal() > viewport->height());
    tip->setProperty("anchorPoint", QPointF(-20, 55));
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
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
    CTEST_CHECK(object);
    auto *tip = object->findChild<QObject *>("tip");
    CTEST_CHECK(tip);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { auto *w = qobject_cast<QQuickWindow *>(object.data()); return w && w->isExposed(); }, 5000));
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "open"));
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "replaceContent"));
    CTEST_WAIT_DEFAULT(tip->findChild<QQuickItem *>("longView"));
    auto *viewport = tip->findChild<QQuickItem *>("tooltip_viewport");
    CTEST_CHECK(viewport);
    CTEST_WAIT_DEFAULT(viewport->property("contentHeight").toReal() > viewport->height());
    CTEST_CHECK(tip->property("bodyRect").toRectF().width() <= 260);
    CTEST_CHECK(tip->property("bodyRect").toRectF().height() <= 180);
    viewport->setProperty("contentY", 90);
    CTEST_CHECK(viewport->property("contentY").toReal() > 0);
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "close"));
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "open"));
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    TOOLTIP_COMPARE(viewport->property("contentY").toReal(), 0.0);

    QScopedPointer<QObject> inlineObject(TestUtils::createFromQml(engine, fixture(R"(
        Rectangle { objectName: "inlineView"; width: parent.width; implicitWidth: 140; implicitHeight: 60; color: "red" }
    )", false)));
    CTEST_CHECK(inlineObject);
    auto *inlineTip = inlineObject->findChild<QObject *>("tip");
    CTEST_CHECK(lvrs_ctest::waitFor([&] { auto *w = qobject_cast<QQuickWindow *>(inlineObject.data()); return w && w->isExposed(); }, 5000));
    CTEST_CHECK(QMetaObject::invokeMethod(inlineTip, "open"));
    CTEST_WAIT_DEFAULT(inlineTip->property("opened").toBool());
    TOOLTIP_COMPARE(inlineTip->property("bodyRect").toRectF().size(), QSizeF(164, 84));
}

void TooltipTests::transformed_target_and_resize()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture()));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    auto *target = object->findChild<QQuickItem *>("trigger");
    CTEST_CHECK(window && tip && target);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "open"));
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    QQuickItem container(window->contentItem());
    container.setSize(QSizeF(400, 300));
    target->setParentItem(&container);
    target->setPosition(QPointF(25, 30));
    container.setPosition(QPointF(70, 50));
    container.setScale(0.8);
    container.setRotation(12);
    const auto expectedPoint = [&] { return target->mapToScene(target->boundingRect().center()); };
    CTEST_WAIT_DEFAULT(QLineF(tip->property("resolvedAnchorPoint").toPointF(), expectedPoint()).length() < 0.01);
    container.setX(110);
    CTEST_WAIT_DEFAULT(QLineF(tip->property("resolvedAnchorPoint").toPointF(), expectedPoint()).length() < 0.01);
    window->resize(280, 230);
    CTEST_WAIT_DEFAULT(tip->property("x").toReal() + tip->property("width").toReal() <= 280.1);
    CTEST_WAIT_DEFAULT(tip->property("y").toReal() + tip->property("height").toReal() <= 230.1);
    target->setVisible(false);
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
    target->setVisible(true);
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "open"));
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    container.setClip(true);
    container.setSize(QSizeF(20, 20));
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
    target->setParentItem(window->contentItem());
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "open"));
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    target->deleteLater();
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
}

void TooltipTests::automatic_hover_does_not_consume_clicks(bool lvrsWindow, int targetY)
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    auto source = fixture("text: \"Help\"", false);
    if (lvrsWindow) source.replace("Controls.ApplicationWindow {",
                                  "LV.ApplicationWindow { windowDragExclusionItems: [trigger]");
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, source));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    auto *target = object->findChild<QQuickItem *>("trigger");
    CTEST_CHECK(window && tip && target);
    target->setY(targetY);
    tip->setProperty("delay", 500);
    tip->setProperty("automatic", true);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    window->requestActivate();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isActive(); }, 5000));
    const auto point = target->mapToScene(target->boundingRect().center()).toPoint();
    mouseMove(window, point);
    mouseClick(window, point);
    TOOLTIP_WAIT_COMPARE(object->property("clicks").toInt(), 1);
    mouseMove(window, QPoint(20, 200));
    mouseMove(window, point);
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    mouseClick(window, point);
    TOOLTIP_WAIT_COMPARE(object->property("clicks").toInt(), 2);
}

void TooltipTests::chrome_target_above_content_safe_area()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        text: "Account"
        availableRect: Qt.rect(0, 32, parent.width, parent.height - 32)
    )", false)));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    auto *target = object->findChild<QQuickItem *>("trigger");
    CTEST_CHECK(window && tip && target);
    target->setPosition({480, 0});
    tip->setProperty("delay", 80);
    tip->setProperty("animationDuration", 120);
    tip->setProperty("automatic", true);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    window->requestActivate();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isActive(); }, 5000));
    const auto point = target->mapToScene(target->boundingRect().center()).toPoint();
    CTEST_CHECK(point.y() < 32);
    mouseMove(window, QPoint(20, 200));
    mouseMove(window, point);
    TOOLTIP_WAIT_MESSAGE(tip->property("opened").toBool(), qPrintable(QStringLiteral("ready=%1 wanted=%2 visible=%3 position=%4,%5")
        .arg(tip->property("_targetReady").toBool()).arg(tip->property("_autoWanted").toBool())
        .arg(tip->property("visible").toBool()).arg(point.x()).arg(point.y())));
    const auto body = tip->property("bodyRect").toRectF();
    CTEST_CHECK(tip->property("y").toReal() + body.y() >= 32);
    TOOLTIP_COMPARE(tip->property("resolvedAnchorPoint").toPointF(), target->mapToScene(target->boundingRect().center()));
    processFor(300);
    CTEST_CHECK(tip->property("opened").toBool());
    keyClick(window, Qt::Key_Escape);
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
    processFor(600);
    CTEST_CHECK(!tip->property("visible").toBool());
    mouseMove(window, QPoint(20, 200));
    mouseMove(window, point);
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    mouseMove(window, QPoint(20, 200));
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
    tip->setProperty("automatic", false);
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "open"));
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    target->setY(-100);
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
}

void TooltipTests::hover_delay_timeout_and_escape()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture()));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    auto *target = object->findChild<QQuickItem *>("trigger");
    CTEST_CHECK(window && tip && target);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    tip->setProperty("delay", 100);
    tip->setProperty("timeout", 400);
    tip->setProperty("automatic", true);
#ifdef Q_OS_WIN
    // Native hover delivery requires activation, beyond window exposure.
    window->requestActivate();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isActive(); }, 5000));
#endif
    mouseMove(window, QPoint(20, 20));
    const QPoint point = target->mapToScene(target->boundingRect().center()).toPoint();
    mouseMove(window, point);
    CTEST_CHECK(!tip->property("visible").toBool());
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    CTEST_CHECK(!tip->property("modal").toBool());
    CTEST_CHECK(!tip->property("focus").toBool());
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
    processFor(180);
    CTEST_CHECK(!tip->property("visible").toBool());
    mouseMove(window, QPoint(20, 20));
    mouseMove(window, point);
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    keyClick(window, Qt::Key_Escape);
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
    mouseMove(window, QPoint(20, 20));
    mouseMove(window, point);
    mouseMove(window, QPoint(20, 20));
    processFor(250);
    CTEST_CHECK(!tip->property("visible").toBool());
    target->forceActiveFocus(Qt::TabFocusReason);
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    TOOLTIP_COMPARE(window->activeFocusItem(), target);
    keyClick(window, Qt::Key_Escape);
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
    target->setFocus(false);
    tip->setProperty("timeout", -1);
    mouseMove(window, QPoint(20, 20));
    mouseMove(window, point);
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    const QPointF bodyCenter = tip->property("bodyRect").toRectF().center()
        + QPointF(tip->property("x").toReal(), tip->property("y").toReal());
    mouseMove(window, bodyCenter.toPoint());
    processFor(250);
    CTEST_CHECK(tip->property("visible").toBool());
    mouseMove(window, QPoint(20, 20));
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
}

void TooltipTests::touch_hold_and_nonmodal_input()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture()));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *tip = object->findChild<QObject *>("tip");
    auto *target = object->findChild<QQuickItem *>("trigger");
    CTEST_CHECK(window && tip && target);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    tip->setProperty("delay", 100);
    tip->setProperty("automatic", true);
    const QPoint point = target->mapToScene(target->boundingRect().center()).toPoint();
    auto *device = lvrs_ctest::createTouchDevice();
    lvrs_ctest::TouchSequence(window, device).press(0, point, window);
    CTEST_CHECK(!tip->property("visible").toBool());
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    lvrs_ctest::TouchSequence(window, device).release(0, point, window);
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
    tip->setProperty("automatic", false);
    CTEST_CHECK(QMetaObject::invokeMethod(tip, "open"));
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    const int previousClicks = object->property("clicks").toInt();
    mouseClick(window, point);
    TOOLTIP_WAIT_COMPARE(object->property("clicks").toInt(), previousClicks + 1);
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
}

void TooltipTests::gallery_loads()
{
    QQmlEngine engine;
    QStringList bindingLoopWarnings;
    const auto warningConnection = QObject::connect(&engine, &QQmlEngine::warnings, &engine,
        [&bindingLoopWarnings](const QList<QQmlError> &warnings) {
            for (const QQmlError &warning : warnings)
                if (warning.description().contains("Binding loop")) bindingLoopWarnings.append(warning.toString());
        });
    CTEST_CHECK(warningConnection);
    const auto disconnectWarnings = qScopeGuard([&] { QObject::disconnect(warningConnection); });
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(900, 500);
    window.setColor(QColor("#141414"));
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine, QDir(QStringLiteral(LVRS_TEST_SOURCE_DIR)).filePath("../example/VisualCatalog/qml/TooltipGallery.qml")));
    CTEST_CHECK(object);
    auto *gallery = qobject_cast<QQuickItem *>(object.data());
    CTEST_CHECK(gallery);
    gallery->setParentItem(window.contentItem());
    gallery->setWidth(900);
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    CTEST_CHECK(QMetaObject::invokeMethod(gallery, "showExample", Q_ARG(QVariant, 0)));
    auto *tip = gallery->findChild<QObject *>("galleryTooltip");
    CTEST_CHECK(tip);
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    const QString captureDirectory = qEnvironmentVariable("LVRS_TOOLTIP_CAPTURE_DIR");
    if (!captureDirectory.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(captureDirectory));
        processFor(160);
        CTEST_CHECK(window.grabWindow().save(captureDirectory + "/tooltip-gallery.png"));
    }
    CTEST_CHECK(QMetaObject::invokeMethod(gallery, "showExample", Q_ARG(QVariant, 2)));
    CTEST_WAIT_DEFAULT(tip->property("opened").toBool());
    auto *action = tip->findChild<QQuickItem *>("tooltipAction");
    CTEST_CHECK(action);
    mouseClick(&window,
                      action->mapToScene(action->boundingRect().center()).toPoint());
    CTEST_WAIT_DEFAULT(!tip->property("visible").toBool());
    object.reset();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    CTEST_CHECK_MESSAGE(bindingLoopWarnings.isEmpty(), qPrintable(bindingLoopWarnings.join('\n')));
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    TooltipTests tests;
    for (const PlacementVariant &variant : placementVariants()) {
        const auto name = QStringLiteral("placement_and_tail:") + QString::fromLatin1(variant.name);
        runner.run(name.toStdString(), [&] { tests.placement_and_tail(variant.point, variant.preferred, variant.expected); });
    }
    for (const HoverVariant &variant : hoverVariants()) {
        const auto name = QStringLiteral("automatic_hover_does_not_consume_clicks:") + QString::fromLatin1(variant.name);
        runner.run(name.toStdString(), [&] { tests.automatic_hover_does_not_consume_clicks(variant.lvrsWindow, variant.targetY); });
    }
    runner.run("constrained_viewport", [&] { tests.constrained_viewport(); });
    runner.run("content_replacement_and_scrolling", [&] { tests.content_replacement_and_scrolling(); });
    runner.run("transformed_target_and_resize", [&] { tests.transformed_target_and_resize(); });
    runner.run("hover_delay_timeout_and_escape", [&] { tests.hover_delay_timeout_and_escape(); });
    runner.run("chrome_target_above_content_safe_area", [&] { tests.chrome_target_above_content_safe_area(); });
    runner.run("touch_hold_and_nonmodal_input", [&] { tests.touch_hold_and_nonmodal_input(); });
    runner.run("gallery_loads", [&] { tests.gallery_loads(); });
    return runner.result();
}
