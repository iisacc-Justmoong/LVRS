#include <QtTest>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QAbstractListModel>
#include <QAbstractTableModel>
#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QVector>
#include <QtPlugin>

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

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

class ImportApiTests : public QObject
{
    Q_OBJECT

private slots:
    void app_bootstrap_window_loads();
    void versionless_import_application_window_loads();
    void application_window_mobile_coverage_visibility_contract_loads();
    void application_window_page_stack_state_loads();
    void application_window_initial_properties_seed_page_stack();
    void application_window_safe_margin_scopes_to_layout_not_render_surface();
    void versionless_import_window_loads();
    void window_safe_margin_scopes_to_layout_not_render_surface();
    void appshell_compat_loads();
    void application_window_platform_adaptive_layout_loads();
    void icon_name_mapping_loads();
    void hierarchy_tree_model_api_loads();
    void hierarchy_string_array_model_loads();
    void hierarchy_direct_model_contract_loads();
    void hierarchy_direct_model_editing_contract_loads();
    void hierarchy_nested_children_indent_contract_loads();
    void hierarchy_editable_drag_depth_contract_loads();
    void hierarchy_editable_drag_per_item_lock_contract_loads();
    void hierarchy_mobile_drag_hold_contract_loads();
    void hierarchy_mobile_activation_commits_on_release_contract_loads();
    void hierarchy_mobile_reactivation_reemits_active_signal();
    void hierarchy_mobile_scroll_physics_contract_loads();
    void hierarchy_optional_footer_contract_loads();
    void hierarchy_toolbar_item_model_contract_loads();
    void hierarchy_toolbar_figma_layout_contract_loads();
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
    void button_default_tone_fallback_borderless_loads();
    void button_injected_methods_contract_loads();
    void stepper_figma_contract_loads();
    void combo_box_figma_contract_loads();
    void input_field_figma_contract_loads();
    void progress_bar_range_contract_loads();
    void control_icons_use_supersampled_raster_contract();
    void input_field_ios_native_text_interaction_contract_loads();
    void input_field_native_event_input_contract_loads();
    void input_field_search_icon_mobile_scaling_contract_loads();
    void toggle_switch_figma_color_contract_loads();
    void checkbox_figma_contract_loads();
    void radio_button_figma_contract_loads();
    void modal_empty_frame_contract_loads();
    void modal_content_action_contract_loads();
    void alert_action_button_padding_scopes_to_alert();
    void menu_item_key_and_chevron_contract_loads();
    void context_menu_item_action_contract_loads();
    void context_menu_visual_contract_loads();
    void context_menu_width_expansion_contract_loads();
    void context_menu_auto_placement_contract_loads();
    void model_component_delegate_contract_loads();
    void table_cell_item_contract_loads();
    void table_cell_merge_split_contract_loads();
    void table_structure_editing_contract_loads();
    void table_resize_contract_loads();
    void table_typed_header_contract_loads();
    void table_cpp_model_contract_loads();
    void table_undo_redo_contract_loads();
    void list_model_contract_loads();
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
    QVERIFY2(root, "Expected AppBootstrapWindow to load");
    QVERIFY2(!component.isError(), "Unexpected QML errors while creating AppBootstrapWindow");
    QVERIFY(root->property("bootstrapContractReady").toBool());
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
    QVERIFY(root);
    QVERIFY(root->property("importReady").toBool());
    QVERIFY(root->property("shellApiReady").toBool());
    QVERIFY(root->property("adaptiveApiReady").toBool());
    QVERIFY(root->property("qualityReady").toBool());
    QVERIFY(root->property("backendOptimizationDefaultsReady").toBool());
    QVERIFY(root->property("bootstrapContractReady").toBool());
    QVERIFY(root->property("platformPolicyDefaultsReady").toBool());
    QVERIFY(root->property("mobileSystemSafeAreaApiReady").toBool());
    QVERIFY(root->property("deviceTierPolicyReady").toBool());
    QVERIFY(root->property("safeAreaObserverReady").toBool());
    QVERIFY(root->property("labelStyleApiReady").toBool());
    QVERIFY(root->property("figmaTextDesignReady").toBool());
    QCOMPARE(root->property("subtitle").toString(), QStringLiteral("Merged"));
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
    QVERIFY(root);
    QVERIFY(root->property("coverageVisibilityReady").toBool());
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
    QVERIFY(root);
    QCOMPARE(root->property("title").toString(), QStringLiteral("Settings"));
    QVERIFY(root->property("solidChrome").toBool());
    QVERIFY(root->property("windowApiReady").toBool());
    QVERIFY(root->property("contentApiReady").toBool());
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
    QVERIFY(root);
    const qreal rootWidth = root->property("width").toReal();
    const qreal rootHeight = root->property("height").toReal();
    root->setProperty("visible", true);
    QTRY_COMPARE(root->property("renderSurfaceBounds").toRectF(), QRectF(0.0, 0.0, rootWidth, rootHeight));
    QTRY_COMPARE(root->property("layoutSafeAreaBounds").toRectF(), QRectF(24.0, 24.0, rootWidth - 48.0, rootHeight - 48.0));
    auto *layoutHost = root->findChild<QQuickItem *>(QStringLiteral("windowLayoutSafeAreaHost"));
    QVERIFY(layoutHost);
    QTRY_COMPARE(layoutHost->width(), rootWidth - 48.0);
    QTRY_COMPARE(layoutHost->height(), rootHeight - 48.0);
    const QPointF layoutOrigin = layoutHost->mapToScene(QPointF(0.0, 0.0));
    QCOMPARE(layoutOrigin, QPointF(24.0, 24.0));
    auto *contentProbe = root->findChild<QQuickItem *>(QStringLiteral("contentProbe"));
    QVERIFY(contentProbe);
    QTRY_COMPARE(contentProbe->width(), rootWidth);
    QTRY_COMPARE(contentProbe->height(), rootHeight);
    QCOMPARE(contentProbe->mapToScene(QPointF(0.0, 0.0)), QPointF(0.0, 0.0));
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
    QVERIFY(root);
    const qreal rootWidth = root->property("width").toReal();
    const qreal rootHeight = root->property("height").toReal();
    root->setProperty("visible", true);
    const QRectF renderSurfaceBounds = root->property("renderSurfaceBounds").toRectF();
    const QRectF layoutSafeAreaBounds = root->property("layoutSafeAreaBounds").toRectF();
    QCOMPARE(renderSurfaceBounds, QRectF(0.0, 0.0, rootWidth, rootHeight));
    QCOMPARE(layoutSafeAreaBounds, QRectF(24.0, 24.0, rootWidth - 48.0, rootHeight - 48.0));
    auto *layoutHost = root->findChild<QQuickItem *>(QStringLiteral("applicationWindowLayoutSafeAreaHost"));
    QVERIFY(layoutHost);
    const QPointF layoutOrigin = layoutHost->mapToScene(QPointF(0.0, 0.0));
    QCOMPARE(layoutOrigin, QPointF(24.0, 24.0));
    QCOMPARE(layoutHost->width(), rootWidth - 48.0);
    QCOMPARE(layoutHost->height(), rootHeight - 48.0);
    auto *contentProbe = root->findChild<QQuickItem *>(QStringLiteral("contentProbe"));
    QVERIFY(contentProbe);
    QTRY_COMPARE(contentProbe->width(), rootWidth);
    QTRY_COMPARE(contentProbe->height(), rootHeight);
    QCOMPARE(contentProbe->mapToScene(QPointF(0.0, 0.0)), QPointF(0.0, 0.0));
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
    QVERIFY2(root, "Expected ApplicationWindow to load with initial properties");
    QVERIFY2(!component.isError(), "Unexpected QML errors while creating ApplicationWindow with initial properties");
    QTRY_VERIFY(root->property("stackReady").toBool());
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
    scaffoldLayoutPlatform: "android-arm64"
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
    QVERIFY(root);
    QVERIFY(root->property("themeAddsSvg").toBool());
    QVERIFY(root->property("themeKeepsSvg").toBool());
    QVERIFY(root->property("themeFlattensGroupedName").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("directModelContractReady").toBool());

