#include "QtEventWait.h"
#include "QtGuiTouch.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScopeGuard>
#include <type_traits>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QQmlEngine>
#include "test_utils.h"
#include "accent_fixture.h"
#include "backend/graphics/colorpickermodel.h"
#include "backend/graphics/colorpickersurface.h"
#include <limits>

class ColorPickerSignalRecorder : public QObject
{
    Q_OBJECT
public:
    QList<QColor> editedColors;
    QList<QColor> acceptedColors;
    int canceled = 0;
    int eyedropper = 0;
public slots:
    void recordEdited(const QColor &color) { editedColors.append(color); }
    void recordAccepted(const QColor &color) { acceptedColors.append(color); }
    void recordCanceled() { ++canceled; }
    void recordEyedropper() { ++eyedropper; }
};

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

template <typename Actual, typename Expected>
bool pickerEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual) + 1.0, qreal(expected) + 1.0);
    else
        return actual == expected;
}

void mouseClick(QQuickWindow *window, const QPoint &point)
{
    static ulong timestamp = 10000;
    const QPointF local(point), global(window->mapToGlobal(point));
    QMouseEvent press(QEvent::MouseButtonPress, local, local, global, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    press.setTimestamp(timestamp += 16);
    QCoreApplication::sendEvent(window, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, local, local, global, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    release.setTimestamp(timestamp += 16);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}

void keyClick(QQuickWindow *window, Qt::Key key)
{
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}

void waitForGrab(const QSharedPointer<QQuickItemGrabResult> &grab)
{
    CTEST_CHECK(grab);
    int readyCount = 0;
    const auto captureConnection = QObject::connect(grab.data(), &QQuickItemGrabResult::ready,
                                                     [&] { ++readyCount; });
    CTEST_CHECK(captureConnection);
    const auto disconnectCapture = qScopeGuard([&] { QObject::disconnect(captureConnection); });
    CTEST_WAIT_DEFAULT(readyCount > 0);
    CTEST_CHECK(!grab->image().isNull());
}
}
#define PICKER_COMPARE(actual, expected) CTEST_CHECK(pickerEqual((actual), (expected)))
#define PICKER_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(pickerEqual((actual), (expected)))

class ColorPickerTests
{
public:
    void authored_palette_defaults();
    void content_variants(int type, int expectedHeight);
    void color_conversion_and_validation();
    void achromatic_hue_and_cmyk_channels();
    void external_binding_and_accept_cancel();
    void component_hosts_and_modal_lifecycle();
    void pointer_keyboard_touch_and_fields();
    void all_color_domains();
    void catalog_renders_six_views();
};

void ColorPickerTests::authored_palette_defaults()
{
    ColorPickerModel model;
    PICKER_COMPARE(model.color(), QColor(Qt::white));
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
Item {
    LV.ColorPicker { objectName: "picker" }
    LV.ColorPickerButton { objectName: "button" }
})"));
    CTEST_CHECK(root);
    auto *picker = root->findChild<QObject *>("picker");
    auto *button = root->findChild<QObject *>("button");
    CTEST_CHECK(picker && button);
    PICKER_COMPARE(picker->property("currentColor").value<QColor>(), AccentFixture::color("accentPurple"));
    PICKER_COMPARE(button->property("currentColor").value<QColor>(), AccentFixture::color("accentPurple"));
    const auto recent = qvariant_cast<QJSValue>(picker->property("recentColors"));
    PICKER_COMPARE(recent.property("length").toInt(), 10);
    const auto colors = AccentFixture::colors();
    for (int i = 0; i < colors.size(); ++i)
        PICKER_COMPARE(recent.property(i).toVariant().value<QColor>(),
                 AccentFixture::color(colors[i].toObject()["property"].toString()));
    auto *last = findVisual(qobject_cast<QQuickItem *>(picker), "colorPicker_recent_9");
    CTEST_CHECK(last);
    PICKER_COMPARE(last->property("modelData").value<QColor>(), AccentFixture::color("accentTeal"));
}

