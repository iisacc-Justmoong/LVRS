#include <QtTest>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QSGRendererInterface>
#include "test_utils.h"

class MaterialTests : public QObject
{
    Q_OBJECT
private slots:
    void window_default_and_primary();
    void window_material_inherits_owning_window_accent();
    void application_window_accent_overlay_matches_figma();
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
    auto *background = root->findChild<QQuickItem *>("applicationWindowMaterial");
    auto *button = root->findChild<QObject *>("accentButton");
    QVERIFY(background && button);
    QCOMPARE(background->property("tintOpacity").toReal(), 0.66);
    QCOMPARE(background->property("blurRadius").toReal(), 64.0);
    QCOMPARE(background->property("intenseOpacity").toReal(), 0.0);
    QCOMPARE(background->property("faintOpacity").toReal(), 0.0);
    QCOMPARE(background->property("primaryColor").value<QColor>(), QColor("#0A84FF"));
    QCOMPARE(background->property("color").value<QColor>(), QColor("#141414"));
    QVERIFY(root->setProperty("primaryColor", QColor("#A571E6")));
    QTRY_COMPARE(background->property("primaryColor").value<QColor>(), QColor("#A571E6"));
    QCOMPARE(background->property("color").value<QColor>(), QColor("#141414"));
    QTRY_COMPARE(button->property("backgroundColor").value<QColor>(), QColor("#A571E6"));
    QCOMPARE(background->opacity(), 1.0);
    QCOMPARE(qobject_cast<QQuickWindow *>(root.data())->opacity(), 1.0);
    QVERIFY(root->setProperty("windowBackgroundOpacity", 0.35));
    QCOMPARE(background->property("tintOpacity").toReal(), 0.35);
    QVERIFY(root->setProperty("windowColor", QColor("#202124")));
    QCOMPARE(background->property("color").value<QColor>(), QColor("#202124"));
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

void MaterialTests::application_window_accent_overlay_matches_figma()
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
    Rectangle { objectName: "foregroundProof"; x: 300; y: 220; width: 48; height: 48; color: "#F0A060" }
})"));
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window && QTest::qWaitForWindowExposed(window));
    auto *material = root->findChild<QQuickItem *>("applicationWindowMaterial");
    auto *auxiliary = root->findChild<QQuickWindow *>("auxiliary");
    QVERIFY(material && auxiliary);
    auto *auxiliaryMaterial = auxiliary->findChild<QObject *>("applicationWindowMaterial");
    QVERIFY(auxiliaryMaterial);
    auto *overlay = material->findChild<QQuickItem *>("applicationWindowAccentOverlay");
    auto *auxiliaryOverlay = auxiliaryMaterial->findChild<QQuickItem *>("applicationWindowAccentOverlay");
    auto *foreground = root->findChild<QQuickItem *>("foregroundProof");
    QVERIFY(overlay && auxiliaryOverlay && foreground);
    QCOMPARE(overlay->opacity(), 0.04);
    QCOMPARE(material->property("intenseOpacity").toReal(), 0.0);
    QCOMPARE(material->property("faintOpacity").toReal(), 0.0);
    const QString directory = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_DIR");
    for (const QSize size : {QSize(900, 600), QSize(1600, 1000)}) {
        window->resize(size);
        QTRY_COMPARE(material->size(), QSizeF(size));
        for (const QColor accent : {QColor("#0A84FF"), QColor("#57965C"), QColor("#A571E6"), QColor("#0A84FF")}) {
            QVERIFY(window->setProperty("primaryColor", accent));
            QTRY_COMPARE(material->property("primaryColor").value<QColor>(), accent);
            QTRY_COMPARE(material->property("displayedPrimaryColor").value<QColor>(), accent);
            QTRY_COMPARE(auxiliaryMaterial->property("primaryColor").value<QColor>(), accent);
            QTRY_COMPARE(overlay->property("color").value<QColor>(), accent);
            QTRY_COMPARE(auxiliaryOverlay->property("color").value<QColor>(), accent);
            QCOMPARE(overlay->size(), QSizeF(size));
            auto capture = material->grabToImage();
            QVERIFY(capture);
            QTRY_VERIFY(!capture->image().isNull());
            const QImage image = capture->image();
            // Figma 997:3: a uniform 4% accent over #141414 at 66% opacity.
            constexpr double alpha = 0.04 + 0.66 * (1.0 - 0.04);
            const auto channel = [&](int value) { return qRound((value * 0.04 + 20 * 0.66 * 0.96) / alpha); };
            const QColor expected(channel(accent.red()), channel(accent.green()), channel(accent.blue()), qRound(alpha * 255));
            for (const QPoint point : {QPoint(48, 32), QPoint(size.width() / 2, 48),
                                      QPoint(size.width() - 12, size.height() / 2),
                                      QPoint(72, size.height() - 40), QPoint(size.width() / 2, size.height() / 2)}) {
                const QColor pixel = image.pixelColor(point.x() * image.width() / size.width(),
                                                     point.y() * image.height() / size.height());
                QVERIFY2(qAbs(pixel.red() - expected.red()) <= 2 && qAbs(pixel.green() - expected.green()) <= 2
                             && qAbs(pixel.blue() - expected.blue()) <= 2 && qAbs(pixel.alpha() - expected.alpha()) <= 2,
                         qPrintable(QString("Uniform Figma accent overlay expected %1, got %2 at %3,%4")
                             .arg(expected.name(QColor::HexArgb), pixel.name(QColor::HexArgb)).arg(point.x()).arg(point.y())));
            }
            QVERIFY(material->findChildren<QObject *>(QRegularExpression("^materialGradient_")).isEmpty());
            QCOMPARE(material->property("color").value<QColor>(), QColor("#141414"));
            QCOMPARE(material->property("tintOpacity").toReal(), 0.66);
            QCOMPARE(material->property("blurRadius").toReal(), 64.0);
            QCOMPARE(window->opacity(), 1.0);
            const QImage frame = window->grabWindow();
            const QPointF position = foreground->mapToScene(QPointF(24, 24));
            QCOMPARE(frame.pixelColor(qRound(position.x() * frame.width() / window->width()),
                                      qRound(position.y() * frame.height() / window->height())), QColor("#F0A060"));
            if (!directory.isEmpty()) {
                QVERIFY(QDir().mkpath(directory));
                QVERIFY(frame.save(directory + "/app-overlay-" + accent.name().mid(1) + "-" + QString::number(size.width()) + ".png"));
            }
        }
    }
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
        QVERIFY(QMetaObject::invokeMethod(popup, "open"));
        QTRY_VERIFY2(popup->property("opened").toBool(), name);
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
        QCOMPARE(material->property("color").value<QColor>(), QColor("#20242a"));
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
