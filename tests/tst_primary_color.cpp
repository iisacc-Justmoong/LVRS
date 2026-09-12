#include <QtTest>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QQmlEngine>
#include "test_utils.h"

namespace {
QByteArray windowQml(const QByteArray &properties = {}, const QByteArray &content = {})
{
    return R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    visible: false
    width: 640; height: 480
    desktopMinWidth: 0; desktopMinHeight: 0
    useInternalPageStack: false
    autoAttachRuntimeEvents: false
    windowChromeInteractionsEnabled: false
    readonly property QtObject theme: LV.Theme
)" + properties + '\n' + content + "\n}";
}

QColor color(QObject *object, const char *property)
{
    return object ? object->property(property).value<QColor>() : QColor();
}

QObject *theme(QObject *window)
{
    return window->property("theme").value<QObject *>();
}

QQuickItem *findVisualItem(QQuickItem *parent, const QString &name)
{
    if (parent->objectName() == name)
        return parent;
    for (QQuickItem *child : parent->childItems()) {
        if (QQuickItem *found = findVisualItem(child, name))
            return found;
    }
    return nullptr;
}

const QByteArray controls = R"(
    Column {
        LV.LabelButton { objectName: "primaryButton"; text: "Continue" }
        LV.LabelButton { objectName: "borderlessButton"; tone: LV.AbstractButton.Borderless }
        LV.LabelButton { objectName: "overrideButton"; backgroundColor: "#57965C" }
        LV.LabelButton { objectName: "dangerButton"; tone: LV.AbstractButton.Destructive }
        LV.CheckBox { objectName: "checkBox"; checked: true }
        LV.RadioButton { objectName: "radioButton"; checked: true }
        LV.ToggleSwitch { objectName: "toggleSwitch"; checked: true; transitionDuration: 0 }
        LV.Slider { objectName: "slider" }
        LV.InputField { objectName: "inputField"; glassEnabled: false }
        LV.TextEditor { objectName: "textEditor"; filePath: "" }
        LV.CodeEditor { objectName: "codeEditor" }
        LV.Stepper { objectName: "stepper" }
        LV.Link { objectName: "link" }
        LV.ListItem { objectName: "listItem"; selected: true }
        LV.HierarchyItem { objectName: "hierarchyItem" }
        LV.Table { objectName: "table" }
        LV.MenuItem { objectName: "menuItem" }
        LV.ListFooter { objectName: "footer" }
        LV.AlertButton { objectName: "alertButton"; dialogStyle: true; tone: LV.AbstractButton.Primary }
    }
    Loader {
        objectName: "lateLoader"
        active: false
        sourceComponent: LV.LabelButton { objectName: "lateButton" }
    }
)";
}

class PrimaryColorTests : public QObject
{
    Q_OBJECT
private slots:
    void init();
    void default_blue_is_compatible();
    void declarative_color_data();
    void declarative_color();
    void initial_properties_color();
    void palette_can_be_observed_from_qml();
    void runtime_color_updates_controls_and_overlays();
    void secondary_windows_inherit_without_resetting();
    void engines_have_independent_themes();
    void custom_color_is_rendered();
    void catalog_accent_choices_update_the_app();
};

void PrimaryColorTests::init()
{
    QTest::failOnWarning(QRegularExpression(".*Binding loop detected.*"));
}

void PrimaryColorTests::default_blue_is_compatible()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> window(TestUtils::createFromQml(engine, windowQml({}, controls)));
    QVERIFY(window);
    QCOMPARE(color(window.data(), "primaryColor"), QColor("#0A84FF"));
    QObject *tokens = theme(window.data());
    QVERIFY(tokens);
    QCOMPARE(color(tokens, "defaultPrimary"), QColor("#0A84FF"));
    QCOMPARE(color(tokens, "primary"), QColor("#0A84FF"));
    QCOMPARE(color(tokens, "accentTint"), QColor("#1F0A84FF"));
    QCOMPARE(color(tokens, "primaryOverlay"), QColor("#400A84FF"));
    QCOMPARE(color(tokens, "accentMuted"), QColor("#25324D"));
    QCOMPARE(color(tokens, "accentDetail"), QColor("#548AF7"));
    QCOMPARE(color(tokens, "alertActionPrimary"), QColor("#027DFF"));
    QCOMPARE(color(tokens, "alertIconSurface"), QColor("#192840"));
    QCOMPARE(color(tokens, "alertIconBorder"), QColor("#244B7E"));
    QCOMPARE(color(window->findChild<QObject *>("primaryButton"), "backgroundColor"), QColor("#0A84FF"));
    auto *checkBox = window->findChild<QObject *>("checkBox");
    QVERIFY(checkBox);
    QVERIFY(checkBox->property("usingFigmaCheckedAsset").toBool());
}

