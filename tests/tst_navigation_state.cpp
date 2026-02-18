#include <QtTest>

#include <QCoreApplication>
#include <QEvent>
#include <QPointer>
#include <QSignalSpy>
#include <QtPlugin>

#include "backend/navigation/pagemonitor.h"
#include "backend/navigation/routematcher.h"
#include "backend/navigation/viewstatetracker.h"
#include "backend/state/viewmodelregistry.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class MutableStatusModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)

public:
    explicit MutableStatusModel(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    QString status() const
    {
        return m_status;
    }

    void setStatus(const QString &value)
    {
        if (m_status == value)
            return;
        m_status = value;
        emit statusChanged();
    }

signals:
    void statusChanged();

private:
    QString m_status = QStringLiteral("Idle");
};

class NavigationStateTests : public QObject
{
    Q_OBJECT

private slots:
    void page_monitor_history_metrics();
    void page_monitor_signal_contract_and_normalization();
    void view_state_tracker_syncs_stack_and_status();
    void view_state_tracker_disable_override_changes_active_target();
    void route_matcher_normalization_and_param_contract();
    void viewmodels_registry_binding_ownership_and_write_permissions();
    void viewmodels_registry_rebinding_clears_stale_ownership();
    void viewmodels_registry_tracks_keys_and_ownership();
    void viewmodels_registry_signal_and_prune_contract();
};

void NavigationStateTests::page_monitor_history_metrics()
{
    PageMonitor monitor;
    QCOMPARE(monitor.count(), 0);
    QCOMPARE(monitor.current(), QString());
    QVERIFY(!monitor.canUndo());

    monitor.record(QStringLiteral("/overview"));
    monitor.record(QStringLiteral("/overview"));
    QCOMPARE(monitor.count(), 1);
    QCOMPARE(monitor.current(), QStringLiteral("/overview"));

    monitor.record(QStringLiteral("/reports"));
    QCOMPARE(monitor.count(), 2);
    QVERIFY(monitor.canUndo());
    QCOMPARE(monitor.undo(), QStringLiteral("/overview"));
    QCOMPARE(monitor.count(), 1);
    QVERIFY(!monitor.canUndo());

    monitor.clear();
    QCOMPARE(monitor.count(), 0);
    QCOMPARE(monitor.current(), QString());
}

void NavigationStateTests::page_monitor_signal_contract_and_normalization()
{
    PageMonitor monitor;
    QSignalSpy historySpy(&monitor, &PageMonitor::historyChanged);
    QVERIFY(historySpy.isValid());

    monitor.record(QStringLiteral("  /overview  "));
    QCOMPARE(monitor.current(), QStringLiteral("/overview"));
    QCOMPARE(historySpy.count(), 1);

    monitor.record(QStringLiteral("/overview"));
    monitor.record(QStringLiteral("   "));
    QCOMPARE(historySpy.count(), 1);

    QCOMPARE(monitor.undo(), QStringLiteral("/overview"));
    QCOMPARE(historySpy.count(), 1);

    monitor.record(QStringLiteral("/details"));
    QCOMPARE(historySpy.count(), 2);
    QCOMPARE(monitor.undo(), QStringLiteral("/overview"));
    QCOMPARE(historySpy.count(), 3);

    monitor.clear();
    QCOMPARE(historySpy.count(), 4);
    monitor.clear();
    QCOMPARE(historySpy.count(), 4);
}

void NavigationStateTests::view_state_tracker_syncs_stack_and_status()
{
    ViewStateTracker tracker;
    QSignalSpy stackSpy(&tracker, &ViewStateTracker::stackChanged);
    QVERIFY(stackSpy.isValid());

    const QVariantList entries = {
        QVariantMap {
            { QStringLiteral("viewId"), QStringLiteral("/overview") },
            { QStringLiteral("path"), QStringLiteral("/overview") },
            { QStringLiteral("enabled"), true }
        },
        QVariantMap {
            { QStringLiteral("viewId"), QStringLiteral("/reports") },
            { QStringLiteral("path"), QStringLiteral("/reports") },
            { QStringLiteral("enabled"), true }
        },
        QVariantMap {
            { QStringLiteral("viewId"), QStringLiteral("/settings") },
            { QStringLiteral("path"), QStringLiteral("/settings") },
            { QStringLiteral("enabled"), false }
        }
    };

    tracker.syncStack(entries);
    QCOMPARE(stackSpy.count(), 1);
    QCOMPARE(tracker.loadedCount(), 3);
    QCOMPARE(tracker.loadedViews(),
             (QStringList { QStringLiteral("/overview"), QStringLiteral("/reports"), QStringLiteral("/settings") }));
    QCOMPARE(tracker.activeViews(), (QStringList { QStringLiteral("/reports") }));
    QCOMPARE(tracker.inactiveViews(), (QStringList { QStringLiteral("/overview") }));
    QCOMPARE(tracker.disabledViews(), (QStringList { QStringLiteral("/settings") }));
    QCOMPARE(tracker.currentActiveView(), QStringLiteral("/reports"));

    QCOMPARE(tracker.stateOf(QStringLiteral("/overview")), QStringLiteral("Inactive"));
    QCOMPARE(tracker.stateOf(QStringLiteral("/reports")), QStringLiteral("Active"));
    QCOMPARE(tracker.stateOf(QStringLiteral("/settings")), QStringLiteral("Disabled"));

    const QVariantMap reportView = tracker.view(QStringLiteral("/reports"));
    QCOMPARE(reportView.value(QStringLiteral("state")).toString(), QStringLiteral("Active"));
    QCOMPARE(reportView.value(QStringLiteral("index")).toInt(), 1);
    QCOMPARE(reportView.value(QStringLiteral("active")).toBool(), true);

    tracker.syncStack(entries);
    QCOMPARE(stackSpy.count(), 1);
}

