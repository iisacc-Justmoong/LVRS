#include <QtTest>
#include <QtMath>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QSGRendererInterface>
#include "test_utils.h"
#include "accent_fixture.h"

namespace {
QColor accent5Mix(const QColor &base, const QColor &accent)
{
    return QColor(qRound((base.redF() * 0.95 + accent.redF() * 0.05) * 255),
                  qRound((base.greenF() * 0.95 + accent.greenF() * 0.05) * 255),
                  qRound((base.blueF() * 0.95 + accent.blueF() * 0.05) * 255));
}

struct GrainStats {
    qreal mean;
    qreal deviation;
    qreal changedNeighbors;
};

GrainStats grainStats(const QImage &image, const QRect &region)
{
    qint64 sum = 0, squares = 0, count = 0, changed = 0, neighbors = 0;
    for (int y = region.top(); y <= region.bottom(); ++y) {
        for (int x = region.left(); x <= region.right(); ++x) {
            const int value = image.pixelColor(x, y).green();
            sum += value;
            squares += value * value;
            ++count;
            if (x > region.left()) {
                changed += value != image.pixelColor(x - 1, y).green();
                ++neighbors;
            }
        }
    }
    const qreal mean = qreal(sum) / count;
    return {mean, qSqrt(qMax(qreal(0), qreal(squares) / count - mean * mean)), qreal(changed) / neighbors};
}
}

class MaterialTests : public QObject
{
    Q_OBJECT
private slots:
    void window_default_and_primary();
    void window_material_inherits_owning_window_accent();
    void application_window_theme_accent_mix();
    void application_window_custom_background();
    void noise_texture_inputs_are_bounded_and_opaque();
    void window_gradient_tracks_accent_and_resize();
    void transient_defaults_and_capture_safety();
    void backdrop_blur_and_color_rendering();
    void catalog_transient_interaction();
    void context_menu_frost_contract();
    void context_menu_frost_rendering();
};

void MaterialTests::window_default_and_primary()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    width: 900; height: 600; visible: false
    LV.LabelButton { objectName: "accentButton"; tone: LV.AbstractButton.Primary; text: "Continue" }
})"));
    QVERIFY(root);
    auto *background = qobject_cast<QQuickItem *>(root->property("background").value<QObject *>());
    auto *button = root->findChild<QObject *>("accentButton");
    QVERIFY(background && button);
    auto *theme = engine.singletonInstance<QObject *>("LVRS", "Theme");
    QVERIFY(theme);
    const QColor windowBase = theme->property("window").value<QColor>();
    QCOMPARE(windowBase, QColor("#141414"));
    QCOMPARE(theme->property("applicationWindowFill").value<QColor>(), windowBase);
    QCOMPARE(root->property("windowColor").value<QColor>(), windowBase);
    QVERIFY(!root->findChild<QObject *>("applicationWindowGlassOverlay"));
    auto *grain = background->findChild<QQuickItem *>("applicationWindowNoiseTexture");
    QVERIFY(grain);
    QCOMPARE(grain->property("noiseStrength").toReal(), 0.0);
    QCOMPARE(grain->property("blurRadius").toReal(), 0.0);
    QCOMPARE(grain->property("grainStrength").toReal(), 2.0 / 255);
    const QColor blue = AccentFixture::color("accentBlue");
    QCOMPARE(background->property("primaryColor").value<QColor>(), blue);
    QCOMPARE(background->property("color").value<QColor>().rgba(), accent5Mix(windowBase, blue).rgba());
    QVERIFY(root->setProperty("primaryColor", QColor("#A571E6")));
    QTRY_COMPARE(background->property("primaryColor").value<QColor>(), QColor("#A571E6"));
    QCOMPARE(background->property("color").value<QColor>().rgba(), accent5Mix(windowBase, QColor("#A571E6")).rgba());
    QTRY_COMPARE(button->property("backgroundColor").value<QColor>(), QColor("#A571E6"));
    QCOMPARE(background->opacity(), 1.0);
    QCOMPARE(qobject_cast<QQuickWindow *>(root.data())->opacity(), 1.0);
    QVERIFY(root->setProperty("windowBackgroundOpacity", 0.35));
    QCOMPARE(background->opacity(), 1.0);
    QVERIFY(root->setProperty("windowColor", QColor("#202124")));
    QCOMPARE(background->property("color").value<QColor>().rgba(), accent5Mix(QColor("#202124"), QColor("#A571E6")).rgba());
}

