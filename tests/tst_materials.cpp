#include "QtEventWait.h"
#include <QEventLoop>
#include <QGuiApplication>
#include <QScopeGuard>
#include <QTimer>
#include <type_traits>
#include <QtMath>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QSGRendererInterface>
#include "test_utils.h"
#include "accent_fixture.h"

namespace {
QColor accent2Mix(const QColor &base, const QColor &accent)
{
    return QColor(qRound((base.redF() * 0.98 + accent.redF() * 0.02) * 255),
                  qRound((base.greenF() * 0.98 + accent.greenF() * 0.02) * 255),
                  qRound((base.blueF() * 0.98 + accent.blueF() * 0.02) * 255));
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

class MaterialPopupRecorder : public QObject
{
    Q_OBJECT
public:
    int openingCount = 0;
    int closingCount = 0;
public slots:
    void recordOpening() { ++openingCount; }
    void recordClosing() { ++closingCount; }
};

namespace {
template <typename Actual, typename Expected>
bool materialEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual), qreal(expected));
    else
        return actual == expected;
}

void awaitCapture(const QSharedPointer<QQuickItemGrabResult> &capture)
{
    CTEST_CHECK(capture);
    bool ready = false;
    const auto captureConnection = QObject::connect(capture.data(), &QQuickItemGrabResult::ready,
                                                      capture.data(), [&ready] { ready = true; });
    CTEST_CHECK(captureConnection);
    const auto disconnectCapture = qScopeGuard([&] { QObject::disconnect(captureConnection); });
    CTEST_WAIT_DEFAULT(ready);
    CTEST_CHECK(!capture->image().isNull());
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
}

#define MATERIAL_COMPARE(actual, expected) CTEST_CHECK(materialEqual((actual), (expected)))
#define MATERIAL_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(materialEqual((actual), (expected)))
#define MATERIAL_WAIT_MESSAGE(expression, message) \
    do { if (!lvrs_ctest::waitFor([&] { return bool(expression); }, 5000)) \
        lvrs_ctest::fail(__FILE__, __LINE__, (message)); } while (false)

