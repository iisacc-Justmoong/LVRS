#include "QtEventWait.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QTimer>
#include <type_traits>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QQmlEngine>
#include <limits>
#include "test_utils.h"

class SheetSignalRecorder : public QObject
{
    Q_OBJECT
public:
    int closedCount = 0;
public slots:
    void recordClosed() { ++closedCount; }
};

namespace {
template <typename Actual, typename Expected>
bool sheetEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual), qreal(expected));
    else
        return actual == expected;
}

QImage captureImage(QQuickItem *item)
{
    const auto capture = item->grabToImage();
    CTEST_CHECK(capture);
    bool ready = false;
    const auto captureConnection = QObject::connect(capture.data(), &QQuickItemGrabResult::ready,
                                                      capture.data(), [&ready] { ready = true; });
    CTEST_CHECK(captureConnection);
    const auto disconnectCapture = qScopeGuard([&] { QObject::disconnect(captureConnection); });
    CTEST_WAIT_DEFAULT(ready);
    const QImage image = capture->image();
    CTEST_CHECK(!image.isNull());
    return image;
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
               Qt::MouseButton button, Qt::MouseButtons buttons)
{
    static ulong timestamp = 10000;
    timestamp += 16;
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

void keyClick(QQuickWindow *window, Qt::Key key)
{
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}
}

#define SHEET_COMPARE(actual, expected) CTEST_CHECK(sheetEqual((actual), (expected)))
#define SHEET_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(sheetEqual((actual), (expected)))

class SheetTests
{
public:
    void placement_content_and_radius();
    void inline_content_and_bounded_scroll();
    void modal_input_dismissal_and_focus();
    void automatic_presentation_and_component_replacement();
    void animated_mobile_drag();
    void gallery_loads();
};

static QByteArray fixture(const QByteArray &body)
{
    return R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    width: 800; height: 844; visible: true
    property alias sheet: sheet
    property int backgroundClicks: 0
    function setTarget(target) { LV.Theme.targetOverride = target }
    property Component alternateView: Component {
        Rectangle { objectName: "replacementView"; implicitHeight: 80; color: "blue" }
    }
    LV.LabelButton {
        objectName: "trigger"; text: "Open"; x: 10; y: 10
        onClicked: backgroundClicks++
    }
    LV.Sheet {
        id: sheet; objectName: "sheet"; animationDuration: 0
)" + body + R"(
    }
})";
}