void MaterialTests::window_material_inherits_owning_window_accent()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    const auto createWindow = [&](const char *accent) {
        return TestUtils::createFromQml(engine, QByteArray(R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    property color primaryColor: ")") + accent + R"("
    width: 640; height: 400; visible: true
    LV.WindowMaterial { objectName: "surface"; anchors.fill: parent }
})");
    };
    QScopedPointer<QObject> blue(createWindow("#0A84FF"));
    QScopedPointer<QObject> green(createWindow("#57965C"));
    QVERIFY(blue && green);
    auto *blueMaterial = blue->findChild<QObject *>("surface");
    auto *greenMaterial = green->findChild<QObject *>("surface");
    QVERIFY(blueMaterial && greenMaterial);
    QTRY_COMPARE(blueMaterial->property("primaryColor").value<QColor>(), QColor("#0A84FF"));
    QTRY_COMPARE(greenMaterial->property("primaryColor").value<QColor>(), QColor("#57965C"));
    auto *theme = engine.singletonInstance<QObject *>("LVRS", "Theme");
    QVERIFY(theme);
    QVERIFY(theme->setProperty("primaryColor", QColor("#FF453A")));
    QCOMPARE(blueMaterial->property("primaryColor").value<QColor>(), QColor("#0A84FF"));
    QCOMPARE(greenMaterial->property("primaryColor").value<QColor>(), QColor("#57965C"));
    QVERIFY(blue->setProperty("primaryColor", QColor("#A571E6")));
    QTRY_COMPARE(blueMaterial->property("primaryColor").value<QColor>(), QColor("#A571E6"));
    QCOMPARE(greenMaterial->property("primaryColor").value<QColor>(), QColor("#57965C"));
}

