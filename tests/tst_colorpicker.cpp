#include <QtTest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QQmlEngine>
#include "test_utils.h"
#include "backend/graphics/colorpickermodel.h"
#include "backend/graphics/colorpickersurface.h"
#include <limits>

namespace {
QQuickItem *findVisual(QQuickItem *item, const QString &name)
{
    if (item->objectName() == name)
        return item;
    for (auto *child : item->childItems())
        if (auto *found = findVisual(child, name))
            return found;
    return nullptr;
}
}

class ColorPickerTests : public QObject
{
    Q_OBJECT
private slots:
    void content_variants_data();
    void content_variants();
    void color_conversion_and_validation();
    void achromatic_hue_and_cmyk_channels();
    void external_binding_and_accept_cancel();
    void component_hosts_and_modal_lifecycle();
    void pointer_keyboard_touch_and_fields();
    void all_color_domains();
    void catalog_renders_six_views();
};

void ColorPickerTests::content_variants_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("expectedHeight");
    const int heights[] = {461, 437, 437, 361, 413, 439};
    for (int type = 0; type < 6; ++type)
        QTest::newRow(qPrintable(QString::number(type))) << type << heights[type];
}

void ColorPickerTests::content_variants()
{
    QQuickWindow window;
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QFETCH(int, type);
    QFETCH(int, expectedHeight);
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, QString(R"(
import QtQuick
import LVRS as LV
LV.ColorPicker { type: %1; currentColor: "#7a5af8"; previousColor: "#0a84ff" }
)").arg(type).toUtf8()));
    QVERIFY(object);
    auto *picker = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(picker);
    picker->setParentItem(window.contentItem());
    window.resize(500, 600);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QCOMPARE(picker->implicitWidth(), 320.0);
    QTRY_COMPARE(picker->implicitHeight(), qreal(expectedHeight));
    QCOMPARE(picker->parentItem(), window.contentItem());
    auto *hueInput = picker->findChild<QQuickItem *>("colorPicker_hex");
    QVERIFY(hueInput);
    QCOMPARE(hueInput->height(), 22.0);
    QVERIFY(!picker->property("open").isValid());
    QVERIFY(!picker->property("useOverlayLayer").isValid());
    const auto fields = picker->findChildren<ColorPickerSurface *>();
    QVERIFY(fields.size() >= 2);
    auto grab = picker->grabToImage();
    QVERIFY(grab);
    QTRY_VERIFY(!grab->image().isNull());
    // The view has no opaque surface, title bar or overlay at its outer padding.
    QCOMPARE(grab->image().pixelColor(0, 0).alpha(), 0);
    const QString captureDir = qEnvironmentVariable("LVRS_COLORPICKER_CAPTURE_DIR");
    if (!captureDir.isEmpty()) {
        QVERIFY(QDir().mkpath(captureDir));
        QVERIFY(grab->saveToFile(captureDir + QString("/colorpicker-%1.png").arg(type)));
    }
    picker->setProperty("showRecentColors", false);
    QTRY_COMPARE(picker->implicitHeight(), qreal(expectedHeight - 32));
    picker->setProperty("showActions", false);
    QTRY_COMPARE(picker->implicitHeight(), qreal(expectedHeight - 66));
    picker->setWidth(400);
    QTRY_COMPARE(picker->findChild<QQuickItem *>("colorPicker_values")->width(), 376.0);
}

void ColorPickerTests::color_conversion_and_validation()
{
    ColorPickerModel model;
    QSignalSpy edits(&model, &ColorPickerModel::colorEdited);
    model.setColor(QColor("#ff0000"));
    QCOMPARE(edits.size(), 0);
    QCOMPARE(model.hex(), QString("FF0000"));
    QCOMPARE(model.channelValue(ColorPickerModel::Red), 255.0);
    model.editChannel(ColorPickerModel::Alpha, 50);
    QVERIFY(qAbs(model.color().alphaF() - .5) < .0001);
    QVERIFY(model.editHex("#00ff00"));
    QVERIFY(qAbs(model.color().alphaF() - .5) < .0001);
    QVERIFY(model.editHex("10203080"));
    QCOMPARE(model.color(), QColor(16, 32, 48, 128));
    const QColor before = model.color();
    for (const QString &invalid : {QString("#xyz123"), QString("123"), QString("#123456789"), QString("")})
        QVERIFY(!model.editHex(invalid));
    model.editChannel(ColorPickerModel::Hue, std::numeric_limits<double>::quiet_NaN());
    model.editHsv(20, std::numeric_limits<double>::infinity(), 1);
    model.editColor(QColor());
    QCOMPARE(model.color(), before);
    model.editChannel(ColorPickerModel::Red, 900);
    QCOMPARE(model.color().red(), 255);
    model.editChannel(ColorPickerModel::Alpha, -5);
    QCOMPARE(model.color().alpha(), 0);
    model.editChannel(ColorPickerModel::Gray, 50);
    QCOMPARE(model.color().red(), model.color().green());
    QCOMPARE(model.color().green(), model.color().blue());
    QCOMPARE(model.color().alpha(), 0);
    QVERIFY(edits.size() >= 5);
}

