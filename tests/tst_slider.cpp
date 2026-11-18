#include "QtEventWait.h"
#include "QtGuiTouch.h"
#include <QCoreApplication>
#include <QDebug>
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
#include <functional>
#include "test_utils.h"

class SliderSignalRecorder : public QObject
{
    Q_OBJECT
public:
    int moved = 0;
public slots:
    void recordMoved() { ++moved; }
};

namespace {
struct SliderVariant {
    int type;
    int size;
    int state;
    QString target;
    QString tag() const { return QString("%1-%2-%3-%4").arg(target).arg(type).arg(size).arg(state); }
};

QList<SliderVariant> sliderVariants()
{
    QList<SliderVariant> result;
    for (const QString &target : {QString("macos"), QString("ios")})
        for (int type = 0; type < 7; ++type)
            for (int size = 0; size < 4; ++size)
                for (int state = 1; state <= 4; ++state)
                    result.append({type, size, state, target});
    return result;
}

template <typename Actual, typename Expected>
bool sliderEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual) + 1.0, qreal(expected) + 1.0);
    else
        return actual == expected;
}

bool matchesAccentPixel(const QColor &pixel, const QColor &expectedAccent)
{
    constexpr int channelTolerance = 2;
    return pixel.alpha() > 200
        && qAbs(pixel.red() - expectedAccent.red()) <= channelTolerance
        && qAbs(pixel.green() - expectedAccent.green()) <= channelTolerance
        && qAbs(pixel.blue() - expectedAccent.blue()) <= channelTolerance;
}

void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
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

void keyClick(QQuickWindow *window, Qt::Key key)
{
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}
}

#define SLIDER_COMPARE(actual, expected) CTEST_CHECK(sliderEqual((actual), (expected)))
#define SLIDER_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(sliderEqual((actual), (expected)))

class SliderTests
{
public:
    void figma_variants(const SliderVariant &variant);
    void ranges_resize_and_state_cycles();
    void keyboard_mouse_touch_and_snapping();
    void right_to_left_and_descending_ranges();
    void labels_icons_and_disabled();
    void catalog_renders_all_designs();
    void accent_pixel_matches_token();
};

void SliderTests::figma_variants(const SliderVariant &variant)
{
    const auto &[type, size, state, target] = variant;
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, QString(R"(
import QtQuick
import LVRS as LV
LV.Slider {
    type: %1; size: %2; displayState: %3
    Component.onCompleted: LV.Theme.targetOverride = "%4"
})").arg(type).arg(size).arg(state).arg(target).toUtf8()));
    CTEST_CHECK(object);
    auto *slider = qobject_cast<QQuickItem *>(object.data());
    CTEST_CHECK(slider);
    const int heights[] = {22, 24, 32, 44};
    const int tracks[] = {4, 6, 6, 8};
    const int filledTracks[] = {12, 16, 28, 44};
    const int thumbs[] = {12, 14, 18, 22};
    const int filledThumbs[] = {8, 8, 20, 36};
    SLIDER_COMPARE(slider->implicitWidth(), 320.0);
    SLIDER_COMPARE(slider->implicitHeight(), qreal(heights[size]));
    SLIDER_COMPARE(slider->property("trackHeight").toReal(), qreal(type >= 4 ? filledTracks[size] : tracks[size]));
    SLIDER_COMPARE(slider->property("thumbDiameter").toReal(), qreal(type >= 4 ? filledThumbs[size] : thumbs[size]));
    SLIDER_COMPARE(slider->opacity(), state == 4 ? 0.32 : 1.0);
    SLIDER_COMPARE(slider->isEnabled(), state != 4);
    SLIDER_COMPARE(slider->property("showMinMax").toBool(), type >= 5);
    SLIDER_COMPARE(slider->property("showTicks").toBool(), type == 2 || type == 3 || type == 6);
    SLIDER_COMPARE(slider->property("effectiveThumbVisible").toBool(), type < 4 || type == 6 || state == 2 || state == 3);
    SLIDER_COMPARE(slider->property("effectiveFocusRing").toBool(), state == 2 || state == 3);
    SLIDER_COMPARE(slider->property("value").toReal(), 0.5);
    SLIDER_COMPARE(slider->property("stepSize").toReal(), type == 6 ? 0.25 : 0.0);
    auto *symbol = slider->findChild<QQuickItem *>("slider_symbol");
    CTEST_CHECK(symbol);
    SLIDER_COMPARE(symbol->isVisible(), type == 4 && size >= 2);
}

