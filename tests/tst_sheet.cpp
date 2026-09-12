#include <QtTest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QSignalSpy>
#include <QQmlEngine>
#include <limits>
#include "test_utils.h"

class SheetTests : public QObject
{
    Q_OBJECT
private slots:
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
    QVERIFY(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *sheet = object->findChild<QObject *>("sheet");
    QVERIFY(window && sheet);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(QMetaObject::invokeMethod(sheet, "open"));
    QTRY_VERIFY(sheet->property("opened").toBool());
    auto *view = sheet->findChild<QQuickItem *>("providedView");
    auto *surface = sheet->findChild<QQuickItem *>("sheet_surface");
    QVERIFY(view && surface);
    QTRY_COMPARE(sheet->property("width").toReal(), 800.0);
    QTRY_COMPARE(view->width(), 760.0);
    QTRY_COMPARE(sheet->property("height").toReal(), 275.0);
    QCOMPARE(sheet->property("y").toReal(), 569.0);
    QCOMPARE(surface->property("radius").toReal(), 55.0);
    auto roundedCapture = surface->grabToImage();
    QVERIFY(roundedCapture);
    QTRY_VERIFY(!roundedCapture->image().isNull());
    QVERIFY(roundedCapture->image().pixelColor(5, 5).alpha() < 10);

    // The source of the measurement, not an OS-name heuristic, sets the radius.
    sheet->setProperty("cornerRadius", 28.5);
    QTRY_COMPARE(surface->property("radius").toReal(), 28.5);
    window->resize(390, 844);
    QTRY_COMPARE(sheet->property("width").toReal(), 390.0);
    QCOMPARE(surface->property("radius").toReal(), 28.5);
    sheet->setProperty("cornerRadius", 0);
    QTRY_COMPARE(surface->property("radius").toReal(), 0.0);
    auto squareCapture = surface->grabToImage();
    QVERIFY(squareCapture);
    QTRY_VERIFY(!squareCapture->image().isNull());
    QVERIFY(squareCapture->image().pixelColor(5, 5).alpha() > 245);
    sheet->setProperty("cornerRadius", -10);
    QTRY_COMPARE(surface->property("radius").toReal(), 0.0);
    sheet->setProperty("cornerRadius", std::numeric_limits<double>::quiet_NaN());
    QTRY_COMPARE(surface->property("radius").toReal(), 0.0);
    sheet->setProperty("cornerRadius", 10000);
    QTRY_COMPARE(surface->property("radius").toReal(), sheet->property("height").toReal() / 2);

    sheet->setProperty("detent", 1); // Medium
    QTRY_COMPARE(sheet->property("height").toReal(), 481.0);
    sheet->setProperty("detent", 2); // Large
    QTRY_COMPARE(sheet->property("height").toReal(), 760.0);
    sheet->setProperty("topSafeInset", 140);
    QTRY_COMPARE(sheet->property("height").toReal(), 704.0);

    sheet->setProperty("presentation", 2); // Desktop
    sheet->setProperty("cornerRadius", 16);
    window->resize(800, 600);
    QTRY_COMPARE(sheet->property("width").toReal(), 560.0);
    QTRY_COMPARE(sheet->property("height").toReal(), 420.0);
    QTRY_COMPARE(sheet->property("x").toReal(), 120.0);
    QTRY_COMPARE(sheet->property("y").toReal(), 90.0);
    QCOMPARE(view->width(), 520.0);
    window->resize(300, 260);
    QTRY_COMPARE(sheet->property("width").toReal(), 252.0);
    QTRY_COMPARE(sheet->property("height").toReal(), 212.0);
    QCOMPARE(sheet->property("x").toReal(), 24.0);
    QCOMPARE(sheet->property("y").toReal(), 24.0);
}

void SheetTests::inline_content_and_bounded_scroll()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        presentation: LV.Sheet.Mobile; title: "Long content"
        Rectangle { objectName: "inlineView"; width: parent.width; implicitHeight: 1600; color: "orange" }
)")));
    QVERIFY(object);
    auto *sheet = object->findChild<QObject *>("sheet");
    QVERIFY(sheet);
    QVERIFY(QMetaObject::invokeMethod(sheet, "open"));
    QTRY_VERIFY(sheet->property("opened").toBool());
    auto *view = sheet->findChild<QQuickItem *>("inlineView");
    auto *viewport = sheet->findChild<QQuickItem *>("sheet_viewport");
    auto *header = sheet->findChild<QQuickItem *>("sheet_header");
    QVERIFY(view && viewport && header);
    QTRY_VERIFY(sheet->property("height").toReal() <= 820);
    QTRY_COMPARE(viewport->property("contentHeight").toReal(), 1600.0);
    const QPointF headerPosition = header->mapToScene(QPointF());
    viewport->setProperty("contentY", 400);
    QTRY_COMPARE(viewport->property("contentY").toReal(), 400.0);
    QCOMPARE(header->mapToScene(QPointF()), headerPosition);
    view->setImplicitHeight(120);
    QTRY_COMPARE(viewport->property("contentHeight").toReal(), 120.0);
    QTRY_COMPARE(viewport->property("contentY").toReal(), 0.0);
    QVERIFY(QMetaObject::invokeMethod(sheet, "close"));
    QTRY_VERIFY(!sheet->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(sheet, "open"));
    QTRY_VERIFY(sheet->property("opened").toBool());
    QCOMPARE(viewport->property("contentY").toReal(), 0.0);
}