void MaterialTests::application_window_theme_accent_mix()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    width: 900; height: 600; visible: true
    primaryColor: LV.Theme.defaultPrimary
    LV.ApplicationWindow { objectName: "auxiliary"; width: 360; height: 240; visible: false }
    Text { x: 40; y: 54; text: "ApplicationWindow"; color: "white"; font.pixelSize: 26 }
    Text { x: 40; y: 96; text: "Theme.window 95% + app accent 5% / fine grain"; color: "#B8C2CC"; font.pixelSize: 13 }
    LV.LabelButton { x: 40; y: 140; text: "Continue"; tone: LV.AbstractButton.Primary }
    Rectangle { objectName: "foregroundProof"; x: 300; y: 220; width: 48; height: 48; color: "#F0A060" }
})"));
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window && QTest::qWaitForWindowExposed(window));
    auto *material = qobject_cast<QQuickItem *>(root->property("background").value<QObject *>());
    auto *auxiliary = root->findChild<QQuickWindow *>("auxiliary");
    auto *foreground = root->findChild<QQuickItem *>("foregroundProof");
    QVERIFY(material && auxiliary && foreground);
    auto *auxiliaryMaterial = qobject_cast<QQuickItem *>(auxiliary->property("background").value<QObject *>());
    QVERIFY(auxiliaryMaterial);
    QVERIFY(!root->findChild<QObject *>("applicationWindowGlassOverlay"));
    auto *grain = material->findChild<QQuickItem *>("applicationWindowNoiseTexture");
    auto *auxiliaryGrain = auxiliaryMaterial->findChild<QQuickItem *>("applicationWindowNoiseTexture");
    QVERIFY(grain && auxiliaryGrain);
    QCOMPARE(grain->property("noiseStrength").toReal(), 0.0);
    QCOMPARE(grain->property("blurRadius").toReal(), 0.0);
    QCOMPARE(grain->property("grainStrength").toReal(), 2.0 / 255);
    QVERIFY(!window->property("backgroundBlurSupported").toBool());
    const auto matches = [](const QColor &actual, const QColor &expected) {
        return actual.alpha() == 255 && qAbs(actual.red() - expected.red()) <= 2
            && qAbs(actual.green() - expected.green()) <= 2 && qAbs(actual.blue() - expected.blue()) <= 2;
    };
    const QString directory = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_DIR");
    QHash<QRgb, QImage> originalTiles;
    for (const QSize size : {QSize(900, 600), QSize(1600, 1000)}) {
        window->resize(size);
        QTRY_COMPARE(material->size(), QSizeF(size));
        QTRY_COMPARE(grain->size(), QSizeF(size));
        for (const QColor accent : {QColor("#0A84FF"), QColor("#66FF48"), QColor("#A571E6"), QColor(Qt::white), QColor(Qt::black)}) {
            QVERIFY(window->setProperty("primaryColor", accent));
            QTRY_COMPARE(material->property("primaryColor").value<QColor>(), accent);
            QTRY_COMPARE(auxiliaryMaterial->property("primaryColor").value<QColor>(), accent);
            const QColor expected = accent5Mix(QColor("#141414"), accent);
            QCOMPARE(material->property("color").value<QColor>().rgba(), expected.rgba());
            QTRY_COMPARE(grain->property("color").value<QColor>().rgba(), expected.rgba());
            QTRY_COMPARE(auxiliaryGrain->property("color").value<QColor>().rgba(), expected.rgba());
            auto capture = material->grabToImage();
            QVERIFY(capture);
            QTRY_VERIFY(!capture->image().isNull());
            const QImage background = capture->image();
            for (int y = 0; y < background.height(); ++y)
                for (int x = 0; x < background.width(); ++x)
                    QVERIFY2(matches(background.pixelColor(x, y), expected), "Fine grain must stay within two RGB levels of the opaque 95:5 mix.");
            const auto stats = grainStats(background, QRect(16, 16, 256, 256));
            QVERIFY(qAbs(stats.mean - expected.green()) < 0.65);
            QVERIFY2(stats.deviation > 0.4 && stats.deviation < 1.5, "Grain must be perceptible without strong contrast.");
            const qreal minimumChanges = expected.green() == 0 ? 0.45 : 0.65;
            QVERIFY2(stats.changedNeighbors > minimumChanges, "Grain must vary at individual physical pixels, including on Retina displays.");
            const QImage tile = background.copy(0, 0, 128, 128);
            if (originalTiles.contains(accent.rgb()))
                QCOMPARE(tile, originalTiles.value(accent.rgb()));
            else
                originalTiles.insert(accent.rgb(), tile);
            const QImage frame = window->grabWindow();
            QVERIFY(!frame.isNull());
            QVERIFY(matches(frame.pixelColor(500 * frame.width() / size.width(), 400 * frame.height() / size.height()), expected));
            const qreal scale = frame.width() / qreal(size.width());
            const auto frameStats = grainStats(frame, QRect(qRound(500 * scale), qRound(350 * scale), qRound(128 * scale), qRound(128 * scale)));
            QVERIFY(frameStats.deviation > 0.4 && frameStats.deviation < 1.5);
            QVERIFY(frameStats.changedNeighbors > minimumChanges);
            const QPointF position = foreground->mapToScene(QPointF(24, 24));
            QCOMPARE(frame.pixelColor(qRound(position.x() * frame.width() / window->width()),
                                      qRound(position.y() * frame.height() / window->height())), QColor("#F0A060"));
            if (!directory.isEmpty()) {
                QVERIFY(QDir().mkpath(directory));
                QVERIFY(frame.save(directory + "/app-accent5-" + accent.name().mid(1) + "-" + QString::number(size.width()) + ".png"));
                if (accent == QColor("#0A84FF") && size.width() == 900)
                    QVERIFY(frame.copy(qRound(500 * scale), qRound(350 * scale), qRound(256 * scale), qRound(192 * scale)).save(directory + "/fine-grain-detail.png"));
            }
            for (const bool enabled : {false, true}) {
                QVERIFY(window->setProperty("backgroundBlurEnabled", enabled));
                QVERIFY(!window->property("backgroundBlurActive").toBool());
                QCOMPARE(material->property("color").value<QColor>().rgba(), expected.rgba());
            }
            QCOMPARE(window->color(), QColor("#141414"));
            QCOMPARE(window->opacity(), 1.0);
        }
    }
    QVERIFY(window->setProperty("windowBackgroundOpacity", 0.05));
    QVERIFY(window->setProperty("windowColor", QColor("#33202124")));
    QVERIFY(window->setProperty("primaryColor", QColor("#00A571E6")));
    QTRY_COMPARE(window->color(), QColor("#202124"));
    const QColor expected = accent5Mix(QColor("#202124"), QColor("#A571E6"));
    QCOMPARE(material->property("color").value<QColor>().rgba(), expected.rgba());
    auto capture = material->grabToImage();
    QVERIFY(capture);
    QTRY_VERIFY(!capture->image().isNull());
    QVERIFY(matches(capture->image().pixelColor(64, 64), expected));
    const QImage original = capture->image();
    QVERIFY(grain->setProperty("grainStrength", 0.0));
    capture = material->grabToImage();
    QVERIFY(capture);
    QTRY_VERIFY(!capture->image().isNull());
    const auto flat = grainStats(capture->image(), QRect(16, 16, 256, 256));
    QCOMPARE(flat.deviation, 0.0);
    QCOMPARE(flat.mean, qreal(expected.green()));
    QVERIFY(grain->setProperty("grainStrength", 2.0 / 255));
    capture = material->grabToImage();
    QVERIFY(capture);
    QTRY_VERIFY(!capture->image().isNull());
    QCOMPARE(capture->image(), original); // A fixed pattern survives toggles and resizing.
}

