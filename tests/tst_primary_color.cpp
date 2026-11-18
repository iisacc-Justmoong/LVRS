#include "QtEventWait.h"
#include <QFileInfo>
#include <QGuiApplication>
#include <QScopeGuard>
#include <atomic>
#include <cstdio>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QQmlEngine>
#include "test_utils.h"
#include "accent_fixture.h"

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
std::atomic_bool bindingLoopObserved{false};
QtMessageHandler previousQtHandler = nullptr;

void recordQtWarning(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (type == QtWarningMsg && message.contains(QStringLiteral("Binding loop detected")))
        bindingLoopObserved.store(true, std::memory_order_relaxed);
    if (previousQtHandler) previousQtHandler(type, context, message);
    else {
        const QByteArray bytes = message.toLocal8Bit();
        std::fprintf(stderr, "%s\n", bytes.constData());
    }
}

template <typename Function>
void checkBindingLoops(Function &&function)
{
    bindingLoopObserved.store(false, std::memory_order_relaxed);
    std::forward<Function>(function)();
    CTEST_CHECK_MESSAGE(!bindingLoopObserved.load(std::memory_order_relaxed),
                        "Qt reported a binding loop during the case");
}

QImage captureImage(QQuickItem *item)
{
    const auto capture = item->grabToImage(item->size().toSize());
    CTEST_CHECK(capture);
    bool ready = false;
    const auto captureConnection = QObject::connect(capture.data(), &QQuickItemGrabResult::ready,
                                                      capture.data(), [&ready] { ready = true; });
    CTEST_CHECK(captureConnection);
    const auto disconnectCapture = qScopeGuard([&] { QObject::disconnect(captureConnection); });
    CTEST_WAIT_DEFAULT(ready);
    const QImage image = capture->image();
    CTEST_CHECK(!image.isNull());
    return image;
}

}

class PrimaryColorTests
{
public:
    void default_blue_is_compatible();
    void declarative_color(const QByteArray &rootType, const QColor &primary);
    void initial_properties_color();
    void palette_can_be_observed_from_qml();
    void runtime_color_updates_controls_and_overlays();
    void secondary_windows_inherit_without_resetting();
    void engines_have_independent_themes();
    void custom_color_is_rendered();
    void catalog_accent_choices_update_the_app();
};

void PrimaryColorTests::default_blue_is_compatible()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> window(TestUtils::createFromQml(engine, windowQml({}, controls)));
    CTEST_CHECK(window);
    const QColor blue = AccentFixture::color("accentBlue");
    CTEST_CHECK(blue.isValid());
    CTEST_COMPARE(color(window.data(), "primaryColor"), blue);
    QObject *tokens = theme(window.data());
    CTEST_CHECK(tokens);
    CTEST_COMPARE(color(tokens, "defaultPrimary"), blue);
    CTEST_COMPARE(color(tokens, "primary"), blue);
    CTEST_COMPARE(color(tokens, "accentTint"), AccentFixture::blueWithAlpha(31.0 / 255));
    CTEST_COMPARE(color(tokens, "primaryOverlay"), AccentFixture::blueWithAlpha(64.0 / 255));
    CTEST_COMPARE(color(tokens, "accentMuted"), AccentFixture::blueComposite(0.25));
    CTEST_COMPARE(color(tokens, "accentDetail"), AccentFixture::role("Blue", "Hover"));
    CTEST_COMPARE(color(tokens, "alertActionPrimary"), blue);
    CTEST_COMPARE(color(tokens, "alertIconSurface"), AccentFixture::blueComposite(0.16));
    CTEST_COMPARE(color(tokens, "alertIconBorder"), AccentFixture::blueComposite(0.36));
    CTEST_COMPARE(color(window->findChild<QObject *>("primaryButton"), "backgroundColor"), blue);
    auto *checkBox = window->findChild<QObject *>("checkBox");
    CTEST_CHECK(checkBox);
    CTEST_CHECK(checkBox->property("usingFigmaCheckedAsset").toBool());
}

