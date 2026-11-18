#include "QtEventWait.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <type_traits>
#include <QColor>
#include <QCryptographicHash>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <cmath>
#include "test_utils.h"
#include "accent_fixture.h"

class FigmaSignalRecorder : public QObject
{
    Q_OBJECT
public:
    QList<QPair<int, QVariant>> triggeredEvents;
    int clicked = 0;
public slots:
    void recordTriggered(int index, const QVariant &value) { triggeredEvents.append({index, value}); }
    void recordClicked() { ++clicked; }
};

namespace {
template <typename Actual, typename Expected>
bool figmaEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual) + 1.0, qreal(expected) + 1.0);
    else
        return actual == expected;
}
void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}
void sendMouse(QQuickWindow *window, QEvent::Type type, const QPoint &point, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    static ulong timestamp = 10000;
    const QPointF local(point), global(window->mapToGlobal(point));
    QMouseEvent event(type, local, local, global, button, buttons, Qt::NoModifier);
    event.setTimestamp(timestamp += 16);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}
void mousePress(QQuickWindow *window, const QPoint &point) { sendMouse(window, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton); }
void mouseRelease(QQuickWindow *window, const QPoint &point) { sendMouse(window, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton); }
void mouseClick(QQuickWindow *window, const QPoint &point) { mousePress(window, point); mouseRelease(window, point); }
void keyClick(QQuickWindow *window, Qt::Key key)
{
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier), release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}
}
#define FIGMA_COMPARE(actual, expected) CTEST_CHECK(figmaEqual((actual), (expected)))
#define FIGMA_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(figmaEqual((actual), (expected)))

class FigmaParityTests
{
public:
    void initTestCase();
    void tokens_and_icon_names();
    void run_saved_theme_tokens_rows();
    void saved_theme_tokens(const QJsonObject &variable);
    void run_compatibility_accent_aliases_remain_fixed_rows();
    void compatibility_accent_aliases_remain_fixed(const QString &property, const QColor &expected);
    void authored_accent_palette_is_rendered();
    void accent_role_tokens_match_figma();
    void icon_accent_tokens_match_figma();
    void material_background_tokens_match_figma();
    void run_legacy_theme_aliases_rows();
    void legacy_theme_aliases(const QString &legacy, const QString &canonical);
    void run_saved_text_style_bindings_rows();
    void saved_text_style_bindings(const QJsonObject &style, const QString &bindingName, const QString &property, const QJsonValue &styleValue);
    void run_saved_text_style_font_recipes_rows();
    void saved_text_style_font_recipes(const QJsonObject &style);
    void run_component_geometry_rows();
    void component_geometry(const QString &type, double width, double height);
    void menu_density_and_typography();
    void button_states_and_activation();
    void footer_overrides();
    void material_inner_highlight();
    void text_layout_keeps_design_metrics();
private:
    QJsonObject fixture;
    QJsonObject themeFixture;
    QHash<QString, QJsonObject> themeVariablesByProperty;
    QHash<QString, QJsonObject> themeVariablesById;
};

void FigmaParityTests::text_layout_keeps_design_metrics()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> label(TestUtils::createFromQml(engine,
        "import LVRS as LV\nLV.Label { text: 'Button'; style: body }"));
    CTEST_CHECK(label);
    FIGMA_COMPARE(label->property("font").value<QFont>().hintingPreference(), QFont::PreferNoHinting);
    const auto width = label->property("implicitWidth").toReal();
    FIGMA_COMPARE(width, std::ceil(width));
    const auto rendering = label->property("renderType").toInt();
    label->setProperty("renderType", rendering == 0 ? 1 : 0);
    FIGMA_COMPARE(label->property("implicitWidth").toReal(), width);
}

void FigmaParityTests::initTestCase()
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-contract.json"));
    CTEST_CHECK(file.open(QIODevice::ReadOnly));
    fixture = QJsonDocument::fromJson(file.readAll()).object();
    CTEST_CHECK(!fixture.isEmpty());

    QFile themeFile(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-theme-contract.json"));
    CTEST_CHECK_MESSAGE(themeFile.open(QIODevice::ReadOnly), qPrintable(themeFile.fileName()));
    QJsonParseError parseError;
    const auto themeDocument = QJsonDocument::fromJson(themeFile.readAll(), &parseError);
    FIGMA_COMPARE(parseError.error, QJsonParseError::NoError);
    CTEST_CHECK(themeDocument.isObject());
    themeFixture = themeDocument.object();
    FIGMA_COMPARE(themeFixture["source"].toString(), QStringLiteral("figma-readback"));
    FIGMA_COMPARE(themeFixture["fileKey"].toString(), QStringLiteral("0GkItQYSNIR0lZ3iJhfJzc"));
    CTEST_CHECK(!themeFixture["capturedAtUtc"].toString().isEmpty());
    CTEST_CHECK(themeFixture["validation"].toObject()["passed"].toBool());
    FIGMA_COMPARE(themeFixture["validation"].toObject()["errors"].toArray().size(), 0);
    FIGMA_COMPARE(themeFixture["variables"].toArray().size(), 50);
    FIGMA_COMPARE(themeFixture["textStyles"].toArray().size(), 7);
    FIGMA_COMPARE(themeFixture["legacyPanelAliases"].toObject().size(), 12);
    FIGMA_COMPARE(themeFixture["legacyTypographyAliases"].toObject().size(), 2);

    int colors = 0;
    int typography = 0;
    for (const auto &entry : themeFixture["variables"].toArray()) {
        const auto variable = entry.toObject();
        CTEST_CHECK_MESSAGE(!variable["name"].toString().startsWith("Action/"), "Deleted Action variables must not remain in the Figma token contract");
        const auto property = variable["property"].toString();
        const auto id = variable["id"].toString();
        CTEST_CHECK_MESSAGE(!property.isEmpty(), qPrintable(id));
        CTEST_CHECK_MESSAGE(!id.isEmpty(), qPrintable(property));
        CTEST_CHECK_MESSAGE(!themeVariablesByProperty.contains(property), qPrintable(property));
        CTEST_CHECK_MESSAGE(!themeVariablesById.contains(id), qPrintable(id));
        CTEST_CHECK_MESSAGE(variable.contains("resolved"), qPrintable(property));
        CTEST_CHECK(variable["codeSyntax"].toObject()["WEB"].toString().startsWith("var(--lvrs-"));
        CTEST_CHECK_MESSAGE(!variable["hidden"].toBool(), qPrintable(property));
        themeVariablesByProperty.insert(property, variable);
        themeVariablesById.insert(id, variable);
        if (variable["type"].toString() == "COLOR") {
            ++colors;
        } else {
            CTEST_CHECK_MESSAGE(variable["type"].toString() == "FLOAT"
                         || variable["type"].toString() == "STRING", qPrintable(property));
            ++typography;
        }
    }
    FIGMA_COMPARE(colors, 30);
    FIGMA_COMPARE(typography, 20);
}