void SliderTests::ranges_resize_and_state_cycles()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, "import LVRS as LV\nLV.Slider { motionEnabled: false }"));
    CTEST_CHECK(object);
    auto *slider = qobject_cast<QQuickItem *>(object.data());
    auto *fill = slider->findChild<QQuickItem *>("slider_fill");
    auto *handle = slider->findChild<QQuickItem *>("slider_handle");
    CTEST_CHECK(fill && handle);
    const qreal insets[] = {8, 8, 9, 11};
    const qreal capsules[] = {12, 16, 28, 44};
    for (int type = 0; type < 7; ++type) {
        slider->setProperty("type", type);
        for (int size = 0; size < 4; ++size) {
            slider->setProperty("size", size);
            for (int width : {200, 480}) {
                slider->setWidth(width);
                QCoreApplication::processEvents();
                const qreal left = slider->property("leftPadding").toReal();
                const qreal right = slider->property("rightPadding").toReal();
                const qreal inset = type >= 4 ? capsules[size] / 2 : insets[size];
                const qreal range = width - left - right - 2 * inset;
                for (qreal value : {0.0, 0.25, 0.5, 0.75, 1.0}) {
                    slider->setProperty("value", value);
                    CTEST_CHECK(qAbs(handle->x() + handle->width() / 2 - (left + inset + range * value)) < 0.01);
                    const bool centered = type == 1 || type == 3;
                    const qreal length = centered ? qAbs(value - 0.5) : value;
                    const qreal start = centered ? qMin(value, 0.5) : 0;
                    CTEST_CHECK(qAbs(fill->width() - (length * range + (type >= 4 ? capsules[size] : 0))) < 0.01);
                    CTEST_CHECK(qAbs(fill->x() - (left + (type >= 4 ? 0 : inset) + start * range)) < 0.01);
                    CTEST_CHECK(fill->x() >= 0 && fill->x() + fill->width() <= width + 0.01);
                    if (type == 4 && size >= 2 && value == 0) {
                        auto *symbol = slider->findChild<QQuickItem *>("slider_symbol");
                        CTEST_CHECK(symbol && symbol->parentItem() == slider && symbol->z() > handle->z());
                        SLIDER_COMPARE(symbol->x() + symbol->width() / 2, handle->x() + handle->width() / 2);
                    }
                }
                slider->setProperty("value", 0.75);
                for (int state : {1, 2, 3, 2, 1, 4, 1, 0}) {
                    slider->setProperty("displayState", state);
                    SLIDER_COMPARE(slider->property("size").toInt(), size);
                    SLIDER_COMPARE(slider->property("value").toReal(), 0.75);
                    CTEST_CHECK(qAbs(handle->x() + handle->width() / 2 - (left + inset + range * 0.75)) < 0.01);
                }
            }
        }
    }
    slider->setProperty("from", 10.0);
    slider->setProperty("to", 20.0);
    slider->setProperty("value", -100.0);
    SLIDER_COMPARE(slider->property("value").toReal(), 10.0);
    slider->setProperty("value", 100.0);
    SLIDER_COMPARE(slider->property("value").toReal(), 20.0);
    slider->setProperty("to", 10.0);
    SLIDER_COMPARE(slider->property("value").toReal(), 10.0);
    CTEST_CHECK(qIsFinite(fill->width()) && qIsFinite(handle->x()));
}