void SheetTests::modal_input_dismissal_and_focus()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        presentation: LV.Sheet.Desktop; title: "Edit project"
        LV.LabelButton { objectName: "insideAction"; text: "Apply" }
)")));
    QVERIFY(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *sheet = object->findChild<QObject *>("sheet");
    auto *trigger = object->findChild<QQuickItem *>("trigger");
    QVERIFY(window && sheet && trigger);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    trigger->forceActiveFocus();
    QVERIFY(QMetaObject::invokeMethod(sheet, "open"));
    QTRY_VERIFY(sheet->property("opened").toBool());
    QTRY_VERIFY(sheet->property("activeFocus").toBool());
    for (int i = 0; i < 5; ++i) {
        QTest::keyClick(window, Qt::Key_Tab);
        QVERIFY(!trigger->hasActiveFocus());
    }
    QSignalSpy closed(sheet, SIGNAL(closed()));
    sheet->setProperty("dismissOnBackground", false);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QVERIFY(sheet->property("visible").toBool());
    QCOMPARE(object->property("backgroundClicks").toInt(), 0);
    sheet->setProperty("dismissOnEscape", false);
    QTest::keyClick(window, Qt::Key_Escape);
    QVERIFY(sheet->property("visible").toBool());
    sheet->setProperty("dismissOnEscape", true);
    QTest::keyClick(window, Qt::Key_Escape);
    QTRY_COMPARE(closed.count(), 1);
    QTRY_VERIFY(trigger->hasActiveFocus());
    QVERIFY(QMetaObject::invokeMethod(sheet, "open"));
    QTRY_VERIFY(sheet->property("opened").toBool());
    sheet->setProperty("dismissOnBackground", true);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
    QTRY_COMPARE(closed.count(), 2);
    QCOMPARE(object->property("backgroundClicks").toInt(), 0);
    QVERIFY(QMetaObject::invokeMethod(sheet, "open"));
    QTRY_VERIFY(sheet->property("opened").toBool());
    auto *closeButton = sheet->findChild<QQuickItem *>("sheet_close");
    QVERIFY(closeButton);
    QCOMPARE(closeButton->size(), QSizeF(44, 44));
    auto *closeIcon = closeButton->findChild<QQuickItem *>("sheet_closeIcon");
    QVERIFY(closeIcon);
    QCOMPARE(closeIcon->size(), QSizeF(16, 16));
    QCOMPARE(closeIcon->mapToItem(closeButton, QPointF(8, 8)), QPointF(22, 22));
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                     closeButton->mapToScene(QPointF(22, 22)).toPoint());
    QTRY_COMPARE(closed.count(), 3);
}

void SheetTests::automatic_presentation_and_component_replacement()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        title: "Reusable view"; cornerRadius: 42.5
        contentComponent: Component { Rectangle { implicitHeight: 240 } }
)")));
    QVERIFY(object);
    auto *sheet = object->findChild<QObject *>("sheet");
    QVERIFY(sheet);
    for (const QString &target : {QString("ios"), QString("android"), QString("macos")}) {
        QVERIFY(QMetaObject::invokeMethod(object.data(), "setTarget", Q_ARG(QVariant, target)));
        QTRY_COMPARE(sheet->property("mobilePresentation").toBool(), target != "macos");
        QCOMPARE(sheet->property("cornerRadius").toReal(), 42.5);
    }
    QVERIFY(QMetaObject::invokeMethod(sheet, "open"));
    QTRY_VERIFY(sheet->property("opened").toBool());
    QObject *original = sheet->property("loadedContent").value<QObject *>();
    QVERIFY(original);
    QVERIFY(QMetaObject::invokeMethod(sheet, "close"));
    QTRY_VERIFY(!sheet->property("visible").toBool());
    QCOMPARE(sheet->property("loadedContent").value<QObject *>(), original);
    QSignalSpy destroyed(original, &QObject::destroyed);
    QVERIFY(sheet->setProperty("contentComponent", object->property("alternateView")));
    QTRY_VERIFY(sheet->findChild<QQuickItem *>("replacementView"));
    QTRY_COMPARE(destroyed.count(), 1);
    QVERIFY(QMetaObject::invokeMethod(sheet, "open"));
    QTRY_VERIFY(sheet->property("opened").toBool());
    QVERIFY(sheet->property("loadedContent").value<QObject *>() != original);
}

