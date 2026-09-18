#pragma once

#include <QObject>
#include <QHash>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QtQml/qqml.h>

#include "backend/navigation/routematcher.h"

class RouteResolver : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RouteResolver)

    Q_PROPERTY(QStringList routePatterns READ routePatterns WRITE setRoutePatterns NOTIFY routePatternsChanged)
    Q_PROPERTY(int cacheCapacity READ cacheCapacity WRITE setCacheCapacity NOTIFY cacheCapacityChanged)

public:
    explicit RouteResolver(QObject *parent = nullptr);

    QStringList routePatterns() const;
    int cacheCapacity() const;

    Q_INVOKABLE QVariantMap resolve(const QString &path);

public slots:
    void setRoutePatterns(const QStringList &patterns);
    void setCacheCapacity(int value);
    void updateRoutes(const QVariant &routes);
    void clearCache();

signals:
    void routePatternsChanged();
    void cacheCapacityChanged();

private:
    static int boundedCapacity(int value);
    static QString normalizedPathToken(const QVariant &rawPath);

    QVariantMap resolveUncached(const QString &normalizedPath) const;
    QStringList extractRoutePatterns(const QVariant &routes) const;
    QStringList extractPatternsFromModelObject(QObject *model) const;
    QString extractPathFromEntry(const QVariant &entry) const;

    void touchCacheKey(const QString &key);
    void putCacheEntry(const QString &key, const QVariantMap &entry);

    QStringList m_routePatterns;
    int m_cacheCapacity = 256;
    QHash<QString, QVariantMap> m_cache;
    QStringList m_cacheOrder;
    RouteMatcher m_matcher;
};