void FigmaParityTests::run_saved_theme_tokens_rows()
{
    lvrs_ctest::CTestRunner rows;
    for (const auto &entry : themeFixture["variables"].toArray()) {
        const auto variable = entry.toObject();
        rows.run(variable["property"].toString().toStdString(), [&] { saved_theme_tokens(variable); });
    }
    CTEST_CHECK(rows.result() == 0);
}

void FigmaParityTests::saved_theme_tokens(const QJsonObject &variable)
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    CTEST_CHECK(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    CTEST_CHECK(theme);
    FIGMA_COMPARE(theme->property("success"), theme->property("accentGreen"));
    FIGMA_COMPARE(theme->property("warning"), theme->property("accentYellow"));
    FIGMA_COMPARE(theme->property("danger"), theme->property("accentRed"));
    FIGMA_COMPARE(theme->property("defaultPrimary"), theme->property("accentBlue"));
    const auto property = variable["property"].toString();
    const auto propertyName = property.toUtf8();
    CTEST_CHECK_MESSAGE(theme->metaObject()->indexOfProperty(propertyName.constData()) >= 0,
             qPrintable("Missing canonical Theme property: " + property));
    const auto actual = theme->property(propertyName.constData());
    CTEST_CHECK_MESSAGE(actual.isValid(), qPrintable(property));
    const auto type = variable["type"].toString();
    const auto expected = variable.value("resolved");
    if (type == "COLOR") {
        FIGMA_COMPARE(actual.metaType(), QMetaType::fromType<QColor>());
        CTEST_CHECK(expected.isObject());
        const QColor color = actual.value<QColor>();
        CTEST_CHECK_MESSAGE(color.isValid(), qPrintable(property));
        const QList<qreal> channels{color.redF(), color.greenF(), color.blueF(), color.alphaF()};
        const QStringList names{"r", "g", "b", "a"};
        for (int i = 0; i < names.size(); ++i) {
            const auto value = expected.toObject().value(names[i]);
            CTEST_CHECK_MESSAGE(value.isDouble(), qPrintable(property + "." + names[i]));
            const double expectedChannel = value.toDouble();
            CTEST_CHECK(qIsFinite(expectedChannel) && expectedChannel >= 0 && expectedChannel <= 1);
            CTEST_CHECK_MESSAGE(qAbs(channels[i] - expectedChannel) <= 1.0 / 65535.0,
                     qPrintable(QString("%1.%2: runtime %3, Figma %4")
                         .arg(property, names[i]).arg(channels[i], 0, 'g', 17)
                         .arg(expectedChannel, 0, 'g', 17)));
        }
    } else if (type == "FLOAT") {
        CTEST_CHECK_MESSAGE(actual.metaType() == QMetaType::fromType<int>()
                     || actual.metaType() == QMetaType::fromType<double>()
                     || actual.metaType() == QMetaType::fromType<float>(), qPrintable(property));
        CTEST_CHECK(expected.isDouble());
        CTEST_CHECK(qIsFinite(actual.toDouble()));
        FIGMA_COMPARE(actual.toDouble(), expected.toDouble());
    } else if (type == "STRING") {
        FIGMA_COMPARE(actual.metaType(), QMetaType::fromType<QString>());
        CTEST_CHECK(expected.isString());
        FIGMA_COMPARE(actual.toString(), expected.toString());
    } else {
        ::lvrs_ctest::fail(__FILE__, __LINE__, qPrintable("Unsupported saved Figma variable type: " + type));
    }
}

