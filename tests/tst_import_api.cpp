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
    void appscaffold_platform_adaptive_layout_loads();
    void icon_name_mapping_loads();
    void hierarchy_tree_model_api_loads();
    void hierarchy_string_array_model_loads();
    void hierarchy_row_click_only_activates_not_toggles();
    void button_padding_matches_figma_spec();
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
    property bool labelStyleApiReady: contentLabel.style === contentLabel.body
        && contentLabel.font.pixelSize === LV.Theme.textBody
        && contentLabel.font.weight === LV.Theme.textBodyWeight
        && contentLabel.color === LV.Theme.bodyColor
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

void ImportApiTests::appscaffold_platform_adaptive_layout_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    width: 1400
    height: 900

    LV.AppScaffold {
        id: mobileScaffold
        width: 1200
        height: 760
        layoutMode: "auto"
        layoutPlatform: "android"
        navModel: ["Home", "Runs", "Settings"]
        navigationEnabled: true
        visible: false
    }

    LV.AppScaffold {
        id: desktopScaffold
        width: 1200
        height: 760
        layoutMode: "auto"
        layoutPlatform: "osx"
        navModel: ["Home", "Runs", "Settings"]
        navigationEnabled: true
        visible: false
    }

    property bool mobileContract:
        mobileScaffold.mobileLayout
        && !mobileScaffold.desktopLayout
        && mobileScaffold.bottomNavigationEnabled
        && !mobileScaffold.navigationRailEnabled
        && !mobileScaffold.drawerNavigationEnabled

    property bool desktopContract:
        desktopScaffold.desktopLayout
        && !desktopScaffold.mobileLayout
        && desktopScaffold.navigationRailEnabled
        && !desktopScaffold.bottomNavigationEnabled
        && !desktopScaffold.drawerNavigationEnabled
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("mobileContract").toBool());
    QVERIFY(root->property("desktopContract").toBool());
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
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("figmaPaddingReady").toBool());
}

QTEST_MAIN(ImportApiTests)
#include "tst_import_api.moc"
