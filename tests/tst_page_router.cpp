#include <QtTest>

#include <QScopedPointer>
#include <QQmlEngine>
#include <QtPlugin>

#include "test_utils.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class PageRouterTests : public QObject
{
    Q_OBJECT

private slots:
    void route_params_are_passed_to_target_component();
    void component_navigation_keeps_path_stack_in_sync();
    void route_pages_are_isolated_and_forced_to_viewport();
    void page_router_retain_inactive_depth_contract();
    void page_router_updates_view_state_tracker_from_stack();
    void global_navigator_allows_one_line_navigation();
    void global_navigator_falls_back_to_previous_router();
    void interactive_navigation_transitions_hold_route_commit_until_finish();
    void page_transition_controller_proxies_application_window_router();
    void route_mvvm_binding_and_write_ownership_are_applied();
    void route_mvvm_ownership_is_released_when_view_is_popped();
};

void PageRouterTests::route_params_are_passed_to_target_component()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 320
    height: 240

    property string capturedRunId: ""
    property string capturedMode: ""
    property int depth: router.depth
    property int pathLength: router.path.length
    property string currentPath: router.currentPath
    property var currentParams: router.currentParams

    Component {
        id: homePage
        Item { }
    }

    Component {
        id: runPage
        Item {
            property string runId: ""
            property string mode: ""
            Component.onCompleted: {
                root.capturedRunId = runId
                root.capturedMode = mode
            }
        }
    }

    LV.PageRouter {
        id: router
        anchors.fill: parent
        initialPath: "/"
        routes: [
            { path: "/", component: homePage },
            { path: "/runs/[runId]", component: runPage }
        ]
    }

    function openRun() {
        router.go("/runs/42", { mode: "push" })
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_COMPARE(root->property("depth").toInt(), 1);
    QCOMPARE(root->property("pathLength").toInt(), 1);
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    QVERIFY(QMetaObject::invokeMethod(root.data(), "openRun"));
    QTRY_COMPARE(root->property("capturedRunId").toString(), QStringLiteral("42"));
    QTRY_COMPARE(root->property("capturedMode").toString(), QStringLiteral("push"));
    QCOMPARE(root->property("depth").toInt(), 2);
    QCOMPARE(root->property("pathLength").toInt(), 2);
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/runs/42"));

    const QVariantMap currentParams = root->property("currentParams").toMap();
    QCOMPARE(currentParams.value(QStringLiteral("runId")).toString(), QStringLiteral("42"));
    QCOMPARE(currentParams.value(QStringLiteral("mode")).toString(), QStringLiteral("push"));
}

void PageRouterTests::component_navigation_keeps_path_stack_in_sync()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 320
    height: 240

    property string componentTag: ""
    property int depth: router.depth
    property int pathLength: router.path.length
    property string currentPath: router.currentPath

    Component {
        id: homePage
        Item { }
    }

    Component {
        id: componentPage
        Item {
            property string tag: ""
            Component.onCompleted: root.componentTag = tag
        }
    }

    LV.PageRouter {
        id: router
        anchors.fill: parent
        initialPath: "/"
        routes: [{ path: "/", component: homePage }]
    }

    function pushComponent() {
        router.goTo(componentPage, { tag: "push" })
    }

    function replaceComponent() {
        router.replaceWith(componentPage, { tag: "replace" })
    }

    function setComponentRoot() {
        router.setRootComponent(componentPage, { tag: "root" })
    }

    function clearExternalPath() {
        router.path = []
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_COMPARE(root->property("depth").toInt(), 1);
    QCOMPARE(root->property("pathLength").toInt(), 1);
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    QVERIFY(QMetaObject::invokeMethod(root.data(), "pushComponent"));
    QTRY_COMPARE(root->property("componentTag").toString(), QStringLiteral("push"));
    QCOMPARE(root->property("depth").toInt(), 2);
    QCOMPARE(root->property("pathLength").toInt(), 2);
    QCOMPARE(root->property("currentPath").toString(), QString());

    QVERIFY(QMetaObject::invokeMethod(root.data(), "replaceComponent"));
    QTRY_COMPARE(root->property("componentTag").toString(), QStringLiteral("replace"));
    QCOMPARE(root->property("depth").toInt(), 2);
    QCOMPARE(root->property("pathLength").toInt(), 2);
    QCOMPARE(root->property("currentPath").toString(), QString());

    QVERIFY(QMetaObject::invokeMethod(root.data(), "setComponentRoot"));
    QTRY_COMPARE(root->property("componentTag").toString(), QStringLiteral("root"));
    QCOMPARE(root->property("depth").toInt(), 1);
    QCOMPARE(root->property("pathLength").toInt(), 1);
    QCOMPARE(root->property("currentPath").toString(), QString());

    QVERIFY(QMetaObject::invokeMethod(root.data(), "clearExternalPath"));
    QTRY_COMPARE(root->property("depth").toInt(), 0);
    QCOMPARE(root->property("pathLength").toInt(), 0);
    QCOMPARE(root->property("currentPath").toString(), QString());
}

