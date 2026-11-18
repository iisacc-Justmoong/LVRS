#include "QtEventWait.h"
#include <QElapsedTimer>
#include <QEventLoop>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <type_traits>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QAbstractListModel>
#include <QAbstractTableModel>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFont>
#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QImage>
#include <QCursor>
#include <QScopeGuard>
#include <QMetaMethod>
#include <QMetaEnum>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QVector>
#include <QtPlugin>
#include <memory>
#include "accent_fixture.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class ImportSignalRecorder : public QObject
{
    Q_OBJECT
public:
    QList<QVariantList> events;
public slots:
    void recordVoid() { events.append(QVariantList{}); }
    void recordActivation(const QVariant &item, int row, int depth) { events.append({item, row, depth}); }
};

namespace {
template <typename Actual, typename Expected>
bool importEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual) + 1.0, qreal(expected) + 1.0);
    else
        return actual == expected;
}
void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}
bool waitForWindowExposed(QWindow *window) { return window && lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000); }
bool waitForWindowActive(QWindow *window) { return window && lvrs_ctest::waitFor([&] { return window->isActive(); }, 5000); }
Qt::MouseButtons pressedButtons = Qt::NoButton;
void emitMouse(QWindow *window, QEvent::Type type, Qt::MouseButton button, Qt::KeyboardModifiers modifiers, const QPoint &point, int delay = -1)
{
    if (type == QEvent::MouseButtonPress) pressedButtons |= button;
    if (type == QEvent::MouseButtonRelease) pressedButtons &= ~Qt::MouseButtons(button);
    static ulong timestamp = 10000;
    timestamp += delay < 0 ? 16 : qMax(1, delay);
    const QPointF local(point), global(window->mapToGlobal(point));
    QMouseEvent event(type, local, local, global, button, pressedButtons, modifiers);
    event.setTimestamp(timestamp);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}
void mousePress(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers modifiers, const QPoint &point, int delay = -1)
{ emitMouse(window, QEvent::MouseButtonPress, button, modifiers, point, delay); }
void mouseRelease(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers modifiers, const QPoint &point, int delay = -1)
{ emitMouse(window, QEvent::MouseButtonRelease, button, modifiers, point, delay); }
void mouseMove(QWindow *window, const QPoint &point, int delay = -1)
{ emitMouse(window, QEvent::MouseMove, Qt::NoButton, Qt::NoModifier, point, delay); }
void mouseClick(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers modifiers, const QPoint &point, int delay = -1)
{ mouseMove(window, point, delay); mousePress(window, button, modifiers, point, delay); mouseRelease(window, button, modifiers, point, delay); }
void mouseDoubleClick(QWindow *window, Qt::MouseButton button, Qt::KeyboardModifiers modifiers, const QPoint &point, int delay = -1)
{
    mouseClick(window, button, modifiers, point, delay);
    mousePress(window, button, modifiers, point, delay);
    emitMouse(window, QEvent::MouseButtonDblClick, button, modifiers, point, delay);
    mouseRelease(window, button, modifiers, point, delay);
}
void keyClick(QWindow *window, Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier, int delay = -1)
{
    Q_UNUSED(delay);
    QString text;
    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        text = QString(modifiers.testFlag(Qt::ShiftModifier) ? QChar(key) : QChar(key).toLower());
    else if (key == Qt::Key_Space)
        text = QStringLiteral(" ");
    QKeyEvent press(QEvent::KeyPress, key, modifiers, text), release(QEvent::KeyRelease, key, modifiers, text);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}
}
#define IMPORT_COMPARE(actual, expected) CTEST_CHECK(importEqual((actual), (expected)))
#define IMPORT_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(importEqual((actual), (expected)))
#define IMPORT_WAIT_MESSAGE(expression, message) CTEST_CHECK_MESSAGE(lvrs_ctest::waitFor([&] { return bool(expression); }, 5000), (message))

namespace {
constexpr int kFlickableDragAndOvershootBounds = 3;
constexpr int kFlickableFollowBoundsBehavior = 1;

QPoint scenePoint(QQuickItem *item, const QPointF &localPoint)
{
    const QPointF scene = item->mapToScene(localPoint);
    return QPoint(qRound(scene.x()), qRound(scene.y()));
}

bool mouseAreaContainsScenePoint(QObject *root, const QPoint &scenePoint)
{
    const QPointF point(scenePoint);
    const auto items = root->findChildren<QQuickItem *>();
    for (QQuickItem *item : items) {
        const QString className = QString::fromLatin1(item->metaObject()->className());
        if (!className.contains(QStringLiteral("MouseArea")))
            continue;
        if (!item->isVisible() || !item->property("enabled").toBool())
            continue;
        if (item->contains(item->mapFromScene(point)))
            return true;
    }
    return false;
}

QQuickItem *visualChildByObjectName(QQuickItem *root, const QString &objectName)
{
    if (!root)
        return nullptr;
    if (root->objectName() == objectName)
        return root;
    for (QQuickItem *child : root->childItems()) {
        if (QQuickItem *match = visualChildByObjectName(child, objectName))
            return match;
    }
    return nullptr;
}

bool waitForInputMaterials(const QList<QQuickItem *> &fields)
{
    QList<std::shared_ptr<ImportSignalRecorder>> paints;
    for (auto *field : fields) {
        auto *canvas = visualChildByObjectName(field, QStringLiteral("inputFieldMaterial"));
        if (!canvas) return false;
        auto recorder = std::make_shared<ImportSignalRecorder>();
        const auto connection = QObject::connect(canvas, SIGNAL(painted()), recorder.get(), SLOT(recordVoid()));
        if (!connection || !QMetaObject::invokeMethod(canvas, "requestPaint")) return false;
        paints.append(std::move(recorder));
    }
    QElapsedTimer elapsed; elapsed.start();
    while (elapsed.elapsed() < 20000) {
        bool ready = true;
        for (const auto &paint : paints) ready = ready && !paint->events.isEmpty();
        if (ready) return true;
        processFor(10);
    }
    return false;
}

class NativeListModel : public QAbstractListModel
{
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        EnabledRole,
        SelectedRole
    };

    explicit NativeListModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        m_rows.append({QStringLiteral("Native A"), true, false});
        m_rows.append({QStringLiteral("Native B"), false, false});
        m_rows.append({QStringLiteral("Native C"), true, true});
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
        if (role == NameRole || role == Qt::DisplayRole)
            return row.name;
        if (role == EnabledRole)
            return row.enabled;
        if (role == SelectedRole)
            return row.selected;

        return {};
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {NameRole, QByteArrayLiteral("name")},
            {EnabledRole, QByteArrayLiteral("enabled")},
            {SelectedRole, QByteArrayLiteral("selected")},
        };
    }

    void appendRow(const QString &name, bool enabled, bool selected)
    {
        const int insertIndex = m_rows.size();
        beginInsertRows(QModelIndex(), insertIndex, insertIndex);
        m_rows.append({name, enabled, selected});
        endInsertRows();
    }

private:
    struct Row
    {
        QString name;
        bool enabled = true;
        bool selected = false;
    };

    QVector<Row> m_rows;
};

class NativeHierarchyModel : public QAbstractListModel
{
public:
    enum Roles {
        KeyRole = Qt::UserRole + 1,
        NameRole,
        DepthRole,
        ExpandedRole,
        CounterRole,
        EnabledRole,
        SelectedRole,
        ParentKeyRole,
        ParentItemKeyRole
    };

    explicit NativeHierarchyModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        m_rows.append({QStringLiteral("native-root"), QStringLiteral("Native Root"), 0, true, 2, true, false});
        m_rows.append({QStringLiteral("native-child"), QStringLiteral("Native Child"), 1, true, 1, true, false});
        m_rows.append({QStringLiteral("native-leaf"), QStringLiteral("Native Leaf"), 2, false, 7, true, true});
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
        if (role == ExpandedRole)
            return row.expanded;
        if (role == CounterRole)
            return row.counter;
        if (role == EnabledRole)
            return row.enabled;
        if (role == SelectedRole)
            return row.selected;
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
            {ExpandedRole, QByteArrayLiteral("expanded")},
            {CounterRole, QByteArrayLiteral("counter")},
            {EnabledRole, QByteArrayLiteral("enabled")},
            {SelectedRole, QByteArrayLiteral("selected")},
            {ParentKeyRole, QByteArrayLiteral("parentKey")},
            {ParentItemKeyRole, QByteArrayLiteral("parentItemKey")},
        };
    }

    void appendRow(const QString &key,
                   const QString &name,
                   int depth,
                   bool expanded,
                   int counter,
                   bool enabled,
                   bool selected)
    {
        const int insertIndex = m_rows.size();
        beginInsertRows(QModelIndex(), insertIndex, insertIndex);
        m_rows.append({key, name, depth, expanded, counter, enabled, selected});
        endInsertRows();
    }

private:
    struct Row
    {
        QString key;
        QString name;
        int depth = 0;
        bool expanded = false;
        int counter = -1;
        bool enabled = true;
        bool selected = false;
        QString parentKey;
    };

    QVector<Row> m_rows;
};

class NativeTableModel : public QAbstractTableModel
{
public:
    explicit NativeTableModel(QObject *parent = nullptr)
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

private:
    struct Row
    {
        QString name;
        int count = 0;
    };

    QVector<Row> m_rows;
};
}

class ImportApiTests
{
public:
    void embedded_qml_matches_source();
    void app_bootstrap_window_loads();
    void versionless_import_application_window_loads();
    void application_window_mobile_coverage_visibility_contract_loads();
    void application_window_page_stack_state_loads();
    void application_window_initial_properties_seed_page_stack();
    void application_window_safe_margin_scopes_to_layout_not_render_surface();
    void versionless_import_window_loads();
    void window_custom_chrome_interaction_contract_loads();
    void window_safe_margin_scopes_to_layout_not_render_surface();
    void appshell_compat_loads();
    void application_window_platform_adaptive_layout_loads();
    void icon_name_mapping_loads();
    void compact_icon_frame_contract_loads();
    void hierarchy_tree_model_api_loads();
    void hierarchy_string_array_model_loads();
    void hierarchy_direct_model_contract_loads();
    void hierarchy_direct_model_editing_contract_loads();
    void hierarchy_nested_children_indent_contract_loads();
    void hierarchy_expansion_uses_incremental_visibility_refresh();
    void hierarchy_editable_drag_depth_contract_loads();
    void hierarchy_editable_drag_per_item_lock_contract_loads();
    void hierarchy_mobile_drag_hold_contract_loads();
    void hierarchy_mobile_activation_commits_on_release_contract_loads();
    void hierarchy_mobile_reactivation_reemits_active_signal();
    void hierarchy_mobile_scroll_physics_contract_loads();
    void hierarchy_optional_footer_contract_loads();
    void hierarchy_toolbar_empty_state_does_not_reschedule();
    void hierarchy_toolbar_item_model_contract_loads();
    void hierarchy_toolbar_figma_layout_contract_loads();
    void hierarchy_figma_composition_contract_loads();
    void hierarchy_toolbar_manual_icon_button_contract_loads();
    void hierarchy_row_click_only_activates_not_toggles();
    void hierarchy_chevron_requires_children_loads();
    void hierarchy_item_chevron_direction_contract_loads();
    void hierarchy_item_hover_and_active_state_visual_contract_loads();
    void hierarchy_item_figma_defaults_contract_loads();
    void hierarchy_item_layout_geometry_contract_loads();
    void hierarchy_item_count_view_contract_loads();
    void hierarchy_item_structure_api_contract_loads();
    void hierarchy_item_ux_state_contract_loads();
    void button_padding_matches_figma_spec();
    void button_family_components_contract(bool mobile);
    void button_default_tone_matches_figma_accent_loads();
    void segmented_control_figma_contract_loads(bool mobile);
    void segmented_button_selected_color();
    void button_injected_methods_contract_loads();
    void stepper_figma_contract_loads();
    void combo_box_figma_contract_loads();
    void input_field_figma_contract_loads();
    void input_field_material_rendering_contract();
    void progress_bar_range_contract_loads();
    void control_icons_use_supersampled_raster_contract();
    void input_field_ios_native_text_interaction_contract_loads();
    void input_field_native_event_input_contract_loads();
    void input_field_search_icon_mobile_scaling_contract_loads();
    void toggle_switch_figma_contract_loads();
    void checkbox_figma_contract_loads();
    void radio_button_figma_contract_loads();
    void modal_empty_frame_contract_loads();
    void modal_content_action_contract_loads();
    void alert_figma_variant_contract_loads();
    void alert_content_arguments_contract_loads();
    void alert_button_methods_are_invoked(int buttonCount, int buttonIndex, const QString &buttonName);
    void alert_action_button_padding_scopes_to_alert();
    void alert_glass_overlay_and_input_contract(bool frameworkWindow, const QString &dataTag);
    void menu_item_key_and_chevron_contract_loads();
    void menu_item_icon_slot_switch_contract_loads();
    void context_menu_item_action_contract_loads();
    void context_menu_visual_contract_loads();
    void context_menu_icon_slot_switch_contract_loads();
    void context_menu_width_expansion_contract_loads();
    void context_menu_auto_placement_contract_loads();
    void model_component_delegate_contract_loads();
    void table_figma_geometry_contract_loads();
    void table_spreadsheet_api_contract_loads();
    void table_viewport_reuses_delegates_and_creates_one_menu_on_demand();
    void table_cell_item_contract_loads();
    void table_cell_merge_split_contract_loads();
    void table_structure_editing_contract_loads();
    void table_resize_contract_loads();
    void table_typed_header_contract_loads();
    void table_cpp_model_contract_loads();
    void table_undo_redo_contract_loads();
    void list_model_contract_loads();
    void list_figma_contract_loads();
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

void ImportApiTests::embedded_qml_matches_source()
{
    const QDir sourceRoot(QStringLiteral(LVRS_TEST_SOURCE_DIR "/../src"));
    QDirIterator files(sourceRoot.filePath(QStringLiteral("qml")),
                       {QStringLiteral("*.qml")}, QDir::Files, QDirIterator::Subdirectories);
    int checked = 0;
    while (files.hasNext()) {
        QFile source(files.next());
        const QString relativePath = sourceRoot.relativeFilePath(source.fileName());
        QFile embedded(QStringLiteral(":/qt/qml/LVRS/") + relativePath);
        if (!embedded.exists())
            embedded.setFileName(QStringLiteral(":/qt/qml/LVRS/") + QFileInfo(source.fileName()).fileName());
        CTEST_CHECK_MESSAGE(source.open(QIODevice::ReadOnly), qPrintable(source.fileName()));
        CTEST_CHECK_MESSAGE(embedded.open(QIODevice::ReadOnly), qPrintable(embedded.fileName()));
        const auto sourceHash = QCryptographicHash::hash(source.readAll(), QCryptographicHash::Sha256);
        const auto embeddedHash = QCryptographicHash::hash(embedded.readAll(), QCryptographicHash::Sha256);
        CTEST_CHECK_MESSAGE(embeddedHash == sourceHash,
                 qPrintable(QStringLiteral("Loaded LVRS QML differs from current source: %1; "
                                           "source=%2 loaded=%3. Check the library loader path.")
                                .arg(relativePath, QString::fromLatin1(sourceHash.toHex()),
                                     QString::fromLatin1(embeddedHash.toHex()))));
        ++checked;
    }
    CTEST_CHECK(checked > 0);
    qInfo() << "Verified embedded QML against" << checked << "current source files";
}

void ImportApiTests::app_bootstrap_window_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);

    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import LVRS as LV

    LV.AppBootstrapWindow {
        width: 430
        height: 932
        visible: false
        pageRoutes: [
        { path: "/", component: homePage }
    ]

    property bool bootstrapContractReady:
        !navigationEnabled
        && autoAttachRuntimeEvents === (backendRuntimeProfile.runtimeEventsAutoAttachRecommended === true)
        && internalRouterRegisterAsGlobalNavigator
        && !mobileOversizedHeightEnabled
        && useInternalPageStack
        && !autoApplyDeviceTierPreset
        && forcedDeviceTierPreset < 0
        && pageInitialPath === initialRoutePath

    Component {
        id: homePage
        Item {}
    }
}
)",
                      QUrl());

    QScopedPointer<QObject> root(component.create());
    CTEST_CHECK_MESSAGE(root, "Expected AppBootstrapWindow to load");
    CTEST_CHECK_MESSAGE(!component.isError(), "Unexpected QML errors while creating AppBootstrapWindow");
    CTEST_CHECK(root->property("bootstrapContractReady").toBool());
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
            === (backendRuntimeProfile.runtimeEventsAutoAttachRecommended === true)
        && !autoHookBackendUserEvents
        && !globalEventListenersEnabled
    property bool bootstrapContractReady: internalRouterRegisterAsGlobalNavigator
        && !mobileOversizedHeightEnabled
        && useInternalPageStack
    property bool platformPolicyDefaultsReady:
        delegateMobileWindowingToSystem === (backendRuntimeProfile.mobileSystemWindowDelegationRecommended === true)
        && delegateMobileInsetsToSystem === (backendRuntimeProfile.mobileSystemInsetsDelegationRecommended === true)
        && forceFullWindowAreaOnMobile === backendMobilePlatform
        && usePlatformSafeMargin === backendMobilePlatform
        && safeMargin === (backendMobilePlatform ? 16 : 0)
        && mobileDisplayCoverageOverrideEnabled === ((backendRuntimeProfile.mobileDisplayCoverageOverrideRecommended === true)
            && !delegateMobileWindowingToSystem)
        && mobileFullscreenVisibilityOverride === ((backendRuntimeProfile.mobileFullscreenVisibilityRecommended === true)
            && !delegateMobileWindowingToSystem)
        && mobileFullscreenGeometryHintOverride === ((backendRuntimeProfile.mobileFullscreenGeometryHintRecommended === true)
            && !delegateMobileWindowingToSystem)
    property bool mobileSystemSafeAreaApiReady:
        typeof mobileSystemSafeLeftInset === "number"
        && typeof mobileSystemSafeTopInset === "number"
        && typeof mobileSystemSafeRightInset === "number"
        && typeof mobileSystemSafeBottomInset === "number"
        && typeof mobileSystemSafeAreaResolved === "boolean"
        && mobileSystemSafeAreaBounds.width >= 0
        && mobileSystemSafeAreaBounds.height >= 0
    property bool deviceTierPolicyReady: !autoApplyDeviceTierPreset
        && forcedDeviceTierPreset < 0
    property bool chromeInteractionReady:
        windowChromeInteractionsEnabled === (solidChrome && isDesktopPlatform)
        && windowDragHandleEnabled === windowChromeInteractionsEnabled
        && windowResizeHandlesEnabled === windowChromeInteractionsEnabled
        && windowResizeBorderThickness > 0
        && windowResizeCornerSize >= windowResizeBorderThickness
        && windowChromeInteractionLayer !== null
        && windowDragHandleItem !== null
        && typeof requestWindowMove === "function"
        && typeof requestWindowResize === "function"
    property bool labelStyleApiReady: contentLabel.style === contentLabel.body
        && contentLabel.font.pixelSize === LV.Theme.textBody
        && contentLabel.font.weight === LV.Theme.textBodyWeight
        && contentLabel.color.toString() === LV.Theme.bodyColor.toString()
        && contentLabel.contentHeight > 0
        && contentLabel.lineCount === 1
        && !contentLabel.sizeToContentHeight
        && contentLabel.renderType === Text.NativeRendering
    property bool figmaTextDesignReady:
        titleLabel.font.pixelSize === LV.Theme.textTitle
        && titleLabel.font.weight === LV.Theme.textTitleWeight
        && titleLabel.color.toString() === LV.Theme.titleHeaderColor.toString()
        && title2Label.font.pixelSize === LV.Theme.textTitle2
        && title2Label.font.weight === LV.Theme.textTitle2Weight
        && title2Label.color.toString() === LV.Theme.titleHeaderColor.toString()
        && headerLabel.font.pixelSize === LV.Theme.textHeader
        && headerLabel.font.weight === LV.Theme.textHeaderWeight
        && headerLabel.color.toString() === LV.Theme.titleHeaderColor.toString()
        && header2Label.font.pixelSize === LV.Theme.textHeader2
        && header2Label.font.weight === LV.Theme.textHeader2Weight
        && header2Label.color.toString() === LV.Theme.titleHeaderColor.toString()
        && bodyLabel.font.pixelSize === LV.Theme.textBody
        && bodyLabel.font.weight === LV.Theme.textBodyWeight
        && bodyLabel.color.toString() === LV.Theme.bodyColor.toString()
        && descriptionLabel.font.pixelSize === LV.Theme.textDescription
        && descriptionLabel.font.weight === LV.Theme.textDescriptionWeight
        && descriptionLabel.color.toString() === LV.Theme.descriptionColor.toString()
        && captionLabel.font.pixelSize === LV.Theme.textCaption
        && captionLabel.font.weight === LV.Theme.textCaptionWeight
        && captionLabel.color.toString() === LV.Theme.captionColor.toString()
        && disabledLabel.font.pixelSize === LV.Theme.textDisabled
        && disabledLabel.font.weight === LV.Theme.textDisabledWeight
        && disabledLabel.color.toString() === LV.Theme.disabledColor.toString()

    LV.Label {
        id: contentLabel
        text: "Content Slot"
        style: body
    }
    LV.WindowSafeAreaObserver {
        id: safeAreaObserver
        window: null
    }
    property bool safeAreaObserverReady:
        safeAreaObserver.leftInset === 0
        && safeAreaObserver.topInset === 0
        && safeAreaObserver.rightInset === 0
        && safeAreaObserver.bottomInset === 0
        && !safeAreaObserver.resolved
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
    CTEST_CHECK(root);
    CTEST_CHECK(root->property("importReady").toBool());
    CTEST_CHECK(root->property("shellApiReady").toBool());
    CTEST_CHECK(root->property("adaptiveApiReady").toBool());
    CTEST_CHECK(root->property("qualityReady").toBool());
    CTEST_CHECK(root->property("backendOptimizationDefaultsReady").toBool());
    CTEST_CHECK(root->property("bootstrapContractReady").toBool());
    CTEST_CHECK(root->property("platformPolicyDefaultsReady").toBool());
    CTEST_CHECK(root->property("mobileSystemSafeAreaApiReady").toBool());
    CTEST_CHECK(root->property("deviceTierPolicyReady").toBool());
    CTEST_CHECK(root->property("chromeInteractionReady").toBool());
    CTEST_CHECK(root->property("safeAreaObserverReady").toBool());
    CTEST_CHECK(root->property("labelStyleApiReady").toBool());
    CTEST_CHECK(root->property("figmaTextDesignReady").toBool());
    IMPORT_COMPARE(root->property("subtitle").toString(), QStringLiteral("Merged"));
}

void ImportApiTests::application_window_mobile_coverage_visibility_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import QtQuick.Window
import LVRS as LV

LV.ApplicationWindow {
    visible: false
    width: 430
    height: 932

    property bool coverageVisibilityReady:
        mobileCoverageTargetVisibilityForPlatform("ios") === Window.Maximized
        && mobileCoverageTargetVisibilityForPlatform("ios-simulator") === Window.Maximized
        && mobileCoverageTargetVisibilityForPlatform("android") === Window.FullScreen
        && mobileCoverageTargetVisibilityForPlatform("macos") === Window.FullScreen
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_CHECK(root->property("coverageVisibilityReady").toBool());
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

    property bool windowApiReady: platform.length > 0
        && (widthClass >= compact && widthClass <= expanded)
        && (heightClass >= compact && heightClass <= expanded)
        && typeof matchesMedia === "function"
        && !autoApplyDeviceTierPreset
        && forcedDeviceTierPreset < 0
        && usePlatformSafeMargin === backendMobilePlatform
        && safeMargin === (backendMobilePlatform ? 16 : 0)
    property bool contentApiReady: contentLabel.text === "Window Content"

    LV.Label {
        id: contentLabel
        text: "Window Content"
        style: body
    }

    Item {
        objectName: "contentProbe"
        anchors.fill: parent
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_COMPARE(root->property("title").toString(), QStringLiteral("Settings"));
    CTEST_CHECK(root->property("solidChrome").toBool());
    CTEST_CHECK(root->property("windowApiReady").toBool());
    CTEST_CHECK(root->property("contentApiReady").toBool());
}

void ImportApiTests::window_custom_chrome_interaction_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.Window {
    id: root
    width: 360
    height: 240
    visible: true
    windowChromeInteractionsEnabled: true
    windowDragHandleHeight: 32
    windowDragHandleTopMargin: 4
    windowDragHandleLeftMargin: 10
    windowDragHandleRightMargin: 12
    windowResizeBorderThickness: 7
    windowResizeCornerSize: 15
    windowChromeInteractionZ: 12000

    property int moveAttemptCount: 0
    property int resizeAttemptCount: 0
    property int lastResizeEdges: 0
    property bool lastInteractionStarted: true
    property bool chromeApiReady:
        windowDragHandleEnabled
        && windowResizeHandlesEnabled
        && windowResizeEdges === (Qt.LeftEdge | Qt.TopEdge | Qt.RightEdge | Qt.BottomEdge)
        && windowChromeInteractionLayer !== null
        && windowDragHandleItem !== null
        && typeof requestWindowMove === "function"
        && typeof requestWindowResize === "function"

    onWindowMoveAttempted: function(started) {
        moveAttemptCount += 1
        lastInteractionStarted = started
    }
    onWindowResizeAttempted: function(edges, started) {
        resizeAttemptCount += 1
        lastResizeEdges = edges
        lastInteractionStarted = started
    }

    Item {
        id: excludedDragItem
        objectName: "excludedDragItem"
        x: 100
        y: 4
        width: 50
        height: 32
    }

    windowDragExclusionItems: [excludedDragItem]
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_CHECK(root->property("chromeApiReady").toBool());

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    CTEST_WAIT_DEFAULT(window->isVisible());

    auto *interactionLayer = root->findChild<QQuickItem *>(QStringLiteral("windowChromeInteractionLayer"));
    auto *dragHandle = root->findChild<QQuickItem *>(QStringLiteral("windowDragHandle"));
    auto *leftResizeHandle = root->findChild<QQuickItem *>(QStringLiteral("windowResizeLeftHandle"));
    auto *topLeftResizeHandle = root->findChild<QQuickItem *>(QStringLiteral("windowResizeTopLeftHandle"));
    CTEST_CHECK(interactionLayer);
    CTEST_CHECK(dragHandle);
    CTEST_CHECK(leftResizeHandle);
    CTEST_CHECK(topLeftResizeHandle);

    IMPORT_COMPARE(interactionLayer->z(), 12000.0);
    IMPORT_COMPARE(dragHandle->x(), 10.0);
    IMPORT_COMPARE(dragHandle->y(), 4.0);
    IMPORT_COMPARE(dragHandle->width(), 338.0);
    IMPORT_COMPARE(dragHandle->height(), 32.0);
    IMPORT_COMPARE(leftResizeHandle->width(), 7.0);
    IMPORT_COMPARE(topLeftResizeHandle->width(), 15.0);
    IMPORT_COMPARE(topLeftResizeHandle->height(), 15.0);

#ifdef Q_OS_WIN
    if (QGuiApplication::platformName() == QStringLiteral("windows"))
        lvrs_ctest::skip("Native Windows move/resize enters an OS modal loop with synthetic input; the offscreen companion verifies these requests");
#endif

    mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(125, 20));
    IMPORT_COMPARE(root->property("moveAttemptCount").toInt(), 0);

    mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(180, 20));
    IMPORT_WAIT_COMPARE(root->property("moveAttemptCount").toInt(), 1);

    mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(2, 120));
    IMPORT_WAIT_COMPARE(root->property("resizeAttemptCount").toInt(), 1);
    IMPORT_COMPARE(root->property("lastResizeEdges").toInt(), int(Qt::LeftEdge));

    mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(2, 2));
    IMPORT_WAIT_COMPARE(root->property("resizeAttemptCount").toInt(), 2);
    IMPORT_COMPARE(root->property("lastResizeEdges").toInt(), int(Qt::LeftEdge | Qt::TopEdge));
}

void ImportApiTests::window_safe_margin_scopes_to_layout_not_render_surface()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.Window {
    id: root
    width: 520
    height: 360
    visible: false
    usePlatformSafeMargin: false
    safeMargin: 24

    LV.Label {
        id: contentLabel
        text: "Window Content"
        style: body
    }

    Item {
        objectName: "contentProbe"
        anchors.fill: parent
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    const qreal rootWidth = root->property("width").toReal();
    const qreal rootHeight = root->property("height").toReal();
    root->setProperty("visible", true);
    IMPORT_WAIT_COMPARE(root->property("renderSurfaceBounds").toRectF(), QRectF(0.0, 0.0, rootWidth, rootHeight));
    IMPORT_WAIT_COMPARE(root->property("layoutSafeAreaBounds").toRectF(), QRectF(24.0, 24.0, rootWidth - 48.0, rootHeight - 48.0));
    auto *layoutHost = root->findChild<QQuickItem *>(QStringLiteral("windowLayoutSafeAreaHost"));
    CTEST_CHECK(layoutHost);
    IMPORT_WAIT_COMPARE(layoutHost->width(), rootWidth - 48.0);
    IMPORT_WAIT_COMPARE(layoutHost->height(), rootHeight - 48.0);
    const QPointF layoutOrigin = layoutHost->mapToScene(QPointF(0.0, 0.0));
    IMPORT_COMPARE(layoutOrigin, QPointF(24.0, 24.0));
    auto *contentProbe = root->findChild<QQuickItem *>(QStringLiteral("contentProbe"));
    CTEST_CHECK(contentProbe);
    IMPORT_WAIT_COMPARE(contentProbe->width(), rootWidth);
    IMPORT_WAIT_COMPARE(contentProbe->height(), rootHeight);
    IMPORT_COMPARE(contentProbe->mapToScene(QPointF(0.0, 0.0)), QPointF(0.0, 0.0));
}

void ImportApiTests::application_window_safe_margin_scopes_to_layout_not_render_surface()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 520
    height: 360
    visible: false
    navigationEnabled: false
    usePlatformSafeMargin: false
    safeMargin: 24

    LV.Label {
        id: contentLabel
        text: "Application Content"
        style: body
    }

    Item {
        objectName: "contentProbe"
        anchors.fill: parent
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    const qreal rootWidth = root->property("width").toReal();
    const qreal rootHeight = root->property("height").toReal();
    root->setProperty("visible", true);
    const QRectF renderSurfaceBounds = root->property("renderSurfaceBounds").toRectF();
    const QRectF layoutSafeAreaBounds = root->property("layoutSafeAreaBounds").toRectF();
    IMPORT_COMPARE(renderSurfaceBounds, QRectF(0.0, 0.0, rootWidth, rootHeight));
    IMPORT_COMPARE(layoutSafeAreaBounds, QRectF(24.0, 24.0, rootWidth - 48.0, rootHeight - 48.0));
    auto *layoutHost = root->findChild<QQuickItem *>(QStringLiteral("applicationWindowLayoutSafeAreaHost"));
    CTEST_CHECK(layoutHost);
    const QPointF layoutOrigin = layoutHost->mapToScene(QPointF(0.0, 0.0));
    IMPORT_COMPARE(layoutOrigin, QPointF(24.0, 24.0));
    IMPORT_COMPARE(layoutHost->width(), rootWidth - 48.0);
    IMPORT_COMPARE(layoutHost->height(), rootHeight - 48.0);
    auto *contentProbe = root->findChild<QQuickItem *>(QStringLiteral("contentProbe"));
    CTEST_CHECK(contentProbe);
    IMPORT_WAIT_COMPARE(contentProbe->width(), rootWidth);
    IMPORT_WAIT_COMPARE(contentProbe->height(), rootHeight);
    IMPORT_COMPARE(contentProbe->mapToScene(QPointF(0.0, 0.0)), QPointF(0.0, 0.0));
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("stackInitialReady").toBool());
    CTEST_WAIT_DEFAULT(root->property("stackNavigationWorked").toBool());
}

void ImportApiTests::application_window_initial_properties_seed_page_stack()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);

    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 430
    height: 932
    visible: false
    pageRoutes: [
        { path: "/", component: homePage }
    ]

    property bool stackReady: internalPageStackEnabled
        && activePageRouter !== null
        && activePageRouter.currentPath === initialRoutePath
        && autoAttachRuntimeEvents === (backendRuntimeProfile.runtimeEventsAutoAttachRecommended === true)
        && !navigationEnabled
        && internalRouterRegisterAsGlobalNavigator
        && !mobileOversizedHeightEnabled

    Component {
        id: homePage
        Item { objectName: "home-page" }
    }
}
)",
                      QUrl());

    QObject *created = component.createWithInitialProperties({
        {QStringLiteral("initialRoutePath"), QStringLiteral("/")}
    });
    QScopedPointer<QObject> root(created);
    CTEST_CHECK_MESSAGE(root, "Expected ApplicationWindow to load with initial properties");
    CTEST_CHECK_MESSAGE(!component.isError(), "Unexpected QML errors while creating ApplicationWindow with initial properties");
    CTEST_WAIT_DEFAULT(root->property("stackReady").toBool());
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
    CTEST_CHECK(root);
    IMPORT_COMPARE(root->property("title").toString(), QStringLiteral("Compat"));
    IMPORT_COMPARE(root->property("subtitle").toString(), QStringLiteral("Wrapper"));
    CTEST_CHECK(root->property("navItems").isValid());
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
    scaffoldLayoutPlatform: "android-arm64"
    navItems: [
        { label: "Home", symbol: "H" },
        { label: "Runs", symbol: "R" },
        { label: "Settings", symbol: "S" }
    ]
    navigationEnabled: true

    Component.onCompleted: LV.Theme.targetOverride = "android"
    Component.onDestruction: LV.Theme.targetOverride = ""

    property bool contract:
        LV.Theme.mobileTarget
        && mobileWindow.navigationIconSize === 18
        && mobileWindow.adaptiveMobileLayout
        && !mobileWindow.adaptiveDesktopLayout
        && mobileWindow.adaptiveBottomNavigation
        && !mobileWindow.adaptiveRailNavigation
        && !mobileWindow.adaptiveDrawerNavigation
}
)";

    QScopedPointer<QObject> mobileRoot(createFromQml(mobileEngine, mobileQml));
    CTEST_CHECK(mobileRoot);
    CTEST_WAIT_DEFAULT(mobileRoot->property("contract").toBool());
    IMPORT_COMPARE(mobileRoot->property("navigationIconSize").toInt(), 18);

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
    navItems: [
        { label: "Home", symbol: "H" },
        { label: "Runs", symbol: "R" },
        { label: "Settings", symbol: "S" }
    ]
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
    CTEST_CHECK(desktopRoot);
    CTEST_WAIT_DEFAULT(desktopRoot->property("contract").toBool());
    IMPORT_COMPARE(desktopRoot->property("navigationIconSize").toInt(), 18);
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

    function sameColor(left, right) {
        return Math.abs(left.r - right.r) < 0.001
            && Math.abs(left.g - right.g) < 0.001
            && Math.abs(left.b - right.b) < 0.001
            && Math.abs(left.a - right.a) < 0.001
    }

    property string iconRoot: "qrc:/qt/qml/LVRS/resources/iconset/"
    property string expectedByName: iconRoot + "generalmoreHorizontal.svg"
    property string expectedByExt: iconRoot + "generalmoreHorizontal.svg"
    property string expectedByGroup: iconRoot + "generalchevronDown.svg"
    property string expectedByUrl: iconRoot + "generalchevronDownAccent.svg"
    property string expectedMenuByName: iconRoot + "generalchevronDownBorderless.svg"
    property bool themeAddsSvg: LV.Theme.iconPath("panDownSymbolicDisabled") === iconRoot + "generalchevronDownDisabled.svg"
    property bool themeKeepsSvg: LV.Theme.iconPath("panDownSymbolicDisabled.svg") === iconRoot + "generalchevronDownDisabled.svg"
    property bool themeFlattensGroupedName: LV.Theme.iconPath("general/projectStructure") === iconRoot + "generalprojectStructure.svg"

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
    CTEST_CHECK(root);
    CTEST_CHECK(root->property("themeAddsSvg").toBool());
    CTEST_CHECK(root->property("themeKeepsSvg").toBool());
    CTEST_CHECK(root->property("themeFlattensGroupedName").toBool());
    CTEST_CHECK(root->property("byNameOk").toBool());
    CTEST_CHECK(root->property("byExtOk").toBool());
    CTEST_CHECK(root->property("byGroupOk").toBool());
    CTEST_CHECK(root->property("byUrlOk").toBool());
    CTEST_CHECK(root->property("menuByNameOk").toBool());
}

void ImportApiTests::compact_icon_frame_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 152
    height: 62

    property string themeTarget: "macos"
    readonly property int expectedIconSize: 18
    readonly property int expectedInputIconSize: 12
    readonly property int expectedCheckBoxSize: 17
    readonly property int expectedRadioDotSize: 8
    readonly property int expectedMenuItemHeight: 24
    readonly property int expectedMenuChevronSize: 16
    readonly property int expectedHierarchyChevronSize: 16

    onThemeTargetChanged: LV.Theme.targetOverride = themeTarget
    Component.onCompleted: LV.Theme.targetOverride = themeTarget
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.IconButton { id: iconButton; visible: false }
    LV.IconMenuButton { id: iconMenuButton; visible: false }
    LV.LabelMenuButton { id: labelMenuButton; visible: false }
    LV.Stepper { id: stepper; visible: false }
    LV.ComboBox { id: comboBox; visible: false }
    LV.InputField { id: inputField; visible: false; search: true; text: "query" }
    LV.CheckBox { id: checkBox; visible: false }
    LV.RadioButton { id: radioButton; visible: false }
    LV.ToggleSwitch { id: toggleSwitch; visible: false }
    LV.ListToolbar { id: listToolbar; visible: false }
    LV.MenuItem { id: menuItem; visible: false }
    LV.HierarchyItem { id: hierarchyItem; visible: false }
    LV.HierarchyList { id: hierarchyList; visible: false }
    LV.IconButton { id: customIconButton; visible: false; iconSize: 14 }

    property string compactIconDiagnostics:
        "target=" + LV.Theme.effectiveTarget
        + " iconSm=" + LV.Theme.iconSm
        + " iconButton=" + iconButton.iconSize
        + " iconMenu=" + iconMenuButton.iconSize + "/" + iconMenuButton.indicatorSize
        + " labelMenu=" + labelMenuButton.indicatorSize
        + " stepper=" + stepper.width + "x" + stepper.height
        + " combo=" + comboBox.figmaIndicatorSize
        + " input=" + inputField.searchIconSize + "/" + inputField.clearIconSize
        + " check=" + checkBox.boxSize
        + " radio=" + radioButton.indicatorSize
        + " toggle=" + toggleSwitch.knobSize
        + " listToolbar=" + listToolbar.iconSize
        + " menu=" + menuItem.itemHeight + "/" + menuItem.iconSize + "/" + menuItem.chevronSize
        + " hierarchy=" + hierarchyItem.iconSize + "/" + hierarchyItem.chevronSize
        + " hierarchyList=" + hierarchyList.generatedIconSize + "/" + hierarchyList.generatedChevronSize
        + " custom=" + customIconButton.iconSize

    property bool compactIconContractReady:
        LV.Theme.iconSm === expectedIconSize
        && iconButton.iconSize === expectedIconSize
        && iconMenuButton.iconSize === expectedIconSize
        && iconMenuButton.indicatorSize === expectedIconSize
        && labelMenuButton.indicatorSize === expectedIconSize
        && stepper.width === expectedIconSize
        && stepper.height === expectedIconSize
        && comboBox.figmaIndicatorSize === expectedIconSize
        && inputField.searchIconSize === expectedInputIconSize
        && inputField.clearIconSize === expectedInputIconSize
        && checkBox.boxSize === expectedCheckBoxSize
        && radioButton.indicatorSize === expectedIconSize
        && radioButton.dotSize === expectedRadioDotSize
        && toggleSwitch.knobSize === expectedIconSize
        && listToolbar.iconSize === expectedIconSize
        && menuItem.itemHeight === expectedMenuItemHeight
        && menuItem.iconSize === expectedIconSize
        && menuItem.chevronSize === expectedMenuChevronSize
        && hierarchyItem.iconSize === expectedIconSize
        && hierarchyItem.chevronSize === expectedHierarchyChevronSize
        && hierarchyList.generatedIconSize === expectedIconSize
        && hierarchyList.generatedChevronSize === expectedHierarchyChevronSize
        && customIconButton.iconSize === 14

    property bool mobileCompactIconContractReady:
        LV.Theme.mobileTarget
        && LV.Theme.iconSm === 18
        && inputField.searchIconSize === 12
        && inputField.clearIconSize === 12
        && compactIconContractReady
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("compactIconContractReady").toBool(),
                 qPrintable(root->property("compactIconDiagnostics").toString()));
    CTEST_CHECK(root->setProperty("themeTarget", QStringLiteral("android")));
    IMPORT_WAIT_MESSAGE(root->property("mobileCompactIconContractReady").toBool(),
                 qPrintable(root->property("compactIconDiagnostics").toString()));
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
        countRole: "counter"
        model: [
            { key: "root", depth: 0, itemId: 10, text: "Root", icon: "viewMoreSymbolicDefault", expanded: true, counter: 2 },
            { key: "child-a", depth: 1, itemId: 11, text: "Child A", icon: "viewMoreSymbolicDefault", expanded: false, counter: 5 },
            { key: "leaf-a1", depth: 2, itemId: 12, text: "Leaf A1", icon: "viewMoreSymbolicBorderless", counter: 14 },
            { key: "child-b", depth: 1, itemId: 20, text: "Child B", icon: "viewMoreSymbolicDisabled", counter: 0 }
        ]
    }

    Component.onCompleted: {
        Qt.callLater(tryActivateLeaf)
    }

    property bool treeApiReady:
        hierarchy.activeListItem !== null
        && hierarchy.countRole === "counter"
        && hierarchy.activeListItemKey === "leaf-a1"
        && hierarchy.activeListItemId === 12
        && hierarchy.activeListItem.label === "Leaf A1"
        && hierarchy.activeListItem.iconName === "viewMoreSymbolicBorderless"
        && hierarchy.activeListItem.pathLabel === "Root / Child A / Leaf A1"
        && hierarchy.activeListItem.count === 14
        && hierarchy.activeListItem.effectiveShowCount
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("treeApiReady").toBool());
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("stringModelReady").toBool());
}

void ImportApiTests::hierarchy_direct_model_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);

    NativeHierarchyModel nativeModel;
    engine.rootContext()->setContextProperty(QStringLiteral("nativeHierarchyModel"), &nativeModel);

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    width: 360
    height: 360

    ListModel {
        id: qmlRows
        ListElement { key: "qml-root"; name: "QML Root"; depth: 0; expanded: true; counter: 1; enabled: true; selected: false }
        ListElement { key: "qml-child"; name: "QML Child"; depth: 1; expanded: false; counter: 3; enabled: true; selected: true }
    }

    LV.Hierarchy {
        id: nativeHierarchy
        width: 260
        height: 260
        model: nativeHierarchyModel
        itemKeyRole: "key"
        labelRole: "name"
        depthRole: "depth"
        countRole: "counter"
        expandedRole: "expanded"
        selectedRole: "selected"
        enabledRole: "enabled"
    }

    LV.HierarchyList {
        id: nativeHierarchyList
        visible: false
        width: 260
        model: nativeHierarchyModel
        itemKeyRole: "key"
        labelRole: "name"
        depthRole: "depth"
        countRole: "counter"
        expandedRole: "expanded"
        selectedRole: "selected"
        enabledRole: "enabled"
    }

    LV.HierarchyList {
        id: qmlModelHierarchyList
        visible: false
        width: 260
        model: qmlRows
        itemKeyRole: "key"
        labelRole: "name"
        depthRole: "depth"
        countRole: "counter"
        expandedRole: "expanded"
        selectedRole: "selected"
        enabledRole: "enabled"
    }

    property int nativeListItemCount: nativeHierarchyList.itemCount

    property bool nativeHierarchyReady:
        nativeHierarchy.activeListItem !== null
        && nativeHierarchy.modelColumn === 0
        && nativeHierarchy.itemKeyRole === "key"
        && nativeHierarchy.labelRole === "name"
        && nativeHierarchy.activeListItemKey === "native-leaf"
        && nativeHierarchy.activeListItem.label === "Native Leaf"
        && nativeHierarchy.activeListItem.pathLabel === "Native Root / Native Child / Native Leaf"
        && nativeHierarchy.activeListItem.count === 7
        && nativeHierarchy.activeListItem.effectiveShowCount

    property bool nativeListReady:
        nativeHierarchyList.usingTreeModel
        && nativeHierarchyList.itemCount === 3
        && nativeHierarchyList.activeItem !== null
        && nativeHierarchyList.activeItemKey === "native-leaf"
        && nativeHierarchyList.activeItem.label === "Native Leaf"
        && nativeHierarchyList.activeItem.pathLabel === "Native Root / Native Child / Native Leaf"
        && nativeHierarchyList.activeItem.count === 7

    property bool qmlModelListReady:
        qmlModelHierarchyList.usingTreeModel
        && qmlModelHierarchyList.itemCount === 2
        && qmlModelHierarchyList.activeItem !== null
        && qmlModelHierarchyList.activeItemKey === "qml-child"
        && qmlModelHierarchyList.activeItem.label === "QML Child"
        && qmlModelHierarchyList.activeItem.count === 3

    property bool appendedNativeModelReady:
        nativeHierarchyList.itemCount === 4
        && nativeHierarchyList.resolveByKey("native-extra") !== null
        && nativeHierarchyList.resolveByKey("native-extra").label === "Native Extra"

    property bool directModelContractReady:
        nativeHierarchyReady
        && nativeListReady
        && qmlModelListReady
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("directModelContractReady").toBool());

    nativeModel.appendRow(QStringLiteral("native-extra"),
                          QStringLiteral("Native Extra"),
                          0,
                          false,
                          0,
                          true,
                          false);
    IMPORT_WAIT_COMPARE(root->property("nativeListItemCount").toInt(), 4);
    CTEST_WAIT_DEFAULT(root->property("appendedNativeModelReady").toBool());
}

void ImportApiTests::hierarchy_direct_model_editing_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);

    NativeHierarchyModel nativeModel;
    engine.rootContext()->setContextProperty(QStringLiteral("nativeHierarchyModel"), &nativeModel);

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 360
    height: 360

    ListModel {
        id: qmlRows
        ListElement { key: "qml-root"; name: "QML Root"; depth: 0; expanded: true; parentItemKey: "" }
        ListElement { key: "qml-child"; name: "QML Child"; depth: 1; expanded: false; parentItemKey: "qml-root" }
        ListElement { key: "qml-sibling"; name: "QML Sibling"; depth: 0; expanded: false; parentItemKey: "" }
    }

    property int lookupAttempts: 0
    property bool itemsReady: false
    property bool nativeMoveIssued: false
    property bool qmlMoveIssued: false
    property bool nativeModelMovedReady: false
    property bool qmlModelMovedReady: false
    property int moveStateAttempts: 0
    property string readinessState: ""

    function ensureItems() {
        if (itemsReady || lookupAttempts >= 80)
            return

        lookupAttempts += 1
        const nativeRoot = nativeList.resolveByKey("native-root")
        const nativeLeaf = nativeList.resolveByKey("native-leaf")
        const qmlChild = qmlList.resolveByKey("qml-child")
        const qmlSibling = qmlList.resolveByKey("qml-sibling")
        readinessState = "attempts=" + lookupAttempts
            + ", nativeCount=" + nativeList.itemCount
            + ", qmlCount=" + qmlList.itemCount
            + ", nativeEditable=" + nativeList.editableSupported
            + ", qmlEditable=" + qmlList.editableSupported
            + ", nativeRoot=" + (nativeRoot !== null)
            + ", nativeLeaf=" + (nativeLeaf !== null)
            + ", qmlChild=" + (qmlChild !== null)
            + ", qmlSibling=" + (qmlSibling !== null)
        if (!nativeRoot || !nativeLeaf || !qmlChild || !qmlSibling
                || !nativeList.editableSupported
                || !qmlList.editableSupported) {
            Qt.callLater(ensureItems)
            return
        }

        itemsReady = true
    }

    function performNativeModelMove() {
        const leaf = nativeList.resolveByKey("native-leaf")
        const rootItem = nativeList.resolveByKey("native-root")
        if (!leaf || !rootItem)
            return false
        nativeMoveIssued = leaf.moveBefore(rootItem)
        moveStateAttempts = 0
        Qt.callLater(refreshMoveStates)
        return nativeMoveIssued
    }

    function performQmlListModelMove() {
        const child = qmlList.resolveByKey("qml-child")
        const sibling = qmlList.resolveByKey("qml-sibling")
        if (!child || !sibling)
            return false
        qmlMoveIssued = child.moveAfter(sibling)
        moveStateAttempts = 0
        Qt.callLater(refreshMoveStates)
        return qmlMoveIssued
    }

    function refreshMoveStates() {
        nativeModelMovedReady =
            nativeMoveIssued
            && nativeList.resolveByKey("native-leaf") !== null
            && nativeList.resolveByKey("native-leaf").flatIndex === 0
            && nativeList.resolveByKey("native-leaf").indentLevel === 0
            && nativeList.resolveByKey("native-root") !== null
            && nativeList.resolveByKey("native-root").flatIndex === 1

        qmlModelMovedReady =
            qmlMoveIssued
            && qmlRows.get(2).key === "qml-child"
            && qmlRows.get(2).depth === 0
            && qmlRows.get(2).parentItemKey === ""
            && qmlList.resolveByKey("qml-child") !== null
            && qmlList.resolveByKey("qml-child").flatIndex === 2
            && qmlList.resolveByKey("qml-child").indentLevel === 0

        if ((!nativeModelMovedReady && nativeMoveIssued) || (!qmlModelMovedReady && qmlMoveIssued)) {
            if (moveStateAttempts < 80) {
                moveStateAttempts += 1
                Qt.callLater(refreshMoveStates)
            }
        }
    }

    LV.HierarchyList {
        id: nativeList
        width: 260
        height: 260
        editable: true
        model: nativeHierarchyModel
        itemKeyRole: "key"
        labelRole: "name"
        depthRole: "depth"
        countRole: "counter"
        expandedRole: "expanded"
        selectedRole: "selected"
        enabledRole: "enabled"
    }

    LV.HierarchyList {
        id: qmlList
        x: 280
        width: 260
        height: 260
        editable: true
        model: qmlRows
        itemKeyRole: "key"
        labelRole: "name"
        depthRole: "depth"
        expandedRole: "expanded"
    }

    Component.onCompleted: Qt.callLater(root.ensureItems)
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("itemsReady").toBool(),
                 qPrintable(root->property("readinessState").toString()));

    QVariant nativeMovePerformed;
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "performNativeModelMove",
                                      Q_RETURN_ARG(QVariant, nativeMovePerformed)));
    CTEST_CHECK(nativeMovePerformed.toBool());
    CTEST_WAIT_DEFAULT(root->property("nativeModelMovedReady").toBool());

    QVariant qmlMovePerformed;
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "performQmlListModelMove",
                                      Q_RETURN_ARG(QVariant, qmlMovePerformed)));
    CTEST_CHECK(qmlMovePerformed.toBool());
    CTEST_WAIT_DEFAULT(root->property("qmlModelMovedReady").toBool());
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
            { key: "root", depth: 0, label: "Root", iconName: "projectStructure", expanded: true },
            { key: "child", depth: 1, label: "Child", iconName: "viewMoreSymbolicDefault", expanded: false },
            { key: "grand", depth: 2, label: "Grand", iconName: "viewMoreSymbolicBorderless", expanded: false },
            { key: "great", depth: 3, label: "Great", iconName: "viewMoreSymbolicDisabled" }
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
            && rootItem.rowHeight === 20
            && childItem.rowHeight === 20
            && grandItem.rowHeight === 20
            && greatItem.rowHeight === 20
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("nestedContractReady").toBool());
}

void ImportApiTests::hierarchy_expansion_uses_incremental_visibility_refresh()
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
    height: 260

    property int lookupAttempts: 0
    property bool collapseReady: false
    property bool expandReady: false
    property bool fastRefreshContractReady: false
    property string readinessState: ""
    property int initialCreated: 0
    property int createdAfterCollapse: 0
    property int destroyedAfterCollapse: 0
    property int createdDuringToggle: 0
    property int destroyedDuringToggle: 0
    property int rowVisibleChangedDuringCollapse: 0
    property int visibleIndexChangedDuringCollapse: 0
    property int initialFullProjectionCount: -1
    property int initialIncrementalRefreshCount: -1
    property int fullProjectionAfterCollapse: -1
    property int incrementalAfterCollapse: -1
    property int fullProjectionAfterExpand: -1
    property int incrementalAfterExpand: -1
    property int fullBeforeToggle: -1
    property int incrementalBeforeToggle: -1

    property var rows: [
        { key: "root", depth: 0, label: "Root", expanded: true },
        { key: "a", depth: 1, label: "A", expanded: true },
        { key: "a1", depth: 2, label: "A1" },
        { key: "a-folder", depth: 2, label: "A Folder", expanded: false },
        { key: "a-folder-leaf", depth: 3, label: "A Folder Leaf" },
        { key: "b", depth: 1, label: "B", expanded: true },
        { key: "b1", depth: 2, label: "B1" },
        { key: "tail", depth: 0, label: "Tail" }
    ]

    function counterValue(name) {
        const value = hierarchyList[name]
        if (value === undefined || value === null)
            return -1
        return Number(value)
    }

    function captureItemState(key) {
        const item = hierarchyList.resolveByKey(key)
        if (!item)
            return key + ":missing"
        return key
            + ":visible=" + item.rowVisible
            + ",visibleIndex=" + item.visibleIndex
            + ",visibleChildCount=" + item.visibleChildCount
            + ",visibleDescendantCount=" + item.visibleDescendantCount
    }

    function ensureItems() {
        if (fastRefreshContractReady || lookupAttempts >= 80)
            return

        lookupAttempts += 1
        hierarchyList.ensureStateUpToDate()
        const branch = hierarchyList.resolveByKey("a")
        const folder = hierarchyList.resolveByKey("a-folder")
        const nestedLeaf = hierarchyList.resolveByKey("a-folder-leaf")
        const tail = hierarchyList.resolveByKey("tail")
        readinessState = "attempts=" + lookupAttempts
            + ", itemCount=" + hierarchyList.itemCount
            + ", visibleItemCount=" + hierarchyList.visibleItemCount
            + ", full=" + counterValue("_fullProjectionRefreshCount")
            + ", incremental=" + counterValue("_incrementalVisibilityRefreshCount")
            + ", refreshScheduled=" + hierarchyList._refreshScheduled
            + ", normalizeScheduled=" + hierarchyList._normalizeScheduled
            + ", rebuildScheduled=" + hierarchyList._rebuildScheduled
            + ", itemsDirty=" + hierarchyList._itemsDirty
            + ", a=" + (branch !== null)
            + ", folder=" + (folder !== null)
            + ", nestedLeaf=" + (nestedLeaf !== null)
            + ", tail=" + (tail !== null)
        if (hierarchyList.itemCount !== 8
                || !branch || !folder || !nestedLeaf || !tail
                || hierarchyList._refreshScheduled
                || hierarchyList._normalizeScheduled
                || hierarchyList._rebuildScheduled
                || hierarchyList._itemsDirty) {
            Qt.callLater(ensureItems)
            return
        }

        initialCreated = delegateStats.created
        initialFullProjectionCount = counterValue("_fullProjectionRefreshCount")
        initialIncrementalRefreshCount = counterValue("_incrementalVisibilityRefreshCount")
        fullBeforeToggle = initialFullProjectionCount
        incrementalBeforeToggle = initialIncrementalRefreshCount
        delegateStats.created = 0
        delegateStats.destroyed = 0
        delegateStats.rowVisibleChanged = 0
        delegateStats.visibleIndexChanged = 0
        branch.expanded = false
        Qt.callLater(captureCollapse)
    }

    function captureCollapse() {
        hierarchyList.ensureStateUpToDate()
        fullProjectionAfterCollapse = counterValue("_fullProjectionRefreshCount")
        incrementalAfterCollapse = counterValue("_incrementalVisibilityRefreshCount")
        createdAfterCollapse = delegateStats.created
        destroyedAfterCollapse = delegateStats.destroyed
        rowVisibleChangedDuringCollapse = delegateStats.rowVisibleChanged
        visibleIndexChangedDuringCollapse = delegateStats.visibleIndexChanged

        const rootItem = hierarchyList.resolveByKey("root")
        const branch = hierarchyList.resolveByKey("a")
        const a1 = hierarchyList.resolveByKey("a1")
        const folder = hierarchyList.resolveByKey("a-folder")
        const nestedLeaf = hierarchyList.resolveByKey("a-folder-leaf")
        const b = hierarchyList.resolveByKey("b")
        const b1 = hierarchyList.resolveByKey("b1")
        const tail = hierarchyList.resolveByKey("tail")

        collapseReady =
            initialCreated === 8
            && initialFullProjectionCount >= 1
            && fullProjectionAfterCollapse === fullBeforeToggle
            && incrementalAfterCollapse > incrementalBeforeToggle
            && createdAfterCollapse === 0
            && destroyedAfterCollapse === 0
            && rowVisibleChangedDuringCollapse === 2
            && visibleIndexChangedDuringCollapse >= 5
            && hierarchyList.visibleItemCount === 5
            && rootItem.visibleDescendantCount === 3
            && branch.visibleDescendantCount === 0
            && !a1.rowVisible
            && !folder.rowVisible
            && !nestedLeaf.rowVisible
            && b.rowVisible
            && b.visibleIndex === 2
            && b1.visibleIndex === 3
            && tail.visibleIndex === 4

        if (!collapseReady) {
            readinessState = "collapse failed: "
                + "initialCreated=" + initialCreated
                + ", fullBefore=" + fullBeforeToggle
                + ", fullAfter=" + fullProjectionAfterCollapse
                + ", incrementalBefore=" + incrementalBeforeToggle
                + ", incrementalAfter=" + incrementalAfterCollapse
                + ", created=" + createdAfterCollapse
                + ", destroyed=" + destroyedAfterCollapse
                + ", rowVisibleChanged=" + rowVisibleChangedDuringCollapse
                + ", visibleIndexChanged=" + visibleIndexChangedDuringCollapse
                + ", root=" + captureItemState("root")
                + ", a=" + captureItemState("a")
                + ", a1=" + captureItemState("a1")
                + ", folder=" + captureItemState("a-folder")
                + ", nestedLeaf=" + captureItemState("a-folder-leaf")
                + ", b=" + captureItemState("b")
                + ", b1=" + captureItemState("b1")
                + ", tail=" + captureItemState("tail")
            return
        }

        fullBeforeToggle = fullProjectionAfterCollapse
        incrementalBeforeToggle = incrementalAfterCollapse
        delegateStats.created = 0
        delegateStats.destroyed = 0
        const branchForExpand = hierarchyList.resolveByKey("a")
        branchForExpand.expanded = true
        Qt.callLater(captureExpand)
    }

    function captureExpand() {
        hierarchyList.ensureStateUpToDate()
        fullProjectionAfterExpand = counterValue("_fullProjectionRefreshCount")
        incrementalAfterExpand = counterValue("_incrementalVisibilityRefreshCount")
        createdDuringToggle = delegateStats.created
        destroyedDuringToggle = delegateStats.destroyed

        const rootItem = hierarchyList.resolveByKey("root")
        const branch = hierarchyList.resolveByKey("a")
        const a1 = hierarchyList.resolveByKey("a1")
        const folder = hierarchyList.resolveByKey("a-folder")
        const nestedLeaf = hierarchyList.resolveByKey("a-folder-leaf")
        const b = hierarchyList.resolveByKey("b")
        const b1 = hierarchyList.resolveByKey("b1")
        const tail = hierarchyList.resolveByKey("tail")

        expandReady =
            fullProjectionAfterExpand === fullBeforeToggle
            && incrementalAfterExpand > incrementalBeforeToggle
            && createdDuringToggle === 0
            && destroyedDuringToggle === 0
            && hierarchyList.visibleItemCount === 7
            && rootItem.visibleDescendantCount === 5
            && branch.visibleDescendantCount === 2
            && a1.rowVisible
            && folder.rowVisible
            && !nestedLeaf.rowVisible
            && b.visibleIndex === 4
            && b1.visibleIndex === 5
            && tail.visibleIndex === 6

        readinessState = "collapseReady=" + collapseReady
            + ", expandReady=" + expandReady
            + ", fullCollapse=" + fullProjectionAfterCollapse
            + ", fullExpand=" + fullProjectionAfterExpand
            + ", incrementalCollapse=" + incrementalAfterCollapse
            + ", incrementalExpand=" + incrementalAfterExpand
            + ", root=" + captureItemState("root")
            + ", a=" + captureItemState("a")
            + ", folder=" + captureItemState("a-folder")
            + ", nestedLeaf=" + captureItemState("a-folder-leaf")
            + ", b=" + captureItemState("b")
            + ", b1=" + captureItemState("b1")
            + ", tail=" + captureItemState("tail")
        fastRefreshContractReady = collapseReady && expandReady
    }

    QtObject {
        id: delegateStats
        property int created: 0
        property int destroyed: 0
        property int rowVisibleChanged: 0
        property int visibleIndexChanged: 0
    }

    LV.HierarchyList {
        id: hierarchyList
        width: 260
        model: root.rows
        itemDelegate: Component {
            LV.HierarchyItem {
                Component.onCompleted: delegateStats.created += 1
                Component.onDestruction: delegateStats.destroyed += 1
                onRowVisibleChanged: delegateStats.rowVisibleChanged += 1
                onVisibleIndexChanged: delegateStats.visibleIndexChanged += 1
            }
        }
    }

    Component.onCompleted: Qt.callLater(root.ensureItems)
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("fastRefreshContractReady").toBool(),
                 qPrintable(root->property("readinessState").toString()));
}

void ImportApiTests::hierarchy_editable_drag_depth_contract_loads()
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
    height: 240

    property int lookupAttempts: 0
    property bool dragItemReady: false
    property int startedCount: 0
    property int updatedCount: 0
    property int endedCount: 0
    property int movedItemId: -1
    property string movedKey: ""
    property int movedFromIndex: -1
    property int movedToIndex: -1
    property int movedDepth: -1
    property string movedModeName: ""
    property string movedParentKey: ""
    property string movedAnchorKey: ""
    property int startedIndex: -1
    property int startedEndIndex: -1
    property int startedDepth: -1
    property int updatedIndex: -1
    property int updatedDepth: -1
    property string updatedModeName: ""
    property string updatedParentKey: ""
    property string updatedAnchorKey: ""
    property bool lastCommitted: false
    property bool previewValid: false
    property int previewIndex: -1
    property int previewDepth: -1
    property string previewModeName: ""
    property string previewParentKey: ""
    property string previewAnchorKey: ""
    property var dragItem: null

    property var treeModel: [
        { key: "root", depth: 0, label: "Root", expanded: true },
        { key: "branch", depth: 1, label: "Branch", expanded: true },
        { key: "leaf", depth: 2, label: "Leaf" },
        { key: "sibling", depth: 0, label: "Sibling", expanded: true }
    ]

    function ensureDragItem() {
        if (dragItemReady || lookupAttempts >= 40)
            return
        lookupAttempts += 1
        const candidate = hierarchyList.resolveByKey("branch")
        if (!candidate) {
            Qt.callLater(ensureDragItem)
            return
        }
        dragItem = candidate
        dragItemReady = true
    }

    function performEditableDrag() {
        if (!dragItem)
            return false

        const siblingItem = hierarchyList.resolveByKey("sibling")
        if (!siblingItem)
            return false

        if (!dragItem.beginDrag(dragItem.width * 0.5, dragItem.height * 0.5))
            return false

        const desiredListPoint = Qt.point(siblingItem.baseLeftPadding + siblingItem.indentStep + 1,
                                          siblingItem.y + siblingItem.height + 1)
        const localUpdatePoint = dragItem.mapFromItem(hierarchyList, desiredListPoint.x, desiredListPoint.y)
        const updated = dragItem.updateDrag(localUpdatePoint.x, localUpdatePoint.y)
        previewValid = dragItem.dragTargetValid
        previewIndex = dragItem.dragTargetIndex
        previewDepth = dragItem.dragTargetDepth
        previewModeName = dragItem.dragTargetModeName
        previewParentKey = dragItem.dragTargetParentItemKey
        previewAnchorKey = dragItem.dragTargetAnchorItemKey
        if (!updated)
            return false

        return dragItem.commitDrag()
    }

    LV.Hierarchy {
        id: hierarchyAliasProbe
        visible: false
        editable: true
        model: []
    }

    LV.HierarchyList {
        id: hierarchyList
        width: parent.width
        height: parent.height
        editable: true
        model: root.treeModel

        onItemMoved: function(item, itemId, itemKey, fromIndex, toIndex, depth) {
            root.movedItemId = itemId
            root.movedKey = itemKey
            root.movedFromIndex = fromIndex
            root.movedToIndex = toIndex
            root.movedDepth = depth
        }
    }

    Connections {
        target: root.dragItem
        function onDragStarted(sourceIndex, sourceEndIndex, sourceDepth) {
            root.startedCount += 1
            root.startedIndex = sourceIndex
            root.startedEndIndex = sourceEndIndex
            root.startedDepth = sourceDepth
        }
        function onDragUpdated(targetIndex, targetDepth, modeName, parentItemKey, anchorItemKey) {
            root.updatedCount += 1
            root.updatedIndex = targetIndex
            root.updatedDepth = targetDepth
            root.updatedModeName = modeName
            root.updatedParentKey = parentItemKey
            root.updatedAnchorKey = anchorItemKey
        }
        function onDragEnded(committed, fromIndex, toIndex, targetDepth, modeName, parentItemKey, anchorItemKey) {
            root.endedCount += 1
            root.lastCommitted = committed
            root.movedFromIndex = fromIndex
            root.movedToIndex = toIndex
            root.movedDepth = targetDepth
            root.movedModeName = modeName
            root.movedParentKey = parentItemKey
            root.movedAnchorKey = anchorItemKey
        }
    }

    Component.onCompleted: Qt.callLater(root.ensureDragItem)

    property bool dragContractReady:
        hierarchyAliasProbe.editable
        && hierarchyList.editable
        && dragItemReady
        && startedCount === 1
        && updatedCount >= 1
        && endedCount === 1
        && startedIndex === 1
        && startedEndIndex === 2
        && startedDepth === 1
        && previewValid
        && previewIndex === 2
        && previewDepth === 1
        && previewModeName === "child"
        && previewParentKey === "sibling"
        && previewAnchorKey === "sibling"
        && updatedIndex === 2
        && updatedDepth === 1
        && updatedModeName === "child"
        && updatedParentKey === "sibling"
        && updatedAnchorKey === "sibling"
        && lastCommitted
        && movedItemId === 1
        && movedKey === "branch"
        && movedFromIndex === 1
        && movedToIndex === 2
        && movedDepth === 1
        && movedModeName === "child"
        && movedParentKey === "sibling"
        && movedAnchorKey === "sibling"
        && treeModel.length === 4
        && treeModel[0].key === "root"
        && treeModel[0].depth === 0
        && treeModel[0].parentKey === ""
        && treeModel[0].parentItemKey === ""
        && treeModel[1].key === "sibling"
        && treeModel[1].depth === 0
        && treeModel[1].parentKey === ""
        && treeModel[1].parentItemKey === ""
        && treeModel[2].key === "branch"
        && treeModel[2].depth === 1
        && treeModel[2].parentKey === "sibling"
        && treeModel[2].parentItemKey === "sibling"
        && treeModel[3].key === "leaf"
        && treeModel[3].depth === 2
        && treeModel[3].parentKey === "branch"
        && treeModel[3].parentItemKey === "branch"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("dragItemReady").toBool());
    QVariant dragPerformed;
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "performEditableDrag",
                                      Q_RETURN_ARG(QVariant, dragPerformed)));
    CTEST_CHECK(dragPerformed.toBool());
    CTEST_WAIT_DEFAULT(root->property("dragContractReady").toBool());
}

void ImportApiTests::hierarchy_editable_drag_per_item_lock_contract_loads()
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
    height: 240

    property int lookupAttempts: 0
    property bool itemsReady: false
    property var lockedItem: null
    property var freeItem: null
    property var fallbackItem: null

    property var customModel: [
        { key: "root", depth: 0, label: "Root", expanded: true, movable: true },
        { key: "locked", depth: 1, label: "Locked", movable: false },
        { key: "free", depth: 1, label: "Free", movable: true }
    ]
    property var fallbackModel: [
        { key: "fallback", depth: 0, label: "Fallback", dragAllowed: false }
    ]

    function ensureItems() {
        if (itemsReady || lookupAttempts >= 40)
            return

        lookupAttempts += 1
        const locked = hierarchyList.resolveByKey("locked")
        const free = hierarchyList.resolveByKey("free")
        const fallback = fallbackList.resolveByKey("fallback")
        if (!locked || !free || !fallback) {
            Qt.callLater(ensureItems)
            return
        }

        lockedItem = locked
        freeItem = free
        fallbackItem = fallback
        itemsReady = true
    }

    function evaluateDragLockContract() {
        if (!lockedItem || !freeItem || !fallbackItem)
            return false

        const lockedStarted = lockedItem.beginDrag(lockedItem.width * 0.5, lockedItem.height * 0.5)
        const freeStarted = freeItem.beginDrag(freeItem.width * 0.5, freeItem.height * 0.5)
        const fallbackStarted = fallbackItem.beginDrag(fallbackItem.width * 0.5, fallbackItem.height * 0.5)
        if (freeStarted)
            freeItem.cancelDrag()

        return hierarchyAliasProbe.draggableRole === "movable"
            && hierarchyList.draggableRole === "movable"
            && !lockedItem.dragEnabled
            && !lockedStarted
            && freeItem.dragEnabled
            && freeStarted
            && !fallbackItem.dragEnabled
            && !fallbackStarted
    }

    LV.Hierarchy {
        id: hierarchyAliasProbe
        visible: false
        editable: true
        draggableRole: "movable"
        model: root.customModel
    }

    LV.HierarchyList {
        id: hierarchyList
        width: parent.width
        height: parent.height
        editable: true
        draggableRole: hierarchyAliasProbe.draggableRole
        model: root.customModel
    }

    LV.HierarchyList {
        id: fallbackList
        visible: false
        width: 1
        height: 1
        editable: true
        model: root.fallbackModel
    }

    Component.onCompleted: Qt.callLater(root.ensureItems)
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("itemsReady").toBool());
    QVariant dragLockContractReady;
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "evaluateDragLockContract",
                                      Q_RETURN_ARG(QVariant, dragLockContractReady)));
    CTEST_CHECK(dragLockContractReady.toBool());
}

void ImportApiTests::hierarchy_mobile_drag_hold_contract_loads()
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
    height: 240

    Component.onCompleted: {
        LV.Theme.targetOverride = "ios"
        Qt.callLater(root.ensureDragItem)
    }
    Component.onDestruction: LV.Theme.targetOverride = ""

    property int lookupAttempts: 0
    property bool itemsReady: false
    property var mobileItem: null

    property var treeModel: [
        { key: "root", depth: 0, label: "Root", expanded: true },
        { key: "branch", depth: 1, label: "Branch", expanded: true },
        { key: "leaf", depth: 2, label: "Leaf" },
        { key: "sibling", depth: 0, label: "Sibling", expanded: true }
    ]

    function ensureDragItem() {
        if (itemsReady || lookupAttempts >= 40)
            return

        lookupAttempts += 1
        const candidate = hierarchyList.resolveByKey("branch")
        if (!candidate) {
            Qt.callLater(ensureDragItem)
            return
        }

        mobileItem = candidate
        itemsReady = true
    }

    function performProgrammaticDrag() {
        if (!mobileItem)
            return false

        const siblingItem = hierarchyList.resolveByKey("sibling")
        if (!siblingItem)
            return false

        if (!mobileItem.beginDrag(mobileItem.width * 0.5, mobileItem.height * 0.5))
            return false

        const desiredListPoint = Qt.point(siblingItem.baseLeftPadding + siblingItem.indentStep + 1,
                                          siblingItem.y + siblingItem.height + 1)
        const localUpdatePoint = mobileItem.mapFromItem(hierarchyList, desiredListPoint.x, desiredListPoint.y)
        const updated = mobileItem.updateDrag(localUpdatePoint.x, localUpdatePoint.y)
        if (!updated)
            return false

        return mobileItem.commitDrag()
    }

    LV.HierarchyList {
        id: hierarchyList
        width: parent.width
        height: parent.height
        editable: true
        model: root.treeModel
    }

    property bool mobileHoldContractReady:
        LV.Theme.mobileTarget
        && itemsReady
        && mobileItem !== null
        && mobileItem.dragEnabled
        && mobileItem.pointerDragRequiresLongPress
        && !mobileItem.immediatePointerDragEnabled
        && mobileItem.mobileDragHoldInterval === 1000
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("mobileHoldContractReady").toBool());
    QVariant dragPerformed;
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "performProgrammaticDrag",
                                      Q_RETURN_ARG(QVariant, dragPerformed)));
    CTEST_CHECK(dragPerformed.toBool());
}

void ImportApiTests::hierarchy_mobile_activation_commits_on_release_contract_loads()
{
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
    height: 240
    visible: true

    Component.onCompleted: {
        LV.Theme.targetOverride = "ios"
        Qt.callLater(root.ensureItems)
    }
    Component.onDestruction: LV.Theme.targetOverride = ""

    property int lookupAttempts: 0
    property bool itemsReady: false
    property var rootItemRef: null
    property var branchItemRef: null

    property var treeModel: [
        { key: "root", depth: 0, label: "Root", expanded: true, selected: true },
        { key: "branch", depth: 1, label: "Branch" },
        { key: "sibling", depth: 0, label: "Sibling" }
    ]

    function ensureItems() {
        if (itemsReady || lookupAttempts >= 40)
            return

        lookupAttempts += 1
        const rootCandidate = hierarchyList.resolveByKey("root")
        const branchCandidate = hierarchyList.resolveByKey("branch")
        if (!rootCandidate || !branchCandidate) {
            Qt.callLater(ensureItems)
            return
        }

        rootItemRef = rootCandidate
        branchItemRef = branchCandidate
        itemsReady = true
    }

    LV.HierarchyList {
        id: hierarchyList
        objectName: "hierarchyList"
        anchors.fill: parent
        editable: true
        model: root.treeModel
    }

    property bool mobileReleaseActivationContractReady:
        LV.Theme.mobileTarget
        && itemsReady
        && hierarchyList.activeItem === rootItemRef
        && branchItemRef !== null
        && branchItemRef.pointerDragRequiresLongPress
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    window->show();
    CTEST_WAIT_DEFAULT(window->isVisible());
    CTEST_WAIT_DEFAULT(root->property("mobileReleaseActivationContractReady").toBool());

    auto *list = root->findChild<QObject *>(QStringLiteral("hierarchyList"));
    CTEST_CHECK(list);
    QObject *rootItemObject = root->property("rootItemRef").value<QObject *>();
    QObject *branchItemObject = root->property("branchItemRef").value<QObject *>();
    CTEST_CHECK(rootItemObject);
    CTEST_CHECK(branchItemObject);
    auto *branchItem = qobject_cast<QQuickItem *>(branchItemObject);
    CTEST_CHECK(branchItem);

    const QPointF branchPoint = branchItem->mapToScene(QPointF(branchItem->width() * 0.5,
                                                               branchItem->height() * 0.5));
    const QPoint branchPointInt(qRound(branchPoint.x()), qRound(branchPoint.y()));

    mousePress(window, Qt::LeftButton, Qt::NoModifier, branchPointInt, 10);
    QCoreApplication::processEvents();
    processFor(50);

    IMPORT_COMPARE(list->property("activeItem").value<QObject *>(), rootItemObject);
    IMPORT_COMPARE(list->property("activeItemKey").toString(), QStringLiteral("root"));
    CTEST_CHECK(!branchItemObject->property("active").toBool());

    mouseRelease(window, Qt::LeftButton, Qt::NoModifier, branchPointInt, 10);

    IMPORT_WAIT_COMPARE(list->property("activeItemKey").toString(), QStringLiteral("branch"));
    CTEST_WAIT_DEFAULT(list->property("activeItem").value<QObject *>() != nullptr);
    QObject *activeBranchObject = list->property("activeItem").value<QObject *>();
    IMPORT_COMPARE(activeBranchObject->property("itemKey").toString(), QStringLiteral("branch"));
    CTEST_WAIT_DEFAULT(activeBranchObject->property("active").toBool());
}

void ImportApiTests::hierarchy_mobile_reactivation_reemits_active_signal()
{
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
    height: 240
    visible: true

    Component.onCompleted: {
        LV.Theme.targetOverride = "ios"
        Qt.callLater(root.captureActiveItem)
    }
    Component.onDestruction: LV.Theme.targetOverride = ""

    property int lookupAttempts: 0
    property var rootItemRef: null

    LV.Hierarchy {
        id: hierarchy
        objectName: "hierarchy"
        anchors.fill: parent
        editable: true
        model: [
            { key: "root", depth: 0, label: "Root", expanded: true, selected: true },
            { key: "branch", depth: 1, label: "Branch" },
            { key: "sibling", depth: 0, label: "Sibling" }
        ]
    }

    function captureActiveItem() {
        if (rootItemRef || lookupAttempts >= 40)
            return

        lookupAttempts += 1
        if (!hierarchy.activeListItem || hierarchy.activeListItemKey !== "root") {
            Qt.callLater(captureActiveItem)
            return
        }

        rootItemRef = hierarchy.activeListItem
    }

    property bool mobileRetapContractReady:
        LV.Theme.mobileTarget
        && rootItemRef !== null
        && hierarchy.activeListItem === rootItemRef
        && hierarchy.activeListItemKey === "root"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    window->show();
    CTEST_WAIT_DEFAULT(window->isVisible());
    CTEST_WAIT_DEFAULT(root->property("mobileRetapContractReady").toBool());

    auto *hierarchy = root->findChild<QObject *>(QStringLiteral("hierarchy"));
    CTEST_CHECK(hierarchy);
    QObject *rootItemObject = root->property("rootItemRef").value<QObject *>();
    CTEST_CHECK(rootItemObject);
    auto *rootItem = qobject_cast<QQuickItem *>(rootItemObject);
    CTEST_CHECK(rootItem);

    ImportSignalRecorder activationSpy;
    const auto activationSpyConnection = QObject::connect(hierarchy, SIGNAL(listItemActivated(QVariant,int,int)), &activationSpy, SLOT(recordActivation(QVariant,int,int)));
    CTEST_CHECK(activationSpyConnection);
    CTEST_CHECK(bool(activationSpyConnection));

    const QPointF rootPoint = rootItem->mapToScene(QPointF(rootItem->width() * 0.5,
                                                           rootItem->height() * 0.5));
    const QPoint rootPointInt(qRound(rootPoint.x()), qRound(rootPoint.y()));

    mousePress(window, Qt::LeftButton, Qt::NoModifier, rootPointInt, 10);
    QCoreApplication::processEvents();
    processFor(50);
    IMPORT_COMPARE(activationSpy.events.size(), 0);

    mouseRelease(window, Qt::LeftButton, Qt::NoModifier, rootPointInt, 10);

    IMPORT_WAIT_COMPARE(activationSpy.events.size(), 1);
    IMPORT_COMPARE(hierarchy->property("activeListItem").value<QObject *>(), rootItemObject);
    IMPORT_COMPARE(hierarchy->property("activeListItemKey").toString(), QStringLiteral("root"));
}

void ImportApiTests::hierarchy_mobile_scroll_physics_contract_loads()
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
    height: 260

    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.Hierarchy {
        id: hierarchy
        objectName: "hierarchy"
        anchors.fill: parent
        toolbarItems: [
            { id: "structure", iconName: "projectStructure" }
        ]
        model: [
            { key: "row-01", depth: 0, label: "Row 01", expanded: true },
            { key: "row-02", depth: 0, label: "Row 02", expanded: true },
            { key: "row-03", depth: 0, label: "Row 03", expanded: true },
            { key: "row-04", depth: 0, label: "Row 04", expanded: true },
            { key: "row-05", depth: 0, label: "Row 05", expanded: true },
            { key: "row-06", depth: 0, label: "Row 06", expanded: true },
            { key: "row-07", depth: 0, label: "Row 07", expanded: true },
            { key: "row-08", depth: 0, label: "Row 08", expanded: true },
            { key: "row-09", depth: 0, label: "Row 09", expanded: true },
            { key: "row-10", depth: 0, label: "Row 10", expanded: true },
            { key: "row-11", depth: 0, label: "Row 11", expanded: true },
            { key: "row-12", depth: 0, label: "Row 12", expanded: true },
            { key: "row-13", depth: 0, label: "Row 13", expanded: true },
            { key: "row-14", depth: 0, label: "Row 14", expanded: true }
        ]
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);

    QObject *hierarchy = root->findChild<QObject *>(QStringLiteral("hierarchy"));
    CTEST_CHECK(hierarchy);
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("hierarchyListViewportFlickable"));
    CTEST_CHECK(viewport);

    IMPORT_WAIT_COMPARE(viewport->property("boundsBehavior").toInt(), kFlickableDragAndOvershootBounds);
    IMPORT_COMPARE(viewport->property("boundsMovement").toInt(), kFlickableFollowBoundsBehavior);
    IMPORT_COMPARE(viewport->property("boundsBehavior").toInt(), hierarchy->property("listBoundsBehavior").toInt());
    IMPORT_COMPARE(viewport->property("boundsMovement").toInt(), hierarchy->property("listBoundsMovement").toInt());
    IMPORT_COMPARE(viewport->property("flickDeceleration").toInt(), hierarchy->property("listFlickDeceleration").toInt());
    IMPORT_COMPARE(viewport->property("maximumFlickVelocity").toInt(), hierarchy->property("listMaximumFlickVelocity").toInt());
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
            { key: "root", depth: 0, label: "Root", iconName: "projectStructure", expanded: true },
            { key: "child", depth: 1, label: "Child", iconName: "viewMoreSymbolicDefault" }
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
        && hierarchy.model.length === 2
        && footerTriggerResult
        && footerSignalCount === 1
        && footerCallbackCount === 1
        && footerIconName === "projectStructure"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("footerContractReady").toBool());
}

void ImportApiTests::hierarchy_toolbar_empty_state_does_not_reschedule()
{
    QQmlEngine engine;
    engine.addImportPath(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/.."));
    QScopedPointer<QObject> toolbar(createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.HierarchyToolbar {}
)"));
    CTEST_CHECK(toolbar);
    ImportSignalRecorder idChanges;
    const auto idChangesConnection = QObject::connect(toolbar.data(), SIGNAL(activeButtonIdChanged()), &idChanges, SLOT(recordVoid()));
    CTEST_CHECK(idChangesConnection);
    CTEST_CHECK(bool(idChangesConnection));
    for (int i = 0; i < 8; ++i)
        CTEST_CHECK(QMetaObject::invokeMethod(toolbar.data(), "normalizeActiveButton"));
    IMPORT_COMPARE(toolbar->property("activeButtonId").toInt(), -1);
    IMPORT_COMPARE(idChanges.events.size(), 0);
    processFor(30);
    CTEST_CHECK(!toolbar->property("_normalizeScheduled").toBool());
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("toolbarContractReady").toBool());
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
        { id: "slot3", iconName: "projectStructure" }
    ]

    LV.HierarchyToolbar {
        id: toolbar
        width: 200
        height: implicitHeight
        buttonItems: root.toolbarItems
    }

    function approximatelyEqual(leftValue, rightValue, tolerance) {
        return Math.abs(leftValue - rightValue) <= tolerance
    }

    function evaluateLayoutContract() {
        const buttons = toolbar.collectButtons().slice().sort(function(leftButton, rightButton) {
            return leftButton.x - rightButton.x
        })
        if (buttons.length !== 4) {
            root.layoutDiagnostics = "buttonCount=" + buttons.length
            return false
        }

        const expectedX = [8.0, 30.0, 52.0, 74.0]

        for (let index = 0; index < expectedX.length; index++) {
            const button = buttons[index]
            if (!button || !button.visible)
                return false
            const buttonPosition = button.mapToItem(toolbar, 0, 0)
            if (!approximatelyEqual(buttonPosition.x, expectedX[index], 0.8)) {
                root.layoutDiagnostics = "xMismatch index=" + index
                    + " actual=" + buttonPosition.x
                    + " expected=" + expectedX[index]
                    + " spacing=" + toolbar.distributedSpacing
                return false
            }
            if (!approximatelyEqual(button.width, 22.0, 0.2)) {
                root.layoutDiagnostics = "widthMismatch index=" + index + " width=" + button.width
                return false
            }
            if (!approximatelyEqual(button.height, 22.0, 0.2)) {
                root.layoutDiagnostics = "heightMismatch index=" + index + " height=" + button.height
                return false
            }
        }
        const ready = toolbar.minimumToolbarWidth === 200
            && toolbar.horizontalPadding === 8
            && toolbar.verticalPadding === 2
            && toolbar.slotSize === 22
            && toolbar.spacing === 0
            && !toolbar.distributeSpacing
            && toolbar.implicitWidth === 200
            && toolbar.implicitHeight === 26
            && toolbar.height === 26
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
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("layoutContractReady").toBool(),
                 qPrintable(root->property("layoutDiagnostics").toString()));
}

void ImportApiTests::hierarchy_figma_composition_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 240
    height: 560

    property var rows: [
        { key: "row-01", label: "Label", depth: 0, expanded: true },
        { key: "row-02", label: "Value", depth: 0 },
        { key: "row-03", label: "Description", depth: 0 },
        { key: "row-04", label: "Description", depth: 0 },
        { key: "row-05", label: "Description", depth: 0 },
        { key: "row-06", label: "Description", depth: 0 },
        { key: "row-07", label: "Description", depth: 0 },
        { key: "row-08", label: "Description", depth: 0 },
        { key: "row-09", label: "Description", depth: 0 },
        { key: "row-10", label: "Description", depth: 0 },
        { key: "row-11", label: "Description", depth: 0 },
        { key: "row-12", label: "Description", depth: 0 },
        { key: "row-13", label: "Description", depth: 0 },
        { key: "row-14", label: "Description", depth: 0 },
        { key: "row-15", label: "Description", depth: 0 },
        { key: "row-16", label: "Description", depth: 0 }
    ]

    LV.HierarchyList {
        id: list
        objectName: "figmaHierarchyList"
        width: 200
        model: root.rows
        opacity: 0
    }

    LV.Hierarchy {
        id: hierarchy
        objectName: "figmaHierarchy"
        width: implicitWidth
        height: implicitHeight
        opacity: 0
        toolbarItems: [
            { id: "slot0", iconName: "projectStructure", selected: true },
            { id: "slot1", iconName: "projectStructure" },
            { id: "slot2", iconName: "projectStructure" },
            { id: "slot3", iconName: "projectStructure" }
        ]
        model: root.rows
    }

    property bool contractReady:
        list.itemCount === 16
        && list.visibleItemCount === 16
        && list.generatedIndentStep === 8
        && list.generatedRowHeight === 20
        && list.generatedItemWidth === 200
        && list.generatedIconSize === 18
        && list.generatedChevronSize === 16
        && list.implicitWidth === 200
        && list.implicitHeight === 320
        && hierarchy.minimumPanelWidth === 200
        && hierarchy.minimumPanelHeight === 530
        && hierarchy.implicitWidth === 200
        && hierarchy.implicitHeight === 530
        && hierarchy.panelColor === LV.Theme.panelBackground05

    property string contractDiagnostics: JSON.stringify({
        listCount: list.itemCount,
        listVisibleCount: list.visibleItemCount,
        listIndent: list.generatedIndentStep,
        listRowHeight: list.generatedRowHeight,
        listItemWidth: list.generatedItemWidth,
        listIconSize: list.generatedIconSize,
        listChevronSize: list.generatedChevronSize,
        listImplicitWidth: list.implicitWidth,
        listImplicitHeight: list.implicitHeight,
        panelMinimum: [hierarchy.minimumPanelWidth, hierarchy.minimumPanelHeight],
        panelImplicit: [hierarchy.implicitWidth, hierarchy.implicitHeight],
        panelColor: hierarchy.panelColor.toString(),
        expectedPanelColor: LV.Theme.panelBackground05.toString()
    })
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("contractReady").toBool(),
                 qPrintable(root->property("contractDiagnostics").toString()));

    auto *hierarchy = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaHierarchy")));
    auto *viewport = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("hierarchyListViewportFlickable")));
    CTEST_CHECK(hierarchy);
    CTEST_CHECK(viewport);
    const QPointF viewportPosition = viewport->mapToItem(hierarchy, QPointF(0.0, 0.0));
    CTEST_CHECK_MESSAGE(qAbs(viewportPosition.x()) <= 0.01, "Hierarchy list must align to the panel left edge.");
    CTEST_CHECK_MESSAGE(qAbs(viewportPosition.y() - 26.0) <= 0.01,
             "Hierarchy list must begin immediately below the 26px toolbar.");
    CTEST_CHECK_MESSAGE(qAbs(viewport->width() - 200.0) <= 0.01,
             "Hierarchy list viewport must preserve the 200px panel width.");
    CTEST_CHECK_MESSAGE(qAbs(viewport->property("contentHeight").toReal() - 320.0) <= 0.01,
             "Sixteen 20px rows must produce the 320px Figma list content height.");
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("contractReady").toBool());
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
            { key: "root", depth: 0, label: "Root", expanded: true, selected: true },
            { key: "branch", depth: 1, label: "Branch", expanded: false },
            { key: "leaf", depth: 2, label: "Leaf" }
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("rowClickContractReady").toBool());
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("chevronRuleReady").toBool());
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("chevronDirectionReady").toBool());
}

void ImportApiTests::hierarchy_item_hover_and_active_state_visual_contract_loads()
{
    const QString requestedPlatform = qEnvironmentVariable("QT_QPA_PLATFORM").trimmed();
    if (requestedPlatform.compare(QStringLiteral("offscreen"), Qt::CaseInsensitive) == 0
        || QGuiApplication::platformName().compare(QStringLiteral("offscreen"), Qt::CaseInsensitive) == 0) {
        lvrs_ctest::skip("Hover pointer delivery is unavailable on offscreen platform plugin");
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
    CTEST_CHECK(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    window->show();
    CTEST_WAIT_DEFAULT(window->isVisible());

    auto *list = root->findChild<QObject *>(QStringLiteral("hierarchyList"));
    CTEST_CHECK(list);
    auto *itemAObject = root->findChild<QObject *>(QStringLiteral("itemA"));
    auto *itemBObject = root->findChild<QObject *>(QStringLiteral("itemB"));
    CTEST_CHECK(itemAObject);
    CTEST_CHECK(itemBObject);
    auto *itemA = qobject_cast<QQuickItem *>(itemAObject);
    auto *itemB = qobject_cast<QQuickItem *>(itemBObject);
    CTEST_CHECK(itemA);
    CTEST_CHECK(itemB);

    CTEST_WAIT_DEFAULT(list->property("activeItem").value<QObject *>() == itemAObject);
    CTEST_WAIT_DEFAULT(itemAObject->property("active").toBool());
    IMPORT_WAIT_COMPARE(itemAObject->property("state").toString(), QStringLiteral("Active"));
    IMPORT_WAIT_COMPARE(itemAObject->property("uxState").toInt(), itemAObject->property("uxStateActive").toInt());
    IMPORT_WAIT_COMPARE(itemBObject->property("state").toString(), QStringLiteral("Idle"));
    IMPORT_WAIT_COMPARE(itemBObject->property("uxState").toInt(), itemBObject->property("uxStateIdle").toInt());
    CTEST_CHECK(!itemBObject->property("isHoverState").toBool());
    CTEST_CHECK(!itemBObject->property("isActiveState").toBool());

    CTEST_CHECK(waitForWindowExposed(window));
    processFor(50);

    const QPointF hoverPoint = itemB->mapToScene(QPointF(itemB->width() * 0.5, itemB->height() * 0.5));
    const QPoint hoverPointInt(qRound(hoverPoint.x()), qRound(hoverPoint.y()));
#ifdef Q_OS_WIN
    const auto previousCursor = QCursor::pos();
    const auto restoreCursor = qScopeGuard([previousCursor] { QCursor::setPos(previousCursor); });
    window->requestActivate();
    CTEST_CHECK(waitForWindowActive(window));
    QCursor::setPos(window->mapToGlobal(hoverPointInt));
#endif
    mouseMove(window, hoverPointInt, 10);

    IMPORT_WAIT_MESSAGE(itemBObject->property("isHoverState").toBool(), qPrintable(
        QStringLiteral("hovered=%1 selected=%2 state=%3 active=%4 pressed=%5 bounds=%6,%7,%8,%9 pointer=%10,%11")
            .arg(itemBObject->property("hovered").toBool())
            .arg(itemBObject->property("resolvedSelected").toBool())
            .arg(itemBObject->property("state").toString())
            .arg(itemBObject->property("active").toBool())
            .arg(itemBObject->property("down").toBool())
            .arg(itemB->x()).arg(itemB->y()).arg(itemB->width()).arg(itemB->height())
            .arg(hoverPointInt.x()).arg(hoverPointInt.y())));
    IMPORT_WAIT_COMPARE(itemBObject->property("state").toString(), QStringLiteral("Hover"));
    IMPORT_WAIT_COMPARE(itemBObject->property("uxState").toInt(), itemBObject->property("uxStateHover").toInt());

    QObject *hoverBackground = itemBObject->property("background").value<QObject *>();
    CTEST_CHECK(hoverBackground);
    const auto interaction = itemBObject->property("interaction").value<QObject *>();
    CTEST_CHECK(interaction);
    IMPORT_WAIT_COMPARE(interaction->property("hovered").toBool(), true);
    const QColor expectedHoverColor = itemBObject->property("backgroundColorHover").value<QColor>();
    IMPORT_WAIT_COMPARE(hoverBackground->property("color").value<QColor>(), expectedHoverColor);

    mouseClick(window, Qt::LeftButton, Qt::NoModifier, hoverPointInt, 10);

    CTEST_WAIT_DEFAULT(list->property("activeItem").value<QObject *>() == itemBObject);
    CTEST_WAIT_DEFAULT(itemBObject->property("active").toBool());
    CTEST_WAIT_DEFAULT(itemBObject->property("isActiveState").toBool());
    IMPORT_WAIT_COMPARE(itemBObject->property("state").toString(), QStringLiteral("Active"));
    IMPORT_WAIT_COMPARE(itemBObject->property("uxState").toInt(), itemBObject->property("uxStateActive").toInt());
    CTEST_WAIT_DEFAULT(!itemAObject->property("isActiveState").toBool());

    QObject *activeBackground = itemBObject->property("background").value<QObject *>();
    CTEST_CHECK(activeBackground);
    const QColor expectedActiveColor = itemBObject->property("backgroundColor").value<QColor>();
    IMPORT_WAIT_COMPARE(activeBackground->property("color").value<QColor>(), expectedActiveColor);
}

void ImportApiTests::hierarchy_item_figma_defaults_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    LV.HierarchyItem {
        id: item
        visible: false
    }

    property bool contractReady:
        item.label === "Label"
        && item.rowHeight === 20
        && item.itemWidth === 200
        && item.indentStep === 8
        && item.iconSize === 18
        && item.chevronSize === 16
        && item.baseLeftPadding === 8
        && item.rowRightPadding === 8
        && item.leadingSpacing === 2
        && item.implicitWidth === 200
        && item.implicitHeight === 20
        && item.cornerRadius === 5
        && item.cornerRadius === LV.Theme.radiusControl
        && item.computedLeftPadding === 8
        && item.effectiveShowChevron
        && item.resolvedSelectionDirection === item.directionRight
        && item.resolvedChevronIconName === "generalchevronRight"
        && item.activatable
        && item.selectable
        && item.childCount == 0
        && item.visibleChildCount == 0
        && item.descendantCount == 0
        && item.count == -1
        && item.flatIndex == -1
        && item.visibleIndex == -1
        && item.textColorNormal === LV.Theme.bodyColor
        && item.rowBackgroundColorActive === LV.Theme.accentBlueMuted
        && item.rowBackgroundColorInactive === LV.Theme.panelBackground12
        && item.iconPlaceholderColor === LV.Theme.darkGrey10
        && !item.effectiveShowCount
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("contractReady").toBool());
}

void ImportApiTests::hierarchy_item_layout_geometry_contract_loads()
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
        width: 200
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);

    auto *itemObject = root->findChild<QObject *>(QStringLiteral("item"));
    CTEST_CHECK(itemObject);
    auto *item = qobject_cast<QQuickItem *>(itemObject);
    CTEST_CHECK(item);

    auto *iconObject = itemObject->findChild<QObject *>(QStringLiteral("hierarchyItemIcon"), Qt::FindChildrenRecursively);
    auto *labelObject = itemObject->findChild<QObject *>(QStringLiteral("hierarchyItemLabel"), Qt::FindChildrenRecursively);
    auto *chevronObject = itemObject->findChild<QObject *>(QStringLiteral("hierarchyItemChevron"), Qt::FindChildrenRecursively);
    CTEST_CHECK(iconObject);
    CTEST_CHECK(labelObject);
    CTEST_CHECK(chevronObject);
    auto *iconItem = qobject_cast<QQuickItem *>(iconObject);
    auto *labelItem = qobject_cast<QQuickItem *>(labelObject);
    auto *chevronItem = qobject_cast<QQuickItem *>(chevronObject);
    CTEST_CHECK(iconItem);
    CTEST_CHECK(labelItem);
    CTEST_CHECK(chevronItem);

    const QPointF iconPos = iconItem->mapToItem(item, QPointF(0, 0));
    const QPointF labelPos = labelItem->mapToItem(item, QPointF(0, 0));
    const QPointF chevronPos = chevronItem->mapToItem(item, QPointF(0, 0));

    CTEST_CHECK_MESSAGE(qAbs(iconPos.x() - 8.0) <= 0.5, "Icon x must match the 8px Figma inset.");
    CTEST_CHECK_MESSAGE(qAbs(labelPos.x() - 28.0) <= 0.5, "Label x must follow icon width + 2px gap.");
    CTEST_CHECK_MESSAGE(qAbs(chevronPos.x() - 176.0) <= 0.5, "Chevron x must match the trailing 8px inset.");
    CTEST_CHECK_MESSAGE(qAbs(labelItem->width() - 146.0) <= 0.5, "Label width must match the 18px icon and 16px chevron layout.");
    CTEST_CHECK_MESSAGE(qAbs(item->height() - 20.0) <= 0.5, "Row height must remain 20px.");
}

void ImportApiTests::hierarchy_item_count_view_contract_loads()
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
    height: 160

    LV.HierarchyItem {
        id: defaultCountItem
        objectName: "defaultCountItem"
        width: 200
        count: 7
        showChevron: true
        hasChildItems: true
    }

    LV.HierarchyItem {
        id: customCountItem
        objectName: "customCountItem"
        y: 40
        width: 200
        count: 12
        showChevron: false
        hasChildItems: false
        countView: Component {
            Item {
                objectName: "customCountView"
                property int count: -1
                property var hierarchyItem: null
                implicitWidth: badge.implicitWidth
                implicitHeight: badge.implicitHeight

                LV.Label {
                    id: badge
                    text: "C" + parent.count
                    style: description
                    color: LV.Theme.descriptionColor
                }
            }
        }
    }

    property bool countViewReady: {
        const customCountObject = customCountItem.countViewItem
        return defaultCountItem.effectiveShowCount
            && customCountItem.effectiveShowCount
            && customCountObject !== null
            && customCountObject.count === 12
            && customCountObject.hierarchyItem === customCountItem
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("countViewReady").toBool());

    auto *defaultItemObject = root->findChild<QObject *>(QStringLiteral("defaultCountItem"));
    auto *customItemObject = root->findChild<QObject *>(QStringLiteral("customCountItem"));
    CTEST_CHECK(defaultItemObject);
    CTEST_CHECK(customItemObject);
    auto *defaultItem = qobject_cast<QQuickItem *>(defaultItemObject);
    auto *customItem = qobject_cast<QQuickItem *>(customItemObject);
    CTEST_CHECK(defaultItem);
    CTEST_CHECK(customItem);

    auto *defaultCountLabelObject = defaultItemObject->findChild<QObject *>(QStringLiteral("hierarchyItemCountLabel"),
                                                                            Qt::FindChildrenRecursively);
    auto *defaultChevronObject = defaultItemObject->findChild<QObject *>(QStringLiteral("hierarchyItemChevron"),
                                                                         Qt::FindChildrenRecursively);
    auto *customCountSlotObject = customItemObject->findChild<QObject *>(QStringLiteral("hierarchyItemCount"),
                                                                         Qt::FindChildrenRecursively);
    auto *customCountObject = customItemObject->findChild<QObject *>(QStringLiteral("customCountView"),
                                                                     Qt::FindChildrenRecursively);
    auto *customChevronObject = customItemObject->findChild<QObject *>(QStringLiteral("hierarchyItemChevron"),
                                                                       Qt::FindChildrenRecursively);
    CTEST_CHECK(defaultCountLabelObject);
    CTEST_CHECK(defaultChevronObject);
    CTEST_CHECK(customCountSlotObject);
    CTEST_CHECK(customCountObject);
    CTEST_CHECK(customChevronObject);

    auto *defaultCountLabel = qobject_cast<QQuickItem *>(defaultCountLabelObject);
    auto *defaultChevron = qobject_cast<QQuickItem *>(defaultChevronObject);
    auto *customCountSlot = qobject_cast<QQuickItem *>(customCountSlotObject);
    auto *customCount = qobject_cast<QQuickItem *>(customCountObject);
    auto *customChevron = qobject_cast<QQuickItem *>(customChevronObject);
    CTEST_CHECK(defaultCountLabel);
    CTEST_CHECK(defaultChevron);
    CTEST_CHECK(customCountSlot);
    CTEST_CHECK(customCount);
    CTEST_CHECK(customChevron);

    IMPORT_COMPARE(defaultCountLabelObject->property("text").toString(), QStringLiteral("7"));
    IMPORT_COMPARE(customCountObject->property("count").toInt(), 12);

    const QPointF defaultCountPos = defaultCountLabel->mapToItem(defaultItem, QPointF(0, 0));
    const QPointF defaultChevronPos = defaultChevron->mapToItem(defaultItem, QPointF(0, 0));
    const qreal defaultGap = defaultChevronPos.x() - (defaultCountPos.x() + defaultCountLabel->width());
    CTEST_CHECK_MESSAGE(qAbs(defaultGap - 8.0) <= 0.5, "Count label must sit 8px left of the chevron slot.");

    const QPointF customCountPos = customCountSlot->mapToItem(customItem, QPointF(0, 0));
    const QPointF customChevronPos = customChevron->mapToItem(customItem, QPointF(0, 0));
    const qreal customGap = customChevronPos.x() - (customCountPos.x() + customCountSlot->width());
    CTEST_CHECK_MESSAGE(qAbs(customGap - 8.0) <= 0.5, "Custom count view must align 8px left of the chevron anchor.");
}

void ImportApiTests::hierarchy_item_structure_api_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    property int probeAttempts: 0
    property bool contractReady: false

    LV.HierarchyList {
        id: list
        visible: false
        model: [
            { key: "root", depth: 0, label: "Root", expanded: true, selected: true },
            { key: "branch", depth: 1, label: "Branch", expanded: false },
            { key: "leaf", depth: 2, label: "Leaf", activatable: false },
            { key: "sibling", depth: 1, label: "Sibling" }
        ]
    }

    function evaluateContract() {
        const rootItem = list.resolveByKey("root")
        const branchItem = list.resolveByKey("branch")
        const leafItem = list.resolveByKey("leaf")
        const siblingItem = list.resolveByKey("sibling")

        if ((!rootItem || !branchItem || !leafItem || !siblingItem) && probeAttempts < 40) {
            probeAttempts += 1
            Qt.callLater(evaluateContract)
            return
        }

        contractReady = rootItem && branchItem && leafItem && siblingItem
            && list.itemCount === 4
            && list.visibleItemCount === 3
            && rootItem.active
            && rootItem.uxState === rootItem.uxStateActive
            && rootItem.parentPathLabel === ""
            && rootItem.childCount === 2
            && rootItem.visibleChildCount === 2
            && rootItem.descendantCount === 3
            && rootItem.visibleDescendantCount === 2
            && rootItem.hiddenDescendantCount === 1
            && rootItem.childItemKeysText === "branch, sibling"
            && rootItem.childItemLabelsText === "Branch, Sibling"
            && rootItem.pathItemKeysText === "root"
            && rootItem.pathItemLabelsText === "Root"
            && rootItem.firstChildItemKey === "branch"
            && rootItem.firstChildItemLabel === "Branch"
            && rootItem.lastChildItemKey === "sibling"
            && rootItem.lastChildItemLabel === "Sibling"
            && rootItem.flatIndex === 0
            && rootItem.visibleIndex === 0
            && rootItem.siblingIndex === 0
            && rootItem.visibleSiblingIndex === 0
            && rootItem.siblingCount === 1
            && rootItem.visibleSiblingCount === 1
            && rootItem.isRootItem
            && rootItem.isBranchItem
            && !rootItem.isLeafItem
            && rootItem.isOnlySibling
            && !rootItem.hasParentItem
            && rootItem.chevronExpandable
            && rootItem.canToggleExpanded
            && rootItem.canCollapse
            && !rootItem.canExpand
            && !rootItem.collapsed
            && branchItem.parentItemKey === "root"
            && branchItem.parentLabel === "Root"
            && branchItem.parentPathLabel === "Root"
            && branchItem.childCount === 1
            && branchItem.visibleChildCount === 0
            && branchItem.descendantCount === 1
            && branchItem.visibleDescendantCount === 0
            && branchItem.hiddenDescendantCount === 1
            && branchItem.childItemKeysText === "leaf"
            && branchItem.childItemLabelsText === "Leaf"
            && branchItem.ancestorItemKeysText === "root"
            && branchItem.ancestorLabelsText === "Root"
            && branchItem.pathItemKeysText === "root, branch"
            && branchItem.pathItemLabelsText === "Root, Branch"
            && branchItem.flatIndex === 1
            && branchItem.visibleIndex === 1
            && branchItem.siblingIndex === 0
            && branchItem.visibleSiblingIndex === 0
            && branchItem.siblingCount === 2
            && branchItem.visibleSiblingCount === 2
            && branchItem.hasParentItem
            && !branchItem.isRootItem
            && branchItem.isBranchItem
            && !branchItem.isLeafItem
            && branchItem.isFirstSibling
            && !branchItem.isLastSibling
            && branchItem.chevronExpandable
            && branchItem.canExpand
            && !branchItem.canCollapse
            && branchItem.collapsed
            && branchItem.canBecomeActive
            && !branchItem.active
            && leafItem.parentItemKey === "branch"
            && leafItem.parentLabel === "Branch"
            && leafItem.parentPathLabel === "Root / Branch"
            && leafItem.childCount === 0
            && leafItem.visibleChildCount === 0
            && leafItem.descendantCount === 0
            && leafItem.visibleDescendantCount === 0
            && leafItem.hiddenDescendantCount === 0
            && leafItem.ancestorItemKeysText === "root, branch"
            && leafItem.ancestorLabelsText === "Root, Branch"
            && leafItem.pathItemKeysText === "root, branch, leaf"
            && leafItem.pathItemLabelsText === "Root, Branch, Leaf"
            && leafItem.flatIndex === 2
            && leafItem.visibleIndex === -1
            && leafItem.siblingIndex === 0
            && leafItem.visibleSiblingIndex === -1
            && leafItem.siblingCount === 1
            && leafItem.visibleSiblingCount === 0
            && !leafItem.isRootItem
            && !leafItem.isBranchItem
            && leafItem.isLeafItem
            && leafItem.isOnlySibling
            && !leafItem.chevronExpandable
            && !leafItem.canToggleExpanded
            && !leafItem.activatable
            && !leafItem.selectable
            && !leafItem.canBecomeActive
            && leafItem.uxState === leafItem.uxStateInactive
            && !leafItem.rowVisible
            && siblingItem.parentItemKey === "root"
            && siblingItem.parentLabel === "Root"
            && siblingItem.parentPathLabel === "Root"
            && siblingItem.siblingIndex === 1
            && siblingItem.visibleSiblingIndex === 1
            && siblingItem.siblingCount === 2
            && siblingItem.visibleSiblingCount === 2
            && siblingItem.visibleIndex === 2
            && siblingItem.isLastSibling
            && siblingItem.pathItemLabelsText === "Root, Sibling"
            && siblingItem.isLeafItem

        if (!contractReady) {
            console.log("hierarchy-structure-debug", JSON.stringify({
                itemCount: list.itemCount,
                visibleItemCount: list.visibleItemCount,
                root: rootItem ? {
                    active: rootItem.active,
                    uxState: rootItem.uxState,
                    parentPathLabel: rootItem.parentPathLabel,
                    childCount: rootItem.childCount,
                    visibleChildCount: rootItem.visibleChildCount,
                    descendantCount: rootItem.descendantCount,
                    visibleDescendantCount: rootItem.visibleDescendantCount,
                    childItemKeysText: rootItem.childItemKeysText,
                    childItemLabelsText: rootItem.childItemLabelsText,
                    pathItemKeysText: rootItem.pathItemKeysText,
                    pathItemLabelsText: rootItem.pathItemLabelsText,
                    flatIndex: rootItem.flatIndex,
                    visibleIndex: rootItem.visibleIndex,
                    siblingIndex: rootItem.siblingIndex,
                    visibleSiblingIndex: rootItem.visibleSiblingIndex,
                    siblingCount: rootItem.siblingCount,
                    visibleSiblingCount: rootItem.visibleSiblingCount,
                    canCollapse: rootItem.canCollapse
                } : null,
                branch: branchItem ? {
                    parentItemKey: branchItem.parentItemKey,
                    parentLabel: branchItem.parentLabel,
                    parentPathLabel: branchItem.parentPathLabel,
                    childCount: branchItem.childCount,
                    visibleChildCount: branchItem.visibleChildCount,
                    descendantCount: branchItem.descendantCount,
                    visibleDescendantCount: branchItem.visibleDescendantCount,
                    childItemKeysText: branchItem.childItemKeysText,
                    childItemLabelsText: branchItem.childItemLabelsText,
                    ancestorItemKeysText: branchItem.ancestorItemKeysText,
                    pathItemLabelsText: branchItem.pathItemLabelsText,
                    flatIndex: branchItem.flatIndex,
                    visibleIndex: branchItem.visibleIndex,
                    siblingIndex: branchItem.siblingIndex,
                    visibleSiblingIndex: branchItem.visibleSiblingIndex,
                    siblingCount: branchItem.siblingCount,
                    visibleSiblingCount: branchItem.visibleSiblingCount,
                    canExpand: branchItem.canExpand,
                    canCollapse: branchItem.canCollapse
                } : null,
                leaf: leafItem ? {
                    parentItemKey: leafItem.parentItemKey,
                    parentLabel: leafItem.parentLabel,
                    parentPathLabel: leafItem.parentPathLabel,
                    activatable: leafItem.activatable,
                    uxState: leafItem.uxState,
                    rowVisible: leafItem.rowVisible,
                    ancestorItemKeysText: leafItem.ancestorItemKeysText,
                    pathItemLabelsText: leafItem.pathItemLabelsText,
                    flatIndex: leafItem.flatIndex,
                    visibleIndex: leafItem.visibleIndex,
                    siblingCount: leafItem.siblingCount,
                    visibleSiblingCount: leafItem.visibleSiblingCount
                } : null,
                sibling: siblingItem ? {
                    parentItemKey: siblingItem.parentItemKey,
                    parentLabel: siblingItem.parentLabel,
                    parentPathLabel: siblingItem.parentPathLabel,
                    siblingIndex: siblingItem.siblingIndex,
                    visibleSiblingIndex: siblingItem.visibleSiblingIndex,
                    siblingCount: siblingItem.siblingCount,
                    visibleSiblingCount: siblingItem.visibleSiblingCount,
                    visibleIndex: siblingItem.visibleIndex,
                    pathItemLabelsText: siblingItem.pathItemLabelsText
                } : null
            }))
        }
    }

    Component.onCompleted: {
        Qt.callLater(evaluateContract)
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("contractReady").toBool());
}

void ImportApiTests::hierarchy_item_ux_state_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    function sameColor(left, right) {
        return Math.abs(left.r - right.r) < 0.001
            && Math.abs(left.g - right.g) < 0.001
            && Math.abs(left.b - right.b) < 0.001
            && Math.abs(left.a - right.a) < 0.001
    }

    LV.HierarchyItem {
        id: idleItem
        visible: false
    }

    LV.HierarchyItem {
        id: inactiveItem
        visible: false
        selectable: false
    }

    LV.HierarchyItem {
        id: activeItem
        visible: false
        selected: true
    }

    LV.HierarchyItem {
        id: dragItem
        visible: false
        dragPreviewActive: true
    }

    LV.HierarchyItem {
        id: collapsedItem
        visible: false
        showChevron: true
        hasChildItems: true
        expanded: false
    }

    property bool contractReady: false

    Component.onCompleted: {
        contractReady =
        idleItem.uxState === idleItem.uxStateIdle
        && idleItem.uxStateName === "Idle"
        && idleItem.canBecomeActive
        && !idleItem.active
        && idleItem.rowBackgroundColor.a === 0
        && inactiveItem.uxState === inactiveItem.uxStateInactive
        && inactiveItem.uxStateName === "Inactive"
        && inactiveItem.inactive
        && !inactiveItem.activatable
        && !inactiveItem.selectable
        && !inactiveItem.canBecomeActive
        && sameColor(inactiveItem.rowBackgroundColor, LV.Theme.panelBackground12)
        && activeItem.uxState === activeItem.uxStateActive
        && activeItem.uxStateName === "Active"
        && activeItem.active
        && sameColor(activeItem.rowBackgroundColor, LV.Theme.accentBlueMuted)
        && dragItem.uxState === dragItem.uxStateDrag
        && dragItem.uxStateName === "Drag"
        && dragItem.isDragState
        && sameColor(dragItem.rowBackgroundColor, LV.Theme.accentBlueMuted)
        && Math.abs(dragItem.opacity - dragItem.dragPreviewOpacity) < 0.01
        && collapsedItem.chevronExpandable
        && collapsedItem.canExpand
        && !collapsedItem.canCollapse
        && collapsedItem.collapsed
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("contractReady").toBool());
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
    property string expectedFallbackIcon: iconRoot + "generalprojectStructure.svg"
    property color transparentColor: "transparent"

    LV.LabelButton {
        id: labelButton
        objectName: "figmaLabelButton"
        text: "Button"
        tone: LV.AbstractButton.Primary
        width: implicitWidth
        height: implicitHeight
        visible: false
    }
    LV.IconButton {
        id: iconButton
        objectName: "figmaIconButton"
        tone: LV.AbstractButton.Primary
        width: implicitWidth
        height: implicitHeight
        visible: false
    }
    LV.LabelMenuButton {
        id: labelMenuButton
        objectName: "figmaLabelMenuButton"
        text: "Open"
        tone: LV.AbstractButton.Primary
        width: implicitWidth
        height: implicitHeight
        visible: false
    }
    LV.IconMenuButton {
        id: iconMenuButton
        objectName: "figmaIconMenuButton"
        tone: LV.AbstractButton.Primary
        width: implicitWidth
        height: implicitHeight
        visible: false
    }
    LV.LabelMenuButton {
        id: constrainedLabelMenuButton
        objectName: "constrainedLabelMenuButton"
        text: "Constrained"
        tone: LV.AbstractButton.Default
        width: 40
        height: implicitHeight
        visible: false
    }
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
        && Math.abs(labelButton.verticalPadding - 4.5) < 0.01
        && iconButton.horizontalPadding === LV.Theme.gap2
        && iconButton.verticalPadding === LV.Theme.gap2
        && labelMenuButton.horizontalPadding === LV.Theme.gap8
        && labelMenuButton.verticalPadding === LV.Theme.gap2
        && iconMenuButton.horizontalPadding === LV.Theme.gap4
        && iconMenuButton.verticalPadding === LV.Theme.gap2
        && labelMenuButton.spacing === LV.Theme.gapNone
        && iconMenuButton.spacing === -LV.Theme.gap2
        && iconButton.iconSize === LV.Theme.iconSm
        && iconMenuButton.iconSize === LV.Theme.iconSm
        && labelMenuButton.indicatorSize === LV.Theme.iconSm
        && iconMenuButton.indicatorSize === LV.Theme.iconSm
        && labelButton.figmaButtonHeight === 22
        && iconButton.figmaButtonHeight === 22
        && labelMenuButton.figmaButtonHeight === 22
        && iconMenuButton.figmaButtonHeight === 22
        && Math.abs(labelButton.implicitHeight - 22) < 0.01
        && Math.abs(iconButton.implicitHeight - 22) < 0.01
        && Math.abs(labelMenuButton.implicitHeight - 22) < 0.01
        && Math.abs(iconMenuButton.implicitHeight - 22) < 0.01
        && Math.abs(labelButton.implicitWidth - 56) < 0.01
        && Math.abs(iconButton.implicitWidth - 22) < 0.01
        && Math.abs(labelMenuButton.implicitWidth - 60) < 0.01
        && Math.abs(iconMenuButton.implicitWidth - 40) < 0.01
        && Math.abs(labelButton.implicitHeight - iconButton.implicitHeight) < 0.01
        && Math.abs(iconButton.implicitHeight - labelMenuButton.implicitHeight) < 0.01
        && Math.abs(labelMenuButton.implicitHeight - iconMenuButton.implicitHeight) < 0.01
        && Math.abs(labelButton.height - 22) < 0.01
        && Math.abs(iconButton.height - 22) < 0.01
        && Math.abs(labelMenuButton.height - 22) < 0.01
        && Math.abs(iconMenuButton.height - 22) < 0.01
        && Math.abs(labelButtonDefault.height - 22) < 0.01
        && Math.abs(iconButtonDefault.height - 22) < 0.01
        && Math.abs(labelMenuButtonDefault.height - 22) < 0.01
        && Math.abs(iconMenuButtonDefault.height - 22) < 0.01
        && Math.abs(labelButtonBorderless.height - 22) < 0.01
        && Math.abs(iconButtonBorderless.height - 22) < 0.01
        && Math.abs(labelMenuButtonBorderless.height - 22) < 0.01
        && Math.abs(iconMenuButtonBorderless.height - 22) < 0.01
        && Math.abs(labelButtonDestructive.height - 22) < 0.01
        && Math.abs(iconButtonDestructive.height - 22) < 0.01
        && Math.abs(labelMenuButtonDestructive.height - 22) < 0.01
        && Math.abs(iconMenuButtonDestructive.height - 22) < 0.01
        && Math.abs(labelButtonDisabled.implicitHeight - 22) < 0.01
        && Math.abs(iconButtonDisabled.implicitHeight - 22) < 0.01
        && Math.abs(labelMenuButtonDisabled.implicitHeight - 22) < 0.01
        && Math.abs(iconMenuButtonDisabled.implicitHeight - 22) < 0.01
        && Math.abs(labelButtonDisabled.height - 22) < 0.01
        && Math.abs(iconButtonDisabled.height - 22) < 0.01
        && Math.abs(labelMenuButtonDisabled.height - 22) < 0.01
        && Math.abs(iconMenuButtonDisabled.height - 22) < 0.01
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

    property string figmaPaddingDebug: JSON.stringify({
        label: [labelButton.implicitWidth, labelButton.implicitHeight,
            labelButton.horizontalPadding, labelButton.verticalPadding],
        icon: [iconButton.implicitWidth, iconButton.implicitHeight,
            iconButton.horizontalPadding, iconButton.verticalPadding],
        labelMenu: [labelMenuButton.implicitWidth, labelMenuButton.implicitHeight,
            labelMenuButton.horizontalPadding, labelMenuButton.verticalPadding,
            labelMenuButton.spacing],
        iconMenu: [iconMenuButton.implicitWidth, iconMenuButton.implicitHeight,
            iconMenuButton.horizontalPadding, iconMenuButton.verticalPadding,
            iconMenuButton.spacing]
    })
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("figmaPaddingReady").toBool(),
                 qPrintable(root->property("figmaPaddingDebug").toString()));

    const auto boundsIn = [](QQuickItem *item, QQuickItem *ancestor) {
        return QRectF(item->mapToItem(ancestor, QPointF(0.0, 0.0)),
                      QSizeF(item->width(), item->height()));
    };
    const auto verifyBounds = [](const QRectF &actual, const QRectF &expected) {
        CTEST_CHECK_MESSAGE(qAbs(actual.x() - expected.x()) < 0.01
                     && qAbs(actual.y() - expected.y()) < 0.01
                     && qAbs(actual.width() - expected.width()) < 0.01
                     && qAbs(actual.height() - expected.height()) < 0.01,
                 qPrintable(QStringLiteral("bounds actual=(%1,%2 %3x%4) expected=(%5,%6 %7x%8)")
                                .arg(actual.x()).arg(actual.y())
                                .arg(actual.width()).arg(actual.height())
                                .arg(expected.x()).arg(expected.y())
                                .arg(expected.width()).arg(expected.height())));
    };

    auto *labelButton = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaLabelButton")));
    auto *iconButton = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaIconButton")));
    auto *labelMenuButton = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaLabelMenuButton")));
    auto *iconMenuButton = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaIconMenuButton")));
    auto *constrainedLabelMenuButton = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("constrainedLabelMenuButton")));
    CTEST_CHECK(labelButton);
    CTEST_CHECK(iconButton);
    CTEST_CHECK(labelMenuButton);
    CTEST_CHECK(iconMenuButton);
    CTEST_CHECK(constrainedLabelMenuButton);

    auto *labelText = qobject_cast<QQuickItem *>(
        labelButton->findChild<QObject *>(QStringLiteral("labelButton_label")));
    auto *iconImage = qobject_cast<QQuickItem *>(
        iconButton->findChild<QObject *>(QStringLiteral("iconButton_icon")));
    auto *labelMenuText = qobject_cast<QQuickItem *>(
        labelMenuButton->findChild<QObject *>(QStringLiteral("labelMenuButton_label")));
    auto *labelMenuIndicator = qobject_cast<QQuickItem *>(
        labelMenuButton->findChild<QObject *>(QStringLiteral("labelMenuButton_indicator")));
    auto *iconMenuImage = qobject_cast<QQuickItem *>(
        iconMenuButton->findChild<QObject *>(QStringLiteral("iconMenuButton_icon")));
    auto *iconMenuIndicator = qobject_cast<QQuickItem *>(
        iconMenuButton->findChild<QObject *>(QStringLiteral("iconMenuButton_indicator")));
    auto *constrainedLabel = qobject_cast<QQuickItem *>(
        constrainedLabelMenuButton->findChild<QObject *>(QStringLiteral("labelMenuButton_label")));
    auto *constrainedIndicator = qobject_cast<QQuickItem *>(
        constrainedLabelMenuButton->findChild<QObject *>(QStringLiteral("labelMenuButton_indicator")));
    CTEST_CHECK(labelText);
    CTEST_CHECK(iconImage);
    CTEST_CHECK(labelMenuText);
    CTEST_CHECK(labelMenuIndicator);
    CTEST_CHECK(iconMenuImage);
    CTEST_CHECK(iconMenuIndicator);
    CTEST_CHECK(constrainedLabel);
    CTEST_CHECK(constrainedIndicator);

    verifyBounds(boundsIn(labelText, labelButton), QRectF(8.0, 4.5, 40.0, 13.0));
    verifyBounds(boundsIn(iconImage, iconButton), QRectF(2.0, 2.0, 18.0, 18.0));
    verifyBounds(boundsIn(labelMenuText, labelMenuButton), QRectF(8.0, 4.5, 32.0, 13.0));
    verifyBounds(boundsIn(labelMenuIndicator, labelMenuButton), QRectF(40.0, 2.0, 18.0, 18.0));
    verifyBounds(boundsIn(iconMenuImage, iconMenuButton), QRectF(4.0, 2.0, 18.0, 18.0));
    verifyBounds(boundsIn(iconMenuIndicator, iconMenuButton), QRectF(20.0, 2.0, 18.0, 18.0));
    verifyBounds(boundsIn(constrainedLabel, constrainedLabelMenuButton),
                 QRectF(8.0, 4.5, 12.0, 13.0));
    verifyBounds(boundsIn(constrainedIndicator, constrainedLabelMenuButton),
                 QRectF(20.0, 2.0, 18.0, 18.0));

    const QFont labelFont = labelText->property("font").value<QFont>();
    const QFont labelMenuFont = labelMenuText->property("font").value<QFont>();
    IMPORT_COMPARE(labelFont.pixelSize(), 13);
    IMPORT_COMPARE(labelFont.weight(), QFont::Medium);
    IMPORT_COMPARE(labelFont.styleName(), QStringLiteral("Medium"));
    IMPORT_COMPARE(labelMenuFont.pixelSize(), 13);
    IMPORT_COMPARE(labelMenuFont.weight(), QFont::Medium);
    IMPORT_COMPARE(labelMenuFont.styleName(), QStringLiteral("Medium"));
    IMPORT_WAIT_COMPARE(iconImage->property("status").toInt(), 1);
    IMPORT_WAIT_COMPARE(labelMenuIndicator->property("status").toInt(), 1);
    IMPORT_WAIT_COMPARE(iconMenuImage->property("status").toInt(), 1);
    IMPORT_WAIT_COMPARE(iconMenuIndicator->property("status").toInt(), 1);
}

void ImportApiTests::button_family_components_contract(bool mobile)
{
    QQmlEngine engine;
    engine.addImportPath(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/.."));
    engine.rootContext()->setContextProperty("testMobile", mobile);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 640
    height: 400
    property int methodCalls: 0
    property int signalCalls: 0
    Component.onCompleted: LV.Theme.targetOverride = testMobile ? "ios" : "macos"
    Rectangle { anchors.fill: parent; color: "#202020" }
    Column {
        x: 20
        y: 20
        spacing: 16
        Repeater {
            model: 5
            Row {
                id: row
                required property int index
                spacing: 20
                LV.PushButton {
                    objectName: "pushLabel" + row.index
                    text: "Button"
                    tone: row.index
                    method: function(eventData) { root.methodCalls++ }
                    onClicked: root.signalCalls++
                }
                LV.PushButton {
                    objectName: "pushIcon" + row.index
                    iconMode: true
                    tone: row.index
                    method: function(eventData) { root.methodCalls++ }
                    onClicked: root.signalCalls++
                }
                LV.DropdownButton {
                    objectName: "dropdownLabel" + row.index
                    text: "Open"
                    tone: row.index
                    method: function(eventData) { root.methodCalls++ }
                    onClicked: root.signalCalls++
                }
                LV.DropdownButton {
                    objectName: "dropdownIcon" + row.index
                    iconMode: true
                    tone: row.index
                    method: function(eventData) { root.methodCalls++ }
                    onClicked: root.signalCalls++
                }
            }
        }
    }
}
)";
    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    QQuickWindow window;
    window.resize(640, 400);
    auto *host = qobject_cast<QQuickItem *>(root.data());
    CTEST_CHECK(host);
    host->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(waitForWindowExposed(&window));
    const int scale = 1;
    const QStringList families = {"pushLabel", "pushIcon", "dropdownLabel", "dropdownIcon"};
    int expectedCalls = 0;
    for (int tone = 0; tone < 5; ++tone) {
        for (int kind = 0; kind < families.size(); ++kind) {
            const QString buttonName = families.at(kind) + QString::number(tone);
            auto *button = visualChildByObjectName(host, buttonName);
            CTEST_CHECK_MESSAGE(button, qPrintable(buttonName));
            IMPORT_WAIT_COMPARE(button->height(), 22.0 * scale);
            IMPORT_COMPARE(button->property("cornerRadius").toInt(), 8 * scale);
            IMPORT_COMPARE(button->property("resolvedCornerRadius").toReal(), 8.0 * scale);
            const QList<int> leftInsets = {8, 2, 8, 4};
            const QList<int> rightInsets = {8, 2, 2, 2};
            IMPORT_COMPARE(button->property("leftPadding").toReal(), qreal(leftInsets.at(kind) * scale));
            IMPORT_COMPARE(button->property("rightPadding").toReal(), qreal(rightInsets.at(kind) * scale));
            const qreal topInset = kind == 0 ? (22.0 * scale - 13) / 2 : 2.0 * scale;
            IMPORT_COMPARE(button->property("topPadding").toReal(), topInset);
            IMPORT_COMPARE(button->property("bottomPadding").toReal(), topInset);
            const qreal gap = kind == 0 ? 10 * scale : kind == 1 ? (tone == 0 ? 0 : 7 * scale)
                : kind == 2 ? 0 : -2 * scale;
            IMPORT_COMPARE(button->property("spacing").toReal(), gap);
            const QList<qreal> widths = {40.0 + 16 * scale, 22.0 * scale,
                                         32.0 + 28 * scale, 40.0 * scale};
            IMPORT_COMPARE(button->width(), widths.at(kind));
            if (kind >= 2) {
                auto *indicator = button->findChild<QQuickItem *>(kind == 2
                    ? "labelMenuButton_indicator" : "iconMenuButton_indicator");
                CTEST_CHECK(indicator);
                const QPointF position = indicator->mapToItem(button, QPointF());
                IMPORT_COMPARE(position.x(), kind == 2 ? 32.0 + 8 * scale : 20.0 * scale);
                IMPORT_COMPARE(position.y(), 2.0 * scale);
                IMPORT_COMPARE(indicator->width(), 18.0 * scale);
                IMPORT_WAIT_COMPARE(indicator->property("status").toInt(), 1);
            }
            mouseClick(&window, Qt::LeftButton, Qt::NoModifier,
                             scenePoint(button, QPointF(button->width() / 2, button->height() / 2)));
            if (tone != 4)
                ++expectedCalls;
            IMPORT_COMPARE(root->property("methodCalls").toInt(), expectedCalls);
            IMPORT_COMPARE(root->property("signalCalls").toInt(), expectedCalls);
        }
    }
    const QString captureDir = qEnvironmentVariable("LVRS_BUTTON_CAPTURE_DIR");
    if (!captureDir.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(captureDir));
        ImportSignalRecorder frameSpy;
        const auto frameSpyConnection = QObject::connect(&window, &QQuickWindow::frameSwapped, &frameSpy, &ImportSignalRecorder::recordVoid);
        CTEST_CHECK(frameSpyConnection);
        window.update();
        CTEST_WAIT_VERIFY(frameSpy.events.size() > 0, 5000);
        const auto capture = host->grabToImage(QSize(1280, 800));
        CTEST_CHECK(capture);
        CTEST_WAIT_DEFAULT(!capture->image().isNull());
        CTEST_CHECK(capture->image().save(captureDir + (mobile ? "/buttons-mobile.png" : "/buttons-desktop.png")));
    }
}

void ImportApiTests::button_default_tone_matches_figma_accent_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.LabelButton { id: labelButton; text: "Button"; visible: false }
    LV.IconButton { id: iconButton; visible: false }
    LV.LabelMenuButton { id: labelMenuButton; text: "Open"; visible: false }
    LV.IconMenuButton { id: iconMenuButton; visible: false }

    property bool defaultToneReady:
        labelButton.tone === LV.AbstractButton.Primary
        && iconButton.tone === LV.AbstractButton.Primary
        && labelMenuButton.tone === LV.AbstractButton.Primary
        && iconMenuButton.tone === LV.AbstractButton.Primary
        && labelButton.backgroundColor === LV.Theme.primary
        && iconButton.backgroundColor === LV.Theme.primary
        && labelMenuButton.backgroundColor === LV.Theme.primary
        && iconMenuButton.backgroundColor === LV.Theme.primary
        && labelMenuButton.resolvedIndicatorName === "generalchevronDownAccent"
        && iconMenuButton.resolvedIndicatorName === "generalchevronDownAccent"
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_CHECK(root->property("defaultToneReady").toBool());
}

void ImportApiTests::segmented_control_figma_contract_loads(bool mobile)
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    engine.rootContext()->setContextProperty("testMobile", mobile);

    const auto verifyBounds = [](QQuickItem *item, QQuickItem *ancestor, const QRectF &expected) {
        const QRectF actual(item->mapToItem(ancestor, QPointF(0.0, 0.0)),
                            QSizeF(item->width(), item->height()));
        CTEST_CHECK_MESSAGE(qAbs(actual.x() - expected.x()) < 0.01
                     && qAbs(actual.y() - expected.y()) < 0.01
                     && qAbs(actual.width() - expected.width()) < 0.01
                     && qAbs(actual.height() - expected.height()) < 0.01,
                 qPrintable(QStringLiteral("bounds actual=(%1,%2 %3x%4) expected=(%5,%6 %7x%8)")
                                .arg(actual.x()).arg(actual.y())
                                .arg(actual.width()).arg(actual.height())
                                .arg(expected.x()).arg(expected.y())
                                .arg(expected.width()).arg(expected.height())));
    };

    for (int count = 2; count <= 15; ++count) {
        QString labelButtons;
        QString iconButtons;
        for (int index = 0; index < count; ++index) {
            labelButtons += QStringLiteral(
                "LV.LabelButton { objectName: \"labelSegmentButton_%1\"; text: \"Button\" }\n")
                                .arg(index);
            iconButtons += QStringLiteral(
                "LV.IconButton { objectName: \"iconSegmentButton_%1\"; iconName: \"projectStructure\" }\n")
                               .arg(index);
        }

        const int expectedLabelWidth = (count * 56) + ((count - 1) * 2) + 8;
        const int expectedIconWidth = (count * 22) + ((count - 1) * 2) + 8;
        QString verticalButtons = iconButtons;
        verticalButtons.replace("iconSegmentButton_", "verticalSegmentButton_");
        const QByteArray qml = QStringLiteral(R"(
import QtQuick
import LVRS as LV

Item {
    width: 1000
    height: 120
    property int borderlessTone: LV.AbstractButton.Borderless
    Component.onCompleted: LV.Theme.targetOverride = testMobile ? "ios" : "macos"

    Rectangle { anchors.fill: parent; color: "#1f1f1f" }

    LV.LabelSegmentedControl {
        id: labelSegment
        objectName: "figmaLabelSegment"
        x: 20
        y: 20
        %4
    }

    LV.IconSegmentedControl {
        id: iconSegment
        objectName: "figmaIconSegment"
        x: 20
        y: 70
        %5
    }

    LV.IconSegmentedControl {
        id: verticalSegment
        objectName: "verticalIconSegment"
        orientation: Qt.Vertical
        x: 930
        y: 20
        %6
    }

    property bool contractReady:
        verticalSegment.segmentCount === %1
        && verticalSegment.width === 30
        && verticalSegment.height === %3
        && labelSegment.segmentCount === %1
        && iconSegment.segmentCount === %1
        && labelSegment.horizontalPadding === 4
        && Math.abs(labelSegment.verticalPadding - 3.5) < 0.01
        && iconSegment.horizontalPadding === 4
        && iconSegment.verticalPadding === 4
        && labelSegment.spacing === 2
        && iconSegment.spacing === 2
        && labelSegment.borderWidth === 2
        && iconSegment.borderWidth === 2
        && labelSegment.cornerRadius === 12
        && iconSegment.cornerRadius === 12
        && labelSegment.resolvedCornerRadius === 12
        && iconSegment.resolvedCornerRadius === 12
        && labelSegment.backgroundColor === LV.Theme.panelBackground08
        && iconSegment.backgroundColor === LV.Theme.panelBackground08
        && labelSegment.borderColor === LV.Theme.panelBackground12
        && iconSegment.borderColor === LV.Theme.panelBackground12
        && labelSegment.implicitWidth === %2
        && labelSegment.width === %2
        && labelSegment.implicitHeight === 29
        && labelSegment.height === 29
        && iconSegment.implicitWidth === %3
        && iconSegment.width === %3
        && iconSegment.implicitHeight === 30
        && iconSegment.height === 30

    property string contractDebug: JSON.stringify({
        count: %1,
        labelCount: labelSegment.segmentCount,
        labelRadius: labelSegment.resolvedCornerRadius,
        labelPadding: [labelSegment.horizontalPadding, labelSegment.verticalPadding],
        labelSize: [labelSegment.width, labelSegment.height,
            labelSegment.implicitWidth, labelSegment.implicitHeight],
        iconCount: iconSegment.segmentCount,
        iconRadius: iconSegment.resolvedCornerRadius,
        iconPadding: [iconSegment.horizontalPadding, iconSegment.verticalPadding],
        iconSize: [iconSegment.width, iconSegment.height,
            iconSegment.implicitWidth, iconSegment.implicitHeight]
    })
}
)")
                                   .arg(count)
                                   .arg(expectedLabelWidth)
                                   .arg(expectedIconWidth)
                                   .arg(labelButtons)
                                   .arg(iconButtons)
                                   .arg(verticalButtons)
                                   .toUtf8();

        QScopedPointer<QObject> root(createFromQml(engine, qml));
        CTEST_CHECK(root);
        IMPORT_WAIT_MESSAGE(root->property("contractReady").toBool(),
                     qPrintable(root->property("contractDebug").toString()));

        auto *labelSegment = qobject_cast<QQuickItem *>(
            root->findChild<QObject *>(QStringLiteral("figmaLabelSegment")));
        auto *iconSegment = qobject_cast<QQuickItem *>(
            root->findChild<QObject *>(QStringLiteral("figmaIconSegment")));
        CTEST_CHECK(labelSegment);
        CTEST_CHECK(iconSegment);
        auto *verticalSegment = qobject_cast<QQuickItem *>(root->findChild<QObject *>(QStringLiteral("verticalIconSegment")));
        CTEST_CHECK(verticalSegment);
        const int borderlessTone = root->property("borderlessTone").toInt();

        for (int index = 0; index < count; ++index) {
            auto *labelButton = qobject_cast<QQuickItem *>(
                root->findChild<QObject *>(QStringLiteral("labelSegmentButton_%1").arg(index)));
            auto *iconButton = qobject_cast<QQuickItem *>(
                root->findChild<QObject *>(QStringLiteral("iconSegmentButton_%1").arg(index)));
            CTEST_CHECK(labelButton);
            CTEST_CHECK(iconButton);
            auto *verticalButton = qobject_cast<QQuickItem *>(root->findChild<QObject *>(QStringLiteral("verticalSegmentButton_%1").arg(index)));
            CTEST_CHECK(verticalButton);
            verifyBounds(verticalButton, verticalSegment, QRectF(4.0, 4.0 + (index * 24.0), 22.0, 22.0));
            IMPORT_COMPARE(verticalButton->property("effectiveTone").toInt(), borderlessTone);
            IMPORT_COMPARE(verticalButton->property("tone").toInt(), 0);
            IMPORT_COMPARE(labelButton->property("effectiveTone").toInt(), borderlessTone);
            IMPORT_COMPARE(iconButton->property("effectiveTone").toInt(), borderlessTone);
            IMPORT_COMPARE(labelButton->property("tone").toInt(), 0);
            IMPORT_COMPARE(iconButton->property("tone").toInt(), 0);
            IMPORT_COMPARE(labelButton->property("resolvedCornerRadius").toReal(), 8.0);
            IMPORT_COMPARE(iconButton->property("resolvedCornerRadius").toReal(), 8.0);
            verifyBounds(labelButton,
                         labelSegment,
                         QRectF(4.0 + (index * 58.0), 3.5, 56.0, 22.0));
            verifyBounds(iconButton,
                         iconSegment,
                         QRectF(4.0 + (index * 24.0), 4.0, 22.0, 22.0));
        }

        // Reorient an existing control and verify both geometry and child order.
        CTEST_CHECK(iconSegment->setProperty("orientation", int(Qt::Vertical)));
        IMPORT_WAIT_COMPARE(iconSegment->width(), 30.0);
        IMPORT_WAIT_COMPARE(iconSegment->height(), double(expectedIconWidth));
        for (int index = 0; index < count; ++index) {
            auto *button = qobject_cast<QQuickItem *>(root->findChild<QObject *>(QStringLiteral("iconSegmentButton_%1").arg(index)));
            verifyBounds(button, iconSegment, QRectF(4.0, 4.0 + index * 24.0, 22.0, 22.0));
        }
        CTEST_CHECK(iconSegment->setProperty("orientation", int(Qt::Horizontal)));
        IMPORT_WAIT_COMPARE(iconSegment->width(), double(expectedIconWidth));
        IMPORT_WAIT_COMPARE(iconSegment->height(), 30.0);

        const QString captureDir = qEnvironmentVariable("LVRS_SEGMENT_CAPTURE_DIR");
        if (!captureDir.isEmpty()) {
            CTEST_CHECK(QDir().mkpath(captureDir));
            QQuickWindow window;
            window.resize(460, 120);
            auto *host = qobject_cast<QQuickItem *>(root.data());
            CTEST_CHECK(host);
            host->setParentItem(window.contentItem());
            window.show();
            CTEST_CHECK(waitForWindowExposed(&window));
            ImportSignalRecorder frameSpy;
        const auto frameSpyConnection = QObject::connect(&window, &QQuickWindow::frameSwapped, &frameSpy, &ImportSignalRecorder::recordVoid);
        CTEST_CHECK(frameSpyConnection);
            window.update();
            CTEST_WAIT_VERIFY(frameSpy.events.size() > 0, 5000);
            const auto capture = host->grabToImage(QSize(920, 240));
            CTEST_CHECK(capture);
            CTEST_WAIT_DEFAULT(!capture->image().isNull());
            const QString fileName = QStringLiteral("/segments-%1-%2.png")
                .arg(mobile ? "mobile" : "desktop").arg(count);
            CTEST_CHECK(capture->image().save(captureDir + fileName));
            host->setParentItem(nullptr);
        }
    }
}

void ImportApiTests::segmented_button_selected_color()
{
    QQmlEngine engine;
    engine.addImportPath(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/.."));
    QScopedPointer<QObject> root(createFromQml(engine, R"(
import QtQuick
import LVRS as LV
Item {
    id: root
    property color expectedSelected: LV.Theme.accentMuted
    property int originalTone: LV.AbstractButton.Destructive
    LV.IconSegmentedControl {
        id: group
        objectName: "segmentGroup"
        orientation: Qt.Vertical
        LV.IconButton { objectName: "segmentButton"; iconName: "projectStructure"; motionEnabled: false;
            tone: LV.AbstractButton.Destructive; checkable: true }
    }
    Component.onCompleted: LV.Theme.primaryColor = "#ef5123"
}
)"));
    CTEST_CHECK(root);
    auto *button = root->findChild<QObject *>(QStringLiteral("segmentButton"));
    auto *group = root->findChild<QObject *>(QStringLiteral("segmentGroup"));
    CTEST_CHECK(button && group);
    IMPORT_WAIT_COMPARE(button->property("effectiveTone").toInt(), 2); // Borderless presentation
    IMPORT_COMPARE(button->property("tone").toInt(), root->property("originalTone").toInt());
    CTEST_CHECK(QMetaObject::invokeMethod(button, "click"));
    CTEST_WAIT_DEFAULT(button->property("checked").toBool());
    IMPORT_COMPARE(button->property("tone").toInt(), root->property("originalTone").toInt());
    auto *background=button->property("background").value<QObject *>();
    CTEST_CHECK(background);
    IMPORT_WAIT_COMPARE(background->property("color").value<QColor>(),root->property("expectedSelected").value<QColor>());
    const auto oldColor=root->property("expectedSelected").value<QColor>();
    // The selected fill follows the app accent, including subsequent changes.
    auto *theme=engine.singletonInstance<QObject *>("LVRS", "Theme");
    CTEST_CHECK(theme); CTEST_CHECK(theme->setProperty("primaryColor", QColor("#31bb75")));
    CTEST_WAIT_DEFAULT(root->property("expectedSelected").value<QColor>()!=oldColor);
    IMPORT_WAIT_COMPARE(background->property("color").value<QColor>(),root->property("expectedSelected").value<QColor>());
    CTEST_CHECK(group->setProperty("forceBorderlessTone",false));
    IMPORT_WAIT_COMPARE(button->property("effectiveTone").toInt(),root->property("originalTone").toInt());
    IMPORT_COMPARE(button->property("tone").toInt(),root->property("originalTone").toInt());
    IMPORT_WAIT_COMPARE(background->property("color").value<QColor>(),root->property("expectedSelected").value<QColor>());
    CTEST_CHECK(QMetaObject::invokeMethod(button, "click"));
    CTEST_WAIT_DEFAULT(!button->property("checked").toBool());
    CTEST_CHECK(group->setProperty("forceBorderlessTone",true));
    IMPORT_WAIT_COMPARE(button->property("effectiveTone").toInt(),2);
    auto *buttonItem=qobject_cast<QQuickItem *>(button);
    auto *rootItem=qobject_cast<QQuickItem *>(root.data());
    CTEST_CHECK(buttonItem && rootItem);
    buttonItem->setParentItem(rootItem);
    IMPORT_WAIT_COMPARE(button->property("effectiveTone").toInt(),root->property("originalTone").toInt());
}

void ImportApiTests::button_injected_methods_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property int runCount: 0
    property int objectInvokeCount: 0
    property bool labelClickedEventReady: false
    property bool stepperClickedEventReady: false
    property bool comboClickedEventReady: false
    property bool labelSegmentEventReady: false
    property bool iconSegmentEventReady: false
    property var manualResults: []
    property string methodLog: ""

    function collect(eventData, tag) {
        runCount += 1
        methodLog += tag + ":" + eventData.trigger + ";"
        return tag + "-" + eventData.trigger
    }

    function runInjectedMethods() {
        manualResults = labelButton.invokeMethods(labelButton.createMethodEvent("manual"))
        labelButton.clicked()
        iconButton.clicked()
        labelMenuButton.clicked()
        iconMenuButton.clicked()
        stepper.clicked()
        comboBox.clicked()
        labelSegment.invokeMethods()
        iconSegment.invokeMethods(iconSegment.createMethodEvent("custom"))
    }

    QtObject {
        id: commandObject
        function invoke(eventData) {
            root.objectInvokeCount += 1
            root.methodLog += "object:" + eventData.trigger + ";"
            return "object-" + eventData.trigger
        }
    }

    LV.LabelButton {
        id: emptyButton
        visible: false
    }

    LV.LabelButton {
        id: labelButton
        text: "Label"
        visible: false
        method: function(eventData) {
            root.labelClickedEventReady = eventData.source === labelButton
                && eventData.trigger === "clicked"
                && eventData.effectiveEnabled
            return root.collect(eventData, "label")
        }
        methods: [
            function(eventData) {
                return root.collect(eventData, "labelExtra")
            },
            commandObject
        ]
    }

    LV.IconButton {
        id: iconButton
        visible: false
        method: function(eventData) {
            return root.collect(eventData, "icon")
        }
    }

    LV.LabelMenuButton {
        id: labelMenuButton
        text: "Menu"
        visible: false
        method: function(eventData) {
            return root.collect(eventData, "labelMenu")
        }
    }

    LV.IconMenuButton {
        id: iconMenuButton
        visible: false
        method: function(eventData) {
            return root.collect(eventData, "iconMenu")
        }
    }

    LV.Stepper {
        id: stepper
        visible: false
        method: function(eventData) {
            root.stepperClickedEventReady = eventData.source === stepper
                && eventData.trigger === "clicked"
                && eventData.effectiveEnabled
            return root.collect(eventData, "stepper")
        }
    }

    LV.ComboBox {
        id: comboBox
        visible: false
        method: function(eventData) {
            root.comboClickedEventReady = eventData.source === comboBox
                && eventData.trigger === "clicked"
                && eventData.effectiveEnabled
            return root.collect(eventData, "combo")
        }
    }

    LV.LabelSegmentedControl {
        id: labelSegment
        visible: false
        method: function(eventData) {
            root.labelSegmentEventReady = eventData.source === labelSegment
                && eventData.trigger === "manual"
            return root.collect(eventData, "labelSegment")
        }
    }

    LV.IconSegmentedControl {
        id: iconSegment
        visible: false
        methods: [
            function(eventData) {
                root.iconSegmentEventReady = eventData.source === iconSegment
                    && eventData.trigger === "custom"
                return root.collect(eventData, "iconSegment")
            }
        ]
    }

    Component.onCompleted: Qt.callLater(runInjectedMethods)

    property bool directMethodApiReady:
        typeof labelButton.createMethodEvent === "function"
        && typeof labelButton.invokeMethod === "function"
        && typeof labelButton.invokeMethods === "function"
        && typeof stepper.createMethodEvent === "function"
        && typeof stepper.invokeMethod === "function"
        && typeof stepper.invokeMethods === "function"
        && typeof comboBox.createMethodEvent === "function"
        && typeof comboBox.invokeMethod === "function"
        && typeof comboBox.invokeMethods === "function"
        && typeof labelSegment.createMethodEvent === "function"
        && typeof labelSegment.invokeMethod === "function"
        && typeof labelSegment.invokeMethods === "function"
        && typeof iconSegment.createMethodEvent === "function"
        && typeof iconSegment.invokeMethod === "function"
        && typeof iconSegment.invokeMethods === "function"
        && !emptyButton.hasInjectedMethods
        && labelButton.hasInjectedMethods
        && iconButton.hasInjectedMethods
        && labelMenuButton.hasInjectedMethods
        && iconMenuButton.hasInjectedMethods
        && stepper.hasInjectedMethods
        && comboBox.hasInjectedMethods
        && labelSegment.hasInjectedMethods
        && iconSegment.hasInjectedMethods

    property bool injectedMethodsReady:
        directMethodApiReady
        && manualResults.length === 3
        && manualResults[0] === "label-manual"
        && manualResults[1] === "labelExtra-manual"
        && manualResults[2] === "object-manual"
        && runCount === 11
        && objectInvokeCount === 2
        && labelClickedEventReady
        && stepperClickedEventReady
        && comboClickedEventReady
        && labelSegmentEventReady
        && iconSegmentEventReady
        && methodLog.indexOf("label:manual;") >= 0
        && methodLog.indexOf("label:clicked;") >= 0
        && methodLog.indexOf("stepper:clicked;") >= 0
        && methodLog.indexOf("combo:clicked;") >= 0
        && methodLog.indexOf("iconSegment:custom;") >= 0
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("injectedMethodsReady").toBool(),
                 qPrintable(root->property("methodLog").toString()));
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
    readonly property real expectedUpDownX:
        (primaryUpDown.width - primaryUpDown.iconWidth) * 0.5
    readonly property real expectedUpDownY:
        (primaryUpDown.height - primaryUpDown.iconHeight) * 0.5
    readonly property real expectedUpX:
        (primaryUp.width - primaryUp.iconWidth) * 0.5
    readonly property real expectedUpY:
        (primaryUp.height - primaryUp.iconHeight) * 0.5
    readonly property real expectedChevronWidth: primaryUp.width * (10.0 / 18.0)
    readonly property real expectedChevronHeight: primaryUp.height * (6.0 / 18.0)
    readonly property real expectedUpDownWidth: primaryUpDown.width * (6.43604 / 18.0)
    readonly property real expectedUpDownHeight: primaryUpDown.height * (11.1455 / 18.0)
    readonly property string iconRoot: "qrc:/qt/qml/LVRS/resources/iconset/"

    LV.Stepper { id: defaultStepper; visible: false }
    LV.Stepper { id: primaryUpDown; objectName: "primaryUpDown"; visible: false; tone: LV.AbstractButton.Primary; arrow: LV.Stepper.UpDown }
    LV.Stepper { id: primaryUp; objectName: "primaryUp"; visible: false; tone: LV.AbstractButton.Primary; arrow: LV.Stepper.Up }
    LV.Stepper { id: primaryDown; visible: false; tone: LV.AbstractButton.Primary; arrow: LV.Stepper.Down }
    LV.Stepper { id: borderlessUpDown; visible: false; tone: LV.AbstractButton.Borderless; arrow: LV.Stepper.UpDown }
    LV.Stepper { id: borderlessUp; objectName: "borderlessUp"; visible: false; tone: LV.AbstractButton.Borderless; arrow: LV.Stepper.Up }
    LV.Stepper { id: borderlessDown; visible: false; tone: LV.AbstractButton.Borderless; arrow: LV.Stepper.Down }
    property string stepperDebug:
        "default="
        + defaultStepper.width + "," + defaultStepper.height + "," + defaultStepper.implicitWidth + "," + defaultStepper.implicitHeight
        + " upBounds=" + primaryUp.iconBounds.x + "," + primaryUp.iconBounds.y + "," + primaryUp.iconBounds.width + "," + primaryUp.iconBounds.height
        + " downBounds=" + primaryDown.iconBounds.x + "," + primaryDown.iconBounds.y + "," + primaryDown.iconBounds.width + "," + primaryDown.iconBounds.height
        + " upDownBounds=" + primaryUpDown.iconBounds.x + "," + primaryUpDown.iconBounds.y + "," + primaryUpDown.iconBounds.width + "," + primaryUpDown.iconBounds.height
        + " colors=" + primaryUp.backgroundColor + "," + borderlessUp.backgroundColor + "," + borderlessUp.backgroundColorHover + "," + borderlessUp.backgroundColorPressed

    property bool stepperContractReady:
        defaultStepper.tone === LV.AbstractButton.Primary
        && defaultStepper.arrow === LV.Stepper.UpDown
        && Math.abs(defaultStepper.width - LV.Theme.iconSm) < 0.01
        && Math.abs(defaultStepper.height - LV.Theme.iconSm) < 0.01
        && Math.abs(defaultStepper.implicitWidth - LV.Theme.iconSm) < 0.01
        && Math.abs(defaultStepper.implicitHeight - LV.Theme.iconSm) < 0.01
        && Math.abs(defaultStepper.cornerRadius - LV.Theme.radiusSm) < 0.01
        && Math.abs(primaryUp.iconWidth - expectedChevronWidth) < 0.01
        && Math.abs(primaryUp.iconHeight - expectedChevronHeight) < 0.01
        && Math.abs(primaryDown.iconWidth - expectedChevronWidth) < 0.01
        && Math.abs(primaryDown.iconHeight - expectedChevronHeight) < 0.01
        && Math.abs(primaryUpDown.iconWidth - expectedUpDownWidth) < 0.01
        && Math.abs(primaryUpDown.iconHeight - expectedUpDownHeight) < 0.01
        && Math.abs(borderlessUpDown.iconWidth - expectedUpDownWidth) < 0.01
        && Math.abs(borderlessUpDown.iconHeight - expectedUpDownHeight) < 0.01
        && Math.abs(primaryUp.iconBounds.x - expectedUpX) < 0.06
        && Math.abs(primaryUp.iconBounds.y - expectedUpY) < 0.06
        && Math.abs(primaryUp.iconBounds.width - expectedChevronWidth) < 0.01
        && Math.abs(primaryUp.iconBounds.height - expectedChevronHeight) < 0.01
        && Math.abs(primaryDown.iconBounds.x - expectedUpX) < 0.06
        && Math.abs(primaryDown.iconBounds.y - expectedUpY) < 0.06
        && Math.abs(primaryDown.iconBounds.width - expectedChevronWidth) < 0.01
        && Math.abs(primaryDown.iconBounds.height - expectedChevronHeight) < 0.01
        && Math.abs(primaryUpDown.iconBounds.x - expectedUpDownX) < 0.06
        && Math.abs(primaryUpDown.iconBounds.y - expectedUpDownY) < 0.06
        && Math.abs(primaryUpDown.iconBounds.width - expectedUpDownWidth) < 0.01
        && Math.abs(primaryUpDown.iconBounds.height - expectedUpDownHeight) < 0.01
        && primaryUp.backgroundColor === LV.Theme.primary
        && borderlessUp.backgroundColor === transparentColor
        && borderlessUp.backgroundColorHover === LV.Theme.surfaceAlt
        && borderlessUp.backgroundColorPressed === LV.Theme.accentBlueMuted
        && primaryUp.renderedBackgroundColor === LV.Theme.primary
        && borderlessUp.renderedBackgroundColor === transparentColor
        && primaryUp.resolvedIconName === "StepperUpPrimary"
        && primaryDown.resolvedIconName === "StepperDownPrimary"
        && borderlessUp.resolvedIconName === "StepperUpBorderless"
        && borderlessDown.resolvedIconName === "StepperDownBorderless"
        && primaryUp.resolvedIconAssetName === "StepperChevron"
        && primaryDown.resolvedIconAssetName === "StepperChevron"
        && primaryUpDown.resolvedIconAssetName === "StepperUpDownChevron"
        && primaryUp.iconSource == iconRoot + "StepperChevron.svg"
        && primaryDown.iconSource == iconRoot + "StepperChevron.svg"
        && primaryUpDown.iconSource == iconRoot + "StepperUpDownChevron.svg"
        && primaryUp.iconRotation === 180
        && primaryDown.iconRotation === 0
        && primaryUpDown.iconRotation === 0
        && primaryUp.iconSourceWidth === Math.ceil(expectedChevronWidth * primaryUp.iconRasterScale)
        && primaryUp.iconSourceHeight === Math.ceil(expectedChevronHeight * primaryUp.iconRasterScale)
        && primaryUpDown.iconSourceWidth === Math.ceil(expectedUpDownWidth * primaryUpDown.iconRasterScale)
        && primaryUpDown.iconSourceHeight === Math.ceil(expectedUpDownHeight * primaryUpDown.iconRasterScale)
        && primaryUp.resolvedIconColor === LV.Theme.accentWhite
        && primaryDown.resolvedIconColor === LV.Theme.accentWhite
        && borderlessUp.resolvedIconColor === LV.Theme.accentWhite
        && borderlessDown.resolvedIconColor === LV.Theme.accentWhite
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    const QString stepperDebug = root->property("stepperDebug").toString();
    CTEST_CHECK_MESSAGE(root->property("stepperContractReady").toBool(), qPrintable(stepperDebug));

    const auto verifyRenderedVariant = [&root](const QString &controlName,
                                                qreal expectedRotation) {
        QObject *control = root->findChild<QObject *>(controlName, Qt::FindChildrenRecursively);
        QObject *background = root->findChild<QObject *>(controlName + QStringLiteral("_background"),
                                                         Qt::FindChildrenRecursively);
        QObject *icon = root->findChild<QObject *>(controlName + QStringLiteral("_iconSnapshot"),
                                                   Qt::FindChildrenRecursively);
        CTEST_CHECK(control);
        CTEST_CHECK(background);
        CTEST_CHECK(icon);
        IMPORT_COMPARE(background->property("color"), control->property("renderedBackgroundColor"));
        CTEST_CHECK(qAbs(background->property("radius").toReal() - control->property("cornerRadius").toReal()) < 0.01);
        IMPORT_COMPARE(icon->property("source"), control->property("renderedIconSource"));
        IMPORT_COMPARE(icon->property("status").toInt(), 1);
        CTEST_CHECK(qAbs(icon->property("x").toReal() - control->property("iconBounds").toRectF().x()) < 0.01);
        CTEST_CHECK(qAbs(icon->property("y").toReal() - control->property("iconBounds").toRectF().y()) < 0.01);
        CTEST_CHECK(qAbs(icon->property("width").toReal() - control->property("iconWidth").toReal()) < 0.01);
        CTEST_CHECK(qAbs(icon->property("height").toReal() - control->property("iconHeight").toReal()) < 0.01);
        CTEST_CHECK(qAbs(icon->property("rotation").toReal() - expectedRotation) < 0.01);
    };

    verifyRenderedVariant(QStringLiteral("primaryUpDown"), 0.0);
    verifyRenderedVariant(QStringLiteral("primaryUp"), 180.0);
    verifyRenderedVariant(QStringLiteral("borderlessUp"), 180.0);
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
    LV.ComboBox { id: scopedToneCombo; visible: false; tone: LV.ComboBox.Tone.Primary }

    property bool comboBoxContractReady:
        defaultCombo.tone === LV.ComboBox.Primary
        && defaultCombo.arrow === LV.Stepper.UpDown
        && defaultCombo.text === "Label"
        && customTextCombo.text === "Control"
        && Math.abs(defaultCombo.width - 97.0) < 0.01
        && Math.abs(defaultCombo.height - 20.0) < 0.01
        && Math.abs(defaultCombo.implicitWidth - 97.0) < 0.01
        && Math.abs(defaultCombo.implicitHeight - 20.0) < 0.01
        && Math.abs(defaultCombo.figmaComboWidth - 97.0) < 0.01
        && Math.abs(defaultCombo.figmaComboHeight - 20.0) < 0.01
        && Math.abs(defaultCombo.figmaComboLeftPadding - 8.0) < 0.01
        && Math.abs(defaultCombo.figmaComboRightPadding - 1.0) < 0.01
        && Math.abs(defaultCombo.figmaComboVerticalPadding - 1.0) < 0.01
        && Math.abs(defaultCombo.figmaComboCornerRadius - 5.0) < 0.01
        && Math.abs(defaultCombo.figmaIndicatorSize - 18.0) < 0.01
        && Math.abs(defaultCombo.figmaLabelLineHeight - 13.0) < 0.01
        && defaultCombo.resolvedTone === LV.ComboBox.Primary
        && primaryUp.resolvedTone === LV.ComboBox.Primary
        && primaryDown.resolvedTone === LV.ComboBox.Primary
        && borderlessUp.resolvedTone === LV.ComboBox.Borderless
        && borderlessDown.resolvedTone === LV.ComboBox.Borderless
        && scopedToneCombo.resolvedTone === LV.ComboBox.Primary
        && defaultCombo.resolvedArrow === LV.Stepper.UpDown
        && primaryUp.resolvedArrow === LV.Stepper.Up
        && primaryDown.resolvedArrow === LV.Stepper.Down
        && borderlessUpDown.resolvedArrow === LV.Stepper.UpDown
        && borderlessUp.resolvedArrow === LV.Stepper.Up
        && borderlessDown.resolvedArrow === LV.Stepper.Down
        && Math.abs(defaultCombo.labelBounds.x - 8.0) < 0.01
        && Math.abs(defaultCombo.labelBounds.y - 3.5) < 0.01
        && Math.abs(defaultCombo.labelBounds.width - 70.0) < 0.01
        && Math.abs(defaultCombo.labelBounds.height - 13.0) < 0.01
        && Math.abs(defaultCombo.indicatorBounds.x - 78.0) < 0.01
        && Math.abs(defaultCombo.indicatorBounds.y - 1.0) < 0.01
        && Math.abs(defaultCombo.indicatorBounds.width - 18.0) < 0.01
        && Math.abs(defaultCombo.indicatorBounds.height - 18.0) < 0.01
        && defaultCombo.backgroundColor === LV.Theme.panelBackground10
        && defaultCombo.backgroundColorHover === LV.Theme.panelBackground11
        && defaultCombo.backgroundColorPressed === LV.Theme.panelBackground12
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_CHECK(root->property("comboBoxContractReady").toBool());
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
    id: root
    width: 698
    height: 84

    property string themeTarget: "macos"
    readonly property real expectedScale: 1.0

    onThemeTargetChanged: LV.Theme.targetOverride = themeTarget
    Component.onCompleted: LV.Theme.targetOverride = themeTarget
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.InputField {
        id: defaultField
        objectName: "figmaInputRounded"
        x: 20
        y: 20
        visible: false
        width: implicitWidth
        height: implicitHeight
        placeholderText: "Placeholder"
    }

    LV.InputField {
        id: searchField
        objectName: "figmaInputSearch"
        x: 20
        y: 45
        visible: false
        width: implicitWidth
        height: implicitHeight
        search: true
        placeholderText: "Search"
        text: "abc"
    }

    LV.InputField {
        id: legacySearchField
        visible: false
        width: implicitWidth
        height: implicitHeight
        mode: searchMode
        placeholderText: "Legacy search"
    }

    LV.InputField {
        id: passwordField
        objectName: "figmaPasswordField"
        visible: false
        placeholderText: "Password"
        mode: passwordMode
        text: "secret"
    }

    LV.InputField {
        id: readOnlyField
        visible: false
        placeholderText: "Read only"
        readOnly: true
        text: "locked"
    }

    LV.InputField {
        id: inlineField
        objectName: "figmaInputInline"
        x: 472
        y: 20
        visible: false
        width: implicitWidth
        height: implicitHeight
        style: inlineStyle
        placeholderText: "Inline"
        text: "value"
    }

    LV.InputField {
        id: inlineDisabledField
        visible: false
        enabled: false
        width: implicitWidth
        height: implicitHeight
        style: inlineStyle
        placeholderText: "Inline disabled"
    }

    LV.InputField {
        id: cylinderField
        objectName: "figmaInputCylinder"
        x: 246
        y: 20
        visible: false
        width: implicitWidth
        height: implicitHeight
        style: cylinderStyle
        placeholderText: "Cylinder"
    }

    Repeater {
        model: 18
        LV.InputField {
            required property int index
            objectName: "inputCombination" + index
            visible: false
            width: 206
            height: 22
            style: Math.floor(index / 6)
            mode: Math.floor((index % 6) / 2)
            enabled: index % 2 === 0
            text: "secret"
            readonly property bool comboReady:
                (style === roundedStyle ? resolvedCornerRadius === 6
                    : style === inlineStyle ? resolvedCornerRadius === 0
                    : resolvedCornerRadius === 11)
                && (mode === passwordMode ? echoMode === TextInput.Password && displayText !== text
                    : echoMode === TextInput.Normal)
                && searchIconVisible === (mode === searchMode)
                && showClearButton === enabled
        }
    }

    property string figmaInputFieldDiagnostics:
        "target=" + LV.Theme.effectiveTarget
        + " default=" + defaultField.implicitWidth + "x" + defaultField.implicitHeight
        + "/" + defaultField.width + "x" + defaultField.height
        + " field=" + defaultField.fieldMinHeight + "/" + defaultField.centeredTextHeight
        + " text=" + defaultField.inputItem.width + "x" + defaultField.inputItem.height
        + " font=" + defaultField.inputItem.font.pixelSize + "/" + defaultField.inputItem.font.weight
        + " inset=" + defaultField.leftInset + "/" + defaultField.rightInset
        + " y=" + defaultField.centeredTextY
        + " radius=" + defaultField.cornerRadius
        + "/token=" + LV.Theme.radiusBase
        + " style=" + defaultField.resolvedStyle + "/" + defaultField.shapeStyle
        + " inline=" + inlineField.resolvedStyle + "/" + inlineField.shapeStyle
        + " cylinder=" + cylinderField.resolvedStyle + "/" + cylinderField.shapeStyle
        + "/" + cylinderField.resolvedCornerRadius
        + " search=" + searchField.searchIconSize + "/" + searchField.clearIconSize
        + "/" + searchField.leftInset + "/" + searchField.rightInset

    property bool figmaInputFieldReady:
        defaultField.textMode === defaultField.defaultMode
        && defaultField.roundedStyle === defaultField.filledStyle
        && defaultField.resolvedStyle === defaultField.roundedStyle
        && defaultField.shapeStyle === defaultField.shapeRoundRect
        && defaultField.backgroundColor === LV.Theme.inputFieldGlassTint
        && defaultField.backgroundColorFocused === LV.Theme.inputFieldGlassTint
        && defaultField.backgroundColorDisabled === LV.Theme.inputFieldGlassTintDisabled
        && Math.abs(defaultField.backgroundColor.a - 0.64) < 0.005
        && Math.abs(defaultField.backgroundColorDisabled.a - 0.36) < 0.005
        && defaultField.glassBlurRadius === 8 * expectedScale
        && defaultField.implicitWidth === 206 * expectedScale
        && defaultField.implicitHeight === 22 * expectedScale
        && defaultField.width === 206 * expectedScale
        && defaultField.height === 22 * expectedScale
        && defaultField.fieldMinHeight === 22 * expectedScale
        && defaultField.insetHorizontal === 7 * expectedScale
        && defaultField.insetVertical === 4.5 * expectedScale
        && defaultField.sideSpacing === 2 * expectedScale
        && defaultField.cornerRadius === 6 * expectedScale
        && defaultField.centeredTextHeight === 13
        && defaultField.textLineBoxHeight === 13
        && defaultField.inputItem.height === 13
        && defaultField.inputItem.font.pixelSize === 13
        && defaultField.inputItem.font.weight === Font.Medium
        && defaultField.leftInset === 7 * expectedScale
        && defaultField.rightInset === 7 * expectedScale
        && defaultField.centeredTextY === 4.5
        && defaultField.textColor === LV.Theme.titleHeaderColor
        && defaultField.textColorDisabled === LV.Theme.disabledColor
        && defaultField.placeholderColor === LV.Theme.titleHeaderColor
        && defaultField.placeholderColorDisabled === LV.Theme.disabledColor
        && Math.abs(defaultField.placeholderOpacity - 1.0) < 0.001
        && defaultField.searchIconColor === LV.Theme.accentGrayLight
        && defaultField.clearIconBackgroundColor === LV.Theme.descriptionColor
        && defaultField.clearIconBackgroundColorDisabled === LV.Theme.disabledColor
        && defaultField.clearIconForegroundColor === LV.Theme.panelBackground10
        && inlineField.style === inlineField.inlineStyle
        && inlineField.resolvedStyle === inlineField.inlineStyle
        && inlineField.shapeStyle === inlineField.shapeRoundRect
        && inlineField.resolvedCornerRadius === 0
        && inlineField.implicitWidth === 206 * expectedScale
        && inlineField.implicitHeight === 22 * expectedScale
        && inlineField.backgroundColor === LV.Theme.inputFieldGlassTintInline
        && inlineField.backgroundColorHover === inlineField.backgroundColor
        && inlineField.backgroundColorPressed === inlineField.backgroundColor
        && inlineField.backgroundColorFocused === inlineField.backgroundColor
        && inlineField.backgroundColorDisabled === inlineField.backgroundColor
        && inlineDisabledField.backgroundColor === inlineField.backgroundColor
        && inlineDisabledField.backgroundColorDisabled === inlineField.backgroundColor
        && Math.abs(inlineField.backgroundColor.a - 0.16) < 0.005
        && inlineField.glassBlurRadius === 6 * expectedScale
        && inlineField.showClearButton
        && cylinderField.resolvedStyle === cylinderField.cylinderStyle
        && cylinderField.shapeStyle === cylinderField.shapeCylinder
        && Math.abs(cylinderField.resolvedCornerRadius - (22 * expectedScale / 2.0)) < 0.01
        && cylinderField.backgroundColor === LV.Theme.inputFieldGlassTint
        && cylinderField.implicitWidth === 206 * expectedScale
        && cylinderField.implicitHeight === 22 * expectedScale
        && searchField.search
        && searchField.searchIconVisible
        && searchField.searchIconSize === 12 * expectedScale
        && searchField.clearIconSize === 12 * expectedScale
        && Math.abs(searchField.clearIconMarkLength - (12 * expectedScale * (8.0 / 14.0))) < 0.01
        && Math.abs(searchField.clearIconMarkThickness - (12 * expectedScale * (1.4 / 14.0))) < 0.01
        && searchField.leftInset === 21 * expectedScale
        && searchField.rightInset === 8 + 14 * expectedScale
        && searchField.implicitWidth === 206 * expectedScale
        && searchField.implicitHeight === 22 * expectedScale
        && searchField.showClearButton
        && legacySearchField.mode === legacySearchField.searchMode
        && legacySearchField.search
        && legacySearchField.searchIconVisible
        && passwordField.echoMode === TextInput.Password
        && passwordField.displayText !== passwordField.text
        && !passwordField.searchIconVisible
        && passwordField.height === 22
        && readOnlyField.readOnly
        && readOnlyField.height === 22
        && inlineDisabledField.height === 22
        && inlineDisabledField.placeholderColorDisabled === LV.Theme.disabledColor
        && !readOnlyField.showClearButton
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    auto *rootItem = qobject_cast<QQuickItem *>(root.data());
    auto *roundedItem = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaInputRounded")));
    auto *cylinderItem = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaInputCylinder")));
    auto *inlineItem = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaInputInline")));
    auto *searchItem = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaInputSearch")));
    CTEST_CHECK(rootItem);
    CTEST_CHECK(roundedItem);
    CTEST_CHECK(cylinderItem);
    CTEST_CHECK(inlineItem);
    CTEST_CHECK(searchItem);
    for (int index = 0; index < 18; ++index) {
        auto *combination = visualChildByObjectName(rootItem, QStringLiteral("inputCombination%1").arg(index));
        CTEST_CHECK(combination);
        IMPORT_WAIT_MESSAGE(combination->property("comboReady").toBool(), qPrintable(combination->objectName()));
    }

    const auto verifyClearPadding = [](QQuickItem *field) {
        auto *clear = field->findChild<QQuickItem *>(field->objectName() + "_clearButton");
        CTEST_CHECK(clear);
        CTEST_WAIT_DEFAULT(field->property("showClearButton").toBool());
        CTEST_WAIT_DEFAULT(qAbs(field->width() - clear->mapToItem(field, QPointF()).x()
                         - clear->width() - 8.0) < 0.01);
        IMPORT_WAIT_COMPARE(clear->mapToItem(field, QPointF()).y() + clear->height() / 2,
                     field->height() / 2);
        auto *input = field->property("inputItem").value<QQuickItem *>();
        CTEST_CHECK(input);
        CTEST_WAIT_DEFAULT(qAbs(clear->mapToItem(field, QPointF()).x() - input->x()
                         - input->width() - field->property("sideSpacing").toReal()) < 0.01);
    };
    const QList<QQuickItem *> fields = {roundedItem, cylinderItem, inlineItem, searchItem};
    for (const QString &target : {QStringLiteral("macos"), QStringLiteral("ios"), QStringLiteral("android"),
                                  QStringLiteral("macos")}) {
        CTEST_CHECK(root->setProperty("themeTarget", target));
        IMPORT_WAIT_MESSAGE(root->property("figmaInputFieldReady").toBool(),
                     qPrintable(root->property("figmaInputFieldDiagnostics").toString()));
        for (QQuickItem *field : fields) {
            auto *input = field->property("inputItem").value<QQuickItem *>();
            CTEST_CHECK(input);
            IMPORT_COMPARE(field->height(), 22.0);
            IMPORT_COMPARE(input->y(), 4.5);
            IMPORT_COMPARE(input->height(), 13.0);
            const QString originalText = field->property("text").toString();
            CTEST_CHECK(field->setProperty("text", QStringLiteral("Filled value")));
            verifyClearPadding(field);
            CTEST_CHECK(field->setProperty("text", originalText));
        }
        auto *searchIcon = searchItem->findChild<QQuickItem *>(QStringLiteral("figmaInputSearch_searchIconImage"));
        CTEST_CHECK(searchIcon);
        IMPORT_COMPARE(searchIcon->mapToItem(searchItem, QPointF()).y(), 5.0);
        IMPORT_COMPARE(searchIcon->height(), 12.0);
    }
    auto *passwordItem = root->findChild<QObject *>(QStringLiteral("figmaPasswordField"));
    CTEST_CHECK(passwordItem);
    const int passwordEchoMode = passwordItem->property("echoMode").toInt();
    CTEST_CHECK(passwordItem->setProperty("mode", passwordItem->property("searchMode")));
    CTEST_WAIT_DEFAULT(passwordItem->property("searchIconVisible").toBool());
    CTEST_CHECK(passwordItem->property("echoMode").toInt() != passwordEchoMode);
    CTEST_CHECK(passwordItem->setProperty("mode", passwordItem->property("passwordMode")));
    CTEST_WAIT_DEFAULT(!passwordItem->property("searchIconVisible").toBool());
    IMPORT_COMPARE(passwordItem->property("echoMode").toInt(), passwordEchoMode);

    // Fractional padding follows an explicit minimum height without clipping the line box.
    CTEST_CHECK(roundedItem->setProperty("fieldMinHeight", 30));
    IMPORT_WAIT_COMPARE(roundedItem->height(), 30.0);
    IMPORT_COMPARE(roundedItem->property("insetVertical").toReal(), 8.5);
    IMPORT_COMPARE(roundedItem->property("inputItem").value<QQuickItem *>()->y(), 8.5);
    CTEST_CHECK(roundedItem->setProperty("fieldMinHeight", 22));
    IMPORT_WAIT_COMPARE(roundedItem->height(), 22.0);

    searchItem->setWidth(333);
    CTEST_CHECK(searchItem->setProperty("insetHorizontal", 19));
    verifyClearPadding(searchItem);
    CTEST_CHECK(searchItem->setProperty("text", QString()));
    CTEST_CHECK(!searchItem->property("showClearButton").toBool());
    IMPORT_WAIT_COMPARE(searchItem->property("rightInset").toInt(), 19);
    CTEST_CHECK(searchItem->setProperty("text", QStringLiteral("abc")));
    verifyClearPadding(searchItem);
    searchItem->setWidth(206);
    CTEST_CHECK(searchItem->setProperty("insetHorizontal", 7));

    roundedItem->setVisible(true);
    cylinderItem->setVisible(true);
    inlineItem->setVisible(true);
    searchItem->setVisible(true);

    QQuickWindow captureWindow;
    captureWindow.setColor(Qt::transparent);
    captureWindow.resize(698, 84);
    rootItem->setParentItem(captureWindow.contentItem());
    rootItem->setPosition(QPointF(0.0, 0.0));
    rootItem->setSize(QSizeF(698.0, 84.0));
    rootItem->setVisible(true);
    captureWindow.show();
    QCoreApplication::processEvents();
    CTEST_CHECK_MESSAGE(waitForInputMaterials({roundedItem, cylinderItem, inlineItem, searchItem}),
             "Input field material painting did not complete");

    processFor(120);
    const QSharedPointer<QQuickItemGrabResult> grabResult = rootItem->grabToImage(QSize(698, 84));
    CTEST_CHECK(grabResult);
    CTEST_WAIT_VERIFY(!grabResult->image().isNull(), 5000);
    const QImage captured = grabResult->image().convertToFormat(QImage::Format_RGBA8888);
    const qreal captureScale = static_cast<qreal>(captured.width()) / 698.0;
    CTEST_CHECK(captureScale >= 1.0);
    IMPORT_COMPARE(captured.height(), qRound(84.0 * captureScale));

    const auto logicalPixel = [&captured, captureScale](int x, int y) {
        return captured.pixelColor(qRound(x * captureScale), qRound(y * captureScale));
    };

    // The material stays translucent without a backdrop source. Inline is lighter.
    const QColor roundedFill = logicalPixel(150, 29);
    const QColor cylinderFill = logicalPixel(396, 29);
    const QColor inlineFill = logicalPixel(622, 29);
    qInfo() << "TextField material alpha:" << roundedFill.alphaF() << inlineFill.alphaF();
    CTEST_CHECK(roundedFill.alphaF() > 0.63 && roundedFill.alphaF() < 0.70);
    CTEST_CHECK(qAbs(roundedFill.alpha() - cylinderFill.alpha()) <= 2);
    CTEST_CHECK(inlineFill.alphaF() > 0.15 && inlineFill.alphaF() < 0.22);
    CTEST_CHECK(logicalPixel(225, 29).alpha() > 0);
    IMPORT_COMPARE(logicalPixel(226, 29).alpha(), 0);
    IMPORT_COMPARE(logicalPixel(20, 20).alpha(), 0);
    IMPORT_COMPARE(logicalPixel(246, 20).alpha(), 0);

    int searchGlyphPixels = 0;
    for (int y = 50; y < 62; ++y) {
        for (int x = 27; x < 39; ++x) {
            if (logicalPixel(x, y).lightness() > 100)
                ++searchGlyphPixels;
        }
    }
    CTEST_CHECK(searchGlyphPixels > 8);

    int clearGlyphPixels = 0;
    for (int y = 50; y < 62; ++y) {
        for (int x = 206; x < 218; ++x) {
            if (logicalPixel(x, y).lightness() > 100)
                ++clearGlyphPixels;
        }
    }
    CTEST_CHECK(clearGlyphPixels > 16);

    const QString capturePath = qEnvironmentVariable("LVRS_INPUT_FIELD_CAPTURE_PATH");
    if (!capturePath.isEmpty())
        CTEST_CHECK_MESSAGE(captured.save(capturePath), qPrintable(capturePath));

    auto *clear = searchItem->findChild<QQuickItem *>(QStringLiteral("figmaInputSearch_clearButton"));
    CTEST_CHECK(clear);
    mouseClick(&captureWindow, Qt::LeftButton, Qt::NoModifier,
                     scenePoint(clear, QPointF(clear->width() / 2, clear->height() / 2)));
    IMPORT_WAIT_COMPARE(searchItem->property("text").toString(), QString());
    CTEST_CHECK(!searchItem->property("showClearButton").toBool());
    CTEST_WAIT_DEFAULT(searchItem->property("focused").toBool());
}

void ImportApiTests::input_field_material_rendering_contract()
{
    QQmlEngine engine;
    engine.addImportPath(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/.."));
    const QByteArray qml = R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV

Controls.ApplicationWindow {
    id: window
    width: 718
    height: 294
    visible: true
    property bool detailedBackdrop: false
    property real contentOffset: 0
    property color stripeColor: "#f0d189"
    readonly property color expectedPlaceholderColor: LV.Theme.titleHeaderColor
    readonly property color expectedDisabledPlaceholderColor: LV.Theme.disabledColor
    Component.onCompleted: LV.Theme.targetOverride = "macos"
    Component.onDestruction: LV.Theme.targetOverride = ""
    background: Rectangle {
        objectName: "materialBackdrop"
        color: window.detailedBackdrop ? "#478de2" : "#1f1f1f"
        Repeater {
            model: window.detailedBackdrop ? 120 : 0
            Rectangle {
                required property int index
                x: index * 6
                width: 3
                height: window.height
                color: window.stripeColor
            }
        }
    }
    Item {
        x: window.contentOffset
        width: window.width
        height: window.height
        Repeater {
            model: 18
            LV.InputField {
                required property int index
                objectName: "materialField" + index
                readonly property int stateIndex: index % 6
                x: 20 + Math.floor(index / 6) * 230
                y: 24 + stateIndex * 42
                style: index < 6 ? roundedStyle : index < 12 ? cylinderStyle : inlineStyle
                enabled: stateIndex !== 1
                placeholderText: enabled ? "Placeholder" : "Disabled"
                text: stateIndex >= 2 ? stateIndex === 3 ? "Selected text" : "Editable text" : ""
                search: stateIndex === 5
                persistentSelection: true
                Component.onCompleted: {
                    if (stateIndex === 3)
                        selectAll()
                }
            }
        }
    }
}
)";
    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    QList<QQuickItem *> fields;
    for (int i = 0; i < 18; ++i) {
        auto *field = visualChildByObjectName(window->contentItem(),
                                             QStringLiteral("materialField%1").arg(i));
        CTEST_CHECK(field);
        fields.append(field);
        auto *materialCanvas = visualChildByObjectName(field, QStringLiteral("inputFieldMaterial"));
        CTEST_CHECK(materialCanvas);
        const auto strategyIndex = materialCanvas->metaObject()->indexOfEnumerator("RenderStrategy");
        CTEST_CHECK(strategyIndex >= 0);
        const auto strategy = materialCanvas->metaObject()->enumerator(strategyIndex);
        IMPORT_COMPARE(materialCanvas->property("renderStrategy").toInt(), strategy.keyToValue("Threaded"));
        IMPORT_COMPARE(field->size(), QSizeF(206, 22));
        auto *input = field->property("inputItem").value<QQuickItem *>();
        CTEST_CHECK(input);
        IMPORT_COMPARE(input->y(), 4.5);
        IMPORT_COMPARE(input->height(), 13.0);
        QQuickItem *placeholder = nullptr;
        for (auto *candidate : field->findChildren<QQuickItem *>()) {
            if (candidate->property("text").toString() == field->property("placeholderText").toString()) {
                placeholder = candidate;
                break;
            }
        }
        CTEST_CHECK(placeholder);
        IMPORT_COMPARE(placeholder->y(), input->y());
        IMPORT_COMPARE(placeholder->height(), input->height());
        IMPORT_COMPARE(placeholder->property("color").value<QColor>().rgba(),
                 root->property(i % 6 == 1 ? "expectedDisabledPlaceholderColor" : "expectedPlaceholderColor").value<QColor>().rgba());
        IMPORT_COMPARE(placeholder->property("visible").toBool(), i % 6 < 2);
        IMPORT_WAIT_COMPARE(field->property("glassActive").toBool(), i % 6 != 1);
    }
    CTEST_CHECK_MESSAGE(waitForInputMaterials(fields), "Input field material painting did not complete");
    const auto grab = [window]() -> QImage {
        processFor(120);
        const auto result = window->contentItem()->grabToImage(window->size());
        if (!result)
            return {};
        QElapsedTimer timer;
        timer.start();
        while (result->image().isNull() && timer.elapsed() < 5000)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        return result->image().scaled(window->size()).convertToFormat(QImage::Format_RGB32);
    };
    const QImage material = grab();
    CTEST_CHECK(!material.isNull());
    const auto depth = [](const QImage &image, QQuickItem *field) {
        const QPoint p = scenePoint(field, QPointF(150, 0));
        return qGray(image.pixel(p + QPoint(0, qRound(field->height()) - 1)))
            - qGray(image.pixel(p + QPoint(0, 1)));
    };
    for (int column = 0; column < 3; ++column) {
        QQuickItem *field = fields[column * 6];
        CTEST_CHECK_MESSAGE(depth(material, field) > (column == 2 ? 1 : 7),
                 "The lower inner reflection must be brighter than the recessed top.");
        CTEST_CHECK(depth(material, field) > depth(material, fields[column * 6 + 1]));
        const QPoint p = scenePoint(field, QPointF(150, 0));
        CTEST_CHECK_MESSAGE(qGray(material.pixel(p + QPoint(0, qRound(field->height()) - 1)))
                     > qGray(material.pixel(p + QPoint(0, qRound(field->height() / 2)))) + (column == 2 ? 2 : 10),
                 "The material must include the lower rim, including when supersampled.");
        IMPORT_COMPARE(material.pixelColor(p + QPoint(0, -1)), QColor("#1f1f1f"));
        IMPORT_COMPARE(material.pixelColor(p + QPoint(0, qRound(field->height()))), QColor("#1f1f1f"));
    }

    CTEST_CHECK(root->setProperty("detailedBackdrop", true));
    const QImage frosted = grab();
    for (QQuickItem *field : fields)
        CTEST_CHECK(field->setProperty("glassEnabled", false));
    const QImage unblurred = grab();
    const QString captureDir = qEnvironmentVariable("LVRS_INPUT_MATERIAL_CAPTURE_DIR");
    if (!captureDir.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(captureDir));
        CTEST_CHECK(material.save(captureDir + "/textfield-material.png"));
        CTEST_CHECK(frosted.save(captureDir + "/textfield-frosted.png"));
        CTEST_CHECK(unblurred.save(captureDir + "/textfield-without-blur.png"));
    }
    const auto variation = [](const QImage &image, QQuickItem *field) {
        const QPoint p = scenePoint(field, QPointF(95, 6));
        double sum = 0;
        for (int y = p.y(); y < p.y() + 7; ++y) {
            for (int x = p.x(); x < p.x() + 75; ++x)
                sum += qAbs(qGray(image.pixel(x, y)) - qGray(image.pixel(x + 1, y)));
        }
        return sum;
    };
    const bool software = window->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
    for (int column = 0; column < 3; ++column) {
        auto *field = fields[column * 6];
        const double sharpDetail = variation(unblurred, field);
        const double frostedDetail = variation(frosted, field);
        CTEST_CHECK_MESSAGE(sharpDetail > 1000, "The fixture must show fine detail through the material.");
        qInfo() << "TextField style" << column << "backdrop detail:"
                << frostedDetail << "unblurred:" << sharpDetail;
        if (software)
            IMPORT_COMPARE(frostedDetail, sharpDetail);
        else
            // MultiEffect mixes blur levels (including some source detail at 6/8px).
            // Require a substantial reduction without changing the Figma blur radii.
            CTEST_CHECK_MESSAGE(frostedDetail < sharpDetail * 0.60, "The actual backdrop must be blurred.");
        IMPORT_COMPARE(variation(frosted, fields[column * 6 + 1]),
                 variation(unblurred, fields[column * 6 + 1]));
    }
    for (QQuickItem *field : fields)
        CTEST_CHECK(field->setProperty("glassEnabled", true));

    auto *field = fields.first();
    auto *capture = visualChildByObjectName(field, QStringLiteral("inputFieldGlassCapture"));
    CTEST_CHECK(capture);
    CTEST_CHECK(!capture->property("recursive").toBool());
    CTEST_CHECK(!capture->property("hideSource").toBool());
    const QRectF beforeMove = capture->property("sourceRect").toRectF();
    CTEST_CHECK(root->setProperty("contentOffset", 11));
    IMPORT_WAIT_COMPARE(capture->property("sourceRect").toRectF().x(), beforeMove.x() + 11);
    IMPORT_COMPARE(capture->property("sourceRect").toRectF().size(), beforeMove.size());
    CTEST_CHECK(root->setProperty("contentOffset", 0));

    // A live sibling source updates, while capturing the field or an ancestor is rejected.
    CTEST_CHECK(root->setProperty("stripeColor", QColor("#e74a62")));
    const QImage changedBackdrop = grab();
    if (!software)
        CTEST_CHECK(changedBackdrop.pixelColor(170, 33) != frosted.pixelColor(170, 33));
    for (QQuickItem *unsafeSource : {window->contentItem(), field,
                                    field->property("inputItem").value<QQuickItem *>()}) {
        CTEST_CHECK(field->setProperty("backdropSource", QVariant::fromValue(unsafeSource)));
        CTEST_WAIT_DEFAULT(!field->property("glassActive").toBool());
        CTEST_CHECK(!capture->property("sourceItem").value<QQuickItem *>());
    }
    auto *backdrop = root->findChild<QQuickItem *>("materialBackdrop");
    CTEST_CHECK(backdrop);
    CTEST_CHECK(field->setProperty("backdropSource", QVariant::fromValue(backdrop)));
    CTEST_WAIT_DEFAULT(field->property("glassActive").toBool());
    field->setVisible(false);
    CTEST_CHECK(!capture->property("live").toBool());
    field->setVisible(true);
    CTEST_WAIT_DEFAULT(capture->property("live").toBool());

    // Repainting the material must preserve the public tint override and native input.
    CTEST_CHECK(root->setProperty("detailedBackdrop", false));
    CTEST_CHECK(field->setProperty("backgroundColor", QColor("#994433")));
    const QImage customTint = grab();
    CTEST_CHECK(customTint.pixelColor(170, 33).red() > 130);
    auto *searchField = fields[5];
    auto *clear = visualChildByObjectName(searchField, QStringLiteral("materialField5_clearButton"));
    CTEST_CHECK(clear);
    mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                     scenePoint(clear, QPointF(clear->width() / 2, clear->height() / 2)));
    IMPORT_WAIT_COMPARE(searchField->property("text").toString(), QString());
    CTEST_WAIT_DEFAULT(searchField->property("focused").toBool());
    for (const Qt::Key key : {Qt::Key_T, Qt::Key_Y, Qt::Key_P, Qt::Key_E, Qt::Key_D})
        keyClick(window, key);
    IMPORT_WAIT_COMPARE(searchField->property("text").toString(), QStringLiteral("typed"));
    CTEST_CHECK(QMetaObject::invokeMethod(searchField, "selectAll"));
    IMPORT_COMPARE(searchField->property("selectedText").toString(), QStringLiteral("typed"));
}

void ImportApiTests::progress_bar_range_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.StateModel {
        id: progressState
        values: ({
            minimumValue: -100,
            maximumValue: 100,
            startValue: -50,
            currentValue: 25
        })
    }

    LV.ProgressBar {
        id: segmentBar
        visible: false
        width: 200
        minimumValue: -50
        maximumValue: 150
        startValue: 50
        currentValue: 100
    }

    LV.ProgressBar {
        id: reverseBar
        visible: false
        width: 200
        minimumValue: 0
        maximumValue: 100
        startValue: 80
        currentValue: 20
    }

    LV.ProgressBar {
        id: legacyEndAliasBar
        visible: false
        width: 200
        minimumValue: 0
        endValue: 50
        startValue: 0
        currentValue: 25
    }

    LV.ProgressBar {
        id: zeroRangeBar
        visible: false
        width: 200
        minimumValue: 10
        maximumValue: 10
        startValue: 0
        currentValue: 10
    }

    LV.ProgressBar {
        id: stateBackedBar
        visible: false
        width: 200
        minimumValue: 0
        maximumValue: 10
        startValue: 0
        currentValue: 1
        stateModel: progressState
    }

    Component.onCompleted: progressState.setValue("currentValue", 50)

    property bool progressBarRangeReady:
        segmentBar.minimumValue === -50
        && segmentBar.maximumValue === 150
        && segmentBar.startValue === 50
        && segmentBar.currentValue === 100
        && Math.abs(segmentBar.valueRange - 200) < 0.01
        && Math.abs(segmentBar.normalizedStart - 0.5) < 0.01
        && Math.abs(segmentBar.normalizedCurrent - 0.75) < 0.01
        && Math.abs(segmentBar.fillStart - 0.5) < 0.01
        && Math.abs(segmentBar.fillProgress - 0.25) < 0.01
        && Math.abs(segmentBar.progress - 0.75) < 0.01
        && Math.abs(reverseBar.normalizedStart - 0.8) < 0.01
        && Math.abs(reverseBar.normalizedCurrent - 0.2) < 0.01
        && Math.abs(reverseBar.fillStart - 0.2) < 0.01
        && Math.abs(reverseBar.fillProgress - 0.6) < 0.01
        && legacyEndAliasBar.maximumValue === 50
        && legacyEndAliasBar.endValue === 50
        && Math.abs(legacyEndAliasBar.progress - 0.5) < 0.01
        && Math.abs(zeroRangeBar.valueRange) < 0.01
        && zeroRangeBar.progress === 1
        && zeroRangeBar.fillProgress === 1
        && stateBackedBar.usingStateModel
        && stateBackedBar.minimumValue === 0
        && Math.abs(stateBackedBar.effectiveMinimumValue + 100) < 0.01
        && Math.abs(stateBackedBar.effectiveMaximumValue - 100) < 0.01
        && Math.abs(stateBackedBar.effectiveStartValue + 50) < 0.01
        && Math.abs(stateBackedBar.effectiveCurrentValue - 50) < 0.01
        && Math.abs(stateBackedBar.valueRange - 200) < 0.01
        && Math.abs(stateBackedBar.normalizedStart - 0.25) < 0.01
        && Math.abs(stateBackedBar.normalizedCurrent - 0.75) < 0.01
        && Math.abs(stateBackedBar.fillProgress - 0.5) < 0.01
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("progressBarRangeReady").toBool());
}

void ImportApiTests::control_icons_use_supersampled_raster_contract()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.Stepper {
        objectName: "stepper"
        width: implicitWidth
        height: implicitHeight
        visible: false
    }

    LV.InputField {
        objectName: "searchField"
        width: implicitWidth
        height: implicitHeight
        visible: false
        search: true
        placeholderText: "Search"
    }

    LV.CheckBox {
        objectName: "checkBox"
        width: implicitWidth
        height: implicitHeight
        visible: false
        checked: true
    }

}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);

    const auto assertSnapshotImage = [&](const QString &controlObjectName,
                                         const QString &objectName,
                                         const char *sourcePropertyName,
                                         const char *widthPropertyName,
                                         const char *heightPropertyName) {
        QObject *controlObject = root->findChild<QObject *>(controlObjectName, Qt::FindChildrenRecursively);
        QObject *imageObject = root->findChild<QObject *>(objectName, Qt::FindChildrenRecursively);
        CTEST_CHECK_MESSAGE(controlObject, qPrintable(QStringLiteral("Expected control object %1").arg(controlObjectName)));
        CTEST_CHECK_MESSAGE(imageObject, qPrintable(QStringLiteral("Expected image object %1").arg(objectName)));

        const QSize sourceSize = imageObject->property("sourceSize").toSize();
        const QUrl source = imageObject->property("source").toUrl();
        const QUrl expectedSource = controlObject->property(sourcePropertyName).toUrl();
        const qreal expectedWidth = controlObject->property(widthPropertyName).toReal();
        const qreal expectedHeight = controlObject->property(heightPropertyName).toReal();

        IMPORT_COMPARE(source, expectedSource);
        IMPORT_COMPARE(sourceSize.width(), qRound(expectedWidth));
        IMPORT_COMPARE(sourceSize.height(), qRound(expectedHeight));
    };

    const auto assertSupersampledCanvas = [&](const QString &controlObjectName,
                                              const QString &objectName,
                                              const char *scalePropertyName) {
        QObject *controlObject = root->findChild<QObject *>(controlObjectName, Qt::FindChildrenRecursively);
        QObject *canvasObject = root->findChild<QObject *>(objectName, Qt::FindChildrenRecursively);
        CTEST_CHECK_MESSAGE(controlObject, qPrintable(QStringLiteral("Expected control object %1").arg(controlObjectName)));
        CTEST_CHECK_MESSAGE(canvasObject, qPrintable(QStringLiteral("Expected canvas object %1").arg(objectName)));

        const QSize canvasSize = canvasObject->property("canvasSize").toSize();
        const qreal logicalWidth = canvasObject->property("width").toReal();
        const qreal logicalHeight = canvasObject->property("height").toReal();
        const qreal rasterScale = controlObject->property(scalePropertyName).toReal();

        CTEST_CHECK_MESSAGE(logicalWidth > 0.0, qPrintable(QStringLiteral("%1 width must be positive").arg(objectName)));
        CTEST_CHECK_MESSAGE(logicalHeight > 0.0, qPrintable(QStringLiteral("%1 height must be positive").arg(objectName)));
        CTEST_CHECK_MESSAGE(canvasSize.width() > qRound(logicalWidth),
                 qPrintable(QStringLiteral("%1 canvas width should exceed logical width").arg(objectName)));
        CTEST_CHECK_MESSAGE(canvasSize.height() > qRound(logicalHeight),
                 qPrintable(QStringLiteral("%1 canvas height should exceed logical height").arg(objectName)));
        IMPORT_COMPARE(canvasSize.width(), qCeil(logicalWidth * rasterScale));
        IMPORT_COMPARE(canvasSize.height(), qCeil(logicalHeight * rasterScale));
    };

    assertSnapshotImage(QStringLiteral("stepper"),
                        QStringLiteral("stepper_iconSnapshot"),
                        "renderedIconSource",
                        "iconSourceWidth",
                        "iconSourceHeight");
    assertSnapshotImage(QStringLiteral("searchField"),
                        QStringLiteral("searchField_searchIconImage"),
                        "renderedSearchIconSource",
                        "searchIconSourceSize",
                        "searchIconSourceSize");
    assertSupersampledCanvas(QStringLiteral("checkBox"),
                             QStringLiteral("checkBox_checkmarkCanvas"),
                             "checkmarkRasterScale");
}

void ImportApiTests::input_field_ios_native_text_interaction_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.InputField {
        id: field
        visible: false
        placeholderText: "Search"
    }

    property bool iosNativeTextReady:
        LV.Theme.mobileTarget
        && field.preferNativeGestures
        && field.preferNativeTextInteraction
        && field.renderType === TextInput.NativeRendering
        && field.inputItem.activeFocusOnPress
        && !field.pressed
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("iosNativeTextReady").toBool());
}

void ImportApiTests::input_field_native_event_input_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 360
    height: 120
    visible: false
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0

    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.InputField {
        id: field
        objectName: "inputField"
        width: 280
        height: implicitHeight
        anchors.centerIn: parent
        text: "alpha beta gamma"
        placeholderText: "Search"

        Component.onCompleted: field.inputItem.objectName = "nativeInput"
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    window->show();
    CTEST_WAIT_DEFAULT(window->isVisible());

    QObject *field = root->findChild<QObject *>(QStringLiteral("inputField"));
    CTEST_CHECK(field);
    auto *inputItem = root->findChild<QQuickItem *>(QStringLiteral("nativeInput"));
    CTEST_CHECK(inputItem);

    const QPoint wordPoint = scenePoint(inputItem, QPointF(28.0, inputItem->height() * 0.5));
    const QPoint dragStart = scenePoint(inputItem, QPointF(8.0, inputItem->height() * 0.5));
    const QPoint dragEnd = scenePoint(inputItem, QPointF(132.0, inputItem->height() * 0.5));

    CTEST_CHECK(!mouseAreaContainsScenePoint(root.data(), wordPoint));

    mouseClick(window, Qt::LeftButton, Qt::NoModifier, wordPoint, 10);
    CTEST_WAIT_DEFAULT(inputItem->property("activeFocus").toBool());

    inputItem->setProperty("cursorPosition", 0);
    keyClick(window, Qt::Key_Z, Qt::NoModifier, 10);
    CTEST_WAIT_DEFAULT(field->property("text").toString().startsWith(QStringLiteral("z")));

    QInputMethodEvent preeditEvent(QStringLiteral("preedit"), {});
    QCoreApplication::sendEvent(inputItem, &preeditEvent);
    IMPORT_WAIT_COMPARE(inputItem->property("preeditText").toString(), QStringLiteral("preedit"));

    QInputMethodEvent commitEvent;
    commitEvent.setCommitString(QStringLiteral("한"));
    QCoreApplication::sendEvent(inputItem, &commitEvent);
    CTEST_WAIT_DEFAULT(field->property("text").toString().contains(QStringLiteral("한")));

    inputItem->setProperty("text", QStringLiteral("alpha beta gamma"));
    inputItem->setProperty("cursorPosition", 0);
    CTEST_CHECK(QMetaObject::invokeMethod(field, "deselect"));
    mouseDoubleClick(window, Qt::LeftButton, Qt::NoModifier, wordPoint, 10);
    CTEST_WAIT_DEFAULT(inputItem->property("selectedText").toString().contains(QStringLiteral("alpha")));
    const QString doubleClickSelection = inputItem->property("selectedText").toString();
    CTEST_CHECK(!doubleClickSelection.isEmpty());

    mouseClick(window, Qt::LeftButton, Qt::NoModifier, wordPoint, 10);
    CTEST_WAIT_DEFAULT(inputItem->property("selectedText").toString() != doubleClickSelection);

    CTEST_CHECK(QMetaObject::invokeMethod(field, "deselect"));
    mousePress(window, Qt::LeftButton, Qt::NoModifier, dragStart, 10);
    mouseMove(window, dragEnd, 10);
    mouseRelease(window, Qt::LeftButton, Qt::NoModifier, dragEnd, 10);
    CTEST_WAIT_DEFAULT(inputItem->property("selectedText").toString().size() > 0);
}

void ImportApiTests::input_field_search_icon_mobile_scaling_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    Component.onCompleted: LV.Theme.targetOverride = "ios"
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.InputField {
        id: searchField
        width: implicitWidth
        height: implicitHeight
        visible: false
        search: true
        placeholderText: "Search"
    }

    property bool mobileSearchContractReady:
        LV.Theme.mobileTarget
        && searchField.search
        && searchField.searchIconVisible
        && searchField.implicitWidth === 206
        && searchField.implicitHeight === 22
        && searchField.width === 206
        && searchField.height === 22
        && searchField.centeredTextHeight === 13
        && searchField.inputItem.font.pixelSize === 13
        && searchField.searchIconSize === 12
        && searchField.clearIconSize === 12
        && searchField.searchIconSource == LV.Theme.iconPath("inputFieldSearch")
        && searchField.searchIconSourceSize === Math.ceil(searchField.searchIconSize * searchField.searchIconRasterScale)
        && LV.Theme.iconSm === 18
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("mobileSearchContractReady").toBool());
}

void ImportApiTests::toggle_switch_figma_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property string themeTarget: "macos"
    readonly property real expectedScale: 1.0

    onThemeTargetChanged: LV.Theme.targetOverride = themeTarget
    Component.onCompleted: LV.Theme.targetOverride = themeTarget
    Component.onDestruction: LV.Theme.targetOverride = ""

    LV.ToggleSwitch {
        id: onSwitch
        objectName: "onSwitch"
        checked: true
        enabled: true
        visible: true
        x: 20
        y: 22
        width: implicitWidth
        height: implicitHeight
        transitionDuration: 0
    }

    LV.ToggleSwitch {
        id: offSwitch
        objectName: "offSwitch"
        checked: false
        enabled: true
        visible: true
        x: 94
        y: 22
        width: implicitWidth
        height: implicitHeight
        transitionDuration: 0
    }

    LV.ToggleSwitch {
        id: labelSwitch
        objectName: "labelSwitch"
        checked: true
        enabled: true
        visible: false
        text: "Label"
        width: implicitWidth
        height: implicitHeight
        transitionDuration: 0
    }

    function descendant(item, expectedObjectName) {
        if (!item)
            return null
        if (item.objectName === expectedObjectName)
            return item
        if (!item.children)
            return null
        for (let index = 0; index < item.children.length; ++index) {
            const match = descendant(item.children[index], expectedObjectName)
            if (match)
                return match
        }
        return null
    }

    readonly property var onIndicator: descendant(onSwitch, "onSwitch_indicator")
    readonly property var offIndicator: descendant(offSwitch, "offSwitch_indicator")
    readonly property var onTrack: descendant(onSwitch, "onSwitch_track")
    readonly property var offTrack: descendant(offSwitch, "offSwitch_track")
    readonly property var onKnob: descendant(onSwitch, "onSwitch_knob")
    readonly property var offKnob: descendant(offSwitch, "offSwitch_knob")

    property bool figmaToggleContractReady:
        typeof onSwitch.state === "boolean"
        && onSwitch.state === onSwitch.checked
        && typeof offSwitch.state === "boolean"
        && offSwitch.state === offSwitch.checked
        && onSwitch.onColor === LV.Theme.accent
        && onSwitch.offColor === LV.Theme.panelBackground12
        && onSwitch.knobFillColor === LV.Theme.titleHeaderColor
        && onSwitch.trackShadowColor === LV.Theme.shadowStrong
        && onSwitch.trackWidth === 38 * expectedScale
        && onSwitch.trackHeight === 22 * expectedScale
        && onSwitch.trackPadding === 2 * expectedScale
        && onSwitch.knobSize === 18 * expectedScale
        && onSwitch.trackCornerRadius === 20 * expectedScale
        && onSwitch.knobCornerRadius === 9 * expectedScale
        && onSwitch.trackShadowBlur === 4 * expectedScale
        && onSwitch.trackShadowVerticalOffset === 4 * expectedScale
        && onSwitch.knobXOff === 2 * expectedScale
        && onSwitch.knobXOn === 18 * expectedScale
        && offSwitch.implicitWidth === 38 * expectedScale
        && offSwitch.implicitHeight === 22 * expectedScale
        && onSwitch.implicitWidth === 38 * expectedScale
        && onSwitch.implicitHeight === 22 * expectedScale
        && labelSwitch.spacing === 8 * expectedScale
        && labelSwitch.contentItem.stylePixelSize === 13
        && labelSwitch.contentItem.styleLineHeight === 13
        && onIndicator !== null
        && offIndicator !== null
        && onTrack !== null
        && offTrack !== null
        && onKnob !== null
        && offKnob !== null
        && onIndicator.width === 38 * expectedScale
        && onIndicator.height === 22 * expectedScale
        && offIndicator.width === 38 * expectedScale
        && offIndicator.height === 22 * expectedScale
        && onTrack.radius === 20 * expectedScale
        && offTrack.radius === 20 * expectedScale
        && onTrack.color === LV.Theme.accent
        && offTrack.color === LV.Theme.panelBackground12
        && onKnob.width === 18 * expectedScale
        && onKnob.height === 18 * expectedScale
        && offKnob.width === 18 * expectedScale
        && offKnob.height === 18 * expectedScale
        && onKnob.x === 18 * expectedScale
        && offKnob.x === 2 * expectedScale

    function verifyStateAliasRoundTrip() {
        offSwitch.state = true
        if (!offSwitch.checked)
            return false
        offSwitch.checked = false
        return offSwitch.state === false
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("figmaToggleContractReady").toBool());

    QVariant stateAliasRoundTrip;
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(),
                                      "verifyStateAliasRoundTrip",
                                      Q_RETURN_ARG(QVariant, stateAliasRoundTrip)));
    CTEST_CHECK(stateAliasRoundTrip.toBool());

    QObject *desktopShadow = root->findChild<QObject *>(QStringLiteral("onSwitch_trackShadowEffect"),
                                                         Qt::FindChildrenRecursively);
    CTEST_CHECK(desktopShadow);
    CTEST_CHECK(desktopShadow->property("shadowEnabled").toBool());
    IMPORT_COMPARE(desktopShadow->property("blurMax").toInt(), 4);
    IMPORT_COMPARE(desktopShadow->property("shadowBlur").toReal(), 1.0);
    IMPORT_COMPARE(desktopShadow->property("shadowHorizontalOffset").toReal(), 0.0);
    IMPORT_COMPARE(desktopShadow->property("shadowVerticalOffset").toReal(), 4.0);
    IMPORT_COMPARE(desktopShadow->property("shadowColor").value<QColor>(), QColor(QStringLiteral("#40000000")));

    root->setProperty("themeTarget", QStringLiteral("ios"));
    CTEST_WAIT_DEFAULT(root->property("figmaToggleContractReady").toBool());

    QObject *mobileShadow = root->findChild<QObject *>(QStringLiteral("onSwitch_trackShadowEffect"),
                                                        Qt::FindChildrenRecursively);
    CTEST_CHECK(mobileShadow);
    IMPORT_COMPARE(mobileShadow->property("blurMax").toInt(), 4);
    IMPORT_COMPARE(mobileShadow->property("shadowVerticalOffset").toReal(), 4.0);

    root->setProperty("themeTarget", QStringLiteral("macos"));
    CTEST_WAIT_DEFAULT(root->property("figmaToggleContractReady").toBool());

    auto *rootItem = qobject_cast<QQuickItem *>(root.data());
    CTEST_CHECK(rootItem);
    QQuickWindow captureWindow;
    captureWindow.setColor(Qt::transparent);
    captureWindow.resize(152, 62);
    rootItem->setParentItem(captureWindow.contentItem());
    rootItem->setPosition(QPointF(0.0, 0.0));
    rootItem->setSize(QSizeF(152.0, 62.0));
    rootItem->setOpacity(1.0);
    rootItem->setVisible(true);
    captureWindow.show();
    QCoreApplication::processEvents();
    processFor(120);
    QCoreApplication::processEvents();

    const QSharedPointer<QQuickItemGrabResult> grabResult = rootItem->grabToImage(QSize(152, 62));
    CTEST_CHECK(grabResult);
    CTEST_WAIT_VERIFY(!grabResult->image().isNull(), 5000);
    const QImage captured = grabResult->image().convertToFormat(QImage::Format_RGBA8888);
    const qreal captureScale = static_cast<qreal>(captured.width()) / 152.0;
    CTEST_CHECK(captureScale >= 1.0);
    IMPORT_COMPARE(captured.height(), qRound(62.0 * captureScale));

    const auto logicalPixel = [&captured, captureScale](int x, int y) {
        return captured.pixelColor(qRound(x * captureScale), qRound(y * captureScale));
    };

    const QString capturePath = qEnvironmentVariable("LVRS_TOGGLE_CAPTURE_PATH");
    if (!capturePath.isEmpty())
        CTEST_CHECK_MESSAGE(captured.save(capturePath), qPrintable(capturePath));

    const auto opaqueRgbMatches = [](const QColor &actual, const QColor &expected) {
        return qAbs(actual.red() - expected.red()) <= 2
            && qAbs(actual.green() - expected.green()) <= 2
            && qAbs(actual.blue() - expected.blue()) <= 2
            && actual.alpha() >= 253;
    };
    const QColor capturedOnTrack = logicalPixel(28, 33);
    const QColor capturedOffTrack = logicalPixel(124, 33);
    CTEST_CHECK_MESSAGE(opaqueRgbMatches(capturedOnTrack, AccentFixture::color("accentBlue")),
             qPrintable(capturedOnTrack.name(QColor::HexArgb)));
    CTEST_CHECK_MESSAGE(opaqueRgbMatches(capturedOffTrack, QColor(QStringLiteral("#313233"))),
             qPrintable(capturedOffTrack.name(QColor::HexArgb)));
    const QColor capturedOnKnob = logicalPixel(47, 33);
    const QColor capturedOffKnob = logicalPixel(105, 33);
    CTEST_CHECK_MESSAGE(capturedOnKnob.lightness() > 220,
             qPrintable(capturedOnKnob.name(QColor::HexArgb)));
    CTEST_CHECK_MESSAGE(capturedOffKnob.lightness() > 200,
             qPrintable(capturedOffKnob.name(QColor::HexArgb)));
    int shadowOverflowAlpha = 0;
    for (int logicalY = 45; logicalY <= 52; ++logicalY)
        shadowOverflowAlpha = qMax(shadowOverflowAlpha, logicalPixel(39, logicalY).alpha());
    if (QGuiApplication::platformName() != QStringLiteral("offscreen"))
        CTEST_CHECK(shadowOverflowAlpha > 0);

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
    width: 120
    height: 100

    LV.CheckBox {
        id: checkedEnabled
        objectName: "checkedEnabled"
        text: "Label"
        checked: true
        enabled: true
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }
    LV.CheckBox {
        id: checkedDisabled
        objectName: "checkedDisabled"
        y: 24
        text: "Label"
        checked: true
        enabled: false
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }
    LV.CheckBox {
        id: uncheckedEnabled
        objectName: "uncheckedEnabled"
        y: 48
        text: "Label"
        checked: false
        enabled: true
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }
    LV.CheckBox {
        id: uncheckedDisabled
        objectName: "uncheckedDisabled"
        y: 72
        text: "Label"
        checked: false
        enabled: false
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }

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
        checkedEnabled.boxSize === LV.Theme.scaleMetric(17)
        && Math.abs(checkedEnabled.framePadding - LV.Theme.scaleRealMetric(0.5)) < 0.01
        && Math.abs(checkedEnabled.boxRadius - (checkedEnabled.boxSize * (3.5 / 17.0))) < 0.01
        && Math.abs(checkedEnabled.checkMarkStrokeWidth - (checkedEnabled.boxSize * (2.0 / 17.0))) < 0.01
        && checkedEnabled.contentItem.spacing === LV.Theme.gap6
        && checkedEnabled.implicitWidth === 57
        && checkedEnabled.implicitHeight === 18
        && checkedEnabled.width === 57
        && checkedEnabled.height === 18
        && checkedEnabled.useFigmaCheckedAssets
        && checkedEnabled.usingFigmaCheckedAsset
        && checkedDisabled.usingFigmaCheckedAsset
        && !uncheckedEnabled.usingFigmaCheckedAsset
        && !uncheckedDisabled.usingFigmaCheckedAsset
        && checkedEnabled.checkedAssetSourceEnabled.toString() === LV.Theme.iconPath("checkboxCheckedEnabled").toString()
        && checkedEnabled.checkedAssetSourceDisabled.toString() === LV.Theme.iconPath("checkboxCheckedDisabled").toString()
        && checkedEnabled.resolvedCheckedAssetSource.toString() === checkedEnabled.checkedAssetSourceEnabled.toString()
        && checkedDisabled.resolvedCheckedAssetSource.toString() === checkedDisabled.checkedAssetSourceDisabled.toString()
        && checkedEnabled.checkColor === LV.Theme.bodyColor
        && checkedEnabled.checkMarkColorDisabled === LV.Theme.disabledColor
        && checkedEnabledIndicator !== null
        && checkedDisabledIndicator !== null
        && uncheckedEnabledIndicator !== null
        && uncheckedDisabledIndicator !== null
        && checkedEnabledIndicator.color.toString() === LV.Theme.accent.toString()
        && checkedDisabledIndicator.color.toString() === LV.Theme.panelBackground12.toString()
        && uncheckedEnabledIndicator.color.toString() === LV.Theme.bodyColor.toString()
        && uncheckedDisabledIndicator.color.toString() === LV.Theme.panelBackground12.toString()
        && Math.abs(checkedEnabledIndicator.border.width - 0) < 0.01
        && Math.abs(checkedDisabledIndicator.border.width - (checkedDisabled.boxSize * (0.5 / 17.0))) < 0.01
        && Math.abs(uncheckedEnabledIndicator.border.width - (uncheckedEnabled.boxSize * (0.5 / 17.0))) < 0.01
        && Math.abs(uncheckedDisabledIndicator.border.width - 0) < 0.01
        && checkedDisabledIndicator.border.color.toString() === LV.Theme.panelBackground12.toString()
        && uncheckedEnabledIndicator.border.color.toString() === LV.Theme.bodyColor.toString()
        && checkedEnabled.showInnerShadow === false
        && checkedDisabled.showInnerShadow === true
        && uncheckedEnabled.showInnerShadow === true
        && uncheckedDisabled.showInnerShadow === true
        && checkedEnabledLabel !== null
        && checkedDisabledLabel !== null
        && checkedEnabledLabel.color.toString() === LV.Theme.bodyColor.toString()
        && checkedDisabledLabel.color.toString() === LV.Theme.disabledColor.toString()
        && checkedEnabledLabel.font.pixelSize === LV.Theme.textBody
        && checkedEnabledLabel.font.weight === LV.Theme.textBodyWeight
        && checkedEnabledLabel.styleLineHeight === LV.Theme.textBodyLineHeight

    property string figmaCheckBoxDiagnostics: JSON.stringify({
        boxSize: checkedEnabled.boxSize,
        framePadding: checkedEnabled.framePadding,
        radius: checkedEnabled.boxRadius,
        spacing: checkedEnabled.contentItem.spacing,
        implicitSize: [checkedEnabled.implicitWidth, checkedEnabled.implicitHeight],
        actualSize: [checkedEnabled.width, checkedEnabled.height],
        indicator: checkedEnabledIndicator !== null
            ? [checkedEnabledIndicator.x, checkedEnabledIndicator.y,
                checkedEnabledIndicator.width, checkedEnabledIndicator.height]
            : null,
        label: checkedEnabledLabel !== null
            ? [checkedEnabledLabel.x, checkedEnabledLabel.y,
                checkedEnabledLabel.width, checkedEnabledLabel.height,
                checkedEnabledLabel.implicitWidth, checkedEnabledLabel.implicitHeight]
            : null,
        enabledSource: checkedEnabled.checkedAssetSourceEnabled.toString(),
        resolvedSource: checkedEnabled.resolvedCheckedAssetSource.toString()
    })
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("figmaCheckBoxReady").toBool(),
                 qPrintable(root->property("figmaCheckBoxDiagnostics").toString()));

    auto *checkedEnabled = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("checkedEnabled")));
    auto *indicator = visualChildByObjectName(
        checkedEnabled, QStringLiteral("checkedEnabled_indicator"));
    auto *label = visualChildByObjectName(
        checkedEnabled, QStringLiteral("checkedEnabled_label"));
    auto *checkedAsset = visualChildByObjectName(
        checkedEnabled, QStringLiteral("checkedEnabled_checkedAsset"));
    CTEST_CHECK(checkedEnabled);
    CTEST_CHECK(indicator);
    CTEST_CHECK(label);
    CTEST_CHECK(checkedAsset);

    const auto boundsIn = [](QQuickItem *item, QQuickItem *ancestor) {
        return QRectF(item->mapToItem(ancestor, QPointF(0.0, 0.0)),
                      QSizeF(item->width(), item->height()));
    };
    const auto verifyBounds = [](const QRectF &actual, const QRectF &expected) {
        CTEST_CHECK_MESSAGE(qAbs(actual.x() - expected.x()) < 0.01
                     && qAbs(actual.y() - expected.y()) < 0.01
                     && qAbs(actual.width() - expected.width()) < 0.01
                     && qAbs(actual.height() - expected.height()) < 0.01,
                 qPrintable(QStringLiteral("bounds actual=(%1,%2 %3x%4) expected=(%5,%6 %7x%8)")
                                .arg(actual.x()).arg(actual.y())
                                .arg(actual.width()).arg(actual.height())
                                .arg(expected.x()).arg(expected.y())
                                .arg(expected.width()).arg(expected.height())));
    };

    verifyBounds(boundsIn(indicator, checkedEnabled), QRectF(0.5, 0.5, 17.0, 17.0));
    verifyBounds(boundsIn(label, checkedEnabled), QRectF(23.5, 2.5, 33.0, 13.0));
    verifyBounds(boundsIn(checkedAsset, checkedEnabled), QRectF(0.5, 0.5, 17.0, 17.0));
    IMPORT_WAIT_COMPARE(checkedAsset->property("status").toInt(), 1);
    CTEST_CHECK(checkedAsset->property("source").toUrl().toString().endsWith(
        QStringLiteral("checkboxCheckedEnabled.svg")));
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
    width: 120
    height: 130

    LV.RadioButton {
        id: onEnabled
        objectName: "onEnabled"
        checked: true
        enabled: true
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }
    LV.RadioButton {
        id: onDisabled
        objectName: "onDisabled"
        y: 24
        checked: true
        enabled: false
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }
    LV.RadioButton {
        id: offEnabled
        objectName: "offEnabled"
        y: 48
        checked: false
        enabled: true
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }
    LV.RadioButton {
        id: offDisabled
        objectName: "offDisabled"
        y: 72
        checked: false
        enabled: false
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }
    LV.RadioButton {
        id: labeled
        objectName: "labeled"
        y: 96
        text: "Label"
        checked: true
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }

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
    readonly property var offEnabledDot: dotOf(offEnabled)
    readonly property var offDisabledDot: dotOf(offDisabled)
    readonly property var labeledIndicator: indicatorOf(labeled)
    readonly property var labeledDot: dotOf(labeled)
    readonly property var labeledLabel: labeled.contentItem.children.length > 1
        ? labeled.contentItem.children[1]
        : null

    property bool figmaRadioReady:
        onEnabled.indicatorSize === LV.Theme.controlIndicatorSize
        && onEnabled.dotSize === LV.Theme.gap8
        && onEnabled.indicatorSize === 18
        && onEnabled.dotSize === 8
        && onEnabled.indicatorRadius === 9
        && onEnabled.dotRadius === 4
        && onEnabled.implicitWidth === 18
        && onEnabled.implicitHeight === 18
        && onEnabled.width === 18
        && onEnabled.height === 18
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
        && offEnabledDot !== null
        && offDisabledDot !== null
        && onEnabledIndicator.color === LV.Theme.accent
        && onDisabledIndicator.color === LV.Theme.panelBackground12
        && offEnabledIndicator.color === LV.Theme.textPrimary
        && offDisabledIndicator.color === LV.Theme.panelBackground12
        && onEnabledDot.color === LV.Theme.textPrimary
        && onDisabledDot.color === LV.Theme.textSeptenary
        && onEnabledDot.visible
        && onDisabledDot.visible
        && !offEnabledDot.visible
        && !offDisabledDot.visible
        && labeledIndicator !== null
        && labeledDot !== null
        && labeledLabel !== null
        && labeled.contentItem.spacing === LV.Theme.gap8
        && labeled.implicitWidth === 59
        && labeled.implicitHeight === 18
        && labeledLabel.stylePixelSize === 13
        && labeledLabel.styleLineHeight === 13
        && labeledLabel.font.pixelSize === 13

    property string figmaRadioDiagnostics: JSON.stringify({
        indicatorSize: onEnabled.indicatorSize,
        dotSize: onEnabled.dotSize,
        indicatorRadius: onEnabled.indicatorRadius,
        dotRadius: onEnabled.dotRadius,
        implicitSize: [onEnabled.implicitWidth, onEnabled.implicitHeight],
        labeledImplicitSize: [labeled.implicitWidth, labeled.implicitHeight],
        indicator: onEnabledIndicator !== null
            ? [onEnabledIndicator.x, onEnabledIndicator.y,
                onEnabledIndicator.width, onEnabledIndicator.height]
            : null,
        dot: onEnabledDot !== null
            ? [onEnabledDot.x, onEnabledDot.y,
                onEnabledDot.width, onEnabledDot.height]
            : null,
        label: labeledLabel !== null
            ? [labeledLabel.x, labeledLabel.y,
                labeledLabel.width, labeledLabel.height]
            : null
    })
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("figmaRadioReady").toBool(),
                 qPrintable(root->property("figmaRadioDiagnostics").toString()));

    auto *onEnabled = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("onEnabled")));
    auto *labeled = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("labeled")));
    CTEST_CHECK(onEnabled);
    CTEST_CHECK(labeled);

    auto *indicator = visualChildByObjectName(
        onEnabled, QStringLiteral("onEnabled_indicator"));
    auto *dot = visualChildByObjectName(
        onEnabled, QStringLiteral("onEnabled_dot"));
    auto *label = visualChildByObjectName(
        labeled, QStringLiteral("labeled_label"));
    CTEST_CHECK(indicator);
    CTEST_CHECK(dot);
    CTEST_CHECK(label);

    const auto boundsIn = [](QQuickItem *item, QQuickItem *ancestor) {
        return QRectF(item->mapToItem(ancestor, QPointF(0.0, 0.0)),
                      QSizeF(item->width(), item->height()));
    };
    const auto verifyBounds = [](const QRectF &actual, const QRectF &expected) {
        CTEST_CHECK_MESSAGE(qAbs(actual.x() - expected.x()) < 0.01
                     && qAbs(actual.y() - expected.y()) < 0.01
                     && qAbs(actual.width() - expected.width()) < 0.01
                     && qAbs(actual.height() - expected.height()) < 0.01,
                 qPrintable(QStringLiteral("bounds actual=(%1,%2 %3x%4) expected=(%5,%6 %7x%8)")
                                .arg(actual.x()).arg(actual.y())
                                .arg(actual.width()).arg(actual.height())
                                .arg(expected.x()).arg(expected.y())
                                .arg(expected.width()).arg(expected.height())));
    };

    verifyBounds(boundsIn(indicator, onEnabled), QRectF(0.0, 0.0, 18.0, 18.0));
    verifyBounds(boundsIn(dot, onEnabled), QRectF(5.0, 5.0, 8.0, 8.0));
    verifyBounds(boundsIn(label, labeled), QRectF(26.0, 2.5, 33.0, 13.0));
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("contractReady").toBool());
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("contractReady").toBool());
}

void ImportApiTests::alert_figma_variant_contract_loads()
{
    QQuickWindow window;
    window.resize(960, 720);
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 960
    height: 720

    property int expectedWidth: LV.Theme.scaleMetric(500)
    property int expectedIconSize: LV.Theme.scaleMetric(64)
    property int expectedTopPadding: LV.Theme.scaleMetric(46)
    property int expectedSidePadding: LV.Theme.gap24
    property int expectedSectionSpacing: LV.Theme.scaleMetric(28)
    property int expectedActionSpacing: LV.Theme.gap14
    property int expectedCardRadius: LV.Theme.scaleMetric(36)
    property real expectedButtonVerticalPadding: 0
    property real expectedButtonHeight: LV.Theme.scaleMetric(56)
    property color expectedCardColor: Qt.rgba(29 / 255, 31 / 255, 33 / 255, 0.72)
    property color expectedTitleColor: "#F4F5F7"
    property color expectedTextColor: "#D6D9DF"

    LV.Alert {
        id: twoActionAlert
        objectName: "twoActionAlert"
        width: root.width
        height: root.height
        useOverlayLayer: false
        open: true
        buttonCount: 2
        title: "Apply changes?"
        message: "Your new settings will be applied\nto this workspace."
        primaryText: "TwoPrimary"
        secondaryText: "TwoSecondary"
    }

    LV.Alert {
        id: threeActionAlert
        objectName: "threeActionAlert"
        width: root.width
        height: root.height
        useOverlayLayer: false
        open: true
        buttonCount: 3
        title: "Save changes?"
        message: "You have unsaved changes.\nSave them before closing?"
        primaryText: "ThreePrimary"
        secondaryText: "ThreeSecondary"
        tertiaryText: "ThreeTertiary"
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);

    auto *host = qobject_cast<QQuickItem *>(root.data());
    CTEST_CHECK(host);
    host->setParentItem(window.contentItem());
    window.show();

    QObject *twoActionAlert = root->findChild<QObject *>(QStringLiteral("twoActionAlert"));
    QObject *threeActionAlert = root->findChild<QObject *>(QStringLiteral("threeActionAlert"));
    CTEST_CHECK(twoActionAlert);
    CTEST_CHECK(threeActionAlert);

    auto findItem = [](QObject *alert, const QString &objectName) -> QQuickItem * {
        return qobject_cast<QQuickItem *>(
            alert->findChild<QObject *>(objectName, Qt::FindChildrenRecursively));
    };

    QQuickItem *twoCard = findItem(twoActionAlert, QStringLiteral("alertCard"));
    QQuickItem *threeCard = findItem(threeActionAlert, QStringLiteral("alertCard"));
    QQuickItem *twoContent = findItem(twoActionAlert, QStringLiteral("alertContentColumn"));
    QQuickItem *threeContent = findItem(threeActionAlert, QStringLiteral("alertContentColumn"));
    QQuickItem *twoIcon = findItem(twoActionAlert, QStringLiteral("alertAppIcon"));
    QQuickItem *threeIcon = findItem(threeActionAlert, QStringLiteral("alertAppIcon"));
    QQuickItem *twoTitle = findItem(twoActionAlert, QStringLiteral("alertTitle"));
    QQuickItem *threeTitle = findItem(threeActionAlert, QStringLiteral("alertTitle"));
    QQuickItem *twoMessage = findItem(twoActionAlert, QStringLiteral("alertMessage"));
    QQuickItem *threeMessage = findItem(threeActionAlert, QStringLiteral("alertMessage"));
    QQuickItem *twoHorizontalActions = findItem(twoActionAlert, QStringLiteral("alertHorizontalActions"));
    QQuickItem *twoVerticalActions = findItem(twoActionAlert, QStringLiteral("alertVerticalActions"));
    QQuickItem *threeHorizontalActions = findItem(threeActionAlert, QStringLiteral("alertHorizontalActions"));
    QQuickItem *threeVerticalActions = findItem(threeActionAlert, QStringLiteral("alertVerticalActions"));
    QQuickItem *twoPrimary = findItem(twoActionAlert, QStringLiteral("alertPrimaryHorizontal"));
    QQuickItem *twoSecondary = findItem(twoActionAlert, QStringLiteral("alertSecondaryHorizontal"));
    QQuickItem *threePrimary = findItem(threeActionAlert, QStringLiteral("alertPrimaryVertical"));
    QQuickItem *threeSecondary = findItem(threeActionAlert, QStringLiteral("alertSecondaryVertical"));
    QQuickItem *threeTertiary = findItem(threeActionAlert, QStringLiteral("alertTertiaryVertical"));

    CTEST_CHECK(twoCard);
    CTEST_CHECK(threeCard);
    CTEST_CHECK(twoContent);
    CTEST_CHECK(threeContent);
    CTEST_CHECK(twoIcon);
    CTEST_CHECK(threeIcon);
    CTEST_CHECK(twoTitle);
    CTEST_CHECK(threeTitle);
    CTEST_CHECK(twoMessage);
    CTEST_CHECK(threeMessage);
    CTEST_CHECK(twoHorizontalActions);
    CTEST_CHECK(twoVerticalActions);
    CTEST_CHECK(threeHorizontalActions);
    CTEST_CHECK(threeVerticalActions);
    CTEST_CHECK(twoPrimary);
    CTEST_CHECK(twoSecondary);
    CTEST_CHECK(threePrimary);
    CTEST_CHECK(threeSecondary);
    CTEST_CHECK(threeTertiary);

    const qreal expectedWidth = root->property("expectedWidth").toReal();
    const qreal expectedIconSize = root->property("expectedIconSize").toReal();
    const qreal expectedTopPadding = root->property("expectedTopPadding").toReal();
    const qreal expectedSidePadding = root->property("expectedSidePadding").toReal();
    const qreal expectedSectionSpacing = root->property("expectedSectionSpacing").toReal();
    const qreal expectedActionSpacing = root->property("expectedActionSpacing").toReal();
    const qreal expectedButtonVerticalPadding = root->property("expectedButtonVerticalPadding").toReal();
    const qreal expectedButtonHeight = root->property("expectedButtonHeight").toReal();
    const QColor expectedCardColor = root->property("expectedCardColor").value<QColor>();
    const QColor expectedTextColor = root->property("expectedTextColor").value<QColor>();

    for (QQuickItem *card : {twoCard, threeCard}) {
        CTEST_CHECK(qAbs(card->width() - expectedWidth) < 0.01);
        CTEST_CHECK(qAbs(card->property("radius").toReal()
                     - root->property("expectedCardRadius").toReal()) < 0.01);
        IMPORT_COMPARE(card->property("color").value<QColor>(), expectedCardColor);
        CTEST_CHECK(card->property("clip").toBool());
    }

    for (QQuickItem *content : {twoContent, threeContent}) {
        CTEST_CHECK(qAbs(content->property("topPadding").toReal() - expectedTopPadding) < 0.01);
        CTEST_CHECK(qAbs(content->property("spacing").toReal() - expectedSectionSpacing) < 0.01);
    }

    for (QQuickItem *icon : {twoIcon, threeIcon}) {
        const qreal size = icon == twoIcon ? expectedIconSize : 56;
        CTEST_CHECK(qAbs(icon->width() - size) < 0.01);
        CTEST_CHECK(qAbs(icon->height() - size) < 0.01);
        CTEST_CHECK(icon->property("source").toUrl().toString().endsWith(icon == twoIcon
            ? QStringLiteral("/resources/images/alert-adjustments.svg")
            : QStringLiteral("/resources/images/alert-file-text.svg")));
        IMPORT_WAIT_COMPARE(icon->property("status").toInt(), 1);
    }

    for (QQuickItem *label : {twoTitle, threeTitle}) {
        IMPORT_COMPARE(label->property("color").value<QColor>(),
                 root->property("expectedTitleColor").value<QColor>());
        IMPORT_COMPARE(label->property("stylePixelSize").toInt(), 26);
        IMPORT_COMPARE(label->property("font").value<QFont>().pixelSize(), 26);
        IMPORT_COMPARE(label->property("lineHeight").toReal(), qreal(26));
    }
    for (QQuickItem *label : {twoMessage, threeMessage}) {
        IMPORT_COMPARE(label->property("color").value<QColor>(), expectedTextColor);
        IMPORT_COMPARE(label->property("stylePixelSize").toInt(), 13);
        IMPORT_COMPARE(label->property("font").value<QFont>().pixelSize(), 13);
        IMPORT_COMPARE(label->property("lineHeight").toReal(), qreal(13));
    }

    CTEST_CHECK(twoHorizontalActions->isVisible());
    CTEST_CHECK(!twoVerticalActions->isVisible());
    CTEST_CHECK(!threeHorizontalActions->isVisible());
    CTEST_CHECK(threeVerticalActions->isVisible());
    CTEST_CHECK(qAbs(twoHorizontalActions->property("spacing").toReal() - expectedActionSpacing) < 0.01);
    IMPORT_COMPARE(threeVerticalActions->property("spacing").toReal(), qreal(12));
    CTEST_CHECK(twoSecondary->x() < twoPrimary->x());
    CTEST_CHECK(qAbs(twoHorizontalActions->x() - expectedSidePadding) < 0.01);
    CTEST_CHECK(qAbs(threeVerticalActions->x() - expectedSidePadding) < 0.01);

    const qreal expectedHorizontalButtonWidth = (expectedWidth - (expectedSidePadding * 2)
                                                 - expectedActionSpacing) / 2;
    for (QQuickItem *button : {twoPrimary, twoSecondary}) {
        CTEST_CHECK(qAbs(button->width() - expectedHorizontalButtonWidth) < 0.01);
        CTEST_CHECK(qAbs(button->height() - expectedButtonHeight) < 0.01);
        IMPORT_COMPARE(button->property("cornerRadius").toReal(), qreal(16));
        CTEST_CHECK(qAbs(button->property("verticalPadding").toReal()
                     - expectedButtonVerticalPadding) < 0.01);
    }

    const qreal expectedVerticalButtonWidth = expectedWidth - (expectedSidePadding * 2);
    for (QQuickItem *button : {threePrimary, threeSecondary, threeTertiary}) {
        CTEST_CHECK(qAbs(button->width() - expectedVerticalButtonWidth) < 0.01);
        const qreal expectedHeight = button == threeTertiary ? 44 : expectedButtonHeight;
        CTEST_CHECK(qAbs(button->height() - expectedHeight) < 0.01);
        IMPORT_COMPARE(button->property("cornerRadius").toReal(), qreal(16));
        CTEST_CHECK(qAbs(button->property("verticalPadding").toReal()
                     - expectedButtonVerticalPadding) < 0.01);
    }

    // Figma 658:225 uses auto-height text with 26px Title / 13px Body lines.
    // The old cached 34px/56px text bounds must not become minimum heights.
    IMPORT_WAIT_COMPARE(twoContent->height(), qreal(262));
    IMPORT_WAIT_COMPARE(threeContent->height(), qreal(262));
    IMPORT_WAIT_COMPARE(twoTitle->parentItem()->height(), qreal(26));
    IMPORT_WAIT_COMPARE(twoMessage->parentItem()->height(), qreal(26));
    IMPORT_WAIT_COMPARE(twoCard->height(), qreal(379));
    IMPORT_WAIT_COMPARE(threeCard->height(), qreal(479));
    IMPORT_COMPARE(threeSecondary->property("textColor").value<QColor>(), AccentFixture::color("accentRed"));
    IMPORT_COMPARE(twoSecondary->property("textColor").value<QColor>(), QColor("#f4f5f7"));
    IMPORT_COMPARE(threeTertiary->property("backgroundColor").value<QColor>(), QColor(Qt::transparent));

    // Narrow hosts must remain contained even below the preferred minimum.
    CTEST_CHECK(root->setProperty("width", 448));
    IMPORT_WAIT_COMPARE(twoCard->width(), qreal(400));
    CTEST_CHECK(root->setProperty("width", 240));
    CTEST_WAIT_DEFAULT(twoCard->width() <= 192);
    CTEST_CHECK(root->setProperty("width", 960));
    IMPORT_WAIT_COMPARE(twoCard->width(), qreal(500));

    // Hidden icons release their frame and the adjacent gap; copy may grow.
    CTEST_CHECK(twoActionAlert->setProperty("showIcon", false));
    IMPORT_WAIT_COMPARE(twoCard->height(), qreal(265));
    CTEST_CHECK(twoActionAlert->setProperty("showIcon", true));
    IMPORT_WAIT_COMPARE(twoCard->height(), qreal(379));
    CTEST_CHECK(twoActionAlert->setProperty("title", QStringLiteral("First line\nSecond line\nThird line")));
    IMPORT_WAIT_COMPARE(twoCard->height(), qreal(431));

    // Changing copy must release both obsolete line height and empty-section gaps.
    CTEST_CHECK(twoActionAlert->setProperty("title", QStringLiteral("Apply changes?")));
    CTEST_CHECK(twoActionAlert->setProperty("message", QStringLiteral("Ready to apply.")));
    IMPORT_WAIT_COMPARE(twoCard->height(), qreal(366));
    CTEST_CHECK(twoActionAlert->setProperty("message", QString()));
    IMPORT_WAIT_COMPARE(twoCard->height(), qreal(339));
    CTEST_CHECK(twoActionAlert->setProperty("title", QString()));
    IMPORT_WAIT_COMPARE(twoCard->height(), qreal(285));
    CTEST_CHECK(twoActionAlert->setProperty("message", QStringLiteral("First line\nSecond line")));
    IMPORT_WAIT_COMPARE(twoCard->height(), qreal(339));
    CTEST_CHECK(twoActionAlert->setProperty("title", QStringLiteral("Apply changes?")));
    IMPORT_WAIT_COMPARE(twoCard->height(), qreal(379));

    // Wrapping remains responsive, and restoring the host releases extra lines.
    CTEST_CHECK(root->setProperty("width", 240));
    CTEST_WAIT_DEFAULT(twoTitle->property("lineCount").toInt() > 1);
    CTEST_WAIT_DEFAULT(twoCard->height() > 379);
    CTEST_CHECK(root->setProperty("width", 960));
    IMPORT_WAIT_COMPARE(twoCard->height(), qreal(379));
}

void ImportApiTests::alert_content_arguments_contract_loads()
{
    QQmlEngine engine;
    engine.addImportPath(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/.."));
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.Alert {
    width: 640
    height: 720
    open: true
    useOverlayLayer: false
    glassEnabled: false
    imageSource: "qrc:/qt/qml/LVRS/resources/images/alert-adjustments.svg"
    title: "Save this document?"
    description: "The document contains unsaved changes."
    button1Text: "Save"
    button2Text: "Discard"
    button3Text: "Cancel"
}
)";
    QScopedPointer<QObject> alert(createFromQml(engine, qml));
    CTEST_CHECK(alert);
    auto *image = alert->findChild<QQuickItem *>("alertAppIcon");
    auto *title = alert->findChild<QQuickItem *>("alertTitle");
    auto *description = alert->findChild<QQuickItem *>("alertMessage");
    CTEST_CHECK(image && title && description);
    IMPORT_COMPARE(image->property("source").toUrl(), alert->property("imageSource").toUrl());
    IMPORT_WAIT_COMPARE(image->property("status").toInt(), 1); // Image.Ready
    IMPORT_COMPARE(title->property("text").toString(), QStringLiteral("Save this document?"));
    IMPORT_COMPARE(description->property("text").toString(),
             QStringLiteral("The document contains unsaved changes."));
    IMPORT_COMPARE(alert->property("resolvedButtonCount").toInt(), 3);

    const QList<QByteArray> legacyTexts = {"primaryText", "secondaryText", "tertiaryText"};
    const QStringList buttonNames = {"alertPrimaryVertical", "alertSecondaryVertical",
                                     "alertTertiaryVertical"};
    for (int index = 0; index < 3; ++index) {
        const QByteArray textProperty = "button" + QByteArray::number(index + 1) + "Text";
        auto *button = alert->findChild<QQuickItem *>(buttonNames.at(index));
        CTEST_CHECK(button);
        IMPORT_COMPARE(button->property("text"), alert->property(textProperty.constData()));
        IMPORT_COMPARE(alert->property(legacyTexts.at(index).constData()),
                 alert->property(textProperty.constData()));
        const QString updatedText = QStringLiteral("Updated action %1").arg(index + 1);
        CTEST_CHECK(alert->setProperty(textProperty.constData(), updatedText));
        IMPORT_COMPARE(button->property("text").toString(), updatedText);
    }

    const QUrl replacementIcon("qrc:/qt/qml/LVRS/resources/images/alert-file-text.svg");
    CTEST_CHECK(alert->setProperty("appIconSource", replacementIcon));
    IMPORT_COMPARE(alert->property("imageSource").toUrl(), replacementIcon);
    IMPORT_COMPARE(image->property("source").toUrl(), replacementIcon);
    IMPORT_WAIT_COMPARE(image->property("status").toInt(), 1);
    CTEST_CHECK(alert->setProperty("message", QStringLiteral("Updated through the legacy API.")));
    IMPORT_COMPARE(alert->property("description").toString(),
             QStringLiteral("Updated through the legacy API."));
    IMPORT_COMPARE(description->property("text"), alert->property("description"));
    CTEST_CHECK(alert->setProperty("description", QStringLiteral("Updated description.")));
    IMPORT_COMPARE(alert->property("message"), alert->property("description"));
    IMPORT_COMPARE(description->property("text"), alert->property("description"));
    CTEST_CHECK(alert->setProperty("button3Text", QString()));
    IMPORT_COMPARE(alert->property("resolvedButtonCount").toInt(), 2);
    CTEST_CHECK(alert->setProperty("secondaryText", QString()));
    IMPORT_COMPARE(alert->property("button2Text").toString(), QString());
    IMPORT_COMPARE(alert->property("resolvedButtonCount").toInt(), 1);
}

void ImportApiTests::alert_button_methods_are_invoked(int buttonCount, int buttonIndex, const QString &buttonName)
{
    QQmlEngine engine;
    engine.addImportPath(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/.."));
    engine.rootContext()->setContextProperty("testButtonCount", buttonCount);
    const QByteArray qml = R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV

Controls.ApplicationWindow {
    id: window
    width: 640
    height: 720
    visible: true
    property int methodCalls: 0
    property int signalCalls: 0
    property int replacementCalls: 0
    property int lastIndex: 0
    property var lastSource: null
    property string lastTrigger: ""
    property bool lastEnabled: false

    function record(index, eventData) {
        methodCalls++
        lastIndex = index
        lastSource = eventData.source
        lastTrigger = eventData.trigger
        lastEnabled = eventData.enabled && eventData.effectiveEnabled
    }

    function replaceMethod(index) {
        alert["button" + index + "Method"] = function(eventData) {
            window.replacementCalls++
            window.record(index, eventData)
        }
    }

    LV.Alert {
        id: alert
        objectName: "argumentAlert"
        open: true
        glassEnabled: false
        buttonCount: testButtonCount === 1 ? 0 : testButtonCount
        button1Text: "Save"
        button2Text: testButtonCount >= 2 ? "Discard" : ""
        button3Text: testButtonCount >= 3 ? "Cancel" : ""
        button1Method: function(eventData) { window.record(1, eventData) }
        button2Method: ({ invoke: function(eventData) { window.record(2, eventData) } })
        button3Method: ({ trigger: function(eventData) { window.record(3, eventData) } })
        onPrimaryClicked: window.signalCalls++
        onSecondaryClicked: window.signalCalls++
        onTertiaryClicked: window.signalCalls++
    }
}
)";
    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    auto *alert = root->findChild<QQuickItem *>("argumentAlert");
    CTEST_CHECK(window && alert);
    auto *button = alert->findChild<QQuickItem *>(buttonName);
    CTEST_CHECK(button);
    CTEST_CHECK(waitForWindowExposed(window));
    CTEST_WAIT_DEFAULT(button->isVisible());
    IMPORT_COMPARE(alert->property("resolvedButtonCount").toInt(), buttonCount);
    auto clickButton = [&]() {
        mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                         scenePoint(button, QPointF(button->width() / 2, button->height() / 2)));
    };
    clickButton();
    IMPORT_COMPARE(root->property("methodCalls").toInt(), 1);
    IMPORT_COMPARE(root->property("signalCalls").toInt(), 1);
    IMPORT_COMPARE(root->property("lastIndex").toInt(), buttonIndex);
    IMPORT_COMPARE(root->property("lastSource").value<QObject *>(), button);
    IMPORT_COMPARE(root->property("lastTrigger").toString(), QStringLiteral("clicked"));
    CTEST_CHECK(root->property("lastEnabled").toBool());
    CTEST_CHECK(alert->property("open").toBool());

    const QList<QByteArray> enabledProperties = {"primaryEnabled", "secondaryEnabled", "tertiaryEnabled"};
    const QByteArray enabledProperty = enabledProperties.at(buttonIndex - 1);
    CTEST_CHECK(alert->setProperty(enabledProperty.constData(), false));
    clickButton();
    IMPORT_COMPARE(root->property("methodCalls").toInt(), 1);
    IMPORT_COMPARE(root->property("signalCalls").toInt(), 1);

    CTEST_CHECK(alert->setProperty(enabledProperty.constData(), true));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "replaceMethod", Q_ARG(QVariant, buttonIndex)));
    clickButton();
    IMPORT_COMPARE(root->property("methodCalls").toInt(), 2);
    IMPORT_COMPARE(root->property("replacementCalls").toInt(), 1);
    IMPORT_COMPARE(root->property("signalCalls").toInt(), 2);
    IMPORT_COMPARE(root->property("lastIndex").toInt(), buttonIndex);
}

void ImportApiTests::alert_action_button_padding_scopes_to_alert()
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
    height: 720

    property real expectedAlertVerticalPadding: 0
    property int expectedDefaultVerticalPadding: LV.Theme.gap4
    property real expectedAlertButtonHeight: LV.Theme.scaleMetric(56)
    property int expectedDefaultButtonHeight: LV.Theme.gap20

    LV.Alert {
        id: alert
        width: root.width
        height: root.height
        open: true
        buttonCount: 3
        title: "Delete item?"
        message: "This action cannot be undone."
        primaryText: "AlertPrimary"
        secondaryText: "AlertSecondary"
        tertiaryText: "AlertTertiary"
    }

    LV.Modal {
        id: modal
        width: root.width
        height: root.height
        open: true
        buttonCount: 3
        title: "Continue?"
        description: "Modal actions should keep their compact baseline."
        primaryText: "ModalPrimary"
        secondaryText: "ModalSecondary"
        tertiaryText: "ModalTertiary"
    }

    LV.AlertButton {
        id: standaloneAlertButton
        visible: false
        text: "StandaloneAlertButton"
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);

    const auto findButtonByText = [&](const QString &text) -> QObject * {
        const auto candidates = root->findChildren<QObject *>();
        for (QObject *candidate : candidates) {
            const QVariant textProperty = candidate->property("text");
            const QVariant verticalPaddingProperty = candidate->property("verticalPadding");
            if (!textProperty.isValid() || !verticalPaddingProperty.isValid())
                continue;
            if (textProperty.toString() == text)
                return candidate;
        }
        return nullptr;
    };

    QObject *alertPrimary = findButtonByText("AlertPrimary");
    QObject *alertSecondary = findButtonByText("AlertSecondary");
    QObject *alertTertiary = findButtonByText("AlertTertiary");
    QObject *modalPrimary = findButtonByText("ModalPrimary");
    QObject *modalSecondary = findButtonByText("ModalSecondary");
    QObject *modalTertiary = findButtonByText("ModalTertiary");
    QObject *standaloneAlertButton = findButtonByText("StandaloneAlertButton");

    CTEST_CHECK(alertPrimary);
    CTEST_CHECK(alertSecondary);
    CTEST_CHECK(alertTertiary);
    CTEST_CHECK(modalPrimary);
    CTEST_CHECK(modalSecondary);
    CTEST_CHECK(modalTertiary);
    CTEST_CHECK(standaloneAlertButton);

    const double expectedAlertVerticalPadding = root->property("expectedAlertVerticalPadding").toDouble();
    const int expectedDefaultVerticalPadding = root->property("expectedDefaultVerticalPadding").toInt();
    const double expectedAlertButtonHeight = root->property("expectedAlertButtonHeight").toDouble();
    const double expectedDefaultButtonHeight = root->property("expectedDefaultButtonHeight").toDouble();

    CTEST_CHECK(qAbs(alertPrimary->property("verticalPadding").toDouble() - expectedAlertVerticalPadding) < 0.01);
    CTEST_CHECK(qAbs(alertSecondary->property("verticalPadding").toDouble() - expectedAlertVerticalPadding) < 0.01);
    CTEST_CHECK(qAbs(alertTertiary->property("verticalPadding").toDouble() - expectedAlertVerticalPadding) < 0.01);
    CTEST_CHECK(qAbs(alertPrimary->property("height").toDouble() - expectedAlertButtonHeight) < 0.01);
    CTEST_CHECK(qAbs(alertSecondary->property("height").toDouble() - expectedAlertButtonHeight) < 0.01);
    IMPORT_COMPARE(alertTertiary->property("height").toDouble(), 44.0);

    IMPORT_COMPARE(modalPrimary->property("verticalPadding").toInt(), expectedDefaultVerticalPadding);
    IMPORT_COMPARE(modalSecondary->property("verticalPadding").toInt(), expectedDefaultVerticalPadding);
    IMPORT_COMPARE(modalTertiary->property("verticalPadding").toInt(), expectedDefaultVerticalPadding);
    CTEST_CHECK(qAbs(modalPrimary->property("height").toDouble() - expectedDefaultButtonHeight) < 0.01);
    CTEST_CHECK(qAbs(modalSecondary->property("height").toDouble() - expectedDefaultButtonHeight) < 0.01);
    CTEST_CHECK(qAbs(modalTertiary->property("height").toDouble() - expectedDefaultButtonHeight) < 0.01);

    IMPORT_COMPARE(standaloneAlertButton->property("verticalPadding").toInt(), expectedDefaultVerticalPadding);
    CTEST_CHECK(qAbs(standaloneAlertButton->property("height").toDouble() - expectedDefaultButtonHeight) < 0.01);
}

void ImportApiTests::alert_glass_overlay_and_input_contract(bool frameworkWindow, const QString &dataTag)
{
    QQmlEngine engine;
    engine.addImportPath(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/.."));
    QByteArray qml = R"(
import QtQuick
import QtQuick.Controls as Controls
import LVRS as LV

Controls.ApplicationWindow {
    id: window
    width: 620
    height: 640
    visible: true
    property int backgroundClicks: 0
    Rectangle {
        anchors.fill: parent
        color: "#478DE2"
        Repeater {
            model: 104
            Rectangle {
                required property int index
                x: index * 6
                width: 3
                height: 640
                color: "#F0D189"
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: window.backgroundClicks++
        }
    }
    LV.Alert {
        objectName: "glassAlert"
        open: true
        buttonCount: 3
        title: "Save changes?"
        message: "You have unsaved changes.\nSave them before closing?"
        primaryText: "Save changes"
        secondaryText: "Discard changes"
        tertiaryText: "Cancel"
    }
}
)";
    if (frameworkWindow) {
        qml.replace("Controls.ApplicationWindow {", R"(LV.ApplicationWindow {
    desktopMinWidth: 0; desktopMinHeight: 0
    navigationEnabled: false; useInternalPageStack: false
    windowChromeInteractionsEnabled: false; windowDragHandleEnabled: false
)");
    }
    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    auto *alert = root->findChild<QQuickItem *>("glassAlert");
    CTEST_CHECK(alert);
    auto *card = alert->findChild<QQuickItem *>("alertCard");
    auto *capture = alert->findChild<QQuickItem *>("alertGlassCapture");
    auto *secondary = alert->findChild<QQuickItem *>("alertSecondaryVertical");
    auto *primary = alert->findChild<QQuickItem *>("alertPrimaryVertical");
    auto *tertiary = alert->findChild<QQuickItem *>("alertTertiaryVertical");
    CTEST_CHECK(card && capture && secondary && primary && tertiary);
    CTEST_WAIT_DEFAULT(alert->property("glassActive").toBool());
    CTEST_CHECK(!capture->property("recursive").toBool());
    CTEST_CHECK(capture->property("live").toBool());
    IMPORT_WAIT_COMPARE(card->width(), 500.0);
    IMPORT_WAIT_COMPARE(primary->height(), 56.0);
    IMPORT_COMPARE(capture->property("sourceRect").toRectF().size(), card->size());
    IMPORT_COMPARE(card->opacity(), 1.0);

    auto grab = [window]() -> QImage {
        processFor(120);
        if (window->rendererInterface()->graphicsApi() != QSGRendererInterface::Software)
            return window->grabWindow().scaled(window->size()).convertToFormat(QImage::Format_RGB32);
        const auto result = window->contentItem()->grabToImage(window->size());
        if (!result)
            return {};
        QElapsedTimer timer;
        timer.start();
        while (result->image().isNull() && timer.elapsed() < 5000)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        return result->image().scaled(window->size()).convertToFormat(QImage::Format_RGB32);
    };
    const QImage frosted = grab();
    CTEST_CHECK(!frosted.isNull());
    CTEST_CHECK(alert->setProperty("glassEnabled", false));
    const QImage unblurred = grab();
    CTEST_CHECK(!unblurred.isNull());
    const QString outputDir = qEnvironmentVariable("LVRS_ALERT_CAPTURE_DIR");
    if (!outputDir.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(outputDir));
        const auto prefix = outputDir + '/' + dataTag;
        CTEST_CHECK(frosted.save(prefix + "-alert-three-actions.png"));
        CTEST_CHECK(unblurred.save(prefix + "-alert-without-blur.png"));
    }
    const QPoint cardTopLeft = scenePoint(card, QPointF(0, 0));
    const QRect probe(cardTopLeft + QPoint(45, 25), QSize(65, 30));
    auto variation = [&probe](const QImage &image) {
        double sum = 0;
        for (int y = probe.top(); y <= probe.bottom(); ++y) {
            for (int x = probe.left(); x < probe.right(); ++x) {
                sum += qAbs(qGray(image.pixel(x, y)) - qGray(image.pixel(x + 1, y)));
            }
        }
        return sum;
    };
    CTEST_CHECK_MESSAGE(variation(unblurred) > 1000, "The backdrop fixture must contain visible fine detail.");
    qInfo() << "Backdrop detail: glass" << variation(frosted)
            << "unblurred" << variation(unblurred);
    if (window->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
        IMPORT_COMPARE(variation(frosted), variation(unblurred));
        qInfo() << "Software renderer: tint fallback verified; native RHI is required for frost.";
    } else {
        CTEST_CHECK_MESSAGE(variation(frosted) < variation(unblurred) * 0.35,
                 "Glass must blur the actual window content, not merely add a translucent tint.");
    }
    if (frameworkWindow)
        IMPORT_COMPARE(alert->property("resolvedBackdropSource").value<QQuickItem *>(),
                 window->property("materialBackdropSource").value<QQuickItem *>());
    int redTextPixels = 0;
    const QPoint buttonOrigin = scenePoint(secondary, QPointF(0, 0));
    for (int y = buttonOrigin.y(); y < buttonOrigin.y() + secondary->height(); ++y) {
        for (int x = buttonOrigin.x(); x < buttonOrigin.x() + secondary->width(); ++x) {
            const QColor pixel = frosted.pixelColor(x, y);
            if (pixel.red() > 150 && pixel.red() > pixel.green() * 1.5 && pixel.blue() < 130)
                ++redTextPixels;
        }
    }
    CTEST_CHECK_MESSAGE(redTextPixels > 20, "Discard text must remain sharp and red over the glass.");
    CTEST_CHECK(alert->setProperty("glassEnabled", true));

    ImportSignalRecorder primarySpy;
    const auto primarySpyConnection = QObject::connect(alert, SIGNAL(primaryClicked()), &primarySpy, SLOT(recordVoid()));
    CTEST_CHECK(primarySpyConnection);
    ImportSignalRecorder secondarySpy;
    const auto secondarySpyConnection = QObject::connect(alert, SIGNAL(secondaryClicked()), &secondarySpy, SLOT(recordVoid()));
    CTEST_CHECK(secondarySpyConnection);
    ImportSignalRecorder tertiarySpy;
    const auto tertiarySpyConnection = QObject::connect(alert, SIGNAL(tertiaryClicked()), &tertiarySpy, SLOT(recordVoid()));
    CTEST_CHECK(tertiarySpyConnection);
    ImportSignalRecorder dismissSpy;
    const auto dismissSpyConnection = QObject::connect(alert, SIGNAL(dismissed()), &dismissSpy, SLOT(recordVoid()));
    CTEST_CHECK(dismissSpyConnection);
    for (QQuickItem *button : {primary, secondary, tertiary})
        mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                         scenePoint(button, QPointF(button->width() / 2, button->height() / 2)));
    IMPORT_COMPARE(primarySpy.events.size(), 1);
    IMPORT_COMPARE(secondarySpy.events.size(), 1);
    IMPORT_COMPARE(tertiarySpy.events.size(), 1);
    CTEST_CHECK(alert->property("open").toBool());
    CTEST_CHECK(alert->setProperty("secondaryEnabled", false));
    mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                     scenePoint(secondary, QPointF(50, 20)));
    IMPORT_COMPARE(secondarySpy.events.size(), 1);
    mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(10, 10));
    IMPORT_COMPARE(root->property("backgroundClicks").toInt(), 0);
    IMPORT_COMPARE(dismissSpy.events.size(), 0);
    CTEST_CHECK(alert->setProperty("dismissOnBackground", true));
    mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(10, 10));
    IMPORT_COMPARE(dismissSpy.events.size(), 1);
    CTEST_CHECK(!alert->property("open").toBool());
    CTEST_CHECK(!capture->property("live").toBool());
    CTEST_CHECK(!capture->property("sourceItem").value<QQuickItem *>());

    CTEST_CHECK(alert->setProperty("open", true));
    CTEST_WAIT_DEFAULT(alert->property("glassActive").toBool());
    CTEST_CHECK(alert->setProperty("useOverlayLayer", false));
    CTEST_WAIT_DEFAULT(!alert->property("glassActive").toBool());
    CTEST_CHECK(alert->setProperty("useOverlayLayer", true));
    CTEST_WAIT_DEFAULT(alert->property("glassActive").toBool());
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
        id: defaultItem
        visible: false
        label: "Default"
    }

    LV.MenuItem {
        id: figmaItem
        objectName: "figmaItem"
        visible: false
        width: 161
    }

    LV.MenuItem {
        id: selectedItem
        visible: false
        state: selectedState
    }

    LV.MenuItem {
        id: inactiveItem
        visible: false
        state: inactiveState
    }

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

    LV.MenuItem {
        id: wideItem
        visible: false
        itemWidth: 120
        label: "A Very Wide Menu Entry"
        keyVisible: true
        key: "Cmd+Shift+P"
        showChevron: true
        hasChildItems: true
        expanded: false
        selectionDirection: "auto"
    }

    LV.MenuItem {
        id: constrainedItem
        objectName: "constrainedItem"
        visible: false
        itemWidth: 120
        width: 120
        label: "A Very Wide Menu Entry"
        keyVisible: true
        key: "Cmd+Shift+P"
        showChevron: true
        hasChildItems: true
        expanded: false
        selectionDirection: "auto"
    }

    LV.MenuItem {
        id: noTrailingConstrainedItem
        objectName: "noTrailingConstrainedItem"
        visible: false
        itemWidth: 120
        width: 160
        label: "A Very Wide Menu Entry"
        keyVisible: false
        showChevron: false
        hasChildItems: false
        expanded: false
        selectionDirection: "auto"
    }

    property string menuItemDiagnostics: JSON.stringify({labelWidth: figmaItem.labelNaturalWidth, row: [figmaItem.implicitWidth, figmaItem.implicitHeight], selected: selectedItem.resolvedBackgroundColor === LV.Theme.accentMuted, inactive: inactiveItem.resolvedBackgroundColor === LV.Theme.panelBackground08, iconColor: collapsedSubmenu.iconPlaceholderColor === LV.Theme.accentBlueMuted})
    property bool menuItemContract:
        defaultItem.keyVisible
        && defaultItem.resolvedShortcutText === "key"
        && defaultItem.effectiveShowChevron
        && figmaItem.label === "Label"
        && figmaItem.key === "key"
        && figmaItem.keyVisible
        && figmaItem.resolvedIconName === "procedure"
        && figmaItem.itemWidth === 161
        && figmaItem.itemHeight === 24
        && figmaItem.implicitWidth === 161
        && figmaItem.implicitHeight === 24
        && figmaItem.leftPadding === 4
        && figmaItem.rightPadding === 4
        && figmaItem.topPadding === 3
        && figmaItem.bottomPadding === 3
        && figmaItem.chevronSize === 16
        && figmaItem.chevronIconName === "generalchevronRight"
        && figmaItem.resolvedChevronRotation === 0
        && figmaItem.cornerRadius === 4
        && figmaItem.labelNaturalWidth === 33
        && figmaItem.resolvedIconSource.toString() === LV.Theme.iconPath("procedure").toString()
        && selectedItem.isSelected
        && selectedItem.resolvedBackgroundColor === LV.Theme.accentMuted
        && inactiveItem.isInactive
        && inactiveItem.resolvedBackgroundColor === LV.Theme.panelBackground08
        && collapsedSubmenu.keyVisible
        && collapsedSubmenu.resolvedShortcutText === "Cmd+K"
        && collapsedSubmenu.effectiveShowChevron
        && collapsedSubmenu.itemHeight === 24
        && collapsedSubmenu.leftPadding === 4
        && collapsedSubmenu.rightPadding === 4
        && collapsedSubmenu.topPadding === 3
        && collapsedSubmenu.bottomPadding === 3
        && collapsedSubmenu.iconPlaceholderColor === LV.Theme.accentBlueMuted
        && collapsedSubmenu.resolvedSelectionDirection === collapsedSubmenu.directionRight
        && expandedSubmenu.resolvedSelectionDirection === expandedSubmenu.directionDown
        && noChild.resolvedShortcutText === "key"
        && !noChild.effectiveShowChevron
        && hiddenKey.resolvedShortcutText === ""
        && hiddenKey.keyVisible === false
        && wideItem.implicitWidth > wideItem.itemWidth
        && constrainedItem.width === 120
        && noTrailingConstrainedItem.width === 160
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("menuItemContract").toBool(), qPrintable(root->property("menuItemDiagnostics").toString()));

    QObject *figmaItem = root->findChild<QObject *>(QStringLiteral("figmaItem"));
    CTEST_CHECK(figmaItem);

    auto *figmaContentRow =
        qobject_cast<QQuickItem *>(figmaItem->findChild<QObject *>(QStringLiteral("menuItem_contentRow")));
    auto *figmaIconSlot =
        qobject_cast<QQuickItem *>(figmaItem->findChild<QObject *>(QStringLiteral("menuItem_iconSlot")));
    auto *figmaIconImage =
        qobject_cast<QQuickItem *>(figmaItem->findChild<QObject *>(QStringLiteral("menuItem_iconImage")));
    auto *figmaLabelNode =
        qobject_cast<QQuickItem *>(figmaItem->findChild<QObject *>(QStringLiteral("menuItem_labelNode")));
    auto *figmaTrailingGroup =
        qobject_cast<QQuickItem *>(figmaItem->findChild<QObject *>(QStringLiteral("menuItem_trailingGroup")));
    auto *figmaShortcutLabel =
        qobject_cast<QQuickItem *>(figmaItem->findChild<QObject *>(QStringLiteral("menuItem_shortcutLabel")));
    auto *figmaChevronIcon =
        qobject_cast<QQuickItem *>(figmaItem->findChild<QObject *>(QStringLiteral("menuItem_chevronIcon")));

    CTEST_CHECK(figmaContentRow);
    CTEST_CHECK(figmaIconSlot);
    CTEST_CHECK(figmaIconImage);
    CTEST_CHECK(figmaLabelNode);
    CTEST_CHECK(figmaTrailingGroup);
    CTEST_CHECK(figmaShortcutLabel);
    CTEST_CHECK(figmaChevronIcon);

    CTEST_CHECK(qAbs(figmaContentRow->width() - 153.0) < 0.01);
    CTEST_CHECK(qAbs(figmaContentRow->height() - 18.0) < 0.01);
    CTEST_CHECK(qAbs(figmaIconSlot->x() - 0.0) < 0.01);
    CTEST_CHECK(qAbs(figmaIconSlot->y() - 0.0) < 0.01);
    CTEST_CHECK(qAbs(figmaIconSlot->width() - 18.0) < 0.01);
    CTEST_CHECK(qAbs(figmaIconSlot->height() - 18.0) < 0.01);
    IMPORT_COMPARE(figmaIconImage->property("status").toInt(), 1);
    CTEST_CHECK(qAbs(figmaLabelNode->x() - 26.0) < 0.01);
    CTEST_CHECK_MESSAGE(qAbs(figmaLabelNode->y() - 2.5) < 0.01,
             qPrintable(QStringLiteral("Figma label y/height: %1/%2")
                            .arg(figmaLabelNode->y())
                            .arg(figmaLabelNode->height())));
    CTEST_CHECK_MESSAGE(qAbs(figmaLabelNode->width() - 33.0) < 0.01,
             qPrintable(QStringLiteral("Figma label bounds: x=%1 y=%2 width=%3 height=%4")
                            .arg(figmaLabelNode->x())
                            .arg(figmaLabelNode->y())
                            .arg(figmaLabelNode->width())
                            .arg(figmaLabelNode->height())));
    CTEST_CHECK(qAbs(figmaLabelNode->height() - 13.0) < 0.01);
    IMPORT_COMPARE(figmaLabelNode->property("style").toInt(), figmaLabelNode->property("body").toInt());
    const QFont figmaLabelFont = figmaLabelNode->property("font").value<QFont>();
    IMPORT_COMPARE(figmaLabelFont.pixelSize(), 13);
    IMPORT_COMPARE(figmaLabelFont.weight(), QFont::Medium);
    IMPORT_COMPARE(figmaLabelFont.styleName(), QStringLiteral("Medium"));
    CTEST_CHECK_MESSAGE(qAbs(figmaTrailingGroup->x() - 108.0) < 0.01,
             qPrintable(QStringLiteral("Figma trailing bounds: x=%1 width=%2 shortcut=%3 natural=%4 content=%5")
                            .arg(figmaTrailingGroup->x())
                            .arg(figmaTrailingGroup->width())
                            .arg(figmaShortcutLabel->width())
                            .arg(figmaItem->property("shortcutNaturalWidth").toDouble())
                            .arg(figmaContentRow->width())));
    CTEST_CHECK(qAbs(figmaTrailingGroup->y() - 1.0) < 0.01);
    CTEST_CHECK(qAbs(figmaTrailingGroup->width() - 45.0) < 0.01);
    CTEST_CHECK(qAbs(figmaTrailingGroup->height() - 16.0) < 0.01);
    CTEST_CHECK(qAbs(figmaShortcutLabel->x() - 0.0) < 0.01);
    CTEST_CHECK_MESSAGE(qAbs(figmaShortcutLabel->y() - 1.5) < 0.01,
             qPrintable(QStringLiteral("Figma shortcut bounds: x=%1 y=%2 width=%3 height=%4")
                            .arg(figmaShortcutLabel->x())
                            .arg(figmaShortcutLabel->y())
                            .arg(figmaShortcutLabel->width())
                            .arg(figmaShortcutLabel->height())));
    CTEST_CHECK(qAbs(figmaShortcutLabel->width() - 21.0) < 0.01);
    CTEST_CHECK(qAbs(figmaShortcutLabel->height() - 13.0) < 0.01);
    IMPORT_COMPARE(figmaShortcutLabel->property("style").toInt(), figmaShortcutLabel->property("body").toInt());
    const QFont figmaShortcutFont = figmaShortcutLabel->property("font").value<QFont>();
    IMPORT_COMPARE(figmaShortcutFont.pixelSize(), 13);
    IMPORT_COMPARE(figmaShortcutFont.weight(), QFont::Medium);
    IMPORT_COMPARE(figmaShortcutFont.styleName(), QStringLiteral("Medium"));
    IMPORT_COMPARE(figmaChevronIcon->property("status").toInt(), 1);
    CTEST_CHECK(figmaChevronIcon->property("source").toUrl().toString().contains(
        QStringLiteral("generalchevronRight")));
    CTEST_CHECK(qAbs(figmaChevronIcon->x() - 29.0) < 0.01);
    CTEST_CHECK(qAbs(figmaChevronIcon->y() - 0.0) < 0.01);
    CTEST_CHECK(qAbs(figmaChevronIcon->width() - 16.0) < 0.01);
    CTEST_CHECK(qAbs(figmaChevronIcon->height() - 16.0) < 0.01);

    QObject *constrainedItem = root->findChild<QObject *>(QStringLiteral("constrainedItem"));
    CTEST_CHECK(constrainedItem);

    auto *contentRow = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_contentRow")));
    auto *labelNode = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_labelNode")));
    auto *spacer = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_flexibleSpacer")));
    auto *trailingGroup = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_trailingGroup")));
    auto *shortcutLabel = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_shortcutLabel")));
    auto *chevronIcon = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_chevronIcon")));

    CTEST_CHECK(contentRow);
    CTEST_CHECK(labelNode);
    CTEST_CHECK(spacer);
    CTEST_CHECK(trailingGroup);
    CTEST_CHECK(shortcutLabel);
    CTEST_CHECK(chevronIcon);
    IMPORT_COMPARE(shortcutLabel->property("style").toInt(), shortcutLabel->property("body").toInt());

    const qreal contentWidth = contentRow->width();
    const qreal labelRight = labelNode->x() + labelNode->width();
    const qreal trailingRight = trailingGroup->x() + trailingGroup->width();
    const qreal shortcutRight = shortcutLabel->x() + shortcutLabel->width();
    const qreal chevronRight = chevronIcon->x() + chevronIcon->width();

    CTEST_CHECK_MESSAGE(labelRight <= contentWidth + 0.01, "Menu label must stay within the content row.");
    CTEST_CHECK_MESSAGE(trailingGroup->x() >= labelRight - 0.01, "Trailing group must not overlap the label block.");
    CTEST_CHECK_MESSAGE(spacer->width() >= -0.01, "Responsive spacer must not resolve to a negative width.");
    CTEST_CHECK_MESSAGE(trailingRight <= contentWidth + 0.01, "Trailing group must stay within the content row.");
    CTEST_CHECK_MESSAGE(shortcutRight <= trailingGroup->width() + 0.01,
             qPrintable(QStringLiteral("Shortcut label must stay within the trailing group (%1 <= %2).")
                            .arg(shortcutRight)
                            .arg(trailingGroup->width())));
    CTEST_CHECK_MESSAGE(chevronRight <= trailingGroup->width() + 0.01,
             qPrintable(QStringLiteral("Chevron must stay within the trailing group (%1 <= %2).")
                            .arg(chevronRight)
                            .arg(trailingGroup->width())));

    QObject *noTrailingConstrainedItem = root->findChild<QObject *>(QStringLiteral("noTrailingConstrainedItem"));
    CTEST_CHECK(noTrailingConstrainedItem);

    auto *noTrailingContentRow =
        qobject_cast<QQuickItem *>(noTrailingConstrainedItem->findChild<QObject *>(QStringLiteral("menuItem_contentRow")));
    auto *noTrailingLabelNode =
        qobject_cast<QQuickItem *>(noTrailingConstrainedItem->findChild<QObject *>(QStringLiteral("menuItem_labelNode")));
    QObject *noTrailingGroup =
        noTrailingConstrainedItem->findChild<QObject *>(QStringLiteral("menuItem_trailingGroup"));

    CTEST_CHECK(noTrailingContentRow);
    CTEST_CHECK(noTrailingLabelNode);
    CTEST_CHECK(noTrailingGroup);

    const qreal noTrailingAvailableLabelWidth = noTrailingContentRow->width() - noTrailingLabelNode->x();
    CTEST_CHECK_MESSAGE(qAbs(noTrailingLabelNode->width() - noTrailingAvailableLabelWidth) < 0.01,
             qPrintable(QStringLiteral("Label should consume the full available width when trailing content is absent (%1 vs %2).")
                            .arg(noTrailingLabelNode->width())
                            .arg(noTrailingAvailableLabelWidth)));
    CTEST_CHECK(!noTrailingGroup->property("visible").toBool());
}

void ImportApiTests::menu_item_icon_slot_switch_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    LV.MenuItem {
        id: defaultSlotItem
        objectName: "defaultSlotItem"
        itemWidth: 0
        label: "Same Label"
    }

    LV.MenuItem {
        id: hiddenSlotItem
        objectName: "hiddenSlotItem"
        itemWidth: 0
        label: "Same Label"
        showIconSlot: false
    }

    property real expectedIconSlotDelta: defaultSlotItem.iconSize + LV.Theme.gap8
    property bool iconSlotSwitchContract:
        defaultSlotItem.showIconSlot === true
        && defaultSlotItem.effectiveShowIconSlot === true
        && hiddenSlotItem.showIconSlot === false
        && hiddenSlotItem.effectiveShowIconSlot === false
        && Math.abs((defaultSlotItem.implicitWidth - hiddenSlotItem.implicitWidth) - expectedIconSlotDelta) < 0.01
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("iconSlotSwitchContract").toBool());

    QObject *defaultSlotItem = root->findChild<QObject *>(QStringLiteral("defaultSlotItem"));
    QObject *hiddenSlotItem = root->findChild<QObject *>(QStringLiteral("hiddenSlotItem"));
    CTEST_CHECK(defaultSlotItem);
    CTEST_CHECK(hiddenSlotItem);

    auto *defaultIconSlot =
        qobject_cast<QQuickItem *>(defaultSlotItem->findChild<QObject *>(QStringLiteral("menuItem_iconSlot")));
    auto *hiddenIconSlot =
        qobject_cast<QQuickItem *>(hiddenSlotItem->findChild<QObject *>(QStringLiteral("menuItem_iconSlot")));
    auto *defaultLabel =
        qobject_cast<QQuickItem *>(defaultSlotItem->findChild<QObject *>(QStringLiteral("menuItem_labelNode")));
    auto *hiddenLabel =
        qobject_cast<QQuickItem *>(hiddenSlotItem->findChild<QObject *>(QStringLiteral("menuItem_labelNode")));

    CTEST_CHECK(defaultIconSlot);
    CTEST_CHECK(hiddenIconSlot);
    CTEST_CHECK(defaultLabel);
    CTEST_CHECK(hiddenLabel);

    CTEST_CHECK(defaultIconSlot->property("visible").toBool());
    CTEST_CHECK(!hiddenIconSlot->property("visible").toBool());
    IMPORT_COMPARE(defaultIconSlot->width(), defaultSlotItem->property("iconSize").toDouble());
    IMPORT_COMPARE(hiddenIconSlot->width(), 0.0);
    CTEST_CHECK(defaultLabel->x() > hiddenLabel->x());
    IMPORT_COMPARE(hiddenLabel->x(), 0.0);
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("actionContract").toBool());
}

void ImportApiTests::context_menu_visual_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    LV.Menu {
        id: menu
        visible: false
        items: [
            {
                label: "Label",
                key: "key",
                keyVisible: true,
                iconName: "procedure",
                showChevron: true,
                hasChildItems: true
            },
            { label: "Label", key: "key", keyVisible: true, iconName: "procedure", showChevron: true, hasChildItems: true },
            { label: "Label", key: "key", keyVisible: true, iconName: "procedure", showChevron: true, hasChildItems: true },
            { type: "divider" },
            { label: "Label", key: "key", keyVisible: true, iconName: "procedure", showChevron: true, hasChildItems: true },
            { label: "Label", key: "key", keyVisible: true, iconName: "procedure", showChevron: true, hasChildItems: true },
            { label: "Label", key: "key", keyVisible: true, iconName: "procedure", showChevron: true, hasChildItems: true },
            { type: "divider" },
            { label: "Label", key: "key", keyVisible: true, iconName: "procedure", showChevron: true, hasChildItems: true },
            { label: "Label", key: "key", keyVisible: true, iconName: "procedure", showChevron: true, hasChildItems: true }
        ]
    }

    LV.MenuDivider {
        id: divider
        objectName: "standaloneDivider"
        visible: false
    }

    LV.MenuDivider {
        id: composedDivider
        objectName: "composedDivider"
        visible: false
        width: 145
    }

    property bool visualContract:
        menu.itemWidth === 145
        && menu.minimumItemWidth === 145
        && menu.resolvedItemWidth === 145
        && menu.itemSpacing === LV.Theme.gap2
        && menu.leftPadding === LV.Theme.gap8
        && menu.rightPadding === LV.Theme.gap8
        && menu.topPadding === LV.Theme.gap4
        && menu.bottomPadding === LV.Theme.gap4
        && menu.menuColor === LV.Theme.contextMenuSurface
        && menu.menuColor === LV.Theme.materialTint
        && menu.menuOpacity === LV.Theme.contextMenuOpacity
        && menu.dividerColor === LV.Theme.menuDivider
        && menu.dividerColor === LV.Theme.panelBackground08
        && menu.background !== null
        && menu.background.radius === LV.Theme.materialPanelRadius
        && menu.background.tintOpacity === 0.12
        && menu.background.blurRadius === LV.Theme.contextMenuBlur
        && Math.abs(menu.implicitWidth - 161.0) < 0.01
        && Math.abs(menu.implicitHeight - 224.0) < 0.01
        && Math.abs(menu.contentItem.width - 145.0) < 0.01
        && Math.abs(menu.contentItem.implicitHeight - 216.0) < 0.01
        && divider.dividerColor === LV.Theme.menuDivider
        && divider.dividerColor === LV.Theme.panelBackground08
        && divider.lineLength === 220
        && divider.linePadding === 0
        && Math.abs(divider.thickness - 1.0) < 0.01
        && Math.abs(divider.implicitWidth - 220.0) < 0.01
        && Math.abs(divider.implicitHeight - 3.0) < 0.01
        && Math.abs(divider.implicitHeight - ((divider.crossPadding * 2) + divider.thickness)) < 0.01
        && Math.abs(composedDivider.implicitHeight - 3.0) < 0.01
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("visualContract").toBool());

    QObject *standaloneDivider = root->findChild<QObject *>(QStringLiteral("standaloneDivider"));
    QObject *composedDivider = root->findChild<QObject *>(QStringLiteral("composedDivider"));
    CTEST_CHECK(standaloneDivider);
    CTEST_CHECK(composedDivider);

    auto *standaloneLine = qobject_cast<QQuickItem *>(
        standaloneDivider->findChild<QObject *>(QStringLiteral("menuDivider_line")));
    auto *composedLine = qobject_cast<QQuickItem *>(
        composedDivider->findChild<QObject *>(QStringLiteral("menuDivider_line")));
    CTEST_CHECK(standaloneLine);
    CTEST_CHECK(composedLine);
    CTEST_CHECK(qAbs(standaloneLine->x() - 0.0) < 0.01);
    CTEST_CHECK(qAbs(standaloneLine->y() - 1.0) < 0.01);
    CTEST_CHECK(qAbs(standaloneLine->width() - 220.0) < 0.01);
    CTEST_CHECK(qAbs(standaloneLine->height() - 1.0) < 0.01);
    CTEST_CHECK(qAbs(composedLine->x() - 0.0) < 0.01);
    CTEST_CHECK(qAbs(composedLine->y() - 1.0) < 0.01);
    CTEST_CHECK(qAbs(composedLine->width() - 145.0) < 0.01);
    CTEST_CHECK(qAbs(composedLine->height() - 1.0) < 0.01);
}

void ImportApiTests::context_menu_icon_slot_switch_contract_loads()
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
    height: 120

    Item {
        id: delegateHost
        objectName: "delegateHost"
        width: 240
        height: 80
    }

    LV.ContextMenu {
        id: menu
        visible: false
        itemWidth: 0
        showIconSlot: false
        items: [
            { label: "No Slot", iconName: "projectStructure" },
            { label: "Override Slot", iconName: "projectStructure", showIconSlot: true }
        ]
    }

    property var noSlotDelegate: null
    property var overrideSlotDelegate: null

    Component.onCompleted: {
        noSlotDelegate = menu.createEntryDelegate(delegateHost, menu.entryDelegateItems[0])
        noSlotDelegate.objectName = "contextMenuNoSlotDelegate"
        overrideSlotDelegate = menu.createEntryDelegate(delegateHost, menu.entryDelegateItems[1])
        overrideSlotDelegate.objectName = "contextMenuOverrideSlotDelegate"
    }

    property bool iconSlotSwitchContract:
        menu.showIconSlot === false
        && menu.itemShowIconSlot(menu.entryAt(0)) === false
        && menu.itemShowIconSlot(menu.entryAt(1)) === true
        && menu.entryDelegateItems[0].showIconSlot === false
        && menu.entryDelegateItems[1].showIconSlot === true
        && noSlotDelegate !== null
        && overrideSlotDelegate !== null
        && noSlotDelegate.showIconSlot === false
        && overrideSlotDelegate.showIconSlot === true
        && overrideSlotDelegate.implicitWidth > noSlotDelegate.implicitWidth
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("iconSlotSwitchContract").toBool());

    QObject *noSlotDelegate = root->findChild<QObject *>(QStringLiteral("contextMenuNoSlotDelegate"));
    QObject *overrideSlotDelegate = root->findChild<QObject *>(QStringLiteral("contextMenuOverrideSlotDelegate"));
    CTEST_CHECK(noSlotDelegate);
    CTEST_CHECK(overrideSlotDelegate);

    auto *noSlotIcon =
        qobject_cast<QQuickItem *>(noSlotDelegate->findChild<QObject *>(QStringLiteral("menuItem_iconSlot")));
    auto *overrideSlotIcon =
        qobject_cast<QQuickItem *>(overrideSlotDelegate->findChild<QObject *>(QStringLiteral("menuItem_iconSlot")));
    CTEST_CHECK(noSlotIcon);
    CTEST_CHECK(overrideSlotIcon);
    CTEST_CHECK(!noSlotIcon->isVisible());
    CTEST_CHECK(overrideSlotIcon->isVisible());
}

void ImportApiTests::context_menu_width_expansion_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    LV.ContextMenu {
        id: contentDrivenMenu
        visible: false
        itemWidth: 120
        items: [
            {
                label: "A Very Wide Menu Entry",
                key: "Cmd+Shift+P",
                keyVisible: true,
                showChevron: true,
                hasChildItems: true
            }
        ]
    }

    LV.ContextMenu {
        id: widthDrivenMenu
        visible: false
        itemWidth: 120
        width: 280
        items: [
            {
                label: "Short",
                showChevron: false,
                hasChildItems: false
            }
        ]
    }

    LV.ContextMenu {
        id: narrowWidthMenu
        visible: false
        itemWidth: 120
        width: 100
        items: [
            {
                label: "A Very Wide Menu Entry",
                key: "Cmd+Shift+P",
                keyVisible: true,
                showChevron: true,
                hasChildItems: true
            }
        ]
    }

    LV.ContextMenu {
        id: inferredShortcutMenu
        visible: false
        itemWidth: 120
        items: [
            {
                label: "Entry Without Shortcut",
                showChevron: false,
                hasChildItems: false
            }
        ]
    }

    readonly property real contentDrivenBaseWidth:
        contentDrivenMenu.itemWidth + contentDrivenMenu.leftPadding + contentDrivenMenu.rightPadding
    readonly property real widthDrivenContentWidth:
        widthDrivenMenu.width - widthDrivenMenu.leftPadding - widthDrivenMenu.rightPadding
    readonly property real narrowWidthContentWidth:
        narrowWidthMenu.width - narrowWidthMenu.leftPadding - narrowWidthMenu.rightPadding
    readonly property string widthDebug:
        contentDrivenMenu.implicitWidth + ","
        + contentDrivenBaseWidth + ","
        + contentDrivenMenu.resolvedItemWidth + ","
        + contentDrivenMenu.contentItem.width + ","
        + contentDrivenMenu.itemWidth + ","
        + widthDrivenMenu.resolvedItemWidth + ","
        + widthDrivenContentWidth + ","
        + widthDrivenMenu.contentItem.width + ","
        + widthDrivenMenu.itemWidth + ","
        + narrowWidthMenu.width + ","
        + narrowWidthMenu.implicitWidth + ","
        + narrowWidthMenu.resolvedPopupWidth + ","
        + narrowWidthContentWidth + ","
        + narrowWidthMenu.resolvedItemWidth + ","
        + narrowWidthMenu.contentItem.width + ","
        + inferredShortcutMenu.itemKeyVisible(inferredShortcutMenu.entryAt(0))

    property bool widthExpansionContract:
        contentDrivenMenu.implicitWidth > contentDrivenBaseWidth
        && contentDrivenMenu.resolvedItemWidth === contentDrivenMenu.contentItem.width
        && contentDrivenMenu.resolvedItemWidth > contentDrivenMenu.itemWidth
        && Math.abs(widthDrivenMenu.resolvedItemWidth - widthDrivenContentWidth) < 0.01
        && Math.abs(widthDrivenMenu.contentItem.width - widthDrivenContentWidth) < 0.01
        && widthDrivenMenu.resolvedItemWidth > widthDrivenMenu.itemWidth
        && Math.abs(narrowWidthMenu.width - narrowWidthMenu.resolvedPopupWidth) < 0.01
        && Math.abs(narrowWidthMenu.width - narrowWidthMenu.implicitWidth) < 0.01
        && Math.abs(narrowWidthMenu.resolvedItemWidth - narrowWidthContentWidth) < 0.01
        && Math.abs(narrowWidthMenu.contentItem.width - narrowWidthContentWidth) < 0.01
        && narrowWidthMenu.width > 100
        && !inferredShortcutMenu.itemKeyVisible(inferredShortcutMenu.entryAt(0))
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    const QString widthDebug = root->property("widthDebug").toString();
    IMPORT_WAIT_MESSAGE(root->property("widthExpansionContract").toBool(), qPrintable(widthDebug));
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
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("placementContract").toBool());
}

void ImportApiTests::model_component_delegate_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 640
    height: 480

    property string listDelegateLabel: ""
    property bool listTriggered: false
    property string menuDelegateLabel: ""
    property bool menuDividerReady: false
    property bool menuTriggered: false
    property string headerDelegateText: ""
    property string cellDelegateText: ""
    property bool hierarchyDelegateReady: false

    Component {
        id: listItemDelegate
        Rectangle {
            property var modelData: ({})
            width: parent ? parent.width : 120
            height: 20
            color: modelData.selected ? "steelblue" : "transparent"

            Component.onCompleted: {
                if (modelData.index === 0)
                    root.listDelegateLabel = modelData.label
                if (modelData.index === 1 && modelData.trigger)
                    modelData.trigger()
            }
        }
    }

    Component {
        id: menuItemDelegate
        LV.MenuItem {
            property var modelData: ({})
            label: modelData.label || ""
            key: modelData.shortcut || ""
            keyVisible: modelData.keyVisible === true
            itemWidth: 120
            Component.onCompleted: {
                if (modelData.index === 0)
                    root.menuDelegateLabel = modelData.label
                if (modelData.index === 0 && modelData.trigger)
                    root.menuTriggered = modelData.trigger()
            }
        }
    }

    Component {
        id: menuDividerDelegate
        LV.MenuDivider {
            property var modelData: ({})
            width: 120
            Component.onCompleted: root.menuDividerReady = modelData.divider === true
        }
    }

    Component {
        id: headerCellDelegate
        Item {
            property var modelData: ({})
            Component.onCompleted: {
                if (modelData.index === 1)
                    root.headerDelegateText = modelData.text
            }
        }
    }

    Component {
        id: tableCellDelegate
        Item {
            property var modelData: ({})
            Component.onCompleted: {
                if (modelData.index === 1 && modelData.rowIndex === 0 && modelData.columnIndex === 1)
                    root.cellDelegateText = modelData.text
            }
        }
    }

    Component {
        id: hierarchyItemDelegate
        LV.HierarchyItem {
            property bool delegateMarker: true
            Component.onCompleted: {
                if (modelData && modelData.itemKey === "child")
                    root.hierarchyDelegateReady = delegateMarker
            }
        }
    }

    LV.List {
        id: list
        visible: false
        model: [
            { label: "List A", selected: true },
            { label: "List B" }
        ]
        itemDelegate: listItemDelegate
        onItemTriggered: function(index, item) {
            if (index === 1 && item.label === "List B")
                root.listTriggered = true
        }
    }

    LV.ContextMenu {
        id: menu
        itemDelegate: menuItemDelegate
        dividerDelegate: menuDividerDelegate
        items: [
            { label: "Open", key: "O", eventName: "open" },
            { type: "divider" }
        ]
        onItemTriggered: function(index, item) {
            if (index === 0 && item.label === "Open")
                root.menuTriggered = true
        }
    }

    LV.TableHeader {
        id: header
        visible: false
        width: 240
        cellDelegate: headerCellDelegate
        cellItems: [
            { label: "Name" },
            { label: "Count" }
        ]
    }

    LV.Table {
        id: table
        visible: false
        width: 240
        height: 96
        structureControlsVisible: false
        headerColumns: ["Name", "Count"]
        cellDelegate: tableCellDelegate
        rows: [[{ value: "Renderer" }, { value: 3 }]]
    }

    LV.HierarchyList {
        id: hierarchyList
        visible: false
        model: [
            { key: "root", label: "Root", depth: 0, expanded: true },
            { key: "child", label: "Child", depth: 1 }
        ]
        itemDelegate: hierarchyItemDelegate
    }

    property bool delegateContractReady:
        listDelegateLabel === "List A"
        && listTriggered
        && menuDelegateLabel === "Open"
        && menuDividerReady
        && menuTriggered
        && headerDelegateText === "Count"
        && cellDelegateText === "3"
        && hierarchyDelegateReady
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("delegateContractReady").toBool());
}

void ImportApiTests::table_figma_geometry_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 1700
    height: 360

    LV.TableHeader {
        id: header
        objectName: "figmaHeader"
        x: 0
        y: 0
        width: implicitWidth
        height: implicitHeight
        opacity: 0
        cellItems: [
            { label: "Column" },
            { label: "No leading", leadingIcon: "", trailingIcon: "generalchevronDown" },
            { label: "Both", leadingIcon: "generalchevronDown", trailingIcon: "generalchevronDown" }
        ]
    }

    LV.TableRow {
        id: row
        objectName: "figmaRow"
        x: 0
        y: 40
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }

    LV.TableCellItem {
        id: cell
        objectName: "figmaCell"
        x: 760
        y: 0
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }

    LV.Table {
        id: table
        objectName: "figmaTable"
        x: 760
        y: 40
        width: implicitWidth
        height: implicitHeight
        opacity: 0
    }

    property bool figmaTableReady:
        header.implicitWidth === 717
        && header.implicitHeight === 25
        && header.rowHeight === 24
        && header.separatorHeight === 1
        && header.cellHorizontalPadding === 8
        && header.resolvedColumnCount === 3
        && header.headerDescriptors[0].x === 0
        && header.headerDescriptors[0].width === 239
        && header.headerDescriptors[0].padding === 8
        && header.headerDescriptors[1].x === 239
        && header.headerDescriptors[2].x === 478
        && header.textColor === LV.Theme.descriptionColor
        && header.separatorColor === LV.Theme.panelBackground10
        && row.implicitWidth === 717
        && row.implicitHeight === 24
        && row.cellWidth === 234
        && row.cellHeight === 24
        && Math.abs(row.resolvedSpacing - 7.5) < 0.01
        && row.dividerColor === LV.Theme.panelBackground10
        && cell.implicitWidth === 234
        && cell.implicitHeight === 24
        && cell.resolvedCellHeight === 24
        && cell.resolvedContentSpacing === 8
        && cell.resolvedDividerColor === LV.Theme.panelBackground03
        && cell.resolvedTextColor === LV.Theme.bodyColor
        && table.implicitWidth === 528
        && table.implicitHeight === 121
        && table.rowHeight === 24
        && table.resolvedRowCount === 4
        && table.resolvedColumnCount === 3
        && String(table.backgroundColor) === "#1e1e1e"
        && table.borderColor === LV.Theme.panelBackground10
        && table.borderWidth === 1
        && table.rowDividerColor === LV.Theme.panelBackground10
        && table.headerSeparatorColor === LV.Theme.panelBackground10
        && !table.structureControlsVisible

    property string figmaTableDiagnostics: JSON.stringify({
        header: [header.implicitWidth, header.implicitHeight, header.rowHeight,
            header.separatorHeight, header.cellHorizontalPadding],
        headerDescriptors: header.headerDescriptors,
        row: [row.implicitWidth, row.implicitHeight, row.cellWidth,
            row.cellHeight, row.resolvedSpacing],
        cell: [cell.implicitWidth, cell.implicitHeight,
            cell.resolvedCellHeight, cell.resolvedContentSpacing],
        table: [table.implicitWidth, table.implicitHeight, table.rowHeight,
            table.resolvedRowCount, table.resolvedColumnCount],
        structureControlsVisible: table.structureControlsVisible,
        colorsDetailed: [String(header.textColor), String(header.separatorColor),
            String(row.dividerColor), String(cell.resolvedDividerColor),
            String(cell.resolvedTextColor), String(table.backgroundColor),
            String(table.borderColor), String(table.rowDividerColor),
            String(table.headerSeparatorColor)],
        widths: [table.borderWidth, table.rowHeight, table.resolvedRowCount,
            table.resolvedColumnCount],
        colors: [String(table.backgroundColor), String(table.borderColor),
            String(table.rowDividerColor)]
    })
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("figmaTableReady").toBool(),
                 qPrintable(root->property("figmaTableDiagnostics").toString()));

    auto *header = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaHeader")));
    auto *cell = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaCell")));
    auto *table = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaTable")));
    CTEST_CHECK(header);
    CTEST_CHECK(cell);
    CTEST_CHECK(table);

    auto *headerLabel = visualChildByObjectName(
        header, QStringLiteral("figmaHeader_cell_0_label"));
    auto *headerSeparator = visualChildByObjectName(
        header, QStringLiteral("figmaHeader_separator"));
    auto *cellDivider = visualChildByObjectName(
        cell, QStringLiteral("figmaCell_divider"));
    auto *cellContent = visualChildByObjectName(
        cell, QStringLiteral("figmaCell_content"));
    auto *cellLabel = visualChildByObjectName(
        cell, QStringLiteral("figmaCell_label"));
    auto *tableFrame = visualChildByObjectName(
        table, QStringLiteral("figmaTable_frame"));
    CTEST_CHECK(headerLabel);
    CTEST_CHECK(headerSeparator);
    CTEST_CHECK(cellDivider);
    CTEST_CHECK(cellContent);
    CTEST_CHECK(cellLabel);
    CTEST_CHECK(tableFrame);

    const auto boundsIn = [](QQuickItem *item, QQuickItem *ancestor) {
        return QRectF(item->mapToItem(ancestor, QPointF(0.0, 0.0)),
                      QSizeF(item->width(), item->height()));
    };
    const auto verifyBounds = [](const QRectF &actual, const QRectF &expected) {
        CTEST_CHECK_MESSAGE(qAbs(actual.x() - expected.x()) < 0.01
                     && qAbs(actual.y() - expected.y()) < 0.01
                     && qAbs(actual.width() - expected.width()) < 0.01
                     && qAbs(actual.height() - expected.height()) < 0.01,
                 qPrintable(QStringLiteral("bounds actual=(%1,%2 %3x%4) expected=(%5,%6 %7x%8)")
                                .arg(actual.x()).arg(actual.y())
                                .arg(actual.width()).arg(actual.height())
                                .arg(expected.x()).arg(expected.y())
                                .arg(expected.width()).arg(expected.height())));
    };

    auto *headerLeading = visualChildByObjectName(
        header, QStringLiteral("figmaHeader_cell_0_leadingIcon"));
    auto *headerTrailing = visualChildByObjectName(
        header, QStringLiteral("figmaHeader_cell_0_trailingIcon"));
    CTEST_CHECK(headerLeading);
    CTEST_CHECK(headerTrailing);
    verifyBounds(boundsIn(headerLeading, header), QRectF(8.0, 3.0, 18.0, 18.0));
    IMPORT_COMPARE(headerLeading->property("source").toUrl().fileName(), QStringLiteral("generalchevronDown.svg"));
    CTEST_CHECK(!headerTrailing->isVisible());
    IMPORT_COMPARE(boundsIn(headerLabel, header).x(), 33.0);
    IMPORT_COMPARE(boundsIn(headerLabel, header).y(), 6.0);
    auto *secondLeading = visualChildByObjectName(header, QStringLiteral("figmaHeader_cell_1_leadingIcon"));
    auto *secondTrailing = visualChildByObjectName(header, QStringLiteral("figmaHeader_cell_1_trailingIcon"));
    auto *secondLabel = visualChildByObjectName(header, QStringLiteral("figmaHeader_cell_1_label"));
    CTEST_CHECK(secondLeading);
    CTEST_CHECK(secondTrailing);
    CTEST_CHECK(secondLabel);
    CTEST_CHECK(!secondLeading->isVisible());
    CTEST_CHECK(secondTrailing->isVisible());
    IMPORT_COMPARE(boundsIn(secondLabel, header).x(), 247.0);
    IMPORT_COMPARE(boundsIn(secondTrailing, header).x(), boundsIn(secondLabel, header).right() + 7.0);
    verifyBounds(boundsIn(headerSeparator, header), QRectF(0.0, 24.0, 717.0, 1.0));
    verifyBounds(boundsIn(cellDivider, cell), QRectF(0.0, 0.0, 1.0, 24.0));
    verifyBounds(boundsIn(cellContent, cell), QRectF(9.0, 5.5, 225.0, 13.0));
    verifyBounds(boundsIn(cellLabel, cell), QRectF(9.0, 5.5, 225.0, 13.0));
    verifyBounds(boundsIn(tableFrame, table), QRectF(0.0, 0.0, 528.0, 121.0));
    IMPORT_COMPARE(headerLabel->property("stylePixelSize").toInt(), 12);
    IMPORT_COMPARE(headerLabel->property("styleLineHeight").toInt(), 12);
    IMPORT_COMPARE(cellLabel->property("stylePixelSize").toInt(), 13);
    IMPORT_COMPARE(cellLabel->property("styleLineHeight").toInt(), 13);

    QQuickWindow captureWindow;
    captureWindow.setColor(Qt::transparent);
    captureWindow.resize(528, 121);
    table->setParentItem(captureWindow.contentItem());
    table->setPosition(QPointF(0.0, 0.0));
    table->setSize(QSizeF(528.0, 121.0));
    table->setOpacity(1.0);
    captureWindow.show();
    QCoreApplication::processEvents();

    const QSharedPointer<QQuickItemGrabResult> grabResult = table->grabToImage(QSize(528, 121));
    CTEST_CHECK(grabResult);
    CTEST_WAIT_VERIFY(!grabResult->image().isNull(), 5000);
    const QImage captured = grabResult->image().scaled(QSize(528, 121), Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
        .convertToFormat(QImage::Format_RGBA8888);
    IMPORT_COMPARE(captured.size(), QSize(528, 121));

    const auto colorMatches = [](const QColor &actual, const QColor &expected) {
        return qAbs(actual.red() - expected.red()) <= 1
            && qAbs(actual.green() - expected.green()) <= 1
            && qAbs(actual.blue() - expected.blue()) <= 1
            && qAbs(actual.alpha() - expected.alpha()) <= 1;
    };
    const QColor panel10(QStringLiteral("#282828"));
    const QColor tableBackground(QStringLiteral("#1e1e1e"));
    CTEST_CHECK(colorMatches(captured.pixelColor(0, 0), panel10));
    CTEST_CHECK(colorMatches(captured.pixelColor(527, 120), panel10));
    CTEST_CHECK(colorMatches(captured.pixelColor(100, 24), panel10));
    CTEST_CHECK(colorMatches(captured.pixelColor(176, 30), panel10));
    CTEST_CHECK(colorMatches(captured.pixelColor(352, 30), panel10));
    CTEST_CHECK(colorMatches(captured.pixelColor(100, 30), tableBackground));

    const QString capturePath = qEnvironmentVariable("LVRS_TABLE_CAPTURE_PATH");
    if (!capturePath.isEmpty())
        CTEST_CHECK_MESSAGE(captured.save(capturePath), qPrintable(capturePath));
}

void ImportApiTests::table_spreadsheet_api_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property int selectionSignalCount: 0
    property int sortedSignalCount: 0
    property bool aliasesReady: false
    property bool addressReady: false
    property bool selectionReady: false
    property bool rangeReady: false
    property bool pasteReady: false
    property bool pasteUndoReady: false
    property bool validationReady: false
    property bool sortingReady: false
    property bool sortUndoReady: false
    property bool navigationReady: false
    property bool tsvReady: false
    property bool blankSortReady: false
    property string rangeDiagnostics: ""

    LV.Table {
        id: sheet
        visible: false
        structureControlsVisible: false
        editable: true
        columns: [
            { label: "Name", type: "string" },
            { label: "Count", type: "int" },
            { label: "Enabled", type: "bool" }
        ]
        model: [
            [{ value: "Beta" }, { value: 2 }, { value: true }],
            [{ value: "Alpha" }, { value: 10 }, { value: false }],
            [{ value: "Gamma" }, { value: 5 }, { value: true }]
        ]

        onSelectionChanged: function(range) {
            root.selectionSignalCount += 1
        }
        onRowsSorted: function(columnIndex, sortOrder) {
            if (columnIndex === 1 && sortOrder === Qt.DescendingOrder)
                root.sortedSignalCount += 1
        }
    }

    LV.Table {
        id: blankSheet
        visible: false
        columns: [
            { label: "Name", type: "string" },
            { label: "Count", type: "int" }
        ]
        model: [
            [{ value: "Zed" }, { value: 2 }],
            [{ value: "Blank" }, { value: null }],
            [{ value: "Alpha" }, { value: 10 }]
        ]
    }

    function runContract() {
        aliasesReady = sheet.columns === sheet.headerCellItems
            && sheet.model === sheet.rows
            && sheet.editable === sheet.inputable
            && sheet.rowCount === 3
            && sheet.columnCount === 3
            && sheet.headerCount === 3
            && sheet.sortingAvailable

        const aa12 = sheet.cellCoordinates("AA12")
        const invalid = sheet.cellCoordinates("12AA")
        addressReady = sheet.columnName(0) === "A"
            && sheet.columnName(25) === "Z"
            && sheet.columnName(26) === "AA"
            && sheet.cellReference(0, 0) === "A1"
            && sheet.cellReference(11, 26) === "AA12"
            && aa12.valid
            && aa12.rowIndex === 11
            && aa12.columnIndex === 26
            && !invalid.valid

        const selectedCell = sheet.selectCell(1, 1)
        selectionReady = selectedCell
            && sheet.hasSelection
            && sheet.currentRow === 1
            && sheet.currentColumn === 1
            && sheet.currentCell.address === "B2"
            && sheet.selectedRange.startRow === 1
            && sheet.selectedRange.startColumn === 1
            && sheet.selectedCellCount === 1
            && sheet.isCellSelected(1, 1)
            && !sheet.isCellSelected(0, 0)

        const selectedRange = sheet.selectRange(0, 0, 1, 1)
        const rangeValues = sheet.selectionValues()
        const descriptors = sheet.selectedCellDescriptors()
        rangeDiagnostics = JSON.stringify({
            selectedRange: sheet.selectedRange,
            selectedCellCount: sheet.selectedCellCount,
            rangeValues: rangeValues,
            descriptors: descriptors,
            tsv: sheet.selectionAsTsv()
        })
        rangeReady = selectedRange
            && sheet.selectedCellCount === 4
            && rangeValues.length === 2
            && rangeValues[0][0] === "Beta"
            && rangeValues[0][1] === 2
            && rangeValues[1][0] === "Alpha"
            && rangeValues[1][1] === 10
            && descriptors.length === 4
            && descriptors[0].address === "A1"
            && descriptors[3].address === "B2"
            && sheet.selectionAsTsv() === "Beta\t2\nAlpha\t10"

        const quotedValues = [["A\tB", "C\"D"], ["E\nF", ""]]
        const quotedTsv = sheet.valuesAsTsv(quotedValues)
        const parsedQuoted = sheet.parseTsv(quotedTsv)
        tsvReady = quotedTsv === "\"A\tB\"\t\"C\"\"D\"\n\"E\nF\"\t"
            && sheet.valuesAsTsv(["One", 2]) === "One\t2"
            && sheet.matrixDimensions(["One", 2]).rowCount === 1
            && sheet.matrixDimensions(["One", 2]).columnCount === 2
            && parsedQuoted.length === 2
            && parsedQuoted[0].length === 2
            && parsedQuoted[0][0] === "A\tB"
            && parsedQuoted[0][1] === "C\"D"
            && parsedQuoted[1][0] === "E\nF"
            && parsedQuoted[1][1] === ""

        const depthBeforePaste = sheet.undoDepth
        pasteReady = sheet.pasteTsv(1, 0, "Delta\t7\ttrue\nEpsilon\t8\tfalse")
            && sheet.undoDepth === depthBeforePaste + 1
            && sheet.cellRawValue(1, 0) === "Delta"
            && sheet.cellRawValue(1, 1) === 7
            && sheet.cellRawValue(1, 2) === true
            && sheet.cellRawValue(2, 0) === "Epsilon"
            && sheet.cellRawValue(2, 1) === 8
            && sheet.cellRawValue(2, 2) === false
            && sheet.selectedRange.startRow === 1
            && sheet.selectedRange.endRow === 2
            && sheet.selectedRange.startColumn === 0
            && sheet.selectedRange.endColumn === 2

        pasteUndoReady = sheet.undo()
            && sheet.cellRawValue(1, 0) === "Alpha"
            && sheet.cellRawValue(2, 0) === "Gamma"
            && sheet.redo()
            && sheet.cellRawValue(1, 0) === "Delta"
            && sheet.cellRawValue(2, 0) === "Epsilon"

        const depthBeforeReject = sheet.undoDepth
        validationReady = !sheet.setRangeValues(0, 1, [["not-an-int"]])
            && sheet.cellRawValue(0, 1) === 2
            && sheet.undoDepth === depthBeforeReject
            && !sheet.pasteTsv(2, 2, "true\textra")

        const depthBeforeSort = sheet.undoDepth
        sortingReady = sheet.sortByColumn(1, Qt.DescendingOrder)
            && sheet.sortColumn === 1
            && sheet.sortOrder === Qt.DescendingOrder
            && sheet.undoDepth === depthBeforeSort + 1
            && sheet.cellRawValue(0, 0) === "Epsilon"
            && sheet.cellRawValue(0, 1) === 8
            && sheet.cellRawValue(1, 0) === "Delta"
            && sheet.cellRawValue(2, 0) === "Beta"
            && root.sortedSignalCount === 1

        sortUndoReady = sheet.undo()
            && sheet.cellRawValue(0, 0) === "Beta"
            && sheet.cellRawValue(1, 0) === "Delta"
            && sheet.cellRawValue(2, 0) === "Epsilon"

        blankSortReady = blankSheet.sortDescending(1)
            && blankSheet.cellRawValue(0, 0) === "Alpha"
            && blankSheet.cellRawValue(1, 0) === "Zed"
            && blankSheet.cellRawValue(2, 0) === "Blank"

        sheet.selectColumn(2)
        const columnSelectionReady = sheet.selectedCellCount === 3
        sheet.selectRow(0)
        const rowSelectionReady = sheet.selectedCellCount === 3
        sheet.selectAll()
        const allSelectionReady = sheet.selectedCellCount === 9
        sheet.selectCell(0, 0)
        navigationReady = sheet.moveCurrentCell(1, 1, false)
            && sheet.currentCell.address === "B2"
            && sheet.selectedCellCount === 1
            && columnSelectionReady
            && rowSelectionReady
            && allSelectionReady

        sheet.clearSelection()
        navigationReady = navigationReady
            && !sheet.hasSelection
            && sheet.currentRow === -1
            && sheet.currentColumn === -1
            && root.selectionSignalCount >= 7
    }

    Component.onCompleted: Qt.callLater(runContract)

    property bool spreadsheetContractReady:
        aliasesReady
        && addressReady
        && selectionReady
        && rangeReady
        && pasteReady
        && pasteUndoReady
        && validationReady
        && sortingReady
        && sortUndoReady
        && navigationReady
        && tsvReady
        && blankSortReady

    property string spreadsheetDiagnostics: JSON.stringify({
        aliasesReady: aliasesReady,
        addressReady: addressReady,
        selectionReady: selectionReady,
        rangeReady: rangeReady,
        pasteReady: pasteReady,
        pasteUndoReady: pasteUndoReady,
        validationReady: validationReady,
        sortingReady: sortingReady,
        sortUndoReady: sortUndoReady,
        navigationReady: navigationReady,
        tsvReady: tsvReady,
        blankSortReady: blankSortReady,
        counts: [sheet.rowCount, sheet.columnCount, sheet.headerCount],
        current: [sheet.currentRow, sheet.currentColumn],
        selectedRange: sheet.selectedRange,
        rangeDiagnostics: rangeDiagnostics,
        undoDepth: sheet.undoDepth,
        rows: sheet.rows
    })
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("spreadsheetContractReady").toBool(),
                 qPrintable(root->property("spreadsheetDiagnostics").toString()));
}

void ImportApiTests::table_viewport_reuses_delegates_and_creates_one_menu_on_demand()
{
    QQmlEngine engine;
    engine.addImportPath(QDir::cleanPath(QCoreApplication::applicationDirPath() + "/.."));
    QScopedPointer<QObject> root(createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.Table {
    id: table
    width: 640
    columns: ["Name", "Value"]
    rows: []
    viewportHeight: 144
    resizeHandlesVisible: false
    deleteContextMenuEnabled: false
    property int created: 0
    cellDelegate: Component {
        Item {
            required property var modelData
            objectName: "reusableCell"
            Component.onCompleted: table.created++
        }
    }
    Component.onCompleted: {
        const values = []
        for (let i = 0; i < 2000; ++i) values.push([String(i), i])
        rows = values
    }
})"));
    CTEST_CHECK(root);
    const auto count = [&] { return engine.newQObject(root.data()).property("visibleCellItems").property("length").toInt(); };
    CTEST_WAIT_DEFAULT(count() > 0);
    auto *table = qobject_cast<QQuickItem *>(root.data()); CTEST_CHECK(table);
    CTEST_WAIT_DEFAULT(visualChildByObjectName(table, "reusableCell"));
    CTEST_CHECK(engine.newQObject(root.data()).property("_cellContextMenu").isNull());
    QPointer<QObject> first = visualChildByObjectName(table, "reusableCell"); CTEST_CHECK(first);
    const auto initial = count();
    const auto created = root->property("created").toInt();
    root->setProperty("viewportY", 2400);
    CTEST_WAIT_DEFAULT(engine.newQObject(first).property("modelData").property("rowIndex").toInt() > 80);
    IMPORT_COMPARE(root->property("created").toInt(), created + qMax(0, count() - initial));
    CTEST_CHECK(first);
    root->setProperty("deleteContextMenuEnabled", true);
    QVariant one, two;
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "cellContextMenu", Q_RETURN_ARG(QVariant, one)));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "cellContextMenu", Q_RETURN_ARG(QVariant, two)));
    const auto menu = engine.newQObject(root.data()).property("_cellContextMenu").toQObject();
    CTEST_CHECK(menu);
    IMPORT_COMPARE(one.value<QObject *>(), menu);
    IMPORT_COMPARE(two.value<QObject *>(), menu);
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
        objectName: "singleCell"
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
        && row.dividerColor === LV.Theme.panelBackground10
        && rowEditedColumn === 1
        && rowEditedValue === "Active v2"
        && rowSubmittedColumn === 2
        && rowSubmittedValue === "Core v2"
        && table.resolvedHeaderCount === 3
        && table.rowDividerColor === LV.Theme.panelBackground10
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
    CTEST_CHECK(root);
    CTEST_CHECK(root->property("tableCellContractReady").toBool());

    CTEST_WAIT_DEFAULT(root->findChild<QObject *>(QStringLiteral("singleCell_inputField"),
                                          Qt::FindChildrenRecursively));
    QObject *cellInput = root->findChild<QObject *>(QStringLiteral("singleCell_inputField"),
                                                    Qt::FindChildrenRecursively);
    IMPORT_COMPARE(cellInput->property("fieldMinHeight").toInt(), 13);
    IMPORT_COMPARE(cellInput->property("centeredTextHeight").toInt(), 13);
    QObject *cellNativeInput = cellInput->property("inputItem").value<QObject *>();
    CTEST_CHECK(cellNativeInput);
    IMPORT_COMPARE(cellNativeInput->property("height").toReal(), 13.0);
    IMPORT_COMPARE(cellNativeInput->property("font").value<QFont>().pixelSize(), 13);
}

void ImportApiTests::table_cell_merge_split_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property bool mergeOk: false
    property bool splitCoveredOk: false
    property bool blockMergeOk: false
    property bool splitAnchorOk: false
    property bool invalidMergeRejected: false
    property int mergedSignalCount: 0
    property int splitSignalCount: 0
    property int initialVisibleCount: -1
    property int afterMergeVisibleCount: -1
    property int afterSplitVisibleCount: -1
    property int afterBlockMergeVisibleCount: -1
    property int afterBlockSplitVisibleCount: -1
    property real mergedWidth: -1
    property real blockMergedHeight: -1
    property bool mergeMarkerReady: false
    property bool splitMarkerCleared: false
    property bool staticSpanReady: false

    LV.Table {
        id: table
        visible: false
        width: 300
        height: 96
        structureControlsVisible: false
        headerColumns: ["A", "B", "C"]
        rows: [
            [{ text: "A1" }, { text: "B1" }, { text: "C1" }],
            [{ text: "A2" }, { text: "B2" }, { text: "C2" }]
        ]
        onCellsMerged: function(rowIndex, columnIndex, rowSpan, columnSpan) {
            if (rowIndex === 0 && columnIndex === 0 && rowSpan >= 1 && columnSpan >= 2)
                root.mergedSignalCount += 1
        }
        onCellSplit: function(rowIndex, columnIndex) {
            if (rowIndex === 0 && columnIndex === 0)
                root.splitSignalCount += 1
        }
    }

    LV.Table {
        id: staticSpanTable
        visible: false
        width: 300
        height: 96
        structureControlsVisible: false
        headerColumns: ["A", "B", "C"]
        rows: [
            [{ text: "S1", columnSpan: 2 }, { text: "S2" }, { text: "S3" }],
            [{ text: "S4", rowSpan: 2 }, { text: "S5" }, { text: "S6" }],
            [{ text: "S7" }, { text: "S8" }, { text: "S9" }]
        ]
    }

    function runContract() {
        initialVisibleCount = table.visibleCellItems.length
        mergeOk = table.mergeCells(0, 0, 1, 2)
        afterMergeVisibleCount = table.visibleCellItems.length
        mergedWidth = table.visibleCellItems[0].width
        mergeMarkerReady = table.visibleCellItems[0].columnSpan === 2
            && table.visibleCellItems[0].rowSpan === 1
            && table.isCoveredCell(0, 1)
            && table.rows[0][1]._lvrsMerged === true
            && table.rows[0][1]._lvrsMergeAnchorRow === 0
            && table.rows[0][1]._lvrsMergeAnchorColumn === 0

        splitCoveredOk = table.splitCell(0, 1)
        afterSplitVisibleCount = table.visibleCellItems.length
        splitMarkerCleared = table.visibleCellItems[0].columnSpan === 1
            && !table.isCoveredCell(0, 1)
            && table.rows[0][1]._lvrsMerged === undefined

        blockMergeOk = table.mergeCells(0, 0, 2, 2)
        afterBlockMergeVisibleCount = table.visibleCellItems.length
        blockMergedHeight = table.visibleCellItems[0].height
        splitAnchorOk = table.splitCell(0, 0)
        afterBlockSplitVisibleCount = table.visibleCellItems.length
        invalidMergeRejected = !table.mergeCells(0, 2, 1, 3)

        staticSpanReady = staticSpanTable.visibleCellItems.length === 7
            && staticSpanTable.visibleCellItems[0].columnSpan === 2
            && Math.abs(staticSpanTable.visibleCellItems[0].width - 200) < 0.01
            && staticSpanTable.isCoveredCell(0, 1)
            && staticSpanTable.isCoveredCell(2, 0)
            && staticSpanTable.splitCell(2, 0)
            && staticSpanTable.visibleCellItems.length === 8
            && !staticSpanTable.isCoveredCell(2, 0)
    }

    Component.onCompleted: Qt.callLater(runContract)

    property bool mergeSplitReady:
        initialVisibleCount === 6
        && mergeOk
        && afterMergeVisibleCount === 5
        && Math.abs(mergedWidth - 200) < 0.01
        && mergeMarkerReady
        && splitCoveredOk
        && afterSplitVisibleCount === 6
        && splitMarkerCleared
        && blockMergeOk
        && afterBlockMergeVisibleCount === 3
        && Math.abs(blockMergedHeight - (table.rowHeight * 2)) < 0.01
        && splitAnchorOk
        && afterBlockSplitVisibleCount === 6
        && invalidMergeRejected
        && mergedSignalCount === 2
        && splitSignalCount === 2
        && staticSpanReady
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("mergeSplitReady").toBool());
}

void ImportApiTests::table_structure_editing_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property int rowInsertedCount: 0
    property int rowDeletedCount: 0
    property int columnInsertedCount: 0
    property int columnDeletedCount: 0
    property bool appendRowOk: false
    property bool insertRowOk: false
    property bool appendColumnOk: false
    property bool insertColumnOk: false
    property bool deleteRowOk: false
    property bool deleteColumnOk: false
    property bool contextRowDeleteOk: false
    property bool contextColumnDeleteOk: false
    property bool oneColumnDeleteRejected: false
    property bool spanNormalizedOnMutation: false
    property bool contextItemsReady: false
    property bool nonCellContextRejected: false
    property bool initialControlsReady: false
    property bool headerMutationReady: false

    LV.Table {
        id: table
        visible: false
        width: 320
        height: 140
        structureControlsVisible: true
        headerColumns: ["A", "B"]
        rows: [
            [{ text: "A1" }, { text: "B1" }],
            [{ text: "A2" }, { text: "B2" }]
        ]

        onRowInserted: function(rowIndex) {
            root.rowInsertedCount += 1
        }
        onRowDeleted: function(rowIndex) {
            root.rowDeletedCount += 1
        }
        onColumnInserted: function(columnIndex) {
            root.columnInsertedCount += 1
        }
        onColumnDeleted: function(columnIndex) {
            root.columnDeletedCount += 1
        }
    }

    LV.Table {
        id: oneColumnTable
        visible: false
        width: 120
        height: 80
        headerColumns: ["Only"]
        rows: [[{ text: "Only" }]]
    }

    LV.Table {
        id: spanTable
        visible: false
        width: 320
        height: 120
        headerColumns: ["A", "B"]
        rows: [
            [{ text: "A1" }, { text: "B1" }],
            [{ text: "A2" }, { text: "B2" }]
        ]
    }

    function runContract() {
        initialControlsReady = table.structureMutationAvailable
            && table.resolvedStructureControlsVisible
            && table.resolvedStructureGutterWidth === table.structureGutterWidth
            && table.resolvedStructureGutterHeight === table.structureGutterHeight
            && Math.abs(table.structureCellWidth() - 150) < 0.01

        appendRowOk = table.appendRow()
            && table.rows.length === 3
            && table.rows[2].length === 2
            && table.rows[2][0].text === table.defaultCellText

        insertRowOk = table.insertRow(1)
            && table.rows.length === 4
            && table.rows[1].length === 2

        appendColumnOk = table.appendColumn()
            && table.resolvedColumnCount === 3
            && table.rows[0].length === 3
            && table.headerColumns.length === 3

        insertColumnOk = table.insertColumn(1)
            && table.resolvedColumnCount === 4
            && table.rows[0].length === 4
            && table.rows[0][1].text === table.defaultCellText
            && table.headerColumns[1].label === table.defaultHeaderText

        deleteRowOk = table.deleteRow(0)
            && table.rows.length === 3

        deleteColumnOk = table.deleteColumn(1)
            && table.resolvedColumnCount === 3
            && table.headerColumns.length === 3

        headerMutationReady = table.headerColumns.length === table.resolvedColumnCount

        const bothItems = table.buildContextMenuItems(1, 1)
        contextItemsReady = bothItems.length === 3
            && bothItems[0].label === "Delete row"
            && bothItems[2].label === "Delete column"

        nonCellContextRejected = table.buildContextMenuItems(-1, 1).length === 0
            && table.buildContextMenuItems(1, -1).length === 0

        const rowsBeforeContextDelete = table.rows.length
        const rowItems = table.buildContextMenuItems(1, 1)
        rowItems[0].onTriggered({})
        contextRowDeleteOk = table.rows.length === rowsBeforeContextDelete - 1

        const columnsBeforeContextDelete = table.resolvedColumnCount
        const columnItems = table.buildContextMenuItems(0, 0)
        columnItems[2].onTriggered({})
        contextColumnDeleteOk = table.resolvedColumnCount === columnsBeforeContextDelete - 1

        oneColumnDeleteRejected = !oneColumnTable.deleteColumn(0)

        spanTable.mergeCells(0, 0, 1, 2)
        const coveredBeforeAppend = spanTable.isCoveredCell(0, 1)
        spanTable.appendRow()
        spanNormalizedOnMutation = coveredBeforeAppend
            && !spanTable.isCoveredCell(0, 1)
            && spanTable.rows[0][0].rowSpan === 1
            && spanTable.rows[0][0].columnSpan === 1
    }

    Component.onCompleted: Qt.callLater(runContract)

    property bool structureEditingReady:
        initialControlsReady
        && appendRowOk
        && insertRowOk
        && appendColumnOk
        && insertColumnOk
        && deleteRowOk
        && deleteColumnOk
        && contextItemsReady
        && nonCellContextRejected
        && contextRowDeleteOk
        && contextColumnDeleteOk
        && oneColumnDeleteRejected
        && spanNormalizedOnMutation
        && headerMutationReady
        && rowInsertedCount === 2
        && rowDeletedCount === 2
        && columnInsertedCount === 2
        && columnDeletedCount === 2
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("structureEditingReady").toBool());
}

void ImportApiTests::table_resize_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property int columnResizeCount: 0
    property int rowResizeCount: 0
    property bool initialGeometryReady: false
    property bool columnDragReady: false
    property bool rowDragReady: false
    property bool minClampReady: false
    property bool headerGeometryReady: false
    property bool structureSizeShiftReady: false

    LV.Table {
        id: table
        visible: false
        width: 300
        height: 160
        structureControlsVisible: false
        minColumnWidth: 40
        minRowHeight: 20
        headerColumns: ["A", "B", "C"]
        columnWidths: [80, 100, 120]
        rowHeights: [24, 30]
        rows: [
            [{ text: "A1" }, { text: "B1" }, { text: "C1" }],
            [{ text: "A2" }, { text: "B2" }, { text: "C2" }]
        ]
        onColumnResized: function(columnIndex, width) {
            root.columnResizeCount += 1
        }
        onRowResized: function(rowIndex, height) {
            root.rowResizeCount += 1
        }
    }

    LV.TableHeader {
        id: header
        visible: false
        width: 300
        minColumnWidth: 40
        columnWidths: [70, 90, 140]
        cellItems: [
            { label: "A" },
            { label: "B" },
            { label: "C" }
        ]
    }

    LV.Table {
        id: shiftTable
        visible: false
        width: 300
        height: 160
        structureControlsVisible: false
        headerColumns: ["A", "B"]
        columnWidths: [75, 125]
        rowHeights: [26, 34]
        rows: [
            [{ text: "A1" }, { text: "B1" }],
            [{ text: "A2" }, { text: "B2" }]
        ]
    }

    function runContract() {
        initialGeometryReady = table.columnWidth(0) === 80
            && table.columnWidth(1) === 100
            && table.columnX(2) === 180
            && table.rowHeightAt(1) === 30
            && table.rowY(1) === 24
            && table.totalBodyHeight() === 54
            && table.visibleCellItems[1].x === 80
            && table.visibleCellItems[1].width === 100
            && table.visibleCellItems[3].y === 24
            && table.visibleCellItems[3].height === 30

        columnDragReady = table.beginColumnResize(1, 100)
            && table.updateColumnResize(135)
            && table.resizingColumnIndex === 1
            && table.columnWidths[1] === 135
        table.endColumnResize()
        columnDragReady = columnDragReady && table.resizingColumnIndex === -1

        rowDragReady = table.beginRowResize(0, 20)
            && table.updateRowResize(32)
            && table.resizingRowIndex === 0
            && table.rowHeights[0] === 36
        table.endRowResize()
        rowDragReady = rowDragReady && table.resizingRowIndex === -1

        minClampReady = table.setColumnWidth(2, 1)
            && table.columnWidths[2] === 40
            && table.setRowHeight(1, 1)
            && table.rowHeights[1] === 20

        headerGeometryReady = header.columnWidth(0) === 70
            && header.columnWidth(1) === 90
            && header.columnX(2) === 160

        shiftTable.insertColumn(1)
        shiftTable.insertRow(1)
        structureSizeShiftReady = shiftTable.columnWidths.length === 3
            && shiftTable.columnWidths[0] === 75
            && shiftTable.columnWidths[2] === 125
            && shiftTable.rowHeights.length === 3
            && shiftTable.rowHeights[0] === 26
            && shiftTable.rowHeights[2] === 34
        shiftTable.deleteColumn(0)
        shiftTable.deleteRow(0)
        structureSizeShiftReady = structureSizeShiftReady
            && shiftTable.columnWidths.length === 2
            && shiftTable.rowHeights.length === 2
    }

    Component.onCompleted: Qt.callLater(runContract)

    property bool resizeContractReady:
        initialGeometryReady
        && columnDragReady
        && rowDragReady
        && minClampReady
        && headerGeometryReady
        && structureSizeShiftReady
        && columnResizeCount === 2
        && rowResizeCount === 2
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("resizeContractReady").toBool());
}

void ImportApiTests::table_typed_header_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property bool objectHeaderTypesReady: false
    property bool primitiveHeaderTypesReady: false
    property bool cellCoercionReady: false
    property bool setCellValueReady: false
    property bool typedDefaultsReady: false
    property bool injectedCellGuardReady: false
    property int rejectedCount: 0

    LV.Table {
        id: typedTable
        visible: false
        width: 360
        height: 120
        structureControlsVisible: false
        inputable: true
        headerCellItems: [
            { label: "Name", type: "string" },
            { label: "Count", valueType: "int" },
            { label: "Ratio", cellType: "float" },
            { label: "Enabled", dataType: "bool" }
        ]
        rows: [
            [
                { value: "Renderer" },
                { value: 3 },
                { value: 1.5 },
                { value: true }
            ]
        ]
        onCellInputRejected: function(rowIndex, columnIndex, text, valueType) {
            root.rejectedCount += 1
        }
    }

    LV.Table {
        id: primitiveTable
        visible: false
        width: 360
        height: 120
        structureControlsVisible: false
        headerCellItems: ["Title", 1, 1.25, false]
        rows: [["Renderer", 3, 1.5, true]]
    }

    LV.TableHeader {
        id: primitiveHeader
        visible: false
        width: 360
        cellItems: ["Title", 1, 1.25, false]
    }

    LV.TableCellItem {
        id: intCell
        visible: false
        valueType: "int"
        valueValidator: function(value, valueType) {
            return typedTable.coerceCellValue(value, valueType)
        }
        onInputRejected: function(text, valueType) {
            root.rejectedCount += 1
        }
    }

    function runContract() {
        objectHeaderTypesReady = typedTable.headerCellType(0) === "string"
            && typedTable.headerCellType(1) === "int"
            && typedTable.headerCellType(2) === "float"
            && typedTable.headerCellType(3) === "bool"
            && typedTable.columnType(2) === "float"
            && typedTable.cellText(0, 0) === "Renderer"
            && typedTable.cellText(0, 1) === "3"
            && typedTable.cellText(0, 2) === "1.5"
            && typedTable.cellText(0, 3) === "true"
            && typedTable.visibleCellItems[1].valueType === "int"
            && typedTable.visibleCellItems[2].valueType === "float"
            && typedTable.visibleCellItems[3].valueType === "bool"

        primitiveHeaderTypesReady = primitiveTable.headerCellType(0) === "string"
            && primitiveTable.headerCellType(1) === "int"
            && primitiveTable.headerCellType(2) === "float"
            && primitiveTable.headerCellType(3) === "bool"
            && primitiveHeader.columnType(0) === "string"
            && primitiveHeader.columnType(1) === "int"
            && primitiveHeader.columnType(2) === "float"
            && primitiveHeader.columnType(3) === "bool"
            && primitiveHeader.columnText(3) === "false"

        const coercedInt = typedTable.coerceCellValue("42", "int")
        const rejectedInt = typedTable.coerceCellValue("42.5", "int")
        const coercedFloat = typedTable.coerceCellValue("4.25", "float")
        const coercedBool = typedTable.coerceCellValue("false", "bool")
        cellCoercionReady = coercedInt.accepted
            && coercedInt.value === 42
            && coercedInt.text === "42"
            && !rejectedInt.accepted
            && coercedFloat.accepted
            && coercedFloat.value === 4.25
            && coercedBool.accepted
            && coercedBool.value === false
            && coercedBool.text === "false"

        const intSetOk = typedTable.setCellValue(0, 1, "42")
        const intRejected = !typedTable.setCellValue(0, 1, "42.5")
        const floatSetOk = typedTable.setCellValue(0, 2, "4.25")
        const boolSetOk = typedTable.setCellValue(0, 3, "false")
        setCellValueReady = intSetOk
            && intRejected
            && floatSetOk
            && boolSetOk
            && typedTable.rows[0][1].value === 42
            && typedTable.rows[0][1].text === "42"
            && typedTable.rows[0][2].value === 4.25
            && typedTable.rows[0][2].text === "4.25"
            && typedTable.rows[0][3].value === false
            && typedTable.rows[0][3].text === "false"

        typedDefaultsReady = typedTable.appendRow()
            && typedTable.rows[1][0].text === typedTable.defaultCellText
            && typedTable.rows[1][1].value === 0
            && typedTable.rows[1][2].value === 0
            && typedTable.rows[1][3].value === false

        const acceptResult = intCell.applyInputResult("7")
        const rejectResult = intCell.applyInputResult("7.5")
        injectedCellGuardReady = acceptResult === "7"
            && rejectResult === "7"
            && intCell.inputResult === "7"
            && intCell.typedValue === 7
            && intCell.inputAccepted === false
            && root.rejectedCount === 1
    }

    Component.onCompleted: Qt.callLater(runContract)

    property bool typedHeaderContractReady:
        objectHeaderTypesReady
        && primitiveHeaderTypesReady
        && cellCoercionReady
        && setCellValueReady
        && typedDefaultsReady
        && injectedCellGuardReady
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("typedHeaderContractReady").toBool());
}

void ImportApiTests::table_cpp_model_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    NativeTableModel nativeTableModel;
    QQmlContext context(engine.rootContext());
    context.setContextProperty(QStringLiteral("nativeTableModel"), &nativeTableModel);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property bool displayReady: false
    property bool editingReady: false

    LV.Table {
        id: displayTable
        visible: false
        width: 240
        height: 96
        structureControlsVisible: false
        rows: nativeTableModel
    }

    LV.Table {
        id: editTable
        visible: false
        width: 240
        height: 96
        structureControlsVisible: false
        inputable: true
        headerCellItems: [
            { label: "Name", type: "string" },
            { label: "Count", type: "int" }
        ]
        rows: nativeTableModel
    }

    function runContract() {
        displayReady = displayTable.rowsModelBacked
            && displayTable.resolvedRowCount === 2
            && displayTable.resolvedColumnCount === 2
            && displayTable.resolvedHeaderCount === 2
            && displayTable.headerAt(0).label === "Name"
            && displayTable.headerAt(1).label === "Count"
            && displayTable.cellText(0, 0) === "Renderer"
            && displayTable.cellText(0, 1) === "3"

        const editOk = editTable.setCellValue(0, 1, "11")
        const rejectOk = !editTable.setCellValue(0, 1, "11.5")
        editingReady = editTable.rowsModelBacked
            && editTable.cellEditingAvailable
            && editTable.cellText(0, 1) === "11"
            && displayTable.cellText(0, 1) === "11"
            && editTable.rows === nativeTableModel
            && editOk
            && rejectOk
    }

    Component.onCompleted: Qt.callLater(runContract)

    property bool cppModelContractReady: displayReady && editingReady
}
)";

    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QScopedPointer<QObject> root(component.create(&context));
    if (component.isError()) {
        const auto errors = component.errors();
        for (const auto &err : errors)
            qWarning() << err;
    }
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("cppModelContractReady").toBool());
}

void ImportApiTests::table_undo_redo_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    property bool cellUndoRedoReady: false
    property bool structureUndoRedoReady: false
    property bool mergeUndoRedoReady: false
    property bool resizeUndoRedoReady: false
    property bool clearReady: false

    LV.Table {
        id: table
        visible: false
        width: 320
        height: 120
        structureControlsVisible: false
        headerCellItems: [
            { label: "Name", type: "string" },
            { label: "Count", type: "int" }
        ]
        rows: [
            [{ value: "Renderer" }, { value: 3 }],
            [{ value: "Writer" }, { value: 2 }]
        ]
    }

    function runContract() {
        const initialUndoReady = !table.canUndo && !table.canRedo && table.undoDepth === 0 && table.redoDepth === 0
        const editOk = table.setCellValue(0, 1, "9")
        const undoCellOk = table.canUndo
            && table.undoDepth === 1
            && table.rows[0][1].value === 9
            && table.undo()
            && table.rows[0][1].value === 3
            && table.canRedo
            && table.redo()
            && table.rows[0][1].value === 9
            && !table.canRedo
        cellUndoRedoReady = initialUndoReady && editOk && undoCellOk

        const insertRowOk = table.insertRow(1)
        const insertColumnOk = table.insertColumn(1)
        structureUndoRedoReady = insertRowOk
            && insertColumnOk
            && table.rows.length === 3
            && table.resolvedColumnCount === 3
            && table.undo()
            && table.resolvedColumnCount === 2
            && table.rows.length === 3
            && table.undo()
            && table.rows.length === 2
            && table.redo()
            && table.rows.length === 3
            && table.redo()
            && table.resolvedColumnCount === 3

        mergeUndoRedoReady = table.mergeCells(0, 0, 1, 2)
            && table.isCoveredCell(0, 1)
            && table.undo()
            && !table.isCoveredCell(0, 1)
            && table.redo()
            && table.isCoveredCell(0, 1)

        const widthBeforeResize = table.columnWidth(0)
        const heightBeforeResize = table.rowHeightAt(0)
        resizeUndoRedoReady = table.setColumnWidth(0, widthBeforeResize + 24)
            && table.columnWidth(0) === widthBeforeResize + 24
            && table.undo()
            && table.columnWidth(0) === widthBeforeResize
            && table.redo()
            && table.columnWidth(0) === widthBeforeResize + 24
            && table.setRowHeight(0, heightBeforeResize + 12)
            && table.rowHeightAt(0) === heightBeforeResize + 12
            && table.undo()
            && table.rowHeightAt(0) === heightBeforeResize

        table.clearUndoStack()
        clearReady = !table.canUndo && !table.canRedo && table.undoDepth === 0 && table.redoDepth === 0
    }

    Component.onCompleted: Qt.callLater(runContract)

    property bool undoRedoContractReady:
        cellUndoRedoReady
        && structureUndoRedoReady
        && mergeUndoRedoReady
        && resizeUndoRedoReady
        && clearReady
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("undoRedoContractReady").toBool());
}

void ImportApiTests::list_model_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);

    NativeListModel nativeModel;
    engine.rootContext()->setContextProperty(QStringLiteral("nativeListModel"), &nativeModel);

    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root

    ListModel {
        id: qmlRows
        ListElement { label: "QML A"; enabled: true; selected: false }
        ListElement { label: "QML B"; enabled: false; selected: true }
    }

    LV.List {
        id: qmlModelList
        visible: false
        footerVisible: false
        model: qmlRows
    }

    LV.List {
        id: nativeModelList
        visible: false
        footerVisible: false
        model: nativeListModel
        labelRole: "name"
    }

    LV.List {
        id: primitiveModelList
        visible: false
        footerVisible: false
        selectedIndex: 2
        model: ["String", 7, true]
    }

    LV.List {
        id: itemsFallbackList
        visible: false
        footerVisible: false
        items: [{ label: "Legacy A" }, { label: "Legacy B" }]
    }

    LV.List {
        id: modelOverridesItemsList
        visible: false
        footerVisible: false
        items: [{ label: "Old" }]
        model: [{ label: "New" }]
    }

    property bool qmlModelReady:
        qmlModelList.usingModel
        && qmlModelList.entryCount === 2
        && qmlModelList.itemLabel(qmlModelList.entryAt(0)) === "QML A"
        && qmlModelList.itemLabel(qmlModelList.entryAt(1)) === "QML B"
        && qmlModelList.itemEnabled(qmlModelList.entryAt(0))
        && !qmlModelList.itemEnabled(qmlModelList.entryAt(1))
        && qmlModelList.itemSelected(qmlModelList.entryAt(1), 1)

    property bool nativeModelReady:
        nativeModelList.usingModel
        && nativeModelList.entryCount === 3
        && nativeModelList.entryAt(0).name === "Native A"
        && nativeModelList.entryAt(1).display === "Native B"
        && nativeModelList.itemLabel(nativeModelList.entryAt(0)) === "Native A"
        && !nativeModelList.itemEnabled(nativeModelList.entryAt(1))
        && nativeModelList.itemSelected(nativeModelList.entryAt(2), 2)
    property int nativeModelEntryCount: nativeModelList.entryCount

    property bool primitiveModelReady:
        primitiveModelList.usingModel
        && primitiveModelList.entryCount === 3
        && primitiveModelList.itemLabel(primitiveModelList.entryAt(0)) === "String"
        && primitiveModelList.itemLabel(primitiveModelList.entryAt(1)) === "7"
        && primitiveModelList.itemLabel(primitiveModelList.entryAt(2)) === "true"
        && primitiveModelList.itemSelected(primitiveModelList.entryAt(2), 2)

    property bool fallbackReady:
        !itemsFallbackList.usingModel
        && itemsFallbackList.entryCount === 2
        && itemsFallbackList.itemLabel(itemsFallbackList.entryAt(0)) === "Legacy A"
        && modelOverridesItemsList.usingModel
        && modelOverridesItemsList.entryCount === 1
        && modelOverridesItemsList.itemLabel(modelOverridesItemsList.entryAt(0)) === "New"

    property bool listModelContractReady:
        qmlModelReady
        && nativeModelReady
        && primitiveModelReady
        && fallbackReady
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("listModelContractReady").toBool());

    nativeModel.appendRow(QStringLiteral("Native D"), true, false);
    IMPORT_WAIT_COMPARE(root->property("nativeModelEntryCount").toInt(), 4);
}

void ImportApiTests::list_figma_contract_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    width: 800
    height: 600

    property bool listDefaultsCaptured: false
    property string listApplyResult: ""
    property int listEditedCount: 0
    property int listSubmittedCount: 0
    property string listEditedValue: ""
    property string listSubmittedValue: ""

    LV.List {
        id: smallList
        objectName: "figmaSmallList"
        width: implicitWidth
        height: implicitHeight
    }

    LV.ListItem {
        id: miniItem
        objectName: "figmaMiniItem"
        x: 200
        width: implicitWidth
        height: implicitHeight
    }

    LV.ListItem {
        id: detailItem
        objectName: "figmaDetailItem"
        x: 400
        size: LV.ListItem.Detail
        width: implicitWidth
        height: implicitHeight
    }

    LV.ListItem {
        id: editableItem
        objectName: "editableItem"
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
        objectName: "figmaListFooter"
        x: 200
        y: 140
        width: implicitWidth
        height: implicitHeight
    }

    Component.onCompleted: {
        listDefaultsCaptured = !editableItem.inputable
        editableItem.inputable = true
        listApplyResult = editableItem.applyInputResult("Label 2")
        editableItem.inputEdited("Label 3")
        editableItem.inputSubmitted("Label 4")
    }

    property bool contractReady:
        listDefaultsCaptured
        && listApplyResult === "Label 2"
        && editableItem.inputResult === "Label 2"
        && editableItem.label === "Label 2"
        && listEditedCount === 1
        && listSubmittedCount === 1
        && listEditedValue === "Label 3"
        && listSubmittedValue === "Label 4"
        && smallList.listWidth === 170
        && smallList.minimumListHeight === 140
        && smallList.itemHeight === 22
        && smallList.itemLabelLeftPadding === 4
        && smallList.selectedIndex === -1
        && smallList.defaultItemIconName === "nodesfolder"
        && smallList.entryCount === 6
        && smallList.contentHeight === 158
        && smallList.implicitWidth === 170
        && smallList.implicitHeight === 140
        && smallList.backgroundColor === LV.Theme.panelBackground03
        && smallList.footerButton1.iconName === "addFile"
        && smallList.footerButton2.iconName === "generaldelete"
        && smallList.footerButton3.type === "menu"
        && smallList.footerButton3.iconName === "settings"
        && miniItem.size === LV.ListItem.Mini
        && miniItem.label === "Label"
        && miniItem.iconName === "nodesfolder"
        && miniItem.rowHorizontalPadding === 4
        && miniItem.rowVerticalPadding === 2
        && miniItem.iconSize === 18
        && miniItem.implicitWidth === 170
        && miniItem.implicitHeight === 22
        && !miniItem.separatorVisible
        && detailItem.size === LV.ListItem.Detail
        && detailItem.detail === "asasdsadasasdsadasasd Maxinum lines: 2 lines"
        && detailItem.dateText === "YYYY-MM-dd"
        && detailItem.folderLabel1 === "Only"
        && detailItem.folderLabel2 === "1 Line"
        && detailItem.tagLabel1 === "Only"
        && detailItem.tagLabel2 === "1 Line"
        && detailItem.horizontalPadding === 12
        && detailItem.verticalPadding === 8
        && detailItem.implicitWidth === 194
        && detailItem.implicitHeight === 106
        && listFooter.button1.iconName === "addFile"
        && listFooter.button2.iconName === "generaldelete"
        && listFooter.button3.type === "menu"
        && listFooter.button3.iconName === "settings"
        && listFooter.horizontalPadding === 2
        && listFooter.verticalPadding === 2
        && listFooter.stockButtonPadding === 2
        && listFooter.stockButtonHeight === 22
        && listFooter.stockMenuButtonSpacing === -2
        && listFooter.implicitWidth === 88
        && listFooter.implicitHeight === 26

    property string contractDebug: JSON.stringify({
        listWidth: smallList.listWidth,
        listMinHeight: smallList.minimumListHeight,
        listItemHeight: smallList.itemHeight,
        listPadding: smallList.itemLabelLeftPadding,
        listSelectedIndex: smallList.selectedIndex,
        delegateDescriptorCount: smallList.itemDelegateItems.length,
        listContentHeight: smallList.contentHeight,
        listImplicitWidth: smallList.implicitWidth,
        listImplicitHeight: smallList.implicitHeight,
        miniWidth: miniItem.implicitWidth,
        miniHeight: miniItem.implicitHeight,
        detailWidth: detailItem.implicitWidth,
        detailHeight: detailItem.implicitHeight,
        footerWidth: listFooter.implicitWidth,
        footerHeight: listFooter.implicitHeight,
        footerButtonHeight: listFooter.stockButtonHeight,
        footerMenuSpacing: listFooter.stockMenuButtonSpacing,
        editedLabel: editableItem.label,
        editedResult: editableItem.inputResult
    })
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    IMPORT_WAIT_MESSAGE(root->property("contractReady").toBool(),
                 qPrintable(root->property("contractDebug").toString()));

    CTEST_WAIT_DEFAULT(root->findChild<QObject *>(QStringLiteral("editableItem_inputField"),
                                          Qt::FindChildrenRecursively));
    QObject *listInput = root->findChild<QObject *>(QStringLiteral("editableItem_inputField"),
                                                    Qt::FindChildrenRecursively);
    IMPORT_COMPARE(listInput->property("fieldMinHeight").toInt(), 13);
    IMPORT_COMPARE(listInput->property("centeredTextHeight").toInt(), 13);
    QObject *listNativeInput = listInput->property("inputItem").value<QObject *>();
    CTEST_CHECK(listNativeInput);
    IMPORT_COMPARE(listNativeInput->property("height").toReal(), 13.0);
    IMPORT_COMPARE(listNativeInput->property("font").value<QFont>().pixelSize(), 13);

    auto *smallList = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaSmallList")));
    CTEST_CHECK(smallList);
    auto *listViewport = qobject_cast<QQuickItem *>(
        smallList->findChild<QObject *>(QStringLiteral("list_itemsViewport")));
    auto *listColumn = qobject_cast<QQuickItem *>(
        smallList->findChild<QObject *>(QStringLiteral("list_itemsColumn")));
    auto *embeddedFooter = qobject_cast<QQuickItem *>(
        smallList->findChild<QObject *>(QStringLiteral("list_footer")));
    auto *firstDelegate = visualChildByObjectName(
        listColumn,
        QStringLiteral("list_delegateRoot_0"));
    auto *sixthDelegate = visualChildByObjectName(
        listColumn,
        QStringLiteral("list_delegateRoot_5"));
    CTEST_CHECK(listViewport);
    CTEST_CHECK(listColumn);
    CTEST_CHECK(embeddedFooter);
    CTEST_CHECK(firstDelegate);
    CTEST_CHECK(sixthDelegate);
    CTEST_CHECK(visualChildByObjectName(firstDelegate, QStringLiteral("listItem_miniIcon")));
    CTEST_CHECK(visualChildByObjectName(firstDelegate, QStringLiteral("listItem_miniLabel")));
    CTEST_CHECK(qAbs(listViewport->x() - 0.0) < 0.01);
    CTEST_CHECK(qAbs(listViewport->y() - 0.0) < 0.01);
    CTEST_CHECK(qAbs(listViewport->width() - 170.0) < 0.01);
    CTEST_CHECK(qAbs(listViewport->height() - 114.0) < 0.01);
    CTEST_CHECK(qAbs(listColumn->height() - 132.0) < 0.01);
    CTEST_CHECK(listViewport->clip());
    CTEST_CHECK(qAbs(firstDelegate->x() - 0.0) < 0.01);
    CTEST_CHECK(qAbs(firstDelegate->y() - 0.0) < 0.01);
    CTEST_CHECK(qAbs(firstDelegate->width() - 170.0) < 0.01);
    CTEST_CHECK(qAbs(firstDelegate->height() - 22.0) < 0.01);
    CTEST_CHECK(qAbs(sixthDelegate->y() - 110.0) < 0.01);
    CTEST_CHECK(qAbs(embeddedFooter->x() - 0.0) < 0.01);
    CTEST_CHECK(qAbs(embeddedFooter->y() - 114.0) < 0.01);
    CTEST_CHECK(qAbs(embeddedFooter->width() - 88.0) < 0.01);
    CTEST_CHECK(qAbs(embeddedFooter->height() - 26.0) < 0.01);

    const auto boundsIn = [](QQuickItem *item, QQuickItem *ancestor) {
        return QRectF(item->mapToItem(ancestor, QPointF(0.0, 0.0)),
                      QSizeF(item->width(), item->height()));
    };
    const auto verifyBounds = [](const QRectF &actual, const QRectF &expected) {
        CTEST_CHECK_MESSAGE(qAbs(actual.x() - expected.x()) < 0.01
                     && qAbs(actual.y() - expected.y()) < 0.01
                     && qAbs(actual.width() - expected.width()) < 0.01
                     && qAbs(actual.height() - expected.height()) < 0.01,
                 qPrintable(QStringLiteral("bounds actual=(%1,%2 %3x%4) expected=(%5,%6 %7x%8)")
                                .arg(actual.x()).arg(actual.y())
                                .arg(actual.width()).arg(actual.height())
                                .arg(expected.x()).arg(expected.y())
                                .arg(expected.width()).arg(expected.height())));
    };

    auto *miniItem = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaMiniItem")));
    CTEST_CHECK(miniItem);
    auto *miniIcon = qobject_cast<QQuickItem *>(
        miniItem->findChild<QObject *>(QStringLiteral("listItem_miniIcon")));
    auto *miniLabel = qobject_cast<QQuickItem *>(
        miniItem->findChild<QObject *>(QStringLiteral("listItem_miniLabel")));
    CTEST_CHECK(miniIcon);
    CTEST_CHECK(miniLabel);
    IMPORT_COMPARE(miniIcon->property("status").toInt(), 1);
    verifyBounds(boundsIn(miniIcon, miniItem), QRectF(4.0, 2.0, 18.0, 18.0));
    verifyBounds(boundsIn(miniLabel, miniItem), QRectF(23.0, 4.5, 33.0, 13.0));
    const QFont miniFont = miniLabel->property("font").value<QFont>();
    IMPORT_COMPARE(miniFont.pixelSize(), 13);
    IMPORT_COMPARE(miniFont.weight(), QFont::Medium);
    IMPORT_COMPARE(miniFont.styleName(), QStringLiteral("Medium"));

    auto *detailItem = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaDetailItem")));
    CTEST_CHECK(detailItem);
    const struct {
        const char *name;
        QRectF bounds;
    } detailBounds[] = {
        { "listItem_detailContent", QRectF(12.0, 8.0, 170.0, 90.0) },
        { "listItem_detailTop", QRectF(12.0, 8.0, 170.0, 24.0) },
        { "listItem_detailTitle", QRectF(12.0, 8.0, 142.0, 24.0) },
        { "listItem_detailBookmark", QRectF(164.0, 8.0, 18.0, 18.0) },
        { "listItem_detailMiddle", QRectF(12.0, 40.0, 170.0, 12.0) },
        { "listItem_detailDate", QRectF(12.0, 40.0, 78.0, 12.0) },
        { "listItem_detailBottom", QRectF(12.0, 60.0, 170.0, 38.0) },
        { "listItem_detailFolders", QRectF(12.0, 60.0, 170.0, 18.0) },
        { "listItem_detailTags", QRectF(12.0, 80.0, 170.0, 18.0) }
    };
    for (const auto &entry : detailBounds) {
        auto *item = qobject_cast<QQuickItem *>(
            detailItem->findChild<QObject *>(QString::fromLatin1(entry.name)));
        CTEST_CHECK_MESSAGE(item, entry.name);
        verifyBounds(boundsIn(item, detailItem), entry.bounds);
    }

    auto *detailTitle = detailItem->findChild<QObject *>(QStringLiteral("listItem_detailTitle"));
    auto *detailDate = detailItem->findChild<QObject *>(QStringLiteral("listItem_detailDate"));
    auto *caption = qobject_cast<QQuickItem *>(
        detailItem->findChild<QObject *>(QStringLiteral("listItem_folderLabel1")));
    auto *detailBookmark = detailItem->findChild<QObject *>(QStringLiteral("listItem_detailBookmark"));
    CTEST_CHECK(detailTitle);
    CTEST_CHECK(detailDate);
    CTEST_CHECK(caption);
    CTEST_CHECK(detailBookmark);
    IMPORT_COMPARE(detailBookmark->property("status").toInt(), 1);
    const struct {
        const char *name;
        const char *assetName;
    } metadataIcons[] = {
        { "listItem_folderIcon1", "folder@14x14" },
        { "listItem_folderIcon2", "folder@14x14" },
        { "listItem_tagIcon1", "vcscurrentBranch" },
        { "listItem_tagIcon2", "vcscurrentBranch" }
    };
    for (const auto &entry : metadataIcons) {
        auto *icon = detailItem->findChild<QObject *>(QString::fromLatin1(entry.name));
        CTEST_CHECK_MESSAGE(icon, entry.name);
        IMPORT_COMPARE(icon->property("status").toInt(), 1);
        CTEST_CHECK_MESSAGE(icon->property("source").toUrl().toString().contains(
                     QString::fromLatin1(entry.assetName)),
                 qPrintable(icon->property("source").toUrl().toString()));
    }
    verifyBounds(boundsIn(caption, detailItem), QRectF(38.0, 63.5, 23.0, 11.0));
    const QFont detailTitleFont = detailTitle->property("font").value<QFont>();
    const QFont detailDateFont = detailDate->property("font").value<QFont>();
    const QFont captionFont = caption->property("font").value<QFont>();
    IMPORT_COMPARE(detailTitleFont.pixelSize(), 12);
    IMPORT_COMPARE(detailTitleFont.weight(), QFont::DemiBold);
    IMPORT_COMPARE(detailTitleFont.styleName(), QStringLiteral("SemiBold"));
    IMPORT_COMPARE(detailDateFont.pixelSize(), 12);
    IMPORT_COMPARE(detailDateFont.weight(), QFont::DemiBold);
    IMPORT_COMPARE(captionFont.pixelSize(), 11);
    IMPORT_COMPARE(captionFont.weight(), QFont::Normal);
    IMPORT_COMPARE(captionFont.styleName(), QStringLiteral("Regular"));

    auto *footer = qobject_cast<QQuickItem *>(
        root->findChild<QObject *>(QStringLiteral("figmaListFooter")));
    CTEST_CHECK(footer);
    const QRectF slotBounds[] = {
        QRectF(2.0, 2.0, 22.0, 22.0),
        QRectF(24.0, 2.0, 22.0, 22.0),
        QRectF(46.0, 2.0, 40.0, 22.0)
    };
    for (int index = 0; index < 3; ++index) {
        auto *slot = visualChildByObjectName(
            footer,
            QStringLiteral("listFooter_slot_%1").arg(index));
        auto *iconButton = visualChildByObjectName(
            slot,
            QStringLiteral("listFooter_iconButton_%1").arg(index));
        auto *slotMenuButton = visualChildByObjectName(
            slot,
            QStringLiteral("listFooter_menuButton_%1").arg(index));
        CTEST_CHECK(slot);
        CTEST_CHECK(iconButton);
        CTEST_CHECK(slotMenuButton);
        verifyBounds(boundsIn(slot, footer), slotBounds[index]);
        IMPORT_COMPARE(iconButton->isVisible(), index < 2);
        IMPORT_COMPARE(slotMenuButton->isVisible(), index == 2);
    }
    QObject *menuButton = visualChildByObjectName(
        footer,
        QStringLiteral("listFooter_menuButton_2"));
    CTEST_CHECK(menuButton);
    IMPORT_COMPARE(menuButton->property("spacing").toInt(), -2);
    IMPORT_COMPARE(menuButton->property("resolvedIconName").toString(), QStringLiteral("settings"));
    IMPORT_COMPARE(menuButton->property("resolvedIndicatorName").toString(),
             QStringLiteral("generalchevronDownBorderless"));
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    const bool nativeSet = qEnvironmentVariable("LVRS_IMPORT_CONTRACT_SET") == "native" && argc == 1;
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    ImportApiTests tests;
    runner.run("embedded_qml_matches_source", [&] { pressedButtons = Qt::NoButton; tests.embedded_qml_matches_source(); });
    runner.run("app_bootstrap_window_loads", [&] { pressedButtons = Qt::NoButton; tests.app_bootstrap_window_loads(); });
    runner.run("versionless_import_application_window_loads", [&] { pressedButtons = Qt::NoButton; tests.versionless_import_application_window_loads(); });
    runner.run("application_window_mobile_coverage_visibility_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.application_window_mobile_coverage_visibility_contract_loads(); });
    runner.run("application_window_page_stack_state_loads", [&] { pressedButtons = Qt::NoButton; tests.application_window_page_stack_state_loads(); });
    runner.run("application_window_initial_properties_seed_page_stack", [&] { pressedButtons = Qt::NoButton; tests.application_window_initial_properties_seed_page_stack(); });
    runner.run("application_window_safe_margin_scopes_to_layout_not_render_surface", [&] { pressedButtons = Qt::NoButton; tests.application_window_safe_margin_scopes_to_layout_not_render_surface(); });
    runner.run("versionless_import_window_loads", [&] { pressedButtons = Qt::NoButton; tests.versionless_import_window_loads(); });
    runner.run("window_custom_chrome_interaction_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.window_custom_chrome_interaction_contract_loads(); });
    runner.run("window_safe_margin_scopes_to_layout_not_render_surface", [&] { pressedButtons = Qt::NoButton; tests.window_safe_margin_scopes_to_layout_not_render_surface(); });
    runner.run("appshell_compat_loads", [&] { pressedButtons = Qt::NoButton; tests.appshell_compat_loads(); });
    runner.run("application_window_platform_adaptive_layout_loads", [&] { pressedButtons = Qt::NoButton; tests.application_window_platform_adaptive_layout_loads(); });
    runner.run("icon_name_mapping_loads", [&] { pressedButtons = Qt::NoButton; tests.icon_name_mapping_loads(); });
    runner.run("compact_icon_frame_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.compact_icon_frame_contract_loads(); });
    runner.run("hierarchy_tree_model_api_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_tree_model_api_loads(); });
    runner.run("hierarchy_string_array_model_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_string_array_model_loads(); });
    runner.run("hierarchy_direct_model_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_direct_model_contract_loads(); });
    runner.run("hierarchy_direct_model_editing_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_direct_model_editing_contract_loads(); });
    runner.run("hierarchy_nested_children_indent_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_nested_children_indent_contract_loads(); });
    runner.run("hierarchy_expansion_uses_incremental_visibility_refresh", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_expansion_uses_incremental_visibility_refresh(); });
    runner.run("hierarchy_editable_drag_depth_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_editable_drag_depth_contract_loads(); });
    runner.run("hierarchy_editable_drag_per_item_lock_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_editable_drag_per_item_lock_contract_loads(); });
    runner.run("hierarchy_mobile_drag_hold_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_mobile_drag_hold_contract_loads(); });
    runner.run("hierarchy_mobile_activation_commits_on_release_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_mobile_activation_commits_on_release_contract_loads(); });
    runner.run("hierarchy_mobile_reactivation_reemits_active_signal", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_mobile_reactivation_reemits_active_signal(); });
    runner.run("hierarchy_mobile_scroll_physics_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_mobile_scroll_physics_contract_loads(); });
    runner.run("hierarchy_optional_footer_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_optional_footer_contract_loads(); });
    runner.run("hierarchy_toolbar_empty_state_does_not_reschedule", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_toolbar_empty_state_does_not_reschedule(); });
    runner.run("hierarchy_toolbar_item_model_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_toolbar_item_model_contract_loads(); });
    runner.run("hierarchy_toolbar_figma_layout_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_toolbar_figma_layout_contract_loads(); });
    runner.run("hierarchy_figma_composition_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_figma_composition_contract_loads(); });
    runner.run("hierarchy_toolbar_manual_icon_button_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_toolbar_manual_icon_button_contract_loads(); });
    runner.run("hierarchy_row_click_only_activates_not_toggles", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_row_click_only_activates_not_toggles(); });
    runner.run("hierarchy_chevron_requires_children_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_chevron_requires_children_loads(); });
    runner.run("hierarchy_item_chevron_direction_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_item_chevron_direction_contract_loads(); });
    runner.run("hierarchy_item_hover_and_active_state_visual_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_item_hover_and_active_state_visual_contract_loads(); });
    runner.run("hierarchy_item_figma_defaults_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_item_figma_defaults_contract_loads(); });
    runner.run("hierarchy_item_layout_geometry_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_item_layout_geometry_contract_loads(); });
    runner.run("hierarchy_item_count_view_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_item_count_view_contract_loads(); });
    runner.run("hierarchy_item_structure_api_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_item_structure_api_contract_loads(); });
    runner.run("hierarchy_item_ux_state_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.hierarchy_item_ux_state_contract_loads(); });
    runner.run("button_padding_matches_figma_spec", [&] { pressedButtons = Qt::NoButton; tests.button_padding_matches_figma_spec(); });
    runner.run("button_default_tone_matches_figma_accent_loads", [&] { pressedButtons = Qt::NoButton; tests.button_default_tone_matches_figma_accent_loads(); });
    runner.run("segmented_button_selected_color", [&] { pressedButtons = Qt::NoButton; tests.segmented_button_selected_color(); });
    runner.run("button_injected_methods_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.button_injected_methods_contract_loads(); });
    runner.run("stepper_figma_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.stepper_figma_contract_loads(); });
    runner.run("combo_box_figma_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.combo_box_figma_contract_loads(); });
    runner.run("input_field_figma_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.input_field_figma_contract_loads(); });
    runner.run("input_field_material_rendering_contract", [&] { pressedButtons = Qt::NoButton; tests.input_field_material_rendering_contract(); });
    runner.run("progress_bar_range_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.progress_bar_range_contract_loads(); });
    runner.run("control_icons_use_supersampled_raster_contract", [&] { pressedButtons = Qt::NoButton; tests.control_icons_use_supersampled_raster_contract(); });
    runner.run("input_field_ios_native_text_interaction_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.input_field_ios_native_text_interaction_contract_loads(); });
    runner.run("input_field_native_event_input_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.input_field_native_event_input_contract_loads(); });
    runner.run("input_field_search_icon_mobile_scaling_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.input_field_search_icon_mobile_scaling_contract_loads(); });
    if (!nativeSet) runner.run("toggle_switch_figma_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.toggle_switch_figma_contract_loads(); });
    runner.run("checkbox_figma_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.checkbox_figma_contract_loads(); });
    runner.run("radio_button_figma_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.radio_button_figma_contract_loads(); });
    runner.run("modal_empty_frame_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.modal_empty_frame_contract_loads(); });
    runner.run("modal_content_action_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.modal_content_action_contract_loads(); });
    runner.run("alert_figma_variant_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.alert_figma_variant_contract_loads(); });
    runner.run("alert_content_arguments_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.alert_content_arguments_contract_loads(); });
    runner.run("alert_action_button_padding_scopes_to_alert", [&] { pressedButtons = Qt::NoButton; tests.alert_action_button_padding_scopes_to_alert(); });
    runner.run("menu_item_key_and_chevron_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.menu_item_key_and_chevron_contract_loads(); });
    runner.run("menu_item_icon_slot_switch_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.menu_item_icon_slot_switch_contract_loads(); });
    runner.run("context_menu_item_action_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.context_menu_item_action_contract_loads(); });
    runner.run("context_menu_visual_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.context_menu_visual_contract_loads(); });
    runner.run("context_menu_icon_slot_switch_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.context_menu_icon_slot_switch_contract_loads(); });
    runner.run("context_menu_width_expansion_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.context_menu_width_expansion_contract_loads(); });
    runner.run("context_menu_auto_placement_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.context_menu_auto_placement_contract_loads(); });
    runner.run("model_component_delegate_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.model_component_delegate_contract_loads(); });
    runner.run("table_figma_geometry_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.table_figma_geometry_contract_loads(); });
    runner.run("table_spreadsheet_api_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.table_spreadsheet_api_contract_loads(); });
    runner.run("table_viewport_reuses_delegates_and_creates_one_menu_on_demand", [&] { pressedButtons = Qt::NoButton; tests.table_viewport_reuses_delegates_and_creates_one_menu_on_demand(); });
    runner.run("table_cell_item_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.table_cell_item_contract_loads(); });
    runner.run("table_cell_merge_split_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.table_cell_merge_split_contract_loads(); });
    runner.run("table_structure_editing_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.table_structure_editing_contract_loads(); });
    runner.run("table_resize_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.table_resize_contract_loads(); });
    runner.run("table_typed_header_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.table_typed_header_contract_loads(); });
    runner.run("table_cpp_model_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.table_cpp_model_contract_loads(); });
    runner.run("table_undo_redo_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.table_undo_redo_contract_loads(); });
    runner.run("list_model_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.list_model_contract_loads(); });
    runner.run("list_figma_contract_loads", [&] { pressedButtons = Qt::NoButton; tests.list_figma_contract_loads(); });
    runner.run("button_family_components_contract:desktop", [&] { pressedButtons = Qt::NoButton; tests.button_family_components_contract(false); });
    runner.run("button_family_components_contract:mobile", [&] { pressedButtons = Qt::NoButton; tests.button_family_components_contract(true); });
    runner.run("segmented_control_figma_contract_loads:desktop", [&] { pressedButtons = Qt::NoButton; tests.segmented_control_figma_contract_loads(false); });
    runner.run("segmented_control_figma_contract_loads:mobile", [&] { pressedButtons = Qt::NoButton; tests.segmented_control_figma_contract_loads(true); });
    runner.run("alert_button_methods_are_invoked:single-primary", [&] { pressedButtons = Qt::NoButton; tests.alert_button_methods_are_invoked(1, 1, QStringLiteral("alertPrimarySingle")); });
    runner.run("alert_button_methods_are_invoked:horizontal-primary", [&] { pressedButtons = Qt::NoButton; tests.alert_button_methods_are_invoked(2, 1, QStringLiteral("alertPrimaryHorizontal")); });
    runner.run("alert_button_methods_are_invoked:horizontal-secondary", [&] { pressedButtons = Qt::NoButton; tests.alert_button_methods_are_invoked(2, 2, QStringLiteral("alertSecondaryHorizontal")); });
    runner.run("alert_button_methods_are_invoked:vertical-primary", [&] { pressedButtons = Qt::NoButton; tests.alert_button_methods_are_invoked(3, 1, QStringLiteral("alertPrimaryVertical")); });
    runner.run("alert_button_methods_are_invoked:vertical-secondary", [&] { pressedButtons = Qt::NoButton; tests.alert_button_methods_are_invoked(3, 2, QStringLiteral("alertSecondaryVertical")); });
    runner.run("alert_button_methods_are_invoked:vertical-tertiary", [&] { pressedButtons = Qt::NoButton; tests.alert_button_methods_are_invoked(3, 3, QStringLiteral("alertTertiaryVertical")); });
    runner.run("alert_glass_overlay_and_input_contract:qt-window", [&] { pressedButtons = Qt::NoButton; tests.alert_glass_overlay_and_input_contract(false, QStringLiteral("qt-window")); });
    runner.run("alert_glass_overlay_and_input_contract:lvrs-window", [&] { pressedButtons = Qt::NoButton; tests.alert_glass_overlay_and_input_contract(true, QStringLiteral("lvrs-window")); });
    return runner.result();
}
#include "tst_import_api.moc"