void ColorPickerTests::achromatic_hue_and_cmyk_channels()
{
    ColorPickerModel model;
    model.editHsv(210, 0, 1);
    QCOMPARE(model.hue(), 210.0);
    model.editChannel(ColorPickerModel::Saturation, 100);
    QVERIFY(qAbs(model.color().hsvHueF() * 360 - 210) < .01);
    model.editChannel(ColorPickerModel::Brightness, 0);
    QCOMPARE(model.saturation(), 1.0);
    model.editChannel(ColorPickerModel::Brightness, 100);
    QVERIFY(qAbs(model.color().hsvHueF() * 360 - 210) < .01);
    model.editChannel(ColorPickerModel::Black, 35);
    model.editChannel(ColorPickerModel::Cyan, 20);
    QCOMPARE(model.channelValue(ColorPickerModel::Black), 35.0);
    QCOMPARE(model.channelValue(ColorPickerModel::Cyan), 20.0);
    model.editChannel(ColorPickerModel::Alpha, 60);
    QCOMPARE(model.channelValue(ColorPickerModel::Black), 35.0);
    const QColor expected = QColor::fromCmykF(.2, model.channelValue(ColorPickerModel::Magenta) / 100,
        model.channelValue(ColorPickerModel::Yellow) / 100, .35, .6).toRgb();
    QCOMPARE(model.color(), expected);
    model.editHex("000000");
    QCOMPARE(model.channelValue(ColorPickerModel::Black), 100.0);
    model.editChannel(ColorPickerModel::Hue, 360);
    QVERIFY(qIsFinite(model.hue()));
}

void ColorPickerTests::external_binding_and_accept_cancel()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
Item {
    id: host
    property color documentColor: "#7a5af8"
    LV.ColorPicker {
        objectName: "picker"
        currentColor: host.documentColor
        previousColor: "#0a84ff"
        onColorEdited: color => host.documentColor = color
    }
})"));
    QVERIFY(object);
    auto *picker = object->findChild<QQuickItem *>("picker");
    auto *state = picker->findChild<ColorPickerModel *>();
    QVERIFY(state);
    QSignalSpy edits(picker, SIGNAL(colorEdited(QColor)));
    QSignalSpy accepted(picker, SIGNAL(accepted(QColor)));
    QSignalSpy canceled(picker, SIGNAL(canceled()));
    state->editHex("ff1020");
    QCOMPARE(object->property("documentColor").value<QColor>(), QColor("#ff1020"));
    QCOMPARE(edits.size(), 1);
    object->setProperty("documentColor", QColor("#00ccff"));
    QCOMPARE(picker->property("currentColor").value<QColor>(), QColor("#00ccff"));
    QCOMPARE(edits.size(), 1);
    for (int type = 0; type < 6; ++type) {
        picker->setProperty("type", type);
        QCOMPARE(state->color(), QColor("#00ccff"));
    }
    QVERIFY(QMetaObject::invokeMethod(picker, "cancel"));
    QCOMPARE(state->color(), QColor("#0a84ff"));
    QCOMPARE(object->property("documentColor").value<QColor>(), QColor("#0a84ff"));
    QCOMPARE(canceled.size(), 1);
    state->editHex("123456");
    QVERIFY(QMetaObject::invokeMethod(picker, "accept"));
    QCOMPARE(accepted.size(), 1);
    state->editHex("abcdef");
    QVERIFY(QMetaObject::invokeMethod(picker, "cancel"));
    QCOMPARE(state->color(), QColor("#123456"));
    object->setProperty("documentColor", QColor("#345678"));
    QCOMPARE(state->color(), QColor("#345678"));
}

