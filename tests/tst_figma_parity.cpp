#include <QtTest>
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
#include <QSignalSpy>
#include <cmath>
#include "test_utils.h"
#include "accent_fixture.h"

class FigmaParityTests : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void tokens_and_icon_names();
    void saved_theme_tokens_data();
    void saved_theme_tokens();
    void compatibility_accent_aliases_remain_fixed_data();
    void compatibility_accent_aliases_remain_fixed();
    void authored_accent_palette_is_rendered();
    void legacy_theme_aliases_data();
    void legacy_theme_aliases();
    void saved_text_style_bindings_data();
    void saved_text_style_bindings();
    void saved_text_style_font_recipes_data();
    void saved_text_style_font_recipes();
    void component_geometry_data();
    void component_geometry();
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
    QVERIFY(label);
    QCOMPARE(label->property("font").value<QFont>().hintingPreference(), QFont::PreferNoHinting);
    const auto width = label->property("implicitWidth").toReal();
    QCOMPARE(width, std::ceil(width));
    const auto rendering = label->property("renderType").toInt();
    label->setProperty("renderType", rendering == 0 ? 1 : 0);
    QCOMPARE(label->property("implicitWidth").toReal(), width);
}

void FigmaParityTests::initTestCase()
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-contract.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    fixture = QJsonDocument::fromJson(file.readAll()).object();
    QVERIFY(!fixture.isEmpty());

    QFile themeFile(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-theme-contract.json"));
    QVERIFY2(themeFile.open(QIODevice::ReadOnly), qPrintable(themeFile.fileName()));
    QJsonParseError parseError;
    const auto themeDocument = QJsonDocument::fromJson(themeFile.readAll(), &parseError);
    QCOMPARE(parseError.error, QJsonParseError::NoError);
    QVERIFY(themeDocument.isObject());
    themeFixture = themeDocument.object();
    QCOMPARE(themeFixture["source"].toString(), QStringLiteral("figma-readback"));
    QCOMPARE(themeFixture["fileKey"].toString(), QStringLiteral("0GkItQYSNIR0lZ3iJhfJzc"));
    QVERIFY(!themeFixture["capturedAtUtc"].toString().isEmpty());
    QVERIFY(themeFixture["validation"].toObject()["passed"].toBool());
    QCOMPARE(themeFixture["validation"].toObject()["errors"].toArray().size(), 0);
    QCOMPARE(themeFixture["variables"].toArray().size(), 73);
    QCOMPARE(themeFixture["textStyles"].toArray().size(), 7);
    QCOMPARE(themeFixture["legacyPanelAliases"].toObject().size(), 12);
    QCOMPARE(themeFixture["legacyTypographyAliases"].toObject().size(), 2);

    int colors = 0;
    int typography = 0;
    for (const auto &entry : themeFixture["variables"].toArray()) {
        const auto variable = entry.toObject();
        QVERIFY2(!variable["name"].toString().startsWith("Action/"), "Deleted Action variables must not remain in the Figma token contract");
        const auto property = variable["property"].toString();
        const auto id = variable["id"].toString();
        QVERIFY2(!property.isEmpty(), qPrintable(id));
        QVERIFY2(!id.isEmpty(), qPrintable(property));
        QVERIFY2(!themeVariablesByProperty.contains(property), qPrintable(property));
        QVERIFY2(!themeVariablesById.contains(id), qPrintable(id));
        QVERIFY2(variable.contains("resolved"), qPrintable(property));
        QCOMPARE(variable["codeSyntax"].toObject()["WEB"].toString(), "Theme." + property);
        QVERIFY2(!variable["hidden"].toBool(), qPrintable(property));
        themeVariablesByProperty.insert(property, variable);
        themeVariablesById.insert(id, variable);
        if (variable["type"].toString() == "COLOR") {
            ++colors;
        } else {
            QVERIFY2(variable["type"].toString() == "FLOAT"
                         || variable["type"].toString() == "STRING", qPrintable(property));
            ++typography;
        }
    }
    QCOMPARE(colors, 37);
    QCOMPARE(typography, 36);
}