void MaterialTests::application_window_custom_background()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    visible: false
    background: Rectangle { objectName: "customBackground"; color: "#334455" }
})"));
    QVERIFY(root);
    auto *custom = root->property("background").value<QObject *>();
    QVERIFY(custom);
    QCOMPARE(custom->objectName(), QStringLiteral("customBackground"));
    QVERIFY(!root->property("backgroundBlurSupported").toBool());
    QVERIFY(!root->property("backgroundBlurActive").toBool());
    QCOMPARE(qobject_cast<QQuickWindow *>(root.data())->color().alpha(), 255);
}

void MaterialTests::noise_texture_inputs_are_bounded_and_opaque()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import LVRS as LV
LV.WindowNoiseTexture { color: "#33123456"; primaryColor: "#00A571E6" }
)"));
    QVERIFY(root);
    QCOMPARE(root->property("color").value<QColor>(), QColor("#123456"));
    QCOMPARE(root->property("primaryColor").value<QColor>(), QColor("#A571E6"));
    QVERIFY(root->setProperty("noiseStrength", -1.0));
    QCOMPARE(root->property("noiseStrength").toReal(), 0.0);
    QVERIFY(root->setProperty("noiseStrength", 2.0));
    QCOMPARE(root->property("noiseStrength").toReal(), 1.0);
    QVERIFY(root->setProperty("blurRadius", -1.0));
    QCOMPARE(root->property("blurRadius").toReal(), 0.0);
    QVERIFY(root->setProperty("blurRadius", 20.0));
    QCOMPARE(root->property("blurRadius").toReal(), 20.0);
    QVERIFY(root->setProperty("blurRadius", 80.0));
    QCOMPARE(root->property("blurRadius").toReal(), 50.0);
    QVERIFY(root->setProperty("blurRadius", qQNaN()));
    QCOMPARE(root->property("blurRadius").toReal(), 50.0);
    QVERIFY(root->setProperty("blurRadius", 0.0));
    QVERIFY(root->setProperty("grainStrength", -1.0));
    QCOMPARE(root->property("grainStrength").toReal(), 0.0);
    QVERIFY(root->setProperty("grainStrength", 2.0));
    QCOMPARE(root->property("grainStrength").toReal(), 1.0);
    QVERIFY(root->setProperty("grainStrength", qQNaN()));
    QCOMPARE(root->property("grainStrength").toReal(), 1.0);
}

