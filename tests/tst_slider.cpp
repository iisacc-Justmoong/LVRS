#include <QtTest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QQmlEngine>
#include <QSignalSpy>
#include <functional>
#include "test_utils.h"

class SliderTests : public QObject
{
    Q_OBJECT
private slots:
    void figma_variants_data();
    void figma_variants();
    void ranges_resize_and_state_cycles();
    void keyboard_mouse_touch_and_snapping();
    void right_to_left_and_descending_ranges();
    void labels_icons_and_disabled();
    void catalog_renders_all_designs();
};

void SliderTests::figma_variants_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("size");
    QTest::addColumn<int>("state");
    QTest::addColumn<QString>("target");
    for (const QString &target : {QString("macos"), QString("ios")})
        for (int type = 0; type < 7; ++type)
            for (int size = 0; size < 4; ++size)
                for (int state = 1; state <= 4; ++state)
                    QTest::newRow(qPrintable(QString("%1-%2-%3-%4").arg(target).arg(type).arg(size).arg(state)))
                        << type << size << state << target;
}

void SliderTests::figma_variants()
{
    QFETCH(int, type); QFETCH(int, size); QFETCH(int, state); QFETCH(QString, target);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, QString(R"(
import QtQuick
import LVRS as LV
LV.Slider {
    type: %1; size: %2; displayState: %3
    Component.onCompleted: LV.Theme.targetOverride = "%4"
})").arg(type).arg(size).arg(state).arg(target).toUtf8()));
    QVERIFY(object);
    auto *slider = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(slider);
    const int heights[] = {22, 24, 32, 44};
    const int tracks[] = {4, 6, 6, 8};
    const int filledTracks[] = {12, 16, 28, 44};
    const int thumbs[] = {12, 14, 18, 22};
    const int filledThumbs[] = {8, 8, 20, 36};
    QCOMPARE(slider->implicitWidth(), 320.0);
    QCOMPARE(slider->implicitHeight(), qreal(heights[size]));
    QCOMPARE(slider->property("trackHeight").toReal(), qreal(type >= 4 ? filledTracks[size] : tracks[size]));
    QCOMPARE(slider->property("thumbDiameter").toReal(), qreal(type >= 4 ? filledThumbs[size] : thumbs[size]));
    QCOMPARE(slider->opacity(), state == 4 ? 0.32 : 1.0);
    QCOMPARE(slider->isEnabled(), state != 4);
    QCOMPARE(slider->property("showMinMax").toBool(), type >= 5);
    QCOMPARE(slider->property("showTicks").toBool(), type == 2 || type == 3 || type == 6);
    QCOMPARE(slider->property("effectiveThumbVisible").toBool(), type < 4 || type == 6 || state == 2 || state == 3);
    QCOMPARE(slider->property("effectiveFocusRing").toBool(), state == 2 || state == 3);
    QCOMPARE(slider->property("value").toReal(), 0.5);
    QCOMPARE(slider->property("stepSize").toReal(), type == 6 ? 0.25 : 0.0);
    auto *symbol = slider->findChild<QQuickItem *>("slider_symbol");
    QVERIFY(symbol);
    QCOMPARE(symbol->isVisible(), type == 4 && size >= 2);
}

