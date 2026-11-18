#include "QtEventWait.h"
#include <QGuiApplication>
#include <QScopeGuard>
#include <type_traits>

#include <QAbstractListModel>
#include <QAbstractTableModel>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
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

class EditableHierarchyRows : public QAbstractListModel
{
public:
    enum Roles {
        KeyRole = Qt::UserRole + 1,
        NameRole,
        DepthRole,
        ParentKeyRole,
        ParentItemKeyRole
    };

    explicit EditableHierarchyRows(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        m_rows.append({QStringLiteral("root"), QStringLiteral("Root"), 0, QString()});
        m_rows.append({QStringLiteral("branch"), QStringLiteral("Branch"), 1, QStringLiteral("root")});
        m_rows.append({QStringLiteral("leaf"), QStringLiteral("Leaf"), 2, QStringLiteral("branch")});
        m_rows.append({QStringLiteral("sibling"), QStringLiteral("Sibling"), 0, QString()});
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_rows.size();
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
            return {};

        const Row &row = m_rows.at(index.row());
        if (role == KeyRole)
            return row.key;
        if (role == NameRole || role == Qt::DisplayRole)
            return row.name;
        if (role == DepthRole)
            return row.depth;
        if (role == ParentKeyRole || role == ParentItemKeyRole)
            return row.parentKey;

        return {};
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
            return false;

        Row &row = m_rows[index.row()];
        if (role == DepthRole) {
            row.depth = qMax(0, value.toInt());
        } else if (role == ParentKeyRole || role == ParentItemKeyRole) {
            row.parentKey = value.toString();
        } else {
            return false;
        }

        emit dataChanged(index, index, {role});
        return true;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return Qt::ItemIsDropEnabled;
        return QAbstractListModel::flags(index)
            | Qt::ItemIsEditable
            | Qt::ItemIsDragEnabled
            | Qt::ItemIsDropEnabled;
    }

    bool moveRows(const QModelIndex &sourceParent,
                  int sourceRow,
                  int count,
                  const QModelIndex &destinationParent,
                  int destinationChild) override
    {
        if (sourceParent.isValid() || destinationParent.isValid())
            return false;
        if (sourceRow < 0 || count <= 0 || sourceRow + count > m_rows.size())
            return false;
        if (destinationChild < 0 || destinationChild > m_rows.size())
            return false;
        if (destinationChild >= sourceRow && destinationChild <= sourceRow + count)
            return false;

        beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild);
        QVector<Row> moved;
        moved.reserve(count);
        for (int offset = 0; offset < count; ++offset)
            moved.append(m_rows.at(sourceRow + offset));
        for (int offset = 0; offset < count; ++offset)
            m_rows.removeAt(sourceRow);

        int insertionRow = destinationChild;
        if (destinationChild > sourceRow)
            insertionRow -= count;
        for (int offset = 0; offset < moved.size(); ++offset)
            m_rows.insert(insertionRow + offset, moved.at(offset));
        endMoveRows();
        return true;
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {KeyRole, QByteArrayLiteral("key")},
            {NameRole, QByteArrayLiteral("name")},
            {DepthRole, QByteArrayLiteral("depth")},
            {ParentKeyRole, QByteArrayLiteral("parentKey")},
            {ParentItemKeyRole, QByteArrayLiteral("parentItemKey")},
        };
    }

    QString keyAt(int index) const
    {
        return index >= 0 && index < m_rows.size() ? m_rows.at(index).key : QString();
    }

    int depthAt(int index) const
    {
        return index >= 0 && index < m_rows.size() ? m_rows.at(index).depth : -1;
    }

    QString parentKeyAt(int index) const
    {
        return index >= 0 && index < m_rows.size() ? m_rows.at(index).parentKey : QString();
    }

private:
    struct Row
    {
        QString key;
        QString name;
        int depth = 0;
        QString parentKey;
    };

    QVector<Row> m_rows;
};

class EditableTableRows : public QAbstractTableModel
{
public:
    explicit EditableTableRows(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
        m_rows.append({QStringLiteral("Renderer"), 3});
        m_rows.append({QStringLiteral("Writer"), 2});
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_rows.size();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : 2;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
            return {};
        if (index.column() < 0 || index.column() >= 2)
            return {};

        const Row &row = m_rows.at(index.row());
        const QVariant value = index.column() == 0 ? QVariant(row.name) : QVariant(row.count);
        if (role == Qt::DisplayRole || role == Qt::EditRole)
            return value;
        return {};
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override
    {
        if (role != Qt::EditRole || !index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
            return false;
        if (index.column() < 0 || index.column() >= 2)
            return false;

        Row &row = m_rows[index.row()];
        if (index.column() == 0) {
            row.name = value.toString();
        } else {
            bool ok = false;
            const int parsed = value.toInt(&ok);
            if (!ok)
                return false;
            row.count = parsed;
        }

        emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole});
        return true;
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return {};
        if (section == 0)
            return QStringLiteral("Name");
        if (section == 1)
            return QStringLiteral("Count");
        return {};
    }

    QVariant valueAt(int row, int column) const
    {
        if (row < 0 || row >= m_rows.size())
            return {};
        if (column == 0)
            return m_rows.at(row).name;
        if (column == 1)
            return m_rows.at(row).count;
        return {};
    }

private:
    struct Row
    {
        QString name;
        int count = 0;
    };

    QVector<Row> m_rows;
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

namespace {
template <typename Actual, typename Expected>
bool navigationEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual) + 1.0, qreal(expected) + 1.0);
    else
        return actual == expected;
}
}
#define NAVIGATION_COMPARE(actual, expected) CTEST_CHECK(navigationEqual((actual), (expected)))

class NavigationStateTests
{
public:
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
    void table_projection_matches_public_cell_queries();
    void table_viewport_projection_preserves_merged_anchors_and_bounds();
    void table_column_count_tracks_headers_structure_and_undo();
    void qml_context_bind_plan_exposes_context_objects_and_viewmodels();
    void qml_context_bind_plan_reports_missing_required_objects();
};

void NavigationStateTests::table_projection_matches_public_cell_queries()
{
    TableModel model;
    QVariantList rows;
    for (int row=0;row<12;++row) {
        QVariantList cells;
        for (int column=0;column<6;++column) cells.append(QVariantMap{{"value",QString::number(row*6+column)}});
        if (row==0 || row==3) cells[1]=QVariantMap{{"value","span"},{"rowSpan",4},{"columnSpan",3}};
        if (row==1) cells[2]=QVariantMap{{"value","overlap"},{"rowSpan",5},{"columnSpan",3}};
        if (row==2) cells[2]=QVariantMap{{"_lvrsMerged",true},{"_lvrsMergeAnchorRow",2},{"_lvrsMergeAnchorColumn",2}};
        if (row==8) cells[0]=QVariantMap{{"_lvrsMerged",true},{"_lvrsMergeAnchorRow",7},{"_lvrsMergeAnchorColumn",0}};
        rows.append(QVariant(cells));
    }
    model.setRows(rows);model.setColumnWidths(QVariantList{41,55,79,81,120,160});
    model.setRowHeights(QVariantList{25,29,35,47});
    const auto actual=model.visibleCells();
    qsizetype i=0;
    for (int row=0;row<model.rowCount();++row) for (int column=0;column<6;++column) {
        if (model.isCoveredCell(row,column)) continue;
        CTEST_CHECK(i<actual.size());
        const auto value=actual[i++].toMap();
        NAVIGATION_COMPARE(value["rowIndex"].toInt(),row);NAVIGATION_COMPARE(value["columnIndex"].toInt(),column);
        NAVIGATION_COMPARE(value["cellData"],model.cellAt(row,column));NAVIGATION_COMPARE(value["text"].toString(),model.cellText(row,column));
        NAVIGATION_COMPARE(value["x"].toInt(),model.columnX(column));NAVIGATION_COMPARE(value["y"].toInt(),model.rowY(row));
        const auto rs=model.cellRowSpan(row,column), cs=model.cellColumnSpan(row,column);
        NAVIGATION_COMPARE(value["rowSpan"].toInt(),rs);NAVIGATION_COMPARE(value["columnSpan"].toInt(),cs);
        NAVIGATION_COMPARE(value["width"].toInt(),model.columnSpanWidth(column,cs));
        NAVIGATION_COMPARE(value["height"].toInt(),model.rowSpanHeight(row,rs));
    }
    NAVIGATION_COMPARE(i,actual.size());
    QVector<int> visiblePerRow(model.rowCount());
    for (const auto &cell : actual)
        ++visiblePerRow[cell.toMap().value("rowIndex").toInt()];
    NAVIGATION_COMPARE(visiblePerRow[6], 3); // The last active span still covers columns 1–3.
    NAVIGATION_COMPARE(visiblePerRow[7], 6); // Its end row immediately exposes every column.
    NAVIGATION_COMPARE(visiblePerRow[8], 5); // An explicit covered marker remains hidden.
    model.setRows(QVariantList{});CTEST_CHECK(model.visibleCells().isEmpty());
}