void MaterialTests::window_gradient_tracks_accent_and_resize()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import QtQuick.Controls
import LVRS as LV
ApplicationWindow {
    width: 900; height: 600; visible: true; color: "#141414"
    LV.WindowMaterial { objectName: "surface"; anchors.fill: parent; radius: 0; borderWidth: 0; shadowOpacity: 0; motionEnabled: false }
})"));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *surface = root->findChild<QQuickItem *>("surface");
    QVERIFY(window && surface);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QCOMPARE(surface->property("tintOpacity").toReal(), 0.5);
    const QColor windowFill("#0B0B0B");
    const QString directory = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_DIR");
    for (const QSize size : {QSize(900, 600), QSize(1600, 1000)}) {
        window->resize(size);
        QTRY_COMPARE(surface->size(), QSizeF(size));
        for (const QColor accent : {QColor("#0A84FF"), QColor("#A571E6")}) {
            QVERIFY(surface->setProperty("primaryColor", accent));
            QCOMPARE(surface->property("color").value<QColor>(), windowFill);
            QTRY_COMPARE(surface->property("displayedPrimaryColor").value<QColor>(), accent);
            auto capture = surface->grabToImage();
            QVERIFY(capture);
            QTRY_VERIFY(!capture->image().isNull());
            const QImage image = capture->image();
            const auto pixelAt = [&](int x, int y) {
                return image.pixelColor(x * image.width() / size.width(), y * image.height() / size.height());
            };
            const QColor strong = pixelAt(48, 32);
            const QColor quiet = pixelAt(size.width() / 2, 48);
            QVERIFY2(strong.blue() > quiet.blue() + 8 && strong.blue() > strong.green() + 8,
                     qPrintable(QString("The window must render an accent gradient, not a uniform tint: strong %1, quiet %2")
                         .arg(strong.name(QColor::HexArgb), quiet.name(QColor::HexArgb))));
            if (accent.red() > accent.green())
                QVERIFY(strong.red() > strong.green() + 8);
            QVERIFY2(quiet.alpha() >= 126 && quiet.alpha() < 160 && strong.alpha() < 220,
                     "The gradient must retain a translucent background.");
            const QColor right = pixelAt(size.width() - 12, size.height() / 2);
            QVERIFY2(right.blue() > quiet.blue() + 8,
                     "The accent at the right edge must follow the resized window.");
            if (!directory.isEmpty()) {
                QVERIFY(QDir().mkpath(directory));
                QVERIFY(window->grabWindow().save(directory + QString("/window-gradient-%1-%2.png")
                    .arg(accent.name().mid(1)).arg(size.width())));
            }
        }
    }
}

void MaterialTests::transient_defaults_and_capture_safety()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    width: 900; height: 600; visible: true; primaryColor: "#A571E6"
    LV.ContextMenu { objectName: "menu"; items: [{label: "Open"}]; enableOpenBounce: false }
    LV.Tooltip { objectName: "tip"; automatic: false; anchorPoint: Qt.point(220, 180); text: "Help"; delay: 0; animationDuration: 0 }
    LV.Popover { objectName: "popover"; width: 240; height: 140; x: 80; y: 80 }
    Item { id: ancestor; objectName: "ancestor"; LV.PanelMaterial { objectName: "unsafe"; width: 40; height: 40; backdropSource: ancestor } }
})"));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    for (const char *name : {"menu", "tip", "popover"}) {
        auto *popup = root->findChild<QObject *>(name);
        QVERIFY(popup);
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTRY_VERIFY2(popup->property("opened").toBool(), name);
        auto *background = qobject_cast<QQuickItem *>(popup->property("background").value<QObject *>());
        QVERIFY(background);
        const bool isMenu = qstrcmp(name, "menu") == 0;
        QCOMPARE(background->property("tintOpacity").toReal(), isMenu ? 0.12 : 0.25);
        QCOMPARE(background->property("blurRadius").toReal(), isMenu ? 64.0 : 16.0);
        QCOMPARE(background->property("primaryColor").value<QColor>(), QColor("#A571E6"));
        QCOMPARE(background->opacity(), 1.0);
        if (background->property("rendererAvailable").toBool())
            QTRY_VERIFY2(background->property("captureActive").toBool(), name);
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTRY_VERIFY(!popup->property("visible").toBool());
    }
    auto *unsafe = root->findChild<QObject *>("unsafe");
    QVERIFY(unsafe);
    QVERIFY(!unsafe->property("resolvedBackdropSource").value<QObject *>());
    QVERIFY(unsafe->setProperty("backdropSource", QVariant::fromValue(qobject_cast<QQuickItem *>(unsafe))));
    QVERIFY(!unsafe->property("resolvedBackdropSource").value<QObject *>());
}