void SheetTests::placement_content_and_radius()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        title: "Export image"; description: "Choose the format and size."
        presentation: LV.Sheet.Mobile; detent: LV.Sheet.Fit
        cornerRadius: 55
        bottomSafeInset: 34
        contentComponent: Component {
            Rectangle { objectName: "providedView"; implicitHeight: 133; color: "orange" }
        }
)")));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *sheet = object->findChild<QObject *>("sheet");
    CTEST_CHECK(window && sheet);
    CTEST_WAIT_DEFAULT(window->isExposed());
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "open"));
    CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
    auto *view = sheet->findChild<QQuickItem *>("providedView");
    auto *surface = sheet->findChild<QQuickItem *>("sheet_surface");
    CTEST_CHECK(view && surface);
    SHEET_WAIT_COMPARE(sheet->property("width").toReal(), 800.0);
    SHEET_WAIT_COMPARE(view->width(), 760.0);
    SHEET_WAIT_COMPARE(sheet->property("height").toReal(), 275.0);
    SHEET_COMPARE(sheet->property("y").toReal(), 569.0);
    SHEET_COMPARE(surface->property("radius").toReal(), 55.0);
    const QImage roundedCapture = captureImage(surface);
    CTEST_CHECK(roundedCapture.pixelColor(5, 5).alpha() < 10);

    // The source of the measurement, not an OS-name heuristic, sets the radius.
    sheet->setProperty("cornerRadius", 28.5);
    SHEET_WAIT_COMPARE(surface->property("radius").toReal(), 28.5);
    window->resize(390, 844);
    SHEET_WAIT_COMPARE(sheet->property("width").toReal(), 390.0);
    SHEET_COMPARE(surface->property("radius").toReal(), 28.5);
    sheet->setProperty("cornerRadius", 0);
    SHEET_WAIT_COMPARE(surface->property("radius").toReal(), 0.0);
    const QImage squareCapture = captureImage(surface);
    CTEST_CHECK(squareCapture.pixelColor(5, 5).alpha() > 245);
    sheet->setProperty("cornerRadius", -10);
    SHEET_WAIT_COMPARE(surface->property("radius").toReal(), 0.0);
    sheet->setProperty("cornerRadius", std::numeric_limits<double>::quiet_NaN());
    SHEET_WAIT_COMPARE(surface->property("radius").toReal(), 0.0);
    sheet->setProperty("cornerRadius", 10000);
    SHEET_WAIT_COMPARE(surface->property("radius").toReal(), sheet->property("height").toReal() / 2);

    sheet->setProperty("detent", 1); // Medium
    SHEET_WAIT_COMPARE(sheet->property("height").toReal(), 481.0);
    sheet->setProperty("detent", 2); // Large
    SHEET_WAIT_COMPARE(sheet->property("height").toReal(), 760.0);
    sheet->setProperty("topSafeInset", 140);
    SHEET_WAIT_COMPARE(sheet->property("height").toReal(), 704.0);

    sheet->setProperty("presentation", 2); // Desktop
    sheet->setProperty("cornerRadius", 16);
    window->resize(800, 600);
    SHEET_WAIT_COMPARE(sheet->property("width").toReal(), 560.0);
    SHEET_WAIT_COMPARE(sheet->property("height").toReal(), 420.0);
    SHEET_WAIT_COMPARE(sheet->property("x").toReal(), 120.0);
    SHEET_WAIT_COMPARE(sheet->property("y").toReal(), 90.0);
    SHEET_COMPARE(view->width(), 520.0);
    window->resize(300, 260);
    SHEET_WAIT_COMPARE(sheet->property("width").toReal(), 252.0);
    SHEET_WAIT_COMPARE(sheet->property("height").toReal(), 212.0);
    SHEET_COMPARE(sheet->property("x").toReal(), 24.0);
    SHEET_COMPARE(sheet->property("y").toReal(), 24.0);
}

void SheetTests::inline_content_and_bounded_scroll()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        presentation: LV.Sheet.Mobile; title: "Long content"
        Rectangle { objectName: "inlineView"; width: parent.width; implicitHeight: 1600; color: "orange" }
)")));
    CTEST_CHECK(object);
    auto *sheet = object->findChild<QObject *>("sheet");
    CTEST_CHECK(sheet);
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "open"));
    CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
    auto *view = sheet->findChild<QQuickItem *>("inlineView");
    auto *viewport = sheet->findChild<QQuickItem *>("sheet_viewport");
    auto *header = sheet->findChild<QQuickItem *>("sheet_header");
    CTEST_CHECK(view && viewport && header);
    CTEST_WAIT_DEFAULT(sheet->property("height").toReal() <= 820);
    SHEET_WAIT_COMPARE(viewport->property("contentHeight").toReal(), 1600.0);
    const QPointF headerPosition = header->mapToScene(QPointF());
    viewport->setProperty("contentY", 400);
    SHEET_WAIT_COMPARE(viewport->property("contentY").toReal(), 400.0);
    SHEET_COMPARE(header->mapToScene(QPointF()), headerPosition);
    view->setImplicitHeight(120);
    SHEET_WAIT_COMPARE(viewport->property("contentHeight").toReal(), 120.0);
    SHEET_WAIT_COMPARE(viewport->property("contentY").toReal(), 0.0);
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "close"));
    CTEST_WAIT_DEFAULT(!sheet->property("visible").toBool());
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "open"));
    CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
    SHEET_COMPARE(viewport->property("contentY").toReal(), 0.0);
}