void SliderTests::ranges_resize_and_state_cycles()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, "import LVRS as LV\nLV.Slider {}"));
    QVERIFY(object);
    auto *slider = qobject_cast<QQuickItem *>(object.data());
    auto *fill = slider->findChild<QQuickItem *>("slider_fill");
    auto *handle = slider->findChild<QQuickItem *>("slider_handle");
    QVERIFY(fill && handle);
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
                    QVERIFY(qAbs(handle->x() + handle->width() / 2 - (left + inset + range * value)) < 0.01);
                    const bool centered = type == 1 || type == 3;
                    const qreal length = centered ? qAbs(value - 0.5) : value;
                    const qreal start = centered ? qMin(value, 0.5) : 0;
                    QVERIFY(qAbs(fill->width() - (length * range + (type >= 4 ? capsules[size] : 0))) < 0.01);
                    QVERIFY(qAbs(fill->x() - (left + (type >= 4 ? 0 : inset) + start * range)) < 0.01);
                    QVERIFY(fill->x() >= 0 && fill->x() + fill->width() <= width + 0.01);
                    if (type == 4 && size >= 2 && value == 0) {
                        auto *symbol = slider->findChild<QQuickItem *>("slider_symbol");
                        QVERIFY(symbol && symbol->parentItem() == slider && symbol->z() > handle->z());
                        QCOMPARE(symbol->x() + symbol->width() / 2, handle->x() + handle->width() / 2);
                    }
                }
                slider->setProperty("value", 0.75);
                for (int state : {1, 2, 3, 2, 1, 4, 1, 0}) {
                    slider->setProperty("displayState", state);
                    QCOMPARE(slider->property("size").toInt(), size);
                    QCOMPARE(slider->property("value").toReal(), 0.75);
                    QVERIFY(qAbs(handle->x() + handle->width() / 2 - (left + inset + range * 0.75)) < 0.01);
                }
            }
        }
    }
    slider->setProperty("from", 10.0);
    slider->setProperty("to", 20.0);
    slider->setProperty("value", -100.0);
    QCOMPARE(slider->property("value").toReal(), 10.0);
    slider->setProperty("value", 100.0);
    QCOMPARE(slider->property("value").toReal(), 20.0);
    slider->setProperty("to", 10.0);
    QCOMPARE(slider->property("value").toReal(), 10.0);
    QVERIFY(qIsFinite(fill->width()) && qIsFinite(handle->x()));
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
    QVERIFY(object);
    auto *slider = qobject_cast<QQuickItem *>(object.data());
    slider->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.requestActivate();
    slider->forceActiveFocus(Qt::TabFocusReason);
    QTRY_VERIFY(slider->hasActiveFocus());
    QVERIFY(slider->property("effectiveFocusRing").toBool());
    QSignalSpy moved(slider, SIGNAL(moved()));
    QTest::keyClick(&window, Qt::Key_Right);
    QCOMPARE(slider->property("value").toReal(), 60.0);
    QTest::keyClick(&window, Qt::Key_Left);
    QCOMPARE(slider->property("value").toReal(), 50.0);
    QTest::keyClick(&window, Qt::Key_Home);
    QCOMPARE(slider->property("value").toReal(), 0.0);
    QTest::keyClick(&window, Qt::Key_End);
    QCOMPARE(slider->property("value").toReal(), 100.0);
    QCOMPARE(moved.count(), 4);

    const auto pointAt = [slider](qreal position) {
        const qreal x = slider->property("leftPadding").toReal() + slider->property("rangeInset").toReal()
            + position * slider->property("rangeWidth").toReal();
        return slider->mapToScene(QPointF(x, slider->height() / 2)).toPoint();
    };
    QTest::mouseMove(&window, pointAt(0.4));
    QTRY_VERIFY(slider->property("hovered").toBool());
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, pointAt(0.4));
    QVERIFY(slider->property("pressed").toBool());
    QVERIFY(slider->property("effectivePressed").toBool());
    QTest::mouseMove(&window, pointAt(0.8), 20);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, pointAt(0.8));
    QVERIFY(qAbs(slider->property("value").toReal() - 80) < 0.3);
    QVERIFY(!slider->property("pressed").toBool());

    slider->setProperty("live", false);
    slider->setProperty("value", 20.0);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, pointAt(0.7));
    QTest::mouseMove(&window, pointAt(0.9), 20);
    QCOMPARE(slider->property("value").toReal(), 20.0);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, pointAt(0.9));
    QVERIFY(qAbs(slider->property("value").toReal() - 90) < 0.3);
    slider->setProperty("live", true);

    auto *device = QTest::createTouchDevice();
    QTest::touchEvent(&window, device).press(0, pointAt(0.9), &window);
    QTest::touchEvent(&window, device).move(0, pointAt(0.2), &window);
    QTest::touchEvent(&window, device).release(0, pointAt(0.2), &window);
    QTRY_VERIFY(qAbs(slider->property("value").toReal() - 20) < 0.3);

    // A separate instance keeps the Segmented step binding intact.
    QScopedPointer<QObject> segmentedObject(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.Slider { x: 20; y: 80; width: 360; type: LV.Slider.Segmented; from: 0; to: 100; value: 50 }
)"));
    QVERIFY(segmentedObject);
    auto *segmented = qobject_cast<QQuickItem *>(segmentedObject.data());
    segmented->setParentItem(window.contentItem());
    const qreal start = segmented->property("leftPadding").toReal() + segmented->property("rangeInset").toReal();
    const QPoint target = segmented->mapToScene(QPointF(start + segmented->property("rangeWidth").toReal() * 0.64, 16)).toPoint();
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, target);
    QCOMPARE(segmented->property("value").toReal(), 75.0);
    segmented->forceActiveFocus();
    QTest::keyClick(&window, Qt::Key_Left);
    QCOMPARE(segmented->property("value").toReal(), 50.0);
    segmented->setProperty("segmentCount", 3);
    QCOMPARE(segmented->property("stepSize").toReal(), 50.0);
    QTest::keyClick(&window, Qt::Key_Right);
    QCOMPARE(segmented->property("value").toReal(), 100.0);
    segmented->setProperty("segmentCount", 0);
    QCOMPARE(segmented->property("stepSize").toReal(), 100.0);
}