void MaterialTests::backdrop_blur_and_color_rendering()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    width: 640; height: 400; visible: true; color: "#141414"
    Item { id: pattern; anchors.fill: parent
        Repeater { model: 160; Rectangle { required property int index; x: index * 4; width: 2; height: 400; color: "white" } }
    }
    LV.PanelMaterial { id: material; objectName: "material"; x: 100; y: 80; width: 380; height: 204; backdropSource: pattern }
})"));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *material = root->findChild<QQuickItem *>("material");
    QVERIFY(window && material);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    if (window->rendererInterface()->graphicsApi() == QSGRendererInterface::Software)
        QSKIP("Backdrop blur requires a native RHI renderer; software keeps the translucent radial fallback.");
    QTRY_VERIFY(material->property("effectsActive").toBool());
    QTRY_VERIFY(material->property("captureActive").toBool());
    auto capture = material->grabToImage();
    QVERIFY(capture);
    QTRY_VERIFY(!capture->image().isNull());
    const QImage blurred = capture->image();
    QVERIFY(material->setProperty("blurEnabled", false));
    QTest::qWait(80);
    capture = material->grabToImage();
    QTRY_VERIFY(!capture->image().isNull());
    const QImage sharp = capture->image();
    auto variation = [](const QImage &image) {
        qint64 sum = 0;
        for (int y = image.height()/4; y < image.height()*3/4; ++y)
            for (int x = image.width()/4 + 1; x < image.width()*3/4; ++x)
                sum += qAbs(qGray(image.pixel(x,y)) - qGray(image.pixel(x-1,y)));
        return sum;
    };
    QVERIFY2(variation(blurred) * 2 < variation(sharp), "Glass must actually soften the captured backdrop.");
    QVERIFY(material->setProperty("backdropSource", QVariant::fromValue<QQuickItem *>(nullptr)));
    QVERIFY(material->setProperty("primaryColor", QColor("#A571E6")));
    QTRY_COMPARE(material->property("displayedPrimaryColor").value<QColor>(), QColor("#A571E6"));
    capture = material->grabToImage();
    QTRY_VERIFY(!capture->image().isNull());
    const QColor purple = capture->image().pixelColor(capture->image().width()*61/380, capture->image().height()*74/204);
    QVERIFY2(purple.blue() > purple.green() && purple.red() > purple.green(), "Primary must tint the rendered radial gradient.");
    const QString directory = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QVERIFY(window->grabWindow().save(directory + "/materials-native.png"));
    }
}

void MaterialTests::catalog_transient_interaction()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::loadQmlFile(engine,
        QFINDTESTDATA("../example/VisualCatalog/qml/Main.qml")));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->resize(1100, 900);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry",
        Q_ARG(QVariant, QStringLiteral("window-material"))));
    QTRY_VERIFY(root->findChild<QObject *>("galleryMaterialPopover"));
    const QString directory = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QTest::qWait(160);
        QVERIFY(window->grabWindow().save(directory + "/material-gallery-blue.png"));
    }
    for (const char *name : {"galleryMaterialMenu", "galleryMaterialTooltip", "galleryMaterialPopover"}) {
        auto *popup = root->findChild<QObject *>(name);
        QVERIFY(popup);
        QSignalSpy opening(popup, SIGNAL(aboutToShow()));
        QSignalSpy closing(popup, SIGNAL(aboutToHide()));
        const auto popupState = [&] {
            return QStringLiteral("%1 visible=%2 opacity=%3 opened=%4 opening=%5 closing=%6 windowActive=%7 exposed=%8")
                .arg(QString::fromLatin1(name))
                .arg(popup->property("visible").toBool())
                .arg(popup->property("opacity").toReal())
                .arg(popup->property("opened").toBool())
                .arg(opening.count()).arg(closing.count())
                .arg(window->isActive()).arg(window->isExposed());
        };
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTRY_VERIFY2(popup->property("opened").toBool(), qPrintable(popupState()));
        auto *background = qobject_cast<QQuickItem *>(popup->property("background").value<QObject *>());
        QVERIFY(background);
        QCOMPARE(background->property("tintOpacity").toReal(), qstrcmp(name, "galleryMaterialMenu") == 0 ? 0.12 : 0.25);
        if (!directory.isEmpty()) {
            QTest::qWait(160);
            QVERIFY(window->grabWindow().save(directory + "/" + name + ".png"));
        }
        QVERIFY(QMetaObject::invokeMethod(popup, "close"));
        QTRY_VERIFY(!popup->property("visible").toBool());
    }
    QVERIFY(root->setProperty("primaryColor", QColor("#A571E6")));
    auto *background = root->findChild<QObject *>("applicationWindowMaterial");
    QVERIFY(background);
    QTRY_COMPARE(background->property("primaryColor").value<QColor>(), QColor("#A571E6"));
    if (!directory.isEmpty()) {
        QTest::qWait(160);
        QVERIFY(window->grabWindow().save(directory + "/material-gallery-purple.png"));
    }
}