class MaterialTests
{
public:
    void window_default_and_primary();
    void window_material_inherits_owning_window_accent();
    void application_window_theme_accent_mix();
    void application_window_custom_background();
    void noise_texture_defaults_match_figma();
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
    CTEST_CHECK(root);
    auto *background = qobject_cast<QQuickItem *>(root->property("background").value<QObject *>());
    auto *button = root->findChild<QObject *>("accentButton");
    CTEST_CHECK(background && button);
    auto *theme = engine.singletonInstance<QObject *>("LVRS", "Theme");
    CTEST_CHECK(theme);
    const QColor windowBase = theme->property("window").value<QColor>();
    MATERIAL_COMPARE(windowBase, AccentFixture::background("Panel/window"));
    MATERIAL_COMPARE(theme->property("applicationWindowFill").value<QColor>(), windowBase);
    MATERIAL_COMPARE(theme->property("applicationWindowAccentOpacity").toReal(), 0.02);
    MATERIAL_COMPARE(root->property("windowColor").value<QColor>(), windowBase);
    CTEST_CHECK(!root->findChild<QObject *>("applicationWindowGlassOverlay"));
    auto *grain = background->findChild<QQuickItem *>("applicationWindowNoiseTexture");
    CTEST_CHECK(grain);
    MATERIAL_COMPARE(grain->property("noiseStrength").toReal(), 0.0);
    MATERIAL_COMPARE(grain->property("blurRadius").toReal(), 0.0);
    MATERIAL_COMPARE(grain->property("grainStrength").toReal(), 2.0 / 255);
    const QColor blue = AccentFixture::color("accentBlue");
    MATERIAL_COMPARE(background->property("primaryColor").value<QColor>(), blue);
    MATERIAL_COMPARE(background->property("color").value<QColor>().rgba(), accent2Mix(windowBase, blue).rgba());
    CTEST_CHECK(root->setProperty("primaryColor", QColor("#A571E6")));
    MATERIAL_WAIT_COMPARE(background->property("primaryColor").value<QColor>(), QColor("#A571E6"));
    MATERIAL_COMPARE(background->property("color").value<QColor>().rgba(), accent2Mix(windowBase, QColor("#A571E6")).rgba());
    MATERIAL_WAIT_COMPARE(button->property("backgroundColor").value<QColor>(), QColor("#A571E6"));
    MATERIAL_COMPARE(background->opacity(), 1.0);
    MATERIAL_COMPARE(qobject_cast<QQuickWindow *>(root.data())->opacity(), 1.0);
    CTEST_CHECK(root->setProperty("windowBackgroundOpacity", 0.35));
    MATERIAL_COMPARE(background->opacity(), 1.0);
    CTEST_CHECK(root->setProperty("windowColor", QColor("#202124")));
    MATERIAL_COMPARE(background->property("color").value<QColor>().rgba(), accent2Mix(QColor("#202124"), QColor("#A571E6")).rgba());
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
    CTEST_CHECK(blue && green);
    auto *blueMaterial = blue->findChild<QObject *>("surface");
    auto *greenMaterial = green->findChild<QObject *>("surface");
    CTEST_CHECK(blueMaterial && greenMaterial);
    MATERIAL_WAIT_COMPARE(blueMaterial->property("primaryColor").value<QColor>(), QColor("#0A84FF"));
    MATERIAL_WAIT_COMPARE(greenMaterial->property("primaryColor").value<QColor>(), QColor("#57965C"));
    auto *theme = engine.singletonInstance<QObject *>("LVRS", "Theme");
    CTEST_CHECK(theme);
    CTEST_CHECK(theme->setProperty("primaryColor", QColor("#FF453A")));
    MATERIAL_COMPARE(blueMaterial->property("primaryColor").value<QColor>(), QColor("#0A84FF"));
    MATERIAL_COMPARE(greenMaterial->property("primaryColor").value<QColor>(), QColor("#57965C"));
    CTEST_CHECK(blue->setProperty("primaryColor", QColor("#A571E6")));
    MATERIAL_WAIT_COMPARE(blueMaterial->property("primaryColor").value<QColor>(), QColor("#A571E6"));
    MATERIAL_COMPARE(greenMaterial->property("primaryColor").value<QColor>(), QColor("#57965C"));
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
    Text { x: 40; y: 96; text: "Theme.window 98% + app accent 2% / fine grain"; color: "#B8C2CC"; font.pixelSize: 13 }
    LV.LabelButton { x: 40; y: 140; text: "Continue"; tone: LV.AbstractButton.Primary }
    Rectangle { objectName: "foregroundProof"; x: 300; y: 220; width: 48; height: 48; color: "#F0A060" }
})"));
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window); CTEST_WAIT_DEFAULT(window->isExposed());
    auto *material = qobject_cast<QQuickItem *>(root->property("background").value<QObject *>());
    auto *auxiliary = root->findChild<QQuickWindow *>("auxiliary");
    auto *foreground = root->findChild<QQuickItem *>("foregroundProof");
    CTEST_CHECK(material && auxiliary && foreground);
    auto *auxiliaryMaterial = qobject_cast<QQuickItem *>(auxiliary->property("background").value<QObject *>());
    CTEST_CHECK(auxiliaryMaterial);
    CTEST_CHECK(!root->findChild<QObject *>("applicationWindowGlassOverlay"));
    auto *grain = material->findChild<QQuickItem *>("applicationWindowNoiseTexture");
    auto *auxiliaryGrain = auxiliaryMaterial->findChild<QQuickItem *>("applicationWindowNoiseTexture");
    CTEST_CHECK(grain && auxiliaryGrain);
    MATERIAL_COMPARE(grain->property("noiseStrength").toReal(), 0.0);
    MATERIAL_COMPARE(grain->property("blurRadius").toReal(), 0.0);
    MATERIAL_COMPARE(grain->property("grainStrength").toReal(), 2.0 / 255);
    CTEST_CHECK(!window->property("backgroundBlurSupported").toBool());
    const auto matches = [](const QColor &actual, const QColor &expected) {
        return actual.alpha() == 255 && qAbs(actual.red() - expected.red()) <= 2
            && qAbs(actual.green() - expected.green()) <= 2 && qAbs(actual.blue() - expected.blue()) <= 2;
    };
    const QString directory = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_DIR");
    QHash<QRgb, QImage> originalTiles;
    for (const QSize size : {QSize(900, 600), QSize(1600, 1000)}) {
        window->resize(size);
        MATERIAL_WAIT_COMPARE(material->size(), QSizeF(size));
        MATERIAL_WAIT_COMPARE(grain->size(), QSizeF(size));
        for (const QColor accent : {QColor("#0A84FF"), AccentFixture::color("accentGreen"), QColor("#A571E6"), QColor(Qt::white), QColor(Qt::black)}) {
            CTEST_CHECK(window->setProperty("primaryColor", accent));
            MATERIAL_WAIT_COMPARE(material->property("primaryColor").value<QColor>(), accent);
            MATERIAL_WAIT_COMPARE(auxiliaryMaterial->property("primaryColor").value<QColor>(), accent);
            const QColor expected = accent2Mix(AccentFixture::background("Panel/window"), accent);
            MATERIAL_COMPARE(material->property("color").value<QColor>().rgba(), expected.rgba());
            MATERIAL_WAIT_COMPARE(grain->property("color").value<QColor>().rgba(), expected.rgba());
            MATERIAL_WAIT_COMPARE(auxiliaryGrain->property("color").value<QColor>().rgba(), expected.rgba());
            auto capture = material->grabToImage();
            CTEST_CHECK(capture);
            awaitCapture(capture);
            const QImage background = capture->image();
            for (int y = 0; y < background.height(); ++y)
                for (int x = 0; x < background.width(); ++x)
                    CTEST_CHECK_MESSAGE(matches(background.pixelColor(x, y), expected), "Fine grain must stay within two RGB levels of the opaque 98:2 mix.");
            const auto stats = grainStats(background, QRect(16, 16, 256, 256));
            CTEST_CHECK(qAbs(stats.mean - expected.green()) < 0.65);
            CTEST_CHECK_MESSAGE(stats.deviation > 0.4 && stats.deviation < 1.5, "Grain must be perceptible without strong contrast.");
            const qreal minimumChanges = expected.green() == 0 ? 0.45 : 0.65;
            CTEST_CHECK_MESSAGE(stats.changedNeighbors > minimumChanges, "Grain must vary at individual physical pixels, including on Retina displays.");
            const QImage tile = background.copy(0, 0, 128, 128);
            if (originalTiles.contains(accent.rgb()))
                MATERIAL_COMPARE(tile, originalTiles.value(accent.rgb()));
            else
                originalTiles.insert(accent.rgb(), tile);
            const QImage frame = window->grabWindow();
            CTEST_CHECK(!frame.isNull());
            CTEST_CHECK(matches(frame.pixelColor(500 * frame.width() / size.width(), 400 * frame.height() / size.height()), expected));
            const qreal scale = frame.width() / qreal(size.width());
            const auto frameStats = grainStats(frame, QRect(qRound(500 * scale), qRound(350 * scale), qRound(128 * scale), qRound(128 * scale)));
            CTEST_CHECK(frameStats.deviation > 0.4 && frameStats.deviation < 1.5);
            CTEST_CHECK(frameStats.changedNeighbors > minimumChanges);
            const QPointF position = foreground->mapToScene(QPointF(24, 24));
            MATERIAL_COMPARE(frame.pixelColor(qRound(position.x() * frame.width() / window->width()),
                                      qRound(position.y() * frame.height() / window->height())), QColor("#F0A060"));
            if (!directory.isEmpty()) {
                CTEST_CHECK(QDir().mkpath(directory));
                CTEST_CHECK(frame.save(directory + "/app-accent2-" + accent.name().mid(1) + "-" + QString::number(size.width()) + ".png"));
                if (accent == QColor("#0A84FF") && size.width() == 900)
                    CTEST_CHECK(frame.copy(qRound(500 * scale), qRound(350 * scale), qRound(256 * scale), qRound(192 * scale)).save(directory + "/fine-grain-detail.png"));
            }
            for (const bool enabled : {false, true}) {
                CTEST_CHECK(window->setProperty("backgroundBlurEnabled", enabled));
                CTEST_CHECK(!window->property("backgroundBlurActive").toBool());
                MATERIAL_COMPARE(material->property("color").value<QColor>().rgba(), expected.rgba());
            }
            MATERIAL_COMPARE(window->color(), AccentFixture::background("Panel/window"));
            MATERIAL_COMPARE(window->opacity(), 1.0);
        }
    }
    CTEST_CHECK(window->setProperty("windowBackgroundOpacity", 0.05));
    CTEST_CHECK(window->setProperty("windowColor", QColor("#33202124")));
    CTEST_CHECK(window->setProperty("primaryColor", QColor("#00A571E6")));
    MATERIAL_WAIT_COMPARE(window->color(), QColor("#202124"));
    const QColor expected = accent2Mix(QColor("#202124"), QColor("#A571E6"));
    MATERIAL_COMPARE(material->property("color").value<QColor>().rgba(), expected.rgba());
    auto capture = material->grabToImage();
    CTEST_CHECK(capture);
    awaitCapture(capture);
    CTEST_CHECK(matches(capture->image().pixelColor(64, 64), expected));
    const QImage original = capture->image();
    CTEST_CHECK(grain->setProperty("grainStrength", 0.0));
    capture = material->grabToImage();
    CTEST_CHECK(capture);
    awaitCapture(capture);
    const auto flat = grainStats(capture->image(), QRect(16, 16, 256, 256));
    MATERIAL_COMPARE(flat.deviation, 0.0);
    MATERIAL_COMPARE(flat.mean, qreal(expected.green()));
    CTEST_CHECK(grain->setProperty("grainStrength", 2.0 / 255));
    capture = material->grabToImage();
    CTEST_CHECK(capture);
    awaitCapture(capture);
    MATERIAL_COMPARE(capture->image(), original); // A fixed pattern survives toggles and resizing.
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
    CTEST_CHECK(root);
    auto *custom = root->property("background").value<QObject *>();
    CTEST_CHECK(custom);
    MATERIAL_COMPARE(custom->objectName(), QStringLiteral("customBackground"));
    CTEST_CHECK(!root->property("backgroundBlurSupported").toBool());
    CTEST_CHECK(!root->property("backgroundBlurActive").toBool());
    MATERIAL_COMPARE(qobject_cast<QQuickWindow *>(root.data())->color().alpha(), 255);
}

