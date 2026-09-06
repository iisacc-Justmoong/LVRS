#include <QtTest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QQuickItemGrabResult>

#include "test_utils.h"

namespace {
QQuickItem *visibleItem(QObject *root, const QString &name)
{
    auto *item = qobject_cast<QQuickItem *>(root);
    if (!item || !item->isVisible())
        return nullptr;
    if (item->objectName() == name)
        return item;
    for (auto *child : item->childItems()) {
        if (auto *match = visibleItem(child, name))
            return match;
    }
    return nullptr;
}

void click(QQuickWindow &window, QQuickItem *item, QPointF local = QPointF(-1, -1))
{
    if (!item) {
        QTest::qFail("Expected a visible interactive control", __FILE__, __LINE__);
        return;
    }
    if (local.x() < 0)
        local = QPointF(item->width() / 2, item->height() / 2);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, item->mapToScene(local).toPoint());
}
}

class ListCompositeTests : public QObject
{
    Q_OBJECT

private slots:
    void figma_variants_data();
    void figma_variants();
    void mixed_list_uses_each_rows_height();
    void controls_deliver_real_edits_and_actions();
    void model_edits_keep_the_live_delegate();
    void long_content_and_custom_composition_fit();
    void variant_changes_reflow_and_keep_footer_fixed();
    void render_catalog_has_all_seventeen_rows();
    void visual_catalog_list_preview_loads();
};

void ListCompositeTests::figma_variants_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    QTest::addColumn<bool>("mobile");
    const char *names[] = {"Mini", "Detail", "Navigation", "Toggle", "Checkable",
                          "Action", "ActionGroup", "Stepper", "Select", "InlineEdit",
                          "DetailActions", "DetailQuantity", "DetailSettings", "Resource",
                          "Media", "Task", "Form"};
    const int widths[] = {170, 194, 280, 280, 280, 400, 400, 400, 400, 400,
                          400, 400, 400, 400, 400, 400, 400};
    const int heights[] = {22, 106, 44, 44, 44, 44, 44, 44, 44, 44,
                           120, 118, 159, 129, 113, 135, 148};
    for (int index = 0; index < 17; ++index) {
        QTest::newRow(names[index]) << index << widths[index] << heights[index] << false;
        QTest::newRow((QByteArray(names[index]) + "-mobile").constData())
            << index << widths[index] << heights[index] << true;
    }
}