void ColorPickerTests::content_variants(int type, int expectedHeight)
{
    QQuickWindow window;
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, QString(R"(
import QtQuick
import LVRS as LV
LV.ColorPicker { type: %1; currentColor: "#7a5af8"; previousColor: "#0a84ff" }
)").arg(type).toUtf8()));
    CTEST_CHECK(object);
    auto *picker = qobject_cast<QQuickItem *>(object.data());
    CTEST_CHECK(picker);
    picker->setParentItem(window.contentItem());
    window.resize(500, 600);
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    PICKER_COMPARE(picker->implicitWidth(), 320.0);
    PICKER_WAIT_COMPARE(picker->implicitHeight(), qreal(expectedHeight));
    PICKER_COMPARE(picker->parentItem(), window.contentItem());
    auto *hueInput = picker->findChild<QQuickItem *>("colorPicker_hex");
    CTEST_CHECK(hueInput);
    PICKER_COMPARE(hueInput->height(), 22.0);
    CTEST_CHECK(!picker->property("open").isValid());
    CTEST_CHECK(!picker->property("useOverlayLayer").isValid());
    const auto fields = picker->findChildren<ColorPickerSurface *>();
    CTEST_CHECK(fields.size() >= 2);
    auto grab = picker->grabToImage();
    waitForGrab(grab);
    // The view has no opaque surface, title bar or overlay at its outer padding.
    PICKER_COMPARE(grab->image().pixelColor(0, 0).alpha(), 0);
    const QString captureDir = qEnvironmentVariable("LVRS_COLORPICKER_CAPTURE_DIR");
    if (!captureDir.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(captureDir));
        CTEST_CHECK(grab->saveToFile(captureDir + QString("/colorpicker-%1.png").arg(type)));
    }
    picker->setProperty("showRecentColors", false);
    PICKER_WAIT_COMPARE(picker->implicitHeight(), qreal(expectedHeight - 32));
    picker->setProperty("showActions", false);
    PICKER_WAIT_COMPARE(picker->implicitHeight(), qreal(expectedHeight - 66));
    picker->setWidth(400);
    PICKER_WAIT_COMPARE(picker->findChild<QQuickItem *>("colorPicker_values")->width(), 376.0);
}

void ColorPickerTests::color_conversion_and_validation()
{
    ColorPickerModel model;
    QList<QColor> edits;
    const auto editsConnection = QObject::connect(&model, &ColorPickerModel::colorEdited,
                                                   [&edits](const QColor &color) { edits.append(color); });
    CTEST_CHECK(editsConnection);
    const auto disconnectEdits = qScopeGuard([&] { QObject::disconnect(editsConnection); });
    model.setColor(QColor("#ff0000"));
    PICKER_COMPARE(edits.size(), 0);
    PICKER_COMPARE(model.hex(), QString("FF0000"));
    PICKER_COMPARE(model.channelValue(ColorPickerModel::Red), 255.0);
    model.editChannel(ColorPickerModel::Alpha, 50);
    CTEST_CHECK(qAbs(model.color().alphaF() - .5) < .0001);
    CTEST_CHECK(model.editHex("#00ff00"));
    CTEST_CHECK(qAbs(model.color().alphaF() - .5) < .0001);
    CTEST_CHECK(model.editHex("10203080"));
    PICKER_COMPARE(model.color(), QColor(16, 32, 48, 128));
    const QColor before = model.color();
    for (const QString &invalid : {QString("#xyz123"), QString("123"), QString("#123456789"), QString("")})
        CTEST_CHECK(!model.editHex(invalid));
    model.editChannel(ColorPickerModel::Hue, std::numeric_limits<double>::quiet_NaN());
    model.editHsv(20, std::numeric_limits<double>::infinity(), 1);
    model.editColor(QColor());
    PICKER_COMPARE(model.color(), before);
    model.editChannel(ColorPickerModel::Red, 900);
    PICKER_COMPARE(model.color().red(), 255);
    model.editChannel(ColorPickerModel::Alpha, -5);
    PICKER_COMPARE(model.color().alpha(), 0);
    model.editChannel(ColorPickerModel::Gray, 50);
    PICKER_COMPARE(model.color().red(), model.color().green());
    PICKER_COMPARE(model.color().green(), model.color().blue());
    PICKER_COMPARE(model.color().alpha(), 0);
    CTEST_CHECK(edits.size() >= 5);
}