void PrimaryColorTests::declarative_color_data()
{
    QTest::addColumn<QByteArray>("rootType");
    QTest::addColumn<QColor>("primary");
    QTest::newRow("purple") << QByteArray("ApplicationWindow") << QColor("#A571E6");
    QTest::newRow("orange") << QByteArray("ApplicationWindow") << QColor("#FF9F45");
    QTest::newRow("black") << QByteArray("ApplicationWindow") << QColor("#000000");
    QTest::newRow("alpha") << QByteArray("ApplicationWindow") << QColor("#80A571E6");
    QTest::newRow("bootstrap") << QByteArray("AppBootstrapWindow") << QColor("#A571E6");
    QTest::newRow("shell") << QByteArray("AppShell") << QColor("#A571E6");
}

void PrimaryColorTests::declarative_color()
{
    QFETCH(QByteArray, rootType);
    QFETCH(QColor, primary);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QByteArray source = windowQml("primaryColor: \"" + primary.name(QColor::HexArgb).toUtf8() + "\"", controls);
    source.replace("LV.ApplicationWindow {", "LV." + rootType + " {");
    QScopedPointer<QObject> window(TestUtils::createFromQml(engine, source));
    QVERIFY(window);
    QCOMPARE(color(theme(window.data()), "primary"), primary);
    QCOMPARE(color(theme(window.data()), "accent"), primary);
    QCOMPARE(color(theme(window.data()), "accentTint").alpha(), qRound(primary.alphaF() * 31));
    QCOMPARE(color(theme(window.data()), "accentOverlay").alpha(), qRound(primary.alphaF() * 64));
    QCOMPARE(color(window->findChild<QObject *>("primaryButton"), "backgroundColor"), primary);
    QObject *palette = window->property("palette").value<QObject *>();
    QVERIFY(palette);
    QCOMPARE(color(palette, "highlight"), primary);
    QCOMPARE(color(palette, "link"), primary);
}

void PrimaryColorTests::initial_properties_color()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQmlComponent component(&engine);
    component.setData(windowQml({}, controls), QUrl());
    const QColor purple("#A571E6");
    QScopedPointer<QObject> window(component.createWithInitialProperties({{"primaryColor", purple}}));
    QVERIFY2(window, qPrintable(component.errorString()));
    QCOMPARE(color(theme(window.data()), "primary"), purple);
    QCOMPARE(color(window->findChild<QObject *>("primaryButton"), "backgroundColor"), purple);
}

