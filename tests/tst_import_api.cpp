#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QtPlugin>

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class ImportApiTests : public QObject
{
    Q_OBJECT

private slots:
    void versionless_import_application_window_loads();
    void application_window_page_stack_state_loads();
    void versionless_import_window_loads();
    void appshell_compat_loads();
    void application_window_platform_adaptive_layout_loads();
    void icon_name_mapping_loads();
    void hierarchy_tree_model_api_loads();
    void hierarchy_string_array_model_loads();
    void hierarchy_nested_children_indent_contract_loads();
    void hierarchy_optional_footer_contract_loads();
    void hierarchy_toolbar_item_model_contract_loads();
    void hierarchy_toolbar_figma_layout_contract_loads();
    void hierarchy_toolbar_manual_icon_button_contract_loads();
    void hierarchy_row_click_only_activates_not_toggles();
    void hierarchy_chevron_requires_children_loads();
    void hierarchy_item_chevron_direction_contract_loads();
    void hierarchy_item_hover_and_active_state_visual_contract_loads();
    void hierarchy_item_inputable_overlay_contract_loads();
    void hierarchy_item_input_overlay_geometry_and_close_contract_loads();
    void button_padding_matches_figma_spec();
    void button_default_tone_fallback_borderless_loads();
    void stepper_figma_contract_loads();
    void combo_box_figma_contract_loads();
    void input_field_figma_contract_loads();
    void toggle_switch_figma_color_contract_loads();
    void checkbox_figma_contract_loads();
    void radio_button_figma_contract_loads();
    void modal_empty_frame_contract_loads();
    void modal_content_action_contract_loads();
    void menu_item_key_and_chevron_contract_loads();
    void context_menu_item_action_contract_loads();
    void context_menu_auto_placement_contract_loads();
    void table_cell_item_contract_loads();
    void list_item_and_footer_figma_contract_loads();
};

static QObject *createFromQml(QQmlEngine &engine, const QByteArray &qml)
{
    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QObject *obj = component.create();
    if (component.isError()) {
        const auto errors = component.errors();
        for (const auto &err : errors)
            qWarning() << err;
    }
    return obj;
}

void ImportApiTests::versionless_import_application_window_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    width: 1200
    height: 800
    visible: false
    title: "API"
    subtitle: "Merged"
    scaffoldLayoutMode: "mobile"
    navItems: ["Overview", "Runs"]
    navigationEnabled: true

    property bool importReady: LV.Theme.dark
    property bool shellApiReady: navItems.length === 2 && navWidth > 0 && navDrawerWidth > 0
    property bool adaptiveApiReady: adaptiveMobileLayout
        && !adaptiveDesktopLayout
        && adaptiveBottomNavigation
        && !adaptiveRailNavigation
        && !adaptiveDrawerNavigation
        && matchesMedia("mobile-layout")
        && matchesMedia("bottom-nav")
        && !matchesMedia("rail-nav")
    property bool qualityReady: LV.RenderQuality.enabled && LV.RenderQuality.supersampleScale >= 3.0
    property bool backendOptimizationDefaultsReady: !autoAttachRuntimeEvents
        && autoAttachRuntimeEvents === globalEventListenersEnabled
        && !autoHookBackendUserEvents
        && !globalEventListenersEnabled
    property bool labelStyleApiReady: contentLabel.style === contentLabel.body
        && contentLabel.font.pixelSize === LV.Theme.textBody
        && contentLabel.font.weight === LV.Theme.textBodyWeight
        && contentLabel.color === LV.Theme.bodyColor
        && contentLabel.renderType === Text.NativeRendering
    property bool figmaTextDesignReady:
        titleLabel.font.pixelSize === LV.Theme.textTitle
        && titleLabel.font.weight === LV.Theme.textTitleWeight
        && titleLabel.color === LV.Theme.titleHeaderColor
        && title2Label.font.pixelSize === LV.Theme.textTitle2
        && title2Label.font.weight === LV.Theme.textTitle2Weight
        && title2Label.color === LV.Theme.titleHeaderColor
        && headerLabel.font.pixelSize === LV.Theme.textHeader
        && headerLabel.font.weight === LV.Theme.textHeaderWeight
        && headerLabel.color === LV.Theme.titleHeaderColor
        && header2Label.font.pixelSize === LV.Theme.textHeader2
        && header2Label.font.weight === LV.Theme.textHeader2Weight
        && header2Label.color === LV.Theme.titleHeaderColor
        && bodyLabel.font.pixelSize === LV.Theme.textBody
        && bodyLabel.font.weight === LV.Theme.textBodyWeight
        && bodyLabel.color === LV.Theme.bodyColor
        && descriptionLabel.font.pixelSize === LV.Theme.textDescription
        && descriptionLabel.font.weight === LV.Theme.textDescriptionWeight
        && descriptionLabel.color === LV.Theme.descriptionColor
        && captionLabel.font.pixelSize === LV.Theme.textCaption
        && captionLabel.font.weight === LV.Theme.textCaptionWeight
        && captionLabel.color === LV.Theme.captionColor
        && disabledLabel.font.pixelSize === LV.Theme.textDisabled
        && disabledLabel.font.weight === LV.Theme.textDisabledWeight
        && disabledLabel.color === LV.Theme.disabledColor

    LV.Label {
        id: contentLabel
        text: "Content Slot"
        style: body
    }
    LV.Label { id: titleLabel; text: "Title"; style: title; visible: false }
    LV.Label { id: title2Label; text: "Title2"; style: title2; visible: false }
    LV.Label { id: headerLabel; text: "Header"; style: header; visible: false }
    LV.Label { id: header2Label; text: "Header2"; style: header2; visible: false }
    LV.Label { id: bodyLabel; text: "Body"; style: body; visible: false }
    LV.Label { id: descriptionLabel; text: "Description"; style: description; visible: false }
    LV.Label { id: captionLabel; text: "Caption"; style: caption; visible: false }
    LV.Label { id: disabledLabel; text: "Disabled"; style: disabled; visible: false }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("importReady").toBool());
    QVERIFY(root->property("shellApiReady").toBool());
    QVERIFY(root->property("adaptiveApiReady").toBool());
    QVERIFY(root->property("qualityReady").toBool());
    QVERIFY(root->property("backendOptimizationDefaultsReady").toBool());
    QVERIFY(root->property("labelStyleApiReady").toBool());
    QVERIFY(root->property("figmaTextDesignReady").toBool());
    QCOMPARE(root->property("subtitle").toString(), QStringLiteral("Merged"));
}

void ImportApiTests::versionless_import_window_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.Window {
    width: 520
    height: 360
    visible: false
    title: "Settings"
    usePlatformSafeMargin: true

    property bool windowApiReady: platform.length > 0
        && (widthClass >= compact && widthClass <= expanded)
        && (heightClass >= compact && heightClass <= expanded)
        && typeof matchesMedia === "function"
    property bool contentApiReady: contentLabel.text === "Window Content"

    LV.Label {
        id: contentLabel
        text: "Window Content"
        style: body
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QCOMPARE(root->property("title").toString(), QStringLiteral("Settings"));
    QVERIFY(root->property("solidChrome").toBool());
    QVERIFY(root->property("windowApiReady").toBool());
    QVERIFY(root->property("contentApiReady").toBool());
}

void ImportApiTests::application_window_page_stack_state_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 900
    height: 620
    visible: false
    title: "StackState"
    useInternalPageStack: true
    pageInitialPath: "/"
    pageRoutes: [
        { path: "/", component: homePage },
        { path: "/reports", component: reportsPage }
    ]

    Component {
        id: homePage
        Item { objectName: "home-page" }
    }
    Component {
        id: reportsPage
        Item { objectName: "reports-page" }
    }

    property bool stackEnabledRule: internalPageStackEnabled && matchesMedia("stack-enabled")
    property bool stackInitialReady: stackEnabledRule
        && activePageRouter !== null
        && activePageRouter.depth >= 1
        && activePageRouter.currentPath === "/"
    property bool stackNavigationWorked: false

    onPageStackNavigated: {
        if (path === "/reports")
            stackNavigationWorked = true
    }

    Component.onCompleted: {
        Qt.callLater(function() {
            if (activePageRouter)
                activePageRouter.go("/reports")
        })
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("stackInitialReady").toBool());
    QTRY_VERIFY(root->property("stackNavigationWorked").toBool());
}

void ImportApiTests::appshell_compat_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.AppShell {
    width: 1000
    height: 700
    visible: false
    title: "Compat"
    subtitle: "Wrapper"
    navItems: ["A"]
    navigationEnabled: true
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QCOMPARE(root->property("title").toString(), QStringLiteral("Compat"));
    QCOMPARE(root->property("subtitle").toString(), QStringLiteral("Wrapper"));
    QVERIFY(root->property("navItems").isValid());
}