void SliderTests::keyboard_mouse_touch_and_snapping()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(420, 140);
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.Slider { x: 20; y: 40; width: 360; from: 0; to: 100; value: 50; stepSize: 10; Accessible.name: "Exposure" }
)"));
    CTEST_CHECK(object);
    auto *slider = qobject_cast<QQuickItem *>(object.data());
    slider->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    window.requestActivate();
    slider->forceActiveFocus(Qt::TabFocusReason);
    CTEST_WAIT_DEFAULT(slider->hasActiveFocus());
    CTEST_CHECK(slider->property("effectiveFocusRing").toBool());
    SliderSignalRecorder observations;
    const auto movedConnection = QObject::connect(slider, SIGNAL(moved()), &observations, SLOT(recordMoved()));
    CTEST_CHECK(movedConnection);
    keyClick(&window, Qt::Key_Right);
    SLIDER_COMPARE(slider->property("value").toReal(), 60.0);
    keyClick(&window, Qt::Key_Left);
    SLIDER_COMPARE(slider->property("value").toReal(), 50.0);
    keyClick(&window, Qt::Key_Home);
    SLIDER_COMPARE(slider->property("value").toReal(), 0.0);
    keyClick(&window, Qt::Key_End);
    SLIDER_COMPARE(slider->property("value").toReal(), 100.0);
    SLIDER_COMPARE(observations.moved, 4);

    const auto pointAt = [slider](qreal position) {
        const qreal x = slider->property("leftPadding").toReal() + slider->property("rangeInset").toReal()
            + position * slider->property("rangeWidth").toReal();
        return slider->mapToScene(QPointF(x, slider->height() / 2)).toPoint();
    };
    sendMouse(&window, QEvent::MouseMove, pointAt(0.4), Qt::NoButton, Qt::NoButton);
    CTEST_WAIT_DEFAULT(slider->property("hovered").toBool());
    sendMouse(&window, QEvent::MouseButtonPress, pointAt(0.4), Qt::LeftButton, Qt::LeftButton);
    CTEST_CHECK(slider->property("pressed").toBool());
    CTEST_CHECK(slider->property("effectivePressed").toBool());
    sendMouse(&window, QEvent::MouseMove, pointAt(0.8), Qt::NoButton, Qt::LeftButton, 20);
    sendMouse(&window, QEvent::MouseButtonRelease, pointAt(0.8), Qt::LeftButton, Qt::NoButton);
    CTEST_CHECK(qAbs(slider->property("value").toReal() - 80) < 0.3);
    CTEST_CHECK(!slider->property("pressed").toBool());

    slider->setProperty("live", false);
    slider->setProperty("value", 20.0);
    sendMouse(&window, QEvent::MouseButtonPress, pointAt(0.7), Qt::LeftButton, Qt::LeftButton);
    sendMouse(&window, QEvent::MouseMove, pointAt(0.9), Qt::NoButton, Qt::LeftButton, 20);
    SLIDER_COMPARE(slider->property("value").toReal(), 20.0);
    sendMouse(&window, QEvent::MouseButtonRelease, pointAt(0.9), Qt::LeftButton, Qt::NoButton);
    CTEST_CHECK(qAbs(slider->property("value").toReal() - 90) < 0.3);
    slider->setProperty("live", true);

    auto *device = lvrs_ctest::createTouchDevice();
    lvrs_ctest::TouchSequence(&window, device).press(0, pointAt(0.9), &window);
    lvrs_ctest::TouchSequence(&window, device).move(0, pointAt(0.2), &window);
    lvrs_ctest::TouchSequence(&window, device).release(0, pointAt(0.2), &window);
    CTEST_WAIT_DEFAULT(qAbs(slider->property("value").toReal() - 20) < 0.3);

    // A separate instance keeps the Segmented step binding intact.
    QScopedPointer<QObject> segmentedObject(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.Slider { x: 20; y: 80; width: 360; type: LV.Slider.Segmented; from: 0; to: 100; value: 50 }
)"));
    CTEST_CHECK(segmentedObject);
    auto *segmented = qobject_cast<QQuickItem *>(segmentedObject.data());
    segmented->setParentItem(window.contentItem());
    const qreal start = segmented->property("leftPadding").toReal() + segmented->property("rangeInset").toReal();
    const QPoint target = segmented->mapToScene(QPointF(start + segmented->property("rangeWidth").toReal() * 0.64, 16)).toPoint();
    mouseClick(&window, target);
    SLIDER_COMPARE(segmented->property("value").toReal(), 75.0);
    segmented->forceActiveFocus();
    keyClick(&window, Qt::Key_Left);
    SLIDER_COMPARE(segmented->property("value").toReal(), 50.0);
    segmented->setProperty("segmentCount", 3);
    SLIDER_COMPARE(segmented->property("stepSize").toReal(), 50.0);
    keyClick(&window, Qt::Key_Right);
    SLIDER_COMPARE(segmented->property("value").toReal(), 100.0);
    segmented->setProperty("segmentCount", 0);
    SLIDER_COMPARE(segmented->property("stepSize").toReal(), 100.0);
}

void SliderTests::right_to_left_and_descending_ranges()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.Slider { motionEnabled: false; width: 480; from: 10; to: -10; value: 5; LayoutMirroring.enabled: true }
)"));
    CTEST_CHECK(object);
    auto *slider = qobject_cast<QQuickItem *>(object.data());
    auto *fill = slider->findChild<QQuickItem *>("slider_fill");
    auto *handle = slider->findChild<QQuickItem *>("slider_handle");
    CTEST_CHECK(fill && handle);
    for (int type = 0; type < 7; ++type) {
        slider->setProperty("type", type);
        QCoreApplication::processEvents();
        SLIDER_COMPARE(slider->property("position").toReal(), 0.25);
        SLIDER_COMPARE(slider->property("visualPosition").toReal(), 0.75);
        const qreal left = slider->property("leftPadding").toReal();
        const qreal inset = slider->property("rangeInset").toReal();
        const qreal range = slider->property("rangeWidth").toReal();
        CTEST_CHECK(qAbs(handle->x() + handle->width() / 2 - (left + inset + range * 0.75)) < 0.01);
        const bool centered = type == 1 || type == 3;
        CTEST_CHECK(qAbs(fill->x() - (left + (type >= 4 ? 0 : inset) + (centered ? 0.5 : 0.75) * range)) < 0.01);
        CTEST_CHECK(fill->x() + fill->width() <= slider->width() + 0.01);
    }
}