void PageRouterTests::route_pages_are_isolated_and_forced_to_viewport()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 360
    height: 240

    property var firstPageItem: null
    property var secondPageItem: null
    property int depth: router.depth
    property bool firstVisibleAtRoot: firstPageItem ? firstPageItem.visible : false
    property bool firstHiddenAfterPush: firstPageItem ? !firstPageItem.visible : false
    property bool secondVisibleAfterPush: secondPageItem ? secondPageItem.visible : false
    property bool secondFillsViewport: router.currentPageItem
        ? Math.abs(router.currentPageItem.width - router.width) < 0.5
          && Math.abs(router.currentPageItem.height - router.height) < 0.5
        : false
    property bool secondAnchoredToOrigin: router.currentPageItem
        ? Math.abs(router.currentPageItem.x) < 0.5 && Math.abs(router.currentPageItem.y) < 0.5
        : false

    Component {
        id: foldersPage
        Item {
            width: 180
            height: 180
            Component.onCompleted: root.firstPageItem = this
        }
    }

    Component {
        id: notesPage
        Item {
            width: 200
            height: 160
            Component.onCompleted: root.secondPageItem = this
        }
    }

    LV.PageRouter {
        id: router
        anchors.fill: parent
        initialPath: "/folders"
        routes: [
            { path: "/folders", component: foldersPage },
            { path: "/notes", component: notesPage }
        ]
    }

    function openNotes() {
        router.go("/notes")
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_COMPARE(root->property("depth").toInt(), 1);
    QTRY_VERIFY(root->property("firstVisibleAtRoot").toBool());

    QVERIFY(QMetaObject::invokeMethod(root.data(), "openNotes"));
    QTRY_COMPARE(root->property("depth").toInt(), 2);
    QTRY_VERIFY(root->property("firstHiddenAfterPush").toBool());
    QTRY_VERIFY(root->property("secondVisibleAfterPush").toBool());
    QTRY_VERIFY(root->property("secondFillsViewport").toBool());
    QTRY_VERIFY(root->property("secondAnchoredToOrigin").toBool());
}

void PageRouterTests::page_router_retain_inactive_depth_contract()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 360
    height: 240

    property var pageAItem: null
    property var pageBItem: null
    property var pageCItem: null
    property int depth: router.depth
    property bool pageAHiddenOnDepth3: pageAItem ? !pageAItem.visible : false
    property bool pageBVisibleOnDepth3: pageBItem ? pageBItem.visible : false
    property bool pageCVisibleOnDepth3: pageCItem ? pageCItem.visible : false

    Component {
        id: pageA
        Item { Component.onCompleted: root.pageAItem = this }
    }

    Component {
        id: pageB
        Item { Component.onCompleted: root.pageBItem = this }
    }

    Component {
        id: pageC
        Item { Component.onCompleted: root.pageCItem = this }
    }

    LV.PageRouter {
        id: router
        anchors.fill: parent
        initialPath: "/a"
        retainInactivePageCount: 1
        routeResolveCacheCapacity: 64
        routes: [
            { path: "/a", component: pageA },
            { path: "/b", component: pageB },
            { path: "/c", component: pageC }
        ]
    }

    function openB() {
        router.go("/b")
    }

    function openC() {
        router.go("/c")
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_COMPARE(root->property("depth").toInt(), 1);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "openB"));
    QVERIFY(QMetaObject::invokeMethod(root.data(), "openC"));
    QTRY_COMPARE(root->property("depth").toInt(), 3);
    QTRY_VERIFY(root->property("pageAHiddenOnDepth3").toBool());
    QTRY_VERIFY(root->property("pageBVisibleOnDepth3").toBool());
    QTRY_VERIFY(root->property("pageCVisibleOnDepth3").toBool());
}