void FigmaParityTests::run_compatibility_accent_aliases_remain_fixed_rows()
{
    const auto aliases = themeFixture["legacyAccentAliases"].toObject();
    CTEST_COMPARE(aliases.size(), 7);
    lvrs_ctest::CTestRunner rows;
    for (auto it = aliases.begin(); it != aliases.end(); ++it) {
        const auto property = it.key();
        const auto target = it.value().toString();
        CTEST_CHECK(themeVariablesByProperty.contains(target));
        CTEST_CHECK_MESSAGE(!themeVariablesByProperty.contains(property), "Retired Figma aliases must not remain in the variable snapshot");
        const QColor expected = AccentFixture::color(target);
        CTEST_CHECK(expected.isValid());
        rows.run(property.toStdString(), [&] { compatibility_accent_aliases_remain_fixed(property, expected); });
    }
    rows.run("accentPaletteGray", [&] { compatibility_accent_aliases_remain_fixed(QStringLiteral("accentPaletteGray"), QColor("#8C8C8C")); });
    CTEST_CHECK(rows.result() == 0);
}

void FigmaParityTests::compatibility_accent_aliases_remain_fixed(const QString &property, const QColor &expected)
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    CTEST_CHECK(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    CTEST_CHECK(theme);
    const auto propertyName = property.toUtf8();
    FIGMA_COMPARE(theme->property(propertyName.constData()).value<QColor>(), expected);
    for (const QColor &primary : {QColor("#A571E6"), QColor("#FF9F45"), QColor("#0A84FF")}) {
        CTEST_CHECK(theme->setProperty("primaryColor", primary));
        FIGMA_COMPARE(theme->property("primary").value<QColor>(), primary);
        FIGMA_COMPARE(theme->property(propertyName.constData()).value<QColor>(), expected);
    }
}

void FigmaParityTests::authored_accent_palette_is_rendered()
{
    const auto colors = AccentFixture::colors();
    FIGMA_COMPARE(colors.size(), 10);
    FIGMA_COMPARE(themeFixture["authoredAccentPalette"].toObject()["colors"].toArray().size(), 10);
    QByteArray properties;
    for (const auto &entry : colors) {
        if (!properties.isEmpty())
            properties += ',';
        properties += '"' + entry.toObject()["property"].toString().toUtf8() + '"';
    }
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, QByteArray(R"(
import QtQuick
import LVRS as LV
Window {
    width: 400; height: 40; visible: true
    Row {
        objectName: "palette"
        Repeater {
            model: [)") + properties + R"(]
            delegate: Rectangle {
                required property string modelData
                width: 40; height: 40
                color: LV.Theme[modelData]
            }
        }
    }
})"));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    auto *palette = root->findChild<QQuickItem *>("palette");
    CTEST_CHECK(palette);
    const auto grab = palette->grabToImage(QSize(400, 40));
    CTEST_CHECK(grab);
    CTEST_WAIT_DEFAULT(!grab->image().isNull());
    const QImage pixels = grab->image();
    for (int i = 0; i < colors.size(); ++i) {
        const auto property = colors[i].toObject()["property"].toString();
        const QColor expected = AccentFixture::color(property);
        const QColor actual = pixels.pixelColor(qRound((i + 0.5) * pixels.width() / 10), pixels.height() / 2);
        CTEST_CHECK_MESSAGE(qAbs(actual.red() - expected.red()) <= 1
                     && qAbs(actual.green() - expected.green()) <= 1
                     && qAbs(actual.blue() - expected.blue()) <= 1
                     && actual.alpha() == 255,
                 qPrintable(property + ": rendered " + actual.name() + ", expected " + expected.name()));
    }
    const QString capturePath = qEnvironmentVariable("LVRS_ACCENT_CAPTURE_PATH");
    if (!capturePath.isEmpty())
        CTEST_CHECK_MESSAGE(pixels.save(capturePath), qPrintable(capturePath));
}

void FigmaParityTests::accent_role_tokens_match_figma()
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-accent-roles.json"));
    CTEST_CHECK(file.open(QIODevice::ReadOnly));
    const auto fixture = QJsonDocument::fromJson(file.readAll()).object();
    const auto variables = fixture["variables"].toArray();
    FIGMA_COMPARE(variables.size(), 90);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { function role(c, r) { return LV.Theme.accentRoleColor(c, r) } }"));
    CTEST_CHECK(root);
    for (const auto &entry : variables) {
        const auto token = entry.toObject();
        const auto path = token["name"].toString().split('/');
        FIGMA_COMPARE(path.size(), 3);
        QVariant result;
        CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "role", Q_RETURN_ARG(QVariant, result),
                                          Q_ARG(QVariant, path[1]), Q_ARG(QVariant, path[2])));
        const QColor actual = result.value<QColor>();
        CTEST_CHECK_MESSAGE(actual.isValid(), qPrintable(token["name"].toString()));
        const auto expected = token["value"].toObject();
        const QList<qreal> channels{actual.redF(), actual.greenF(), actual.blueF(), actual.alphaF()};
        const QStringList keys{"r", "g", "b", "a"};
        for (int index = 0; index < keys.size(); ++index)
            CTEST_CHECK_MESSAGE(qAbs(channels[index] - expected[keys[index]].toDouble()) <= 1.0 / 65535.0,
                     qPrintable(token["name"].toString() + "." + keys[index]));
    }
}