void ImportApiTests::application_window_platform_adaptive_layout_loads()
{
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    QQmlEngine mobileEngine;
    mobileEngine.addImportPath(importBase);
    const QByteArray mobileQml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: mobileWindow
    width: 1400
    height: 900
    visible: false
    scaffoldLayoutMode: "auto"
    scaffoldLayoutPlatform: "android"
    navItems: ["Home", "Runs", "Settings"]
    navigationEnabled: true

    property bool contract:
        mobileWindow.adaptiveMobileLayout
        && !mobileWindow.adaptiveDesktopLayout
        && mobileWindow.adaptiveBottomNavigation
        && !mobileWindow.adaptiveRailNavigation
        && !mobileWindow.adaptiveDrawerNavigation
}
)";

    QScopedPointer<QObject> mobileRoot(createFromQml(mobileEngine, mobileQml));
    QVERIFY(mobileRoot);
    QTRY_VERIFY(mobileRoot->property("contract").toBool());

    QQmlEngine desktopEngine;
    desktopEngine.addImportPath(importBase);
    const QByteArray desktopQml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: desktopWindow
    width: 1400
    height: 900
    visible: false
    scaffoldLayoutMode: "auto"
    scaffoldLayoutPlatform: "osx"
    navItems: ["Home", "Runs", "Settings"]
    navigationEnabled: true

    property bool contract:
        desktopWindow.adaptiveDesktopLayout
        && !desktopWindow.adaptiveMobileLayout
        && desktopWindow.adaptiveRailNavigation
        && !desktopWindow.adaptiveBottomNavigation
        && !desktopWindow.adaptiveDrawerNavigation
}
)";

    QScopedPointer<QObject> desktopRoot(createFromQml(desktopEngine, desktopQml));
    QVERIFY(desktopRoot);
    QTRY_VERIFY(desktopRoot->property("contract").toBool());
}

void ImportApiTests::icon_name_mapping_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property string iconRoot: "qrc:/qt/qml/LVRS/resources/iconset/"
    property string expectedByName: iconRoot + "viewMoreSymbolicDefault.svg"
    property string expectedByExt: iconRoot + "viewMoreSymbolicBorderless.svg"
    property string expectedByGroup: iconRoot + "panDownSymbolicDefault.svg"
    property string expectedByUrl: iconRoot + "panDownSymbolicAccent.svg"
    property string expectedMenuByName: iconRoot + "panDownSymbolicBorderless.svg"
    property bool themeAddsSvg: LV.Theme.iconPath("panDownSymbolicDisabled") === iconRoot + "panDownSymbolicDisabled.svg"
    property bool themeKeepsSvg: LV.Theme.iconPath("panDownSymbolicDisabled.svg") === iconRoot + "panDownSymbolicDisabled.svg"

    LV.IconButton {
        id: byName
        iconName: "viewMoreSymbolicDefault"
        visible: false
    }

    LV.IconButton {
        id: byExt
        iconName: "viewMoreSymbolicBorderless.svg"
        visible: false
    }

    LV.IconButton {
        id: byGroupName
        icon.name: "panDownSymbolicDefault"
        visible: false
    }

    LV.IconButton {
        id: byUrl
        iconSource: root.expectedByUrl
        iconName: "viewMoreSymbolicDefault"
        visible: false
    }

    LV.IconMenuButton {
        id: menuByName
        iconName: "panDownSymbolicBorderless"
        visible: false
    }

    property bool byNameOk: byName.resolvedIconSource.toString() === expectedByName
    property bool byExtOk: byExt.resolvedIconSource.toString() === expectedByExt
    property bool byGroupOk: byGroupName.resolvedIconSource.toString() === expectedByGroup
    property bool byUrlOk: byUrl.resolvedIconSource.toString() === expectedByUrl
    property bool menuByNameOk: menuByName.resolvedIconSource.toString() === expectedMenuByName
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("themeAddsSvg").toBool());
    QVERIFY(root->property("themeKeepsSvg").toBool());
    QVERIFY(root->property("byNameOk").toBool());
    QVERIFY(root->property("byExtOk").toBool());
    QVERIFY(root->property("byGroupOk").toBool());
    QVERIFY(root->property("byUrlOk").toBool());
    QVERIFY(root->property("menuByNameOk").toBool());
}

void ImportApiTests::hierarchy_tree_model_api_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    width: 640
    height: 420
    property int activationAttempts: 0

    function tryActivateLeaf() {
        if (hierarchy.activeListItemKey === "leaf-a1")
            return
        if (activationAttempts >= 40)
            return
        activationAttempts += 1
        const activated = hierarchy.activateListItemByKey("leaf-a1")
        if (!activated)
            Qt.callLater(tryActivateLeaf)
    }

    LV.Hierarchy {
        id: hierarchy
        objectName: "hierarchy"
        width: 280
        height: 300
        model: [
            {
                key: "root",
                depth: 0,
                itemId: 10,
                text: "Root",
                icon: "viewMoreSymbolicDefault",
                expanded: true,
                children: [
                    {
                        key: "child-a",
                        depth: 1,
                        itemId: 11,
                        text: "Child A",
                        icon: "viewMoreSymbolicDefault",
                        expanded: false,
                        children: [
                            { key: "leaf-a1", depth: 2, itemId: 12, text: "Leaf A1", icon: "viewMoreSymbolicBorderless" }
                        ]
                    },
                    {
                        key: "child-b",
                        depth: 1,
                        itemId: 20,
                        text: "Child B",
                        icon: "viewMoreSymbolicDisabled"
                    }
                ]
            }
        ]
    }

    Component.onCompleted: {
        Qt.callLater(tryActivateLeaf)
    }

    property bool treeApiReady:
        hierarchy.activeListItem !== null
        && hierarchy.activeListItemKey === "leaf-a1"
        && hierarchy.activeListItemId === 12
        && hierarchy.activeListItem.label === "Leaf A1"
        && hierarchy.activeListItem.iconName === "viewMoreSymbolicBorderless"
        && hierarchy.activeListItem.pathLabel === "Root / Child A / Leaf A1"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("treeApiReady").toBool());
}

void ImportApiTests::hierarchy_string_array_model_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.Hierarchy {
        id: hierarchy
        width: 240
        height: 220
        model: ["Overview", "Reports", "Settings"]
    }

    property bool stringModelReady:
        hierarchy.activeListItem !== null
        && hierarchy.activeListItem.label === "Overview"
        && hierarchy.activeListItemKey === "0"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("stringModelReady").toBool());
}

void ImportApiTests::hierarchy_nested_children_indent_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 280
    height: 280

    property bool activatedGreat: false

    function tryActivateGreat() {
        if (activatedGreat)
            return
        if (hierarchyList.itemCount < 4)
            return
        activatedGreat = hierarchyList.activateByKey("great")
    }

    LV.HierarchyList {
        id: hierarchyList
        visible: false
        width: 200
        model: [
            {
                key: "root",
                depth: 0,
                label: "Root",
                iconName: "projectStructure",
                expanded: true,
                children: [
                    {
                        key: "child",
                        depth: 1,
                        label: "Child",
                        iconName: "viewMoreSymbolicDefault",
                        children: [
                            {
                                key: "grand",
                                depth: 2,
                                label: "Grand",
                                iconName: "viewMoreSymbolicBorderless",
                                children: [
                                    {
                                        key: "great",
                                        depth: 3,
                                        label: "Great",
                                        iconName: "viewMoreSymbolicDisabled"
                                    }
                                ]
                            }
                        ]
                    }
                ]
            }
        ]
    }

    Component.onCompleted: Qt.callLater(root.tryActivateGreat)

    Connections {
        target: hierarchyList
        function onItemCountChanged() {
            root.tryActivateGreat()
        }
    }

    property bool nestedContractReady: {
        const items = hierarchyList.collectItems()
        if (!items || items.length !== 4)
            return false

        const rootItem = items[0]
        const childItem = items[1]
        const grandItem = items[2]
        const greatItem = items[3]

        return rootItem.label === "Root"
            && rootItem.iconName === "projectStructure"
            && childItem.label === "Child"
            && childItem.iconName === "viewMoreSymbolicDefault"
            && grandItem.label === "Grand"
            && grandItem.iconName === "viewMoreSymbolicBorderless"
            && greatItem.label === "Great"
            && greatItem.iconName === "viewMoreSymbolicDisabled"
            && rootItem.indentLevel === 0
            && childItem.indentLevel === 1
            && grandItem.indentLevel === 2
            && greatItem.indentLevel === 3
            && rootItem.computedLeftPadding === 8
            && childItem.computedLeftPadding === 16
            && grandItem.computedLeftPadding === 24
            && greatItem.computedLeftPadding === 32
            && activatedGreat
            && hierarchyList.activeItemKey === "great"
            && childItem.expanded
            && grandItem.expanded
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("nestedContractReady").toBool());
}

void ImportApiTests::hierarchy_optional_footer_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 320
    height: 420

    property int footerSignalCount: 0
    property int footerCallbackCount: 0
    property bool footerTriggerResult: false
    property string footerIconName: ""

    LV.Hierarchy {
        id: hierarchy
        width: 200
        height: 320
        toolbarItems: [
            { id: "structure", iconName: "projectStructure" },
            { id: "layers", iconName: "projectStructure" }
        ]
        model: [
            {
                key: "root",
                depth: 0,
                label: "Root",
                iconName: "projectStructure",
                expanded: true,
                children: [
                    { key: "child", depth: 1, label: "Child", iconName: "viewMoreSymbolicDefault" }
                ]
            }
        ]
        footerVisible: true
        footerButton1: ({
            type: "icon",
            iconName: "projectStructure",
            onClicked: function() { root.footerCallbackCount += 1 }
        })
        footerButton2: ({ type: "icon", iconName: "delete" })
        footerButton3: ({ type: "menu", iconName: "viewMoreSymbolicDefault" })

        onFooterButtonTriggered: function(index, config) {
            root.footerSignalCount += 1
            if (index === 0 && config && config.iconName !== undefined)
                root.footerIconName = String(config.iconName)
        }
    }

    Component.onCompleted: {
        Qt.callLater(function() {
            root.footerTriggerResult = hierarchy.triggerFooterButton(0)
        })
    }

    property bool footerContractReady:
        hierarchy.footerVisible
        && hierarchy.toolbarItems.length === 2
        && hierarchy.model.length === 1
        && footerTriggerResult
        && footerSignalCount === 1
        && footerCallbackCount === 1
        && footerIconName === "projectStructure"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("footerContractReady").toBool());
}