void SheetTests::animated_mobile_drag()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, fixture(R"(
        presentation: LV.Sheet.Mobile; title: "Drag to close"
        contentComponent: Component { Rectangle { implicitHeight: 240 } }
)")));
    QVERIFY(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    auto *sheet = object->findChild<QObject *>("sheet");
    QVERIFY(window && sheet);
    window->resize(390, 844);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    sheet->setProperty("animationDuration", 120);
    QVERIFY(QMetaObject::invokeMethod(sheet, "open"));
    QVERIFY(sheet->property("y").toReal() > sheet->property("_restingY").toReal());
    QTRY_VERIFY(sheet->property("opened").toBool());
    QTRY_COMPARE(sheet->property("y").toReal() + sheet->property("height").toReal(), 844.0);
    auto *grabber = sheet->findChild<QQuickItem *>("sheet_grabber");
    QVERIFY(grabber);
    QPoint start = grabber->mapToScene(QPointF(195, 14)).toPoint();
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, start);
    for (int offset = 20; offset <= 160; offset += 20)
        QTest::mouseMove(window, start + QPoint(0, offset), 16);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, start + QPoint(0, 160));
    QTRY_VERIFY(!sheet->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(sheet, "open"));
    QTRY_VERIFY(sheet->property("opened").toBool());
    QTRY_COMPARE(sheet->property("y").toReal() + sheet->property("height").toReal(), 844.0);
}

void SheetTests::gallery_loads()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(1000, 1200);
    window.setColor(QColor("#141414"));
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
        QFINDTESTDATA("../example/VisualCatalog/qml/SheetGallery.qml")));
    QVERIFY(object);
    auto *gallery = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(gallery);
    gallery->setWidth(900);
    gallery->setPosition(QPointF(20, 20));
    gallery->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *sheet = object->findChild<QObject *>("gallery_sheet");
    QVERIFY(sheet);
    sheet->setProperty("animationDuration", 0);
    const QString captureDir = qEnvironmentVariable("LVRS_SHEET_CAPTURE_DIR");
    if (!captureDir.isEmpty())
        QVERIFY(QDir().mkpath(captureDir));
    for (int index = 0; index < 5; ++index) {
        QVERIFY(QMetaObject::invokeMethod(object.data(), "showExample", Q_ARG(QVariant, index)));
        QTRY_VERIFY(sheet->property("opened").toBool());
        QCOMPARE(sheet->property("mobilePresentation").toBool(), index != 3);
        QCOMPARE(sheet->property("resolvedCornerRadius").toReal(), index == 3 ? 16.0 : 55.0);
        if (index == 2) {
            auto *list = sheet->findChild<QQuickItem *>("gallery_sheetFolders");
            auto *save = sheet->findChild<QQuickItem *>("gallery_sheetSave");
            QVERIFY(list && save);
            QTRY_COMPARE(save->height(), 44.0);
            QTRY_VERIFY(list->property("contentHeight").toReal() > list->height());
            const QPointF savePosition = save->mapToScene(QPointF());
            list->setProperty("contentY", 100);
            QTRY_COMPARE(list->property("contentY").toReal(), 100.0);
            QCOMPARE(save->mapToScene(QPointF()), savePosition);
        }
        if (!captureDir.isEmpty()) {
            window.update();
            QTest::qWait(40);
            const QImage screenshot = window.grabWindow();
            QVERIFY(!screenshot.isNull());
            QVERIFY(screenshot.save(QDir(captureDir).filePath(QString("sheet-%1.png").arg(index))));
        }
        QVERIFY(QMetaObject::invokeMethod(sheet, "close"));
        QTRY_VERIFY(!sheet->property("visible").toBool());
    }
}

QTEST_MAIN(SheetTests)
#include "tst_sheet.moc"
