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
    void window_fill_is_uniform_when_resized();
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
    QCOMPARE(background->property("tintOpacity").toReal(), 0.5);
    QCOMPARE(background->property("blurRadius").toReal(), 64.0);
    QCOMPARE(background->property("intenseOpacity").toReal(), 0.0);
    QCOMPARE(background->property("faintOpacity").toReal(), 0.0);
    QCOMPARE(background->property("primaryColor").value<QColor>(), QColor("#0A84FF"));
    QCOMPARE(background->property("color").value<QColor>(), QColor("#0B0B0B"));
    QVERIFY(root->setProperty("primaryColor", QColor("#A571E6")));
    QTRY_COMPARE(background->property("primaryColor").value<QColor>(), QColor("#A571E6"));
    QCOMPARE(background->property("color").value<QColor>(), QColor("#0B0B0B"));
    QTRY_COMPARE(button->property("backgroundColor").value<QColor>(), QColor("#A571E6"));
    QCOMPARE(background->opacity(), 1.0);
    QCOMPARE(qobject_cast<QQuickWindow *>(root.data())->opacity(), 1.0);
    QVERIFY(root->setProperty("windowBackgroundOpacity", 0.35));
    QCOMPARE(background->property("tintOpacity").toReal(), 0.35);
    QVERIFY(root->setProperty("windowColor", QColor("#202124")));
    QCOMPARE(background->property("color").value<QColor>(), QColor("#202124"));
}

void MaterialTests::window_fill_is_uniform_when_resized()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import QtQuick.Controls
import LVRS as LV
ApplicationWindow {
    width: 900; height: 600; visible: true; color: "#141414"
    LV.WindowMaterial { objectName: "surface"; anchors.fill: parent; radius: 0; borderWidth: 0; shadowOpacity: 0 }
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
            QTRY_COMPARE(surface->property("displayedTint").value<QColor>(),
                         surface->property("resolvedTint").value<QColor>());
            auto capture = surface->grabToImage();
            QVERIFY(capture);
            QTRY_VERIFY(!capture->image().isNull());
            const QImage image = capture->image();
            for (const QPoint sample : {QPoint(48,32), QPoint(size.width()-12,size.height()/2),
                                       QPoint(72,size.height()-40), QPoint(size.width()/2,size.height()/2)}) {
                const QColor pixel = image.pixelColor(sample.x()*image.width()/size.width(), sample.y()*image.height()/size.height());
                QVERIFY2(qAbs(pixel.red() - windowFill.red()) <= 2 && qAbs(pixel.green() - windowFill.green()) <= 2
                         && qAbs(pixel.blue() - windowFill.blue()) <= 2 && qAbs(pixel.alpha() - 128) <= 1,
                         qPrintable(QString("Near-black uniform 50% fill expected, got %1 at %2,%3")
                                        .arg(pixel.name(QColor::HexArgb)).arg(sample.x()).arg(sample.y())));
            }
            if (!directory.isEmpty()) {
                QVERIFY(QDir().mkpath(directory));
                QVERIFY(window->grabWindow().save(directory + QString("/window-dark-accent-%1-%2.png")
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
