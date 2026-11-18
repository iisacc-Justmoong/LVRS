#include "QtEventWait.h"
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QTimer>
#include <type_traits>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlEngine>
#include <QQuickItemGrabResult>

#include "test_utils.h"

class ListCompositeSignalRecorder : public QObject
{
    Q_OBJECT
public:
    QList<QPair<QString, QVariant>> actionEvents;
    QList<QPair<QString, QVariant>> editEvents;
public slots:
    void recordAction(const QString &name, const QVariant &value) { actionEvents.append({name, value}); }
    void recordEdit(const QString &name, const QVariant &value) { editEvents.append({name, value}); }
};

namespace {
struct ListVariant {
    const char *name;
    int type;
    int width;
    int height;
    bool mobile;
};

QList<ListVariant> listVariants()
{
    const char *names[] = {"Mini", "Detail", "Navigation", "Toggle", "Checkable",
                           "Action", "ActionGroup", "Stepper", "Select", "InlineEdit",
                           "DetailActions", "DetailQuantity", "DetailSettings", "Resource",
                           "Media", "Task", "Form"};
    const int widths[] = {170, 194, 280, 280, 280, 400, 400, 400, 400, 400,
                          400, 400, 400, 400, 400, 400, 400};
    const int heights[] = {22, 106, 44, 44, 44, 44, 44, 44, 44, 44,
                           120, 118, 159, 129, 113, 135, 154};
    QList<ListVariant> result;
    for (int type = 0; type < 17; ++type)
        for (bool mobile : {false, true})
            result.append({names[type], type, widths[type], heights[type], mobile});
    return result;
}

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

template <typename Actual, typename Expected>
bool listEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual) + 1.0, qreal(expected) + 1.0);
    else
        return actual == expected;
}

void click(QQuickWindow &window, QQuickItem *item, QPointF local = QPointF(-1, -1))
{
    CTEST_CHECK_MESSAGE(item, "Expected a visible interactive control");
    if (local.x() < 0)
        local = QPointF(item->width() / 2, item->height() / 2);
    const QPointF point(item->mapToScene(local));
    const QPointF global(window.mapToGlobal(point.toPoint()));
    static ulong timestamp = 10000;
    QMouseEvent press(QEvent::MouseButtonPress, point, point, global, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    press.setTimestamp(timestamp += 16);
    QCoreApplication::sendEvent(&window, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, point, point, global, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    release.setTimestamp(timestamp += 16);
    QCoreApplication::sendEvent(&window, &release);
    QCoreApplication::processEvents();
}

void keyClick(QQuickWindow *window, Qt::Key key)
{
    const QString text = key >= Qt::Key_A && key <= Qt::Key_Z ? QString(QChar(key).toLower()) : QString();
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier, text);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier, text);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}

void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}
}
#define LIST_COMPARE(actual, expected) CTEST_CHECK(listEqual((actual), (expected)))
#define LIST_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(listEqual((actual), (expected)))