void PrimaryColorTests::declarative_color(const QByteArray &rootType, const QColor &primary)
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QByteArray source = windowQml("primaryColor: \"" + primary.name(QColor::HexArgb).toUtf8() + "\"", controls);
    source.replace("LV.ApplicationWindow {", "LV." + rootType + " {");
    QScopedPointer<QObject> window(TestUtils::createFromQml(engine, source));
    CTEST_CHECK(window);
    CTEST_COMPARE(color(theme(window.data()), "primary"), primary);
    CTEST_COMPARE(color(theme(window.data()), "accent"), primary);
    CTEST_COMPARE(color(theme(window.data()), "accentTint").alpha(), qRound(primary.alphaF() * 31));
    CTEST_COMPARE(color(theme(window.data()), "accentOverlay").alpha(), qRound(primary.alphaF() * 64));
    CTEST_COMPARE(color(window->findChild<QObject *>("primaryButton"), "backgroundColor"), primary);
    QObject *palette = window->property("palette").value<QObject *>();
    CTEST_CHECK(palette);
    CTEST_COMPARE(color(palette, "highlight"), primary);
    CTEST_COMPARE(color(palette, "link"), primary);
}

void PrimaryColorTests::initial_properties_color()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQmlComponent component(&engine);
    component.setData(windowQml({}, controls), QUrl());
    const QColor purple("#A571E6");
    QScopedPointer<QObject> window(component.createWithInitialProperties({{"primaryColor", purple}}));
    CTEST_CHECK_MESSAGE(window, qPrintable(component.errorString()));
    CTEST_COMPARE(color(theme(window.data()), "primary"), purple);
    CTEST_COMPARE(color(window->findChild<QObject *>("primaryButton"), "backgroundColor"), purple);
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
    CTEST_CHECK(window);
    CTEST_COMPARE(color(window.data(), "observedHighlight"), QColor("#A571E6"));
    CTEST_COMPARE(color(window.data(), "observedLink"), QColor("#A571E6"));
    CTEST_CHECK(window->setProperty("primaryColor", QColor("#FF9F45")));
    CTEST_COMPARE(color(window.data(), "observedHighlight"), QColor("#FF9F45"));
    CTEST_COMPARE(color(window.data(), "observedLink"), QColor("#FF9F45"));
}

void PrimaryColorTests::runtime_color_updates_controls_and_overlays()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> window(TestUtils::createFromQml(engine, windowQml({}, controls)));
    CTEST_CHECK(window);
    QObject *tokens = theme(window.data());
    auto *loader = window->findChild<QObject *>("lateLoader");
    CTEST_CHECK(loader);
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
    for (const QColor &primary : {QColor("#A571E6"), QColor("#57965C"), QColor("#FF9F45"), QColor("#0A84FF"), AccentFixture::color("accentBlue")}) {
        CTEST_CHECK(window->setProperty("primaryColor", primary));
        CTEST_COMPARE(color(tokens, "primary"), primary);
        for (const auto &consumer : primaryConsumers) {
            QObject *control = findVisualItem(qobject_cast<QQuickWindow *>(window.data())->contentItem(), consumer.first);
            CTEST_CHECK_MESSAGE(control, qPrintable(consumer.first));
            CTEST_COMPARE(color(control, consumer.second.constData()), primary);
        }
        for (const auto &consumer : mutedConsumers) {
            QObject *control = findVisualItem(qobject_cast<QQuickWindow *>(window.data())->contentItem(), consumer.first);
            CTEST_CHECK_MESSAGE(control, qPrintable(consumer.first));
            CTEST_COMPARE(color(control, consumer.second.constData()), color(tokens, "accentMuted"));
        }
        QColor tint = color(tokens, "accentTint");
        CTEST_COMPARE(tint.alpha(), 31);
        tint.setAlpha(255);
        CTEST_COMPARE(tint, primary);
        QColor overlay = color(tokens, "accentOverlay");
        CTEST_COMPARE(overlay.alpha(), 64);
        overlay.setAlpha(255);
        CTEST_COMPARE(overlay, primary);
        CTEST_COMPARE(color(window->findChild<QObject *>("listItem"), "selectedBackgroundColor"), color(tokens, "accentOverlay"));
        CTEST_COMPARE(color(window->findChild<QObject *>("table"), "selectionColor"), color(tokens, "accentTint"));
        CTEST_COMPARE(color(window->findChild<QObject *>("alertButton"), "backgroundColor"),
                 primary);
        CTEST_COMPARE(color(window->findChild<QObject *>("overrideButton"), "backgroundColor"), QColor("#57965C"));
        CTEST_COMPARE(color(window->findChild<QObject *>("dangerButton"), "backgroundColor"), AccentFixture::color("accentRed"));
        CTEST_COMPARE(color(tokens, "success"), AccentFixture::color("accentGreen"));
        CTEST_COMPARE(color(tokens, "warning"), AccentFixture::color("accentYellow"));
        CTEST_COMPARE(color(tokens, "accentBlue"), AccentFixture::color("accentBlue"));
        CTEST_COMPARE(color(tokens, "accentBlueMuted"), AccentFixture::blueComposite(0.25));
        CTEST_COMPARE(color(window->property("palette").value<QObject *>(), "highlight"), primary);
        auto *checkBox = window->findChild<QObject *>("checkBox");
        CTEST_COMPARE(checkBox->property("usingFigmaCheckedAsset").toBool(), primary == AccentFixture::color("accentBlue"));
        if (primary != AccentFixture::color("accentBlue")) {
            CTEST_CHECK(color(tokens, "accentMuted") != AccentFixture::blueComposite(0.25));
            CTEST_CHECK(color(tokens, "alertIconSurface") != AccentFixture::blueComposite(0.16));
            CTEST_CHECK(color(tokens, "alertIconBorder") != AccentFixture::blueComposite(0.36));
        }
        CTEST_CHECK(loader->setProperty("active", true));
        CTEST_WAIT_DEFAULT(loader->property("item").value<QObject *>());
        CTEST_COMPARE(color(loader->property("item").value<QObject *>(), "backgroundColor"), primary);
    }
}