void NavigationStateTests::table_viewport_projection_preserves_merged_anchors_and_bounds()
{
    TableModel model;
    QVariantList rows;
    for (int row = 0; row < 10000; ++row)
        rows.append(QVariant(QVariantList{QString::number(row), "date", "kind", "size"}));
    model.setRows(rows);
    model.setDefaultRowHeight(24);
    const auto window = model.visibleCellsInRange(2400, 2640);
    NAVIGATION_COMPARE(window.size(), 40);
    NAVIGATION_COMPARE(window.first().toMap().value("rowIndex").toInt(), 100);
    NAVIGATION_COMPARE(window.last().toMap().value("rowIndex").toInt(), 109);
    NAVIGATION_COMPARE(window.first().toMap().value("y").toInt(), 2400);
    NAVIGATION_COMPARE(model.totalBodyHeight(), 240000);
    CTEST_CHECK(model.visibleCellsInRange(240000, 240100).isEmpty());
    CTEST_CHECK(model.visibleCellsInRange(20, 10).isEmpty());
    // An anchor above the viewport must remain present until its last covered row.
    model.setRows(QVariantList{
        QVariant(QVariantList{QVariantMap{{"value", "span"}, {"rowSpan", 3}}, "a"}),
        QVariant(QVariantList{"covered", "b"}),
        QVariant(QVariantList{"covered", "c"}),
        QVariant(QVariantList{"plain", "d"})});
    model.setRowHeights(QVariantList{24, 30, 40, 24});
    const auto full = model.visibleCells();
    for (const auto bounds : {QPair<double, double>{24, 54}, {54, 94}, {94, 118}}) {
        QVariantList expected;
        for (const auto &cell : full) {
            const auto map = cell.toMap();
            if (map.value("y").toDouble() < bounds.second
                && map.value("y").toDouble() + map.value("height").toDouble() > bounds.first)
                expected.append(cell);
        }
        NAVIGATION_COMPARE(model.visibleCellsInRange(bounds.first, bounds.second), expected);
    }
}

void NavigationStateTests::table_column_count_tracks_headers_structure_and_undo()
{
    TableModel model;
    model.setHeaderColumns(QVariantList{"Name", "Size"});
    model.setRows(QVariantList{QVariant(QVariantList{"a"}), QVariant(QVariantList{"b", 1, true})});
    NAVIGATION_COMPARE(model.columnCount(), 3);
    NAVIGATION_COMPARE(model.columnCount(), 3);
    model.setRows(QVariantList{QVariant(QVariantList{"a", 1})});
    NAVIGATION_COMPARE(model.columnCount(), 2);
    CTEST_CHECK(model.insertColumn(1)); NAVIGATION_COMPARE(model.columnCount(), 3);
    CTEST_CHECK(model.undo()); NAVIGATION_COMPARE(model.columnCount(), 2);
    CTEST_CHECK(model.redo()); NAVIGATION_COMPARE(model.columnCount(), 3);
    model.setHeaderColumns(QVariantList{"a", "b", "c", "d"});
    NAVIGATION_COMPARE(model.columnCount(), 4);
}

void NavigationStateTests::page_monitor_history_metrics()
{
    PageMonitor monitor;
    NAVIGATION_COMPARE(monitor.count(), 0);
    NAVIGATION_COMPARE(monitor.current(), QString());
    CTEST_CHECK(!monitor.canUndo());

    monitor.record(QStringLiteral("/overview"));
    monitor.record(QStringLiteral("/overview"));
    NAVIGATION_COMPARE(monitor.count(), 1);
    NAVIGATION_COMPARE(monitor.current(), QStringLiteral("/overview"));

    monitor.record(QStringLiteral("/reports"));
    NAVIGATION_COMPARE(monitor.count(), 2);
    CTEST_CHECK(monitor.canUndo());
    NAVIGATION_COMPARE(monitor.undo(), QStringLiteral("/overview"));
    NAVIGATION_COMPARE(monitor.count(), 1);
    CTEST_CHECK(!monitor.canUndo());

    monitor.clear();
    NAVIGATION_COMPARE(monitor.count(), 0);
    NAVIGATION_COMPARE(monitor.current(), QString());
}

void NavigationStateTests::page_monitor_signal_contract_and_normalization()
{
    PageMonitor monitor;
    QList<QVariantList> historySpyEvents;
    const auto historySpyConnection = QObject::connect(&monitor, &PageMonitor::historyChanged,
        [&historySpyEvents]() { historySpyEvents.append(QVariantList{}); });
    const auto historySpyDisconnect = qScopeGuard([&] { QObject::disconnect(historySpyConnection); });
    CTEST_CHECK(bool(historySpyConnection));

    monitor.record(QStringLiteral("  /overview  "));
    NAVIGATION_COMPARE(monitor.current(), QStringLiteral("/overview"));
    NAVIGATION_COMPARE(historySpyEvents.size(), 1);

    monitor.record(QStringLiteral("/overview"));
    monitor.record(QStringLiteral("   "));
    NAVIGATION_COMPARE(historySpyEvents.size(), 1);

    NAVIGATION_COMPARE(monitor.undo(), QStringLiteral("/overview"));
    NAVIGATION_COMPARE(historySpyEvents.size(), 1);

    monitor.record(QStringLiteral("/details"));
    NAVIGATION_COMPARE(historySpyEvents.size(), 2);
    NAVIGATION_COMPARE(monitor.undo(), QStringLiteral("/overview"));
    NAVIGATION_COMPARE(historySpyEvents.size(), 3);

    monitor.clear();
    NAVIGATION_COMPARE(historySpyEvents.size(), 4);
    monitor.clear();
    NAVIGATION_COMPARE(historySpyEvents.size(), 4);
}