void FigmaParityTests::icon_accent_tokens_match_figma()
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-accent-roles.json"));
    CTEST_CHECK(file.open(QIODevice::ReadOnly));
    const auto variables = QJsonDocument::fromJson(file.readAll()).object()["iconVariables"].toArray();
    FIGMA_COMPARE(variables.size(), 29);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { function role(c, r) { return LV.Theme.iconAccentRoleColor(c, r) } }"));
    CTEST_CHECK(root);
    for (const auto &entry : variables) {
        const auto token = entry.toObject();
        const auto path = token["name"].toString().split('/');
        FIGMA_COMPARE(path.size(), 5);
        QVariant result;
        CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "role", Q_RETURN_ARG(QVariant, result),
                                          Q_ARG(QVariant, path[2]), Q_ARG(QVariant, path[3])));
        const QColor actual = result.value<QColor>();
        CTEST_CHECK_MESSAGE(actual.isValid(), qPrintable(token["name"].toString()));
        const auto expected = token["value"].toObject();
        const QList<qreal> channels{actual.redF(), actual.greenF(), actual.blueF(), actual.alphaF()};
        const QStringList keys{"r", "g", "b", "a"};
        for (int i = 0; i < keys.size(); ++i)
            CTEST_CHECK_MESSAGE(qAbs(channels[i] - expected[keys[i]].toDouble()) <= 2.0 / 65535.0,
                     qPrintable(token["name"].toString() + "." + keys[i]));
    }
}

void FigmaParityTests::material_background_tokens_match_figma()
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-accent-roles.json"));
    CTEST_CHECK(file.open(QIODevice::ReadOnly));
    const auto variables = QJsonDocument::fromJson(file.readAll()).object()["backgroundVariables"].toArray();
    FIGMA_COMPARE(variables.size(), 22);
    const QHash<QString, QString> mapping{
        {"Glass/Fill", "alertGlassTint"}, {"TextField/Fill", "inputFieldGlassTint"},
        {"TextField/Disabled", "inputFieldGlassTintDisabled"}, {"TextField/Inline", "inputFieldGlassTintInline"},
        {"Surface/Dense/Fill", "materialDenseFill"}, {"Surface/Glass/Fill", "materialGlassFill"},
        {"Backdrop/Base", "materialBackdropBase"}, {"Alert/canvas", "alertCanvas"}, {"Alert/surface", "alertSurface"}
    };
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    CTEST_CHECK(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    CTEST_CHECK(theme);
    int checked = 0;
    for (const auto &entry : variables) {
        const auto token = entry.toObject();
        const QString name = token["name"].toString();
        if (!mapping.contains(name)) continue;
        const auto property = mapping.value(name).toUtf8();
        const QColor actual = theme->property(property.constData()).value<QColor>();
        CTEST_CHECK_MESSAGE(actual.isValid(), qPrintable(name));
        const auto expected = token["value"].toObject();
        const QList<qreal> channels{actual.redF(), actual.greenF(), actual.blueF(), actual.alphaF()};
        const QStringList keys{"r", "g", "b", "a"};
        for (int i = 0; i < keys.size(); ++i)
            CTEST_CHECK_MESSAGE(qAbs(channels[i] - expected[keys[i]].toDouble()) <= 1.0 / 65535.0,
                     qPrintable(name + "." + keys[i]));
        ++checked;
    }
    FIGMA_COMPARE(checked, mapping.size());
    const QColor windowFill = theme->property("materialWindowFill").value<QColor>();
    const QColor denseFill = theme->property("materialDenseFill").value<QColor>();
    FIGMA_COMPARE(windowFill.red(), denseFill.red());
    FIGMA_COMPARE(windowFill.green(), denseFill.green());
    FIGMA_COMPARE(windowFill.blue(), denseFill.blue());
    FIGMA_COMPARE(windowFill.alpha(), 255);
}

void FigmaParityTests::run_legacy_theme_aliases_rows()
{
    lvrs_ctest::CTestRunner rows;
    const auto panelAliases = themeFixture["legacyPanelAliases"].toObject();
    for (auto it = panelAliases.begin(); it != panelAliases.end(); ++it) {
        const auto legacy = it.key(), canonical = it.value().toString();
        rows.run(legacy.toStdString(), [&] { legacy_theme_aliases(legacy, canonical); });
    }
    const QStringList suffixes{"", "Weight", "StyleName", "LineHeight", "LetterSpacing"};
    const auto typographyAliases = themeFixture["legacyTypographyAliases"].toObject();
    for (auto it = typographyAliases.begin(); it != typographyAliases.end(); ++it) {
        for (const auto &suffix : suffixes) {
            const auto legacy = it.key() + suffix, canonical = it.value().toString() + suffix;
            rows.run(legacy.toStdString(), [&] { legacy_theme_aliases(legacy, canonical); });
        }
    }
    CTEST_CHECK(rows.result() == 0);
}

void FigmaParityTests::legacy_theme_aliases(const QString &legacy, const QString &canonical)
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    CTEST_CHECK(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    CTEST_CHECK(theme);
    const auto legacyName = legacy.toUtf8();
    const auto canonicalName = canonical.toUtf8();
    CTEST_CHECK_MESSAGE(theme->metaObject()->indexOfProperty(legacyName.constData()) >= 0,
             qPrintable("Missing compatibility Theme property: " + legacy));
    CTEST_CHECK_MESSAGE(theme->metaObject()->indexOfProperty(canonicalName.constData()) >= 0,
             qPrintable("Missing canonical Theme property: " + canonical));
    const auto legacyValue = theme->property(legacyName.constData());
    const auto canonicalValue = theme->property(canonicalName.constData());
    CTEST_CHECK(legacyValue.isValid() && canonicalValue.isValid());
    FIGMA_COMPARE(legacyValue.metaType(), canonicalValue.metaType());
    FIGMA_COMPARE(legacyValue, canonicalValue);
}