void PrimaryColorTests::secondary_windows_inherit_without_resetting()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> earlyWindow(TestUtils::createFromQml(engine, windowQml()));
    QScopedPointer<QObject> mainWindow(TestUtils::createFromQml(engine, windowQml("primaryColor: \"#A571E6\"")));
    QScopedPointer<QObject> lateWindow(TestUtils::createFromQml(engine, windowQml()));
    CTEST_CHECK(earlyWindow && mainWindow && lateWindow);
    CTEST_COMPARE(color(earlyWindow.data(), "primaryColor"), QColor("#A571E6"));
    CTEST_COMPARE(color(lateWindow.data(), "primaryColor"), QColor("#A571E6"));
    CTEST_CHECK(mainWindow->setProperty("primaryColor", QColor("#FF9F45")));
    CTEST_COMPARE(color(earlyWindow.data(), "primaryColor"), QColor("#FF9F45"));
    CTEST_COMPARE(color(lateWindow.data(), "primaryColor"), QColor("#FF9F45"));
    mainWindow.reset();
    CTEST_COMPARE(color(theme(lateWindow.data()), "primary"), QColor("#FF9F45"));
}

void PrimaryColorTests::engines_have_independent_themes()
{
    QQmlEngine firstEngine, secondEngine;
    firstEngine.addImportPath(TestUtils::qmlImportBase());
    secondEngine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> first(TestUtils::createFromQml(firstEngine, windowQml("primaryColor: \"#A571E6\"")));
    QScopedPointer<QObject> second(TestUtils::createFromQml(secondEngine, windowQml()));
    CTEST_CHECK(first && second);
    CTEST_COMPARE(color(theme(first.data()), "primary"), QColor("#A571E6"));
    CTEST_COMPARE(color(theme(second.data()), "primary"), AccentFixture::color("accentBlue"));
}

void PrimaryColorTests::custom_color_is_rendered()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, windowQml("primaryColor: \"#A571E6\"", R"(
        LV.LabelButton { objectName: "button"; x: 32; y: 32; width: 120; height: 32; hoverEnabled: false }
        LV.CheckBox { objectName: "check"; x: 32; y: 112; boxSize: 34; checked: true; hoverEnabled: false }
    )")));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("button");
    auto *indicator = root->findChild<QQuickItem *>("check_indicator");
    CTEST_CHECK(window && button && indicator);
    window->show();
    CTEST_WAIT_DEFAULT(window->isExposed());
    for (const QColor &primary : {QColor("#A571E6"), QColor("#FF9F45")}) {
        CTEST_CHECK(root->setProperty("primaryColor", primary));
        auto *background = qvariant_cast<QQuickItem *>(button->property("background"));
        CTEST_CHECK(background);
        CTEST_WAIT_COMPARE(background->property("color").value<QColor>(), primary);
        CTEST_WAIT_COMPARE(indicator->property("color").value<QColor>(), primary);
        for (QQuickItem *item : {button, indicator}) {
            const QImage image = captureImage(item);
            const QColor pixel = image.pixelColor(item == button ? QPoint(60, 16) : QPoint(8, 5));
            CTEST_CHECK_MESSAGE(qAbs(pixel.red() - primary.red()) <= 2
                         && qAbs(pixel.green() - primary.green()) <= 2
                         && qAbs(pixel.blue() - primary.blue()) <= 2,
                     qPrintable(QString("Rendered %1, expected %2").arg(pixel.name(), primary.name())));
        }
        CTEST_CHECK(button->setProperty("down", true));
        CTEST_WAIT_COMPARE(background->property("color").value<QColor>(), primary.darker(120));
        const QImage pressed = captureImage(button);
        const QColor pixel = pressed.pixelColor(60, 16);
        const QColor expected = primary.darker(120);
        CTEST_CHECK(qAbs(pixel.red() - expected.red()) <= 2
                && qAbs(pixel.green() - expected.green()) <= 2
                && qAbs(pixel.blue() - expected.blue()) <= 2);
        CTEST_CHECK(button->setProperty("down", false));
    }
}