void NavigationStateTests::view_state_tracker_syncs_stack_and_status()
{
    ViewStateTracker tracker;
    QList<QVariantList> stackSpyEvents;
    const auto stackSpyConnection = QObject::connect(&tracker, &ViewStateTracker::stackChanged,
        [&stackSpyEvents]() { stackSpyEvents.append(QVariantList{}); });
    const auto stackSpyDisconnect = qScopeGuard([&] { QObject::disconnect(stackSpyConnection); });
    CTEST_CHECK(bool(stackSpyConnection));

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
    NAVIGATION_COMPARE(stackSpyEvents.size(), 1);
    NAVIGATION_COMPARE(tracker.loadedCount(), 3);
    NAVIGATION_COMPARE(tracker.loadedViews(),
             (QStringList { QStringLiteral("/overview"), QStringLiteral("/reports"), QStringLiteral("/settings") }));
    NAVIGATION_COMPARE(tracker.activeViews(), (QStringList { QStringLiteral("/reports") }));
    NAVIGATION_COMPARE(tracker.inactiveViews(), (QStringList { QStringLiteral("/overview") }));
    NAVIGATION_COMPARE(tracker.disabledViews(), (QStringList { QStringLiteral("/settings") }));
    NAVIGATION_COMPARE(tracker.currentActiveView(), QStringLiteral("/reports"));

    NAVIGATION_COMPARE(tracker.stateOf(QStringLiteral("/overview")), QStringLiteral("Inactive"));
    NAVIGATION_COMPARE(tracker.stateOf(QStringLiteral("/reports")), QStringLiteral("Active"));
    NAVIGATION_COMPARE(tracker.stateOf(QStringLiteral("/settings")), QStringLiteral("Disabled"));

    const QVariantMap reportView = tracker.view(QStringLiteral("/reports"));
    NAVIGATION_COMPARE(reportView.value(QStringLiteral("state")).toString(), QStringLiteral("Active"));
    NAVIGATION_COMPARE(reportView.value(QStringLiteral("index")).toInt(), 1);
    NAVIGATION_COMPARE(reportView.value(QStringLiteral("active")).toBool(), true);

    tracker.syncStack(entries);
    NAVIGATION_COMPARE(stackSpyEvents.size(), 1);
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

    NAVIGATION_COMPARE(tracker.currentActiveView(), QStringLiteral("/c"));

    QList<QVariantList> stackSpyEvents;
    const auto stackSpyConnection = QObject::connect(&tracker, &ViewStateTracker::stackChanged,
        [&stackSpyEvents]() { stackSpyEvents.append(QVariantList{}); });
    const auto stackSpyDisconnect = qScopeGuard([&] { QObject::disconnect(stackSpyConnection); });
    CTEST_CHECK(bool(stackSpyConnection));

    tracker.setViewDisabled(QStringLiteral("/c"), true);
    NAVIGATION_COMPARE(tracker.currentActiveView(), QStringLiteral("/b"));
    NAVIGATION_COMPARE(tracker.stateOf(QStringLiteral("/c")), QStringLiteral("Disabled"));
    NAVIGATION_COMPARE(tracker.activeViews(), (QStringList { QStringLiteral("/b") }));

    tracker.setViewEnabled(QStringLiteral("/c"), true);
    NAVIGATION_COMPARE(tracker.currentActiveView(), QStringLiteral("/c"));
    NAVIGATION_COMPARE(tracker.stateOf(QStringLiteral("/c")), QStringLiteral("Active"));

    tracker.clear();
    NAVIGATION_COMPARE(tracker.loadedCount(), 0);
    NAVIGATION_COMPARE(tracker.currentActiveView(), QString());
    CTEST_CHECK(stackSpyEvents.size() >= 3);
}

void NavigationStateTests::route_matcher_normalization_and_param_contract()
{
    RouteMatcher matcher;

    NAVIGATION_COMPARE(matcher.normalizePath(QString()), QStringLiteral("/"));
    NAVIGATION_COMPARE(matcher.normalizePath(QStringLiteral("reports/")), QStringLiteral("/reports"));
    NAVIGATION_COMPARE(matcher.normalizePath(QStringLiteral("/runs/123/")), QStringLiteral("/runs/123"));

    const QVariantMap rootMatch = matcher.match(QStringLiteral("/"), QStringLiteral("/"));
    CTEST_CHECK(rootMatch.value(QStringLiteral("matched")).toBool());
    CTEST_CHECK(rootMatch.value(QStringLiteral("params")).toMap().isEmpty());

    const QVariantMap paramMatch = matcher.match(QStringLiteral("/runs/42"), QStringLiteral("/runs/[id]"));
    CTEST_CHECK(paramMatch.value(QStringLiteral("matched")).toBool());
    NAVIGATION_COMPARE(paramMatch.value(QStringLiteral("params")).toMap().value(QStringLiteral("id")).toString(),
             QStringLiteral("42"));

    const QVariantMap restMatch = matcher.match(QStringLiteral("/logs/a/b/c"), QStringLiteral("/logs/[...path]"));
    CTEST_CHECK(restMatch.value(QStringLiteral("matched")).toBool());
    NAVIGATION_COMPARE(restMatch.value(QStringLiteral("params")).toMap().value(QStringLiteral("path")).toString(),
             QStringLiteral("a/b/c"));

    const QVariantMap noMatch = matcher.match(QStringLiteral("/runs"), QStringLiteral("/runs/[id]"));
    CTEST_CHECK(!noMatch.value(QStringLiteral("matched")).toBool());
}

void NavigationStateTests::viewmodels_registry_binding_ownership_and_write_permissions()
{
    ViewModelRegistry registry;
    auto *model = new MutableStatusModel;
    registry.set(QStringLiteral("Example"), model);

    QList<QVariantList> viewsSpyEvents;
    const auto viewsSpyConnection = QObject::connect(&registry, &ViewModelRegistry::viewsChanged,
        [&viewsSpyEvents]() { viewsSpyEvents.append(QVariantList{}); });
    const auto viewsSpyDisconnect = qScopeGuard([&] { QObject::disconnect(viewsSpyConnection); });
    QList<QVariantList> ownershipSpyEvents;
    const auto ownershipSpyConnection = QObject::connect(&registry, &ViewModelRegistry::ownershipChanged,
        [&ownershipSpyEvents]() { ownershipSpyEvents.append(QVariantList{}); });
    const auto ownershipSpyDisconnect = qScopeGuard([&] { QObject::disconnect(ownershipSpyConnection); });
    QList<QVariantList> errorSpyEvents;
    const auto errorSpyConnection = QObject::connect(&registry, &ViewModelRegistry::lastErrorChanged,
        [&errorSpyEvents]() { errorSpyEvents.append(QVariantList{}); });
    const auto errorSpyDisconnect = qScopeGuard([&] { QObject::disconnect(errorSpyConnection); });
    CTEST_CHECK(bool(viewsSpyConnection));
    CTEST_CHECK(bool(ownershipSpyConnection));
    CTEST_CHECK(bool(errorSpyConnection));

    CTEST_CHECK(registry.bindView(QStringLiteral("OverviewView"), QStringLiteral("Example"), true));
    NAVIGATION_COMPARE(registry.keyForView(QStringLiteral("OverviewView")), QStringLiteral("Example"));
    CTEST_CHECK(registry.getForView(QStringLiteral("OverviewView")) == model);
    NAVIGATION_COMPARE(registry.ownerOf(QStringLiteral("Example")), QStringLiteral("OverviewView"));
    CTEST_CHECK(registry.canWrite(QStringLiteral("OverviewView")));

    CTEST_CHECK(registry.bindView(QStringLiteral("ReportsView"), QStringLiteral("Example"), false));
    NAVIGATION_COMPARE(registry.keyForView(QStringLiteral("ReportsView")), QStringLiteral("Example"));
    CTEST_CHECK(!registry.canWrite(QStringLiteral("ReportsView")));

    CTEST_CHECK(!registry.updateProperty(QStringLiteral("ReportsView"),
                                     QStringLiteral("status"),
                                     QStringLiteral("Working")));
    NAVIGATION_COMPARE(registry.lastError(), QStringLiteral("View has no write permission for the model"));
    NAVIGATION_COMPARE(model->status(), QStringLiteral("Idle"));

    CTEST_CHECK(registry.updateProperty(QStringLiteral("OverviewView"),
                                    QStringLiteral("status"),
                                    QStringLiteral("Working")));
    NAVIGATION_COMPARE(model->status(), QStringLiteral("Working"));
    NAVIGATION_COMPARE(registry.readProperty(QStringLiteral("OverviewView"), QStringLiteral("status")).toString(),
             QStringLiteral("Working"));

    CTEST_CHECK(!registry.bindView(QStringLiteral("ReportsView"), QStringLiteral("Example"), true));
    NAVIGATION_COMPARE(registry.lastError(), QStringLiteral("ViewModel is already owned by another view"));

    CTEST_CHECK(registry.releaseOwnership(QStringLiteral("OverviewView"), QStringLiteral("Example")));
    NAVIGATION_COMPARE(registry.ownerOf(QStringLiteral("Example")), QString());
    CTEST_CHECK(registry.claimOwnership(QStringLiteral("ReportsView"), QStringLiteral("Example")));
    NAVIGATION_COMPARE(registry.ownerOf(QStringLiteral("Example")), QStringLiteral("ReportsView"));
    CTEST_CHECK(registry.canWrite(QStringLiteral("ReportsView"), QStringLiteral("Example")));

    CTEST_CHECK(registry.updateProperty(QStringLiteral("ReportsView"),
                                    QStringLiteral("status"),
                                    QStringLiteral("Ready")));
    NAVIGATION_COMPARE(model->status(), QStringLiteral("Ready"));

    registry.unbindView(QStringLiteral("ReportsView"));
    NAVIGATION_COMPARE(registry.keyForView(QStringLiteral("ReportsView")), QString());
    NAVIGATION_COMPARE(registry.ownerOf(QStringLiteral("Example")), QString());
    CTEST_CHECK(!registry.canWrite(QStringLiteral("ReportsView")));

    NAVIGATION_COMPARE(registry.bindings().value(QStringLiteral("OverviewView")).toString(),
             QStringLiteral("Example"));
    CTEST_CHECK(viewsSpyEvents.size() >= 3);
    CTEST_CHECK(ownershipSpyEvents.size() >= 3);
    CTEST_CHECK(errorSpyEvents.size() >= 1);

    registry.remove(QStringLiteral("Example"));
    NAVIGATION_COMPARE(registry.keyForView(QStringLiteral("OverviewView")), QString());
}