void NavigationStateTests::view_state_tracker_disable_override_changes_active_target()
{
    ViewStateTracker tracker;
    tracker.syncStack(QVariantList {
        QVariantMap {
            { QStringLiteral("viewId"), QStringLiteral("/a") },
            { QStringLiteral("enabled"), true }
        },
        QVariantMap {
            { QStringLiteral("viewId"), QStringLiteral("/b") },
            { QStringLiteral("enabled"), true }
        },
        QVariantMap {
            { QStringLiteral("viewId"), QStringLiteral("/c") },
            { QStringLiteral("enabled"), true }
        }
    });

    QCOMPARE(tracker.currentActiveView(), QStringLiteral("/c"));

    QSignalSpy stackSpy(&tracker, &ViewStateTracker::stackChanged);
    QVERIFY(stackSpy.isValid());

    tracker.setViewDisabled(QStringLiteral("/c"), true);
    QCOMPARE(tracker.currentActiveView(), QStringLiteral("/b"));
    QCOMPARE(tracker.stateOf(QStringLiteral("/c")), QStringLiteral("Disabled"));
    QCOMPARE(tracker.activeViews(), (QStringList { QStringLiteral("/b") }));

    tracker.setViewEnabled(QStringLiteral("/c"), true);
    QCOMPARE(tracker.currentActiveView(), QStringLiteral("/c"));
    QCOMPARE(tracker.stateOf(QStringLiteral("/c")), QStringLiteral("Active"));

    tracker.clear();
    QCOMPARE(tracker.loadedCount(), 0);
    QCOMPARE(tracker.currentActiveView(), QString());
    QVERIFY(stackSpy.count() >= 3);
}

void NavigationStateTests::route_matcher_normalization_and_param_contract()
{
    RouteMatcher matcher;

    QCOMPARE(matcher.normalizePath(QString()), QStringLiteral("/"));
    QCOMPARE(matcher.normalizePath(QStringLiteral("reports/")), QStringLiteral("/reports"));
    QCOMPARE(matcher.normalizePath(QStringLiteral("/runs/123/")), QStringLiteral("/runs/123"));

    const QVariantMap rootMatch = matcher.match(QStringLiteral("/"), QStringLiteral("/"));
    QVERIFY(rootMatch.value(QStringLiteral("matched")).toBool());
    QVERIFY(rootMatch.value(QStringLiteral("params")).toMap().isEmpty());

    const QVariantMap paramMatch = matcher.match(QStringLiteral("/runs/42"), QStringLiteral("/runs/[id]"));
    QVERIFY(paramMatch.value(QStringLiteral("matched")).toBool());
    QCOMPARE(paramMatch.value(QStringLiteral("params")).toMap().value(QStringLiteral("id")).toString(),
             QStringLiteral("42"));

    const QVariantMap restMatch = matcher.match(QStringLiteral("/logs/a/b/c"), QStringLiteral("/logs/[...path]"));
    QVERIFY(restMatch.value(QStringLiteral("matched")).toBool());
    QCOMPARE(restMatch.value(QStringLiteral("params")).toMap().value(QStringLiteral("path")).toString(),
             QStringLiteral("a/b/c"));

    const QVariantMap noMatch = matcher.match(QStringLiteral("/runs"), QStringLiteral("/runs/[id]"));
    QVERIFY(!noMatch.value(QStringLiteral("matched")).toBool());
}