void PrimaryColorTests::palette_can_be_observed_from_qml()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> window(TestUtils::createFromQml(engine, windowQml(R"(
        primaryColor: "#A571E6"
        readonly property color observedHighlight: palette.highlight
        readonly property color observedLink: palette.link
    )")));
    QVERIFY(window);
    QCOMPARE(color(window.data(), "observedHighlight"), QColor("#A571E6"));
    QCOMPARE(color(window.data(), "observedLink"), QColor("#A571E6"));
    QVERIFY(window->setProperty("primaryColor", QColor("#FF9F45")));
    QCOMPARE(color(window.data(), "observedHighlight"), QColor("#FF9F45"));
    QCOMPARE(color(window.data(), "observedLink"), QColor("#FF9F45"));
}

void PrimaryColorTests::runtime_color_updates_controls_and_overlays()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> window(TestUtils::createFromQml(engine, windowQml({}, controls)));
    QVERIFY(window);
    QObject *tokens = theme(window.data());
    auto *loader = window->findChild<QObject *>("lateLoader");
    QVERIFY(loader);
    const QList<QPair<QString, QByteArray>> primaryConsumers = {
        {"primaryButton", "backgroundColor"}, {"borderlessButton", "textColor"},
        {"checkBox", "checkedColor"}, {"radioButton", "onColor"}, {"toggleSwitch", "onColor"},
        {"slider", "fillColor"}, {"inputField", "selectionColor"},
        {"textEditor", "selectionColor"}, {"codeEditor", "selectionColor"},
        {"stepper", "backgroundColor"}, {"link", "linkColor"}
    };
    const QList<QPair<QString, QByteArray>> mutedConsumers = {
        {"borderlessButton", "backgroundColorPressed"}, {"textEditor", "backgroundColorPressed"},
        {"codeEditor", "backgroundColorPressed"}, {"hierarchyItem", "rowBackgroundColorActive"},
        {"hierarchyItem", "rowBackgroundColorDrag"}, {"table", "resizeHandleHoverColor"},
        {"menuItem", "iconPlaceholderColor"}, {"listFooter_iconButton_0", "backgroundColorPressed"},
        {"listFooter_menuButton_2", "backgroundColorPressed"}
    };
    for (const QColor &primary : {QColor("#A571E6"), QColor("#FF9F45"), QColor("#0A84FF")}) {
        QVERIFY(window->setProperty("primaryColor", primary));
        QCOMPARE(color(tokens, "primary"), primary);
        for (const auto &consumer : primaryConsumers) {
            QObject *control = findVisualItem(qobject_cast<QQuickWindow *>(window.data())->contentItem(), consumer.first);
            QVERIFY2(control, qPrintable(consumer.first));
            QCOMPARE(color(control, consumer.second.constData()), primary);
        }
        for (const auto &consumer : mutedConsumers) {
            QObject *control = findVisualItem(qobject_cast<QQuickWindow *>(window.data())->contentItem(), consumer.first);
            QVERIFY2(control, qPrintable(consumer.first));
            QCOMPARE(color(control, consumer.second.constData()), color(tokens, "accentMuted"));
        }
        QColor tint = color(tokens, "accentTint");
        QCOMPARE(tint.alpha(), 31);
        tint.setAlpha(255);
        QCOMPARE(tint, primary);
        QColor overlay = color(tokens, "accentOverlay");
        QCOMPARE(overlay.alpha(), 64);
        overlay.setAlpha(255);
        QCOMPARE(overlay, primary);
        QCOMPARE(color(window->findChild<QObject *>("listItem"), "selectedBackgroundColor"), color(tokens, "accentOverlay"));
        QCOMPARE(color(window->findChild<QObject *>("table"), "selectionColor"), color(tokens, "accentTint"));
        QCOMPARE(color(window->findChild<QObject *>("alertButton"), "backgroundColor"),
                 primary == QColor("#0A84FF") ? QColor("#027DFF") : primary);
        QCOMPARE(color(window->findChild<QObject *>("overrideButton"), "backgroundColor"), QColor("#57965C"));
        QCOMPARE(color(window->findChild<QObject *>("dangerButton"), "backgroundColor"), QColor("#FF453A"));
        QCOMPARE(color(tokens, "success"), QColor("#32D74B"));
        QCOMPARE(color(tokens, "warning"), QColor("#FFD60A"));
        QCOMPARE(color(tokens, "accentBlue"), QColor("#548AF7"));
        QCOMPARE(color(tokens, "accentBlueMuted"), QColor("#25324D"));
        QCOMPARE(color(window->property("palette").value<QObject *>(), "highlight"), primary);
        auto *checkBox = window->findChild<QObject *>("checkBox");
        QCOMPARE(checkBox->property("usingFigmaCheckedAsset").toBool(), primary == QColor("#0A84FF"));
        if (primary != QColor("#0A84FF")) {
            QVERIFY(color(tokens, "accentMuted") != QColor("#25324D"));
            QVERIFY(color(tokens, "alertIconSurface") != QColor("#192840"));
            QVERIFY(color(tokens, "alertIconBorder") != QColor("#244B7E"));
        }
        QVERIFY(loader->setProperty("active", true));
        QTRY_VERIFY(loader->property("item").value<QObject *>());
        QCOMPARE(color(loader->property("item").value<QObject *>(), "backgroundColor"), primary);
    }
}

void PrimaryColorTests::secondary_windows_inherit_without_resetting()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> earlyWindow(TestUtils::createFromQml(engine, windowQml()));
    QScopedPointer<QObject> mainWindow(TestUtils::createFromQml(engine, windowQml("primaryColor: \"#A571E6\"")));
    QScopedPointer<QObject> lateWindow(TestUtils::createFromQml(engine, windowQml()));
    QVERIFY(earlyWindow && mainWindow && lateWindow);
    QCOMPARE(color(earlyWindow.data(), "primaryColor"), QColor("#A571E6"));
    QCOMPARE(color(lateWindow.data(), "primaryColor"), QColor("#A571E6"));
    QVERIFY(mainWindow->setProperty("primaryColor", QColor("#FF9F45")));
    QCOMPARE(color(earlyWindow.data(), "primaryColor"), QColor("#FF9F45"));
    QCOMPARE(color(lateWindow.data(), "primaryColor"), QColor("#FF9F45"));
    mainWindow.reset();
    QCOMPARE(color(theme(lateWindow.data()), "primary"), QColor("#FF9F45"));
}