void SliderTests::right_to_left_and_descending_ranges()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.Slider { width: 480; from: 10; to: -10; value: 5; LayoutMirroring.enabled: true }
)"));
    QVERIFY(object);
    auto *slider = qobject_cast<QQuickItem *>(object.data());
    auto *fill = slider->findChild<QQuickItem *>("slider_fill");
    auto *handle = slider->findChild<QQuickItem *>("slider_handle");
    QVERIFY(fill && handle);
    for (int type = 0; type < 7; ++type) {
        slider->setProperty("type", type);
        QCoreApplication::processEvents();
        QCOMPARE(slider->property("position").toReal(), 0.25);
        QCOMPARE(slider->property("visualPosition").toReal(), 0.75);
        const qreal left = slider->property("leftPadding").toReal();
        const qreal inset = slider->property("rangeInset").toReal();
        const qreal range = slider->property("rangeWidth").toReal();
        QVERIFY(qAbs(handle->x() + handle->width() / 2 - (left + inset + range * 0.75)) < 0.01);
        const bool centered = type == 1 || type == 3;
        QVERIFY(qAbs(fill->x() - (left + (type >= 4 ? 0 : inset) + (centered ? 0.5 : 0.75) * range)) < 0.01);
        QVERIFY(fill->x() + fill->width() <= slider->width() + 0.01);
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
    QVERIFY(object);
    auto *slider = qobject_cast<QQuickItem *>(object.data());
    slider->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *minimum = slider->findChild<QQuickItem *>("slider_minimum");
    auto *maximum = slider->findChild<QQuickItem *>("slider_maximum");
    auto *fill = slider->findChild<QQuickItem *>("slider_fill");
    QVERIFY(minimum && maximum && fill);
    QTRY_VERIFY(slider->property("availableWidth").toReal() >= 39.99);
    QCOMPARE(minimum->property("iconUrl").toUrl().toString(), slider->property("expectedMinimumIcon").toString());
    QVERIFY(minimum->x() + minimum->width() < fill->x());
    QVERIFY(maximum->x() + maximum->width() <= slider->width());
    const qreal padding = slider->property("leftPadding").toReal();
    slider->setVisible(false);
    QCoreApplication::processEvents();
    QCOMPARE(slider->property("leftPadding").toReal(), padding);
    QVERIFY(padding > 0); // Ancestor visibility must not change the layout contract.
    slider->setVisible(true);
    QCOMPARE(slider->property("leftPadding").toReal(), padding);
    slider->setProperty("showLabels", false);
    QTRY_COMPARE(minimum->width(), 18.0);
    slider->setProperty("showMinMax", false);
    QTRY_COMPARE(slider->property("availableWidth").toReal(), 180.0);
    slider->setProperty("active", false);
    QCOMPARE(fill->property("color").value<QColor>(), slider->property("expectedInactiveFill").value<QColor>());
    QVERIFY(slider->isEnabled());
    slider->setEnabled(false);
    const qreal value = slider->property("value").toReal();
    QSignalSpy moved(slider, SIGNAL(moved()));
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(145, 46));
    QTest::keyClick(&window, Qt::Key_Right);
    QCOMPARE(slider->property("value").toReal(), value);
    QCOMPARE(moved.count(), 0);
    QCOMPARE(slider->opacity(), 0.32);
}