void ColorPickerTests::achromatic_hue_and_cmyk_channels()
{
    ColorPickerModel model;
    model.editHsv(210, 0, 1);
    PICKER_COMPARE(model.hue(), 210.0);
    model.editChannel(ColorPickerModel::Saturation, 100);
    CTEST_CHECK(qAbs(model.color().hsvHueF() * 360 - 210) < .01);
    model.editChannel(ColorPickerModel::Brightness, 0);
    PICKER_COMPARE(model.saturation(), 1.0);
    model.editChannel(ColorPickerModel::Brightness, 100);
    CTEST_CHECK(qAbs(model.color().hsvHueF() * 360 - 210) < .01);
    model.editChannel(ColorPickerModel::Black, 35);
    model.editChannel(ColorPickerModel::Cyan, 20);
    PICKER_COMPARE(model.channelValue(ColorPickerModel::Black), 35.0);
    PICKER_COMPARE(model.channelValue(ColorPickerModel::Cyan), 20.0);
    model.editChannel(ColorPickerModel::Alpha, 60);
    PICKER_COMPARE(model.channelValue(ColorPickerModel::Black), 35.0);
    const QColor expected = QColor::fromCmykF(.2, model.channelValue(ColorPickerModel::Magenta) / 100,
        model.channelValue(ColorPickerModel::Yellow) / 100, .35, .6).toRgb();
    PICKER_COMPARE(model.color(), expected);
    model.editHex("000000");
    PICKER_COMPARE(model.channelValue(ColorPickerModel::Black), 100.0);
    model.editChannel(ColorPickerModel::Hue, 360);
    CTEST_CHECK(qIsFinite(model.hue()));
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
    CTEST_CHECK(object);
    auto *picker = object->findChild<QQuickItem *>("picker");
    auto *state = picker->findChild<ColorPickerModel *>();
    CTEST_CHECK(state);
    ColorPickerSignalRecorder observations;
    const auto editsConnection = QObject::connect(picker, SIGNAL(colorEdited(QColor)), &observations, SLOT(recordEdited(QColor)));
    const auto acceptedConnection = QObject::connect(picker, SIGNAL(accepted(QColor)), &observations, SLOT(recordAccepted(QColor)));
    const auto canceledConnection = QObject::connect(picker, SIGNAL(canceled()), &observations, SLOT(recordCanceled()));
    CTEST_CHECK(editsConnection && acceptedConnection && canceledConnection);
    state->editHex("ff1020");
    PICKER_COMPARE(object->property("documentColor").value<QColor>(), QColor("#ff1020"));
    PICKER_COMPARE(observations.editedColors.size(), 1);
    PICKER_COMPARE(observations.editedColors.first(), QColor("#ff1020"));
    object->setProperty("documentColor", QColor("#00ccff"));
    PICKER_COMPARE(picker->property("currentColor").value<QColor>(), QColor("#00ccff"));
    PICKER_COMPARE(observations.editedColors.size(), 1);
    for (int type = 0; type < 6; ++type) {
        picker->setProperty("type", type);
        PICKER_COMPARE(state->color(), QColor("#00ccff"));
    }
    CTEST_CHECK(QMetaObject::invokeMethod(picker, "cancel"));
    PICKER_COMPARE(state->color(), QColor("#0a84ff"));
    PICKER_COMPARE(object->property("documentColor").value<QColor>(), QColor("#0a84ff"));
    PICKER_COMPARE(observations.canceled, 1);
    state->editHex("123456");
    CTEST_CHECK(QMetaObject::invokeMethod(picker, "accept"));
    PICKER_COMPARE(observations.acceptedColors.size(), 1);
    PICKER_COMPARE(observations.acceptedColors.first(), QColor("#123456"));
    state->editHex("abcdef");
    CTEST_CHECK(QMetaObject::invokeMethod(picker, "cancel"));
    PICKER_COMPARE(state->color(), QColor("#123456"));
    object->setProperty("documentColor", QColor("#345678"));
    PICKER_COMPARE(state->color(), QColor("#345678"));
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
    CTEST_CHECK(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    root->setParentItem(window.contentItem());
    window.resize(1000, 720);
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    auto *first = object->findChild<QQuickItem *>("firstHost");
    auto *second = object->findChild<QQuickItem *>("secondHost");
    auto *firstView = qvariant_cast<QQuickItem *>(first->property("item"));
    auto *secondView = qvariant_cast<QQuickItem *>(second->property("item"));
    CTEST_CHECK(firstView && secondView && firstView != secondView);
    PICKER_COMPARE(firstView->parentItem(), first);
    PICKER_COMPARE(secondView->parentItem(), second);
    PICKER_COMPARE(firstView->width(), 320.0);
    PICKER_COMPARE(secondView->width(), 340.0);
    firstView->findChild<ColorPickerModel *>()->editHex("ff0000");
    PICKER_COMPARE(secondView->property("currentColor").value<QColor>(), QColor("#aabbcc"));
    auto *modal = object->findChild<QQuickItem *>("modal");
    CTEST_CHECK(modal);
    CTEST_CHECK(!qvariant_cast<QQuickItem *>(modal->property("contentItem")));
    modal->setProperty("open", true);
    CTEST_WAIT_DEFAULT(qvariant_cast<QQuickItem *>(modal->property("contentItem")));
    QPointer<QQuickItem> modalView = qvariant_cast<QQuickItem *>(modal->property("contentItem"));
    PICKER_WAIT_COMPARE(modalView->width(), 320.0);
    PICKER_COMPARE(modalView->parentItem(), modal->findChild<QQuickItem *>("modalContentLoader"));
    modalView->findChild<ColorPickerModel *>()->editHex("00ff00");
    modal->setProperty("open", false);
    CTEST_WAIT_DEFAULT(modalView.isNull());
    modal->setProperty("open", true);
    CTEST_WAIT_DEFAULT(qvariant_cast<QQuickItem *>(modal->property("contentItem")));
    PICKER_COMPARE(qvariant_cast<QQuickItem *>(modal->property("contentItem"))->property("currentColor").value<QColor>(), QColor("#aabbcc"));
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
    CTEST_CHECK(object);
    auto *picker = qobject_cast<QQuickItem *>(object.data());
    picker->setParentItem(window.contentItem());
    window.resize(400, 540);
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    auto *state = picker->findChild<ColorPickerModel *>();
    auto *field = picker->findChild<ColorPickerSurface *>("colorPicker_field");
    CTEST_CHECK(state && field);
    const auto at = [field](double x, double y) { return field->mapToScene(QPointF(x * field->width(), y * field->height())).toPoint(); };
    mouseClick(&window, at(.5, .25));
    CTEST_CHECK(qAbs(state->saturation() - .5) < .01);
    CTEST_CHECK(qAbs(state->brightness() - .75) < .01);
    field->forceActiveFocus(Qt::TabFocusReason);
    keyClick(&window, Qt::Key_Right);
    CTEST_CHECK(qAbs(state->saturation() - .51) < .01);
    auto *device = lvrs_ctest::createTouchDevice();
    lvrs_ctest::TouchSequence(&window, device).press(0, at(.2, .2), &window);
    lvrs_ctest::TouchSequence(&window, device).move(0, at(.8, .5), &window);
    lvrs_ctest::TouchSequence(&window, device).release(0, at(.8, .5), &window);
    CTEST_WAIT_DEFAULT(qAbs(state->saturation() - .8) < .02);
    CTEST_WAIT_DEFAULT(qAbs(state->brightness() - .5) < .02);
    auto *hex = picker->findChild<QQuickItem *>("colorPicker_hex");
    auto *input = qvariant_cast<QQuickItem *>(hex->property("inputItem"));
    CTEST_CHECK(input);
    input->forceActiveFocus(Qt::TabFocusReason);
    input->setProperty("text", "10203080");
    CTEST_CHECK(QMetaObject::invokeMethod(input, "textEdited"));
    keyClick(&window, Qt::Key_Return);
    PICKER_COMPARE(state->color(), QColor(16, 32, 48, 128));
    input->setProperty("text", "invalid");
    CTEST_CHECK(QMetaObject::invokeMethod(input, "textEdited"));
    keyClick(&window, Qt::Key_Return);
    PICKER_COMPARE(state->hex(), QString("102030"));
    PICKER_COMPARE(input->property("text").toString(), QString("102030"));
    auto *alpha = picker->findChild<QQuickItem *>("colorPicker_alpha");
    auto *alphaInput = qvariant_cast<QQuickItem *>(alpha->property("inputItem"));
    alphaInput->forceActiveFocus(Qt::TabFocusReason);
    const QColor beforeFocusCycle = state->color();
    field->forceActiveFocus(Qt::TabFocusReason);
    PICKER_COMPARE(state->color(), beforeFocusCycle);
    alphaInput->forceActiveFocus(Qt::TabFocusReason);
    alphaInput->setProperty("text", "25");
    CTEST_CHECK(QMetaObject::invokeMethod(alphaInput, "textEdited"));
    field->forceActiveFocus(Qt::TabFocusReason);
    CTEST_CHECK(qAbs(state->color().alphaF() - .25) < .001);
    ColorPickerSignalRecorder observations;
    const auto eyedropperConnection = QObject::connect(picker, SIGNAL(eyedropperRequested()), &observations, SLOT(recordEyedropper()));
    CTEST_CHECK(eyedropperConnection);
    auto *button = picker->findChild<QQuickItem *>("colorPicker_eyedropper");
    mouseClick(&window, button->mapToScene(QPointF(11, 11)).toPoint());
    PICKER_COMPARE(observations.eyedropper, 1);
    auto *recent = findVisual(picker, QStringLiteral("colorPicker_recent_1"));
    CTEST_CHECK(recent);
    mouseClick(&window, recent->mapToScene(QPointF(12, 10)).toPoint());
    PICKER_COMPARE(state->color(), AccentFixture::color("accentRed"));
    picker->setEnabled(false);
    mouseClick(&window, at(.1, .1));
    PICKER_COMPARE(state->color(), AccentFixture::color("accentRed"));
}

void ColorPickerTests::catalog_renders_six_views()
{
    QQuickWindow window;
    window.setColor(QColor("#161819"));
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../example/VisualCatalog/qml/ColorPickerGallery.qml")));
    CTEST_CHECK(object);
    auto *gallery = qobject_cast<QQuickItem *>(object.data());
    gallery->setWidth(1008);
    gallery->setX(16); gallery->setY(16);
    gallery->setParentItem(window.contentItem());
    QCoreApplication::processEvents();
    window.resize(1040, qCeil(gallery->implicitHeight()) + 32);
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    for (int i = 0; i < 6; ++i) {
        auto *picker = findVisual(gallery, QString("gallery_colorPicker_%1").arg(i));
        CTEST_CHECK(picker);
        PICKER_COMPARE(picker->property("type").toInt(), i);
        PICKER_COMPARE(picker->property("currentColor").value<QColor>(), AccentFixture::color("accentPurple"));
    }
    auto grab = gallery->grabToImage();
    waitForGrab(grab);
    const QString captureDir = qEnvironmentVariable("LVRS_COLORPICKER_CAPTURE_DIR");
    if (!captureDir.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(captureDir));
        CTEST_CHECK(grab->saveToFile(captureDir + "/gallery.png"));
    }
    auto *modal = gallery->findChild<QQuickItem *>("gallery_colorModal");
    CTEST_CHECK(modal);
    gallery->setProperty("modalStartColor", QColor("#7a5af8"));
    modal->setProperty("open", true);
    CTEST_WAIT_DEFAULT(qvariant_cast<QQuickItem *>(modal->property("contentItem")));
    auto *modalPicker = qvariant_cast<QQuickItem *>(modal->property("contentItem"));
    modalPicker->findChild<ColorPickerModel *>()->editHex("ff0000");
    PICKER_COMPARE(gallery->property("selectedColor").value<QColor>(), QColor("#ff0000"));
    CTEST_CHECK(QMetaObject::invokeMethod(modal, "cancel"));
    PICKER_COMPARE(gallery->property("selectedColor").value<QColor>(), QColor("#7a5af8"));
}

