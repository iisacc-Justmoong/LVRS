#include <QtTest>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QCoreApplication>
#include <QDir>
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
    void hierarchy_row_click_only_activates_not_toggles();
    void button_padding_matches_figma_spec();
    void checkbox_figma_contract_loads();
    void context_menu_item_action_contract_loads();
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
    property bool backendOptimizationDefaultsReady: autoAttachRuntimeEvents
        && autoAttachRuntimeEvents === globalEventListenersEnabled
        && !autoHookBackendUserEvents
        && globalEventListenersEnabled
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

    LV.Hierarchy {
        id: hierarchy
        objectName: "hierarchy"
        width: 280
        height: 300
        model: [
            {
                key: "root",
                itemId: 10,
                text: "Root",
                icon: "viewMoreSymbolicDefault",
                expanded: true,
                children: [
                    {
                        key: "child-a",
                        itemId: 11,
                        text: "Child A",
                        icon: "viewMoreSymbolicDefault",
                        expanded: false,
                        children: [
                            { key: "leaf-a1", itemId: 12, text: "Leaf A1", icon: "viewMoreSymbolicBorderless" }
                        ]
                    },
                    {
                        key: "child-b",
                        itemId: 20,
                        text: "Child B",
                        icon: "viewMoreSymbolicDisabled"
                    }
                ]
            }
        ]
    }

    Component.onCompleted: {
        Qt.callLater(function() {
            hierarchy.activateListItemByKey("leaf-a1")
        })
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
                label: "Root",
                iconName: "projectStructure",
                expanded: true,
                children: [
                    {
                        key: "child",
                        label: "Child",
                        iconName: "viewMoreSymbolicDefault",
                        children: [
                            {
                                key: "grand",
                                label: "Grand",
                                iconName: "viewMoreSymbolicBorderless",
                                children: [
                                    {
                                        key: "great",
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
            && childItem.computedLeftPadding === 21
            && grandItem.computedLeftPadding === 34
            && greatItem.computedLeftPadding === 47
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
                label: "Root",
                iconName: "projectStructure",
                expanded: true,
                children: [
                    { key: "child", label: "Child", iconName: "viewMoreSymbolicDefault" }
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

void ImportApiTests::hierarchy_row_click_only_activates_not_toggles()
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
        width: 260
        height: 220
        model: [
            {
                key: "root",
                label: "Root",
                expanded: true,
                selected: true,
                children: [
                    { key: "child", label: "Child" }
                ]
            }
        ]
    }

    property bool rowClickToggleBlocked: false

    Component.onCompleted: {
        Qt.callLater(function() {
            const row = hierarchy.activeListItem
            if (!row || !row.clicked) {
                rowClickToggleBlocked = false
                return
            }
            const wasExpanded = !!row.expanded
            row.clicked()
            rowClickToggleBlocked = (row.expanded === wasExpanded)
        })
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("rowClickToggleBlocked").toBool());
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
        && labelMenuButton.resolvedIndicatorName === "panDownSymbolicAccent"
        && labelMenuButtonDefault.resolvedIndicatorName === "panDownSymbolicDefault"
        && labelMenuButtonBorderless.resolvedIndicatorName === "panDownSymbolicBorderless"
        && labelMenuButtonDestructive.resolvedIndicatorName === "panDownSymbolicAccent"
        && labelMenuButtonDisabled.resolvedIndicatorName === "panDownSymbolicDisabled"
        && iconMenuButton.resolvedIndicatorName === "panDownSymbolicAccent"
        && iconMenuButtonDefault.resolvedIndicatorName === "panDownSymbolicDefault"
        && iconMenuButtonBorderless.resolvedIndicatorName === "panDownSymbolicBorderless"
        && iconMenuButtonDestructive.resolvedIndicatorName === "panDownSymbolicAccent"
        && iconMenuButtonDisabled.resolvedIndicatorName === "panDownSymbolicDisabled"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("figmaPaddingReady").toBool());
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

void ImportApiTests::list_item_and_footer_figma_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.ListItem {
        id: listItem
        label: "Label"
        visible: false
    }

    LV.ListFooter {
        id: listFooter
        visible: false
        button1: ({ "type": "icon", "iconName": "projectStructure", "tone": LV.AbstractButton.Borderless })
        button2: ({ "type": "IconMenuButton", "iconName": "projectStructure", "tone": LV.AbstractButton.Borderless })
        button3: ({ "type": "menu", "iconName": "viewMoreSymbolicDefault", "enabled": false })
    }

    property bool contractReady:
        listItem.label === "Label"
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
