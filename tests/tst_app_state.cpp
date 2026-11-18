#include "QtEventWait.h"
#include <QGuiApplication>

#include <QtPlugin>

#include "backend/state/appstate.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class AppStateTests : public QObject
{
public:
    void defaults_and_progress_clamping();
    void route_tracking_updates_models();
    void snapshot_sync_bridges_backend_state();
    void resilient_normalization_and_index_clamping();
    void definitions_drive_route_matching();
};

void AppStateTests::defaults_and_progress_clamping()
{
    AppState state;

    CTEST_COMPARE(state.currentRoute(), QStringLiteral("/overview"));
    CTEST_COMPARE(state.progressStart(), 0.0);
    CTEST_COMPARE(state.progressEnd(), 100.0);
    CTEST_COMPARE(state.progressCurrent(), 46.0);

    state.setProgressCurrent(140.0);
    CTEST_COMPARE(state.progressCurrent(), 100.0);

    state.setProgressStart(40.0);
    CTEST_COMPARE(state.progressCurrent(), 100.0);

    state.setProgressEnd(80.0);
    CTEST_COMPARE(state.progressCurrent(), 80.0);

    state.nudgeProgress(-1000.0);
    CTEST_COMPARE(state.progressCurrent(), 40.0);
}

void AppStateTests::route_tracking_updates_models()
{
    AppState state;
    state.bootstrap();

    CTEST_CHECK(!state.navItems().isEmpty());
    CTEST_CHECK(!state.scaffoldNavModel().isEmpty());
    CTEST_CHECK(!state.demoListItems().isEmpty());

    state.recordNavigation(QStringLiteral("/reports"));
    CTEST_COMPARE(state.currentRoute(), QStringLiteral("/reports"));
    CTEST_COMPARE(state.scaffoldNavIndex(), -1);
    CTEST_COMPARE(state.pageHistory().size(), 1);
    CTEST_COMPARE(state.pageHistory().last(), QStringLiteral("/reports"));

    const QVariantList listItems = state.demoListItems();
    CTEST_CHECK(listItems.size() >= 2);
    CTEST_COMPARE(listItems.at(0).toMap().value(QStringLiteral("selected")).toBool(), false);
    CTEST_COMPARE(listItems.at(1).toMap().value(QStringLiteral("selected")).toBool(), true);

    state.recordNavigation(QStringLiteral("/reports"));
    CTEST_COMPARE(state.pageHistory().size(), 2);
    CTEST_COMPARE(state.pageHistory().last(), QStringLiteral("/reports"));

    QString reportsBadge;
    const QVariantList navItems = state.navItems();
    for (const QVariant &entry : navItems) {
        const QVariantMap map = entry.toMap();
        if (map.value(QStringLiteral("path")).toString() == QStringLiteral("/reports")) {
            reportsBadge = map.value(QStringLiteral("badge")).toString();
            break;
        }
    }
    CTEST_COMPARE(reportsBadge, QStringLiteral("2"));
}

void AppStateTests::snapshot_sync_bridges_backend_state()
{
    AppState state;

    const QVariantMap runtime {
        {QStringLiteral("pid"), 77},
        {QStringLiteral("uptimeMs"), 64000}
    };
    state.syncRuntimeSnapshot(runtime);
    CTEST_COMPARE(state.runtimeSnapshot(), runtime);

    const QVariantMap viewState {
        {QStringLiteral("currentActiveView"), QStringLiteral("/runs")},
        {QStringLiteral("loadedViews"), QStringList {QStringLiteral("/overview"), QStringLiteral("/runs")}}
    };
    state.syncViewStateSnapshot(viewState);
    CTEST_COMPARE(state.viewStateSnapshot(), viewState);

    state.syncPageHistory(QStringList {
        QStringLiteral("/overview"),
        QStringLiteral("/runs"),
        QStringLiteral("/runs")
    });

    CTEST_COMPARE(state.currentRoute(), QStringLiteral("/runs"));
    CTEST_COMPARE(state.scaffoldNavIndex(), 1);
    CTEST_COMPARE(state.pageHistory().size(), 3);

    const QVariantList scaffold = state.scaffoldNavModel();
    CTEST_CHECK(scaffold.size() >= 2);
    CTEST_COMPARE(scaffold.at(1).toMap().value(QStringLiteral("badge")).toString(), QStringLiteral("2"));
}

void AppStateTests::resilient_normalization_and_index_clamping()
{
    AppState state;

    state.recordNavigation(QStringLiteral(" reports?tab=summary "));
    CTEST_COMPARE(state.currentRoute(), QStringLiteral("/reports"));

    state.recordNavigation(QStringLiteral("https://example.com/runs/42?tab=logs#tail"));
    CTEST_COMPARE(state.currentRoute(), QStringLiteral("/runs/42"));

    state.setCurrentRoute(QStringLiteral("not-found: settings?x=1"));
    CTEST_COMPARE(state.currentRoute(), QStringLiteral("not-found: /settings"));

    state.setScaffoldNavIndex(99);
    CTEST_COMPARE(state.scaffoldNavIndex(), 3);
    state.setScaffoldNavIndex(-9);
    CTEST_COMPARE(state.scaffoldNavIndex(), -1);

    state.syncPageHistory(QStringList {
        QStringLiteral("/overview"),
        QStringLiteral("/runs/42"),
        QStringLiteral("/runs/9")
    });
    const QVariantList scaffold = state.scaffoldNavModel();
    CTEST_CHECK(scaffold.size() >= 2);
    CTEST_COMPARE(scaffold.at(1).toMap().value(QStringLiteral("badge")).toString(), QStringLiteral("2"));
}

void AppStateTests::definitions_drive_route_matching()
{
    AppState state;

    state.setScaffoldDefinitions(QVariantList {
        QVariantMap {
            {QStringLiteral("label"), QStringLiteral("Overview")},
            {QStringLiteral("path"), QStringLiteral("/overview")}
        },
        QVariantMap {
            {QStringLiteral("label"), QStringLiteral("Reports")},
            {QStringLiteral("path"), QStringLiteral("/reports")}
        }
    });

    const QVariantList scaffoldModel = state.scaffoldNavModel();
    CTEST_COMPARE(scaffoldModel.size(), 2);
    CTEST_COMPARE(scaffoldModel.at(1).toMap().value(QStringLiteral("path")).toString(), QStringLiteral("/reports"));

    state.recordNavigation(QStringLiteral("/reports"));
    CTEST_COMPARE(state.scaffoldNavIndex(), 1);

    state.recordNavigation(QStringLiteral("not-found: reports"));
    CTEST_COMPARE(state.currentRoute(), QStringLiteral("not-found: /reports"));
    CTEST_COMPARE(state.scaffoldNavIndex(), -1);

    const QVariantList refreshedScaffold = state.scaffoldNavModel();
    CTEST_COMPARE(refreshedScaffold.at(1).toMap().value(QStringLiteral("badge")).toString(), QStringLiteral("1"));
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    AppStateTests tests;
    runner.run("defaults_and_progress_clamping", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.defaults_and_progress_clamping(); });
    runner.run("route_tracking_updates_models", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.route_tracking_updates_models(); });
    runner.run("snapshot_sync_bridges_backend_state", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.snapshot_sync_bridges_backend_state(); });
    runner.run("resilient_normalization_and_index_clamping", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.resilient_normalization_and_index_clamping(); });
    runner.run("definitions_drive_route_matching", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.definitions_drive_route_matching(); });
    return runner.result();
}