void FigmaParityTests::run_saved_text_style_bindings_rows()
{
    const QHash<QString, QString> prefixes{
        {"Title", "textTitle"}, {"Subtitle", "textSubtitle"}, {"Header", "textHeader"},
        {"Subheader", "textSubheader"}, {"Body", "textBody"},
        {"Description", "textDescription"}, {"Caption", "textCaption"}};
    lvrs_ctest::CTestRunner rows;
    for (const auto &entry : themeFixture["textStyles"].toArray()) {
        const auto style = entry.toObject();
        const auto name = style["name"].toString();
        CTEST_CHECK_MESSAGE(prefixes.contains(name), qPrintable(name));
        const auto prefix = prefixes.value(name);
        const auto font = style["font"].toObject();
        const auto weightId = style["bindings"].toObject()["fontWeight"].toObject()["id"].toString();
        CTEST_CHECK(themeVariablesById.contains(weightId));
        const auto weight = themeVariablesById.value(weightId).value("resolved");
        const auto runRow = [&](const QString &binding, const QString &property, const QJsonValue &value) {
            const auto tag = name + ":" + binding;
            rows.run(tag.toStdString(), [&] { saved_text_style_bindings(style, binding, property, value); });
        };
        runRow("fontFamily", "fontBody", font["family"]);
        runRow("fontSize", prefix, style["size"]);
        runRow("fontWeight", prefix + "Weight", weight);
        runRow("lineHeight", prefix + "LineHeight", style["lineHeight"].toObject().value("value"));
        runRow("letterSpacing", prefix + "LetterSpacing", style["letterSpacing"].toObject().value("value"));
    }
    CTEST_CHECK(rows.result() == 0);
}