void PageRouterTests::page_router_updates_view_state_tracker_from_stack()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 320
    height: 240

    property string activeView: LV.ViewStateTracker.currentActiveView
    property string rootState: {
        LV.ViewStateTracker.stack
        return LV.ViewStateTracker.stateOf("/")
    }
    property string runState: {
        LV.ViewStateTracker.stack
        return LV.ViewStateTracker.stateOf("/runs/42")
    }
    property int loadedCount: LV.ViewStateTracker.loadedCount

    Component {
        id: homePage
        Item { }
    }

    Component {
        id: runPage
        Item { property string runId: "" }
    }

    LV.PageRouter {
        id: router
        anchors.fill: parent
        initialPath: "/"
        routes: [
            { path: "/", component: homePage },
            { path: "/runs/[runId]", component: runPage }
        ]
    }

    function resetTracker() {
        LV.ViewStateTracker.clear()
        router.setRoot("/")
    }

    function openRun() {
        router.go("/runs/42")
    }

    function goBack() {
        router.back()
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "resetTracker"));

    QTRY_COMPARE(root->property("activeView").toString(), QStringLiteral("/"));
    QTRY_COMPARE(root->property("rootState").toString(), QStringLiteral("Active"));
    QTRY_COMPARE(root->property("loadedCount").toInt(), 1);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "openRun"));
    QTRY_COMPARE(root->property("activeView").toString(), QStringLiteral("/runs/42"));
    QTRY_COMPARE(root->property("rootState").toString(), QStringLiteral("Inactive"));
    QTRY_COMPARE(root->property("runState").toString(), QStringLiteral("Active"));
    QTRY_COMPARE(root->property("loadedCount").toInt(), 2);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "goBack"));
    QTRY_COMPARE(root->property("activeView").toString(), QStringLiteral("/"));
    QTRY_COMPARE(root->property("rootState").toString(), QStringLiteral("Active"));
    QTRY_COMPARE(root->property("runState").toString(), QString());
    QTRY_COMPARE(root->property("loadedCount").toInt(), 1);
}

void PageRouterTests::global_navigator_allows_one_line_navigation()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 320
    height: 240

    property string currentPath: router.currentPath
    property int depth: router.depth

    Component { id: homePage; Item { } }
    Component { id: reportsPage; Item { } }
    Component { id: settingsPage; Item { } }

    LV.PageRouter {
        id: router
        anchors.fill: parent
        initialPath: "/"
        routes: [
            { path: "/", component: homePage },
            { path: "/reports", component: reportsPage },
            { path: "/settings", component: settingsPage }
        ]
    }

    LV.Link {
        id: globalLink
        visible: false
        href: "/settings"
    }

    function goReportsInOneLine() {
        LV.Navigator.go("/reports")
    }

    function goSettingsByLink() {
        globalLink.clicked()
    }

    function goBackInOneLine() {
        LV.Navigator.back()
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));
    QCOMPARE(root->property("depth").toInt(), 1);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "goReportsInOneLine"));
    QTRY_COMPARE(root->property("currentPath").toString(), QStringLiteral("/reports"));
    QCOMPARE(root->property("depth").toInt(), 2);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "goSettingsByLink"));
    QTRY_COMPARE(root->property("currentPath").toString(), QStringLiteral("/settings"));
    QCOMPARE(root->property("depth").toInt(), 3);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "goBackInOneLine"));
    QTRY_COMPARE(root->property("currentPath").toString(), QStringLiteral("/reports"));
    QCOMPARE(root->property("depth").toInt(), 2);
}

void PageRouterTests::global_navigator_falls_back_to_previous_router()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 320
    height: 240

    property string firstCurrentPath: firstRouter.currentPath
    property string secondCurrentPath: secondRouter.currentPath

    Component { id: firstRootPage; Item { } }
    Component { id: firstPrimaryPage; Item { } }
    Component { id: secondRootPage; Item { } }
    Component { id: secondSecondaryPage; Item { } }

    LV.PageRouter {
        id: firstRouter
        anchors.fill: parent
        initialPath: "/"
        routes: [
            { path: "/", component: firstRootPage },
            { path: "/primary", component: firstPrimaryPage }
        ]
    }

    LV.PageRouter {
        id: secondRouter
        anchors.fill: parent
        initialPath: "/"
        routes: [
            { path: "/", component: secondRootPage },
            { path: "/secondary", component: secondSecondaryPage }
        ]
    }

    function goSecondary() {
        LV.Navigator.go("/secondary")
    }

    function fallbackToFirstAndGoPrimary() {
        LV.Navigator.unregisterRouter(secondRouter)
        LV.Navigator.go("/primary")
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_COMPARE(root->property("firstCurrentPath").toString(), QStringLiteral("/"));
    QTRY_COMPARE(root->property("secondCurrentPath").toString(), QStringLiteral("/"));

    QVERIFY(QMetaObject::invokeMethod(root.data(), "goSecondary"));
    QTRY_COMPARE(root->property("secondCurrentPath").toString(), QStringLiteral("/secondary"));
    QCOMPARE(root->property("firstCurrentPath").toString(), QStringLiteral("/"));

    QVERIFY(QMetaObject::invokeMethod(root.data(), "fallbackToFirstAndGoPrimary"));
    QTRY_COMPARE(root->property("firstCurrentPath").toString(), QStringLiteral("/primary"));
}