    nativeModel.appendRow(QStringLiteral("native-extra"),
                          QStringLiteral("Native Extra"),
                          0,
                          false,
                          0,
                          true,
                          false);
    QTRY_COMPARE(root->property("nativeListItemCount").toInt(), 4);
    QTRY_VERIFY(root->property("appendedNativeModelReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY2(root->property("itemsReady").toBool(),
                 qPrintable(root->property("readinessState").toString()));

    QVariant nativeMovePerformed;
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "performNativeModelMove",
                                      Q_RETURN_ARG(QVariant, nativeMovePerformed)));
    QVERIFY(nativeMovePerformed.toBool());
    QTRY_VERIFY(root->property("nativeModelMovedReady").toBool());

    QVariant qmlMovePerformed;
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "performQmlListModelMove",
                                      Q_RETURN_ARG(QVariant, qmlMovePerformed)));
    QVERIFY(qmlMovePerformed.toBool());
    QTRY_VERIFY(root->property("qmlModelMovedReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("nestedContractReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("dragItemReady").toBool());
    QVariant dragPerformed;
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "performEditableDrag",
                                      Q_RETURN_ARG(QVariant, dragPerformed)));
    QVERIFY(dragPerformed.toBool());
    QTRY_VERIFY(root->property("dragContractReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("itemsReady").toBool());
    QVariant dragLockContractReady;
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "evaluateDragLockContract",
                                      Q_RETURN_ARG(QVariant, dragLockContractReady)));
    QVERIFY(dragLockContractReady.toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("mobileHoldContractReady").toBool());
    QVariant dragPerformed;
    QVERIFY(QMetaObject::invokeMethod(root.data(),
                                      "performProgrammaticDrag",
                                      Q_RETURN_ARG(QVariant, dragPerformed)));
    QVERIFY(dragPerformed.toBool());
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
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());
    QTRY_VERIFY(root->property("mobileReleaseActivationContractReady").toBool());

    auto *list = root->findChild<QObject *>(QStringLiteral("hierarchyList"));
    QVERIFY(list);
    QObject *rootItemObject = root->property("rootItemRef").value<QObject *>();
    QObject *branchItemObject = root->property("branchItemRef").value<QObject *>();
    QVERIFY(rootItemObject);
    QVERIFY(branchItemObject);
    auto *branchItem = qobject_cast<QQuickItem *>(branchItemObject);
    QVERIFY(branchItem);

    const QPointF branchPoint = branchItem->mapToScene(QPointF(branchItem->width() * 0.5,
                                                               branchItem->height() * 0.5));
    const QPoint branchPointInt(qRound(branchPoint.x()), qRound(branchPoint.y()));

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, branchPointInt, 10);
    QCoreApplication::processEvents();
    QTest::qWait(50);

    QCOMPARE(list->property("activeItem").value<QObject *>(), rootItemObject);
    QCOMPARE(list->property("activeItemKey").toString(), QStringLiteral("root"));
    QVERIFY(!branchItemObject->property("active").toBool());

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, branchPointInt, 10);

    QTRY_COMPARE(list->property("activeItemKey").toString(), QStringLiteral("branch"));
    QTRY_VERIFY(list->property("activeItem").value<QObject *>() != nullptr);
    QObject *activeBranchObject = list->property("activeItem").value<QObject *>();
    QCOMPARE(activeBranchObject->property("itemKey").toString(), QStringLiteral("branch"));
    QTRY_VERIFY(activeBranchObject->property("active").toBool());
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
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());
    QTRY_VERIFY(root->property("mobileRetapContractReady").toBool());

    auto *hierarchy = root->findChild<QObject *>(QStringLiteral("hierarchy"));
    QVERIFY(hierarchy);
    QObject *rootItemObject = root->property("rootItemRef").value<QObject *>();
    QVERIFY(rootItemObject);
    auto *rootItem = qobject_cast<QQuickItem *>(rootItemObject);
    QVERIFY(rootItem);

    QSignalSpy activationSpy(hierarchy, SIGNAL(listItemActivated(QVariant,int,int)));
    QVERIFY(activationSpy.isValid());

    const QPointF rootPoint = rootItem->mapToScene(QPointF(rootItem->width() * 0.5,
                                                           rootItem->height() * 0.5));
    const QPoint rootPointInt(qRound(rootPoint.x()), qRound(rootPoint.y()));

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, rootPointInt, 10);
    QCoreApplication::processEvents();
    QTest::qWait(50);
    QCOMPARE(activationSpy.count(), 0);

    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, rootPointInt, 10);

    QTRY_COMPARE(activationSpy.count(), 1);
    QCOMPARE(hierarchy->property("activeListItem").value<QObject *>(), rootItemObject);
    QCOMPARE(hierarchy->property("activeListItemKey").toString(), QStringLiteral("root"));
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
    QVERIFY(root);

    QObject *hierarchy = root->findChild<QObject *>(QStringLiteral("hierarchy"));
    QVERIFY(hierarchy);
    QObject *viewport = root->findChild<QObject *>(QStringLiteral("hierarchyListViewportFlickable"));
    QVERIFY(viewport);

    QTRY_COMPARE(viewport->property("boundsBehavior").toInt(), kFlickableDragAndOvershootBounds);
    QCOMPARE(viewport->property("boundsMovement").toInt(), kFlickableFollowBoundsBehavior);
    QCOMPARE(viewport->property("boundsBehavior").toInt(), hierarchy->property("listBoundsBehavior").toInt());
    QCOMPARE(viewport->property("boundsMovement").toInt(), hierarchy->property("listBoundsMovement").toInt());
    QCOMPARE(viewport->property("flickDeceleration").toInt(), hierarchy->property("listFlickDeceleration").toInt());
    QCOMPARE(viewport->property("maximumFlickVelocity").toInt(), hierarchy->property("listMaximumFlickVelocity").toInt());
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
    QTRY_VERIFY(itemAObject->property("active").toBool());
    QTRY_COMPARE(itemAObject->property("state").toString(), QStringLiteral("Active"));
    QTRY_COMPARE(itemAObject->property("uxState").toInt(), itemAObject->property("uxStateActive").toInt());
    QTRY_COMPARE(itemBObject->property("state").toString(), QStringLiteral("Idle"));
    QTRY_COMPARE(itemBObject->property("uxState").toInt(), itemBObject->property("uxStateIdle").toInt());
    QVERIFY(!itemBObject->property("isHoverState").toBool());
    QVERIFY(!itemBObject->property("isActiveState").toBool());

    const QPointF hoverPoint = itemB->mapToScene(QPointF(itemB->width() * 0.5, itemB->height() * 0.5));
    const QPoint hoverPointInt(qRound(hoverPoint.x()), qRound(hoverPoint.y()));
    QTest::mouseMove(window, hoverPointInt, 10);

    QTRY_VERIFY(itemBObject->property("isHoverState").toBool());
    QTRY_COMPARE(itemBObject->property("state").toString(), QStringLiteral("Hover"));
    QTRY_COMPARE(itemBObject->property("uxState").toInt(), itemBObject->property("uxStateHover").toInt());

    QObject *hoverBackground = itemBObject->property("background").value<QObject *>();
    QVERIFY(hoverBackground);
    const QColor hoverRenderedColor = hoverBackground->property("color").value<QColor>();
    const QColor expectedHoverColor = itemBObject->property("backgroundColorHover").value<QColor>();
    QCOMPARE(hoverRenderedColor, expectedHoverColor);

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, hoverPointInt, 10);

    QTRY_VERIFY(list->property("activeItem").value<QObject *>() == itemBObject);
    QTRY_VERIFY(itemBObject->property("active").toBool());
    QTRY_VERIFY(itemBObject->property("isActiveState").toBool());
    QTRY_COMPARE(itemBObject->property("state").toString(), QStringLiteral("Active"));
    QTRY_COMPARE(itemBObject->property("uxState").toInt(), itemBObject->property("uxStateActive").toInt());
    QTRY_VERIFY(!itemAObject->property("isActiveState").toBool());

    QObject *activeBackground = itemBObject->property("background").value<QObject *>();
    QVERIFY(activeBackground);
    const QColor activeRenderedColor = activeBackground->property("color").value<QColor>();
    const QColor expectedActiveColor = itemBObject->property("backgroundColor").value<QColor>();
    QCOMPARE(activeRenderedColor, expectedActiveColor);
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
        && item.iconSize === 16
        && item.chevronSize === 16
        && item.baseLeftPadding === 8
        && item.rowRightPadding === 8
        && item.leadingSpacing === 2
        && item.implicitWidth === 200
        && item.implicitHeight === 20
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("contractReady").toBool());
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
    QVERIFY(root);

    auto *itemObject = root->findChild<QObject *>(QStringLiteral("item"));
    QVERIFY(itemObject);
    auto *item = qobject_cast<QQuickItem *>(itemObject);
    QVERIFY(item);

    auto *iconObject = itemObject->findChild<QObject *>(QStringLiteral("hierarchyItemIcon"), Qt::FindChildrenRecursively);
    auto *labelObject = itemObject->findChild<QObject *>(QStringLiteral("hierarchyItemLabel"), Qt::FindChildrenRecursively);
    auto *chevronObject = itemObject->findChild<QObject *>(QStringLiteral("hierarchyItemChevron"), Qt::FindChildrenRecursively);
    QVERIFY(iconObject);
    QVERIFY(labelObject);
    QVERIFY(chevronObject);
    auto *iconItem = qobject_cast<QQuickItem *>(iconObject);
    auto *labelItem = qobject_cast<QQuickItem *>(labelObject);
    auto *chevronItem = qobject_cast<QQuickItem *>(chevronObject);
    QVERIFY(iconItem);
    QVERIFY(labelItem);
    QVERIFY(chevronItem);

    const QPointF iconPos = iconItem->mapToItem(item, QPointF(0, 0));
    const QPointF labelPos = labelItem->mapToItem(item, QPointF(0, 0));
    const QPointF chevronPos = chevronItem->mapToItem(item, QPointF(0, 0));

    QVERIFY2(qAbs(iconPos.x() - 8.0) <= 0.5, "Icon x must match the 8px Figma inset.");
    QVERIFY2(qAbs(labelPos.x() - 26.0) <= 0.5, "Label x must follow icon width + 2px gap.");
    QVERIFY2(qAbs(chevronPos.x() - 176.0) <= 0.5, "Chevron x must match the trailing 8px inset.");
    QVERIFY2(qAbs(labelItem->width() - 148.0) <= 0.5, "Label width must match the Figma baseline layout.");
    QVERIFY2(qAbs(item->height() - 20.0) <= 0.5, "Row height must remain 20px.");
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("countViewReady").toBool());

    auto *defaultItemObject = root->findChild<QObject *>(QStringLiteral("defaultCountItem"));
    auto *customItemObject = root->findChild<QObject *>(QStringLiteral("customCountItem"));
    QVERIFY(defaultItemObject);
    QVERIFY(customItemObject);
    auto *defaultItem = qobject_cast<QQuickItem *>(defaultItemObject);
    auto *customItem = qobject_cast<QQuickItem *>(customItemObject);
    QVERIFY(defaultItem);
    QVERIFY(customItem);

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
    QVERIFY(defaultCountLabelObject);
    QVERIFY(defaultChevronObject);
    QVERIFY(customCountSlotObject);
    QVERIFY(customCountObject);
    QVERIFY(customChevronObject);

    auto *defaultCountLabel = qobject_cast<QQuickItem *>(defaultCountLabelObject);
    auto *defaultChevron = qobject_cast<QQuickItem *>(defaultChevronObject);
    auto *customCountSlot = qobject_cast<QQuickItem *>(customCountSlotObject);
    auto *customCount = qobject_cast<QQuickItem *>(customCountObject);
    auto *customChevron = qobject_cast<QQuickItem *>(customChevronObject);
    QVERIFY(defaultCountLabel);
    QVERIFY(defaultChevron);
    QVERIFY(customCountSlot);
    QVERIFY(customCount);
    QVERIFY(customChevron);

    QCOMPARE(defaultCountLabelObject->property("text").toString(), QStringLiteral("7"));
    QCOMPARE(customCountObject->property("count").toInt(), 12);

    const QPointF defaultCountPos = defaultCountLabel->mapToItem(defaultItem, QPointF(0, 0));
    const QPointF defaultChevronPos = defaultChevron->mapToItem(defaultItem, QPointF(0, 0));
    const qreal defaultGap = defaultChevronPos.x() - (defaultCountPos.x() + defaultCountLabel->width());
    QVERIFY2(qAbs(defaultGap - 8.0) <= 0.5, "Count label must sit 8px left of the chevron slot.");

    const QPointF customCountPos = customCountSlot->mapToItem(customItem, QPointF(0, 0));
    const QPointF customChevronPos = customChevron->mapToItem(customItem, QPointF(0, 0));
    const qreal customGap = customChevronPos.x() - (customCountPos.x() + customCountSlot->width());
    QVERIFY2(qAbs(customGap - 8.0) <= 0.5, "Custom count view must align 8px left of the chevron anchor.");
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("contractReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("contractReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY2(root->property("injectedMethodsReady").toBool(),
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
        Math.round(((primaryUpDown.width - primaryUpDown.iconWidth) * 0.5) * primaryUpDown.devicePixelRatio)
        / primaryUpDown.devicePixelRatio
    readonly property real expectedUpDownY:
        Math.round(((primaryUpDown.height - primaryUpDown.iconHeight) * 0.5) * primaryUpDown.devicePixelRatio)
        / primaryUpDown.devicePixelRatio

    LV.Stepper { id: defaultStepper; visible: false }
    LV.Stepper { id: primaryUpDown; visible: false; tone: LV.AbstractButton.Primary; arrow: LV.Stepper.UpDown }
    LV.Stepper { id: primaryUp; visible: false; tone: LV.AbstractButton.Primary; arrow: LV.Stepper.Up }
    LV.Stepper { id: primaryDown; visible: false; tone: LV.AbstractButton.Primary; arrow: LV.Stepper.Down }
    LV.Stepper { id: borderlessUpDown; visible: false; tone: LV.AbstractButton.Borderless; arrow: LV.Stepper.UpDown }
    LV.Stepper { id: borderlessUp; visible: false; tone: LV.AbstractButton.Borderless; arrow: LV.Stepper.Up }
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
        && Math.abs(primaryUp.iconWidth - 10.0) < 0.01
        && Math.abs(primaryUp.iconHeight - 6.0) < 0.01
        && Math.abs(primaryDown.iconWidth - 10.0) < 0.01
        && Math.abs(primaryDown.iconHeight - 6.0) < 0.01
        && Math.abs(primaryUpDown.iconWidth - 6.436) < 0.05
        && Math.abs(primaryUpDown.iconHeight - 11.146) < 0.05
        && Math.abs(borderlessUpDown.iconWidth - 6.436) < 0.05
        && Math.abs(borderlessUpDown.iconHeight - 11.146) < 0.05
        && Math.abs(primaryUp.iconBounds.x - 3.0) < 0.05
        && Math.abs(primaryUp.iconBounds.y - 5.0) < 0.05
        && Math.abs(primaryUp.iconBounds.width - 10.0) < 0.05
        && Math.abs(primaryUp.iconBounds.height - 6.0) < 0.05
        && Math.abs(primaryDown.iconBounds.x - 3.0) < 0.05
        && Math.abs(primaryDown.iconBounds.y - 5.0) < 0.05
        && Math.abs(primaryDown.iconBounds.width - 10.0) < 0.05
        && Math.abs(primaryDown.iconBounds.height - 6.0) < 0.05
        && Math.abs(primaryUpDown.iconBounds.x - expectedUpDownX) < 0.06
        && Math.abs(primaryUpDown.iconBounds.y - expectedUpDownY) < 0.06
        && Math.abs(primaryUpDown.iconBounds.width - 6.436) < 0.06
        && Math.abs(primaryUpDown.iconBounds.height - 11.146) < 0.06
        && primaryUp.backgroundColor === LV.Theme.primary
        && borderlessUp.backgroundColor === transparentColor
        && borderlessUp.backgroundColorHover === LV.Theme.surfaceAlt
        && borderlessUp.backgroundColorPressed === LV.Theme.accentBlueMuted
        && primaryUp.resolvedIconName === "StepperUpPrimary"
        && primaryDown.resolvedIconName === "StepperDownPrimary"
        && borderlessUp.resolvedIconName === "StepperUpBorderless"
        && borderlessDown.resolvedIconName === "StepperDownBorderless"
        && primaryUp.resolvedIconColor === LV.Theme.accentWhite
        && primaryDown.resolvedIconColor === LV.Theme.accentWhite
        && borderlessUp.resolvedIconColor === LV.Theme.accentWhite
        && borderlessDown.resolvedIconColor === LV.Theme.accentWhite
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    const QString stepperDebug = root->property("stepperDebug").toString();
    QVERIFY2(root->property("stepperContractReady").toBool(), qPrintable(stepperDebug));
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
        && Math.abs(defaultCombo.height - 18.0) < 0.01
        && Math.abs(defaultCombo.implicitWidth - 97.0) < 0.01
        && Math.abs(defaultCombo.implicitHeight - 18.0) < 0.01
        && Math.abs(defaultCombo.figmaComboWidth - 97.0) < 0.01
        && Math.abs(defaultCombo.figmaComboHeight - 18.0) < 0.01
        && Math.abs(defaultCombo.figmaComboLeftPadding - 8.0) < 0.01
        && Math.abs(defaultCombo.figmaComboRightPadding - 1.0) < 0.01
        && Math.abs(defaultCombo.figmaComboVerticalPadding - 1.0) < 0.01
        && Math.abs(defaultCombo.figmaComboCornerRadius - 5.0) < 0.01
        && Math.abs(defaultCombo.figmaIndicatorSize - 16.0) < 0.01
        && Math.abs(defaultCombo.figmaLabelLineHeight - 12.0) < 0.01
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
        && Math.abs(defaultCombo.labelBounds.y - 3.0) < 0.01
        && Math.abs(defaultCombo.labelBounds.width - 72.0) < 0.01
        && Math.abs(defaultCombo.labelBounds.height - 12.0) < 0.01
        && Math.abs(defaultCombo.indicatorBounds.x - 80.0) < 0.01
        && Math.abs(defaultCombo.indicatorBounds.y - 1.0) < 0.01
        && Math.abs(defaultCombo.indicatorBounds.width - 16.0) < 0.01
        && Math.abs(defaultCombo.indicatorBounds.height - 16.0) < 0.01
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
        search: true
        placeholderText: "Search"
        text: "abc"
    }

    LV.InputField {
        id: legacySearchField
        visible: false
        mode: searchMode
        placeholderText: "Legacy search"
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

    LV.InputField {
        id: inlineField
        visible: false
        style: inlineStyle
        placeholderText: "Inline"
        text: "value"
    }

    LV.InputField {
        id: inlineDisabledField
        visible: false
        enabled: false
        style: inlineStyle
        placeholderText: "Inline disabled"
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
        && defaultField.searchIconColor === LV.Theme.accentGrayLight
        && defaultField.clearIconBackgroundColor === LV.Theme.descriptionColor
        && defaultField.clearIconBackgroundColorDisabled === LV.Theme.disabledColor
        && defaultField.clearIconForegroundColor === LV.Theme.panelBackground10
        && inlineField.style === inlineField.inlineStyle
        && inlineField.backgroundColor === LV.Theme.accentTransparent
        && inlineField.backgroundColorHover === LV.Theme.accentTransparent
        && inlineField.backgroundColorPressed === LV.Theme.accentTransparent
        && inlineField.backgroundColorFocused === LV.Theme.accentTransparent
        && inlineField.backgroundColorDisabled === LV.Theme.accentTransparent
        && inlineDisabledField.backgroundColor === LV.Theme.accentTransparent
        && inlineDisabledField.backgroundColorDisabled === LV.Theme.accentTransparent
        && inlineField.showClearButton
        && searchField.search
        && searchField.searchIconVisible
        && Math.abs(searchField.searchIconSize - LV.Theme.scaleMetric(12)) < 0.01
        && searchField.showClearButton
        && legacySearchField.mode === legacySearchField.searchMode
        && legacySearchField.search
        && legacySearchField.searchIconVisible
        && passwordField.echoMode === TextInput.Password
        && readOnlyField.readOnly
        && !readOnlyField.showClearButton
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QVERIFY(root->property("figmaInputFieldReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("progressBarRangeReady").toBool());
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

    LV.ToggleSwitch {
        objectName: "toggleSwitch"
        width: implicitWidth
        height: implicitHeight
        visible: false
        checked: true
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);

    const auto assertSnapshotImage = [&](const QString &controlObjectName,
                                         const QString &objectName,
                                         const char *sourcePropertyName,
                                         const char *widthPropertyName,
                                         const char *heightPropertyName) {
        QObject *controlObject = root->findChild<QObject *>(controlObjectName, Qt::FindChildrenRecursively);
        QObject *imageObject = root->findChild<QObject *>(objectName, Qt::FindChildrenRecursively);
        QVERIFY2(controlObject, qPrintable(QStringLiteral("Expected control object %1").arg(controlObjectName)));
        QVERIFY2(imageObject, qPrintable(QStringLiteral("Expected image object %1").arg(objectName)));

        const QSize sourceSize = imageObject->property("sourceSize").toSize();
        const QUrl source = imageObject->property("source").toUrl();
        const QUrl expectedSource = controlObject->property(sourcePropertyName).toUrl();
        const qreal expectedWidth = controlObject->property(widthPropertyName).toReal();
        const qreal expectedHeight = controlObject->property(heightPropertyName).toReal();

        QCOMPARE(source, expectedSource);
        QCOMPARE(sourceSize.width(), qRound(expectedWidth));
        QCOMPARE(sourceSize.height(), qRound(expectedHeight));
    };

    const auto assertSupersampledCanvas = [&](const QString &controlObjectName,
                                              const QString &objectName,
                                              const char *scalePropertyName) {
        QObject *controlObject = root->findChild<QObject *>(controlObjectName, Qt::FindChildrenRecursively);
        QObject *canvasObject = root->findChild<QObject *>(objectName, Qt::FindChildrenRecursively);
        QVERIFY2(controlObject, qPrintable(QStringLiteral("Expected control object %1").arg(controlObjectName)));
        QVERIFY2(canvasObject, qPrintable(QStringLiteral("Expected canvas object %1").arg(objectName)));

        const QSize canvasSize = canvasObject->property("canvasSize").toSize();
        const qreal logicalWidth = canvasObject->property("width").toReal();
        const qreal logicalHeight = canvasObject->property("height").toReal();
        const qreal rasterScale = controlObject->property(scalePropertyName).toReal();

        QVERIFY2(logicalWidth > 0.0, qPrintable(QStringLiteral("%1 width must be positive").arg(objectName)));
        QVERIFY2(logicalHeight > 0.0, qPrintable(QStringLiteral("%1 height must be positive").arg(objectName)));
        QVERIFY2(canvasSize.width() > qRound(logicalWidth),
                 qPrintable(QStringLiteral("%1 canvas width should exceed logical width").arg(objectName)));
        QVERIFY2(canvasSize.height() > qRound(logicalHeight),
                 qPrintable(QStringLiteral("%1 canvas height should exceed logical height").arg(objectName)));
        QCOMPARE(canvasSize.width(), qCeil(logicalWidth * rasterScale));
        QCOMPARE(canvasSize.height(), qCeil(logicalHeight * rasterScale));
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
    assertSupersampledCanvas(QStringLiteral("toggleSwitch"),
                             QStringLiteral("toggleSwitch_knobCanvas"),
                             "knobRasterScale");
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("iosNativeTextReady").toBool());
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
    QVERIFY(root);

    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    window->show();
    QTRY_VERIFY(window->isVisible());

    QObject *field = root->findChild<QObject *>(QStringLiteral("inputField"));
    QVERIFY(field);
    auto *inputItem = root->findChild<QQuickItem *>(QStringLiteral("nativeInput"));
    QVERIFY(inputItem);

    const QPoint wordPoint = scenePoint(inputItem, QPointF(28.0, inputItem->height() * 0.5));
    const QPoint dragStart = scenePoint(inputItem, QPointF(8.0, inputItem->height() * 0.5));
    const QPoint dragEnd = scenePoint(inputItem, QPointF(132.0, inputItem->height() * 0.5));

    QVERIFY(!mouseAreaContainsScenePoint(root.data(), wordPoint));

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, wordPoint, 10);
    QTRY_VERIFY(inputItem->property("activeFocus").toBool());

    inputItem->setProperty("cursorPosition", 0);
    QTest::keyClick(window, Qt::Key_Z, Qt::NoModifier, 10);
    QTRY_VERIFY(field->property("text").toString().startsWith(QStringLiteral("z")));

    QInputMethodEvent preeditEvent(QStringLiteral("preedit"), {});
    QCoreApplication::sendEvent(inputItem, &preeditEvent);
    QTRY_COMPARE(inputItem->property("preeditText").toString(), QStringLiteral("preedit"));

    QInputMethodEvent commitEvent;
    commitEvent.setCommitString(QStringLiteral("한"));
    QCoreApplication::sendEvent(inputItem, &commitEvent);
    QTRY_VERIFY(field->property("text").toString().contains(QStringLiteral("한")));

    inputItem->setProperty("text", QStringLiteral("alpha beta gamma"));
    inputItem->setProperty("cursorPosition", 0);
    QVERIFY(QMetaObject::invokeMethod(field, "deselect"));
    QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, wordPoint, 10);
    QTRY_VERIFY(inputItem->property("selectedText").toString().contains(QStringLiteral("alpha")));
    const QString doubleClickSelection = inputItem->property("selectedText").toString();
    QVERIFY(!doubleClickSelection.isEmpty());

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, wordPoint, 10);
    QTRY_VERIFY(inputItem->property("selectedText").toString() != doubleClickSelection);

    QVERIFY(QMetaObject::invokeMethod(field, "deselect"));
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, dragStart, 10);
    QTest::mouseMove(window, dragEnd, 10);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, dragEnd, 10);
    QTRY_VERIFY(inputItem->property("selectedText").toString().size() > 0);
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
        && Math.abs(searchField.searchIconSize - LV.Theme.scaleMetric(12)) < 0.01
        && Math.abs(searchField.searchIconSize - 15.0) < 0.01
        && searchField.searchIconSource == LV.Theme.iconPath("generalsearch")
        && searchField.searchIconSourceSize === Math.ceil(searchField.searchIconSize * searchField.searchIconRasterScale)
        && LV.Theme.iconSm === 20
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("mobileSearchContractReady").toBool());
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

    property int expectedAlertVerticalPadding: LV.Theme.gap8
    property int expectedDefaultVerticalPadding: LV.Theme.gap4
    property int expectedAlertButtonHeight: Math.max(LV.Theme.gap20,
                                                     LV.Theme.textBodyLineHeight + (LV.Theme.gap8 * 2))
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
    QVERIFY(root);

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

    QVERIFY(alertPrimary);
    QVERIFY(alertSecondary);
    QVERIFY(alertTertiary);
    QVERIFY(modalPrimary);
    QVERIFY(modalSecondary);
    QVERIFY(modalTertiary);
    QVERIFY(standaloneAlertButton);

    const int expectedAlertVerticalPadding = root->property("expectedAlertVerticalPadding").toInt();
    const int expectedDefaultVerticalPadding = root->property("expectedDefaultVerticalPadding").toInt();
    const double expectedAlertButtonHeight = root->property("expectedAlertButtonHeight").toDouble();
    const double expectedDefaultButtonHeight = root->property("expectedDefaultButtonHeight").toDouble();

    QCOMPARE(alertPrimary->property("verticalPadding").toInt(), expectedAlertVerticalPadding);
    QCOMPARE(alertSecondary->property("verticalPadding").toInt(), expectedAlertVerticalPadding);
    QCOMPARE(alertTertiary->property("verticalPadding").toInt(), expectedAlertVerticalPadding);
    QVERIFY(qAbs(alertPrimary->property("height").toDouble() - expectedAlertButtonHeight) < 0.01);
    QVERIFY(qAbs(alertSecondary->property("height").toDouble() - expectedAlertButtonHeight) < 0.01);
    QVERIFY(qAbs(alertTertiary->property("height").toDouble() - expectedAlertButtonHeight) < 0.01);

    QCOMPARE(modalPrimary->property("verticalPadding").toInt(), expectedDefaultVerticalPadding);
    QCOMPARE(modalSecondary->property("verticalPadding").toInt(), expectedDefaultVerticalPadding);
    QCOMPARE(modalTertiary->property("verticalPadding").toInt(), expectedDefaultVerticalPadding);
    QVERIFY(qAbs(modalPrimary->property("height").toDouble() - expectedDefaultButtonHeight) < 0.01);
    QVERIFY(qAbs(modalSecondary->property("height").toDouble() - expectedDefaultButtonHeight) < 0.01);
    QVERIFY(qAbs(modalTertiary->property("height").toDouble() - expectedDefaultButtonHeight) < 0.01);

    QCOMPARE(standaloneAlertButton->property("verticalPadding").toInt(), expectedDefaultVerticalPadding);
    QVERIFY(qAbs(standaloneAlertButton->property("height").toDouble() - expectedDefaultButtonHeight) < 0.01);
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

    property bool menuItemContract:
        !defaultItem.keyVisible
        && defaultItem.resolvedShortcutText === ""
        && !defaultItem.effectiveShowChevron
        && collapsedSubmenu.keyVisible
        && collapsedSubmenu.resolvedShortcutText === "Cmd+K"
        && collapsedSubmenu.effectiveShowChevron
        && collapsedSubmenu.itemHeight === 16
        && collapsedSubmenu.leftPadding === 4
        && collapsedSubmenu.rightPadding === 4
        && collapsedSubmenu.topPadding === 0
        && collapsedSubmenu.bottomPadding === 0
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("menuItemContract").toBool());

    QObject *constrainedItem = root->findChild<QObject *>(QStringLiteral("constrainedItem"));
    QVERIFY(constrainedItem);

    auto *contentRow = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_contentRow")));
    auto *labelNode = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_labelNode")));
    auto *spacer = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_flexibleSpacer")));
    auto *trailingGroup = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_trailingGroup")));
    auto *shortcutLabel = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_shortcutLabel")));
    auto *chevronIcon = qobject_cast<QQuickItem *>(constrainedItem->findChild<QObject *>(QStringLiteral("menuItem_chevronIcon")));

    QVERIFY(contentRow);
    QVERIFY(labelNode);
    QVERIFY(spacer);
    QVERIFY(trailingGroup);
    QVERIFY(shortcutLabel);
    QVERIFY(chevronIcon);
    QCOMPARE(shortcutLabel->property("style").toInt(), shortcutLabel->property("description").toInt());

    const qreal contentWidth = contentRow->width();
    const qreal labelRight = labelNode->x() + labelNode->width();
    const qreal trailingRight = trailingGroup->x() + trailingGroup->width();
    const qreal shortcutRight = shortcutLabel->x() + shortcutLabel->width();
    const qreal chevronRight = chevronIcon->x() + chevronIcon->width();

    QVERIFY2(labelRight <= contentWidth + 0.01, "Menu label must stay within the content row.");
    QVERIFY2(trailingGroup->x() >= labelRight - 0.01, "Trailing group must not overlap the label block.");
    QVERIFY2(spacer->width() >= -0.01, "Responsive spacer must not resolve to a negative width.");
    QVERIFY2(trailingRight <= contentWidth + 0.01, "Trailing group must stay within the content row.");
    QVERIFY2(shortcutRight <= trailingGroup->width() + 0.01,
             qPrintable(QStringLiteral("Shortcut label must stay within the trailing group (%1 <= %2).")
                            .arg(shortcutRight)
                            .arg(trailingGroup->width())));
    QVERIFY2(chevronRight <= trailingGroup->width() + 0.01,
             qPrintable(QStringLiteral("Chevron must stay within the trailing group (%1 <= %2).")
                            .arg(chevronRight)
                            .arg(trailingGroup->width())));

    QObject *noTrailingConstrainedItem = root->findChild<QObject *>(QStringLiteral("noTrailingConstrainedItem"));
    QVERIFY(noTrailingConstrainedItem);

    auto *noTrailingContentRow =
        qobject_cast<QQuickItem *>(noTrailingConstrainedItem->findChild<QObject *>(QStringLiteral("menuItem_contentRow")));
    auto *noTrailingLabelNode =
        qobject_cast<QQuickItem *>(noTrailingConstrainedItem->findChild<QObject *>(QStringLiteral("menuItem_labelNode")));
    QObject *noTrailingGroup =
        noTrailingConstrainedItem->findChild<QObject *>(QStringLiteral("menuItem_trailingGroup"));

    QVERIFY(noTrailingContentRow);
    QVERIFY(noTrailingLabelNode);
    QVERIFY(noTrailingGroup);

    const qreal noTrailingAvailableLabelWidth = noTrailingContentRow->width() - noTrailingLabelNode->x();
    QVERIFY2(qAbs(noTrailingLabelNode->width() - noTrailingAvailableLabelWidth) < 0.01,
             qPrintable(QStringLiteral("Label should consume the full available width when trailing content is absent (%1 vs %2).")
                            .arg(noTrailingLabelNode->width())
                            .arg(noTrailingAvailableLabelWidth)));
    QVERIFY(!noTrailingGroup->property("visible").toBool());
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

    LV.ContextMenu {
        id: menu
        visible: false
        items: [
            {
                label: "Label",
                key: "Key",
                keyVisible: true,
                showChevron: false,
                hasChildItems: false
            },
            { type: "divider" }
        ]
    }

    LV.MenuDivider {
        id: divider
        visible: false
    }

    property bool visualContract:
        menu.itemSpacing === LV.Theme.gap2
        && menu.leftPadding === LV.Theme.gap8
        && menu.rightPadding === LV.Theme.gap8
        && menu.topPadding === LV.Theme.gap8
        && menu.bottomPadding === LV.Theme.gap8
        && menu.menuColor === LV.Theme.contextMenuSurface
        && menu.menuColor === LV.Theme.panelBackground06
        && menu.dividerColor === LV.Theme.contextMenuDivider
        && menu.dividerColor === LV.Theme.disabledColor
        && menu.background !== null
        && menu.background.radius === LV.Theme.radiusMd
        && divider.dividerColor === LV.Theme.contextMenuDivider
        && divider.dividerColor === LV.Theme.disabledColor
        && Math.abs(divider.thickness - LV.Theme.scaleRealMetric(0.2)) < 0.01
        && Math.abs(divider.implicitHeight - ((divider.crossPadding * 2) + divider.thickness)) < 0.01
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    QVERIFY(root);
    QTRY_VERIFY(root->property("visualContract").toBool());
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
    QVERIFY(root);
    const QString widthDebug = root->property("widthDebug").toString();
    QTRY_VERIFY2(root->property("widthExpansionContract").toBool(), qPrintable(widthDebug));
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("delegateContractReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("mergeSplitReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("structureEditingReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("resizeContractReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("typedHeaderContractReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("cppModelContractReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("undoRedoContractReady").toBool());
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
    QVERIFY(root);
    QTRY_VERIFY(root->property("listModelContractReady").toBool());

    nativeModel.appendRow(QStringLiteral("Native D"), true, false);
    QTRY_COMPARE(root->property("nativeModelEntryCount").toInt(), 4);
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
