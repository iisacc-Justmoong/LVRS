#include "QtEventWait.h"
#include <QGuiApplication>

#include <QScopedPointer>
#include <QQmlEngine>
#include <QtPlugin>

#include "test_utils.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class PageRouterTests : public QObject
{
public:
    void route_params_are_passed_to_target_component();
    void default_initial_path_does_not_override_bound_path();
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
    CTEST_CHECK(root);
    CTEST_WAIT_COMPARE(root->property("depth").toInt(), 1);
    CTEST_COMPARE(root->property("pathLength").toInt(), 1);
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "openRun"));
    CTEST_WAIT_COMPARE(root->property("capturedRunId").toString(), QStringLiteral("42"));
    CTEST_WAIT_COMPARE(root->property("capturedMode").toString(), QStringLiteral("push"));
    CTEST_COMPARE(root->property("depth").toInt(), 2);
    CTEST_COMPARE(root->property("pathLength").toInt(), 2);
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/runs/42"));

    const QVariantMap currentParams = root->property("currentParams").toMap();
    CTEST_COMPARE(currentParams.value(QStringLiteral("runId")).toString(), QStringLiteral("42"));
    CTEST_COMPARE(currentParams.value(QStringLiteral("mode")).toString(), QStringLiteral("push"));
}

void PageRouterTests::default_initial_path_does_not_override_bound_path()
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

    property bool homeLoaded: false
    property bool reportsLoaded: false
    property int depth: router.depth
    property int pathLength: router.path.length
    property string currentPath: router.currentPath
    property string firstPath: router.path.length > 0 ? router.path[0] : ""

    Component {
        id: homePage
        Item {
            Component.onCompleted: root.homeLoaded = true
        }
    }

    Component {
        id: reportsPage
        Item {
            Component.onCompleted: root.reportsLoaded = true
        }
    }

    LV.PageRouter {
        id: router
        anchors.fill: parent
        path: ["/reports"]
        routes: [
            { path: "/", component: homePage },
            { path: "/reports", component: reportsPage }
        ]
    }
}
)";

    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_COMPARE(root->property("currentPath").toString(), QStringLiteral("/reports"));
    CTEST_COMPARE(root->property("depth").toInt(), 1);
    CTEST_COMPARE(root->property("pathLength").toInt(), 1);
    CTEST_COMPARE(root->property("firstPath").toString(), QStringLiteral("/reports"));
    CTEST_COMPARE(root->property("reportsLoaded").toBool(), true);
    CTEST_COMPARE(root->property("homeLoaded").toBool(), false);
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
    CTEST_CHECK(root);
    CTEST_WAIT_COMPARE(root->property("depth").toInt(), 1);
    CTEST_COMPARE(root->property("pathLength").toInt(), 1);
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "pushComponent"));
    CTEST_WAIT_COMPARE(root->property("componentTag").toString(), QStringLiteral("push"));
    CTEST_COMPARE(root->property("depth").toInt(), 2);
    CTEST_COMPARE(root->property("pathLength").toInt(), 2);
    CTEST_COMPARE(root->property("currentPath").toString(), QString());

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "replaceComponent"));
    CTEST_WAIT_COMPARE(root->property("componentTag").toString(), QStringLiteral("replace"));
    CTEST_COMPARE(root->property("depth").toInt(), 2);
    CTEST_COMPARE(root->property("pathLength").toInt(), 2);
    CTEST_COMPARE(root->property("currentPath").toString(), QString());

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "setComponentRoot"));
    CTEST_WAIT_COMPARE(root->property("componentTag").toString(), QStringLiteral("root"));
    CTEST_COMPARE(root->property("depth").toInt(), 1);
    CTEST_COMPARE(root->property("pathLength").toInt(), 1);
    CTEST_COMPARE(root->property("currentPath").toString(), QString());

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "clearExternalPath"));
    CTEST_WAIT_COMPARE(root->property("depth").toInt(), 0);
    CTEST_COMPARE(root->property("pathLength").toInt(), 0);
    CTEST_COMPARE(root->property("currentPath").toString(), QString());
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
    CTEST_CHECK(root);
    CTEST_WAIT_COMPARE(root->property("depth").toInt(), 1);
    CTEST_WAIT_DEFAULT(root->property("firstVisibleAtRoot").toBool());

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "openNotes"));
    CTEST_WAIT_COMPARE(root->property("depth").toInt(), 2);
    CTEST_WAIT_DEFAULT(root->property("firstHiddenAfterPush").toBool());
    CTEST_WAIT_DEFAULT(root->property("secondVisibleAfterPush").toBool());
    CTEST_WAIT_DEFAULT(root->property("secondFillsViewport").toBool());
    CTEST_WAIT_DEFAULT(root->property("secondAnchoredToOrigin").toBool());
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
    CTEST_CHECK(root);
    CTEST_WAIT_COMPARE(root->property("depth").toInt(), 1);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "openB"));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "openC"));
    CTEST_WAIT_COMPARE(root->property("depth").toInt(), 3);
    CTEST_WAIT_DEFAULT(root->property("pageAHiddenOnDepth3").toBool());
    CTEST_WAIT_DEFAULT(root->property("pageBVisibleOnDepth3").toBool());
    CTEST_WAIT_DEFAULT(root->property("pageCVisibleOnDepth3").toBool());
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
    CTEST_CHECK(root);
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "resetTracker"));

    CTEST_WAIT_COMPARE(root->property("activeView").toString(), QStringLiteral("/"));
    CTEST_WAIT_COMPARE(root->property("rootState").toString(), QStringLiteral("Active"));
    CTEST_WAIT_COMPARE(root->property("loadedCount").toInt(), 1);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "openRun"));
    CTEST_WAIT_COMPARE(root->property("activeView").toString(), QStringLiteral("/runs/42"));
    CTEST_WAIT_COMPARE(root->property("rootState").toString(), QStringLiteral("Inactive"));
    CTEST_WAIT_COMPARE(root->property("runState").toString(), QStringLiteral("Active"));
    CTEST_WAIT_COMPARE(root->property("loadedCount").toInt(), 2);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "goBack"));
    CTEST_WAIT_COMPARE(root->property("activeView").toString(), QStringLiteral("/"));
    CTEST_WAIT_COMPARE(root->property("rootState").toString(), QStringLiteral("Active"));
    CTEST_WAIT_COMPARE(root->property("runState").toString(), QString());
    CTEST_WAIT_COMPARE(root->property("loadedCount").toInt(), 1);
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
    CTEST_CHECK(root);
    CTEST_WAIT_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));
    CTEST_COMPARE(root->property("depth").toInt(), 1);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "goReportsInOneLine"));
    CTEST_WAIT_COMPARE(root->property("currentPath").toString(), QStringLiteral("/reports"));
    CTEST_COMPARE(root->property("depth").toInt(), 2);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "goSettingsByLink"));
    CTEST_WAIT_COMPARE(root->property("currentPath").toString(), QStringLiteral("/settings"));
    CTEST_COMPARE(root->property("depth").toInt(), 3);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "goBackInOneLine"));
    CTEST_WAIT_COMPARE(root->property("currentPath").toString(), QStringLiteral("/reports"));
    CTEST_COMPARE(root->property("depth").toInt(), 2);
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
    CTEST_CHECK(root);
    CTEST_WAIT_COMPARE(root->property("firstCurrentPath").toString(), QStringLiteral("/"));
    CTEST_WAIT_COMPARE(root->property("secondCurrentPath").toString(), QStringLiteral("/"));

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "goSecondary"));
    CTEST_WAIT_COMPARE(root->property("secondCurrentPath").toString(), QStringLiteral("/secondary"));
    CTEST_COMPARE(root->property("firstCurrentPath").toString(), QStringLiteral("/"));

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "fallbackToFirstAndGoPrimary"));
    CTEST_WAIT_COMPARE(root->property("firstCurrentPath").toString(), QStringLiteral("/primary"));
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
    property int settleDuration: router.interactiveTransitionSettleDuration
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
    CTEST_CHECK(root);
    CTEST_COMPARE(root->property("settleDuration").toInt(), 360);
    CTEST_WAIT_COMPARE(root->property("depth").toInt(), 1);
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "openDetail"));
    CTEST_WAIT_COMPARE(root->property("depth").toInt(), 2);
    CTEST_WAIT_COMPARE(root->property("currentPath").toString(), QStringLiteral("/detail"));

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "beginBack"));
    CTEST_WAIT_DEFAULT(root->property("interactiveActive").toBool());
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.5)),
                                      Q_ARG(QVariant, QVariant(0.0))));
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/detail"));
    CTEST_COMPARE(root->property("depth").toInt(), 2);
    CTEST_CHECK(root->property("homeVisible").toBool());
    CTEST_CHECK(root->property("currentPageX").toReal() > 100.0);
    CTEST_CHECK(root->property("homePageX").toReal() < -40.0);
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "forcePresentationSync"));
    CTEST_WAIT_DEFAULT(root->property("homeVisible").toBool());
    CTEST_WAIT_DEFAULT(!root->property("homeEnabled").toBool());
    CTEST_WAIT_DEFAULT(!root->property("currentPageEnabled").toBool());

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "cancelTransition"));
    CTEST_WAIT_DEFAULT(!root->property("interactiveActive").toBool());
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/detail"));
    CTEST_COMPARE(root->property("depth").toInt(), 2);
    CTEST_CHECK(qAbs(root->property("currentPageX").toReal()) < 0.5);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "beginBack"));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.65)),
                                      Q_ARG(QVariant, QVariant(0.0))));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "finishTransition", Q_ARG(QVariant, QVariant(true))));
    CTEST_WAIT_DEFAULT(!root->property("interactiveActive").toBool());
    CTEST_WAIT_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));
    CTEST_COMPARE(root->property("depth").toInt(), 1);
    CTEST_CHECK(!root->property("currentStackBusy").toBool());
    CTEST_CHECK(qAbs(root->property("currentPageX").toReal()) < 0.5);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "beginForward"));
    CTEST_WAIT_DEFAULT(root->property("interactiveActive").toBool());
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));
    CTEST_COMPARE(root->property("depth").toInt(), 1);
    CTEST_COMPARE(root->property("interactiveToPath").toString(), QStringLiteral("/detail"));
    CTEST_CHECK(root->property("previewVisible").toBool());

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.4)),
                                      Q_ARG(QVariant, QVariant(-200.0))));
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));
    CTEST_COMPARE(root->property("depth").toInt(), 1);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "cancelTransition"));
    CTEST_WAIT_DEFAULT(!root->property("interactiveActive").toBool());
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));
    CTEST_COMPARE(root->property("depth").toInt(), 1);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "beginForward"));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.7)),
                                      Q_ARG(QVariant, QVariant(-200.0))));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "finishTransition", Q_ARG(QVariant, QVariant(true))));
    CTEST_WAIT_DEFAULT(!root->property("interactiveActive").toBool());
    CTEST_WAIT_COMPARE(root->property("currentPath").toString(), QStringLiteral("/detail"));
    CTEST_COMPARE(root->property("depth").toInt(), 2);
    const QVariantMap currentParams = root->property("currentParams").toMap();
    CTEST_COMPARE(currentParams.value(QStringLiteral("token")).toString(), QStringLiteral("preview"));
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("controllerAvailable").toBool());
    CTEST_WAIT_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "beginForward"));
    CTEST_WAIT_DEFAULT(root->property("transitionActive").toBool());
    CTEST_COMPARE(root->property("pendingPath").toString(), QStringLiteral("/detail"));
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.15)),
                                      Q_ARG(QVariant, QVariant(-1400.0))));
    CTEST_WAIT_DEFAULT(root->property("transitionCanCommit").toBool());
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "cancelTransition"));
    CTEST_WAIT_DEFAULT(!root->property("transitionActive").toBool());
    CTEST_COMPARE(root->property("currentPath").toString(), QStringLiteral("/"));

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "beginForward"));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "updateTransition",
                                      Q_ARG(QVariant, QVariant(0.15)),
                                      Q_ARG(QVariant, QVariant(-1400.0))));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "finishAuto"));
    CTEST_WAIT_DEFAULT(!root->property("transitionActive").toBool());
    CTEST_WAIT_COMPARE(root->property("currentPath").toString(), QStringLiteral("/detail"));
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
    CTEST_CHECK(root);
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "prepare"));

    CTEST_WAIT_COMPARE(root->property("ownerOverview").toString(), QStringLiteral("/overview"));
    CTEST_COMPARE(root->property("ownerReports").toString(), QString());
    CTEST_COMPARE(root->property("canWriteOverview").toBool(), true);
    CTEST_COMPARE(root->property("canWriteReports").toBool(), false);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "writeOverviewStatus",
                                      Q_ARG(QVariant, QVariant(QStringLiteral("Working")))));
    CTEST_WAIT_COMPARE(root->property("overviewStatus").toString(), QStringLiteral("Working"));

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "goReports"));
    CTEST_WAIT_COMPARE(root->property("canWriteReports").toBool(), false);
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
    CTEST_CHECK(root);
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "prepare"));

    CTEST_WAIT_COMPARE(root->property("ownerOverview").toString(), QStringLiteral("/overview"));
    CTEST_COMPARE(root->property("ownerReports").toString(), QString());

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "goReports"));
    CTEST_WAIT_COMPARE(root->property("ownerReports").toString(), QStringLiteral("/reports"));
    CTEST_COMPARE(root->property("canWriteReports").toBool(), true);

    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "goBack"));
    CTEST_WAIT_COMPARE(root->property("ownerReports").toString(), QString());
    CTEST_COMPARE(root->property("canWriteReports").toBool(), false);
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    PageRouterTests tests;
    runner.run("route_params_are_passed_to_target_component", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.route_params_are_passed_to_target_component(); });
    runner.run("default_initial_path_does_not_override_bound_path", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.default_initial_path_does_not_override_bound_path(); });
    runner.run("component_navigation_keeps_path_stack_in_sync", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.component_navigation_keeps_path_stack_in_sync(); });
    runner.run("route_pages_are_isolated_and_forced_to_viewport", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.route_pages_are_isolated_and_forced_to_viewport(); });
    runner.run("page_router_retain_inactive_depth_contract", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.page_router_retain_inactive_depth_contract(); });
    runner.run("page_router_updates_view_state_tracker_from_stack", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.page_router_updates_view_state_tracker_from_stack(); });
    runner.run("global_navigator_allows_one_line_navigation", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.global_navigator_allows_one_line_navigation(); });
    runner.run("global_navigator_falls_back_to_previous_router", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.global_navigator_falls_back_to_previous_router(); });
    runner.run("interactive_navigation_transitions_hold_route_commit_until_finish", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.interactive_navigation_transitions_hold_route_commit_until_finish(); });
    runner.run("page_transition_controller_proxies_application_window_router", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.page_transition_controller_proxies_application_window_router(); });
    runner.run("route_mvvm_binding_and_write_ownership_are_applied", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.route_mvvm_binding_and_write_ownership_are_applied(); });
    runner.run("route_mvvm_ownership_is_released_when_view_is_popped", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.route_mvvm_ownership_is_released_when_view_is_popped(); });
    return runner.result();
}