void PrimaryColorTests::catalog_accent_choices_update_the_app()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    const QString path = QStringLiteral(LVRS_TEST_SOURCE_DIR "/../example/VisualCatalog/qml/Main.qml");
    CTEST_CHECK(QFileInfo::exists(path));
    QScopedPointer<QObject> root(TestUtils::loadQmlFile(engine, path));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry",
                                     Q_ARG(QVariant, QStringLiteral("application-window"))));
    QList<QPair<QString, QColor>> choices;
    for (const auto &entry : AccentFixture::colors()) {
        const auto property = entry.toObject()["property"].toString();
        choices.append({property.mid(6).toLower(), AccentFixture::color(property)});
    }
    for (const auto &choice : choices) {
        const QString name = "catalogPrimaryColor_" + choice.first;
        CTEST_WAIT_DEFAULT(findVisualItem(window->contentItem(), name));
        QQuickItem *button = findVisualItem(window->contentItem(), name);
        CTEST_CHECK(QMetaObject::invokeMethod(button, "clicked"));
        CTEST_COMPARE(color(root.data(), "primaryColor"), choice.second);
        CTEST_COMPARE(color(button, "backgroundColor"), choice.second);
        CTEST_COMPARE(color(root->property("palette").value<QObject *>(), "highlight"), choice.second);
    }
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    previousQtHandler = qInstallMessageHandler(recordQtWarning);
    const auto restoreHandler = qScopeGuard([] { qInstallMessageHandler(previousQtHandler); });
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    PrimaryColorTests tests;
    runner.run("binding_loop_warning_is_caught", [&] {
        bindingLoopObserved.store(false, std::memory_order_relaxed);
        qWarning("Binding loop detected: test warning probe");
        CTEST_CHECK(bindingLoopObserved.load(std::memory_order_relaxed));
    });
    runner.run("default_blue_is_compatible", [&] { checkBindingLoops([&] { tests.default_blue_is_compatible(); }); });
    runner.run("declarative_color:purple", [&] { checkBindingLoops([&] { tests.declarative_color(QByteArray("ApplicationWindow"), QColor("#A571E6")); }); });
    runner.run("declarative_color:orange", [&] { checkBindingLoops([&] { tests.declarative_color(QByteArray("ApplicationWindow"), QColor("#FF9F45")); }); });
    runner.run("declarative_color:black", [&] { checkBindingLoops([&] { tests.declarative_color(QByteArray("ApplicationWindow"), QColor("#000000")); }); });
    runner.run("declarative_color:alpha", [&] { checkBindingLoops([&] { tests.declarative_color(QByteArray("ApplicationWindow"), QColor("#80A571E6")); }); });
    runner.run("declarative_color:bootstrap", [&] { checkBindingLoops([&] { tests.declarative_color(QByteArray("AppBootstrapWindow"), QColor("#A571E6")); }); });
    runner.run("declarative_color:shell", [&] { checkBindingLoops([&] { tests.declarative_color(QByteArray("AppShell"), QColor("#A571E6")); }); });
    runner.run("initial_properties_color", [&] { checkBindingLoops([&] { tests.initial_properties_color(); }); });
    runner.run("palette_can_be_observed_from_qml", [&] { checkBindingLoops([&] { tests.palette_can_be_observed_from_qml(); }); });
    runner.run("runtime_color_updates_controls_and_overlays", [&] { checkBindingLoops([&] { tests.runtime_color_updates_controls_and_overlays(); }); });
    runner.run("secondary_windows_inherit_without_resetting", [&] { checkBindingLoops([&] { tests.secondary_windows_inherit_without_resetting(); }); });
    runner.run("engines_have_independent_themes", [&] { checkBindingLoops([&] { tests.engines_have_independent_themes(); }); });
    runner.run("custom_color_is_rendered", [&] { checkBindingLoops([&] { tests.custom_color_is_rendered(); }); });
    runner.run("catalog_accent_choices_update_the_app", [&] { checkBindingLoops([&] { tests.catalog_accent_choices_update_the_app(); }); });
    return runner.result();
}