void ImportApiTests::hierarchy_toolbar_item_model_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 300
    height: 120

    property int activatedCount: 0
    property int triggeredCount: 0
    property int eventCount: 0
    property string callbackButtonId: ""
    property string lastEventName: ""
    property string payloadKind: ""
    property bool disabledTriggered: false
    property bool outOfRangeTriggered: false
    property bool iconNameResolved: false

    property var toolbarItems: [
        {
            id: "structure",
            iconName: "projectStructure",
            selected: true
        },
        {
            id: "layers",
            iconName: "projectStructure",
            events: [
                "hierarchy.layers",
                { name: "analytics.hierarchy.layers", payload: ({ "kind": "analytics" }) }
            ],
            onClicked: function(ctx) {
                root.callbackButtonId = String(ctx.buttonId)
                ctx.emit("hierarchy.layers.custom", ({ "kind": "custom" }))
            }
        },
        {
            id: "disabled",
            iconName: "projectStructure",
            enabled: false,
            eventName: "hierarchy.disabled"
        }
    ]

    LV.HierarchyToolbar {
        id: toolbar
        visible: false
        width: 200
        buttonItems: root.toolbarItems
        onActiveChanged: function(button, buttonId, index) {
            root.activatedCount += 1
        }
        onButtonTriggered: function(button, buttonId, index, item) {
            root.triggeredCount += 1
        }
        onButtonEventTriggered: function(eventName, payload, index, item, buttonId) {
            root.eventCount += 1
            root.lastEventName = eventName
            if (payload && payload.kind !== undefined)
                root.payloadKind = payload.kind
        }
    }

    Component.onCompleted: {
        const buttons = toolbar.collectButtons()
        root.iconNameResolved = buttons.length > 0 && buttons[0].resolvedIconName === "projectStructure"

        toolbar.triggerIndex(1)
        root.disabledTriggered = toolbar.triggerIndex(2)
        root.outOfRangeTriggered = toolbar.triggerIndex(9)
    }

    property bool toolbarContractReady:
        toolbar.itemCount === 3
        && toolbar.buttonCount === 3
        && toolbar.activeButtonId === "layers"
        && toolbar.activeIndex === 1
        && activatedCount === 1
        && triggeredCount === 1
        && eventCount === 3
        && callbackButtonId === "layers"
        && lastEventName === "hierarchy.layers.custom"
        && payloadKind === "custom"
        && disabledTriggered === false
        && outOfRangeTriggered === false
        && iconNameResolved
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("toolbarContractReady").toBool());
}

void ImportApiTests::hierarchy_toolbar_figma_layout_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 220
    height: 60

    property int probeAttempts: 0
    property bool layoutContractReady: false
    property string layoutDiagnostics: ""

    property var toolbarItems: [
        { id: "slot0", iconName: "projectStructure", selected: true },
        { id: "slot1", iconName: "projectStructure" },
        { id: "slot2", iconName: "projectStructure" },
        { id: "slot3", iconName: "projectStructure" },
        { id: "slot4", iconName: "projectStructure" },
        { id: "slot5", iconName: "projectStructure" },
        { id: "slot6", iconName: "projectStructure" },
        { id: "slot7", iconName: "projectStructure" }
    ]

    LV.HierarchyToolbar {
        id: toolbar
        width: 200
        height: 20
        buttonItems: root.toolbarItems
    }

    function approximatelyEqual(leftValue, rightValue, tolerance) {
        return Math.abs(leftValue - rightValue) <= tolerance
    }

    function evaluateLayoutContract() {
        const buttons = toolbar.collectButtons().slice().sort(function(leftButton, rightButton) {
            return leftButton.x - rightButton.x
        })
        if (buttons.length !== 8) {
            root.layoutDiagnostics = "buttonCount=" + buttons.length
            return false
        }

        const expectedX = [
            0.0,
            25.7142857143,
            51.4285714286,
            77.1428571429,
            102.8571428571,
            128.5714285714,
            154.2857142857,
            180.0
        ]

        for (let index = 0; index < expectedX.length; index++) {
            const button = buttons[index]
            if (!button || !button.visible)
                return false
            if (!approximatelyEqual(button.x, expectedX[index], 0.8)) {
                root.layoutDiagnostics = "xMismatch index=" + index
                    + " actual=" + button.x
                    + " expected=" + expectedX[index]
                    + " spacing=" + toolbar.distributedSpacing
                return false
            }
            if (!approximatelyEqual(button.width, 20.0, 0.2)) {
                root.layoutDiagnostics = "widthMismatch index=" + index + " width=" + button.width
                return false
            }
            if (!approximatelyEqual(button.height, 20.0, 0.2)) {
                root.layoutDiagnostics = "heightMismatch index=" + index + " height=" + button.height
                return false
            }
        }
        const ready = toolbar.horizontalPadding === 0
            && toolbar.verticalPadding === 0
            && approximatelyEqual(toolbar.backgroundOpacity, 0.0, 0.001)
            && toolbar.activeButtonId === "slot0"
            && toolbar.activeIndex === 0
        if (!ready) {
            root.layoutDiagnostics = "stateMismatch hPad=" + toolbar.horizontalPadding
                + " vPad=" + toolbar.verticalPadding
                + " bgOpacity=" + toolbar.backgroundOpacity
                + " activeId=" + toolbar.activeButtonId
                + " activeIndex=" + toolbar.activeIndex
        }
        return ready
    }

    function probeLayoutContract() {
        root.probeAttempts += 1
        root.layoutContractReady = evaluateLayoutContract()
        if (!root.layoutContractReady && root.probeAttempts < 10)
            Qt.callLater(probeLayoutContract)
    }

    Component.onCompleted: {
        Qt.callLater(probeLayoutContract)
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY2(root->property("layoutContractReady").toBool(),
                 qPrintable(root->property("layoutDiagnostics").toString()));
}

void ImportApiTests::hierarchy_toolbar_manual_icon_button_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 220
    height: 80

    property int probeAttempts: 0
    property int activatedCount: 0
    property bool contractReady: false

    LV.HierarchyToolbar {
        id: toolbar
        width: 120
        height: 20
        buttonItems: []

        LV.IconButton { iconName: "projectStructure" }
        LV.IconButton { iconName: "projectStructure" }
        LV.IconButton { iconName: "projectStructure" }

        onActiveChanged: function(button, buttonId, index) {
            root.activatedCount += 1
        }
    }

    function evaluateContract() {
        const buttons = toolbar.collectButtons()
        if (buttons.length !== 3)
            return false

        const secondButton = buttons[1]
        if (!secondButton)
            return false

        if (secondButton.click !== undefined)
            secondButton.click()
        else if (secondButton.clicked !== undefined)
            secondButton.clicked()

        return toolbar.buttonCount === 3
            && toolbar.activeIndex === 1
            && toolbar.activeButtonId === 1
            && root.activatedCount === 1
            && buttons[1].tone === LV.AbstractButton.Default
            && buttons[0].tone === LV.AbstractButton.Borderless
            && buttons[2].tone === LV.AbstractButton.Borderless
    }

    function probeContract() {
        root.probeAttempts += 1
        root.contractReady = evaluateContract()
        if (!root.contractReady && root.probeAttempts < 10)
            Qt.callLater(probeContract)
    }

    Component.onCompleted: Qt.callLater(probeContract)
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("contractReady").toBool());
}

void ImportApiTests::hierarchy_row_click_only_activates_not_toggles()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    property int rowProbeAttempts: 0

    LV.Hierarchy {
        id: hierarchy
        width: 260
        height: 220
        model: [
            {
                key: "root",
                depth: 0,
                label: "Root",
                expanded: true,
                selected: true,
                children: [
                    {
                        key: "branch",
                        depth: 1,
                        label: "Branch",
                        expanded: false,
                        children: [
                            { key: "leaf", depth: 2, label: "Leaf" }
                        ]
                    }
                ]
            }
        ]
    }

    property bool rowClickToggleBlocked: false
    property bool collapsedBranchClickActivates: false
    property bool rowClickContractReady: rowClickToggleBlocked && collapsedBranchClickActivates

    function evaluateRowClickToggle() {
        const row = hierarchy.activeListItem
        const list = row && row.hierarchyList ? row.hierarchyList : null
        const collapsedBranch = list && list.resolveByKey ? list.resolveByKey("branch") : null
        if (!row || !row.clicked || !list || !collapsedBranch || !collapsedBranch.clicked) {
            if (rowProbeAttempts < 40) {
                rowProbeAttempts += 1
                Qt.callLater(evaluateRowClickToggle)
            }
            return
        }

        const rootWasExpanded = !!row.expanded
        row.clicked()
        rowClickToggleBlocked = (row.expanded === rootWasExpanded)

        const branchWasExpanded = !!collapsedBranch.expanded
        collapsedBranch.clicked()
        collapsedBranchClickActivates =
            list.activeItem === collapsedBranch
            && collapsedBranch.expanded === branchWasExpanded
    }

    Component.onCompleted: {
        Qt.callLater(evaluateRowClickToggle)
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("rowClickContractReady").toBool());
}

void ImportApiTests::hierarchy_chevron_requires_children_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 260
    height: 220

    LV.HierarchyList {
        id: list
        visible: false

        LV.HierarchyItem {
            id: parentItem
            label: "Parent"
            indentLevel: 0
            showChevron: true
            expanded: true
        }

        LV.HierarchyItem {
            id: childLeaf
            label: "Child Leaf"
            indentLevel: 1
            showChevron: true
        }

        LV.HierarchyItem {
            id: rootLeaf
            label: "Root Leaf"
            indentLevel: 0
            showChevron: true
        }
    }

    property bool chevronRuleReady:
        list.itemCount === 3
        && parentItem.hasChildItems
        && parentItem.effectiveShowChevron
        && !childLeaf.hasChildItems
        && !childLeaf.effectiveShowChevron
        && !rootLeaf.hasChildItems
        && !rootLeaf.effectiveShowChevron
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("chevronRuleReady").toBool());
}