void ListCompositeTests::controls_deliver_real_edits_and_actions()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(500, 900);
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
Item {
    id: root
    width: 500
    height: 900
    property int methodCalls: 0
    property int rowClicks: 0
    property int menuCalls: 0
    property string savedText: ""
    function setStepperMode(mode) {
        quantityRow.stepper = mode === "disabled" ? ({tone: LV.AbstractButton.Disabled})
            : mode === "up" ? ({arrow: LV.Stepper.Up}) : ({arrow: LV.Stepper.Down})
    }
    LV.ListItem {
        objectName: "actions"
        type: LV.ListItem.ActionGroup
        primaryAction: ({ text: "Run", method: function(event) { root.methodCalls += 1 } })
        moreMenu: ({ items: [{ label: "Duplicate", onTriggered: function() { root.menuCalls += 1 } }] })
        onClicked: root.rowClicks += 1
    }
    LV.ListItem {
        id: quantityRow
        objectName: "quantity"
        y: 60
        type: LV.ListItem.DetailQuantity
        quantity: 2
        minimumQuantity: 1
        maximumQuantity: 3
        selector: ({ items: ["PNG", "JPEG"] })
        unitSelector: ({ items: ["Pixels", "Percent"] })
    }
    LV.ListItem {
        objectName: "form"
        y: 200
        type: LV.ListItem.Form
        inputText1: "Alpha"
        inputText2: "Beta"
        primaryAction: ({ method: function(event) { root.savedText = event.values.inputText1 } })
    }
    LV.ListItem { objectName: "task"; y: 370; type: LV.ListItem.Task }
    LV.ListItem { objectName: "settings"; y: 530; type: LV.ListItem.DetailSettings }
}
)"));
    QVERIFY(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(root);
    root->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *actions = object->findChild<QQuickItem *>("actions");
    auto *quantity = object->findChild<QQuickItem *>("quantity");
    auto *form = object->findChild<QQuickItem *>("form");
    auto *task = object->findChild<QQuickItem *>("task");
    auto *settings = object->findChild<QQuickItem *>("settings");
    QVERIFY(actions && quantity && form && task && settings);
    QSignalSpy actionsSpy(actions, SIGNAL(actionTriggered(QString,QVariant)));
    QSignalSpy editsSpy(quantity, SIGNAL(edited(QString,QVariant)));
    QVERIFY(actionsSpy.isValid() && editsSpy.isValid());

    QTRY_VERIFY(visibleItem(actions, "listItem_primaryAction"));
    click(window, visibleItem(actions, "listItem_primaryAction"));
    QTRY_COMPARE(object->property("methodCalls").toInt(), 1);
    QCOMPARE(object->property("rowClicks").toInt(), 0);
    QCOMPARE(actionsSpy.count(), 1);
    QCOMPARE(actionsSpy.at(0).at(0).toString(), QString("primary"));
    click(window, visibleItem(actions, "listItem_secondaryAction"));
    QTRY_COMPARE(actionsSpy.count(), 2);
    QCOMPARE(actionsSpy.at(1).at(0).toString(), QString("secondary"));

    click(window, visibleItem(actions, "listItem_moreMenu"));
    auto *actionMenu = actions->findChild<QObject *>("listItem_actionMenu");
    QVERIFY(actionMenu);
    QTRY_VERIFY(actionMenu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(actionMenu, "triggerEntry", Q_ARG(QVariant, 0)));
    QTRY_COMPARE(object->property("menuCalls").toInt(), 1);
    QTRY_VERIFY(!actionMenu->property("visible").toBool());

    auto *stepper = visibleItem(quantity, "listItem_stepper");
    QVERIFY(stepper);
    click(window, stepper, QPointF(9, 3));
    QTRY_COMPARE(quantity->property("quantity").toDouble(), 3.0);
    click(window, stepper, QPointF(9, 3));
    QCOMPARE(quantity->property("quantity").toDouble(), 3.0);
    click(window, stepper, QPointF(9, 15));
    QTRY_COMPARE(quantity->property("quantity").toDouble(), 2.0);
    QTest::keyClick(&window, Qt::Key_Down);
    QTRY_COMPARE(quantity->property("quantity").toDouble(), 1.0);
    QTest::keyClick(&window, Qt::Key_Down);
    QCOMPARE(quantity->property("quantity").toDouble(), 1.0);
    QVERIFY(editsSpy.count() >= 3);

    click(window, visibleItem(quantity, "listItem_toggle"));
    QTRY_VERIFY(!quantity->property("checked").toBool());
    click(window, visibleItem(task, "listItem_selection"));
    QTRY_VERIFY(task->property("checked").toBool());
    click(window, visibleItem(settings, "listItem_segment_1"));
    QTRY_COMPARE(settings->property("segmentIndex").toInt(), 1);

    click(window, visibleItem(quantity, "listItem_selector"));
    auto *selectorMenu = quantity->findChild<QObject *>("listItem_selectorMenu");
    QVERIFY(selectorMenu);
    QTRY_VERIFY(selectorMenu->property("opened").toBool());
    QVERIFY(QMetaObject::invokeMethod(selectorMenu, "triggerEntry", Q_ARG(QVariant, 1)));
    QTRY_COMPARE(quantity->property("selectorIndex").toInt(), 1);
    QTRY_VERIFY(!selectorMenu->property("visible").toBool());
    QCOMPARE(visibleItem(quantity, "listItem_selector")->property("text").toString(), QString("JPEG"));

    auto *input = visibleItem(form, "listItem_input1");
    QVERIFY(input);
    QVERIFY(QMetaObject::invokeMethod(input, "forceInputFocus"));
    QVERIFY(QMetaObject::invokeMethod(input, "selectAll"));
    QTest::keyClick(&window, Qt::Key_Z);
    QTRY_COMPARE(form->property("inputText1").toString(), QString("z"));
    QCOMPARE(form->property("inputText2").toString(), QString("Beta"));
    click(window, visibleItem(form, "listItem_primaryAction"));
    QTRY_COMPARE(object->property("savedText").toString(), QString("z"));

    QVERIFY(QMetaObject::invokeMethod(object.data(), "setStepperMode", Q_ARG(QVariant, "up")));
    click(window, stepper, QPointF(9, 15));
    QTRY_COMPARE(quantity->property("quantity").toDouble(), 2.0);
    QTest::keyClick(&window, Qt::Key_Down);
    QCOMPARE(quantity->property("quantity").toDouble(), 2.0);
    QVERIFY(QMetaObject::invokeMethod(object.data(), "setStepperMode", Q_ARG(QVariant, "disabled")));
    QTest::keyClick(&window, Qt::Key_Up);
    QCOMPARE(quantity->property("quantity").toDouble(), 2.0);
    click(window, stepper, QPointF(9, 3));
    QCOMPARE(quantity->property("quantity").toDouble(), 2.0);

    actions->setEnabled(false);
    click(window, visibleItem(actions, "listItem_primaryAction"));
    QCOMPARE(object->property("methodCalls").toInt(), 1);
    quantity->setEnabled(false);
    click(window, stepper, QPointF(9, 3));
    QCOMPARE(quantity->property("quantity").toDouble(), 2.0);
}