void NavigationStateTests::viewmodels_registry_rebinding_clears_stale_ownership()
{
    ViewModelRegistry registry;
    auto *modelA = new MutableStatusModel;
    auto *modelB = new MutableStatusModel;

    registry.set(QStringLiteral("ModelA"), modelA);
    registry.set(QStringLiteral("ModelB"), modelB);

    CTEST_CHECK(registry.bindView(QStringLiteral("EditorView"), QStringLiteral("ModelA"), true));
    NAVIGATION_COMPARE(registry.ownerOf(QStringLiteral("ModelA")), QStringLiteral("EditorView"));
    NAVIGATION_COMPARE(registry.ownerOf(QStringLiteral("ModelB")), QString());
    CTEST_CHECK(registry.canWrite(QStringLiteral("EditorView"), QStringLiteral("ModelA")));

    CTEST_CHECK(registry.bindView(QStringLiteral("EditorView"), QStringLiteral("ModelB"), true));
    NAVIGATION_COMPARE(registry.ownerOf(QStringLiteral("ModelA")), QString());
    NAVIGATION_COMPARE(registry.ownerOf(QStringLiteral("ModelB")), QStringLiteral("EditorView"));
    CTEST_CHECK(!registry.canWrite(QStringLiteral("EditorView"), QStringLiteral("ModelA")));
    CTEST_CHECK(registry.canWrite(QStringLiteral("EditorView"), QStringLiteral("ModelB")));

    CTEST_CHECK(registry.bindView(QStringLiteral("EditorView"), QStringLiteral("ModelB"), false));
    NAVIGATION_COMPARE(registry.ownerOf(QStringLiteral("ModelB")), QString());
    CTEST_CHECK(!registry.canWrite(QStringLiteral("EditorView"), QStringLiteral("ModelB")));
}

void NavigationStateTests::viewmodels_registry_tracks_keys_and_ownership()
{
    ViewModelRegistry registry;
    NAVIGATION_COMPARE(registry.keys().size(), 0);
    CTEST_CHECK(registry.get(QStringLiteral("missing")) == nullptr);

    auto *shared = new QObject;
    QPointer<QObject> sharedGuard(shared);
    registry.set(QStringLiteral("alpha"), shared);
    registry.set(QStringLiteral("beta"), shared);
    CTEST_CHECK(registry.keys().contains(QStringLiteral("alpha")));
    CTEST_CHECK(registry.keys().contains(QStringLiteral("beta")));
    CTEST_CHECK(registry.get(QStringLiteral("alpha")) == shared);
    CTEST_CHECK(shared->parent() == &registry);

    registry.remove(QStringLiteral("alpha"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    CTEST_CHECK(!sharedGuard.isNull());

    registry.remove(QStringLiteral("beta"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    CTEST_WAIT_DEFAULT(sharedGuard.isNull());

    auto *single = new QObject;
    QPointer<QObject> singleGuard(single);
    registry.set(QStringLiteral("single"), single);
    NAVIGATION_COMPARE(registry.keys().size(), 1);
    registry.clear();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    CTEST_WAIT_DEFAULT(singleGuard.isNull());
    NAVIGATION_COMPARE(registry.keys().size(), 0);
}

void NavigationStateTests::viewmodels_registry_signal_and_prune_contract()
{
    ViewModelRegistry registry;
    QList<QVariantList> keysSpyEvents;
    const auto keysSpyConnection = QObject::connect(&registry, &ViewModelRegistry::keysChanged,
        [&keysSpyEvents]() { keysSpyEvents.append(QVariantList{}); });
    const auto keysSpyDisconnect = qScopeGuard([&] { QObject::disconnect(keysSpyConnection); });
    CTEST_CHECK(bool(keysSpyConnection));

    auto *ignored = new QObject;
    QPointer<QObject> ignoredGuard(ignored);
    registry.set(QStringLiteral("   "), ignored);
    NAVIGATION_COMPARE(keysSpyEvents.size(), 0);
    CTEST_CHECK(!ignoredGuard.isNull());
    delete ignored;
    CTEST_WAIT_DEFAULT(ignoredGuard.isNull());

    QObject externalParent;
    auto *externalOwned = new QObject(&externalParent);
    registry.set(QStringLiteral("primary"), externalOwned);
    NAVIGATION_COMPARE(keysSpyEvents.size(), 1);
    NAVIGATION_COMPARE(externalOwned->parent(), &externalParent);
    registry.set(QStringLiteral("primary"), externalOwned);
    NAVIGATION_COMPARE(keysSpyEvents.size(), 1);
    registry.set(QStringLiteral("secondary"), externalOwned);
    NAVIGATION_COMPARE(keysSpyEvents.size(), 2);

    registry.remove(QStringLiteral("missing"));
    NAVIGATION_COMPARE(keysSpyEvents.size(), 2);
    registry.remove(QStringLiteral("primary"));
    NAVIGATION_COMPARE(keysSpyEvents.size(), 3);
    CTEST_CHECK(!externalOwned->parent()->inherits("ViewModelRegistry"));

    QPointer<QObject> externalGuard(externalOwned);
    delete externalOwned;
    CTEST_WAIT_DEFAULT(externalGuard.isNull());

    auto *owned = new QObject;
    QPointer<QObject> ownedGuard(owned);
    registry.set(QStringLiteral("owned"), owned);
    NAVIGATION_COMPARE(keysSpyEvents.size(), 5);
    CTEST_CHECK(owned->parent() == &registry);
    registry.set(QStringLiteral("owned"), owned);
    NAVIGATION_COMPARE(keysSpyEvents.size(), 5);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    CTEST_CHECK(!ownedGuard.isNull());

    auto *trigger = new QObject;
    registry.set(QStringLiteral("trigger"), trigger);
    NAVIGATION_COMPARE(keysSpyEvents.size(), 6);
    CTEST_CHECK(!registry.keys().contains(QStringLiteral("secondary")));

    registry.remove(QStringLiteral("owned"));
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::processEvents();
    CTEST_WAIT_DEFAULT(ownedGuard.isNull());

    const int beforeClear = keysSpyEvents.size();
    registry.clear();
    NAVIGATION_COMPARE(keysSpyEvents.size(), beforeClear + 1);
    registry.clear();
    NAVIGATION_COMPARE(registry.keys().size(), 0);
}

void NavigationStateTests::viewmodel_base_and_registry_descriptors_track_cpp_state()
{
    ViewModelRegistry registry;
    auto *viewModel = new DedicatedStatusViewModel;
    viewModel->setKey(QStringLiteral("StatusVM"));
    viewModel->setDisplayName(QStringLiteral("Status"));
    viewModel->setMetadata({{QStringLiteral("domain"), QStringLiteral("navigation-test")}});

    QList<QVariantList> descriptorSpyEvents;
    const auto descriptorSpyConnection = QObject::connect(&registry, &ViewModelRegistry::descriptorsChanged,
        [&descriptorSpyEvents]() { descriptorSpyEvents.append(QVariantList{}); });
    const auto descriptorSpyDisconnect = qScopeGuard([&] { QObject::disconnect(descriptorSpyConnection); });
    CTEST_CHECK(bool(descriptorSpyConnection));

    CTEST_CHECK(registry.registerViewModel(viewModel));
    NAVIGATION_COMPARE(registry.get(QStringLiteral("StatusVM")), viewModel);

    QVariantMap descriptor = registry.descriptor(QStringLiteral("StatusVM"));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("viewModel")).toBool(), true);
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("viewModelKey")).toString(), QStringLiteral("StatusVM"));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("displayName")).toString(), QStringLiteral("Status"));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("busy")).toBool(), false);
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("hasError")).toBool(), false);
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("metadata")).toMap().value(QStringLiteral("domain")).toString(),
             QStringLiteral("navigation-test"));

    viewModel->setBusy(true);
    viewModel->setError(QStringLiteral("loading failed"));

    descriptor = registry.descriptor(QStringLiteral("StatusVM"));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("busy")).toBool(), true);
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("error")).toString(), QStringLiteral("loading failed"));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("hasError")).toBool(), true);
    CTEST_CHECK(descriptorSpyEvents.size() >= 3);

    const int beforeBindSignals = descriptorSpyEvents.size();
    CTEST_CHECK(registry.bindView(QStringLiteral("StatusView"), QStringLiteral("StatusVM"), true));
    CTEST_CHECK(descriptorSpyEvents.size() > beforeBindSignals);

    descriptor = registry.descriptor(QStringLiteral("StatusVM"));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("owner")).toString(), QStringLiteral("StatusView"));
    CTEST_CHECK(descriptor.value(QStringLiteral("views")).toList().contains(QStringLiteral("StatusView")));

    QVariantMap snapshot = viewModel->snapshot();
    NAVIGATION_COMPARE(snapshot.value(QStringLiteral("key")).toString(), QStringLiteral("StatusVM"));
    NAVIGATION_COMPARE(snapshot.value(QStringLiteral("displayName")).toString(), QStringLiteral("Status"));
    NAVIGATION_COMPARE(snapshot.value(QStringLiteral("busy")).toBool(), true);
    NAVIGATION_COMPARE(snapshot.value(QStringLiteral("hasError")).toBool(), true);

    viewModel->clearError();
    NAVIGATION_COMPARE(viewModel->hasError(), false);
}