void ImportApiTests::hierarchy_item_chevron_direction_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.HierarchyItem {
        id: autoCollapsed
        visible: false
        showChevron: true
        hasChildItems: true
        expanded: false
        selectionDirection: "auto"
    }

    LV.HierarchyItem {
        id: autoExpanded
        visible: false
        showChevron: true
        hasChildItems: true
        expanded: true
        selectionDirection: "auto"
    }

    LV.HierarchyItem {
        id: forcedUp
        visible: false
        showChevron: true
        hasChildItems: true
        expanded: false
        selectionDirection: "up"
    }

    LV.HierarchyItem {
        id: leafNode
        visible: false
        showChevron: true
        hasChildItems: false
        expanded: false
        selectionDirection: "auto"
    }

    property bool chevronDirectionReady:
        autoCollapsed.effectiveShowChevron
        && autoCollapsed.resolvedSelectionDirection === autoCollapsed.directionRight
        && autoCollapsed.resolvedChevronRotation === -90
        && autoExpanded.resolvedSelectionDirection === autoExpanded.directionDown
        && autoExpanded.resolvedChevronRotation === 0
        && forcedUp.resolvedSelectionDirection === forcedUp.directionUp
        && forcedUp.resolvedChevronRotation === 180
        && !leafNode.effectiveShowChevron
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("chevronDirectionReady").toBool());
}

void ImportApiTests::hierarchy_item_hover_and_active_state_visual_contract_loads()
{
    const QString requestedPlatform = qEnvironmentVariable("QT_QPA_PLATFORM").trimmed();
    if (requestedPlatform.compare(QStringLiteral("offscreen"), Qt::CaseInsensitive) == 0
        || QGuiApplication::platformName().compare(QStringLiteral("offscreen"), Qt::CaseInsensitive) == 0) {
        QSKIP("Hover pointer delivery is unavailable on offscreen platform plugin");
    }

    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import QtQuick.Window
import LVRS as LV

Window {
    id: root
    width: 320
    height: 200
    visible: true

    LV.HierarchyList {
        id: list
        objectName: "hierarchyList"
        anchors.fill: parent
        keyboardNavigationEnabled: false

        LV.HierarchyItem {
            id: itemA
            objectName: "itemA"
            itemKey: "itemA"
            label: "Item A"
            showChevron: false
            hasChildItems: false
            selected: true
        }

        LV.HierarchyItem {
            id: itemB
            objectName: "itemB"
            itemKey: "itemB"
            label: "Item B"
            showChevron: false
            hasChildItems: false
        }
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    auto *list = root->findChild<QObject *>(QStringLiteral("hierarchyList"));
    QVERIFY(list);
    auto *itemAObject = root->findChild<QObject *>(QStringLiteral("itemA"));
    auto *itemBObject = root->findChild<QObject *>(QStringLiteral("itemB"));
    QVERIFY(itemAObject);
    QVERIFY(itemBObject);
    auto *itemA = qobject_cast<QQuickItem *>(itemAObject);
    auto *itemB = qobject_cast<QQuickItem *>(itemBObject);
    QVERIFY(itemA);
    QVERIFY(itemB);

    QTRY_VERIFY(list->property("activeItem").value<QObject *>() == itemAObject);
    QTRY_COMPARE(itemAObject->property("state").toString(), QStringLiteral("Active"));
    QTRY_COMPARE(itemBObject->property("state").toString(), QStringLiteral("Idle"));
    QVERIFY(!itemBObject->property("isHoverState").toBool());
    QVERIFY(!itemBObject->property("isActiveState").toBool());

    const QPointF hoverPoint = itemB->mapToScene(QPointF(itemB->width() * 0.5, itemB->height() * 0.5));
    const QPoint hoverPointInt(qRound(hoverPoint.x()), qRound(hoverPoint.y()));
    QTest::mouseMove(window, hoverPointInt, 10);

    QTRY_VERIFY(itemBObject->property("isHoverState").toBool());
    QTRY_COMPARE(itemBObject->property("state").toString(), QStringLiteral("Hover"));

    QObject *hoverBackground = itemBObject->property("background").value<QObject *>();
    QVERIFY(hoverBackground);
    const QColor hoverRenderedColor = hoverBackground->property("color").value<QColor>();
    const QColor expectedHoverColor = itemBObject->property("backgroundColorHover").value<QColor>();
    QCOMPARE(hoverRenderedColor, expectedHoverColor);

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, hoverPointInt, 10);

    QTRY_VERIFY(list->property("activeItem").value<QObject *>() == itemBObject);
    QTRY_VERIFY(itemBObject->property("isActiveState").toBool());
    QTRY_COMPARE(itemBObject->property("state").toString(), QStringLiteral("Active"));
    QTRY_VERIFY(!itemAObject->property("isActiveState").toBool());

    QObject *activeBackground = itemBObject->property("background").value<QObject *>();
    QVERIFY(activeBackground);
    const QColor activeRenderedColor = activeBackground->property("color").value<QColor>();
    const QColor expectedActiveColor = itemBObject->property("backgroundColor").value<QColor>();
    QCOMPARE(activeRenderedColor, expectedActiveColor);
}

void ImportApiTests::hierarchy_item_inputable_overlay_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property int editedCount: 0
    property int submittedCount: 0
    property string editedValue: ""
    property string submittedValue: ""
    property string applyResult: ""

    LV.HierarchyItem {
        id: item
        visible: false
        label: "Node"
        onInputEdited: function(text) {
            root.editedCount += 1
            root.editedValue = text
        }
        onInputSubmitted: function(text) {
            root.submittedCount += 1
            root.submittedValue = text
        }
    }

    Component.onCompleted: {
        item.inputable = true
        item.applyInputResult("Node 2")
        applyResult = item.applyInputResult("Node 3")
        item.inputEdited("Node 4")
        item.inputSubmitted("Node 5")
    }

    property bool contractReady:
        item.inputable === true
        && item.rowHeight === 20
        && item.indentStep === 8
        && applyResult === "Node 3"
        && item.inputResult === "Node 3"
        && item.label === "Node 3"
        && editedCount === 1
        && submittedCount === 1
        && editedValue === "Node 4"
        && submittedValue === "Node 5"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("contractReady").toBool());
}

void ImportApiTests::hierarchy_item_input_overlay_geometry_and_close_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    width: 360
    height: 120

    LV.HierarchyItem {
        id: item
        objectName: "item"
        width: 320
        label: "Node"
        indentLevel: 2
        showChevron: false
        hasChildItems: false
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);

    auto *itemObject = root->findChild<QObject *>(QStringLiteral("item"));
    QVERIFY(itemObject);
    auto *item = qobject_cast<QQuickItem *>(itemObject);
    QVERIFY(item);

    auto *labelObject = itemObject->findChild<QObject *>(QStringLiteral("hierarchyItemLabel"), Qt::FindChildrenRecursively);
    auto *loaderObject = itemObject->findChild<QObject *>(QStringLiteral("hierarchyItemInputLoader"), Qt::FindChildrenRecursively);
    QVERIFY(labelObject);
    QVERIFY(loaderObject);
    auto *labelItem = qobject_cast<QQuickItem *>(labelObject);
    QVERIFY(labelItem);

    itemObject->setProperty("inputable", true);

    QTRY_VERIFY(loaderObject->property("active").toBool());
    auto *overlayObject = itemObject->findChild<QObject *>(QStringLiteral("hierarchyItemInputOverlay"), Qt::FindChildrenRecursively);
    QTRY_VERIFY(overlayObject != nullptr);
    auto *overlayItem = qobject_cast<QQuickItem *>(overlayObject);
    QVERIFY(overlayItem);

    const QPointF labelPos = labelItem->mapToItem(item, QPointF(0, 0));
    const QPointF overlayPos = overlayItem->mapToItem(item, QPointF(0, 0));

    QVERIFY2(qAbs(labelPos.x() - overlayPos.x()) <= 0.5, "Overlay x must match label x.");
    QVERIFY2(qAbs(labelPos.y() - overlayPos.y()) <= 0.5, "Overlay y must match label y.");
    QVERIFY2(qAbs(labelItem->width() - overlayItem->width()) <= 0.5, "Overlay width must match label width.");
    QVERIFY2(qAbs(labelItem->height() - overlayItem->height()) <= 0.5, "Overlay height must match label height.");

    const bool acceptedInvoked = QMetaObject::invokeMethod(overlayObject,
                                                            "accepted",
                                                            Q_ARG(QString, QStringLiteral("Node Renamed")));
    QVERIFY(acceptedInvoked);

    QTRY_VERIFY(!itemObject->property("inputable").toBool());
    QTRY_COMPARE(itemObject->property("inputResult").toString(), QStringLiteral("Node Renamed"));
    QTRY_COMPARE(itemObject->property("label").toString(), QStringLiteral("Node Renamed"));
    QTRY_VERIFY(labelObject->property("visible").toBool());
    QTRY_VERIFY(!loaderObject->property("active").toBool());
}

