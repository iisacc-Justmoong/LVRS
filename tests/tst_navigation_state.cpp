#include <QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSignalSpy>
#include <QtPlugin>

#include "backend/model/hierarchymodel.h"
#include "backend/model/modelsource.h"
#include "backend/model/progressmodel.h"
#include "backend/model/tableheadermodel.h"
#include "backend/model/tablemodel.h"
#include "backend/navigation/navigationstackmodel.h"
#include "backend/navigation/pagemonitor.h"
#include "backend/navigation/routematcher.h"
#include "backend/navigation/viewstatetracker.h"
#include "backend/runtime/qmlcontextbinder.h"
#include "backend/state/statemodel.h"
#include "backend/state/viewmodel.h"
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

class VariantListObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count CONSTANT)

public:
    explicit VariantListObject(const QVariantList &entries, QObject *parent = nullptr)
        : QObject(parent)
        , m_entries(entries)
    {
    }

    int count() const
    {
        return m_entries.size();
    }

    Q_INVOKABLE QVariant get(int index) const
    {
        return index >= 0 && index < m_entries.size() ? m_entries.at(index) : QVariant();
    }

private:
    QVariantList m_entries;
};

class DedicatedStatusViewModel : public ViewModel
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)

public:
    explicit DedicatedStatusViewModel(QObject *parent = nullptr)
        : ViewModel(parent)
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
    void viewmodel_base_and_registry_descriptors_track_cpp_state();
    void state_model_stores_state_and_registry_descriptors_track_values();
    void cpp_model_sources_own_list_hierarchy_and_table_contracts();
    void qml_context_bind_plan_exposes_context_objects_and_viewmodels();
    void qml_context_bind_plan_reports_missing_required_objects();
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

void NavigationStateTests::viewmodel_base_and_registry_descriptors_track_cpp_state()
{
    ViewModelRegistry registry;
    auto *viewModel = new DedicatedStatusViewModel;
    viewModel->setKey(QStringLiteral("StatusVM"));
    viewModel->setDisplayName(QStringLiteral("Status"));
    viewModel->setMetadata({{QStringLiteral("domain"), QStringLiteral("navigation-test")}});

    QSignalSpy descriptorSpy(&registry, &ViewModelRegistry::descriptorsChanged);
    QVERIFY(descriptorSpy.isValid());

    QVERIFY(registry.registerViewModel(viewModel));
    QCOMPARE(registry.get(QStringLiteral("StatusVM")), viewModel);

    QVariantMap descriptor = registry.descriptor(QStringLiteral("StatusVM"));
    QCOMPARE(descriptor.value(QStringLiteral("viewModel")).toBool(), true);
    QCOMPARE(descriptor.value(QStringLiteral("viewModelKey")).toString(), QStringLiteral("StatusVM"));
    QCOMPARE(descriptor.value(QStringLiteral("displayName")).toString(), QStringLiteral("Status"));
    QCOMPARE(descriptor.value(QStringLiteral("busy")).toBool(), false);
    QCOMPARE(descriptor.value(QStringLiteral("hasError")).toBool(), false);
    QCOMPARE(descriptor.value(QStringLiteral("metadata")).toMap().value(QStringLiteral("domain")).toString(),
             QStringLiteral("navigation-test"));

    viewModel->setBusy(true);
    viewModel->setError(QStringLiteral("loading failed"));

    descriptor = registry.descriptor(QStringLiteral("StatusVM"));
    QCOMPARE(descriptor.value(QStringLiteral("busy")).toBool(), true);
    QCOMPARE(descriptor.value(QStringLiteral("error")).toString(), QStringLiteral("loading failed"));
    QCOMPARE(descriptor.value(QStringLiteral("hasError")).toBool(), true);
    QVERIFY(descriptorSpy.count() >= 3);

    const int beforeBindSignals = descriptorSpy.count();
    QVERIFY(registry.bindView(QStringLiteral("StatusView"), QStringLiteral("StatusVM"), true));
    QVERIFY(descriptorSpy.count() > beforeBindSignals);

    descriptor = registry.descriptor(QStringLiteral("StatusVM"));
    QCOMPARE(descriptor.value(QStringLiteral("owner")).toString(), QStringLiteral("StatusView"));
    QVERIFY(descriptor.value(QStringLiteral("views")).toList().contains(QStringLiteral("StatusView")));

    QVariantMap snapshot = viewModel->snapshot();
    QCOMPARE(snapshot.value(QStringLiteral("key")).toString(), QStringLiteral("StatusVM"));
    QCOMPARE(snapshot.value(QStringLiteral("displayName")).toString(), QStringLiteral("Status"));
    QCOMPARE(snapshot.value(QStringLiteral("busy")).toBool(), true);
    QCOMPARE(snapshot.value(QStringLiteral("hasError")).toBool(), true);

    viewModel->clearError();
    QCOMPARE(viewModel->hasError(), false);
}