void NavigationStateTests::state_model_stores_state_and_registry_descriptors_track_values()
{
    StateModel model;
    model.setKey(QStringLiteral("ProgressState"));
    model.setDisplayName(QStringLiteral("Progress State"));

    QList<QVariantList> valuesSpyEvents;
    const auto valuesSpyConnection = QObject::connect(&model, &StateModel::valuesChanged,
        [&valuesSpyEvents]() { valuesSpyEvents.append(QVariantList{}); });
    const auto valuesSpyDisconnect = qScopeGuard([&] { QObject::disconnect(valuesSpyConnection); });
    QList<QVariantList> revisionSpyEvents;
    const auto revisionSpyConnection = QObject::connect(&model, &StateModel::revisionChanged,
        [&revisionSpyEvents]() { revisionSpyEvents.append(QVariantList{}); });
    const auto revisionSpyDisconnect = qScopeGuard([&] { QObject::disconnect(revisionSpyConnection); });
    QList<QVariantList> valueSpyEvents;
    const auto valueSpyConnection = QObject::connect(&model, &StateModel::valueChanged,
        [&valueSpyEvents](const QString &key, const QVariant &value, const QVariant &previousValue) { valueSpyEvents.append(QVariantList{key, value, previousValue}); });
    const auto valueSpyDisconnect = qScopeGuard([&] { QObject::disconnect(valueSpyConnection); });
    CTEST_CHECK(bool(valuesSpyConnection));
    CTEST_CHECK(bool(revisionSpyConnection));
    CTEST_CHECK(bool(valueSpyConnection));

    CTEST_CHECK(model.setValue(QStringLiteral(" currentValue "), 40));
    CTEST_CHECK(model.setValue(QStringLiteral("minimumValue"), -10));
    CTEST_CHECK(model.setValue(QStringLiteral("maximumValue"), 90));
    NAVIGATION_COMPARE(model.value(QStringLiteral("currentValue")).toInt(), 40);
    NAVIGATION_COMPARE(model.value(QStringLiteral("missing"), QStringLiteral("fallback")).toString(),
             QStringLiteral("fallback"));
    CTEST_CHECK(model.hasValue(QStringLiteral("minimumValue")));
    NAVIGATION_COMPARE(model.stateKeys(),
             (QStringList {
                 QStringLiteral("currentValue"),
                 QStringLiteral("maximumValue"),
                 QStringLiteral("minimumValue")
             }));

    CTEST_CHECK(model.applyPatch(QVariantMap {
        {QStringLiteral("currentValue"), 64},
        {QStringLiteral("startValue"), 10}
    }));
    NAVIGATION_COMPARE(model.value(QStringLiteral("currentValue")).toInt(), 64);
    NAVIGATION_COMPARE(model.value(QStringLiteral("startValue")).toInt(), 10);

    const QVariantMap snapshot = model.stateSnapshot();
    NAVIGATION_COMPARE(snapshot.value(QStringLiteral("key")).toString(), QStringLiteral("ProgressState"));
    NAVIGATION_COMPARE(snapshot.value(QStringLiteral("displayName")).toString(), QStringLiteral("Progress State"));
    NAVIGATION_COMPARE(snapshot.value(QStringLiteral("values")).toMap().value(QStringLiteral("currentValue")).toInt(), 64);
    CTEST_CHECK(snapshot.value(QStringLiteral("revision")).toInt() >= 4);
    CTEST_CHECK(!snapshot.value(QStringLiteral("empty")).toBool());

    CTEST_CHECK(model.removeValue(QStringLiteral("minimumValue")));
    CTEST_CHECK(!model.hasValue(QStringLiteral("minimumValue")));
    CTEST_CHECK(!model.setValue(QStringLiteral(" "), 1));
    NAVIGATION_COMPARE(model.error(), QStringLiteral("Empty state key"));
    model.clearValues();
    CTEST_CHECK(model.empty());
    CTEST_CHECK(valuesSpyEvents.size() >= 5);
    CTEST_CHECK(revisionSpyEvents.size() >= 5);
    CTEST_CHECK(valueSpyEvents.size() >= 5);

    ViewModelRegistry registry;
    auto *registered = new StateModel;
    registered->setKey(QStringLiteral("PanelState"));
    CTEST_CHECK(registered->setValue(QStringLiteral("activeKey"), QStringLiteral("row-a")));

    QList<QVariantList> descriptorSpyEvents;
    const auto descriptorSpyConnection = QObject::connect(&registry, &ViewModelRegistry::descriptorsChanged,
        [&descriptorSpyEvents]() { descriptorSpyEvents.append(QVariantList{}); });
    const auto descriptorSpyDisconnect = qScopeGuard([&] { QObject::disconnect(descriptorSpyConnection); });
    CTEST_CHECK(bool(descriptorSpyConnection));

    CTEST_CHECK(registry.registerViewModel(registered));
    QVariantMap descriptor = registry.descriptor(QStringLiteral("PanelState"));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("viewModel")).toBool(), true);
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("stateModel")).toBool(), true);
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("values")).toMap().value(QStringLiteral("activeKey")).toString(),
             QStringLiteral("row-a"));
    CTEST_CHECK(descriptor.value(QStringLiteral("stateKeys")).toStringList().contains(QStringLiteral("activeKey")));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("empty")).toBool(), false);

    const int beforeStateChange = descriptorSpyEvents.size();
    CTEST_CHECK(registered->setValue(QStringLiteral("activeKey"), QStringLiteral("row-b")));
    CTEST_CHECK(descriptorSpyEvents.size() > beforeStateChange);
    descriptor = registry.descriptor(QStringLiteral("PanelState"));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("values")).toMap().value(QStringLiteral("activeKey")).toString(),
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
    NAVIGATION_COMPARE(listSource.count(), 2);
    NAVIGATION_COMPARE(listSource.textValue(listSource.at(0), QStringList {QStringLiteral("label")}, QString()),
             QStringLiteral("Native A"));
    NAVIGATION_COMPARE(listSource.boolValue(listSource.at(1), QStringLiteral("enabled"), true), false);

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
    NAVIGATION_COMPARE(hierarchyModel.count(), 2);
    const QVariantMap leafDescriptor = hierarchyModel.descriptorAt(1);
    NAVIGATION_COMPARE(leafDescriptor.value(QStringLiteral("itemKey")).toString(), QStringLiteral("leaf"));
    NAVIGATION_COMPARE(leafDescriptor.value(QStringLiteral("label")).toString(), QStringLiteral("Leaf"));
    NAVIGATION_COMPARE(leafDescriptor.value(QStringLiteral("indentLevel")).toInt(), 1);
    NAVIGATION_COMPARE(leafDescriptor.value(QStringLiteral("count")).toInt(), 7);
    NAVIGATION_COMPARE(leafDescriptor.value(QStringLiteral("selected")).toBool(), true);
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
    NAVIGATION_COMPARE(interactionState.value(QStringLiteral("visibleItemCount")).toInt(), 2);
    NAVIGATION_COMPARE(interactionState.value(QStringLiteral("metadata")).toList().at(0).toMap().value(QStringLiteral("childCount")).toInt(), 1);
    NAVIGATION_COMPARE(interactionState.value(QStringLiteral("metadata")).toList().at(0).toMap().value(QStringLiteral("pathLabel")).toString(),
             QStringLiteral("Root"));
    const QVariantMap childMetadata = interactionState.value(QStringLiteral("metadata")).toList().at(1).toMap();
    NAVIGATION_COMPARE(childMetadata.value(QStringLiteral("ancestorItemKeys")).toList().at(0).toString(), QStringLiteral("root"));
    NAVIGATION_COMPARE(childMetadata.value(QStringLiteral("pathItemLabels")).toList().at(0).toString(), QStringLiteral("Root"));
    NAVIGATION_COMPARE(childMetadata.value(QStringLiteral("pathItemLabels")).toList().at(1).toString(), QStringLiteral("Child"));
    NAVIGATION_COMPARE(hierarchyModel.descendantRangeEnd(hierarchyInteractionRows, 0), 2);
    const QVariantMap dragTarget = hierarchyModel.resolveDragTarget(hierarchyInteractionRows, 1, 2, 3, 8, 8, 8);
    CTEST_CHECK(!dragTarget.isEmpty());
    CTEST_CHECK(dragTarget.value(QStringLiteral("depth")).toInt() >= 0);
    const QVariantMap moveResult = hierarchyModel.moveDescriptors(hierarchyInteractionRows, 1, 2, 0, 0);
    CTEST_CHECK(moveResult.value(QStringLiteral("accepted")).toBool());
    NAVIGATION_COMPARE(moveResult.value(QStringLiteral("reorderedDescriptors")).toList().size(), 3);

    EditableHierarchyRows editableRows;
    HierarchyModel editableHierarchyModel;
    editableHierarchyModel.setLabelRole(QStringLiteral("name"));
    editableHierarchyModel.setSource(QVariant::fromValue(static_cast<QObject *>(&editableRows)));
    NAVIGATION_COMPARE(editableHierarchyModel.count(), 4);
    CTEST_CHECK(editableHierarchyModel.sourceSupportsEditing());
    const QVariantMap sourceMoveResult = editableHierarchyModel.moveSourceRows(1, 2, 0, 0);
    CTEST_CHECK(sourceMoveResult.value(QStringLiteral("accepted")).toBool());
    CTEST_CHECK(sourceMoveResult.value(QStringLiteral("sourceWritten")).toBool());
    NAVIGATION_COMPARE(editableRows.keyAt(0), QStringLiteral("branch"));
    NAVIGATION_COMPARE(editableRows.depthAt(0), 0);
    NAVIGATION_COMPARE(editableRows.parentKeyAt(0), QString());
    NAVIGATION_COMPARE(editableRows.keyAt(1), QStringLiteral("leaf"));
    NAVIGATION_COMPARE(editableRows.depthAt(1), 1);
    NAVIGATION_COMPARE(editableRows.parentKeyAt(1), QStringLiteral("branch"));
    NAVIGATION_COMPARE(editableRows.keyAt(2), QStringLiteral("root"));
    NAVIGATION_COMPARE(editableHierarchyModel.descriptorAt(0).value(QStringLiteral("itemKey")).toString(), QStringLiteral("branch"));
    NAVIGATION_COMPARE(editableHierarchyModel.descriptorAt(1).value(QStringLiteral("indentLevel")).toInt(), 1);

    TableHeaderModel headerModel;
    headerModel.setTableWidth(300);
    headerModel.setMinColumnWidth(40);
    headerModel.setCellItems(QVariantList {
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Name")}, {QStringLiteral("type"), QStringLiteral("string")}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Count")}, {QStringLiteral("valueType"), QStringLiteral("int")}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Ratio")}, {QStringLiteral("cellType"), QStringLiteral("float")}}
    });
    headerModel.setColumnWidths(QVariantList {80, 100, 120});
    NAVIGATION_COMPARE(headerModel.columnCount(), 3);
    NAVIGATION_COMPARE(headerModel.columnType(1), QStringLiteral("int"));
    NAVIGATION_COMPARE(headerModel.columnX(2), 180);
    NAVIGATION_COMPARE(headerModel.descriptorAt(2).value(QStringLiteral("width")).toInt(), 120);
    TableHeaderModel emptyHeaderModel;
    emptyHeaderModel.setTableWidth(120);
    emptyHeaderModel.setCellItems(QVariantList {});
    NAVIGATION_COMPARE(emptyHeaderModel.columnCount(), 0);
    NAVIGATION_COMPARE(emptyHeaderModel.descriptorAt(0).value(QStringLiteral("text")).toString(), QStringLiteral("Column"));
    NAVIGATION_COMPARE(emptyHeaderModel.descriptorAt(0).value(QStringLiteral("width")).toInt(), 120);

    StateModel progressState;
    progressState.setValue(QStringLiteral("minimumValue"), 10);
    progressState.setValue(QStringLiteral("maximumValue"), 110);
    progressState.setValue(QStringLiteral("startValue"), 30);
    progressState.setValue(QStringLiteral("currentValue"), 80);
    ProgressModel progressModel;
    progressModel.setStateModel(&progressState);
    NAVIGATION_COMPARE(progressModel.effectiveMinimumValue(), 10.0);
    NAVIGATION_COMPARE(progressModel.effectiveMaximumValue(), 110.0);
    CTEST_CHECK(qAbs(progressModel.normalizedStart() - 0.2) < 0.001);
    CTEST_CHECK(qAbs(progressModel.normalizedCurrent() - 0.7) < 0.001);
    CTEST_CHECK(qAbs(progressModel.fillProgress() - 0.5) < 0.001);
    CTEST_CHECK(qFuzzyCompare(progressModel.radiusFor(1, 6, 100, 8), 4.0));

    NavigationStackModel navigationStack;
    navigationStack.applyPathOperation(QStringLiteral("/"), QVariantMap(), QStringLiteral("set"));
    navigationStack.applyPathOperation(QStringLiteral("reports"), QVariantMap {{QStringLiteral("page"), 2}}, QStringLiteral("push"));
    NAVIGATION_COMPARE(navigationStack.depth(), 2);
    NAVIGATION_COMPARE(navigationStack.currentPath(), QStringLiteral("/reports"));
    NAVIGATION_COMPARE(navigationStack.currentParams().toMap().value(QStringLiteral("page")).toInt(), 2);
    NAVIGATION_COMPARE(navigationStack.viewTrackingEntries().size(), 2);
    navigationStack.pop();
    NAVIGATION_COMPARE(navigationStack.depth(), 1);
    NAVIGATION_COMPARE(navigationStack.currentPath(), QStringLiteral("/"));

    VariantListObject readOnlyRows(QVariantList {
        QVariant(QVariantList {
            QVariantMap {{QStringLiteral("text"), QStringLiteral("Model Cell")}},
            QVariantMap {{QStringLiteral("text"), QStringLiteral("Second")}}
        })
    });
    NAVIGATION_COMPARE(readOnlyRows.count(), 1);
    TableModel readOnlyTableModel;
    readOnlyTableModel.setRows(QVariant::fromValue(static_cast<QObject *>(&readOnlyRows)));
    NAVIGATION_COMPARE(readOnlyTableModel.rowCount(), 1);
    NAVIGATION_COMPARE(readOnlyTableModel.cellText(0, 0), QStringLiteral("Model Cell"));
    CTEST_CHECK(!readOnlyTableModel.structureMutationAvailable());
    CTEST_CHECK(!readOnlyTableModel.insertRow(1));

    EditableTableRows editableTableRows;
    TableModel editableTableModel;
    editableTableModel.setHeaderCellItems(QVariantList {
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Name")}, {QStringLiteral("type"), QStringLiteral("string")}},
        QVariantMap {{QStringLiteral("label"), QStringLiteral("Count")}, {QStringLiteral("type"), QStringLiteral("int")}}
    });
    editableTableModel.setRows(QVariant::fromValue(static_cast<QObject *>(&editableTableRows)));
    NAVIGATION_COMPARE(editableTableModel.rowCount(), 2);
    NAVIGATION_COMPARE(editableTableModel.columnCount(), 2);
    CTEST_CHECK(editableTableModel.rowsModelBacked());
    CTEST_CHECK(editableTableModel.cellEditingAvailable());
    NAVIGATION_COMPARE(editableTableModel.cellText(0, 0), QStringLiteral("Renderer"));
    NAVIGATION_COMPARE(editableTableModel.cellText(0, 1), QStringLiteral("3"));
    CTEST_CHECK(editableTableModel.setCellValue(0, 1, QStringLiteral("42")));
    NAVIGATION_COMPARE(editableTableRows.valueAt(0, 1).toInt(), 42);
    NAVIGATION_COMPARE(editableTableModel.cellText(0, 1), QStringLiteral("42"));
    CTEST_CHECK(!editableTableModel.setCellValue(0, 1, QStringLiteral("42.5")));
    NAVIGATION_COMPARE(editableTableRows.valueAt(0, 1).toInt(), 42);
    CTEST_CHECK(editableTableModel.setCellValue(1, 0, QStringLiteral("Compiler")));
    NAVIGATION_COMPARE(editableTableRows.valueAt(1, 0).toString(), QStringLiteral("Compiler"));
    CTEST_CHECK(!editableTableModel.structureMutationAvailable());
    CTEST_CHECK(!editableTableModel.appendColumn());

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
    NAVIGATION_COMPARE(layoutTableModel.columnWidth(0), 80);
    NAVIGATION_COMPARE(layoutTableModel.columnX(2), 180);
    NAVIGATION_COMPARE(layoutTableModel.columnSpanWidth(1, 2), 220);
    NAVIGATION_COMPARE(layoutTableModel.rowHeightAt(1), 30);
    NAVIGATION_COMPARE(layoutTableModel.rowY(1), 24);
    NAVIGATION_COMPARE(layoutTableModel.totalBodyHeight(), 54);
    const QVariantMap layoutCell = layoutTableModel.visibleCells().at(1).toMap();
    NAVIGATION_COMPARE(layoutCell.value(QStringLiteral("x")).toInt(), 80);
    NAVIGATION_COMPARE(layoutCell.value(QStringLiteral("width")).toInt(), 100);
    NAVIGATION_COMPARE(layoutCell.value(QStringLiteral("height")).toInt(), 24);
    CTEST_CHECK(layoutTableModel.setColumnWidth(2, 1));
    NAVIGATION_COMPARE(layoutTableModel.columnWidth(2), 40);
    CTEST_CHECK(layoutTableModel.undo());
    NAVIGATION_COMPARE(layoutTableModel.columnWidth(2), 120);
    CTEST_CHECK(layoutTableModel.beginColumnResize(1, 100));
    CTEST_CHECK(layoutTableModel.updateColumnResize(135));
    NAVIGATION_COMPARE(layoutTableModel.resizingColumnIndex(), 1);
    NAVIGATION_COMPARE(layoutTableModel.columnWidth(1), 135);
    layoutTableModel.endColumnResize();
    NAVIGATION_COMPARE(layoutTableModel.resizingColumnIndex(), -1);
    CTEST_CHECK(layoutTableModel.undo());
    NAVIGATION_COMPARE(layoutTableModel.columnWidth(1), 100);
    CTEST_CHECK(layoutTableModel.beginRowResize(0, 20));
    CTEST_CHECK(layoutTableModel.updateRowResize(32));
    NAVIGATION_COMPARE(layoutTableModel.resizingRowIndex(), 0);
    NAVIGATION_COMPARE(layoutTableModel.rowHeightAt(0), 36);
    layoutTableModel.endRowResize();
    NAVIGATION_COMPARE(layoutTableModel.resizingRowIndex(), -1);
    CTEST_CHECK(layoutTableModel.undo());
    NAVIGATION_COMPARE(layoutTableModel.rowHeightAt(0), 24);
    CTEST_CHECK(layoutTableModel.setContextCell(0, 1));
    NAVIGATION_COMPARE(layoutTableModel.contextRowIndex(), 0);
    NAVIGATION_COMPARE(layoutTableModel.contextColumnIndex(), 1);
    const QVariantList contextDescriptors = layoutTableModel.contextMenuDescriptors(0, 1);
    NAVIGATION_COMPARE(contextDescriptors.size(), 3);
    NAVIGATION_COMPARE(contextDescriptors.at(0).toMap().value(QStringLiteral("action")).toString(), QStringLiteral("deleteRow"));
    NAVIGATION_COMPARE(contextDescriptors.at(2).toMap().value(QStringLiteral("action")).toString(), QStringLiteral("deleteColumn"));
    const QVariantMap contextResult = layoutTableModel.triggerContextAction(QStringLiteral("deleteColumn"), 0, 1);
    CTEST_CHECK(contextResult.value(QStringLiteral("accepted")).toBool());
    NAVIGATION_COMPARE(layoutTableModel.columnCount(), 2);
    NAVIGATION_COMPARE(layoutTableModel.columnWidths().toList().size(), 2);
    NAVIGATION_COMPARE(layoutTableModel.contextColumnIndex(), -1);
    CTEST_CHECK(layoutTableModel.undo());
    NAVIGATION_COMPARE(layoutTableModel.columnCount(), 3);

    ModelUndoStack stack;
    stack.pushSnapshot(QVariantMap {{QStringLiteral("value"), 1}});
    stack.pushSnapshot(QVariantMap {{QStringLiteral("value"), 2}});
    NAVIGATION_COMPARE(stack.undoDepth(), 2);
    CTEST_CHECK(stack.canUndo());
    const QVariant firstUndo = stack.takeUndoSnapshot(QVariantMap {{QStringLiteral("value"), 3}});
    NAVIGATION_COMPARE(firstUndo.toMap().value(QStringLiteral("value")).toInt(), 2);
    NAVIGATION_COMPARE(stack.undoDepth(), 1);
    NAVIGATION_COMPARE(stack.redoDepth(), 1);
    CTEST_CHECK(stack.canRedo());
    const QVariant firstRedo = stack.takeRedoSnapshot(firstUndo);
    NAVIGATION_COMPARE(firstRedo.toMap().value(QStringLiteral("value")).toInt(), 3);
    NAVIGATION_COMPARE(stack.undoDepth(), 2);
    NAVIGATION_COMPARE(stack.redoDepth(), 0);

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

    NAVIGATION_COMPARE(tableModel.rowCount(), 2);
    NAVIGATION_COMPARE(tableModel.columnCount(), 4);
    NAVIGATION_COMPARE(tableModel.headerCellType(0), QStringLiteral("string"));
    NAVIGATION_COMPARE(tableModel.headerCellType(1), QStringLiteral("int"));
    NAVIGATION_COMPARE(tableModel.headerCellType(2), QStringLiteral("float"));
    NAVIGATION_COMPARE(tableModel.headerCellType(3), QStringLiteral("bool"));
    NAVIGATION_COMPARE(tableModel.cellText(0, 0), QStringLiteral("Renderer"));
    CTEST_CHECK(tableModel.setCellValue(0, 1, QStringLiteral("42")));
    CTEST_CHECK(!tableModel.setCellValue(0, 1, QStringLiteral("42.5")));
    CTEST_CHECK(tableModel.setCellValue(0, 3, QStringLiteral("false")));
    NAVIGATION_COMPARE(tableModel.cellAt(0, 1).toMap().value(QStringLiteral("value")).toInt(), 42);
    NAVIGATION_COMPARE(tableModel.cellAt(0, 3).toMap().value(QStringLiteral("value")).toBool(), false);
    CTEST_CHECK(tableModel.canUndo());
    CTEST_CHECK(tableModel.undoDepth() >= 2);
    CTEST_CHECK(tableModel.undo());
    NAVIGATION_COMPARE(tableModel.cellAt(0, 3).toMap().value(QStringLiteral("value")).toBool(), true);
    CTEST_CHECK(tableModel.canRedo());
    CTEST_CHECK(tableModel.redo());
    NAVIGATION_COMPARE(tableModel.cellAt(0, 3).toMap().value(QStringLiteral("value")).toBool(), false);
    CTEST_CHECK(!tableModel.canRedo());

    NAVIGATION_COMPARE(tableModel.visibleCells().size(), 8);
    CTEST_CHECK(tableModel.mergeCells(0, 0, 1, 2));
    CTEST_CHECK(tableModel.isCoveredCell(0, 1));
    NAVIGATION_COMPARE(tableModel.visibleCells().size(), 7);
    CTEST_CHECK(tableModel.splitCell(0, 1));
    CTEST_CHECK(!tableModel.isCoveredCell(0, 1));
    NAVIGATION_COMPARE(tableModel.visibleCells().size(), 8);

    CTEST_CHECK(tableModel.insertRow(1));
    NAVIGATION_COMPARE(tableModel.rowCount(), 3);
    NAVIGATION_COMPARE(tableModel.cellAt(1, 1).toMap().value(QStringLiteral("value")).toInt(), 0);
    NAVIGATION_COMPARE(tableModel.cellAt(1, 3).toMap().value(QStringLiteral("value")).toBool(), false);
    CTEST_CHECK(tableModel.insertColumn(2));
    NAVIGATION_COMPARE(tableModel.columnCount(), 5);
    CTEST_CHECK(tableModel.deleteColumn(2));
    NAVIGATION_COMPARE(tableModel.columnCount(), 4);
    CTEST_CHECK(tableModel.deleteRow(1));
    NAVIGATION_COMPARE(tableModel.rowCount(), 2);
    CTEST_CHECK(tableModel.undo());
    NAVIGATION_COMPARE(tableModel.rowCount(), 3);
    CTEST_CHECK(tableModel.redo());
    NAVIGATION_COMPARE(tableModel.rowCount(), 2);

    tableModel.setRows(QVariantList {QVariantList {QStringLiteral("External")}});
    CTEST_CHECK(!tableModel.canUndo());
    CTEST_CHECK(!tableModel.canRedo());
    NAVIGATION_COMPARE(tableModel.rowCount(), 1);
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
    CTEST_CHECK_MESSAGE(result.ok, qPrintable(result.errorMessage()));
    NAVIGATION_COMPARE(result.contextNames,
             (QStringList {QStringLiteral("statusService"), QStringLiteral("statusViewModel")}));
    NAVIGATION_COMPARE(result.viewModelKeys, (QStringList {QStringLiteral("StatusVM")}));

    NAVIGATION_COMPARE(qvariant_cast<QObject *>(engine.rootContext()->contextProperty(QStringLiteral("statusService"))),
             &service);
    NAVIGATION_COMPARE(qvariant_cast<QObject *>(engine.rootContext()->contextProperty(QStringLiteral("statusViewModel"))),
             viewModel);

    auto *registry = engine.singletonInstance<ViewModelRegistry *>(QStringLiteral("LVRS"),
                                                                   QStringLiteral("ViewModels"));
    CTEST_CHECK(registry);
    NAVIGATION_COMPARE(registry->get(QStringLiteral("StatusVM")), viewModel);
    NAVIGATION_COMPARE(registry->ownerOf(QStringLiteral("StatusVM")), QStringLiteral("StatusView"));
    CTEST_CHECK(registry->canWrite(QStringLiteral("StatusView"), QStringLiteral("StatusVM")));

    const QVariantMap descriptor = registry->descriptor(QStringLiteral("StatusVM"));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("displayName")).toString(), QStringLiteral("Status"));
    NAVIGATION_COMPARE(descriptor.value(QStringLiteral("metadata")).toMap().value(QStringLiteral("domain")).toString(),
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
    CTEST_CHECK(!result.ok);
    CTEST_CHECK(result.errorMessage().contains(QStringLiteral("missingContext")));
    CTEST_CHECK(result.errorMessage().contains(QStringLiteral("MissingVM")));
    CTEST_CHECK(result.contextNames.isEmpty());
    CTEST_CHECK(result.viewModelKeys.isEmpty());
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    NavigationStateTests tests;
    runner.run("page_monitor_history_metrics", [&] { tests.page_monitor_history_metrics(); });
    runner.run("page_monitor_signal_contract_and_normalization", [&] { tests.page_monitor_signal_contract_and_normalization(); });
    runner.run("view_state_tracker_syncs_stack_and_status", [&] { tests.view_state_tracker_syncs_stack_and_status(); });
    runner.run("view_state_tracker_disable_override_changes_active_target", [&] { tests.view_state_tracker_disable_override_changes_active_target(); });
    runner.run("route_matcher_normalization_and_param_contract", [&] { tests.route_matcher_normalization_and_param_contract(); });
    runner.run("viewmodels_registry_binding_ownership_and_write_permissions", [&] { tests.viewmodels_registry_binding_ownership_and_write_permissions(); });
    runner.run("viewmodels_registry_rebinding_clears_stale_ownership", [&] { tests.viewmodels_registry_rebinding_clears_stale_ownership(); });
    runner.run("viewmodels_registry_tracks_keys_and_ownership", [&] { tests.viewmodels_registry_tracks_keys_and_ownership(); });
    runner.run("viewmodels_registry_signal_and_prune_contract", [&] { tests.viewmodels_registry_signal_and_prune_contract(); });
    runner.run("viewmodel_base_and_registry_descriptors_track_cpp_state", [&] { tests.viewmodel_base_and_registry_descriptors_track_cpp_state(); });
    runner.run("state_model_stores_state_and_registry_descriptors_track_values", [&] { tests.state_model_stores_state_and_registry_descriptors_track_values(); });
    runner.run("cpp_model_sources_own_list_hierarchy_and_table_contracts", [&] { tests.cpp_model_sources_own_list_hierarchy_and_table_contracts(); });
    runner.run("table_projection_matches_public_cell_queries", [&] { tests.table_projection_matches_public_cell_queries(); });
    runner.run("table_viewport_projection_preserves_merged_anchors_and_bounds", [&] { tests.table_viewport_projection_preserves_merged_anchors_and_bounds(); });
    runner.run("table_column_count_tracks_headers_structure_and_undo", [&] { tests.table_column_count_tracks_headers_structure_and_undo(); });
    runner.run("qml_context_bind_plan_exposes_context_objects_and_viewmodels", [&] { tests.qml_context_bind_plan_exposes_context_objects_and_viewmodels(); });
    runner.run("qml_context_bind_plan_reports_missing_required_objects", [&] { tests.qml_context_bind_plan_reports_missing_required_objects(); });
    return runner.result();
}
#include "tst_navigation_state.moc"
