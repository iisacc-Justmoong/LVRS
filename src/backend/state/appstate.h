#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

class AppState : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(AppState)
    QML_SINGLETON

    Q_PROPERTY(QVariantList navItems READ navItems NOTIFY navItemsChanged)
    Q_PROPERTY(QVariantList scaffoldNavModel READ scaffoldNavModel NOTIFY scaffoldNavModelChanged)
    Q_PROPERTY(QVariantList demoListItems READ demoListItems NOTIFY demoListItemsChanged)
    Q_PROPERTY(QVariantList demoContextMenuItems READ demoContextMenuItems NOTIFY demoContextMenuItemsChanged)
    Q_PROPERTY(QVariantList navDefinitions READ navDefinitions WRITE setNavDefinitions NOTIFY navDefinitionsChanged)
    Q_PROPERTY(QVariantList scaffoldDefinitions READ scaffoldDefinitions WRITE setScaffoldDefinitions NOTIFY scaffoldDefinitionsChanged)

    Q_PROPERTY(bool alertOpen READ alertOpen WRITE setAlertOpen NOTIFY alertOpenChanged)
    Q_PROPERTY(qreal progressStart READ progressStart WRITE setProgressStart NOTIFY progressChanged)
    Q_PROPERTY(qreal progressEnd READ progressEnd WRITE setProgressEnd NOTIFY progressChanged)
    Q_PROPERTY(qreal progressCurrent READ progressCurrent WRITE setProgressCurrent NOTIFY progressChanged)
    Q_PROPERTY(QString currentRoute READ currentRoute WRITE setCurrentRoute NOTIFY currentRouteChanged)
    Q_PROPERTY(int scaffoldNavIndex READ scaffoldNavIndex WRITE setScaffoldNavIndex NOTIFY scaffoldNavIndexChanged)
    Q_PROPERTY(int hierarchyActiveButtonId READ hierarchyActiveButtonId WRITE setHierarchyActiveButtonId NOTIFY hierarchyActiveButtonIdChanged)

    Q_PROPERTY(QVariantMap runtimeSnapshot READ runtimeSnapshot NOTIFY runtimeSnapshotChanged)
    Q_PROPERTY(QVariantMap viewStateSnapshot READ viewStateSnapshot NOTIFY viewStateSnapshotChanged)
    Q_PROPERTY(QStringList pageHistory READ pageHistory NOTIFY pageHistoryChanged)

public:
    explicit AppState(QObject *parent = nullptr);

    QVariantList navItems() const;
    QVariantList scaffoldNavModel() const;
    QVariantList demoListItems() const;
    QVariantList demoContextMenuItems() const;
    QVariantList navDefinitions() const;
    void setNavDefinitions(const QVariantList &definitions);
    QVariantList scaffoldDefinitions() const;
    void setScaffoldDefinitions(const QVariantList &definitions);

    bool alertOpen() const;
    void setAlertOpen(bool value);

    qreal progressStart() const;
    void setProgressStart(qreal value);

    qreal progressEnd() const;
    void setProgressEnd(qreal value);

    qreal progressCurrent() const;
    void setProgressCurrent(qreal value);

    QString currentRoute() const;
    void setCurrentRoute(const QString &value);

    int scaffoldNavIndex() const;
    void setScaffoldNavIndex(int value);

    int hierarchyActiveButtonId() const;
    void setHierarchyActiveButtonId(int value);

    QVariantMap runtimeSnapshot() const;
    QVariantMap viewStateSnapshot() const;
    QStringList pageHistory() const;

    Q_INVOKABLE void bootstrap();
    Q_INVOKABLE void nudgeProgress(qreal delta);
    Q_INVOKABLE void recordNavigation(const QString &path);
    Q_INVOKABLE void selectScaffoldNavIndex(int index);
    Q_INVOKABLE void syncRuntimeSnapshot(const QVariantMap &snapshot);
    Q_INVOKABLE void syncViewStateSnapshot(const QVariantMap &snapshot);
    Q_INVOKABLE void syncPageHistory(const QStringList &history);

signals:
    void navItemsChanged();
    void scaffoldNavModelChanged();
    void demoListItemsChanged();
    void demoContextMenuItemsChanged();
    void navDefinitionsChanged();
    void scaffoldDefinitionsChanged();
    void alertOpenChanged();
    void progressChanged();
    void currentRouteChanged();
    void scaffoldNavIndexChanged();
    void hierarchyActiveButtonIdChanged();
    void runtimeSnapshotChanged();
    void viewStateSnapshotChanged();
    void pageHistoryChanged();

private:
    static QString normalizeBasicPath(const QString &rawPath);
    QString normalizeRoutePath(const QString &rawPath) const;
    bool routeMatches(const QString &candidatePath, const QString &currentPath) const;
    static bool isNotFoundRoute(const QString &path);
    static QString stripNotFoundPrefix(const QString &path);
    QVariantList normalizedDefinitions(const QVariantList &definitions, const QVariantList &fallback) const;
    QVariantList defaultNavDefinitions() const;
    QVariantList defaultScaffoldDefinitions() const;
    qreal clampedProgress(qreal value) const;
    QString routeForScaffoldIndex(int index) const;
    int routeVisitCount(const QString &path) const;
    QString badgeForRoute(const QString &path, const QString &fallbackBadge) const;

    void rebuildNavItems();
    void rebuildScaffoldNavModel();
    void rebuildDemoListItems();
    void rebuildAllModels();
    void recalculateRouteCountsFromHistory();
    void updateScaffoldIndexFromRoute();

    QVariantList m_navItems;
    QVariantList m_scaffoldNavModel;
    QVariantList m_demoListItems;
    QVariantList m_demoContextMenuItems;
    QVariantList m_navDefinitions;
    QVariantList m_scaffoldDefinitions;

    bool m_alertOpen = false;
    qreal m_progressStart = 0.0;
    qreal m_progressEnd = 100.0;
    qreal m_progressCurrent = 46.0;
    QString m_currentRoute;
    int m_scaffoldNavIndex = 0;
    int m_hierarchyActiveButtonId = 1;

    QVariantMap m_runtimeSnapshot;
    QVariantMap m_viewStateSnapshot;
    QStringList m_pageHistory;
    QVariantMap m_routeVisitCounts;
};