void NavigationStateTests::state_model_stores_state_and_registry_descriptors_track_values()
{
    StateModel model;
    model.setKey(QStringLiteral("ProgressState"));
    model.setDisplayName(QStringLiteral("Progress State"));

    QSignalSpy valuesSpy(&model, &StateModel::valuesChanged);
    QSignalSpy revisionSpy(&model, &StateModel::revisionChanged);
    QSignalSpy valueSpy(&model, &StateModel::valueChanged);
    QVERIFY(valuesSpy.isValid());
    QVERIFY(revisionSpy.isValid());
    QVERIFY(valueSpy.isValid());

    QVERIFY(model.setValue(QStringLiteral(" currentValue "), 40));
    QVERIFY(model.setValue(QStringLiteral("minimumValue"), -10));
    QVERIFY(model.setValue(QStringLiteral("maximumValue"), 90));
    QCOMPARE(model.value(QStringLiteral("currentValue")).toInt(), 40);
    QCOMPARE(model.value(QStringLiteral("missing"), QStringLiteral("fallback")).toString(),
             QStringLiteral("fallback"));
    QVERIFY(model.hasValue(QStringLiteral("minimumValue")));
    QCOMPARE(model.stateKeys(),
             (QStringList {
                 QStringLiteral("currentValue"),
                 QStringLiteral("maximumValue"),
                 QStringLiteral("minimumValue")
             }));

    QVERIFY(model.applyPatch(QVariantMap {
        {QStringLiteral("currentValue"), 64},
        {QStringLiteral("startValue"), 10}
    }));
    QCOMPARE(model.value(QStringLiteral("currentValue")).toInt(), 64);
    QCOMPARE(model.value(QStringLiteral("startValue")).toInt(), 10);

    const QVariantMap snapshot = model.stateSnapshot();
    QCOMPARE(snapshot.value(QStringLiteral("key")).toString(), QStringLiteral("ProgressState"));
    QCOMPARE(snapshot.value(QStringLiteral("displayName")).toString(), QStringLiteral("Progress State"));
    QCOMPARE(snapshot.value(QStringLiteral("values")).toMap().value(QStringLiteral("currentValue")).toInt(), 64);
    QVERIFY(snapshot.value(QStringLiteral("revision")).toInt() >= 4);
    QVERIFY(!snapshot.value(QStringLiteral("empty")).toBool());

    QVERIFY(model.removeValue(QStringLiteral("minimumValue")));
    QVERIFY(!model.hasValue(QStringLiteral("minimumValue")));
    QVERIFY(!model.setValue(QStringLiteral(" "), 1));
    QCOMPARE(model.error(), QStringLiteral("Empty state key"));
    model.clearValues();
    QVERIFY(model.empty());
    QVERIFY(valuesSpy.count() >= 5);
    QVERIFY(revisionSpy.count() >= 5);
    QVERIFY(valueSpy.count() >= 5);

    ViewModelRegistry registry;
    auto *registered = new StateModel;
    registered->setKey(QStringLiteral("PanelState"));
    QVERIFY(registered->setValue(QStringLiteral("activeKey"), QStringLiteral("row-a")));

    QSignalSpy descriptorSpy(&registry, &ViewModelRegistry::descriptorsChanged);
    QVERIFY(descriptorSpy.isValid());

    QVERIFY(registry.registerViewModel(registered));
    QVariantMap descriptor = registry.descriptor(QStringLiteral("PanelState"));
    QCOMPARE(descriptor.value(QStringLiteral("viewModel")).toBool(), true);
    QCOMPARE(descriptor.value(QStringLiteral("stateModel")).toBool(), true);
    QCOMPARE(descriptor.value(QStringLiteral("values")).toMap().value(QStringLiteral("activeKey")).toString(),
             QStringLiteral("row-a"));
    QVERIFY(descriptor.value(QStringLiteral("stateKeys")).toStringList().contains(QStringLiteral("activeKey")));
    QCOMPARE(descriptor.value(QStringLiteral("empty")).toBool(), false);

    const int beforeStateChange = descriptorSpy.count();
    QVERIFY(registered->setValue(QStringLiteral("activeKey"), QStringLiteral("row-b")));
    QVERIFY(descriptorSpy.count() > beforeStateChange);
    descriptor = registry.descriptor(QStringLiteral("PanelState"));
    QCOMPARE(descriptor.value(QStringLiteral("values")).toMap().value(QStringLiteral("activeKey")).toString(),
             QStringLiteral("row-b"));
}