void FigmaParityTests::saved_text_style_bindings(const QJsonObject &style, const QString &bindingName, const QString &property, const QJsonValue &styleValue)
{
    const auto bindings = style["bindings"].toObject();
    FIGMA_COMPARE(bindings.size(), 5);
    CTEST_CHECK_MESSAGE(bindings.contains(bindingName), qPrintable(style["name"].toString() + ":" + bindingName));
    const auto binding = bindings[bindingName].toObject();
    FIGMA_COMPARE(binding["type"].toString(), QStringLiteral("VARIABLE_ALIAS"));
    const auto id = binding["id"].toString();
    CTEST_CHECK_MESSAGE(themeVariablesById.contains(id), qPrintable("Unresolved text-style variable: " + id));
    const auto variable = themeVariablesById.value(id);
    FIGMA_COMPARE(variable["resolved"], styleValue);
    if (bindingName == "lineHeight" || bindingName == "letterSpacing")
        FIGMA_COMPARE(style[bindingName].toObject()["unit"].toString(), QStringLiteral("PIXELS"));

    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    CTEST_CHECK(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    CTEST_CHECK(theme);
    const auto propertyName = property.toUtf8();
    CTEST_CHECK_MESSAGE(theme->metaObject()->indexOfProperty(propertyName.constData()) >= 0,
             qPrintable("Missing text-style Theme property: " + property));
    const auto runtimeValue = theme->property(propertyName.constData());
    CTEST_CHECK(runtimeValue.isValid());
    if (bindingName == "fontFamily") {
        FIGMA_COMPARE(variable["type"].toString(), QStringLiteral("STRING"));
        FIGMA_COMPARE(runtimeValue.metaType(), QMetaType::fromType<QString>());
        FIGMA_COMPARE(runtimeValue.toString(), styleValue.toString());
    } else {
        FIGMA_COMPARE(variable["type"].toString(), QStringLiteral("FLOAT"));
        CTEST_CHECK(styleValue.isDouble());
        FIGMA_COMPARE(runtimeValue.toDouble(), styleValue.toDouble());
    }
}

void FigmaParityTests::run_saved_text_style_font_recipes_rows()
{
    lvrs_ctest::CTestRunner rows;
    for (const auto &entry : themeFixture["textStyles"].toArray()) {
        const auto style = entry.toObject();
        rows.run(style["name"].toString().toStdString(), [&] { saved_text_style_font_recipes(style); });
    }
    CTEST_CHECK(rows.result() == 0);
}

void FigmaParityTests::saved_text_style_font_recipes(const QJsonObject &style)
{
    const auto bindings = style["bindings"].toObject();
    // Figma's shared weight binding selects the face; the runtime retains
    // style-name compatibility properties without a separate Figma variable.
    CTEST_CHECK(!bindings.contains("fontStyle"));
    const auto weightId = bindings["fontWeight"].toObject()["id"].toString();
    CTEST_CHECK(themeVariablesById.contains(weightId));
    const auto weightVariable = themeVariablesById.value(weightId);
    const QHash<QString, QString> prefixes{{"Title", "textTitle"}, {"Subtitle", "textSubtitle"},
        {"Header", "textHeader"}, {"Subheader", "textSubheader"}, {"Body", "textBody"},
        {"Description", "textDescription"}, {"Caption", "textCaption"}};
    const auto prefix = prefixes.value(style["name"].toString());
    CTEST_CHECK(!prefix.isEmpty());
    const auto styleNameProperty = prefix + "StyleName";
    const auto weightProperty = prefix + "Weight";
    const auto font = style["font"].toObject();
    const auto styleName = font["style"].toString();
    const QHash<QString, int> weights{{"Bold", QFont::Bold}, {"SemiBold", QFont::DemiBold},
                                    {"Medium", QFont::Medium}, {"Regular", QFont::Normal}};
    CTEST_CHECK_MESSAGE(weights.contains(styleName), qPrintable(styleName));
    FIGMA_COMPARE(weightVariable["resolved"].toInt(), weights.value(styleName));

    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    CTEST_CHECK(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    CTEST_CHECK(theme);
    const auto stylePropertyName = styleNameProperty.toUtf8();
    const auto weightPropertyName = weightProperty.toUtf8();
    CTEST_CHECK_MESSAGE(theme->metaObject()->indexOfProperty(stylePropertyName.constData()) >= 0,
             qPrintable("Missing text-style Theme property: " + styleNameProperty));
    CTEST_CHECK_MESSAGE(theme->metaObject()->indexOfProperty(weightPropertyName.constData()) >= 0,
             qPrintable("Missing text-style Theme property: " + weightProperty));
    FIGMA_COMPARE(theme->property(stylePropertyName.constData()).toString(), styleName);
    FIGMA_COMPARE(theme->property(weightPropertyName.constData()).toInt(), weights.value(styleName));
    FIGMA_COMPARE(theme->property("fontBody").toString(), font["family"].toString());
    CTEST_CHECK_MESSAGE(QFontDatabase::families().contains(font["family"].toString()),
             qPrintable("Missing bundled text-style family: " + font["family"].toString()));
    const auto installedStyle = QFontDatabase::font(font["family"].toString(), styleName,
                                                  style["size"].toInt());
    FIGMA_COMPARE(installedStyle.family(), font["family"].toString());
    FIGMA_COMPARE(installedStyle.styleName(), styleName);
    FIGMA_COMPARE(int(installedStyle.weight()), weights.value(styleName));
}

void FigmaParityTests::tokens_and_icon_names()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    CTEST_CHECK(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    CTEST_CHECK(theme);
    const QHash<QString, QString> textNames = {
        {"TitleHeader", "titleHeaderColor"}, {"Body", "bodyColor"},
        {"Description", "descriptionColor"}, {"Caption", "captionColor"},
        {"Disabled", "disabledColor"}, {"accent", "defaultPrimary"}
    };
    for (const auto &entry : fixture["tokens"].toArray()) {
        const auto token = entry.toObject();
        const auto name = token["name"].toString();
        const auto value = token["value"].toObject();
        const QColor color = theme->property(textNames.value(name, name).toUtf8()).value<QColor>();
        CTEST_CHECK_MESSAGE(color.isValid(), qPrintable(name));
        const QList<qreal> actual{color.redF(), color.greenF(), color.blueF(), color.alphaF()};
        const QStringList channels{"r", "g", "b", "a"};
        for (int i = 0; i < channels.size(); ++i)
            CTEST_CHECK_MESSAGE(qAbs(actual[i] - value[channels[i]].toDouble()) < 1.0 / 65535.0,
                     qPrintable(name + "." + channels[i]));
    }
    const auto aliases = fixture["iconAliases"].toObject();
    for (auto it = aliases.begin(); it != aliases.end(); ++it) {
        QQmlExpression expression(qmlContext(root.data()), root.data(),
            "theme.iconPath(" + QString::fromUtf8(QJsonDocument(QJsonArray{it.key()}).toJson(QJsonDocument::Compact)).mid(1).chopped(1) + ")");
        const QString path = expression.evaluate().toString();
        CTEST_CHECK(!expression.hasError());
        CTEST_CHECK_MESSAGE(path.endsWith(it.value().toString() + ".svg"), qPrintable(path));
        CTEST_CHECK_MESSAGE(QFile::exists(QString(path).replace("qrc:", ":")), qPrintable(path));
    }
    const auto assets = fixture["assets"].toObject();
    for (auto it = assets.begin(); it != assets.end(); ++it) {
        QFile file(":/qt/qml/LVRS/resources/" + it.key());
        CTEST_CHECK_MESSAGE(file.open(QIODevice::ReadOnly), qPrintable(file.fileName()));
        FIGMA_COMPARE(QString(QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex()), it.value().toString());
    }
    CTEST_CHECK(QFile::exists(":/qt/qml/LVRS/resources/font/Inter-OFL.txt"));
    QFile canonical(":/qt/qml/LVRS/resources/iconset/nodesTest.svg");
    QFile legacy(":/qt/qml/LVRS/resources/iconset/nodestest.svg");
    CTEST_CHECK(canonical.open(QIODevice::ReadOnly));
    CTEST_CHECK(legacy.open(QIODevice::ReadOnly));
    FIGMA_COMPARE(canonical.readAll(), legacy.readAll());
}

void FigmaParityTests::run_component_geometry_rows()
{
    lvrs_ctest::CTestRunner rows;
    for (const auto &entry : fixture["geometry"].toArray()) {
        const auto geometry = entry.toObject();
        const auto type = geometry["type"].toString();
        const auto width = geometry["width"].toDouble(), height = geometry["height"].toDouble();
        rows.run(type.toStdString(), [&] { component_geometry(type, width, height); });
    }
    CTEST_CHECK(rows.result() == 0);
}

void FigmaParityTests::component_geometry(const QString &type, double width, double height)
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        ("import QtQuick; import LVRS as LV; LV." + type + " {}").toUtf8()));
    CTEST_CHECK(root);
    FIGMA_WAIT_COMPARE(root->property("implicitWidth").toReal(), width);
    FIGMA_COMPARE(root->property("implicitHeight").toReal(), height);
}