void ColorPickerTests::component_hosts_and_modal_lifecycle()
{
    QQuickWindow window;
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
Item {
    width: 1000; height: 720
    Component { id: view; LV.ColorPicker { currentColor: "#aabbcc" } }
    Loader { objectName: "firstHost"; sourceComponent: view; width: 320 }
    Loader { objectName: "secondHost"; x: 360; sourceComponent: view; width: 340 }
    LV.Modal {
        objectName: "modal"; useOverlayLayer: false
        minWidth: 360; maxWidth: 360; frameMinHeight: 0
        showIcon: false; primaryText: ""; contentComponent: view
    }
})"));
    QVERIFY(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    root->setParentItem(window.contentItem());
    window.resize(1000, 720);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *first = object->findChild<QQuickItem *>("firstHost");
    auto *second = object->findChild<QQuickItem *>("secondHost");
    auto *firstView = qvariant_cast<QQuickItem *>(first->property("item"));
    auto *secondView = qvariant_cast<QQuickItem *>(second->property("item"));
    QVERIFY(firstView && secondView && firstView != secondView);
    QCOMPARE(firstView->parentItem(), first);
    QCOMPARE(secondView->parentItem(), second);
    QCOMPARE(firstView->width(), 320.0);
    QCOMPARE(secondView->width(), 340.0);
    firstView->findChild<ColorPickerModel *>()->editHex("ff0000");
    QCOMPARE(secondView->property("currentColor").value<QColor>(), QColor("#aabbcc"));
    auto *modal = object->findChild<QQuickItem *>("modal");
    QVERIFY(modal);
    QVERIFY(!qvariant_cast<QQuickItem *>(modal->property("contentItem")));
    modal->setProperty("open", true);
    QTRY_VERIFY(qvariant_cast<QQuickItem *>(modal->property("contentItem")));
    QPointer<QQuickItem> modalView = qvariant_cast<QQuickItem *>(modal->property("contentItem"));
    QTRY_COMPARE(modalView->width(), 320.0);
    QCOMPARE(modalView->parentItem(), modal->findChild<QQuickItem *>("modalContentLoader"));
    modalView->findChild<ColorPickerModel *>()->editHex("00ff00");
    modal->setProperty("open", false);
    QTRY_VERIFY(modalView.isNull());
    modal->setProperty("open", true);
    QTRY_VERIFY(qvariant_cast<QQuickItem *>(modal->property("contentItem")));
    QCOMPARE(qvariant_cast<QQuickItem *>(modal->property("contentItem"))->property("currentColor").value<QColor>(), QColor("#aabbcc"));
}

void ColorPickerTests::pointer_keyboard_touch_and_fields()
{
    QQuickWindow window;
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ColorPicker { type: LV.ColorPicker.SaturationBrightness; currentColor: "#ff0000" }
)"));
    QVERIFY(object);
    auto *picker = qobject_cast<QQuickItem *>(object.data());
    picker->setParentItem(window.contentItem());
    window.resize(400, 540);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *state = picker->findChild<ColorPickerModel *>();
    auto *field = picker->findChild<ColorPickerSurface *>("colorPicker_field");
    QVERIFY(state && field);
    const auto at = [field](double x, double y) { return field->mapToScene(QPointF(x * field->width(), y * field->height())).toPoint(); };
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, at(.5, .25));
    QVERIFY(qAbs(state->saturation() - .5) < .01);
    QVERIFY(qAbs(state->brightness() - .75) < .01);
    field->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyClick(&window, Qt::Key_Right);
    QVERIFY(qAbs(state->saturation() - .51) < .01);
    auto *device = QTest::createTouchDevice();
    QTest::touchEvent(&window, device).press(0, at(.2, .2), &window);
    QTest::touchEvent(&window, device).move(0, at(.8, .5), &window);
    QTest::touchEvent(&window, device).release(0, at(.8, .5), &window);
    QTRY_VERIFY(qAbs(state->saturation() - .8) < .02);
    QTRY_VERIFY(qAbs(state->brightness() - .5) < .02);
    auto *hex = picker->findChild<QQuickItem *>("colorPicker_hex");
    auto *input = qvariant_cast<QQuickItem *>(hex->property("inputItem"));
    QVERIFY(input);
    input->forceActiveFocus(Qt::TabFocusReason);
    input->setProperty("text", "10203080");
    QVERIFY(QMetaObject::invokeMethod(input, "textEdited"));
    QTest::keyClick(&window, Qt::Key_Return);
    QCOMPARE(state->color(), QColor(16, 32, 48, 128));
    input->setProperty("text", "invalid");
    QVERIFY(QMetaObject::invokeMethod(input, "textEdited"));
    QTest::keyClick(&window, Qt::Key_Return);
    QCOMPARE(state->hex(), QString("102030"));
    QCOMPARE(input->property("text").toString(), QString("102030"));
    auto *alpha = picker->findChild<QQuickItem *>("colorPicker_alpha");
    auto *alphaInput = qvariant_cast<QQuickItem *>(alpha->property("inputItem"));
    alphaInput->forceActiveFocus(Qt::TabFocusReason);
    const QColor beforeFocusCycle = state->color();
    field->forceActiveFocus(Qt::TabFocusReason);
    QCOMPARE(state->color(), beforeFocusCycle);
    alphaInput->forceActiveFocus(Qt::TabFocusReason);
    alphaInput->setProperty("text", "25");
    QVERIFY(QMetaObject::invokeMethod(alphaInput, "textEdited"));
    field->forceActiveFocus(Qt::TabFocusReason);
    QVERIFY(qAbs(state->color().alphaF() - .25) < .001);
    QSignalSpy eyedropper(picker, SIGNAL(eyedropperRequested()));
    auto *button = picker->findChild<QQuickItem *>("colorPicker_eyedropper");
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, button->mapToScene(QPointF(11, 11)).toPoint());
    QCOMPARE(eyedropper.size(), 1);
    auto *recent = findVisual(picker, QStringLiteral("colorPicker_recent_1"));
    QVERIFY(recent);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, recent->mapToScene(QPointF(12, 10)).toPoint());
    QCOMPARE(state->color(), QColor("#0a84ff"));
    picker->setEnabled(false);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, at(.1, .1));
    QCOMPARE(state->color(), QColor("#0a84ff"));
}