void ListCompositeTests::model_edits_keep_the_live_delegate()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(450, 250);
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.List {
    id: list
    width: 400
    height: 230
    footerVisible: false
    property int savedEvents: 0
    items: [{ type: "InlineEdit", label: "Name", inputText1: "Alpha" },
            { type: "Form", label: "Second" }]
    onItemEdited: function(index, item, field, value) {
        const next = items.slice()
        next[index] = Object.assign({}, item)
        next[index][field] = value
        items = next
    }
    onItemActionTriggered: function(index, item, action, payload) {
        if (index === 0 && action === "primary" && item.inputText1 === "x") savedEvents += 1
    }
}
)"));
    QVERIFY(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    root->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *delegateRoot = visibleItem(root, "list_delegateRoot_0");
    QVERIFY(delegateRoot);
    QPointer<QQuickItem> delegateItem = visibleItem(delegateRoot, "list_defaultDelegate_0");
    QVERIFY(delegateItem);
    auto *input = visibleItem(delegateItem, "listItem_input1");
    QVERIFY(input);
    QVERIFY(QMetaObject::invokeMethod(input, "forceInputFocus"));
    QVERIFY(QMetaObject::invokeMethod(input, "selectAll"));
    QTest::keyClick(&window, Qt::Key_X);
    QTRY_COMPARE(delegateItem->property("inputText1").toString(), QString("x"));
    QCOMPARE(visibleItem(delegateRoot, "list_defaultDelegate_0"), delegateItem.data());
    QVERIFY(input->property("focused").toBool());
    click(window, visibleItem(delegateItem, "listItem_primaryAction"));
    QTRY_COMPARE(object->property("savedEvents").toInt(), 1);
}