void MaterialTests::context_menu_frost_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    width: 640; height: 520; visible: true; windowColor: "#20242a"
    LV.ContextMenu { objectName: "frostMenu"; items: ["Open", "Duplicate"]; motionEnabled: false }
    LV.Menu { objectName: "regularMenu"; items: ["Open", "Duplicate"]; motionEnabled: false }
})"));
    QVERIFY(root);
    auto *windowBackground = root->property("background").value<QObject *>();
    QVERIFY(windowBackground);
    for (const char *name : {"frostMenu", "regularMenu"}) {
        auto *menu = root->findChild<QObject *>(name);
        QVERIFY(menu);
        auto *material = menu->property("background").value<QObject *>();
        QVERIFY(material);
        QCOMPARE(menu->property("backdropSource").value<QObject *>(),
                 root->property("materialBackdropSource").value<QObject *>());
        QCOMPARE(menu->property("menuOpacity").toReal(), 0.12);
        QCOMPARE(material->property("tintOpacity").toReal(), 0.12);
        QCOMPARE(material->property("blurRadius").toReal(), 64.0);
        QCOMPARE(material->property("intenseOpacity").toReal(), 0.032);
        QCOMPARE(material->property("faintOpacity").toReal(), 0.0088);
        QCOMPARE(material->property("shadowRadius").toReal(), 48.0);
        QCOMPARE(menu->property("backdropBackground").value<QObject *>(), windowBackground);
        QCOMPARE(material->property("color").value<QColor>(), windowBackground->property("color").value<QColor>());
        const QColor captureBase = material->property("backdropBaseColor").value<QColor>();
        QCOMPARE(captureBase.alpha(), 255);
        QCOMPARE(captureBase.red(), captureBase.green());
        QCOMPARE(captureBase.green(), captureBase.blue());
        QCOMPARE(menu->property("opacity").toReal(), 1.0);
        QVERIFY(menu->setProperty("menuOpacity", 0.2));
        QVERIFY(menu->setProperty("menuBlurRadius", 32.0));
        QVERIFY(menu->setProperty("menuAccentStrength", 0.5));
        QCOMPARE(material->property("tintOpacity").toReal(), 0.2);
        QCOMPARE(material->property("blurRadius").toReal(), 32.0);
        QCOMPARE(material->property("intenseOpacity").toReal(), 0.2);
        QCOMPARE(material->property("faintOpacity").toReal(), 0.055);
    }
}