void NavigationStateTests::cpp_model_sources_own_list_hierarchy_and_table_contracts()
{
    ModelSource listSource;
    listSource.setSource(QVariantList {
        QVariantMap {
            {QStringLiteral("label"), QStringLiteral("Native A")},
            {QStringLiteral("enabled"), true}
        },
        QVariantMap {
            {QStringLiteral("label"), QStringLiteral("Native B")},
            {QStringLiteral("enabled"), false}
        }
    });
    QCOMPARE(listSource.count(), 2);
    QCOMPARE(listSource.textValue(listSource.at(0), QStringList {QStringLiteral("label")}, QString()),
             QStringLiteral("Native A"));
    QCOMPARE(listSource.boolValue(listSource.at(1), QStringLiteral("enabled"), true), false);

    HierarchyModel hierarchyModel;
    hierarchyModel.setLabelRole(QStringLiteral("name"));
    hierarchyModel.setCountRole(QStringLiteral("counter"));
    hierarchyModel.setSource(QVariantList {
        QVariantMap {
            {QStringLiteral("key"), QStringLiteral("root")},
            {QStringLiteral("name"), QStringLiteral("Root")},
            {QStringLiteral("depth"), 0},
            {QStringLiteral("expanded"), true},
            {QStringLiteral("counter"), 2}
        },
        QVariantMap {
            {QStringLiteral("key"), QStringLiteral("leaf")},
            {QStringLiteral("name"), QStringLiteral("Leaf")},
            {QStringLiteral("depth"), 1},
            {QStringLiteral("selected"), true},
            {QStringLiteral("counter"), 7}
        }
    });
    QCOMPARE(hierarchyModel.count(), 2);
    const QVariantMap leafDescriptor = hierarchyModel.descriptorAt(1);
    QCOMPARE(leafDescriptor.value(QStringLiteral("itemKey")).toString(), QStringLiteral("leaf"));
    QCOMPARE(leafDescriptor.value(QStringLiteral("label")).toString(), QStringLiteral("Leaf"));
    QCOMPARE(leafDescriptor.value(QStringLiteral("indentLevel")).toInt(), 1);
    QCOMPARE(leafDescriptor.value(QStringLiteral("count")).toInt(), 7);
    QCOMPARE(leafDescriptor.value(QStringLiteral("selected")).toBool(), true);
    const QVariantList hierarchyInteractionRows {
        QVariantMap {
            {QStringLiteral("itemId"), 0},
            {QStringLiteral("itemKey"), QStringLiteral("root")},
            {QStringLiteral("label"), QStringLiteral("Root")},
            {QStringLiteral("indentLevel"), 0},
            {QStringLiteral("expanded"), true},
            {QStringLiteral("enabled"), true},
            {QStringLiteral("activatable"), true},
            {QStringLiteral("showChevron"), true}
        },
        QVariantMap {
            {QStringLiteral("itemId"), 1},
            {QStringLiteral("itemKey"), QStringLiteral("child")},
            {QStringLiteral("label"), QStringLiteral("Child")},
            {QStringLiteral("indentLevel"), 1},
            {QStringLiteral("expanded"), false},
            {QStringLiteral("enabled"), true},
            {QStringLiteral("activatable"), true},
            {QStringLiteral("showChevron"), true}
        },
        QVariantMap {
            {QStringLiteral("itemId"), 2},
            {QStringLiteral("itemKey"), QStringLiteral("leaf")},
            {QStringLiteral("label"), QStringLiteral("Leaf")},
            {QStringLiteral("indentLevel"), 2},
            {QStringLiteral("enabled"), true},
            {QStringLiteral("activatable"), true}
        }
    };
    const QVariantMap interactionState = hierarchyModel.projectInteractionState(hierarchyInteractionRows);
    QCOMPARE(interactionState.value(QStringLiteral("visibleItemCount")).toInt(), 2);
    QCOMPARE(interactionState.value(QStringLiteral("metadata")).toList().at(0).toMap().value(QStringLiteral("childCount")).toInt(), 1);
    QCOMPARE(interactionState.value(QStringLiteral("metadata")).toList().at(0).toMap().value(QStringLiteral("pathLabel")).toString(),
             QStringLiteral("Root"));
    const QVariantMap childMetadata = interactionState.value(QStringLiteral("metadata")).toList().at(1).toMap();
    QCOMPARE(childMetadata.value(QStringLiteral("ancestorItemKeys")).toList().at(0).toString(), QStringLiteral("root"));
    QCOMPARE(childMetadata.value(QStringLiteral("pathItemLabels")).toList().at(0).toString(), QStringLiteral("Root"));
    QCOMPARE(childMetadata.value(QStringLiteral("pathItemLabels")).toList().at(1).toString(), QStringLiteral("Child"));
    QCOMPARE(hierarchyModel.descendantRangeEnd(hierarchyInteractionRows, 0), 2);
    const QVariantMap dragTarget = hierarchyModel.resolveDragTarget(hierarchyInteractionRows, 1, 2, 3, 8, 8, 8);
    QVERIFY(!dragTarget.isEmpty());
    QVERIFY(dragTarget.value(QStringLiteral("depth")).toInt() >= 0);
    const QVariantMap moveResult = hierarchyModel.moveDescriptors(hierarchyInteractionRows, 1, 2, 0, 0);
    QVERIFY(moveResult.value(QStringLiteral("accepted")).toBool());
    QCOMPARE(moveResult.value(QStringLiteral("reorderedDescriptors")).toList().size(), 3);

    TableHeaderModel headerModel;
    headerModel.setTableWidth(300);
    headerModel.setMinColumnWidth(40);
    headerModel.setCellItems(QVariantList {
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Name")}, {QStringLiteral("type"), QStringLiteral("string")}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Count")}, {QStringLiteral("valueType"), QStringLiteral("int")}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Ratio")}, {QStringLiteral("cellType"), QStringLiteral("float")}}
    });
    headerModel.setColumnWidths(QVariantList {80, 100, 120});
    QCOMPARE(headerModel.columnCount(), 3);
    QCOMPARE(headerModel.columnType(1), QStringLiteral("int"));
    QCOMPARE(headerModel.columnX(2), 180);
    QCOMPARE(headerModel.descriptorAt(2).value(QStringLiteral("width")).toInt(), 120);
    TableHeaderModel emptyHeaderModel;
    emptyHeaderModel.setTableWidth(120);
    emptyHeaderModel.setCellItems(QVariantList {});
    QCOMPARE(emptyHeaderModel.columnCount(), 0);
    QCOMPARE(emptyHeaderModel.descriptorAt(0).value(QStringLiteral("text")).toString(), QStringLiteral("Column"));
    QCOMPARE(emptyHeaderModel.descriptorAt(0).value(QStringLiteral("width")).toInt(), 120);

    StateModel progressState;
    progressState.setValue(QStringLiteral("minimumValue"), 10);
    progressState.setValue(QStringLiteral("maximumValue"), 110);
    progressState.setValue(QStringLiteral("startValue"), 30);
    progressState.setValue(QStringLiteral("currentValue"), 80);
    ProgressModel progressModel;
    progressModel.setStateModel(&progressState);
    QCOMPARE(progressModel.effectiveMinimumValue(), 10.0);
    QCOMPARE(progressModel.effectiveMaximumValue(), 110.0);
    QVERIFY(qAbs(progressModel.normalizedStart() - 0.2) < 0.001);
    QVERIFY(qAbs(progressModel.normalizedCurrent() - 0.7) < 0.001);
    QVERIFY(qAbs(progressModel.fillProgress() - 0.5) < 0.001);
    QVERIFY(qFuzzyCompare(progressModel.radiusFor(1, 6, 100, 8), 4.0));

    NavigationStackModel navigationStack;
    navigationStack.applyPathOperation(QStringLiteral("/"), QVariantMap(), QStringLiteral("set"));
    navigationStack.applyPathOperation(QStringLiteral("reports"), QVariantMap {{QStringLiteral("page"), 2}}, QStringLiteral("push"));
    QCOMPARE(navigationStack.depth(), 2);
    QCOMPARE(navigationStack.currentPath(), QStringLiteral("/reports"));
    QCOMPARE(navigationStack.currentParams().toMap().value(QStringLiteral("page")).toInt(), 2);
    QCOMPARE(navigationStack.viewTrackingEntries().size(), 2);
    navigationStack.pop();
    QCOMPARE(navigationStack.depth(), 1);
    QCOMPARE(navigationStack.currentPath(), QStringLiteral("/"));

    VariantListObject readOnlyRows(QVariantList {
        QVariantList {
            QVariantMap {{QStringLiteral("text"), QStringLiteral("Model Cell")}},
            QVariantMap {{QStringLiteral("text"), QStringLiteral("Second")}}
        }
    });
    TableModel readOnlyTableModel;
    readOnlyTableModel.setRows(QVariant::fromValue(static_cast<QObject *>(&readOnlyRows)));
    QCOMPARE(readOnlyTableModel.rowCount(), 1);
    QCOMPARE(readOnlyTableModel.cellText(0, 0), QStringLiteral("Model Cell"));
    QVERIFY(!readOnlyTableModel.structureMutationAvailable());
    QVERIFY(!readOnlyTableModel.insertRow(1));

    TableModel layoutTableModel;
    layoutTableModel.setTableWidth(300);
    layoutTableModel.setMinColumnWidth(40);
    layoutTableModel.setMinRowHeight(20);
    layoutTableModel.setHeaderColumns(QStringList {
        QStringLiteral("A"),
        QStringLiteral("B"),
        QStringLiteral("C")
    });
    layoutTableModel.setRows(QVariantList {
        QVariantList {
            QVariantMap {{QStringLiteral("text"), QStringLiteral("A1")}},
            QVariantMap {{QStringLiteral("text"), QStringLiteral("B1")}},
            QVariantMap {{QStringLiteral("text"), QStringLiteral("C1")}}
        },
        QVariantList {
            QVariantMap {{QStringLiteral("text"), QStringLiteral("A2")}},
            QVariantMap {{QStringLiteral("text"), QStringLiteral("B2")}},
            QVariantMap {{QStringLiteral("text"), QStringLiteral("C2")}}
        }
    });
    layoutTableModel.setColumnWidths(QVariantList {80, 100, 120});
    layoutTableModel.setRowHeights(QVariantList {24, 30});
    QCOMPARE(layoutTableModel.columnWidth(0), 80);
    QCOMPARE(layoutTableModel.columnX(2), 180);
    QCOMPARE(layoutTableModel.columnSpanWidth(1, 2), 220);
    QCOMPARE(layoutTableModel.rowHeightAt(1), 30);
    QCOMPARE(layoutTableModel.rowY(1), 24);
    QCOMPARE(layoutTableModel.totalBodyHeight(), 54);
    const QVariantMap layoutCell = layoutTableModel.visibleCells().at(1).toMap();
    QCOMPARE(layoutCell.value(QStringLiteral("x")).toInt(), 80);
    QCOMPARE(layoutCell.value(QStringLiteral("width")).toInt(), 100);
    QCOMPARE(layoutCell.value(QStringLiteral("height")).toInt(), 24);
    QVERIFY(layoutTableModel.setColumnWidth(2, 1));
    QCOMPARE(layoutTableModel.columnWidth(2), 40);
    QVERIFY(layoutTableModel.undo());
    QCOMPARE(layoutTableModel.columnWidth(2), 120);
    QVERIFY(layoutTableModel.beginColumnResize(1, 100));
    QVERIFY(layoutTableModel.updateColumnResize(135));
    QCOMPARE(layoutTableModel.resizingColumnIndex(), 1);
    QCOMPARE(layoutTableModel.columnWidth(1), 135);
    layoutTableModel.endColumnResize();
    QCOMPARE(layoutTableModel.resizingColumnIndex(), -1);
    QVERIFY(layoutTableModel.undo());
    QCOMPARE(layoutTableModel.columnWidth(1), 100);
    QVERIFY(layoutTableModel.beginRowResize(0, 20));
    QVERIFY(layoutTableModel.updateRowResize(32));
    QCOMPARE(layoutTableModel.resizingRowIndex(), 0);
    QCOMPARE(layoutTableModel.rowHeightAt(0), 36);
    layoutTableModel.endRowResize();
    QCOMPARE(layoutTableModel.resizingRowIndex(), -1);
    QVERIFY(layoutTableModel.undo());
    QCOMPARE(layoutTableModel.rowHeightAt(0), 24);
    QVERIFY(layoutTableModel.setContextCell(0, 1));
    QCOMPARE(layoutTableModel.contextRowIndex(), 0);
    QCOMPARE(layoutTableModel.contextColumnIndex(), 1);
    const QVariantList contextDescriptors = layoutTableModel.contextMenuDescriptors(0, 1);
    QCOMPARE(contextDescriptors.size(), 3);
    QCOMPARE(contextDescriptors.at(0).toMap().value(QStringLiteral("action")).toString(), QStringLiteral("deleteRow"));
    QCOMPARE(contextDescriptors.at(2).toMap().value(QStringLiteral("action")).toString(), QStringLiteral("deleteColumn"));
    const QVariantMap contextResult = layoutTableModel.triggerContextAction(QStringLiteral("deleteColumn"), 0, 1);
    QVERIFY(contextResult.value(QStringLiteral("accepted")).toBool());
    QCOMPARE(layoutTableModel.columnCount(), 2);
    QCOMPARE(layoutTableModel.columnWidths().toList().size(), 2);
    QCOMPARE(layoutTableModel.contextColumnIndex(), -1);
    QVERIFY(layoutTableModel.undo());
    QCOMPARE(layoutTableModel.columnCount(), 3);

    ModelUndoStack stack;
    stack.pushSnapshot(QVariantMap {{QStringLiteral("value"), 1}});
    stack.pushSnapshot(QVariantMap {{QStringLiteral("value"), 2}});
    QCOMPARE(stack.undoDepth(), 2);
    QVERIFY(stack.canUndo());
    const QVariant firstUndo = stack.takeUndoSnapshot(QVariantMap {{QStringLiteral("value"), 3}});
    QCOMPARE(firstUndo.toMap().value(QStringLiteral("value")).toInt(), 2);
    QCOMPARE(stack.undoDepth(), 1);
    QCOMPARE(stack.redoDepth(), 1);
    QVERIFY(stack.canRedo());
    const QVariant firstRedo = stack.takeRedoSnapshot(firstUndo);
    QCOMPARE(firstRedo.toMap().value(QStringLiteral("value")).toInt(), 3);
    QCOMPARE(stack.undoDepth(), 2);
    QCOMPARE(stack.redoDepth(), 0);

    TableModel tableModel;
    tableModel.setInputable(true);
    tableModel.setHeaderCellItems(QVariantList {
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Name")}, {QStringLiteral("type"), QStringLiteral("string")}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Count")}, {QStringLiteral("valueType"), QStringLiteral("int")}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Ratio")}, {QStringLiteral("cellType"), QStringLiteral("float")}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Enabled")}, {QStringLiteral("dataType"), QStringLiteral("bool")}}
    });
    tableModel.setRows(QVariantList {
        QVariantList {
            QVariantMap {{QStringLiteral("value"), QStringLiteral("Renderer")}},
            QVariantMap {{QStringLiteral("value"), 3}},
            QVariantMap {{QStringLiteral("value"), 1.5}},
            QVariantMap {{QStringLiteral("value"), true}}
        },
        QVariantList {
            QVariantMap {{QStringLiteral("value"), QStringLiteral("Writer")}},
            QVariantMap {{QStringLiteral("value"), 2}},
            QVariantMap {{QStringLiteral("value"), 0.5}},
            QVariantMap {{QStringLiteral("value"), false}}
        }
    });

    QCOMPARE(tableModel.rowCount(), 2);
    QCOMPARE(tableModel.columnCount(), 4);
    QCOMPARE(tableModel.headerCellType(0), QStringLiteral("string"));
    QCOMPARE(tableModel.headerCellType(1), QStringLiteral("int"));
    QCOMPARE(tableModel.headerCellType(2), QStringLiteral("float"));
    QCOMPARE(tableModel.headerCellType(3), QStringLiteral("bool"));
    QCOMPARE(tableModel.cellText(0, 0), QStringLiteral("Renderer"));
    QVERIFY(tableModel.setCellValue(0, 1, QStringLiteral("42")));
    QVERIFY(!tableModel.setCellValue(0, 1, QStringLiteral("42.5")));
    QVERIFY(tableModel.setCellValue(0, 3, QStringLiteral("false")));
    QCOMPARE(tableModel.cellAt(0, 1).toMap().value(QStringLiteral("value")).toInt(), 42);
    QCOMPARE(tableModel.cellAt(0, 3).toMap().value(QStringLiteral("value")).toBool(), false);
    QVERIFY(tableModel.canUndo());
    QVERIFY(tableModel.undoDepth() >= 2);
    QVERIFY(tableModel.undo());
    QCOMPARE(tableModel.cellAt(0, 3).toMap().value(QStringLiteral("value")).toBool(), true);
    QVERIFY(tableModel.canRedo());
    QVERIFY(tableModel.redo());
    QCOMPARE(tableModel.cellAt(0, 3).toMap().value(QStringLiteral("value")).toBool(), false);
    QVERIFY(!tableModel.canRedo());

    QCOMPARE(tableModel.visibleCells().size(), 8);
    QVERIFY(tableModel.mergeCells(0, 0, 1, 2));
    QVERIFY(tableModel.isCoveredCell(0, 1));
    QCOMPARE(tableModel.visibleCells().size(), 7);
    QVERIFY(tableModel.splitCell(0, 1));
    QVERIFY(!tableModel.isCoveredCell(0, 1));
    QCOMPARE(tableModel.visibleCells().size(), 8);

    QVERIFY(tableModel.insertRow(1));
    QCOMPARE(tableModel.rowCount(), 3);
    QCOMPARE(tableModel.cellAt(1, 1).toMap().value(QStringLiteral("value")).toInt(), 0);
    QCOMPARE(tableModel.cellAt(1, 3).toMap().value(QStringLiteral("value")).toBool(), false);
    QVERIFY(tableModel.insertColumn(2));
    QCOMPARE(tableModel.columnCount(), 5);
    QVERIFY(tableModel.deleteColumn(2));
    QCOMPARE(tableModel.columnCount(), 4);
    QVERIFY(tableModel.deleteRow(1));
    QCOMPARE(tableModel.rowCount(), 2);
    QVERIFY(tableModel.undo());
    QCOMPARE(tableModel.rowCount(), 3);
    QVERIFY(tableModel.redo());
    QCOMPARE(tableModel.rowCount(), 2);

    tableModel.setRows(QVariantList {QVariantList {QStringLiteral("External")}});
    QVERIFY(!tableModel.canUndo());
    QVERIFY(!tableModel.canRedo());
    QCOMPARE(tableModel.rowCount(), 1);
}