void ImportApiTests::button_padding_matches_figma_spec()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    property string iconRoot: "qrc:/qt/qml/LVRS/resources/iconset/"
    property string expectedFallbackIcon: iconRoot + "projectStructure.svg"
    property color transparentColor: "transparent"

    LV.LabelButton { id: labelButton; text: "Button"; tone: LV.AbstractButton.Primary; visible: false }
    LV.IconButton { id: iconButton; tone: LV.AbstractButton.Primary; visible: false }
    LV.LabelMenuButton { id: labelMenuButton; text: "Open"; tone: LV.AbstractButton.Primary; visible: false }
    LV.IconMenuButton { id: iconMenuButton; tone: LV.AbstractButton.Primary; visible: false }
    LV.LabelButton { id: labelButtonDefault; text: "Button"; tone: LV.AbstractButton.Default; visible: false }
    LV.IconButton { id: iconButtonDefault; tone: LV.AbstractButton.Default; visible: false }
    LV.LabelMenuButton { id: labelMenuButtonDefault; text: "Open"; tone: LV.AbstractButton.Default; visible: false }
    LV.IconMenuButton { id: iconMenuButtonDefault; tone: LV.AbstractButton.Default; visible: false }
    LV.LabelButton { id: labelButtonBorderless; text: "Button"; tone: LV.AbstractButton.Borderless; visible: false }
    LV.IconButton { id: iconButtonBorderless; tone: LV.AbstractButton.Borderless; visible: false }
    LV.LabelMenuButton { id: labelMenuButtonBorderless; text: "Open"; tone: LV.AbstractButton.Borderless; visible: false }
    LV.IconMenuButton { id: iconMenuButtonBorderless; tone: LV.AbstractButton.Borderless; visible: false }
    LV.LabelButton { id: labelButtonDestructive; text: "Button"; tone: LV.AbstractButton.Destructive; visible: false }
    LV.IconButton { id: iconButtonDestructive; tone: LV.AbstractButton.Destructive; visible: false }
    LV.LabelMenuButton { id: labelMenuButtonDestructive; text: "Open"; tone: LV.AbstractButton.Destructive; visible: false }
    LV.IconMenuButton { id: iconMenuButtonDestructive; tone: LV.AbstractButton.Destructive; visible: false }
    LV.LabelButton { id: labelButtonDisabled; text: "Button"; tone: LV.AbstractButton.Disabled; visible: false }
    LV.IconButton { id: iconButtonDisabled; tone: LV.AbstractButton.Disabled; visible: false }
    LV.LabelMenuButton { id: labelMenuButtonDisabled; text: "Open"; tone: LV.AbstractButton.Disabled; visible: false }
    LV.IconMenuButton { id: iconMenuButtonDisabled; tone: LV.AbstractButton.Disabled; visible: false }

    property bool figmaPaddingReady:
        labelButton.horizontalPadding === LV.Theme.gap8
        && labelButton.verticalPadding === LV.Theme.gap4
        && iconButton.horizontalPadding === LV.Theme.gap2
        && iconButton.verticalPadding === LV.Theme.gap2
        && labelMenuButton.horizontalPadding === LV.Theme.gap8
        && labelMenuButton.verticalPadding === LV.Theme.gap2
        && iconMenuButton.horizontalPadding === LV.Theme.gap2
        && iconMenuButton.verticalPadding === LV.Theme.gap2
        && labelMenuButton.spacing === LV.Theme.gap2
        && iconMenuButton.spacing === LV.Theme.gap4
        && Math.abs(labelButton.implicitHeight - LV.Theme.gap20) < 0.01
        && Math.abs(iconButton.implicitHeight - LV.Theme.gap20) < 0.01
        && Math.abs(labelMenuButton.implicitHeight - LV.Theme.gap20) < 0.01
        && Math.abs(iconMenuButton.implicitHeight - LV.Theme.gap20) < 0.01
        && Math.abs(labelButton.implicitHeight - iconButton.implicitHeight) < 0.01
        && Math.abs(iconButton.implicitHeight - labelMenuButton.implicitHeight) < 0.01
        && Math.abs(labelMenuButton.implicitHeight - iconMenuButton.implicitHeight) < 0.01
        && Math.abs(labelButton.height - LV.Theme.gap20) < 0.01
        && Math.abs(iconButton.height - LV.Theme.gap20) < 0.01
        && Math.abs(labelMenuButton.height - LV.Theme.gap20) < 0.01
        && Math.abs(iconMenuButton.height - LV.Theme.gap20) < 0.01
        && Math.abs(labelButtonDefault.height - LV.Theme.gap20) < 0.01
        && Math.abs(iconButtonDefault.height - LV.Theme.gap20) < 0.01
        && Math.abs(labelMenuButtonDefault.height - LV.Theme.gap20) < 0.01
        && Math.abs(iconMenuButtonDefault.height - LV.Theme.gap20) < 0.01
        && Math.abs(labelButtonBorderless.height - LV.Theme.gap20) < 0.01
        && Math.abs(iconButtonBorderless.height - LV.Theme.gap20) < 0.01
        && Math.abs(labelMenuButtonBorderless.height - LV.Theme.gap20) < 0.01
        && Math.abs(iconMenuButtonBorderless.height - LV.Theme.gap20) < 0.01
        && Math.abs(labelButtonDestructive.height - LV.Theme.gap20) < 0.01
        && Math.abs(iconButtonDestructive.height - LV.Theme.gap20) < 0.01
        && Math.abs(labelMenuButtonDestructive.height - LV.Theme.gap20) < 0.01
        && Math.abs(iconMenuButtonDestructive.height - LV.Theme.gap20) < 0.01
        && Math.abs(labelButtonDisabled.implicitHeight - LV.Theme.gap20) < 0.01
        && Math.abs(iconButtonDisabled.implicitHeight - LV.Theme.gap20) < 0.01
        && Math.abs(labelMenuButtonDisabled.implicitHeight - LV.Theme.gap20) < 0.01
        && Math.abs(iconMenuButtonDisabled.implicitHeight - LV.Theme.gap20) < 0.01
        && Math.abs(labelButtonDisabled.height - LV.Theme.gap20) < 0.01
        && Math.abs(iconButtonDisabled.height - LV.Theme.gap20) < 0.01
        && Math.abs(labelMenuButtonDisabled.height - LV.Theme.gap20) < 0.01
        && Math.abs(iconMenuButtonDisabled.height - LV.Theme.gap20) < 0.01
        && labelButtonDefault.backgroundColor === LV.Theme.panelBackground12
        && iconButtonDefault.backgroundColor === LV.Theme.panelBackground12
        && labelMenuButtonDefault.backgroundColor === LV.Theme.panelBackground12
        && iconMenuButtonDefault.backgroundColor === LV.Theme.panelBackground12
        && labelButtonBorderless.backgroundColor === transparentColor
        && iconButtonBorderless.backgroundColor === transparentColor
        && labelButtonDisabled.backgroundColorDisabled === LV.Theme.panelBackground04
        && iconButtonDisabled.backgroundColorDisabled === LV.Theme.panelBackground04
        && iconButton.resolvedIconSource.toString() === expectedFallbackIcon
        && iconButtonDefault.resolvedIconSource.toString() === expectedFallbackIcon
        && iconButtonBorderless.resolvedIconSource.toString() === expectedFallbackIcon
        && iconButtonDestructive.resolvedIconSource.toString() === expectedFallbackIcon
        && iconButtonDisabled.resolvedIconSource.toString() === expectedFallbackIcon
        && iconMenuButton.resolvedIconSource.toString() === expectedFallbackIcon
        && iconMenuButtonDefault.resolvedIconSource.toString() === expectedFallbackIcon
        && iconMenuButtonBorderless.resolvedIconSource.toString() === expectedFallbackIcon
        && iconMenuButtonDestructive.resolvedIconSource.toString() === expectedFallbackIcon
        && iconMenuButtonDisabled.resolvedIconSource.toString() === expectedFallbackIcon
        && labelMenuButton.resolvedIndicatorName === "generalchevronDownAccent"
        && labelMenuButtonDefault.resolvedIndicatorName === "generalchevronDown"
        && labelMenuButtonBorderless.resolvedIndicatorName === "generalchevronDownBorderless"
        && labelMenuButtonDestructive.resolvedIndicatorName === "generalchevronDownAccent"
        && labelMenuButtonDisabled.resolvedIndicatorName === "generalchevronDownDisabled"
        && iconMenuButton.resolvedIndicatorName === "generalchevronDownAccent"
        && iconMenuButtonDefault.resolvedIndicatorName === "generalchevronDown"
        && iconMenuButtonBorderless.resolvedIndicatorName === "generalchevronDownBorderless"
        && iconMenuButtonDestructive.resolvedIndicatorName === "generalchevronDownAccent"
        && iconMenuButtonDisabled.resolvedIndicatorName === "generalchevronDownDisabled"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("figmaPaddingReady").toBool());
}

void ImportApiTests::button_default_tone_fallback_borderless_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    property color transparentColor: "transparent"

    LV.LabelButton { id: labelButton; text: "Button"; visible: false }
    LV.IconButton { id: iconButton; visible: false }
    LV.LabelMenuButton { id: labelMenuButton; text: "Open"; visible: false }
    LV.IconMenuButton { id: iconMenuButton; visible: false }

    property bool defaultFallbackReady:
        labelButton.tone === LV.AbstractButton.Borderless
        && iconButton.tone === LV.AbstractButton.Borderless
        && labelMenuButton.tone === LV.AbstractButton.Borderless
        && iconMenuButton.tone === LV.AbstractButton.Borderless
        && labelButton.backgroundColor === transparentColor
        && iconButton.backgroundColor === transparentColor
        && labelMenuButton.backgroundColor === transparentColor
        && iconMenuButton.backgroundColor === transparentColor
        && labelMenuButton.resolvedIndicatorName === "generalchevronDownBorderless"
        && iconMenuButton.resolvedIndicatorName === "generalchevronDownBorderless"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("defaultFallbackReady").toBool());
}