void ColorPickerTests::catalog_renders_six_views()
{
    QQuickWindow window;
    window.setColor(QColor("#161819"));
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../example/VisualCatalog/qml/ColorPickerGallery.qml")));
    QVERIFY(object);
    auto *gallery = qobject_cast<QQuickItem *>(object.data());
    gallery->setWidth(1008);
    gallery->setX(16); gallery->setY(16);
    gallery->setParentItem(window.contentItem());
    QCoreApplication::processEvents();
    window.resize(1040, qCeil(gallery->implicitHeight()) + 32);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    for (int i = 0; i < 6; ++i) {
        auto *picker = findVisual(gallery, QString("gallery_colorPicker_%1").arg(i));
        QVERIFY(picker);
        QCOMPARE(picker->property("type").toInt(), i);
        QCOMPARE(picker->property("currentColor").value<QColor>(), QColor("#7a5af8"));
    }
    auto grab = gallery->grabToImage();
    QVERIFY(grab);
    QTRY_VERIFY(!grab->image().isNull());
    const QString captureDir = qEnvironmentVariable("LVRS_COLORPICKER_CAPTURE_DIR");
    if (!captureDir.isEmpty()) {
        QVERIFY(QDir().mkpath(captureDir));
        QVERIFY(grab->saveToFile(captureDir + "/gallery.png"));
    }
    auto *modal = gallery->findChild<QQuickItem *>("gallery_colorModal");
    QVERIFY(modal);
    gallery->setProperty("modalStartColor", QColor("#7a5af8"));
    modal->setProperty("open", true);
    QTRY_VERIFY(qvariant_cast<QQuickItem *>(modal->property("contentItem")));
    auto *modalPicker = qvariant_cast<QQuickItem *>(modal->property("contentItem"));
    modalPicker->findChild<ColorPickerModel *>()->editHex("ff0000");
    QCOMPARE(gallery->property("selectedColor").value<QColor>(), QColor("#ff0000"));
    QVERIFY(QMetaObject::invokeMethod(modal, "cancel"));
    QCOMPARE(gallery->property("selectedColor").value<QColor>(), QColor("#7a5af8"));
}

void ColorPickerTests::all_color_domains()
{
    QQuickWindow window;
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, "import LVRS as LV\nLV.ColorPicker {}"));
    QVERIFY(object);
    auto *picker = qobject_cast<QQuickItem *>(object.data());
    picker->setParentItem(window.contentItem());
    window.resize(400, 540);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *state = picker->findChild<ColorPickerModel *>();
    auto *field = picker->findChild<ColorPickerSurface *>("colorPicker_field");
    QVERIFY(state && field);
    for (int type = 0; type < 6; ++type) {
        picker->setProperty("type", type);
        state->editHsv(210, .4, .8);
        QCoreApplication::processEvents();
        const QPointF point = type == 0 ? QPointF(232, 116) : QPointF(field->width() * .5, field->height() * .25);
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, field->mapToScene(point).toPoint());
        QVERIFY(qAbs(state->hue() - (type == 0 || type == 2 ? 210 : 180)) < .01);
        QVERIFY(qAbs(state->saturation() - (type == 0 ? 1 : type == 1 ? .75 : type == 2 ? .5 : .25 / .45)) < .01);
        QVERIFY(qAbs(state->brightness() - (type == 1 ? .8 : type == 2 ? .75 : 1)) < .01);
    }
    picker->setProperty("type", 0);
    QCoreApplication::processEvents();
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, field->mapToScene(QPointF(148, 216)).toPoint());
    QVERIFY(qAbs(state->hue()) < .01);
    QImage ring(QStringLiteral(":/qt/qml/LVRS/resources/images/colorpicker/hue-ring.png"));
    QVERIFY(!ring.isNull());
    const QColor bottom = ring.pixelColor(224, 425);
    QVERIFY(bottom.red() > 240 && bottom.green() < 20 && bottom.blue() < 20);
}

QTEST_MAIN(ColorPickerTests)
#include "tst_colorpicker.moc"