void NavigationStateTests::qml_context_bind_plan_exposes_context_objects_and_viewmodels()
{
    QQmlApplicationEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);

    QObject service;
    auto *viewModel = new DedicatedStatusViewModel(&engine);

    lvrs::QmlContextBindPlan plan;
    lvrs::QmlContextObjectBinding serviceBinding;
    serviceBinding.contextName = QStringLiteral("statusService");
    serviceBinding.object = &service;
    plan.contextObjects.append(serviceBinding);

    lvrs::QmlViewModelBinding viewModelBinding;
    viewModelBinding.key = QStringLiteral("StatusVM");
    viewModelBinding.object = viewModel;
    viewModelBinding.contextName = QStringLiteral("statusViewModel");
    viewModelBinding.displayName = QStringLiteral("Status");
    viewModelBinding.metadata = {{QStringLiteral("domain"), QStringLiteral("navigation-test")}};
    viewModelBinding.viewId = QStringLiteral("StatusView");
    viewModelBinding.writable = true;
    plan.viewModels.append(viewModelBinding);

    const lvrs::QmlContextBindResult result = lvrs::applyQmlContextBindPlan(engine, plan);
    QVERIFY2(result.ok, qPrintable(result.errorMessage()));
    QCOMPARE(result.contextNames,
             (QStringList {QStringLiteral("statusService"), QStringLiteral("statusViewModel")}));
    QCOMPARE(result.viewModelKeys, (QStringList {QStringLiteral("StatusVM")}));

    QCOMPARE(qvariant_cast<QObject *>(engine.rootContext()->contextProperty(QStringLiteral("statusService"))),
             &service);
    QCOMPARE(qvariant_cast<QObject *>(engine.rootContext()->contextProperty(QStringLiteral("statusViewModel"))),
             viewModel);

    auto *registry = engine.singletonInstance<ViewModelRegistry *>(QStringLiteral("LVRS"),
                                                                   QStringLiteral("ViewModels"));
    QVERIFY(registry);
    QCOMPARE(registry->get(QStringLiteral("StatusVM")), viewModel);
    QCOMPARE(registry->ownerOf(QStringLiteral("StatusVM")), QStringLiteral("StatusView"));
    QVERIFY(registry->canWrite(QStringLiteral("StatusView"), QStringLiteral("StatusVM")));

    const QVariantMap descriptor = registry->descriptor(QStringLiteral("StatusVM"));
    QCOMPARE(descriptor.value(QStringLiteral("displayName")).toString(), QStringLiteral("Status"));
    QCOMPARE(descriptor.value(QStringLiteral("metadata")).toMap().value(QStringLiteral("domain")).toString(),
             QStringLiteral("navigation-test"));
}

void NavigationStateTests::qml_context_bind_plan_reports_missing_required_objects()
{
    QQmlApplicationEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);

    lvrs::QmlContextBindPlan plan;
    lvrs::QmlContextObjectBinding missingContext;
    missingContext.contextName = QStringLiteral("missingContext");
    missingContext.required = true;
    plan.contextObjects.append(missingContext);

    lvrs::QmlViewModelBinding missingViewModel;
    missingViewModel.key = QStringLiteral("MissingVM");
    missingViewModel.required = true;
    plan.viewModels.append(missingViewModel);

    const lvrs::QmlContextBindResult result = lvrs::applyQmlContextBindPlan(engine, plan);
    QVERIFY(!result.ok);
    QVERIFY(result.errorMessage().contains(QStringLiteral("missingContext")));
    QVERIFY(result.errorMessage().contains(QStringLiteral("MissingVM")));
    QVERIFY(result.contextNames.isEmpty());
    QVERIFY(result.viewModelKeys.isEmpty());
}

QTEST_MAIN(NavigationStateTests)
#include "tst_navigation_state.moc"