void FigmaParityTests::saved_theme_tokens_data()
{
    QTest::addColumn<QJsonObject>("variable");
    for (const auto &entry : themeFixture["variables"].toArray()) {
        const auto variable = entry.toObject();
        QTest::newRow(qPrintable(variable["property"].toString())) << variable;
    }
}

void FigmaParityTests::saved_theme_tokens()
{
    QFETCH(QJsonObject, variable);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    QVERIFY(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    QVERIFY(theme);
    QCOMPARE(theme->property("success"), theme->property("accentGreen"));
    QCOMPARE(theme->property("warning"), theme->property("accentYellow"));
    QCOMPARE(theme->property("danger"), theme->property("accentRed"));
    QCOMPARE(theme->property("defaultPrimary"), theme->property("accentBlue"));
    const auto property = variable["property"].toString();
    const auto propertyName = property.toUtf8();
    QVERIFY2(theme->metaObject()->indexOfProperty(propertyName.constData()) >= 0,
             qPrintable("Missing canonical Theme property: " + property));
    const auto actual = theme->property(propertyName.constData());
    QVERIFY2(actual.isValid(), qPrintable(property));
    const auto type = variable["type"].toString();
    const auto expected = variable.value("resolved");
    if (type == "COLOR") {
        QCOMPARE(actual.metaType(), QMetaType::fromType<QColor>());
        QVERIFY(expected.isObject());
        const QColor color = actual.value<QColor>();
        QVERIFY2(color.isValid(), qPrintable(property));
        const QList<qreal> channels{color.redF(), color.greenF(), color.blueF(), color.alphaF()};
        const QStringList names{"r", "g", "b", "a"};
        for (int i = 0; i < names.size(); ++i) {
            const auto value = expected.toObject().value(names[i]);
            QVERIFY2(value.isDouble(), qPrintable(property + "." + names[i]));
            const double expectedChannel = value.toDouble();
            QVERIFY(qIsFinite(expectedChannel) && expectedChannel >= 0 && expectedChannel <= 1);
            QVERIFY2(qAbs(channels[i] - expectedChannel) <= 1.0 / 65535.0,
                     qPrintable(QString("%1.%2: runtime %3, Figma %4")
                         .arg(property, names[i]).arg(channels[i], 0, 'g', 17)
                         .arg(expectedChannel, 0, 'g', 17)));
        }
    } else if (type == "FLOAT") {
        QVERIFY2(actual.metaType() == QMetaType::fromType<int>()
                     || actual.metaType() == QMetaType::fromType<double>()
                     || actual.metaType() == QMetaType::fromType<float>(), qPrintable(property));
        QVERIFY(expected.isDouble());
        QVERIFY(qIsFinite(actual.toDouble()));
        QCOMPARE(actual.toDouble(), expected.toDouble());
    } else if (type == "STRING") {
        QCOMPARE(actual.metaType(), QMetaType::fromType<QString>());
        QVERIFY(expected.isString());
        QCOMPARE(actual.toString(), expected.toString());
    } else {
        QFAIL(qPrintable("Unsupported saved Figma variable type: " + type));
    }
}

void FigmaParityTests::compatibility_accent_aliases_remain_fixed_data()
{
    QTest::addColumn<QString>("property");
    QTest::addColumn<QColor>("expected");
    const auto aliases = themeFixture["legacyAccentAliases"].toObject();
    QCOMPARE(aliases.size(), 7);
    for (auto it = aliases.begin(); it != aliases.end(); ++it) {
        const auto target = it.value().toString();
        QVERIFY(themeVariablesByProperty.contains(target));
        const auto alias = themeVariablesByProperty.value(it.key())["value"].toObject();
        QCOMPARE(alias["type"].toString(), QStringLiteral("VARIABLE_ALIAS"));
        QCOMPARE(alias["id"].toString(), themeVariablesByProperty.value(target)["id"].toString());
        const QColor expected = AccentFixture::color(target);
        QVERIFY(expected.isValid());
        QTest::newRow(qPrintable(it.key())) << it.key() << expected;
    }
    QTest::newRow("accentPaletteGray") << QStringLiteral("accentPaletteGray") << QColor("#8C8C8C");
}

void FigmaParityTests::compatibility_accent_aliases_remain_fixed()
{
    QFETCH(QString, property);
    QFETCH(QColor, expected);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    QVERIFY(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    QVERIFY(theme);
    const auto propertyName = property.toUtf8();
    QCOMPARE(theme->property(propertyName.constData()).value<QColor>(), expected);
    for (const QColor &primary : {QColor("#A571E6"), QColor("#FF9F45"), QColor("#0A84FF")}) {
        QVERIFY(theme->setProperty("primaryColor", primary));
        QCOMPARE(theme->property("primary").value<QColor>(), primary);
        QCOMPARE(theme->property(propertyName.constData()).value<QColor>(), expected);
    }
}

void FigmaParityTests::authored_accent_palette_is_rendered()
{
    const auto colors = AccentFixture::colors();
    QCOMPARE(colors.size(), 10);
    QCOMPARE(themeFixture["authoredAccentPalette"].toObject()["colors"].toArray().size(), 10);
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
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    auto *palette = root->findChild<QQuickItem *>("palette");
    QVERIFY(palette);
    const auto grab = palette->grabToImage(QSize(400, 40));
    QVERIFY(grab);
    QTRY_VERIFY(!grab->image().isNull());
    const QImage pixels = grab->image();
    for (int i = 0; i < colors.size(); ++i) {
        const auto property = colors[i].toObject()["property"].toString();
        const QColor expected = AccentFixture::color(property);
        const QColor actual = pixels.pixelColor(qRound((i + 0.5) * pixels.width() / 10), pixels.height() / 2);
        QVERIFY2(qAbs(actual.red() - expected.red()) <= 1
                     && qAbs(actual.green() - expected.green()) <= 1
                     && qAbs(actual.blue() - expected.blue()) <= 1
                     && actual.alpha() == 255,
                 qPrintable(property + ": rendered " + actual.name() + ", expected " + expected.name()));
    }
    const QString capturePath = qEnvironmentVariable("LVRS_ACCENT_CAPTURE_PATH");
    if (!capturePath.isEmpty())
        QVERIFY2(pixels.save(capturePath), qPrintable(capturePath));
}

void FigmaParityTests::legacy_theme_aliases_data()
{
    QTest::addColumn<QString>("legacy");
    QTest::addColumn<QString>("canonical");
    const auto panelAliases = themeFixture["legacyPanelAliases"].toObject();
    for (auto it = panelAliases.begin(); it != panelAliases.end(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value().toString();

    const QStringList suffixes{"", "Weight", "StyleName", "LineHeight", "LetterSpacing"};
    const auto typographyAliases = themeFixture["legacyTypographyAliases"].toObject();
    for (auto it = typographyAliases.begin(); it != typographyAliases.end(); ++it) {
        for (const auto &suffix : suffixes)
            QTest::newRow(qPrintable(it.key() + suffix))
                << it.key() + suffix << it.value().toString() + suffix;
    }
}

void FigmaParityTests::legacy_theme_aliases()
{
    QFETCH(QString, legacy);
    QFETCH(QString, canonical);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    QVERIFY(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    QVERIFY(theme);
    const auto legacyName = legacy.toUtf8();
    const auto canonicalName = canonical.toUtf8();
    QVERIFY2(theme->metaObject()->indexOfProperty(legacyName.constData()) >= 0,
             qPrintable("Missing compatibility Theme property: " + legacy));
    QVERIFY2(theme->metaObject()->indexOfProperty(canonicalName.constData()) >= 0,
             qPrintable("Missing canonical Theme property: " + canonical));
    const auto legacyValue = theme->property(legacyName.constData());
    const auto canonicalValue = theme->property(canonicalName.constData());
    QVERIFY(legacyValue.isValid() && canonicalValue.isValid());
    QCOMPARE(legacyValue.metaType(), canonicalValue.metaType());
    QCOMPARE(legacyValue, canonicalValue);
}

void FigmaParityTests::saved_text_style_bindings_data()
{
    QTest::addColumn<QJsonObject>("style");
    QTest::addColumn<QString>("bindingName");
    QTest::addColumn<QString>("property");
    QTest::addColumn<QJsonValue>("styleValue");
    const QHash<QString, QString> prefixes{
        {"Title", "textTitle"}, {"Subtitle", "textSubtitle"}, {"Header", "textHeader"},
        {"Subheader", "textSubheader"}, {"Body", "textBody"},
        {"Description", "textDescription"}, {"Caption", "textCaption"}
    };
    for (const auto &entry : themeFixture["textStyles"].toArray()) {
        const auto style = entry.toObject();
        const auto name = style["name"].toString();
        QVERIFY2(prefixes.contains(name), qPrintable(name));
        const auto prefix = prefixes.value(name);
        const auto font = style["font"].toObject();
        const auto weight = themeVariablesByProperty.value(prefix + "Weight").value("resolved");
        QTest::newRow(qPrintable(name + ":fontFamily"))
            << style << QString("fontFamily") << QString("fontBody") << font["family"];
        QTest::newRow(qPrintable(name + ":fontSize"))
            << style << QString("fontSize") << prefix << style["size"];
        QTest::newRow(qPrintable(name + ":fontWeight"))
            << style << QString("fontWeight") << prefix + "Weight" << weight;
        QTest::newRow(qPrintable(name + ":lineHeight"))
            << style << QString("lineHeight") << prefix + "LineHeight"
            << style["lineHeight"].toObject().value("value");
        QTest::newRow(qPrintable(name + ":letterSpacing"))
            << style << QString("letterSpacing") << prefix + "LetterSpacing"
            << style["letterSpacing"].toObject().value("value");
    }
}

void FigmaParityTests::saved_text_style_bindings()
{
    QFETCH(QJsonObject, style);
    QFETCH(QString, bindingName);
    QFETCH(QString, property);
    QFETCH(QJsonValue, styleValue);
    const auto bindings = style["bindings"].toObject();
    QCOMPARE(bindings.size(), 5);
    QVERIFY2(bindings.contains(bindingName), qPrintable(style["name"].toString() + ":" + bindingName));
    const auto binding = bindings[bindingName].toObject();
    QCOMPARE(binding["type"].toString(), QStringLiteral("VARIABLE_ALIAS"));
    const auto id = binding["id"].toString();
    QVERIFY2(themeVariablesById.contains(id), qPrintable("Unresolved text-style variable: " + id));
    QVERIFY2(themeVariablesByProperty.contains(property), qPrintable(property));
    const auto variable = themeVariablesById.value(id);
    QCOMPARE(variable["property"].toString(), property);
    QCOMPARE(id, themeVariablesByProperty.value(property)["id"].toString());
    QCOMPARE(variable["resolved"], styleValue);
    if (bindingName == "lineHeight" || bindingName == "letterSpacing")
        QCOMPARE(style[bindingName].toObject()["unit"].toString(), QStringLiteral("PIXELS"));

    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    QVERIFY(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    QVERIFY(theme);
    const auto propertyName = property.toUtf8();
    QVERIFY2(theme->metaObject()->indexOfProperty(propertyName.constData()) >= 0,
             qPrintable("Missing text-style Theme property: " + property));
    const auto runtimeValue = theme->property(propertyName.constData());
    QVERIFY(runtimeValue.isValid());
    if (bindingName == "fontFamily") {
        QCOMPARE(variable["type"].toString(), QStringLiteral("STRING"));
        QCOMPARE(runtimeValue.metaType(), QMetaType::fromType<QString>());
        QCOMPARE(runtimeValue.toString(), styleValue.toString());
    } else {
        QCOMPARE(variable["type"].toString(), QStringLiteral("FLOAT"));
        QVERIFY(styleValue.isDouble());
        QCOMPARE(runtimeValue.toDouble(), styleValue.toDouble());
    }
}

void FigmaParityTests::saved_text_style_font_recipes_data()
{
    QTest::addColumn<QJsonObject>("style");
    for (const auto &entry : themeFixture["textStyles"].toArray()) {
        const auto style = entry.toObject();
        QTest::newRow(qPrintable(style["name"].toString())) << style;
    }
}

void FigmaParityTests::saved_text_style_font_recipes()
{
    QFETCH(QJsonObject, style);
    const auto bindings = style["bindings"].toObject();
    // Figma's weight binding selects the static face; the style-name token
    // records the same QML recipe without an incompatible fontStyle binding.
    QVERIFY(!bindings.contains("fontStyle"));
    const auto weightId = bindings["fontWeight"].toObject()["id"].toString();
    QVERIFY(themeVariablesById.contains(weightId));
    const auto weightVariable = themeVariablesById.value(weightId);
    const auto weightProperty = weightVariable["property"].toString();
    QVERIFY(weightProperty.endsWith("Weight"));
    const auto styleNameProperty = weightProperty.chopped(6) + "StyleName";
    QVERIFY(themeVariablesByProperty.contains(styleNameProperty));
    const auto font = style["font"].toObject();
    const auto styleName = font["style"].toString();
    const QHash<QString, int> weights{{"Bold", QFont::Bold}, {"SemiBold", QFont::DemiBold},
                                    {"Medium", QFont::Medium}, {"Regular", QFont::Normal}};
    QVERIFY2(weights.contains(styleName), qPrintable(styleName));
    QCOMPARE(weightVariable["resolved"].toInt(), weights.value(styleName));
    QCOMPARE(themeVariablesByProperty.value(styleNameProperty)["resolved"].toString(), styleName);

    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    QVERIFY(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    QVERIFY(theme);
    const auto stylePropertyName = styleNameProperty.toUtf8();
    const auto weightPropertyName = weightProperty.toUtf8();
    QVERIFY2(theme->metaObject()->indexOfProperty(stylePropertyName.constData()) >= 0,
             qPrintable("Missing text-style Theme property: " + styleNameProperty));
    QVERIFY2(theme->metaObject()->indexOfProperty(weightPropertyName.constData()) >= 0,
             qPrintable("Missing text-style Theme property: " + weightProperty));
    QCOMPARE(theme->property(stylePropertyName.constData()).toString(), styleName);
    QCOMPARE(theme->property(weightPropertyName.constData()).toInt(), weights.value(styleName));
    QCOMPARE(theme->property("fontBody").toString(), font["family"].toString());
    QVERIFY2(QFontDatabase::families().contains(font["family"].toString()),
             qPrintable("Missing bundled text-style family: " + font["family"].toString()));
    const auto installedStyle = QFontDatabase::font(font["family"].toString(), styleName,
                                                  style["size"].toInt());
    QCOMPARE(installedStyle.family(), font["family"].toString());
    QCOMPARE(installedStyle.styleName(), styleName);
    QCOMPARE(int(installedStyle.weight()), weights.value(styleName));
}

void FigmaParityTests::tokens_and_icon_names()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        "import QtQuick; import LVRS as LV; Item { property var theme: LV.Theme }"));
    QVERIFY(root);
    auto *theme = qvariant_cast<QObject *>(root->property("theme"));
    QVERIFY(theme);
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
        QVERIFY2(color.isValid(), qPrintable(name));
        const QList<qreal> actual{color.redF(), color.greenF(), color.blueF(), color.alphaF()};
        const QStringList channels{"r", "g", "b", "a"};
        for (int i = 0; i < channels.size(); ++i)
            QVERIFY2(qAbs(actual[i] - value[channels[i]].toDouble()) < 1.0 / 65535.0,
                     qPrintable(name + "." + channels[i]));
    }
    const auto aliases = fixture["iconAliases"].toObject();
    for (auto it = aliases.begin(); it != aliases.end(); ++it) {
        QQmlExpression expression(qmlContext(root.data()), root.data(),
            "theme.iconPath(" + QString::fromUtf8(QJsonDocument(QJsonArray{it.key()}).toJson(QJsonDocument::Compact)).mid(1).chopped(1) + ")");
        const QString path = expression.evaluate().toString();
        QVERIFY(!expression.hasError());
        QVERIFY2(path.endsWith(it.value().toString() + ".svg"), qPrintable(path));
        QVERIFY2(QFile::exists(QString(path).replace("qrc:", ":")), qPrintable(path));
    }
    const auto assets = fixture["assets"].toObject();
    for (auto it = assets.begin(); it != assets.end(); ++it) {
        QFile file(":/qt/qml/LVRS/resources/" + it.key());
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.fileName()));
        QCOMPARE(QString(QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex()), it.value().toString());
    }
    QVERIFY(QFile::exists(":/qt/qml/LVRS/resources/font/Inter-OFL.txt"));
    QFile canonical(":/qt/qml/LVRS/resources/iconset/nodesTest.svg");
    QFile legacy(":/qt/qml/LVRS/resources/iconset/nodestest.svg");
    QVERIFY(canonical.open(QIODevice::ReadOnly));
    QVERIFY(legacy.open(QIODevice::ReadOnly));
    QCOMPARE(canonical.readAll(), legacy.readAll());
}

void FigmaParityTests::component_geometry_data()
{
    QTest::addColumn<QString>("type");
    QTest::addColumn<double>("width");
    QTest::addColumn<double>("height");
    for (const auto &entry : fixture["geometry"].toArray()) {
        const auto g = entry.toObject();
        QTest::newRow(qPrintable(g["type"].toString())) << g["type"].toString()
            << g["width"].toDouble() << g["height"].toDouble();
    }
}

void FigmaParityTests::component_geometry()
{
    QFETCH(QString, type);
    QFETCH(double, width);
    QFETCH(double, height);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine,
        ("import QtQuick; import LVRS as LV; LV." + type + " {}").toUtf8()));
    QVERIFY(root);
    QTRY_COMPARE(root->property("implicitWidth").toReal(), width);
    QCOMPARE(root->property("implicitHeight").toReal(), height);
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
    QVERIFY(root);
    auto *context = root->findChild<QObject *>("context");
    auto *menu = root->findChild<QObject *>("menu");
    QVERIFY(context && menu);
    QTRY_COMPARE(context->property("implicitWidth").toInt(), 157);
    QTRY_COMPARE(context->property("implicitHeight").toInt(), 119);
    QTRY_COMPARE(menu->property("implicitWidth").toInt(), 161);
    QTRY_COMPARE(menu->property("implicitHeight").toInt(), 224);
    auto *compact = root->findChild<QObject *>("compact");
    auto *regular = root->findChild<QObject *>("regular");
    const auto compactFont = compact->findChild<QObject *>("menuItem_labelNode")->property("font").value<QFont>();
    QCOMPARE(compactFont.family(), QString("Inter"));
    QCOMPARE(compactFont.pixelSize(), 12);
    QCOMPARE(compactFont.weight(), QFont::Normal);
    QVERIFY(QFontDatabase::families().contains("Inter"));
    QCOMPARE(compact->findChild<QObject *>("menuItem_shortcutLabel")->property("font").value<QFont>().weight(), QFont::DemiBold);
    QCOMPARE(regular->findChild<QObject *>("menuItem_labelNode")->property("font").value<QFont>().pixelSize(), 13);
    QSignalSpy triggered(context, SIGNAL(itemTriggered(int,QVariant)));
    QVERIFY(QMetaObject::invokeMethod(context, "triggerEntry", Q_ARG(QVariant, 0)));
    QCOMPARE(triggered.count(), 1);
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
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *button = root->findChild<QQuickItem *>("color");
    auto *help = root->findChild<QQuickItem *>("help");
    QVERIFY(window && button && help);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QSignalSpy clicked(button, SIGNAL(clicked()));
    for (int size : {22, 28, 36}) {
        button->setProperty("buttonSize", size);
        QCOMPARE(button->implicitWidth(), qreal(size));
        QCOMPARE(button->implicitHeight(), qreal(size));
    }
    button->setProperty("currentColor", QColor("#ff453a"));
    QCOMPARE(button->findChild<QObject *>("colorPickerButton_currentColor")->property("color").value<QColor>(), QColor("#ff453a"));
    button->setProperty("showHueRing", false);
    QVERIFY(!button->findChild<QObject *>("colorPickerButton_hueRing")->property("visible").toBool());
    const QPoint center = button->mapToScene(button->boundingRect().center()).toPoint();
    QTest::mousePress(window, Qt::LeftButton, {}, center);
    QTRY_VERIFY(button->property("down").toBool());
    QTRY_VERIFY(button->property("contentMotion").value<QObject *>()->property("xScale").toReal() < 1.0);
    QTest::mouseRelease(window, Qt::LeftButton, {}, center);
    QCOMPARE(clicked.count(), 1);
    help->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyClick(window, Qt::Key_Tab);
    button->forceActiveFocus(Qt::TabFocusReason);
    QTRY_COMPARE(button->findChild<QObject *>("colorPickerButton_background")->property("borderWidth").toReal(), 2.0);
    QTest::keyClick(window, Qt::Key_Space);
    QCOMPARE(clicked.count(), 2);
    button->setEnabled(false);
    QTRY_COMPARE(button->opacity(), 0.32);
    QTest::mouseClick(window, Qt::LeftButton, {}, center);
    QCOMPARE(clicked.count(), 2);
    QSignalSpy helpClicked(help, SIGNAL(clicked()));
    help->forceActiveFocus(Qt::TabFocusReason);
    QTest::keyClick(window, Qt::Key_Space);
    QCOMPARE(helpClicked.count(), 1);
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
    QVERIFY(root);
    QCoreApplication::processEvents();
    const auto findVisual = [](auto &&self, QQuickItem *item, const QString &name) -> QQuickItem * {
        if (item->objectName() == name) return item;
        for (auto *child : item->childItems())
            if (auto *found = self(self, child, name)) return found;
        return nullptr;
    };
    auto *button = findVisual(findVisual, qobject_cast<QQuickItem *>(root.data()), "listFooter_menuButton_2");
    QVERIFY(button);
    QTRY_COMPARE(button->property("leftPadding").toReal(), 7.0);
    QCOMPARE(button->property("rightPadding").toReal(), 6.0);
    QCOMPARE(root->property("implicitWidth").toReal(), 95.0);
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
    QVERIFY(root);
    auto *material = root->findChild<QQuickItem *>("material");
    QCOMPARE(material->property("innerHighlightOpacity").toReal(), 0.14);
    QCOMPARE(material->property("innerHighlightOffsetY").toReal(), 1.0);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(QTest::qWaitForWindowExposed(window));
    if (!material->property("rendererAvailable").toBool())
        QSKIP("Inset highlight rendering requires the native RHI scene graph.");
    QTest::qWait(150);
    auto frame = window->grabWindow();
    QVERIFY(!frame.isNull());
    const auto capture = qEnvironmentVariable("LVRS_MATERIAL_CAPTURE_PATH");
    if (!capture.isEmpty()) QVERIFY(frame.save(capture));
    const qreal ratio = qreal(frame.width()) / window->width();
    // QImage indices address pixel cells. Round-up selects the next row on a
    // 1x display and misses this exactly one-logical-pixel inset highlight.
    const auto top = frame.pixelColor(qFloor(100 * ratio), qFloor(20.5 * ratio));
    const auto body = frame.pixelColor(qRound(100 * ratio), qRound(24 * ratio));
    QVERIFY2(top.red() > body.red() + 20, qPrintable(QString("Top %1, body %2; size %3x%4; DPR %5").arg(top.red()).arg(body.red()).arg(frame.width()).arg(frame.height()).arg(window->devicePixelRatio())));
    material->setProperty("density", 0);
    QCOMPARE(material->property("innerHighlightOpacity").toReal(), 0.06);
    QTest::qWait(150);
    frame = window->grabWindow();
    const auto denseTop = frame.pixelColor(qFloor(100 * ratio), qFloor(20.5 * ratio));
    QVERIFY(denseTop.red() < top.red() - 10);
}

QTEST_MAIN(FigmaParityTests)
#include "tst_figma_parity.moc"