void ImportApiTests::stepper_figma_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    property color transparentColor: "transparent"

    LV.Stepper { id: defaultStepper; visible: false }
    LV.Stepper { id: primaryUpDown; visible: false; tone: LV.AbstractButton.Primary; arrow: LV.Stepper.UpDown }
    LV.Stepper { id: primaryUp; visible: false; tone: LV.AbstractButton.Primary; arrow: LV.Stepper.Up }
    LV.Stepper { id: primaryDown; visible: false; tone: LV.AbstractButton.Primary; arrow: LV.Stepper.Down }
    LV.Stepper { id: borderlessUpDown; visible: false; tone: LV.AbstractButton.Borderless; arrow: LV.Stepper.UpDown }
    LV.Stepper { id: borderlessUp; visible: false; tone: LV.AbstractButton.Borderless; arrow: LV.Stepper.Up }
    LV.Stepper { id: borderlessDown; visible: false; tone: LV.AbstractButton.Borderless; arrow: LV.Stepper.Down }

    property bool stepperContractReady:
        defaultStepper.tone === LV.AbstractButton.Primary
        && defaultStepper.arrow === LV.Stepper.UpDown
        && Math.abs(defaultStepper.width - LV.Theme.iconSm) < 0.01
        && Math.abs(defaultStepper.height - LV.Theme.iconSm) < 0.01
        && Math.abs(defaultStepper.implicitWidth - LV.Theme.iconSm) < 0.01
        && Math.abs(defaultStepper.implicitHeight - LV.Theme.iconSm) < 0.01
        && Math.abs(defaultStepper.cornerRadius - LV.Theme.radiusSm) < 0.01
        && Math.abs(primaryUp.iconWidth - 10.0) < 0.01
        && Math.abs(primaryUp.iconHeight - 6.0) < 0.01
        && Math.abs(primaryDown.iconWidth - 10.0) < 0.01
        && Math.abs(primaryDown.iconHeight - 6.0) < 0.01
        && Math.abs(primaryUpDown.iconWidth - 6.436) < 0.05
        && Math.abs(primaryUpDown.iconHeight - 11.146) < 0.05
        && Math.abs(borderlessUpDown.iconWidth - 6.436) < 0.05
        && Math.abs(borderlessUpDown.iconHeight - 11.146) < 0.05
        && primaryUp.backgroundColor === LV.Theme.primary
        && borderlessUp.backgroundColor === transparentColor
        && borderlessUp.backgroundColorHover === LV.Theme.surfaceAlt
        && borderlessUp.backgroundColorPressed === LV.Theme.accentBlueMuted
        && primaryUp.resolvedIconColor === LV.Theme.accentWhite
        && primaryDown.resolvedIconColor === LV.Theme.accentWhite
        && borderlessUp.resolvedIconColor === LV.Theme.primary
        && borderlessDown.resolvedIconColor === LV.Theme.primary
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("stepperContractReady").toBool());
}

void ImportApiTests::combo_box_figma_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.ComboBox { id: defaultCombo; visible: false }
    LV.ComboBox { id: customTextCombo; visible: false; text: "Control" }
    LV.ComboBox { id: primaryUpDown; visible: false; tone: LV.ComboBox.Primary; arrow: LV.Stepper.UpDown }
    LV.ComboBox { id: primaryUp; visible: false; tone: LV.ComboBox.Primary; arrow: LV.Stepper.Up }
    LV.ComboBox { id: primaryDown; visible: false; tone: LV.ComboBox.Primary; arrow: LV.Stepper.Down }
    LV.ComboBox { id: borderlessUpDown; visible: false; tone: LV.ComboBox.Borderless; arrow: LV.Stepper.UpDown }
    LV.ComboBox { id: borderlessUp; visible: false; tone: LV.ComboBox.Borderless; arrow: LV.Stepper.Up }
    LV.ComboBox { id: borderlessDown; visible: false; tone: LV.ComboBox.Borderless; arrow: LV.Stepper.Down }

    property bool comboBoxContractReady:
        defaultCombo.tone === LV.ComboBox.Primary
        && defaultCombo.arrow === LV.Stepper.UpDown
        && defaultCombo.text === "Label"
        && customTextCombo.text === "Control"
        && Math.abs(defaultCombo.width - 97.0) < 0.01
        && Math.abs(defaultCombo.height - 18.0) < 0.01
        && Math.abs(defaultCombo.implicitWidth - 97.0) < 0.01
        && Math.abs(defaultCombo.implicitHeight - 18.0) < 0.01
        && Math.abs(defaultCombo.figmaComboWidth - 97.0) < 0.01
        && Math.abs(defaultCombo.figmaComboHeight - 18.0) < 0.01
        && Math.abs(defaultCombo.figmaComboLeftPadding - 8.0) < 0.01
        && Math.abs(defaultCombo.figmaComboRightPadding - 1.0) < 0.01
        && Math.abs(defaultCombo.figmaComboVerticalPadding - 1.0) < 0.01
        && Math.abs(defaultCombo.figmaComboCornerRadius - 5.0) < 0.01
        && defaultCombo.resolvedTone === LV.ComboBox.Primary
        && primaryUp.resolvedTone === LV.ComboBox.Primary
        && primaryDown.resolvedTone === LV.ComboBox.Primary
        && borderlessUp.resolvedTone === LV.ComboBox.Borderless
        && borderlessDown.resolvedTone === LV.ComboBox.Borderless
        && defaultCombo.resolvedArrow === LV.Stepper.UpDown
        && primaryUp.resolvedArrow === LV.Stepper.Up
        && primaryDown.resolvedArrow === LV.Stepper.Down
        && borderlessUpDown.resolvedArrow === LV.Stepper.UpDown
        && borderlessUp.resolvedArrow === LV.Stepper.Up
        && borderlessDown.resolvedArrow === LV.Stepper.Down
        && defaultCombo.backgroundColor === LV.Theme.panelBackground10
        && defaultCombo.backgroundColorHover === LV.Theme.panelBackground11
        && defaultCombo.backgroundColorPressed === LV.Theme.panelBackground12
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("comboBoxContractReady").toBool());
}

void ImportApiTests::input_field_figma_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.InputField {
        id: defaultField
        visible: false
        placeholderText: "Placeholder"
    }

    LV.InputField {
        id: searchField
        visible: false
        mode: searchMode
        placeholderText: "Search"
        text: "abc"
    }

    LV.InputField {
        id: passwordField
        visible: false
        placeholderText: "Password"
        echoMode: TextInput.Password
        text: "secret"
    }

    LV.InputField {
        id: readOnlyField
        visible: false
        placeholderText: "Read only"
        readOnly: true
        text: "locked"
    }

    property bool figmaInputFieldReady:
        defaultField.backgroundColor === LV.Theme.panelBackground10
        && defaultField.backgroundColorFocused === LV.Theme.panelBackground10
        && defaultField.backgroundColorDisabled === LV.Theme.panelBackground10
        && defaultField.textColor === LV.Theme.titleHeaderColor
        && defaultField.textColorDisabled === LV.Theme.disabledColor
        && defaultField.placeholderColor === LV.Theme.titleHeaderColor
        && defaultField.placeholderColorDisabled === LV.Theme.disabledColor
        && Math.abs(defaultField.placeholderOpacity - 1.0) < 0.001
        && defaultField.searchIconColor === LV.Theme.descriptionColor
        && defaultField.clearIconBackgroundColor === LV.Theme.descriptionColor
        && defaultField.clearIconBackgroundColorDisabled === LV.Theme.disabledColor
        && defaultField.clearIconForegroundColor === LV.Theme.panelBackground10
        && searchField.mode === searchField.searchMode
        && searchField.searchIconVisible
        && searchField.showClearButton
        && passwordField.echoMode === TextInput.Password
        && readOnlyField.readOnly
        && !readOnlyField.showClearButton
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("figmaInputFieldReady").toBool());
}

void ImportApiTests::toggle_switch_figma_color_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.ToggleSwitch { id: onSwitch; checked: true; enabled: true; visible: false }
    LV.ToggleSwitch { id: offSwitch; checked: false; enabled: true; visible: false }

    function trackOf(control) {
        if (!control || !control.indicator || !control.indicator.children || control.indicator.children.length < 2)
            return null
        return control.indicator.children[1]
    }

    readonly property var onTrack: trackOf(onSwitch)
    readonly property var offTrack: trackOf(offSwitch)

    property bool figmaToggleColorReady:
        onSwitch.onColor === LV.Theme.accent
        && onSwitch.offColor === LV.Theme.panelBackground12
        && onSwitch.knobFillColor === LV.Theme.textPrimary
        && onTrack !== null
        && offTrack !== null
        && onTrack.color === LV.Theme.accent
        && offTrack.color === LV.Theme.panelBackground12
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("figmaToggleColorReady").toBool());
}