void ListCompositeTests::long_content_and_custom_composition_fit()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
Item {
    width: 400
    height: 400
    Component {
        id: accessories
        Row {
            property var listItem
            objectName: "customAccessories"
            spacing: 8
            LV.PushButton { width: 56; text: "A" }
            LV.PushButton { width: 56; text: listItem ? listItem.label : "B" }
        }
    }
    LV.ListItem { objectName: "custom"; type: LV.ListItem.Action; trailingComponent: accessories; label: "Owner" }
    LV.ListItem {
        objectName: "long"
        y: 60
        type: LV.ListItem.ActionGroup
        label: "Long label ".repeat(20)
        description: "Long description ".repeat(20)
        primaryAction: ({text: "Long action ".repeat(20)})
        secondaryAction: ({text: "Long action ".repeat(20)})
        moreMenu: ({text: "Long menu ".repeat(20)})
    }
}
)"));
    QVERIFY(object);
    auto *custom = object->findChild<QQuickItem *>("custom");
    QTRY_VERIFY(visibleItem(custom, "customAccessories"));
    auto *accessories = visibleItem(custom, "customAccessories");
    QCOMPARE(accessories->property("listItem").value<QObject *>(), custom);
    QCOMPARE(qRound(accessories->width()), 120);
    QVERIFY(!visibleItem(custom, "listItem_primaryAction"));
    auto *longRow = object->findChild<QQuickItem *>("long");
    QTRY_COMPARE(qRound(longRow->implicitWidth()), 400);
    QTRY_COMPARE(qRound(longRow->implicitHeight()), 44);
    for (const QString &name : {QString("listItem_primaryAction"), QString("listItem_secondaryAction"), QString("listItem_moreMenu")}) {
        auto *button = visibleItem(longRow, name);
        QVERIFY(button);
        QCOMPARE(qRound(button->width()), 72);
        const QPointF corner = button->mapToItem(longRow, QPointF(button->width(), button->height()));
        QVERIFY(corner.x() <= longRow->width() && corner.y() <= longRow->height());
    }
    longRow->setProperty("showPrimaryAction", false);
    QTRY_VERIFY(!visibleItem(longRow, "listItem_primaryAction"));
    QTRY_COMPARE(qRound(longRow->implicitHeight()), 44);
}

void ListCompositeTests::figma_variants()
{
    QFETCH(int, type);
    QFETCH(int, width);
    QFETCH(int, height);
    QFETCH(bool, mobile);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(width, height);
    const QByteArray qml = "import QtQuick\nimport LVRS as LV\nLV.ListItem { type: "
        + QByteArray::number(type) + "; Component.onCompleted: LV.Theme.targetOverride = \""
        + (mobile ? "ios" : "macos") + "\" }";
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, qml));
    QVERIFY(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(root);
    root->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_COMPARE(object->property("implicitWidth").toInt(), width);
    QTRY_COMPARE(object->property("implicitHeight").toInt(), height);
    QCOMPARE(object->property("size").toInt(), type);
    if (type >= 2) {
        auto *label = visibleItem(object.data(), type == 15 ? "listItem_taskLabel" : "listItem_label");
        QVERIFY(label);
        QCOMPARE(label->property("font").value<QFont>().pixelSize(), 13);
    }
}

void ListCompositeTests::mixed_list_uses_each_rows_height()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.List {
    width: 400
    listWidth: 400
    minimumListHeight: 0
    footerVisible: false
    expandToContent: true
    items: [
        { type: "Mini", label: "First" },
        { type: "Form", label: "Editable", inputText1: "File name" },
        { type: "Navigation", label: "Next" },
        { type: "Media", label: "Audio" }
    ]
}
)"));
    QVERIFY(object);
    QTRY_COMPARE(object->property("contentHeight").toInt(), 327);
    QTRY_COMPARE(object->property("implicitHeight").toInt(), 327);
    const int heights[] = {22, 148, 44, 113};
    int top = 0;
    for (int i = 0; i < 4; ++i) {
        auto *row = visibleItem(object.data(), QStringLiteral("list_delegateRoot_%1").arg(i));
        QVERIFY(row);
        QTRY_COMPARE(qRound(row->y()), top);
        QTRY_COMPARE(qRound(row->height()), heights[i]);
        top += heights[i];
    }
}