void MaterialTests::context_menu_frost_rendering()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    width: 720; height: 620; visible: true; windowColor: "#141414"
    LV.Label { x: 50; y: 24; text: "Window material behind a frosted menu"; style: header }
    Item {
        id: pattern; objectName: "frostPattern"; x: 0; y: 180; width: 720; height: 100
        Repeater {
            model: 90
            Rectangle { required property int index; x: index * 8; width: 4; height: 100; color: "#dddddd" }
        }
    }
    LV.ContextMenu {
        objectName: "frostMenu"; x: 80; y: 75; itemWidth: 200; motionEnabled: false
        showIconSlot: false
        items: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "15", "20"]
        selectedIndex: 1
    }
})"));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *menu = root->findChild<QObject *>("frostMenu");
    auto *material = qobject_cast<QQuickItem *>(menu->property("background").value<QObject *>());
    QVERIFY(window && menu && material);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(QMetaObject::invokeMethod(menu, "openAt", Q_ARG(QVariant, 80), Q_ARG(QVariant, 75)));
    QTRY_VERIFY(menu->property("opened").toBool());
    if (!material->property("rendererAvailable").toBool())
        QSKIP("Frost sampling requires a native RHI renderer.");
    QTRY_VERIFY(material->property("captureActive").toBool());
    QVERIFY2(material->property("resolvedBackdropSource").value<QObject *>(),
             "App content, not only its window background, must be captured.");
    const auto capture = [&]() {
        auto request = material->grabToImage();
        QTest::qWait(180);
        return request->image();
    };
    const auto blueExcess = [](const QImage &image) {
        qreal sum = 0;
        // Bottom right contains no foreground label or stripe pattern.
        for (int y = image.height() * 4 / 5; y < image.height() * 9 / 10; ++y)
            for (int x = image.width() / 2; x < image.width() * 4 / 5; ++x) {
                const QColor pixel = image.pixelColor(x, y);
                sum += qMax(0, pixel.blue() - pixel.red());
            }
        return sum;
    };
    const QString directory = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_DIR");
    QVERIFY(menu->setProperty("menuOpacity", 0.25));
    QVERIFY(menu->setProperty("menuBlurRadius", 16.0));
    QVERIFY(menu->setProperty("menuAccentStrength", 1.0));
    QTest::qWait(200);
    const auto previous = capture();
    QVERIFY(!previous.isNull());
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QVERIFY(window->grabWindow().save(directory + "/menu-before.png"));
    }
    menu->setProperty("menuOpacity", 0.12);
    menu->setProperty("menuBlurRadius", 64.0);
    menu->setProperty("menuAccentStrength", 0.08);
    QTest::qWait(200);
    const auto frosted = capture();
    QVERIFY(!frosted.isNull());
    QVERIFY2(blueExcess(frosted) < blueExcess(previous) * 0.6,
        "The menu must inherit the window backdrop without another saturated blue layer.");
    if (!directory.isEmpty())
        QVERIFY(window->grabWindow().save(directory + "/menu-after.png"));
    // A larger blur must suppress variation of a stripe pattern behind the menu.
    const auto variation = [](const QImage &image) {
        qint64 sum = 0;
        for (int y = image.height() / 4; y < image.height() * 3 / 4; ++y)
            for (int x = image.width() / 4 + 1; x < image.width() * 3 / 4; ++x)
                sum += qAbs(qGray(image.pixel(x, y)) - qGray(image.pixel(x - 1, y)));
        return sum;
    };
    auto *pattern = root->findChild<QQuickItem *>("frostPattern");
    QVERIFY(pattern);
    const auto region = pattern->mapRectToItem(nullptr, pattern->boundingRect()).intersected(
        material->mapRectToItem(nullptr, material->boundingRect().adjusted(35, 25, -35, -25)));
    QVERIFY(!region.isEmpty());
    const auto visibleBackdrop = [&]() {
        const QImage frame = window->grabWindow();
        const qreal ratio = qreal(frame.width()) / window->width();
        return frame.copy(QRect(qRound(region.x() * ratio), qRound(region.y() * ratio),
                                qRound(region.width() * ratio), qRound(region.height() * ratio)));
    };
    const QImage visibleFrost = visibleBackdrop();
    material->setProperty("blurEnabled", false);
    QTest::qWait(100);
    const auto sharp = visibleBackdrop();
    QVERIFY(!sharp.isNull());
    QVERIFY2(variation(visibleFrost) * 4 < variation(sharp),
        qPrintable(QString("Visible background variation: frosted %1, sharp %2")
                   .arg(variation(visibleFrost)).arg(variation(sharp))));
    QCOMPARE(menu->property("opacity").toReal(), 1.0);
    QVERIFY(QMetaObject::invokeMethod(menu, "close"));
    QTRY_VERIFY(!menu->property("visible").toBool());
}

QTEST_MAIN(MaterialTests)
#include "tst_materials.moc"