void FigmaParityTests::menu_density_and_typography()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    width: 600; height: 400; visible: true
    LV.ContextMenuItem { objectName: "compact"; y: 10 }
    LV.MenuItem { objectName: "regular"; y: 50 }
    LV.ContextMenu { objectName: "context"; items: [
        {label: "Label", key: "Key"}, {label: "Label", key: "Key"},
        {type: "divider"}, {label: "Label", key: "Key"},
        {label: "Label", key: "Key"}, {label: "Label", key: "Key"}] }
    LV.Menu { objectName: "menu"; items: [
        {label: "Label"}, {label: "Label"}, {label: "Label"}, {type: "divider"},
        {label: "Label"}, {label: "Label"}, {label: "Label"}, {type: "divider"},
        {label: "Label"}, {label: "Label"}] }
})"));
    CTEST_CHECK(root);
    auto *context = root->findChild<QObject *>("context");
    auto *menu = root->findChild<QObject *>("menu");
    CTEST_CHECK(context && menu);
    FIGMA_WAIT_COMPARE(context->property("implicitWidth").toInt(), 157);
    FIGMA_WAIT_COMPARE(context->property("implicitHeight").toInt(), 119);
    FIGMA_WAIT_COMPARE(menu->property("implicitWidth").toInt(), 161);
    FIGMA_WAIT_COMPARE(menu->property("implicitHeight").toInt(), 224);
    auto *compact = root->findChild<QObject *>("compact");
    auto *regular = root->findChild<QObject *>("regular");
    const auto compactFont = compact->findChild<QObject *>("menuItem_labelNode")->property("font").value<QFont>();
    FIGMA_COMPARE(compactFont.family(), QString("Inter"));
    FIGMA_COMPARE(compactFont.pixelSize(), 12);
    FIGMA_COMPARE(compactFont.weight(), QFont::Normal);
    CTEST_CHECK(QFontDatabase::families().contains("Inter"));
    FIGMA_COMPARE(compact->findChild<QObject *>("menuItem_shortcutLabel")->property("font").value<QFont>().weight(), QFont::DemiBold);
    FIGMA_COMPARE(regular->findChild<QObject *>("menuItem_labelNode")->property("font").value<QFont>().pixelSize(), 13);
    FigmaSignalRecorder triggered;
    const auto triggeredConnection = QObject::connect(context, SIGNAL(itemTriggered(int,QVariant)), &triggered, SLOT(recordTriggered(int,QVariant)));
    CTEST_CHECK(triggeredConnection);
    CTEST_CHECK(QMetaObject::invokeMethod(context, "triggerEntry", Q_ARG(QVariant, 0)));
    FIGMA_COMPARE(triggered.triggeredEvents.size(), 1);
}

void FigmaParityTests::button_states_and_activation()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    width: 240; height: 120; visible: true
    LV.HelpButton { objectName: "help"; x: 20; y: 30 }
    LV.ColorPickerButton { objectName: "color"; x: 80; y: 30; hoverEnabled: false }
})"));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("color");
    auto *help = root->findChild<QQuickItem *>("help");
    CTEST_CHECK(window && button && help);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    FigmaSignalRecorder clicked;
    const auto clickedConnection = QObject::connect(button, SIGNAL(clicked()), &clicked, SLOT(recordClicked()));
    CTEST_CHECK(clickedConnection);
    for (int size : {22, 28, 36}) {
        button->setProperty("buttonSize", size);
        FIGMA_COMPARE(button->implicitWidth(), qreal(size));
        FIGMA_COMPARE(button->implicitHeight(), qreal(size));
    }
    button->setProperty("currentColor", QColor("#ff453a"));
    FIGMA_COMPARE(button->findChild<QObject *>("colorPickerButton_currentColor")->property("color").value<QColor>(), QColor("#ff453a"));
    button->setProperty("showHueRing", false);
    CTEST_CHECK(!button->findChild<QObject *>("colorPickerButton_hueRing")->property("visible").toBool());
    const QPoint center = button->mapToScene(button->boundingRect().center()).toPoint();
    mousePress(window, center);
    CTEST_WAIT_DEFAULT(button->property("down").toBool());
    CTEST_WAIT_DEFAULT(button->property("contentMotion").value<QObject *>()->property("xScale").toReal() < 1.0);
    mouseRelease(window, center);
    FIGMA_COMPARE(clicked.clicked, 1);
    help->forceActiveFocus(Qt::TabFocusReason);
    keyClick(window, Qt::Key_Tab);
    button->forceActiveFocus(Qt::TabFocusReason);
    FIGMA_WAIT_COMPARE(button->findChild<QObject *>("colorPickerButton_background")->property("borderWidth").toReal(), 2.0);
    keyClick(window, Qt::Key_Space);
    FIGMA_COMPARE(clicked.clicked, 2);
    button->setEnabled(false);
    FIGMA_WAIT_COMPARE(button->opacity(), 0.32);
    mouseClick(window, center);
    FIGMA_COMPARE(clicked.clicked, 2);
    FigmaSignalRecorder helpClicked;
    const auto helpClickedConnection = QObject::connect(help, SIGNAL(clicked()), &helpClicked, SLOT(recordClicked()));
    CTEST_CHECK(helpClickedConnection);
    help->forceActiveFocus(Qt::TabFocusReason);
    keyClick(window, Qt::Key_Space);
    FIGMA_COMPARE(helpClicked.clicked, 1);
}