void NavigationStateTests::viewmodels_registry_binding_ownership_and_write_permissions()
{
    ViewModelRegistry registry;
    auto *model = new MutableStatusModel;
    registry.set(QStringLiteral("Example"), model);

    QSignalSpy viewsSpy(&registry, &ViewModelRegistry::viewsChanged);
    QSignalSpy ownershipSpy(&registry, &ViewModelRegistry::ownershipChanged);
    QSignalSpy errorSpy(&registry, &ViewModelRegistry::lastErrorChanged);
    QVERIFY(viewsSpy.isValid());
    QVERIFY(ownershipSpy.isValid());
    QVERIFY(errorSpy.isValid());

    QVERIFY(registry.bindView(QStringLiteral("OverviewView"), QStringLiteral("Example"), true));
    QCOMPARE(registry.keyForView(QStringLiteral("OverviewView")), QStringLiteral("Example"));
    QVERIFY(registry.getForView(QStringLiteral("OverviewView")) == model);
    QCOMPARE(registry.ownerOf(QStringLiteral("Example")), QStringLiteral("OverviewView"));
    QVERIFY(registry.canWrite(QStringLiteral("OverviewView")));

    QVERIFY(registry.bindView(QStringLiteral("ReportsView"), QStringLiteral("Example"), false));
    QCOMPARE(registry.keyForView(QStringLiteral("ReportsView")), QStringLiteral("Example"));
    QVERIFY(!registry.canWrite(QStringLiteral("ReportsView")));

    QVERIFY(!registry.updateProperty(QStringLiteral("ReportsView"),
                                     QStringLiteral("status"),
                                     QStringLiteral("Working")));
    QCOMPARE(registry.lastError(), QStringLiteral("View has no write permission for the model"));
    QCOMPARE(model->status(), QStringLiteral("Idle"));

    QVERIFY(registry.updateProperty(QStringLiteral("OverviewView"),
                                    QStringLiteral("status"),
                                    QStringLiteral("Working")));
    QCOMPARE(model->status(), QStringLiteral("Working"));
    QCOMPARE(registry.readProperty(QStringLiteral("OverviewView"), QStringLiteral("status")).toString(),
             QStringLiteral("Working"));

    QVERIFY(!registry.bindView(QStringLiteral("ReportsView"), QStringLiteral("Example"), true));
    QCOMPARE(registry.lastError(), QStringLiteral("ViewModel is already owned by another view"));

    QVERIFY(registry.releaseOwnership(QStringLiteral("OverviewView"), QStringLiteral("Example")));
    QCOMPARE(registry.ownerOf(QStringLiteral("Example")), QString());
    QVERIFY(registry.claimOwnership(QStringLiteral("ReportsView"), QStringLiteral("Example")));
    QCOMPARE(registry.ownerOf(QStringLiteral("Example")), QStringLiteral("ReportsView"));
    QVERIFY(registry.canWrite(QStringLiteral("ReportsView"), QStringLiteral("Example")));

    QVERIFY(registry.updateProperty(QStringLiteral("ReportsView"),
                                    QStringLiteral("status"),
                                    QStringLiteral("Ready")));
    QCOMPARE(model->status(), QStringLiteral("Ready"));

    registry.unbindView(QStringLiteral("ReportsView"));
    QCOMPARE(registry.keyForView(QStringLiteral("ReportsView")), QString());
    QCOMPARE(registry.ownerOf(QStringLiteral("Example")), QString());
    QVERIFY(!registry.canWrite(QStringLiteral("ReportsView")));

    QCOMPARE(registry.bindings().value(QStringLiteral("OverviewView")).toString(),
             QStringLiteral("Example"));
    QVERIFY(viewsSpy.count() >= 3);
    QVERIFY(ownershipSpy.count() >= 3);
    QVERIFY(errorSpy.count() >= 1);

    registry.remove(QStringLiteral("Example"));
    QCOMPARE(registry.keyForView(QStringLiteral("OverviewView")), QString());
}

void NavigationStateTests::viewmodels_registry_rebinding_clears_stale_ownership()
{
    ViewModelRegistry registry;
    auto *modelA = new MutableStatusModel;
    auto *modelB = new MutableStatusModel;

    registry.set(QStringLiteral("ModelA"), modelA);
    registry.set(QStringLiteral("ModelB"), modelB);

    QVERIFY(registry.bindView(QStringLiteral("EditorView"), QStringLiteral("ModelA"), true));
    QCOMPARE(registry.ownerOf(QStringLiteral("ModelA")), QStringLiteral("EditorView"));
    QCOMPARE(registry.ownerOf(QStringLiteral("ModelB")), QString());
    QVERIFY(registry.canWrite(QStringLiteral("EditorView"), QStringLiteral("ModelA")));

    QVERIFY(registry.bindView(QStringLiteral("EditorView"), QStringLiteral("ModelB"), true));
    QCOMPARE(registry.ownerOf(QStringLiteral("ModelA")), QString());
    QCOMPARE(registry.ownerOf(QStringLiteral("ModelB")), QStringLiteral("EditorView"));
    QVERIFY(!registry.canWrite(QStringLiteral("EditorView"), QStringLiteral("ModelA")));
    QVERIFY(registry.canWrite(QStringLiteral("EditorView"), QStringLiteral("ModelB")));

    QVERIFY(registry.bindView(QStringLiteral("EditorView"), QStringLiteral("ModelB"), false));
    QCOMPARE(registry.ownerOf(QStringLiteral("ModelB")), QString());
    QVERIFY(!registry.canWrite(QStringLiteral("EditorView"), QStringLiteral("ModelB")));
}