void SliderTests::catalog_renders_all_designs()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(1100, 1000);
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
        QStringLiteral(LVRS_TEST_SOURCE_DIR) + "/../example/VisualCatalog/qml/SliderGallery.qml"));
    QVERIFY(object);
    auto *gallery = qobject_cast<QQuickItem *>(object.data());
    gallery->setWidth(1100);
    gallery->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_VERIFY(gallery->height() > 0);
    QList<QQuickItem *> variants;
    std::function<void(QQuickItem *)> collect = [&](QQuickItem *item) {
        if (item->objectName().startsWith("slider_variant_"))
            variants.append(item);
        for (auto *child : item->childItems())
            collect(child);
    };
    collect(gallery);
    QCOMPARE(variants.size(), 112);
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
        QTRY_VERIFY(variantsFit()); // Positioners finish their polish on the next native frame.
    }
    gallery->setWidth(1100);
    QTRY_VERIFY(variantsFit());
    QTest::qWait(200);
    const auto capture = gallery->grabToImage(QSize(1100, qCeil(gallery->height())));
    QVERIFY(capture);
    QSignalSpy ready(capture.data(), &QQuickItemGrabResult::ready);
    QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty(), 5000);
    const QImage pixels = capture->image();
    const qreal pixelRatio = pixels.width() / 1100.0;
    const QString directory = qEnvironmentVariable("LVRS_SLIDER_CAPTURE_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QVERIFY(capture->saveToFile(directory + "/slider-gallery.png"));
        const int previewHeight = qRound((gallery->property("examplesHeight").toReal() + 12) * pixelRatio);
        QVERIFY(pixels.copy(0, 0, pixels.width(), qMin(pixels.height(), previewHeight))
                    .save(directory + "/slider-examples.png"));
    }
    for (auto *variant : variants) {
        if (!variant->property("effectiveThumbVisible").toBool())
            continue;
        auto *handle = variant->findChild<QQuickItem *>("slider_handle");
        QVERIFY(handle);
        const QPointF center = handle->mapToItem(gallery, QPointF(handle->width() / 2, handle->height() / 2));
        const QColor pixel = pixels.pixelColor(qRound(center.x() * pixelRatio), qRound(center.y() * pixelRatio));
        QVERIFY2(pixel.red() > (variant->isEnabled() ? 215 : 75), qPrintable(variant->objectName()));
        // TitleHeader is 90% white: the blue capsule remains faintly visible through it.
        QVERIFY2(qAbs(pixel.red() - pixel.green()) < 35 && qAbs(pixel.red() - pixel.blue()) < 35,
                 qPrintable(variant->objectName() + " must render the translucent light thumb: " + pixel.name()));
    }
    int blue = 0, light = 0;
    for (int y = 0; y < pixels.height(); ++y)
        for (int x = 0; x < pixels.width(); ++x) {
            const QColor color = pixels.pixelColor(x, y);
            if (color.blue() > 220 && color.red() < 40 && color.green() > 100 && color.green() < 160)
                ++blue;
            if (color.red() > 200 && color.green() > 200 && color.blue() > 200 && color.alpha() > 200)
                ++light;
        }
    QVERIFY(blue > 5000 && light > 1000);
}

QTEST_MAIN(SliderTests)
#include "tst_slider.moc"