void PageRouterTests::interactive_navigation_transitions_hold_route_commit_until_finish()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 360
    height: 240

    property int depth: router.depth
    property string currentPath: router.currentPath
    property var currentParams: router.currentParams
    property bool interactiveActive: router.interactiveTransitionActive
    property real interactiveProgress: router.interactiveTransitionProgress
    property string interactiveToPath: router.interactiveTransitionToPath
    property real currentPageX: router.currentPageItem ? router.currentPageItem.x : 0
    property bool currentPageEnabled: router.currentPageItem ? router.currentPageItem.enabled : false
    property bool currentStackBusy: router.currentPageItem
        && router.currentPageItem.StackView.view
        ? router.currentPageItem.StackView.view.busy
        : false
    property bool previewVisible: router.interactiveTransitionPreviewItem
        ? router.interactiveTransitionPreviewItem.visible
        : false
    property var homePageItem: null
    property real homePageX: homePageItem ? homePageItem.x : 0
    property bool homeVisible: homePageItem ? homePageItem.visible : false
    property bool homeEnabled: homePageItem ? homePageItem.enabled : false

    Component {
        id: homePage
        Item {
            Component.onCompleted: root.homePageItem = this
        }
    }

    Component {
        id: detailPage
        Item {
            property string token: ""
        }
    }

    LV.PageRouter {
        id: router
        anchors.fill: parent
        initialPath: "/"
        interactiveTransitionSettleDuration: 0
        routes: [
            { path: "/", component: homePage },
            { path: "/detail", component: detailPage }
        ]
    }

    function openDetail() {
        router.go("/detail", { token: "committed" })
    }

    function beginBack() {
        return router.beginInteractiveBack({ source: "test" })
    }

    function updateTransition(progress, velocityX) {
        return router.updateInteractiveTransition(progress, { velocityX: velocityX })
    }

    function finishTransition(commit) {
        return router.finishInteractiveTransition(commit)
    }

    function cancelTransition() {
        return router.cancelInteractiveTransition()
    }

    function beginForward() {
        return router.beginInteractivePush("/detail", { token: "preview" }, { source: "test" })
    }

    function forcePresentationSync() {
        router.scheduleActivePagePresentationSync()
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_COMPARE(root->property("depth").toInt(), 1);
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    QVERIFY(QMetaObject::invokeMethod(root.data(), "openDetail"));
    QTRY_COMPARE(root->property("depth").toInt(), 2);
    QTRY_COMPARE(root->property("currentPath").toString(), QStringLiteral("/detail"));

    QVERIFY(QMetaObject::invokeMethod(root.data(), "beginBack"));
    QTRY_VERIFY(root->property("interactiveActive").toBool());
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.5)),
                                      Q_ARG(QVariant, QVariant(0.0))));
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/detail"));
    QCOMPARE(root->property("depth").toInt(), 2);
    QVERIFY(root->property("homeVisible").toBool());
    QVERIFY(root->property("currentPageX").toReal() > 100.0);
    QVERIFY(root->property("homePageX").toReal() < -40.0);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "forcePresentationSync"));
    QTRY_VERIFY(root->property("homeVisible").toBool());
    QTRY_VERIFY(!root->property("homeEnabled").toBool());
    QTRY_VERIFY(!root->property("currentPageEnabled").toBool());

    QVERIFY(QMetaObject::invokeMethod(root.data(), "cancelTransition"));
    QTRY_VERIFY(!root->property("interactiveActive").toBool());
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/detail"));
    QCOMPARE(root->property("depth").toInt(), 2);
    QVERIFY(qAbs(root->property("currentPageX").toReal()) < 0.5);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "beginBack"));
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.65)),
                                      Q_ARG(QVariant, QVariant(0.0))));
    QVERIFY(QMetaObject::invokeMethod(root.data(), "finishTransition", Q_ARG(QVariant, QVariant(true))));
    QTRY_VERIFY(!root->property("interactiveActive").toBool());
    QTRY_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));
    QCOMPARE(root->property("depth").toInt(), 1);
    QVERIFY(!root->property("currentStackBusy").toBool());
    QVERIFY(qAbs(root->property("currentPageX").toReal()) < 0.5);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "beginForward"));
    QTRY_VERIFY(root->property("interactiveActive").toBool());
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/"));
    QCOMPARE(root->property("depth").toInt(), 1);
    QCOMPARE(root->property("interactiveToPath").toString(), QStringLiteral("/detail"));
    QVERIFY(root->property("previewVisible").toBool());

    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.4)),
                                      Q_ARG(QVariant, QVariant(-200.0))));
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/"));
    QCOMPARE(root->property("depth").toInt(), 1);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "cancelTransition"));
    QTRY_VERIFY(!root->property("interactiveActive").toBool());
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/"));
    QCOMPARE(root->property("depth").toInt(), 1);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "beginForward"));
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.7)),
                                      Q_ARG(QVariant, QVariant(-200.0))));
    QVERIFY(QMetaObject::invokeMethod(root.data(), "finishTransition", Q_ARG(QVariant, QVariant(true))));
    QTRY_VERIFY(!root->property("interactiveActive").toBool());
    QTRY_COMPARE(root->property("currentPath").toString(), QStringLiteral("/detail"));
    QCOMPARE(root->property("depth").toInt(), 2);
    const QVariantMap currentParams = root->property("currentParams").toMap();
    QCOMPARE(currentParams.value(QStringLiteral("token")).toString(), QStringLiteral("preview"));
}