void ImportApiTests::checkbox_figma_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.CheckBox { id: checkedEnabled; text: "Label"; checked: true; enabled: true; visible: false }
    LV.CheckBox { id: checkedDisabled; text: "Label"; checked: true; enabled: false; visible: false }
    LV.CheckBox { id: uncheckedEnabled; text: "Label"; checked: false; enabled: true; visible: false }
    LV.CheckBox { id: uncheckedDisabled; text: "Label"; checked: false; enabled: false; visible: false }

    function indicatorOf(control) {
        if (!control || !control.contentItem || control.contentItem.children.length < 1)
            return null
        return control.contentItem.children[0]
    }

    function labelOf(control) {
        if (!control || !control.contentItem || control.contentItem.children.length < 2)
            return null
        return control.contentItem.children[1]
    }

    readonly property var checkedEnabledIndicator: indicatorOf(checkedEnabled)
    readonly property var checkedDisabledIndicator: indicatorOf(checkedDisabled)
    readonly property var uncheckedEnabledIndicator: indicatorOf(uncheckedEnabled)
    readonly property var uncheckedDisabledIndicator: indicatorOf(uncheckedDisabled)
    readonly property var checkedEnabledLabel: labelOf(checkedEnabled)
    readonly property var checkedDisabledLabel: labelOf(checkedDisabled)

    property bool figmaCheckBoxReady:
        checkedEnabled.boxSize === 17
        && Math.abs(checkedEnabled.boxRadius - 3.5) < 0.01
        && checkedEnabled.contentItem.spacing === LV.Theme.gap6
        && checkedEnabled.checkColor === LV.Theme.bodyColor
        && checkedEnabled.checkMarkColorDisabled === LV.Theme.disabledColor
        && checkedEnabledIndicator !== null
        && checkedDisabledIndicator !== null
        && uncheckedEnabledIndicator !== null
        && uncheckedDisabledIndicator !== null
        && checkedEnabledIndicator.color === LV.Theme.accent
        && checkedDisabledIndicator.color === LV.Theme.panelBackground12
        && uncheckedEnabledIndicator.color === LV.Theme.bodyColor
        && uncheckedDisabledIndicator.color === LV.Theme.panelBackground12
        && Math.abs(checkedEnabledIndicator.border.width - 0) < 0.01
        && Math.abs(checkedDisabledIndicator.border.width - 0.5) < 0.01
        && Math.abs(uncheckedEnabledIndicator.border.width - 0.5) < 0.01
        && Math.abs(uncheckedDisabledIndicator.border.width - 0) < 0.01
        && checkedDisabledIndicator.border.color === LV.Theme.panelBackground12
        && uncheckedEnabledIndicator.border.color === LV.Theme.bodyColor
        && checkedEnabled.showInnerShadow === false
        && checkedDisabled.showInnerShadow === true
        && uncheckedEnabled.showInnerShadow === true
        && uncheckedDisabled.showInnerShadow === true
        && checkedEnabledLabel !== null
        && checkedDisabledLabel !== null
        && checkedEnabledLabel.color === LV.Theme.bodyColor
        && checkedDisabledLabel.color === LV.Theme.disabledColor
        && checkedEnabledLabel.font.pixelSize === LV.Theme.textBody
        && checkedEnabledLabel.font.weight === LV.Theme.textBodyWeight
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("figmaCheckBoxReady").toBool());
}

void ImportApiTests::radio_button_figma_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.RadioButton { id: onEnabled; checked: true; enabled: true; visible: false }
    LV.RadioButton { id: onDisabled; checked: true; enabled: false; visible: false }
    LV.RadioButton { id: offEnabled; checked: false; enabled: true; visible: false }
    LV.RadioButton { id: offDisabled; checked: false; enabled: false; visible: false }

    function indicatorOf(control) {
        if (!control || !control.contentItem || control.contentItem.children.length < 1)
            return null
        return control.contentItem.children[0]
    }

    function dotOf(control) {
        const indicator = indicatorOf(control)
        if (!indicator || !indicator.children || indicator.children.length < 1)
            return null
        return indicator.children[0]
    }

    readonly property var onEnabledIndicator: indicatorOf(onEnabled)
    readonly property var onDisabledIndicator: indicatorOf(onDisabled)
    readonly property var offEnabledIndicator: indicatorOf(offEnabled)
    readonly property var offDisabledIndicator: indicatorOf(offDisabled)
    readonly property var onEnabledDot: dotOf(onEnabled)
    readonly property var onDisabledDot: dotOf(onDisabled)

    property bool figmaRadioReady:
        onEnabled.indicatorSize === LV.Theme.controlIndicatorSize
        && onEnabled.dotSize === LV.Theme.gap8
        && onEnabled.onColor === LV.Theme.accent
        && onEnabled.offColor === LV.Theme.textPrimary
        && onEnabled.onColorDisabled === LV.Theme.panelBackground12
        && onEnabled.offColorDisabled === LV.Theme.panelBackground12
        && onEnabled.dotColor === LV.Theme.textPrimary
        && onEnabled.dotColorDisabled === LV.Theme.textSeptenary
        && onEnabledIndicator !== null
        && onDisabledIndicator !== null
        && offEnabledIndicator !== null
        && offDisabledIndicator !== null
        && onEnabledDot !== null
        && onDisabledDot !== null
        && onEnabledIndicator.color === LV.Theme.accent
        && onDisabledIndicator.color === LV.Theme.panelBackground12
        && offEnabledIndicator.color === LV.Theme.textPrimary
        && offDisabledIndicator.color === LV.Theme.panelBackground12
        && onEnabledDot.color === LV.Theme.textPrimary
        && onDisabledDot.color === LV.Theme.textSeptenary
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("figmaRadioReady").toBool());
}

void ImportApiTests::modal_empty_frame_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 420
    height: 320
    property int canceledCount: 0
    property bool contractReady: false

    LV.Modal {
        id: modal
        width: root.width
        height: root.height
        open: true
        onCanceled: root.canceledCount += 1
    }

    Component.onCompleted: {
        const insideX = modal.width * 0.5
        const insideY = (modal.height * 0.5) + modal.verticalOffset
        const insideIgnored = modal.handleBackdropClick(insideX, insideY)
        const outsideCanceled = modal.handleBackdropClick(1, 1)
        contractReady =
            modal.verticalOffset < 0
            && insideIgnored === false
            && outsideCanceled === true
            && !modal.open
            && root.canceledCount === 1
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("contractReady").toBool());
}

void ImportApiTests::modal_content_action_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 960
    height: 420
    property int primaryCount: 0
    property int secondaryCount: 0
    property int tertiaryCount: 0
    property bool contractReady: false

    LV.Modal {
        id: modal
        width: root.width
        height: root.height
        open: true
        title: "Unlock iPhone 15 Pro Max to Continue"
        description: "Xcode cannot launch because the device is locked."
        buttonCount: 4
        primaryText: "Cancel Running"
        secondaryText: "Later"
        tertiaryText: "Help"
        onPrimaryClicked: root.primaryCount += 1
        onSecondaryClicked: root.secondaryCount += 1
        onTertiaryClicked: root.tertiaryCount += 1
    }

    Component.onCompleted: {
        const insideX = modal.width * 0.5
        const insideY = (modal.height * 0.5) + modal.verticalOffset
        const insideIgnored = modal.handleBackdropClick(insideX, insideY)
        const primaryTriggered = modal.triggerAction(1)
        const secondaryTriggered = modal.triggerAction(2)
        const tertiaryTriggered = modal.triggerAction(3)
        const invalidTriggered = modal.triggerAction(4)
        contractReady =
            modal.verticalOffset < 0
            && modal.resolvedDescription.length > 0
            && modal.resolvedButtonCount === 3
            && modal.actionVisible(1)
            && modal.actionVisible(2)
            && modal.actionVisible(3)
            && insideIgnored === false
            && primaryTriggered
            && secondaryTriggered
            && tertiaryTriggered
            && !invalidTriggered
            && root.primaryCount === 1
            && root.secondaryCount === 1
            && root.tertiaryCount === 1
            && modal.open
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("contractReady").toBool());
}

void ImportApiTests::menu_item_key_and_chevron_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.MenuItem {
        id: collapsedSubmenu
        visible: false
        label: "Collapsed"
        keyVisible: true
        key: "Cmd+K"
        showChevron: true
        hasChildItems: true
        expanded: false
        selectionDirection: "auto"
    }

    LV.MenuItem {
        id: expandedSubmenu
        visible: false
        label: "Expanded"
        keyVisible: true
        key: "Alt+K"
        showChevron: true
        hasChildItems: true
        expanded: true
        selectionDirection: "auto"
    }

    LV.MenuItem {
        id: noChild
        visible: false
        label: "Leaf"
        keyVisible: true
        key: ""
        keyPlaceholder: "key"
        showChevron: true
        hasChildItems: false
        expanded: false
        selectionDirection: "auto"
    }

    LV.MenuItem {
        id: hiddenKey
        visible: false
        label: "HiddenKey"
        keyVisible: false
        key: "Ctrl+H"
        showChevron: false
        hasChildItems: false
        selectionDirection: "right"
    }

    property bool menuItemContract:
        collapsedSubmenu.keyVisible
        && collapsedSubmenu.resolvedShortcutText === "Cmd+K"
        && collapsedSubmenu.effectiveShowChevron
        && collapsedSubmenu.resolvedSelectionDirection === collapsedSubmenu.directionRight
        && expandedSubmenu.resolvedSelectionDirection === expandedSubmenu.directionDown
        && noChild.resolvedShortcutText === "key"
        && !noChild.effectiveShowChevron
        && hiddenKey.resolvedShortcutText === ""
        && hiddenKey.keyVisible === false
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("menuItemContract").toBool());
}