void SliderTests::labels_icons_and_disabled()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(220, 140);
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.Slider {
    y: 30; width: 180; type: LV.Slider.MinMaxLabels; showEndpointIcons: true
    minimumLabel: "A very long minimum label"; maximumLabel: "A very long maximum label"
    minimumIconName: "generaladd"; maximumIconName: "sun"
    property string expectedMinimumIcon: LV.Theme.iconPath("generaladd")
    property color expectedInactiveFill: LV.Theme.disabledColor
}
)"));
    CTEST_CHECK(object);
    auto *slider = qobject_cast<QQuickItem *>(object.data());
    slider->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    auto *minimum = slider->findChild<QQuickItem *>("slider_minimum");
    auto *maximum = slider->findChild<QQuickItem *>("slider_maximum");
    auto *fill = slider->findChild<QQuickItem *>("slider_fill");
    CTEST_CHECK(minimum && maximum && fill);
    CTEST_WAIT_DEFAULT(slider->property("availableWidth").toReal() >= 39.99);
    SLIDER_COMPARE(minimum->property("iconUrl").toUrl().toString(), slider->property("expectedMinimumIcon").toString());
    CTEST_CHECK(minimum->x() + minimum->width() < fill->x());
    CTEST_CHECK(maximum->x() + maximum->width() <= slider->width());
    const qreal padding = slider->property("leftPadding").toReal();
    slider->setVisible(false);
    QCoreApplication::processEvents();
    SLIDER_COMPARE(slider->property("leftPadding").toReal(), padding);
    CTEST_CHECK(padding > 0); // Ancestor visibility must not change the layout contract.
    slider->setVisible(true);
    SLIDER_COMPARE(slider->property("leftPadding").toReal(), padding);
    slider->setProperty("showLabels", false);
    SLIDER_WAIT_COMPARE(minimum->width(), 18.0);
    slider->setProperty("showMinMax", false);
    SLIDER_WAIT_COMPARE(slider->property("availableWidth").toReal(), 180.0);
    slider->setProperty("active", false);
    SLIDER_COMPARE(fill->property("color").value<QColor>(), slider->property("expectedInactiveFill").value<QColor>());
    CTEST_CHECK(slider->isEnabled());
    slider->setEnabled(false);
    const qreal value = slider->property("value").toReal();
    SliderSignalRecorder observations;
    const auto movedConnection = QObject::connect(slider, SIGNAL(moved()), &observations, SLOT(recordMoved()));
    CTEST_CHECK(movedConnection);
    mouseClick(&window, QPoint(145, 46));
    keyClick(&window, Qt::Key_Right);
    SLIDER_COMPARE(slider->property("value").toReal(), value);
    SLIDER_COMPARE(observations.moved, 0);
    SLIDER_COMPARE(slider->opacity(), 0.32);
}