void PageRouterTests::page_transition_controller_proxies_application_window_router()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 360
    height: 240
    visible: true
    initialRoutePath: "/"

    property bool controllerAvailable: pageTransitionController !== null
    property bool transitionActive: pageTransitionController ? pageTransitionController.active : false
    property bool transitionCanCommit: pageTransitionController ? pageTransitionController.canCommit : false
    property string currentPath: activePageRouter ? activePageRouter.currentPath : ""
    property string pendingPath: pageTransitionController ? pageTransitionController.toPath : ""

    pageRoutes: [
        { path: "/", component: homePage },
        { path: "/detail", component: detailPage }
    ]

    Component { id: homePage; Item { } }
    Component {
        id: detailPage
        Item {
            property string token: ""
        }
    }

    Component.onCompleted: {
        if (activePageRouter)
            activePageRouter.interactiveTransitionSettleDuration = 0
    }

    function beginForward() {
        return pageTransitionController.beginPush("/detail", { token: "controller" }, { source: "controller" })
    }

    function updateTransition(progress, velocityX) {
        return pageTransitionController.update(progress, { velocityX: velocityX })
    }

    function finishAuto() {
        return pageTransitionController.finish()
    }

    function cancelTransition() {
        return pageTransitionController.cancel()
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("controllerAvailable").toBool());
    QTRY_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    QVERIFY(QMetaObject::invokeMethod(root.data(), "beginForward"));
    QTRY_VERIFY(root->property("transitionActive").toBool());
    QCOMPARE(root->property("pendingPath").toString(), QStringLiteral("/detail"));
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.15)),
                                      Q_ARG(QVariant, QVariant(-1400.0))));
    QTRY_VERIFY(root->property("transitionCanCommit").toBool());
    QVERIFY(QMetaObject::invokeMethod(root.data(), "cancelTransition"));
    QTRY_VERIFY(!root->property("transitionActive").toBool());
    QCOMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    QVERIFY(QMetaObject::invokeMethod(root.data(), "beginForward"));
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.15)),
                                      Q_ARG(QVariant, QVariant(-1400.0))));
    QVERIFY(QMetaObject::invokeMethod(root.data(), "finishAuto"));
    QTRY_VERIFY(!root->property("transitionActive").toBool());
    QTRY_COMPARE(root->property("currentPath").toString(), QStringLiteral("/detail"));
}

