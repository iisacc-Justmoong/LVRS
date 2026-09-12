#include <QtTest>
#include <QColor>
#include <QCryptographicHash>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include "test_utils.h"

class FigmaParityTests : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void tokens_and_icon_names();
    void component_geometry_data();
    void component_geometry();
    void menu_density_and_typography();
    void button_states_and_activation();
    void footer_overrides();
    void material_inner_highlight();
private:
    QJsonObject fixture;
};

void FigmaParityTests::initTestCase()
{
    QFile file(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/figma-contract.json"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    fixture = QJsonDocument::fromJson(file.readAll()).object();
    QVERIFY(!fixture.isEmpty());
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
        QVERIFY(QFile::exists(QString(path).replace("qrc:", ":")));
    }
    const auto assets = fixture["assets"].toObject();
    for (auto it = assets.begin(); it != assets.end(); ++it) {
        QFile file(":/qt/qml/LVRS/resources/" + it.key());
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.fileName()));
        QCOMPARE(QString(QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha256).toHex()), it.value().toString());
    }
    QVERIFY(QFile::exists(":/qt/qml/LVRS/resources/font/Inter-OFL.txt"));
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
    const qreal ratio = qreal(frame.width()) / window->width();
    const auto top = frame.pixelColor(qRound(100 * ratio), qRound(20.5 * ratio));
    const auto body = frame.pixelColor(qRound(100 * ratio), qRound(24 * ratio));
    QVERIFY2(top.red() > body.red() + 20, qPrintable(QString("Top %1, body %2").arg(top.red()).arg(body.red())));
    material->setProperty("density", 0);
    QCOMPARE(material->property("innerHighlightOpacity").toReal(), 0.06);
    QTest::qWait(150);
    frame = window->grabWindow();
    const auto denseTop = frame.pixelColor(qRound(100 * ratio), qRound(20.5 * ratio));
    QVERIFY(denseTop.red() < top.red() - 10);
}

QTEST_MAIN(FigmaParityTests)
#include "tst_figma_parity.moc"