void FigmaParityTests::footer_overrides()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ListFooter { button3: ({type: "menu", props: {horizontalPadding: 6, leftPadding: 7}}) }
)"));
    CTEST_CHECK(root);
    QCoreApplication::processEvents();
    const auto findVisual = [](auto &&self, QQuickItem *item, const QString &name) -> QQuickItem * {
        if (item->objectName() == name) return item;
        for (auto *child : item->childItems())
            if (auto *found = self(self, child, name)) return found;
        return nullptr;
    };
    auto *button = findVisual(findVisual, qobject_cast<QQuickItem *>(root.data()), "listFooter_menuButton_2");
    CTEST_CHECK(button);
    FIGMA_WAIT_COMPARE(button->property("leftPadding").toReal(), 7.0);
    FIGMA_COMPARE(button->property("rightPadding").toReal(), 6.0);
    FIGMA_COMPARE(root->property("implicitWidth").toReal(), 95.0);
}

void FigmaParityTests::material_inner_highlight()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV
Controls.ApplicationWindow {
    width: 200; height: 120; visible: true; color: "black"
    LV.PanelMaterial {
        objectName: "material"; x: 20; y: 20; width: 160; height: 80
        color: "black"; tintOpacity: 1; primaryColor: "transparent"
        borderWidth: 0; shadowOpacity: 0
    }
})"));
    CTEST_CHECK(root);
    auto *material = root->findChild<QQuickItem *>("material");
    FIGMA_COMPARE(material->property("innerHighlightOpacity").toReal(), 0.14);
    FIGMA_COMPARE(material->property("innerHighlightOffsetY").toReal(), 1.0);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    if (!material->property("rendererAvailable").toBool())
        lvrs_ctest::skip("Inset highlight rendering requires the native RHI scene graph.");
    processFor(150);
    auto frame = window->grabWindow();
    CTEST_CHECK(!frame.isNull());
    const auto capture = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_PATH");
    if (!capture.isEmpty()) CTEST_CHECK(frame.save(capture));
    const qreal ratio = qreal(frame.width()) / window->width();
    // QImage indices address pixel cells. Round-up selects the next row on a
    // 1x display and misses this exactly one-logical-pixel inset highlight.
    const auto top = frame.pixelColor(qFloor(100 * ratio), qFloor(20.5 * ratio));
    const auto body = frame.pixelColor(qRound(100 * ratio), qRound(24 * ratio));
    CTEST_CHECK_MESSAGE(top.red() > body.red() + 20, qPrintable(QString("Top %1, body %2; size %3x%4; DPR %5").arg(top.red()).arg(body.red()).arg(frame.width()).arg(frame.height()).arg(window->devicePixelRatio())));
    material->setProperty("density", 0);
    FIGMA_COMPARE(material->property("innerHighlightOpacity").toReal(), 0.06);
    processFor(150);
    frame = window->grabWindow();
    const auto denseTop = frame.pixelColor(qFloor(100 * ratio), qFloor(20.5 * ratio));
    CTEST_CHECK(denseTop.red() < top.red() - 10);
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    FigmaParityTests tests;
    try { tests.initTestCase(); }
    catch (const std::exception &error) { std::cerr << "Figma fixture: " << error.what() << '\n'; return 1; }
    runner.run("tokens_and_icon_names", [&] { tests.tokens_and_icon_names(); });
    runner.run("saved_theme_tokens", [&] { tests.run_saved_theme_tokens_rows(); });
    runner.run("compatibility_accent_aliases_remain_fixed", [&] { tests.run_compatibility_accent_aliases_remain_fixed_rows(); });
    runner.run("authored_accent_palette_is_rendered", [&] { tests.authored_accent_palette_is_rendered(); });
    runner.run("accent_role_tokens_match_figma", [&] { tests.accent_role_tokens_match_figma(); });
    runner.run("icon_accent_tokens_match_figma", [&] { tests.icon_accent_tokens_match_figma(); });
    runner.run("material_background_tokens_match_figma", [&] { tests.material_background_tokens_match_figma(); });
    runner.run("legacy_theme_aliases", [&] { tests.run_legacy_theme_aliases_rows(); });
    runner.run("saved_text_style_bindings", [&] { tests.run_saved_text_style_bindings_rows(); });
    runner.run("saved_text_style_font_recipes", [&] { tests.run_saved_text_style_font_recipes_rows(); });
    runner.run("component_geometry", [&] { tests.run_component_geometry_rows(); });
    runner.run("menu_density_and_typography", [&] { tests.menu_density_and_typography(); });
    runner.run("button_states_and_activation", [&] { tests.button_states_and_activation(); });
    runner.run("footer_overrides", [&] { tests.footer_overrides(); });
    runner.run("material_inner_highlight", [&] { tests.material_inner_highlight(); });
    runner.run("text_layout_keeps_design_metrics", [&] { tests.text_layout_keeps_design_metrics(); });
    return runner.result();
}
#include "tst_figma_parity.moc"