void PrimaryColorTests::engines_have_independent_themes()
{
    QQmlEngine firstEngine, secondEngine;
    firstEngine.addImportPath(TestUtils::qmlImportBase());
    secondEngine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> first(TestUtils::createFromQml(firstEngine, windowQml("primaryColor: \"#A571E6\"")));
    QScopedPointer<QObject> second(TestUtils::createFromQml(secondEngine, windowQml()));
    QVERIFY(first && second);
    QCOMPARE(color(theme(first.data()), "primary"), QColor("#A571E6"));
    QCOMPARE(color(theme(second.data()), "primary"), QColor("#0A84FF"));
}

void PrimaryColorTests::custom_color_is_rendered()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, windowQml("primaryColor: \"#A571E6\"", R"(
        LV.LabelButton { objectName: "button"; x: 32; y: 32; width: 120; height: 32; hoverEnabled: false }
        LV.CheckBox { objectName: "check"; x: 32; y: 112; boxSize: 34; checked: true; hoverEnabled: false }
    )")));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *indicator = root->findChild<QQuickItem *>("check_indicator");
    QVERIFY(window && button && indicator);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    for (const QColor &primary : {QColor("#A571E6"), QColor("#FF9F45")}) {
        QVERIFY(root->setProperty("primaryColor", primary));
        auto *background = qvariant_cast<QQuickItem *>(button->property("background"));
        QVERIFY(background);
        QTRY_COMPARE(background->property("color").value<QColor>(), primary);
        QTRY_COMPARE(indicator->property("color").value<QColor>(), primary);
        for (QQuickItem *item : {button, indicator}) {
            const auto grab = item->grabToImage(item->size().toSize());
            QVERIFY(grab);
            QTRY_VERIFY(!grab->image().isNull());
            const QColor pixel = grab->image().pixelColor(item == button ? QPoint(60, 16) : QPoint(8, 5));
            QVERIFY2(qAbs(pixel.red() - primary.red()) <= 2
                         && qAbs(pixel.green() - primary.green()) <= 2
                         && qAbs(pixel.blue() - primary.blue()) <= 2,
                     qPrintable(QString("Rendered %1, expected %2").arg(pixel.name(), primary.name())));
        }
        QVERIFY(button->setProperty("down", true));
        QTRY_COMPARE(background->property("color").value<QColor>(), primary.darker(120));
        const auto pressed = button->grabToImage(button->size().toSize());
        QVERIFY(pressed);
        QTRY_VERIFY(!pressed->image().isNull());
        const QColor pixel = pressed->image().pixelColor(60, 16);
        const QColor expected = primary.darker(120);
        QVERIFY(qAbs(pixel.red() - expected.red()) <= 2
                && qAbs(pixel.green() - expected.green()) <= 2
                && qAbs(pixel.blue() - expected.blue()) <= 2);
        QVERIFY(button->setProperty("down", false));
    }
}

void PrimaryColorTests::catalog_accent_choices_update_the_app()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    const QString path = QFINDTESTDATA("../example/VisualCatalog/qml/Main.qml");
    QVERIFY(!path.isEmpty());
    QScopedPointer<QObject> root(TestUtils::loadQmlFile(engine, path));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry",
                                     Q_ARG(QVariant, QStringLiteral("application-window"))));
    const QList<QPair<QString, QColor>> choices = {
        {"purple", QColor("#A571E6")}, {"orange", QColor("#FF9F45")}, {"blue", QColor("#0A84FF")}
    };
    for (const auto &choice : choices) {
        const QString name = "catalogPrimaryColor_" + choice.first;
        QTRY_VERIFY(findVisualItem(window->contentItem(), name));
        QQuickItem *button = findVisualItem(window->contentItem(), name);
        QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
        QCOMPARE(color(root.data(), "primaryColor"), choice.second);
        QCOMPARE(color(button, "backgroundColor"), choice.second);
        QCOMPARE(color(root->property("palette").value<QObject *>(), "highlight"), choice.second);
    }
}

QTEST_MAIN(PrimaryColorTests)
#include "tst_primary_color.moc"