void SheetTests::modal_input_dismissal_and_focus()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        presentation: LV.Sheet.Desktop; title: "Edit project"
        LV.LabelButton { objectName: "insideAction"; text: "Apply" }
)")));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *sheet = object->findChild<QObject *>("sheet");
    auto *trigger = object->findChild<QQuickItem *>("trigger");
    CTEST_CHECK(window && sheet && trigger);
    CTEST_WAIT_DEFAULT(window->isExposed());
    trigger->forceActiveFocus();
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "open"));
    CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
    CTEST_WAIT_DEFAULT(sheet->property("activeFocus").toBool());
    for (int i = 0; i < 5; ++i) {
        keyClick(window, Qt::Key_Tab);
        CTEST_CHECK(!trigger->hasActiveFocus());
    }
    SheetSignalRecorder recorder;
    const auto closedConnection = QObject::connect(sheet, SIGNAL(closed()), &recorder, SLOT(recordClosed()));
    CTEST_CHECK(closedConnection);
    sheet->setProperty("dismissOnBackground", false);
    mouseClick(window, QPoint(20, 20));
    CTEST_CHECK(sheet->property("visible").toBool());
    SHEET_COMPARE(object->property("backgroundClicks").toInt(), 0);
    sheet->setProperty("dismissOnEscape", false);
    keyClick(window, Qt::Key_Escape);
    CTEST_CHECK(sheet->property("visible").toBool());
    sheet->setProperty("dismissOnEscape", true);
    keyClick(window, Qt::Key_Escape);
    SHEET_WAIT_COMPARE(recorder.closedCount, 1);
    CTEST_WAIT_DEFAULT(trigger->hasActiveFocus());
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "open"));
    CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
    sheet->setProperty("dismissOnBackground", true);
    mouseClick(window, QPoint(20, 20));
    SHEET_WAIT_COMPARE(recorder.closedCount, 2);
    SHEET_COMPARE(object->property("backgroundClicks").toInt(), 0);
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "open"));
    CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
    auto *closeButton = sheet->findChild<QQuickItem *>("sheet_close");
    CTEST_CHECK(closeButton);
    SHEET_COMPARE(closeButton->size(), QSizeF(44, 44));
    auto *closeIcon = closeButton->findChild<QQuickItem *>("sheet_closeIcon");
    CTEST_CHECK(closeIcon);
    SHEET_COMPARE(closeIcon->size(), QSizeF(16, 16));
    SHEET_COMPARE(closeIcon->mapToItem(closeButton, QPointF(8, 8)), QPointF(22, 22));
    mouseClick(window, closeButton->mapToScene(QPointF(22, 22)).toPoint());
    SHEET_WAIT_COMPARE(recorder.closedCount, 3);
}

void SheetTests::automatic_presentation_and_component_replacement()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        title: "Reusable view"; cornerRadius: 42.5
        contentComponent: Component { Rectangle { implicitHeight: 240 } }
)")));
    CTEST_CHECK(object);
    auto *sheet = object->findChild<QObject *>("sheet");
    CTEST_CHECK(sheet);
    for (const QString &target : {QString("ios"), QString("android"), QString("macos")}) {
        CTEST_CHECK(QMetaObject::invokeMethod(object.data(), "setTarget", Q_ARG(QVariant, target)));
        SHEET_WAIT_COMPARE(sheet->property("mobilePresentation").toBool(), target != "macos");
        SHEET_COMPARE(sheet->property("cornerRadius").toReal(), 42.5);
    }
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "open"));
    CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
    QObject *original = sheet->property("loadedContent").value<QObject *>();
    CTEST_CHECK(original);
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "close"));
    CTEST_WAIT_DEFAULT(!sheet->property("visible").toBool());
    SHEET_COMPARE(sheet->property("loadedContent").value<QObject *>(), original);
    int destroyedCount = 0;
    QObject destroyedObserver;
    const auto destroyedConnection = QObject::connect(original, &QObject::destroyed,
                                                       &destroyedObserver, [&] { ++destroyedCount; });
    CTEST_CHECK(destroyedConnection);
    CTEST_CHECK(sheet->setProperty("contentComponent", object->property("alternateView")));
    CTEST_WAIT_DEFAULT(sheet->findChild<QQuickItem *>("replacementView"));
    SHEET_WAIT_COMPARE(destroyedCount, 1);
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "open"));
    CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
    CTEST_CHECK(sheet->property("loadedContent").value<QObject *>() != original);
}