void ListCompositeTests::render_catalog_has_all_seventeen_rows()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(1312, 1476);
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
Rectangle {
    width: 1312
    height: 1476
    color: LV.Theme.panelBackground03
    LV.Label { x: 32; y: 24; text: "LVRS · ListItem"; style: header }
    LV.Label { x: 32; y: 58; text: "17 component presets · live Qt Quick rendering"; style: caption }
    Grid {
        x: 32
        y: 100
        columns: 3
        spacing: 24
        Repeater {
            model: 17
            delegate: Item {
                required property int index
                width: 400
                height: 204
                LV.Label {
                    text: row.variantName + " · " + row.implicitWidth + " × " + row.implicitHeight
                    style: caption
                }
                LV.ListItem {
                    id: row
                    objectName: "capture_row_" + index
                    y: 28
                    type: index
                }
            }
        }
    }
}
)"));
    QVERIFY(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(root);
    root->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTest::qWait(100);
    const auto result = root->grabToImage(window.size());
    QVERIFY(result);
    QSignalSpy ready(result.data(), &QQuickItemGrabResult::ready);
    QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty(), 5000);
    const QImage rendered = result->image();
    QCOMPARE(rendered.size(), window.size());
    for (int index = 0; index < 17; ++index) {
        auto *row = visibleItem(root, QStringLiteral("capture_row_%1").arg(index));
        QVERIFY(row);
        const QRect bounds(row->mapToItem(root, QPointF()).toPoint(), QSize(qRound(row->width()), qRound(row->height())));
        QVERIFY(QRect(QPoint(), rendered.size()).contains(bounds));
        const QImage pixels = rendered.copy(bounds);
        const QColor background = pixels.pixelColor(0, 0);
        int painted = 0;
        for (int y = 0; y < pixels.height(); ++y) {
            for (int x = 0; x < pixels.width(); ++x) {
                if (pixels.pixelColor(x, y) != background)
                    ++painted;
            }
        }
        QVERIFY2(painted > 100, qPrintable(QStringLiteral("Variant %1 must render visible content").arg(index)));
    }
    const QString captureDir = qEnvironmentVariable("LVRS_LIST_CAPTURE_DIR");
    if (!captureDir.isEmpty()) {
        QVERIFY(QDir().mkpath(captureDir));
        QVERIFY(rendered.save(QDir(captureDir).filePath("listitem-presets.png")));
    }
}

void ListCompositeTests::variant_changes_reflow_and_keep_footer_fixed()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(450, 250);
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.List {
    height: 140
    itemSpacing: 8
    items: [{ type: "Mini", label: "First" }, { type: "Mini", label: "Second" }]
    function expandFirst() { items = [{ type: "Resource", label: "First" }, items[1]] }
}
)"));
    QVERIFY(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    root->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_COMPARE(qRound(root->width()), 170);
    auto *first = visibleItem(root, "list_delegateRoot_0");
    auto *second = visibleItem(root, "list_delegateRoot_1");
    auto *footer = visibleItem(root, "list_footer");
    auto *viewport = visibleItem(root, "list_itemsViewport");
    QVERIFY(first && second && footer && viewport);
    const QPointF footerPosition = footer->mapToItem(root, QPointF());
    QPointer<QQuickItem> original = visibleItem(first, "list_defaultDelegate_0");
    QVERIFY(QMetaObject::invokeMethod(root, "expandFirst"));
    QTRY_COMPARE(qRound(root->width()), 400);
    QTRY_COMPARE(qRound(first->height()), 129);
    QTRY_COMPARE(qRound(second->y()), 137);
    QTRY_COMPARE(viewport->property("contentHeight").toInt(), 159);
    QCOMPARE(visibleItem(first, "list_defaultDelegate_0"), original.data());
    QVERIFY(viewport->property("interactive").toBool());
    const double offset = viewport->property("contentHeight").toDouble() - viewport->height();
    QVERIFY(offset > 0);
    QVERIFY(viewport->setProperty("contentY", offset));
    QTRY_COMPARE(viewport->property("contentY").toDouble(), offset);
    QCOMPARE(footer->mapToItem(root, QPointF()), footerPosition);
    QVERIFY(viewport->clip());
    QVERIFY(root->setProperty("scrollable", false));
    QTRY_VERIFY(!viewport->property("interactive").toBool());
}

void ListCompositeTests::visual_catalog_list_preview_loads()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    const QString path = QFINDTESTDATA("../example/VisualCatalog/qml/Main.qml");
    QVERIFY(!path.isEmpty());
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine, path));
    QVERIFY(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QVERIFY(object->setProperty("activeEntryKey", "list"));
    QTRY_VERIFY(visibleItem(window->contentItem(), "list_defaultDelegate_16"));
    for (int index = 0; index < 17; ++index) {
        auto *row = visibleItem(window->contentItem(), QStringLiteral("list_defaultDelegate_%1").arg(index));
        QVERIFY(row);
        QCOMPARE(row->property("type").toInt(), index);
        QVERIFY(row->width() >= 400);
    }
}

QTEST_MAIN(ListCompositeTests)
#include "tst_list_composites.moc"