void PageRouterTests::route_mvvm_binding_and_write_ownership_are_applied()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 320
    height: 240

    property string ownerOverview: {
        LV.ViewModels.owners
        return LV.ViewModels.ownerOf("OverviewVM")
    }
    property string ownerReports: {
        LV.ViewModels.owners
        return LV.ViewModels.ownerOf("ReportsVM")
    }
    property bool canWriteOverview: {
        LV.ViewModels.bindings
        LV.ViewModels.owners
        return LV.ViewModels.canWrite("/overview")
    }
    property bool canWriteReports: {
        LV.ViewModels.bindings
        LV.ViewModels.owners
        return LV.ViewModels.canWrite("/reports")
    }
    property string overviewStatus: overviewVm.status

    QtObject {
        id: overviewVm
        property string status: "Idle"
    }

    QtObject {
        id: reportsVm
        property string status: "Ready"
    }

    Component {
        id: overviewPage
        Item { }
    }

    Component {
        id: reportsPage
        Item { }
    }

    LV.PageRouter {
        id: router
        anchors.fill: parent
        initialPath: "/overview"
        routes: [
            { path: "/overview", component: overviewPage, viewModelKey: "OverviewVM", writable: true },
            { path: "/reports", component: reportsPage, viewModelKey: "ReportsVM" }
        ]
    }

    function prepare() {
        LV.ViewModels.clear()
        LV.ViewModels.set("OverviewVM", overviewVm)
        LV.ViewModels.set("ReportsVM", reportsVm)
        router.setRoot("/overview")
    }

    function goReports() {
        router.go("/reports")
    }

    function writeOverviewStatus(nextStatus) {
        LV.ViewModels.updateProperty("/overview", "status", nextStatus)
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "prepare"));

    QTRY_COMPARE(root->property("ownerOverview").toString(), QStringLiteral("/overview"));
    QCOMPARE(root->property("ownerReports").toString(), QString());
    QCOMPARE(root->property("canWriteOverview").toBool(), true);
    QCOMPARE(root->property("canWriteReports").toBool(), false);

    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "writeOverviewStatus",
                                      Q_ARG(QVariant, QVariant(QStringLiteral("Working")))));
    QTRY_COMPARE(root->property("overviewStatus").toString(), QStringLiteral("Working"));

    QVERIFY(QMetaObject::invokeMethod(root.data(), "goReports"));
    QTRY_COMPARE(root->property("canWriteReports").toBool(), false);
}

void PageRouterTests::route_mvvm_ownership_is_released_when_view_is_popped()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 320
    height: 240

    property string ownerOverview: {
        LV.ViewModels.owners
        return LV.ViewModels.ownerOf("OverviewVM")
    }
    property string ownerReports: {
        LV.ViewModels.owners
        return LV.ViewModels.ownerOf("ReportsVM")
    }
    property bool canWriteReports: {
        LV.ViewModels.bindings
        LV.ViewModels.owners
        return LV.ViewModels.canWrite("/reports")
    }

    QtObject { id: overviewVm; property string status: "Idle" }
    QtObject { id: reportsVm; property string status: "Ready" }

    Component { id: overviewPage; Item { } }
    Component { id: reportsPage; Item { } }

    LV.PageRouter {
        id: router
        anchors.fill: parent
        initialPath: "/overview"
        routes: [
            { path: "/overview", component: overviewPage, viewModelKey: "OverviewVM", writable: true },
            { path: "/reports", component: reportsPage, viewModelKey: "ReportsVM", writable: true }
        ]
    }

    function prepare() {
        LV.ViewModels.clear()
        LV.ViewModels.set("OverviewVM", overviewVm)
        LV.ViewModels.set("ReportsVM", reportsVm)
        router.setRoot("/overview")
    }

    function goReports() {
        router.go("/reports")
    }

    function goBack() {
        router.back()
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "prepare"));

    QTRY_COMPARE(root->property("ownerOverview").toString(), QStringLiteral("/overview"));
    QCOMPARE(root->property("ownerReports").toString(), QString());

    QVERIFY(QMetaObject::invokeMethod(root.data(), "goReports"));
    QTRY_COMPARE(root->property("ownerReports").toString(), QStringLiteral("/reports"));
    QCOMPARE(root->property("canWriteReports").toBool(), true);

    QVERIFY(QMetaObject::invokeMethod(root.data(), "goBack"));
    QTRY_COMPARE(root->property("ownerReports").toString(), QString());
    QCOMPARE(root->property("canWriteReports").toBool(), false);
}

QTEST_MAIN(PageRouterTests)
#include "tst_page_router.moc"