void SheetTests::animated_mobile_drag()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        presentation: LV.Sheet.Mobile; title: "Drag to close"
        contentComponent: Component { Rectangle { implicitHeight: 240 } }
)")));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *sheet = object->findChild<QObject *>("sheet");
    CTEST_CHECK(window && sheet);
    window->resize(390, 844);
    CTEST_WAIT_DEFAULT(window->isExposed());
    sheet->setProperty("animationDuration", 120);
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "open"));
    CTEST_CHECK(sheet->property("y").toReal() > sheet->property("_restingY").toReal());
    CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
    SHEET_WAIT_COMPARE(sheet->property("y").toReal() + sheet->property("height").toReal(), 844.0);
    auto *grabber = sheet->findChild<QQuickItem *>("sheet_grabber");
    CTEST_CHECK(grabber);
    QPoint start = grabber->mapToScene(QPointF(195, 14)).toPoint();
    sendMouse(window, QEvent::MouseButtonPress, start, Qt::LeftButton, Qt::LeftButton);
    for (int offset = 20; offset <= 160; offset += 20)
        sendMouse(window, QEvent::MouseMove, start + QPoint(0, offset), Qt::NoButton, Qt::LeftButton);
    sendMouse(window, QEvent::MouseButtonRelease, start + QPoint(0, 160), Qt::LeftButton, Qt::NoButton);
    CTEST_WAIT_DEFAULT(!sheet->property("visible").toBool());
    CTEST_CHECK(QMetaObject::invokeMethod(sheet, "open"));
    CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
    SHEET_WAIT_COMPARE(sheet->property("y").toReal() + sheet->property("height").toReal(), 844.0);
}

void SheetTests::gallery_loads()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(1000, 1200);
    window.setColor(QColor("#141414"));
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../example/VisualCatalog/qml/SheetGallery.qml")));
    CTEST_CHECK(object);
    auto *gallery = qobject_cast<QQuickItem *>(object.data());
    CTEST_CHECK(gallery);
    gallery->setWidth(900);
    gallery->setPosition(QPointF(20, 20));
    gallery->setParentItem(window.contentItem());
    window.show();
    CTEST_WAIT_DEFAULT(window.isExposed());
    auto *sheet = object->findChild<QObject *>("gallery_sheet");
    CTEST_CHECK(sheet);
    sheet->setProperty("animationDuration", 0);
    const QString captureDir = qEnvironmentVariable("LVRS_SHEET_CAPTURE_DIR");
    if (!captureDir.isEmpty())
        CTEST_CHECK(QDir().mkpath(captureDir));
    for (int index = 0; index < 5; ++index) {
        CTEST_CHECK(QMetaObject::invokeMethod(object.data(), "showExample", Q_ARG(QVariant, index)));
        CTEST_WAIT_DEFAULT(sheet->property("opened").toBool());
        SHEET_COMPARE(sheet->property("mobilePresentation").toBool(), index != 3);
        SHEET_COMPARE(sheet->property("resolvedCornerRadius").toReal(), index == 3 ? 16.0 : 55.0);
        if (index == 2) {
            auto *list = sheet->findChild<QQuickItem *>("gallery_sheetFolders");
            auto *save = sheet->findChild<QQuickItem *>("gallery_sheetSave");
            CTEST_CHECK(list && save);
            SHEET_WAIT_COMPARE(save->height(), 44.0);
            CTEST_WAIT_DEFAULT(list->property("contentHeight").toReal() > list->height());
            const QPointF savePosition = save->mapToScene(QPointF());
            list->setProperty("contentY", 100);
            SHEET_WAIT_COMPARE(list->property("contentY").toReal(), 100.0);
            SHEET_COMPARE(save->mapToScene(QPointF()), savePosition);
        }
        if (!captureDir.isEmpty()) {
            window.update();
            processFor(40);
            const QImage screenshot = window.grabWindow();
            CTEST_CHECK(!screenshot.isNull());
            CTEST_CHECK(screenshot.save(QDir(captureDir).filePath(QString("sheet-%1.png").arg(index))));
        }
        CTEST_CHECK(QMetaObject::invokeMethod(sheet, "close"));
        CTEST_WAIT_DEFAULT(!sheet->property("visible").toBool());
    }
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    SheetTests tests;
    runner.run("placement_content_and_radius", [&] { tests.placement_content_and_radius(); });
    runner.run("inline_content_and_bounded_scroll", [&] { tests.inline_content_and_bounded_scroll(); });
    runner.run("modal_input_dismissal_and_focus", [&] { tests.modal_input_dismissal_and_focus(); });
    runner.run("automatic_presentation_and_component_replacement", [&] { tests.automatic_presentation_and_component_replacement(); });
    runner.run("animated_mobile_drag", [&] { tests.animated_mobile_drag(); });
    runner.run("gallery_loads", [&] { tests.gallery_loads(); });
    return runner.result();
}
#include "tst_sheet.moc"