void SliderTests::catalog_renders_all_designs()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(1100, 1000);
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
        QStringLiteral(LVRS_TEST_SOURCE_DIR) + "/../example/VisualCatalog/qml/SliderGallery.qml"));
    CTEST_CHECK(object);
    auto *gallery = qobject_cast<QQuickItem *>(object.data());
    gallery->setWidth(1100);
    gallery->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    CTEST_WAIT_DEFAULT(gallery->height() > 0);
    QList<QQuickItem *> variants;
    std::function<void(QQuickItem *)> collect = [&](QQuickItem *item) {
        if (item->objectName().startsWith("slider_variant_"))
            variants.append(item);
        for (auto *child : item->childItems())
            collect(child);
    };
    collect(gallery);
    SLIDER_COMPARE(variants.size(), 112);
    const auto variantsFit = [&] {
        for (auto *variant : variants) {
            const QPointF bottom = variant->mapToItem(gallery, QPointF(variant->width(), variant->height()));
            if (bottom.x() > gallery->width() + 0.01 || bottom.y() > gallery->height() + 0.01)
                return false;
        }
        return true;
    };
    for (int width : {1100, 390}) {
        gallery->setWidth(width);
        CTEST_WAIT_DEFAULT(variantsFit()); // Positioners finish their polish on the next native frame.
    }
    gallery->setWidth(1100);
    CTEST_WAIT_DEFAULT(variantsFit());
    processFor(200);
    const auto capture = gallery->grabToImage(QSize(1100, qCeil(gallery->height())));
    CTEST_CHECK(capture);
    int readyCount = 0;
    const auto captureConnection = QObject::connect(capture.data(), &QQuickItemGrabResult::ready,
                                                     [&] { ++readyCount; });
    CTEST_CHECK(captureConnection);
    const auto disconnectCapture = qScopeGuard([&] { QObject::disconnect(captureConnection); });
    CTEST_WAIT_VERIFY(readyCount > 0, 5000);
    const QImage pixels = capture->image();
    const qreal pixelRatio = pixels.width() / 1100.0;
    const QString directory = qEnvironmentVariable("LVRS_SLIDER_CAPTURE_DIR");
    if (!directory.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(directory));
        CTEST_CHECK(capture->saveToFile(directory + "/slider-gallery.png"));
        const int previewHeight = qRound((gallery->property("examplesHeight").toReal() + 12) * pixelRatio);
        CTEST_CHECK(pixels.copy(0, 0, pixels.width(), qMin(pixels.height(), previewHeight))
                    .save(directory + "/slider-examples.png"));
    }
    for (auto *variant : variants) {
        if (!variant->property("effectiveThumbVisible").toBool())
            continue;
        auto *handle = variant->findChild<QQuickItem *>("slider_handle");
        CTEST_CHECK(handle);
        const QPointF center = handle->mapToItem(gallery, QPointF(handle->width() / 2, handle->height() / 2));
        const QColor pixel = pixels.pixelColor(qRound(center.x() * pixelRatio), qRound(center.y() * pixelRatio));
        CTEST_CHECK_MESSAGE(pixel.red() > (variant->isEnabled() ? 215 : 75), qPrintable(variant->objectName()));
        // TitleHeader is 90% white: the blue capsule remains faintly visible through it.
        CTEST_CHECK_MESSAGE(qAbs(pixel.red() - pixel.green()) < 35 && qAbs(pixel.red() - pixel.blue()) < 35,
                 qPrintable(variant->objectName() + " must render the translucent light thumb: " + pixel.name()));
    }
    const QColor expectedAccent = variants.first()->property("fillColor").value<QColor>();
    CTEST_CHECK(expectedAccent.isValid());
    int accent = 0, light = 0;
    for (int y = 0; y < pixels.height(); ++y)
        for (int x = 0; x < pixels.width(); ++x) {
            const QColor color = pixels.pixelColor(x, y);
            if (matchesAccentPixel(color, expectedAccent))
                ++accent;
            if (color.red() > 200 && color.green() > 200 && color.blue() > 200 && color.alpha() > 200)
                ++light;
        }
    qInfo() << "slider gallery pixel counts" << "accent" << accent << "light" << light
            << "expected accent" << expectedAccent;
    CTEST_CHECK(accent > 5000 && light > 1000);
}

void SliderTests::accent_pixel_matches_token()
{
    const QColor accent(Qt::red);
    CTEST_CHECK(matchesAccentPixel(accent, accent));
    CTEST_CHECK(!matchesAccentPixel(QColor(Qt::blue), accent));
    CTEST_CHECK(!matchesAccentPixel(QColor(255, 0, 0, 0), accent));
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    SliderTests tests;
    for (const SliderVariant &variant : sliderVariants()) {
        const auto caseName = QStringLiteral("figma_variants:") + variant.tag();
        runner.run(caseName.toStdString(), [&] { tests.figma_variants(variant); });
    }
    runner.run("ranges_resize_and_state_cycles", [&] { tests.ranges_resize_and_state_cycles(); });
    runner.run("keyboard_mouse_touch_and_snapping", [&] { tests.keyboard_mouse_touch_and_snapping(); });
    runner.run("right_to_left_and_descending_ranges", [&] { tests.right_to_left_and_descending_ranges(); });
    runner.run("labels_icons_and_disabled", [&] { tests.labels_icons_and_disabled(); });
    runner.run("catalog_renders_all_designs", [&] { tests.catalog_renders_all_designs(); });
    runner.run("accent_pixel_matches_token", [&] { tests.accent_pixel_matches_token(); });
    return runner.result();
}
#include "tst_slider.moc"