void ImportApiTests::context_menu_item_action_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 320
    height: 180

    property int triggerCount: 0
    property int eventCount: 0
    property string lastEvent: ""
    property string callbackMarker: ""
    property string callbackAliasMarker: ""
    property string payloadMarker: ""
    property bool dividerTriggered: false

    property var menuItems: [
        {
            id: "rename",
            label: "Rename",
            showChevron: false,
            eventName: "menu.rename",
            eventPayload: ({ "origin": "context-menu" }),
            keepOpen: true,
            onTriggered: function(ctx) {
                root.callbackMarker = "cb:" + ctx.eventName
                root.payloadMarker = ctx.payload.origin
            }
        },
        { type: "divider" },
        {
            id: "duplicate",
            label: "Duplicate",
            showChevron: false,
            events: [
                "menu.duplicate",
                { name: "menu.audit", payload: ({ "kind": "duplicate" }) }
            ],
            closeOnTrigger: true
        },
        {
            id: "share",
            label: "Share",
            showChevron: false,
            eventName: "",
            onClicked: function(ctx) {
                root.callbackAliasMarker = "alias:" + ctx.index
                ctx.emit("menu.share", ({ "kind": "share" }))
                ctx.close()
            }
        }
    ]

    LV.ContextMenu {
        id: menu
        visible: false
        items: root.menuItems
        autoCloseOnTrigger: true
        onItemTriggered: root.triggerCount += 1
        onItemEventTriggered: function(eventName, payload, index, item) {
            root.eventCount += 1
            root.lastEvent = eventName
            if (payload && payload.kind !== undefined)
                root.payloadMarker = payload.kind
        }
    }

    Component.onCompleted: {
        menu.triggerEntry(0)
        root.dividerTriggered = menu.triggerEntry(1)
        menu.triggerEntry(2)
        menu.triggerEntry(3)
    }

    property bool actionContract:
        triggerCount === 3
        && eventCount === 4
        && dividerTriggered === false
        && callbackMarker === "cb:menu.rename"
        && callbackAliasMarker === "alias:3"
        && lastEvent === "menu.share"
        && payloadMarker === "share"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("actionContract").toBool());
}

void ImportApiTests::context_menu_auto_placement_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 400
    height: 300

    LV.ContextMenu {
        id: menu
        visible: false
        edgeMargin: 4
        itemWidth: 120
    }

    property var downRight: menu.resolveOpenPlacement(40, 40, 120, 80, root.width, root.height)
    property var downLeft: menu.resolveOpenPlacement(390, 40, 120, 80, root.width, root.height)
    property var upRight: menu.resolveOpenPlacement(40, 295, 120, 80, root.width, root.height)
    property var upLeft: menu.resolveOpenPlacement(395, 295, 120, 80, root.width, root.height)
    property var oversized: menu.resolveOpenPlacement(200, 150, 500, 380, root.width, root.height)

    property bool placementContract:
        downRight.horizontalDirection === menu.directionRight
        && downRight.verticalDirection === menu.directionDown
        && downRight.x === 40
        && downRight.y === 40
        && downLeft.horizontalDirection === menu.directionLeft
        && downLeft.verticalDirection === menu.directionDown
        && downLeft.x === 270
        && downLeft.y === 40
        && upRight.horizontalDirection === menu.directionRight
        && upRight.verticalDirection === menu.directionUp
        && upRight.x === 40
        && upRight.y === 215
        && upLeft.horizontalDirection === menu.directionLeft
        && upLeft.verticalDirection === menu.directionUp
        && upLeft.x === 275
        && upLeft.y === 215
        && oversized.x === 0
        && oversized.y === 0
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("placementContract").toBool());
}

void ImportApiTests::table_cell_item_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property bool defaultsCaptured: false
    property int cellEditedCount: 0
    property int cellSubmittedCount: 0
    property string cellEditedValue: ""
    property string cellSubmittedValue: ""
    property int rowEditedColumn: -1
    property int rowSubmittedColumn: -1
    property string rowEditedValue: ""
    property string rowSubmittedValue: ""
    property int tableEditedRow: -1
    property int tableEditedColumn: -1
    property int tableSubmittedRow: -1
    property int tableSubmittedColumn: -1
    property string tableEditedValue: ""
    property string tableSubmittedValue: ""
    property string cellApplyResult: ""

    LV.TableCellItem {
        id: singleCell
        visible: false
        itemData: ({
            label: "Renderer",
            dividerColor: LV.Theme.panelBackground03,
            textColor: LV.Theme.bodyColor
        })
        onInputEdited: function(text) {
            root.cellEditedCount += 1
            root.cellEditedValue = text
        }
        onInputSubmitted: function(text) {
            root.cellSubmittedCount += 1
            root.cellSubmittedValue = text
        }
    }

    LV.TableHeader {
        id: header
        visible: false
        width: 717
        cellItems: [
            { label: "Name" },
            { text: "State" },
            { title: "Owner" }
        ]
    }

    LV.TableRow {
        id: row
        visible: false
        width: 717
        cellItems: [
            { text: "Renderer" },
            { text: "Active", inputable: true },
            "Core"
        ]
        onCellInputEdited: function(columnIndex, text) {
            root.rowEditedColumn = columnIndex
            root.rowEditedValue = text
        }
        onCellInputSubmitted: function(columnIndex, text) {
            root.rowSubmittedColumn = columnIndex
            root.rowSubmittedValue = text
        }
    }

    LV.Table {
        id: table
        visible: false
        width: 717
        headerCellItems: [
            { label: "Name" },
            { label: "State" },
            { label: "Owner" }
        ]
        rows: [
            [
                { text: "Renderer" },
                { text: "Active" },
                { text: "Core" }
            ]
        ]
        onCellInputEdited: function(rowIndex, columnIndex, text) {
            root.tableEditedRow = rowIndex
            root.tableEditedColumn = columnIndex
            root.tableEditedValue = text
        }
        onCellInputSubmitted: function(rowIndex, columnIndex, text) {
            root.tableSubmittedRow = rowIndex
            root.tableSubmittedColumn = columnIndex
            root.tableSubmittedValue = text
        }
    }

    Component.onCompleted: {
        defaultsCaptured = !singleCell.inputable && !row.inputable && !table.inputable

        singleCell.inputable = true
        singleCell.applyInputResult("Renderer v2")
        singleCell.inputEdited("Renderer v3")
        singleCell.inputSubmitted("Renderer v4")
        cellApplyResult = singleCell.applyInputResult("Renderer v5")

        row.cellInputEdited(1, "Active v2")
        row.cellInputSubmitted(2, "Core v2")

        table.cellInputEdited(0, 1, "Active v3")
        table.cellInputSubmitted(0, 2, "Core v3")
    }

    property bool tableCellContractReady:
        defaultsCaptured
        && singleCell.resolvedText === "Renderer"
        && cellApplyResult === "Renderer v5"
        && singleCell.inputResult === "Renderer v5"
        && singleCell.resolvedDividerColor === LV.Theme.panelBackground03
        && singleCell.resolvedTextColor === LV.Theme.bodyColor
        && cellEditedCount === 1
        && cellSubmittedCount === 1
        && cellEditedValue === "Renderer v3"
        && cellSubmittedValue === "Renderer v4"
        && header.resolvedColumnCount === 3
        && header.columnText(0) === "Name"
        && header.columnText(1) === "State"
        && header.columnText(2) === "Owner"
        && header.separatorColor === LV.Theme.panelBackground10
        && row.resolvedCellCount === 3
        && row.cellInputable(0) === false
        && row.cellInputable(1) === true
        && row.cellText(0) === "Renderer"
        && row.cellText(1) === "Active"
        && row.cellText(2) === "Core"
        && row.dividerColor === LV.Theme.panelBackground03
        && rowEditedColumn === 1
        && rowEditedValue === "Active v2"
        && rowSubmittedColumn === 2
        && rowSubmittedValue === "Core v2"
        && table.resolvedHeaderCount === 3
        && table.rowDividerColor === LV.Theme.panelBackground03
        && table.headerSeparatorColor === LV.Theme.panelBackground10
        && table.rowInputable(({ inputable: true })) === true
        && table.rowInputable(({ })) === false
        && tableEditedRow === 0
        && tableEditedColumn === 1
        && tableEditedValue === "Active v3"
        && tableSubmittedRow === 0
        && tableSubmittedColumn === 2
        && tableSubmittedValue === "Core v3"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("tableCellContractReady").toBool());
}

void ImportApiTests::list_item_and_footer_figma_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property bool listDefaultsCaptured: false
    property string listApplyResult: ""
    property int listEditedCount: 0
    property int listSubmittedCount: 0
    property string listEditedValue: ""
    property string listSubmittedValue: ""

    LV.ListItem {
        id: listItem
        label: "Label"
        visible: false
        onInputEdited: function(text) {
            root.listEditedCount += 1
            root.listEditedValue = text
        }
        onInputSubmitted: function(text) {
            root.listSubmittedCount += 1
            root.listSubmittedValue = text
        }
    }

    LV.ListFooter {
        id: listFooter
        visible: false
        button1: ({ "type": "icon", "iconName": "projectStructure", "tone": LV.AbstractButton.Borderless })
        button2: ({ "type": "IconMenuButton", "iconName": "projectStructure", "tone": LV.AbstractButton.Borderless })
        button3: ({ "type": "menu", "iconName": "viewMoreSymbolicDefault", "enabled": false })
    }

    Component.onCompleted: {
        listDefaultsCaptured = !listItem.inputable
        listItem.inputable = true
        listApplyResult = listItem.applyInputResult("Label 2")
        listItem.inputEdited("Label 3")
        listItem.inputSubmitted("Label 4")
    }

    property bool contractReady:
        listDefaultsCaptured
        && listApplyResult === "Label 2"
        && listItem.inputResult === "Label 2"
        && listItem.label === "Label 2"
        && listEditedCount === 1
        && listSubmittedCount === 1
        && listEditedValue === "Label 3"
        && listSubmittedValue === "Label 4"
        && listItem.horizontalPadding === LV.Theme.gap4
        && listItem.verticalPadding === LV.Theme.gap2
        && Math.abs(listItem.separatorHeight - 1) < 0.01
        && listItem.implicitWidth >= listItem.minItemWidth
        && listFooter.button1.iconName === "projectStructure"
        && listFooter.button2.type === "IconMenuButton"
        && listFooter.button3.enabled === false
        && listFooter.implicitHeight > 0
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("contractReady").toBool());
}

QTEST_MAIN(ImportApiTests)
#include "tst_import_api.moc"