class ListCompositeTests
{
public:
    void figma_variants(int type, int width, int height, bool mobile);
    void mixed_list_uses_each_rows_height();
    void controls_deliver_real_edits_and_actions();
    void model_edits_keep_the_live_delegate();
    void long_content_and_custom_composition_fit();
    void variant_changes_reflow_and_keep_footer_fixed();
    void render_catalog_has_all_seventeen_rows();
    void visual_catalog_list_preview_loads();
};

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
    CTEST_CHECK(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    CTEST_CHECK(root);
    root->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    auto *actions = object->findChild<QQuickItem *>("actions");
    auto *quantity = object->findChild<QQuickItem *>("quantity");
    auto *form = object->findChild<QQuickItem *>("form");
    auto *task = object->findChild<QQuickItem *>("task");
    auto *settings = object->findChild<QQuickItem *>("settings");
    CTEST_CHECK(actions && quantity && form && task && settings);
    ListCompositeSignalRecorder observations;
    const auto actionsConnection = QObject::connect(actions, SIGNAL(actionTriggered(QString,QVariant)), &observations, SLOT(recordAction(QString,QVariant)));
    const auto editsConnection = QObject::connect(quantity, SIGNAL(edited(QString,QVariant)), &observations, SLOT(recordEdit(QString,QVariant)));
    CTEST_CHECK(actionsConnection && editsConnection);

    CTEST_WAIT_DEFAULT(visibleItem(actions, "listItem_primaryAction"));
    click(window, visibleItem(actions, "listItem_primaryAction"));
    LIST_WAIT_COMPARE(object->property("methodCalls").toInt(), 1);
    LIST_COMPARE(object->property("rowClicks").toInt(), 0);
    LIST_COMPARE(observations.actionEvents.size(), 1);
    LIST_COMPARE(observations.actionEvents.at(0).first, QString("primary"));
    click(window, visibleItem(actions, "listItem_secondaryAction"));
    LIST_WAIT_COMPARE(observations.actionEvents.size(), 2);
    LIST_COMPARE(observations.actionEvents.at(1).first, QString("secondary"));

    click(window, visibleItem(actions, "listItem_moreMenu"));
    auto *actionMenu = actions->findChild<QObject *>("listItem_actionMenu");
    CTEST_CHECK(actionMenu);
    CTEST_WAIT_DEFAULT(actionMenu->property("opened").toBool());
    CTEST_CHECK(QMetaObject::invokeMethod(actionMenu, "triggerEntry", Q_ARG(QVariant, 0)));
    LIST_WAIT_COMPARE(object->property("menuCalls").toInt(), 1);
    CTEST_WAIT_DEFAULT(!actionMenu->property("visible").toBool());

    auto *stepper = visibleItem(quantity, "listItem_stepper");
    CTEST_CHECK(stepper);
    click(window, stepper, QPointF(9, 3));
    LIST_WAIT_COMPARE(quantity->property("quantity").toDouble(), 3.0);
    click(window, stepper, QPointF(9, 3));
    LIST_COMPARE(quantity->property("quantity").toDouble(), 3.0);
    click(window, stepper, QPointF(9, 15));
    LIST_WAIT_COMPARE(quantity->property("quantity").toDouble(), 2.0);
    keyClick(&window, Qt::Key_Down);
    LIST_WAIT_COMPARE(quantity->property("quantity").toDouble(), 1.0);
    keyClick(&window, Qt::Key_Down);
    LIST_COMPARE(quantity->property("quantity").toDouble(), 1.0);
    CTEST_CHECK(observations.editEvents.size() >= 3);

    click(window, visibleItem(quantity, "listItem_toggle"));
    CTEST_WAIT_DEFAULT(!quantity->property("checked").toBool());
    click(window, visibleItem(task, "listItem_selection"));
    CTEST_WAIT_DEFAULT(task->property("checked").toBool());
    auto *firstSegment = visibleItem(settings, "listItem_segment_0");
    auto *secondSegment = visibleItem(settings, "listItem_segment_1");
    CTEST_CHECK(firstSegment && secondSegment);
    CTEST_CHECK(firstSegment->property("checked").toBool());
    const auto originalTone = secondSegment->property("tone").toInt();
    click(window, secondSegment);
    LIST_WAIT_COMPARE(settings->property("segmentIndex").toInt(), 1);
    CTEST_WAIT_DEFAULT(secondSegment->property("checked").toBool());
    CTEST_WAIT_DEFAULT(!firstSegment->property("checked").toBool());
    LIST_COMPARE(secondSegment->property("tone").toInt(), originalTone);
    LIST_COMPARE(firstSegment->property("tone").toInt(), originalTone);
    const auto surface = secondSegment->property("background").value<QObject *>();
    CTEST_CHECK(surface);
    LIST_WAIT_COMPARE(surface->property("color").value<QColor>(), secondSegment->property("selectedBackgroundColor").value<QColor>());

    click(window, visibleItem(quantity, "listItem_selector"));
    auto *selectorMenu = quantity->findChild<QObject *>("listItem_selectorMenu");
    CTEST_CHECK(selectorMenu);
    CTEST_WAIT_DEFAULT(selectorMenu->property("opened").toBool());
    CTEST_CHECK(QMetaObject::invokeMethod(selectorMenu, "triggerEntry", Q_ARG(QVariant, 1)));
    LIST_WAIT_COMPARE(quantity->property("selectorIndex").toInt(), 1);
    CTEST_WAIT_DEFAULT(!selectorMenu->property("visible").toBool());
    LIST_COMPARE(visibleItem(quantity, "listItem_selector")->property("text").toString(), QString("JPEG"));

    auto *input = visibleItem(form, "listItem_input1");
    CTEST_CHECK(input);
    CTEST_CHECK(QMetaObject::invokeMethod(input, "forceInputFocus"));
    CTEST_CHECK(QMetaObject::invokeMethod(input, "selectAll"));
    keyClick(&window, Qt::Key_Z);
    LIST_WAIT_COMPARE(form->property("inputText1").toString(), QString("z"));
    LIST_COMPARE(form->property("inputText2").toString(), QString("Beta"));
    click(window, visibleItem(form, "listItem_primaryAction"));
    LIST_WAIT_COMPARE(object->property("savedText").toString(), QString("z"));

    CTEST_CHECK(QMetaObject::invokeMethod(object.data(), "setStepperMode", Q_ARG(QVariant, "up")));
    click(window, stepper, QPointF(9, 15));
    LIST_WAIT_COMPARE(quantity->property("quantity").toDouble(), 2.0);
    keyClick(&window, Qt::Key_Down);
    LIST_COMPARE(quantity->property("quantity").toDouble(), 2.0);
    CTEST_CHECK(QMetaObject::invokeMethod(object.data(), "setStepperMode", Q_ARG(QVariant, "disabled")));
    keyClick(&window, Qt::Key_Up);
    LIST_COMPARE(quantity->property("quantity").toDouble(), 2.0);
    click(window, stepper, QPointF(9, 3));
    LIST_COMPARE(quantity->property("quantity").toDouble(), 2.0);

    actions->setEnabled(false);
    click(window, visibleItem(actions, "listItem_primaryAction"));
    LIST_COMPARE(object->property("methodCalls").toInt(), 1);
    quantity->setEnabled(false);
    click(window, stepper, QPointF(9, 3));
    LIST_COMPARE(quantity->property("quantity").toDouble(), 2.0);
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
    CTEST_CHECK(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    root->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    auto *delegateRoot = visibleItem(root, "list_delegateRoot_0");
    CTEST_CHECK(delegateRoot);
    QPointer<QQuickItem> delegateItem = visibleItem(delegateRoot, "list_defaultDelegate_0");
    CTEST_CHECK(delegateItem);
    auto *input = visibleItem(delegateItem, "listItem_input1");
    CTEST_CHECK(input);
    CTEST_CHECK(QMetaObject::invokeMethod(input, "forceInputFocus"));
    CTEST_CHECK(QMetaObject::invokeMethod(input, "selectAll"));
    keyClick(&window, Qt::Key_X);
    LIST_WAIT_COMPARE(delegateItem->property("inputText1").toString(), QString("x"));
    LIST_COMPARE(visibleItem(delegateRoot, "list_defaultDelegate_0"), delegateItem.data());
    CTEST_CHECK(input->property("focused").toBool());
    click(window, visibleItem(delegateItem, "listItem_primaryAction"));
    LIST_WAIT_COMPARE(object->property("savedEvents").toInt(), 1);
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
    CTEST_CHECK(object);
    auto *custom = object->findChild<QQuickItem *>("custom");
    CTEST_WAIT_DEFAULT(visibleItem(custom, "customAccessories"));
    auto *accessories = visibleItem(custom, "customAccessories");
    LIST_COMPARE(accessories->property("listItem").value<QObject *>(), custom);
    LIST_COMPARE(qRound(accessories->width()), 120);
    CTEST_CHECK(!visibleItem(custom, "listItem_primaryAction"));
    auto *longRow = object->findChild<QQuickItem *>("long");
    LIST_WAIT_COMPARE(qRound(longRow->implicitWidth()), 400);
    LIST_WAIT_COMPARE(qRound(longRow->implicitHeight()), 44);
    for (const QString &name : {QString("listItem_primaryAction"), QString("listItem_secondaryAction"), QString("listItem_moreMenu")}) {
        auto *button = visibleItem(longRow, name);
        CTEST_CHECK(button);
        LIST_COMPARE(qRound(button->width()), 72);
        const QPointF corner = button->mapToItem(longRow, QPointF(button->width(), button->height()));
        CTEST_CHECK(corner.x() <= longRow->width() && corner.y() <= longRow->height());
    }
    longRow->setProperty("showPrimaryAction", false);
    CTEST_WAIT_DEFAULT(!visibleItem(longRow, "listItem_primaryAction"));
    LIST_WAIT_COMPARE(qRound(longRow->implicitHeight()), 44);
}

void ListCompositeTests::figma_variants(int type, int width, int height, bool mobile)
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(width, height);
    const QByteArray qml = "import QtQuick\nimport LVRS as LV\nLV.ListItem { type: "
        + QByteArray::number(type) + "; Component.onCompleted: LV.Theme.targetOverride = \""
        + (mobile ? "ios" : "macos") + "\" }";
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, qml));
    CTEST_CHECK(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    CTEST_CHECK(root);
    root->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    LIST_WAIT_COMPARE(object->property("implicitWidth").toInt(), width);
    LIST_WAIT_COMPARE(object->property("implicitHeight").toInt(), height);
    LIST_COMPARE(object->property("size").toInt(), type);
    if (type >= 2) {
        auto *label = visibleItem(object.data(), type == 15 ? "listItem_taskLabel" : "listItem_label");
        CTEST_CHECK(label);
        LIST_COMPARE(label->property("font").value<QFont>().pixelSize(), 13);
    }
    if (type == 9 || type == 16) {
        const int inputCount = type == 16 ? 2 : 1;
        for (int index = 1; index <= inputCount; ++index) {
            auto *input = visibleItem(root, QStringLiteral("listItem_input%1").arg(index));
            CTEST_CHECK(input);
            LIST_COMPARE(input->height(), 22.0);
            auto *nativeInput = input->property("inputItem").value<QQuickItem *>();
            CTEST_CHECK(nativeInput);
            LIST_COMPARE(nativeInput->y(), 4.5);
            const QPointF bottom = input->mapToItem(root, QPointF(input->width(), input->height()));
            CTEST_CHECK(bottom.x() <= root->width() && bottom.y() <= root->height());
        }
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
    CTEST_CHECK(object);
    LIST_WAIT_COMPARE(object->property("contentHeight").toInt(), 333);
    LIST_WAIT_COMPARE(object->property("implicitHeight").toInt(), 333);
    const int heights[] = {22, 154, 44, 113};
    int top = 0;
    for (int i = 0; i < 4; ++i) {
        auto *row = visibleItem(object.data(), QStringLiteral("list_delegateRoot_%1").arg(i));
        CTEST_CHECK(row);
        LIST_WAIT_COMPARE(qRound(row->y()), top);
        LIST_WAIT_COMPARE(qRound(row->height()), heights[i]);
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
    CTEST_CHECK(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    CTEST_CHECK(root);
    root->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    processFor(100);
    // Windows may clamp a top-level window to the available monitor height.
    // Capture in the root item's coordinates, which the row bounds also use.
    const QSize captureSize(qRound(root->width()), qRound(root->height()));
    const auto result = root->grabToImage(captureSize);
    CTEST_CHECK(result);
    int readyCount = 0;
    const auto captureConnection = QObject::connect(result.data(), &QQuickItemGrabResult::ready,
                                                     [&] { ++readyCount; });
    CTEST_CHECK(captureConnection);
    const auto disconnectCapture = qScopeGuard([&] { QObject::disconnect(captureConnection); });
    CTEST_WAIT_VERIFY(readyCount > 0, 5000);
    const QImage rendered = result->image().scaled(captureSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    LIST_COMPARE(rendered.size(), captureSize);
    const QString diagnosticCapture = qEnvironmentVariable("LVRS_LIST_DIAGNOSTIC_CAPTURE");
    if (!diagnosticCapture.isEmpty()) CTEST_CHECK(rendered.save(diagnosticCapture));
    for (int index = 0; index < 17; ++index) {
        auto *row = visibleItem(root, QStringLiteral("capture_row_%1").arg(index));
        CTEST_CHECK(row);
        const QRect bounds(row->mapToItem(root, QPointF()).toPoint(), QSize(qRound(row->width()), qRound(row->height())));
        CTEST_CHECK(QRect(QPoint(), rendered.size()).contains(bounds));
        const QImage pixels = rendered.copy(bounds);
        const QColor background = pixels.pixelColor(0, 0);
        int painted = 0;
        for (int y = 0; y < pixels.height(); ++y) {
            for (int x = 0; x < pixels.width(); ++x) {
                if (pixels.pixelColor(x, y) != background)
                    ++painted;
            }
        }
        CTEST_CHECK_MESSAGE(painted > 100, qPrintable(QStringLiteral("Variant %1 must render visible content at %2,%3 (%4x%5), painted %6").arg(index).arg(bounds.x()).arg(bounds.y()).arg(bounds.width()).arg(bounds.height()).arg(painted)));
    }
    const QString captureDir = qEnvironmentVariable("LVRS_LIST_CAPTURE_DIR");
    if (!captureDir.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(captureDir));
        CTEST_CHECK(rendered.save(QDir(captureDir).filePath("listitem-presets.png")));
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
    CTEST_CHECK(object);
    auto *root = qobject_cast<QQuickItem *>(object.data());
    root->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    LIST_WAIT_COMPARE(qRound(root->width()), 170);
    auto *first = visibleItem(root, "list_delegateRoot_0");
    auto *second = visibleItem(root, "list_delegateRoot_1");
    auto *footer = visibleItem(root, "list_footer");
    auto *viewport = visibleItem(root, "list_itemsViewport");
    CTEST_CHECK(first && second && footer && viewport);
    const QPointF footerPosition = footer->mapToItem(root, QPointF());
    QPointer<QQuickItem> original = visibleItem(first, "list_defaultDelegate_0");
    CTEST_CHECK(QMetaObject::invokeMethod(root, "expandFirst"));
    LIST_WAIT_COMPARE(qRound(root->width()), 400);
    LIST_WAIT_COMPARE(qRound(first->height()), 129);
    LIST_WAIT_COMPARE(qRound(second->y()), 137);
    LIST_WAIT_COMPARE(viewport->property("contentHeight").toInt(), 159);
    LIST_COMPARE(visibleItem(first, "list_defaultDelegate_0"), original.data());
    CTEST_CHECK(viewport->property("interactive").toBool());
    const double offset = viewport->property("contentHeight").toDouble() - viewport->height();
    CTEST_CHECK(offset > 0);
    CTEST_CHECK(viewport->setProperty("contentY", offset));
    LIST_WAIT_COMPARE(viewport->property("contentY").toDouble(), offset);
    LIST_COMPARE(footer->mapToItem(root, QPointF()), footerPosition);
    CTEST_CHECK(viewport->clip());
    CTEST_CHECK(root->setProperty("scrollable", false));
    CTEST_WAIT_DEFAULT(!viewport->property("interactive").toBool());
}

void ListCompositeTests::visual_catalog_list_preview_loads()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    const QString candidate = QDir(QStringLiteral(LVRS_TEST_SOURCE_DIR)).filePath("../example/VisualCatalog/qml/Main.qml");
    const QString path = QFileInfo::exists(candidate) ? candidate : QString();
    CTEST_CHECK(!path.isEmpty());
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine, path));
    CTEST_CHECK(object);
    auto *window = qobject_cast<QQuickWindow *>(object.data());
    CTEST_CHECK(window);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    CTEST_CHECK(object->setProperty("activeEntryKey", "list"));
    CTEST_WAIT_DEFAULT(visibleItem(window->contentItem(), "list_defaultDelegate_16"));
    for (int index = 0; index < 17; ++index) {
        auto *row = visibleItem(window->contentItem(), QStringLiteral("list_defaultDelegate_%1").arg(index));
        CTEST_CHECK(row);
        LIST_COMPARE(row->property("type").toInt(), index);
        CTEST_CHECK(row->width() >= 400);
    }
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    ListCompositeTests tests;
    for (const ListVariant &variant : listVariants()) {
        const auto name = QStringLiteral("figma_variants:") + QString::fromLatin1(variant.name)
            + (variant.mobile ? QStringLiteral("-mobile") : QString());
        runner.run(name.toStdString(), [&] { tests.figma_variants(variant.type, variant.width, variant.height, variant.mobile); });
    }
    runner.run("mixed_list_uses_each_rows_height", [&] { tests.mixed_list_uses_each_rows_height(); });
    runner.run("controls_deliver_real_edits_and_actions", [&] { tests.controls_deliver_real_edits_and_actions(); });
    runner.run("model_edits_keep_the_live_delegate", [&] { tests.model_edits_keep_the_live_delegate(); });
    runner.run("long_content_and_custom_composition_fit", [&] { tests.long_content_and_custom_composition_fit(); });
    runner.run("variant_changes_reflow_and_keep_footer_fixed", [&] { tests.variant_changes_reflow_and_keep_footer_fixed(); });
    runner.run("render_catalog_has_all_seventeen_rows", [&] { tests.render_catalog_has_all_seventeen_rows(); });
    runner.run("visual_catalog_list_preview_loads", [&] { tests.visual_catalog_list_preview_loads(); });
    return runner.result();
}
#include "tst_list_composites.moc"