void ColorPickerTests::all_color_domains()
{
    QQuickWindow window;
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, "import LVRS as LV\nLV.ColorPicker {}"));
    CTEST_CHECK(object);
    auto *picker = qobject_cast<QQuickItem *>(object.data());
    picker->setParentItem(window.contentItem());
    window.resize(400, 540);
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    auto *state = picker->findChild<ColorPickerModel *>();
    auto *field = picker->findChild<ColorPickerSurface *>("colorPicker_field");
    CTEST_CHECK(state && field);
    for (int type = 0; type < 6; ++type) {
        picker->setProperty("type", type);
        state->editHsv(210, .4, .8);
        QCoreApplication::processEvents();
        const QPointF point = type == 0 ? QPointF(232, 116) : QPointF(field->width() * .5, field->height() * .25);
        mouseClick(&window, field->mapToScene(point).toPoint());
        CTEST_CHECK(qAbs(state->hue() - (type == 0 || type == 2 ? 210 : 180)) < .01);
        CTEST_CHECK(qAbs(state->saturation() - (type == 0 ? 1 : type == 1 ? .75 : type == 2 ? .5 : .25 / .45)) < .01);
        CTEST_CHECK(qAbs(state->brightness() - (type == 1 ? .8 : type == 2 ? .75 : 1)) < .01);
    }
    picker->setProperty("type", 0);
    QCoreApplication::processEvents();
    mouseClick(&window, field->mapToScene(QPointF(148, 216)).toPoint());
    CTEST_CHECK(qAbs(state->hue()) < .01);
    QImage ring(QStringLiteral(":/qt/qml/LVRS/resources/images/colorpicker/hue-ring.png"));
    CTEST_CHECK(!ring.isNull());
    const QColor bottom = ring.pixelColor(224, 425);
    CTEST_CHECK(bottom.red() > 240 && bottom.green() < 20 && bottom.blue() < 20);
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    ColorPickerTests tests;
    const int heights[] = {461, 437, 437, 361, 413, 439};
    for (int type = 0; type < 6; ++type) {
        const auto name = QStringLiteral("content_variants:") + QString::number(type);
        runner.run(name.toStdString(), [&] { tests.content_variants(type, heights[type]); });
    }
    runner.run("authored_palette_defaults", [&] { tests.authored_palette_defaults(); });
    runner.run("color_conversion_and_validation", [&] { tests.color_conversion_and_validation(); });
    runner.run("achromatic_hue_and_cmyk_channels", [&] { tests.achromatic_hue_and_cmyk_channels(); });
    runner.run("external_binding_and_accept_cancel", [&] { tests.external_binding_and_accept_cancel(); });
    runner.run("component_hosts_and_modal_lifecycle", [&] { tests.component_hosts_and_modal_lifecycle(); });
    runner.run("pointer_keyboard_touch_and_fields", [&] { tests.pointer_keyboard_touch_and_fields(); });
    runner.run("all_color_domains", [&] { tests.all_color_domains(); });
    runner.run("catalog_renders_six_views", [&] { tests.catalog_renders_six_views(); });
    return runner.result();
}
#include "tst_colorpicker.moc"