void NavigationStateTests::viewmodels_registry_tracks_keys_and_ownership()
{
    ViewModelRegistry registry;
    QCOMPARE(registry.keys().size(), 0);
    QVERIFY(registry.get(QStringLiteral("missing")) == nullptr);

    auto *shared = new QObject;
    QPointer<QObject> sharedGuard(shared);
    registry.set(QStringLiteral("alpha"), shared);
    registry.set(QStringLiteral("beta"), shared);
    QVERIFY(registry.keys().contains(QStringLiteral("alpha")));
    QVERIFY(registry.keys().contains(QStringLiteral("beta")));
    QVERIFY(registry.get(QStringLiteral("alpha")) == shared);
    QVERIFY(shared->parent() == &registry);

    registry.remove(QStringLiteral("alpha"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QVERIFY(!sharedGuard.isNull());

    registry.remove(QStringLiteral("beta"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QTRY_VERIFY(sharedGuard.isNull());

    auto *single = new QObject;
    QPointer<QObject> singleGuard(single);
    registry.set(QStringLiteral("single"), single);
    QCOMPARE(registry.keys().size(), 1);
    registry.clear();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QTRY_VERIFY(singleGuard.isNull());
    QCOMPARE(registry.keys().size(), 0);
}

void NavigationStateTests::viewmodels_registry_signal_and_prune_contract()
{
    ViewModelRegistry registry;
    QSignalSpy keysSpy(&registry, &ViewModelRegistry::keysChanged);
    QVERIFY(keysSpy.isValid());

    auto *ignored = new QObject;
    QPointer<QObject> ignoredGuard(ignored);
    registry.set(QStringLiteral("   "), ignored);
    QCOMPARE(keysSpy.count(), 0);
    QVERIFY(!ignoredGuard.isNull());
    delete ignored;
    QTRY_VERIFY(ignoredGuard.isNull());

    QObject externalParent;
    auto *externalOwned = new QObject(&externalParent);
    registry.set(QStringLiteral("primary"), externalOwned);
    QCOMPARE(keysSpy.count(), 1);
    QCOMPARE(externalOwned->parent(), &externalParent);
    registry.set(QStringLiteral("primary"), externalOwned);
    QCOMPARE(keysSpy.count(), 1);
    registry.set(QStringLiteral("secondary"), externalOwned);
    QCOMPARE(keysSpy.count(), 2);

    registry.remove(QStringLiteral("missing"));
    QCOMPARE(keysSpy.count(), 2);
    registry.remove(QStringLiteral("primary"));
    QCOMPARE(keysSpy.count(), 3);
    QVERIFY(!externalOwned->parent()->inherits("ViewModelRegistry"));

    QPointer<QObject> externalGuard(externalOwned);
    delete externalOwned;
    QTRY_VERIFY(externalGuard.isNull());

    auto *owned = new QObject;
    QPointer<QObject> ownedGuard(owned);
    registry.set(QStringLiteral("owned"), owned);
    QCOMPARE(keysSpy.count(), 5);
    QVERIFY(owned->parent() == &registry);
    registry.set(QStringLiteral("owned"), owned);
    QCOMPARE(keysSpy.count(), 5);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QVERIFY(!ownedGuard.isNull());

    auto *trigger = new QObject;
    registry.set(QStringLiteral("trigger"), trigger);
    QCOMPARE(keysSpy.count(), 6);
    QVERIFY(!registry.keys().contains(QStringLiteral("secondary")));

    registry.remove(QStringLiteral("owned"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    QTRY_VERIFY(ownedGuard.isNull());

    const int beforeClear = keysSpy.count();
    registry.clear();
    QCOMPARE(keysSpy.count(), beforeClear + 1);
    registry.clear();
    QCOMPARE(registry.keys().size(), 0);
}

QTEST_MAIN(NavigationStateTests)
#include "tst_navigation_state.moc"