void MaterialTests::noise_texture_defaults_match_figma()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import LVRS as LV; LV.WindowNoiseTexture {}"));
    CTEST_CHECK(root);
    MATERIAL_COMPARE(root->property("color").value<QColor>(), AccentFixture::background("Panel/window"));
    MATERIAL_COMPARE(root->property("primaryColor").value<QColor>(), AccentFixture::color("accentBlue"));
}

void MaterialTests::noise_texture_inputs_are_bounded_and_opaque()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import LVRS as LV
LV.WindowNoiseTexture { color: "#33123456"; primaryColor: "#00A571E6" }
)"));
    CTEST_CHECK(root);
    MATERIAL_COMPARE(root->property("color").value<QColor>(), QColor("#123456"));
    MATERIAL_COMPARE(root->property("primaryColor").value<QColor>(), QColor("#A571E6"));
    CTEST_CHECK(root->setProperty("noiseStrength", -1.0));
    MATERIAL_COMPARE(root->property("noiseStrength").toReal(), 0.0);
    CTEST_CHECK(root->setProperty("noiseStrength", 2.0));
    MATERIAL_COMPARE(root->property("noiseStrength").toReal(), 1.0);
    CTEST_CHECK(root->setProperty("blurRadius", -1.0));
    MATERIAL_COMPARE(root->property("blurRadius").toReal(), 0.0);
    CTEST_CHECK(root->setProperty("blurRadius", 20.0));
    MATERIAL_COMPARE(root->property("blurRadius").toReal(), 20.0);
    CTEST_CHECK(root->setProperty("blurRadius", 80.0));
    MATERIAL_COMPARE(root->property("blurRadius").toReal(), 50.0);
    CTEST_CHECK(root->setProperty("blurRadius", qQNaN()));
    MATERIAL_COMPARE(root->property("blurRadius").toReal(), 50.0);
    CTEST_CHECK(root->setProperty("blurRadius", 0.0));
    CTEST_CHECK(root->setProperty("grainStrength", -1.0));
    MATERIAL_COMPARE(root->property("grainStrength").toReal(), 0.0);
    CTEST_CHECK(root->setProperty("grainStrength", 2.0));
    MATERIAL_COMPARE(root->property("grainStrength").toReal(), 1.0);
    CTEST_CHECK(root->setProperty("grainStrength", qQNaN()));
    MATERIAL_COMPARE(root->property("grainStrength").toReal(), 1.0);
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *surface = root->findChild<QQuickItem *>("surface");
    CTEST_CHECK(window && surface);
    CTEST_WAIT_DEFAULT(window->isExposed());
    MATERIAL_COMPARE(surface->property("tintOpacity").toReal(), 0.5);
    QColor windowFill = AccentFixture::background("Surface/Dense/Fill");
    windowFill.setAlpha(255);
    const QString directory = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_DIR");
    for (const QSize size : {QSize(900, 600), QSize(1600, 1000)}) {
        window->resize(size);
        MATERIAL_WAIT_COMPARE(surface->size(), QSizeF(size));
        for (const QColor accent : {QColor("#0A84FF"), QColor("#A571E6")}) {
            CTEST_CHECK(surface->setProperty("primaryColor", accent));
            MATERIAL_COMPARE(surface->property("color").value<QColor>(), windowFill);
            MATERIAL_WAIT_COMPARE(surface->property("displayedPrimaryColor").value<QColor>(), accent);
            auto capture = surface->grabToImage();
            CTEST_CHECK(capture);
            awaitCapture(capture);
            const QImage image = capture->image();
            const auto pixelAt = [&](int x, int y) {
                return image.pixelColor(x * image.width() / size.width(), y * image.height() / size.height());
            };
            const QColor strong = pixelAt(48, 32);
            const QColor quiet = pixelAt(size.width() / 2, 48);
            CTEST_CHECK_MESSAGE(strong.blue() > quiet.blue() + 8 && strong.blue() > strong.green() + 8,
                     qPrintable(QString("The window must render an accent gradient, not a uniform tint: strong %1, quiet %2")
                         .arg(strong.name(QColor::HexArgb), quiet.name(QColor::HexArgb))));
            if (accent.red() > accent.green())
                CTEST_CHECK(strong.red() > strong.green() + 8);
            CTEST_CHECK_MESSAGE(quiet.alpha() >= 126 && quiet.alpha() < 160 && strong.alpha() < 220,
                     "The gradient must retain a translucent background.");
            const QColor right = pixelAt(size.width() - 12, size.height() / 2);
            CTEST_CHECK_MESSAGE(right.blue() > quiet.blue() + 8,
                     "The accent at the right edge must follow the resized window.");
            if (!directory.isEmpty()) {
                CTEST_CHECK(QDir().mkpath(directory));
                CTEST_CHECK(window->grabWindow().save(directory + QString("/window-gradient-%1-%2.png")
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    CTEST_WAIT_DEFAULT(window->isExposed());
    for (const char *name : {"menu", "tip", "popover"}) {
        auto *popup = root->findChild<QObject *>(name);
        CTEST_CHECK(popup);
        CTEST_CHECK(QMetaObject::invokeMethod(popup, "open"));
        MATERIAL_WAIT_MESSAGE(popup->property("opened").toBool(), name);
        auto *background = qobject_cast<QQuickItem *>(popup->property("background").value<QObject *>());
        CTEST_CHECK(background);
        const bool isMenu = qstrcmp(name, "menu") == 0;
        MATERIAL_COMPARE(background->property("tintOpacity").toReal(), isMenu ? 0.12 : 0.25);
        MATERIAL_COMPARE(background->property("blurRadius").toReal(), isMenu ? 64.0 : 16.0);
        MATERIAL_COMPARE(background->property("primaryColor").value<QColor>(), QColor("#A571E6"));
        MATERIAL_COMPARE(background->opacity(), 1.0);
        if (background->property("rendererAvailable").toBool())
            MATERIAL_WAIT_MESSAGE(background->property("captureActive").toBool(), name);
        CTEST_CHECK(QMetaObject::invokeMethod(popup, "close"));
        CTEST_WAIT_DEFAULT(!popup->property("visible").toBool());
    }
    auto *unsafe = root->findChild<QObject *>("unsafe");
    CTEST_CHECK(unsafe);
    CTEST_CHECK(!unsafe->property("resolvedBackdropSource").value<QObject *>());
    CTEST_CHECK(unsafe->setProperty("backdropSource", QVariant::fromValue(qobject_cast<QQuickItem *>(unsafe))));
    CTEST_CHECK(!unsafe->property("resolvedBackdropSource").value<QObject *>());
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *material = root->findChild<QQuickItem *>("material");
    CTEST_CHECK(window && material);
    CTEST_WAIT_DEFAULT(window->isExposed());
    if (window->rendererInterface()->graphicsApi() == QSGRendererInterface::Software)
        lvrs_ctest::skip("Backdrop blur requires a native RHI renderer; software keeps the translucent radial fallback.");
    CTEST_WAIT_DEFAULT(material->property("effectsActive").toBool());
    CTEST_WAIT_DEFAULT(material->property("captureActive").toBool());
    auto capture = material->grabToImage();
    CTEST_CHECK(capture);
    awaitCapture(capture);
    const QImage blurred = capture->image();
    CTEST_CHECK(material->setProperty("blurEnabled", false));
    processFor(80);
    capture = material->grabToImage();
    awaitCapture(capture);
    const QImage sharp = capture->image();
    auto variation = [](const QImage &image) {
        qint64 sum = 0;
        for (int y = image.height()/4; y < image.height()*3/4; ++y)
            for (int x = image.width()/4 + 1; x < image.width()*3/4; ++x)
                sum += qAbs(qGray(image.pixel(x,y)) - qGray(image.pixel(x-1,y)));
        return sum;
    };
    CTEST_CHECK_MESSAGE(variation(blurred) * 2 < variation(sharp), "Glass must actually soften the captured backdrop.");
    CTEST_CHECK(material->setProperty("backdropSource", QVariant::fromValue<QQuickItem *>(nullptr)));
    CTEST_CHECK(material->setProperty("primaryColor", QColor("#A571E6")));
    MATERIAL_WAIT_COMPARE(material->property("displayedPrimaryColor").value<QColor>(), QColor("#A571E6"));
    capture = material->grabToImage();
    awaitCapture(capture);
    const QColor purple = capture->image().pixelColor(capture->image().width()*61/380, capture->image().height()*74/204);
    CTEST_CHECK_MESSAGE(purple.blue() > purple.green() && purple.red() > purple.green(), "Primary must tint the rendered radial gradient.");
    const QString directory = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_DIR");
    if (!directory.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(directory));
        CTEST_CHECK(window->grabWindow().save(directory + "/materials-native.png"));
    }
}

void MaterialTests::catalog_transient_interaction()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::loadQmlFile(engine,
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../example/VisualCatalog/qml/Main.qml")));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    window->resize(1100, 900);
    CTEST_WAIT_DEFAULT(window->isExposed());
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry",
        Q_ARG(QVariant, QStringLiteral("window-material"))));
    CTEST_WAIT_DEFAULT(root->findChild<QObject *>("galleryMaterialPopover"));
    const QString directory = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_DIR");
    if (!directory.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(directory));
        processFor(160);
        CTEST_CHECK(window->grabWindow().save(directory + "/material-gallery-blue.png"));
    }
    for (const char *name : {"galleryMaterialMenu", "galleryMaterialTooltip", "galleryMaterialPopover"}) {
        auto *popup = root->findChild<QObject *>(name);
        CTEST_CHECK(popup);
        MaterialPopupRecorder recorder;
        const auto openingConnection = QObject::connect(popup, SIGNAL(aboutToShow()),
                                                         &recorder, SLOT(recordOpening()));
        const auto closingConnection = QObject::connect(popup, SIGNAL(aboutToHide()),
                                                         &recorder, SLOT(recordClosing()));
        CTEST_CHECK(openingConnection);
        CTEST_CHECK(closingConnection);
        const auto popupState = [&] {
            return QStringLiteral("%1 visible=%2 opacity=%3 opened=%4 opening=%5 closing=%6 windowActive=%7 exposed=%8")
                .arg(QString::fromLatin1(name))
                .arg(popup->property("visible").toBool())
                .arg(popup->property("opacity").toReal())
                .arg(popup->property("opened").toBool())
                .arg(recorder.openingCount).arg(recorder.closingCount)
                .arg(window->isActive()).arg(window->isExposed());
        };
        CTEST_CHECK(QMetaObject::invokeMethod(popup, "open"));
        MATERIAL_WAIT_MESSAGE(popup->property("opened").toBool(), qPrintable(popupState()));
        auto *background = qobject_cast<QQuickItem *>(popup->property("background").value<QObject *>());
        CTEST_CHECK(background);
        MATERIAL_COMPARE(background->property("tintOpacity").toReal(), qstrcmp(name, "galleryMaterialMenu") == 0 ? 0.12 : 0.25);
        if (!directory.isEmpty()) {
            processFor(160);
            CTEST_CHECK(window->grabWindow().save(directory + "/" + name + ".png"));
        }
        CTEST_CHECK(QMetaObject::invokeMethod(popup, "close"));
        CTEST_WAIT_DEFAULT(!popup->property("visible").toBool());
    }
    CTEST_CHECK(root->setProperty("primaryColor", QColor("#A571E6")));
    auto *background = root->findChild<QObject *>("applicationWindowMaterial");
    CTEST_CHECK(background);
    MATERIAL_WAIT_COMPARE(background->property("primaryColor").value<QColor>(), QColor("#A571E6"));
    if (!directory.isEmpty()) {
        processFor(160);
        CTEST_CHECK(window->grabWindow().save(directory + "/material-gallery-purple.png"));
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
    CTEST_CHECK(root);
    auto *windowBackground = root->property("background").value<QObject *>();
    CTEST_CHECK(windowBackground);
    for (const char *name : {"frostMenu", "regularMenu"}) {
        auto *menu = root->findChild<QObject *>(name);
        CTEST_CHECK(menu);
        auto *material = menu->property("background").value<QObject *>();
        CTEST_CHECK(material);
        MATERIAL_COMPARE(menu->property("backdropSource").value<QObject *>(),
                 root->property("materialBackdropSource").value<QObject *>());
        MATERIAL_COMPARE(menu->property("menuOpacity").toReal(), 0.12);
        MATERIAL_COMPARE(material->property("tintOpacity").toReal(), 0.12);
        MATERIAL_COMPARE(material->property("blurRadius").toReal(), 64.0);
        MATERIAL_COMPARE(material->property("intenseOpacity").toReal(), 0.032);
        MATERIAL_COMPARE(material->property("faintOpacity").toReal(), 0.0088);
        MATERIAL_COMPARE(material->property("shadowRadius").toReal(), 48.0);
        MATERIAL_COMPARE(menu->property("backdropBackground").value<QObject *>(), windowBackground);
        MATERIAL_COMPARE(material->property("color").value<QColor>(), windowBackground->property("color").value<QColor>());
        const QColor captureBase = material->property("backdropBaseColor").value<QColor>();
        MATERIAL_COMPARE(captureBase.alpha(), 255);
        MATERIAL_COMPARE(captureBase.red(), captureBase.green());
        MATERIAL_COMPARE(captureBase.green(), captureBase.blue());
        MATERIAL_COMPARE(menu->property("opacity").toReal(), 1.0);
        CTEST_CHECK(menu->setProperty("menuOpacity", 0.2));
        CTEST_CHECK(menu->setProperty("menuBlurRadius", 32.0));
        CTEST_CHECK(menu->setProperty("menuAccentStrength", 0.5));
        MATERIAL_COMPARE(material->property("tintOpacity").toReal(), 0.2);
        MATERIAL_COMPARE(material->property("blurRadius").toReal(), 32.0);
        MATERIAL_COMPARE(material->property("intenseOpacity").toReal(), 0.2);
        MATERIAL_COMPARE(material->property("faintOpacity").toReal(), 0.055);
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *menu = root->findChild<QObject *>("frostMenu");
    auto *material = qobject_cast<QQuickItem *>(menu->property("background").value<QObject *>());
    CTEST_CHECK(window && menu && material);
    CTEST_WAIT_DEFAULT(window->isExposed());
    CTEST_CHECK(QMetaObject::invokeMethod(menu, "openAt", Q_ARG(QVariant, 80), Q_ARG(QVariant, 75)));
    CTEST_WAIT_DEFAULT(menu->property("opened").toBool());
    if (!material->property("rendererAvailable").toBool())
        lvrs_ctest::skip("Frost sampling requires a native RHI renderer.");
    CTEST_WAIT_DEFAULT(material->property("captureActive").toBool());
    CTEST_CHECK_MESSAGE(material->property("resolvedBackdropSource").value<QObject *>(),
             "App content, not only its window background, must be captured.");
    const auto capture = [&]() {
        auto request = material->grabToImage();
        awaitCapture(request);
        processFor(180);
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
    CTEST_CHECK(menu->setProperty("menuOpacity", 0.25));
    CTEST_CHECK(menu->setProperty("menuBlurRadius", 16.0));
    CTEST_CHECK(menu->setProperty("menuAccentStrength", 1.0));
    processFor(200);
    const auto previous = capture();
    CTEST_CHECK(!previous.isNull());
    if (!directory.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(directory));
        CTEST_CHECK(window->grabWindow().save(directory + "/menu-before.png"));
    }
    menu->setProperty("menuOpacity", 0.12);
    menu->setProperty("menuBlurRadius", 64.0);
    menu->setProperty("menuAccentStrength", 0.08);
    processFor(200);
    const auto frosted = capture();
    CTEST_CHECK(!frosted.isNull());
    CTEST_CHECK_MESSAGE(blueExcess(frosted) < blueExcess(previous) * 0.6,
        "The menu must inherit the window backdrop without another saturated blue layer.");
    if (!directory.isEmpty())
        CTEST_CHECK(window->grabWindow().save(directory + "/menu-after.png"));
    // A larger blur must suppress variation of a stripe pattern behind the menu.
    const auto variation = [](const QImage &image) {
        qint64 sum = 0;
        for (int y = image.height() / 4; y < image.height() * 3 / 4; ++y)
            for (int x = image.width() / 4 + 1; x < image.width() * 3 / 4; ++x)
                sum += qAbs(qGray(image.pixel(x, y)) - qGray(image.pixel(x - 1, y)));
        return sum;
    };
    auto *pattern = root->findChild<QQuickItem *>("frostPattern");
    CTEST_CHECK(pattern);
    const auto region = pattern->mapRectToItem(nullptr, pattern->boundingRect()).intersected(
        material->mapRectToItem(nullptr, material->boundingRect().adjusted(35, 25, -35, -25)));
    CTEST_CHECK(!region.isEmpty());
    const auto visibleBackdrop = [&]() {
        const QImage frame = window->grabWindow();
        const qreal ratio = qreal(frame.width()) / window->width();
        return frame.copy(QRect(qRound(region.x() * ratio), qRound(region.y() * ratio),
                                qRound(region.width() * ratio), qRound(region.height() * ratio)));
    };
    const QImage visibleFrost = visibleBackdrop();
    material->setProperty("blurEnabled", false);
    processFor(100);
    const auto sharp = visibleBackdrop();
    CTEST_CHECK(!sharp.isNull());
    CTEST_CHECK_MESSAGE(variation(visibleFrost) * 4 < variation(sharp),
        qPrintable(QString("Visible background variation: frosted %1, sharp %2")
                   .arg(variation(visibleFrost)).arg(variation(sharp))));
    MATERIAL_COMPARE(menu->property("opacity").toReal(), 1.0);
    CTEST_CHECK(QMetaObject::invokeMethod(menu, "close"));
    CTEST_WAIT_DEFAULT(!menu->property("visible").toBool());
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    MaterialTests tests;
    runner.run("window_default_and_primary", [&] { tests.window_default_and_primary(); });
    runner.run("window_material_inherits_owning_window_accent", [&] { tests.window_material_inherits_owning_window_accent(); });
    runner.run("application_window_theme_accent_mix", [&] { tests.application_window_theme_accent_mix(); });
    runner.run("application_window_custom_background", [&] { tests.application_window_custom_background(); });
    runner.run("noise_texture_defaults_match_figma", [&] { tests.noise_texture_defaults_match_figma(); });
    runner.run("noise_texture_inputs_are_bounded_and_opaque", [&] { tests.noise_texture_inputs_are_bounded_and_opaque(); });
    runner.run("window_gradient_tracks_accent_and_resize", [&] { tests.window_gradient_tracks_accent_and_resize(); });
    runner.run("transient_defaults_and_capture_safety", [&] { tests.transient_defaults_and_capture_safety(); });
    runner.run("backdrop_blur_and_color_rendering", [&] { tests.backdrop_blur_and_color_rendering(); });
    runner.run("catalog_transient_interaction", [&] { tests.catalog_transient_interaction(); });
    runner.run("context_menu_frost_contract", [&] { tests.context_menu_frost_contract(); });
    runner.run("context_menu_frost_rendering", [&] { tests.context_menu_frost_rendering(); });
    return runner.result();
}
#include "tst_materials.moc"
