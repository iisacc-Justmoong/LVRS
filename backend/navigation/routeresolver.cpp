#include "backend/navigation/routeresolver.h"

#include <QJSValue>
#include <QMetaObject>

RouteResolver::RouteResolver(QObject *parent)
    : QObject(parent)
{
}

QStringList RouteResolver::routePatterns() const
{
    return m_routePatterns;
}

int RouteResolver::cacheCapacity() const
{
    return m_cacheCapacity;
}

QVariantMap RouteResolver::resolve(const QString &path)
{
    const QString normalizedPath = m_matcher.normalizePath(path);

    const auto cacheIt = m_cache.constFind(normalizedPath);
    if (cacheIt != m_cache.constEnd()) {
        touchCacheKey(normalizedPath);
        return cacheIt.value();
    }

    const QVariantMap resolved = resolveUncached(normalizedPath);
    putCacheEntry(normalizedPath, resolved);
    return resolved;
}

void RouteResolver::setRoutePatterns(const QStringList &patterns)
{
    if (m_routePatterns == patterns)
        return;

    m_routePatterns = patterns;
    clearCache();
    emit routePatternsChanged();
}

void RouteResolver::setCacheCapacity(int value)
{
    const int next = boundedCapacity(value);
    if (m_cacheCapacity == next)
        return;

    m_cacheCapacity = next;

    while (m_cacheOrder.size() > m_cacheCapacity) {
        const QString oldest = m_cacheOrder.takeFirst();
        m_cache.remove(oldest);
    }

    emit cacheCapacityChanged();
}

void RouteResolver::updateRoutes(const QVariant &routes)
{
    setRoutePatterns(extractRoutePatterns(routes));
}

void RouteResolver::clearCache()
{
    if (m_cache.isEmpty() && m_cacheOrder.isEmpty())
        return;

    m_cache.clear();
    m_cacheOrder.clear();
}

int RouteResolver::boundedCapacity(int value)
{
    return qMax(16, value);
}

QString RouteResolver::normalizedPathToken(const QVariant &rawPath)
{
    if (!rawPath.isValid() || rawPath.isNull())
        return QString();

    return rawPath.toString().trimmed();
}

QVariantMap RouteResolver::resolveUncached(const QString &normalizedPath) const
{
    QVariantMap resolved;
    resolved.insert(QStringLiteral("matched"), false);
    resolved.insert(QStringLiteral("index"), -1);
    resolved.insert(QStringLiteral("params"), QVariantMap());

    for (int index = 0; index < m_routePatterns.size(); ++index) {
        const QString routePath = m_routePatterns.at(index);
        if (routePath.isEmpty())
            continue;

        const QVariantMap matchResult = m_matcher.match(normalizedPath, routePath);
        if (!matchResult.value(QStringLiteral("matched")).toBool())
            continue;

        resolved.insert(QStringLiteral("matched"), true);
        resolved.insert(QStringLiteral("index"), index);
        resolved.insert(QStringLiteral("params"), matchResult.value(QStringLiteral("params")).toMap());
        return resolved;
    }

    return resolved;
}

QStringList RouteResolver::extractRoutePatterns(const QVariant &routes) const
{
    if (!routes.isValid() || routes.isNull())
        return QStringList();

    if (routes.userType() == qMetaTypeId<QJSValue>()) {
        const QJSValue value = routes.value<QJSValue>();
        if (value.isArray()) {
            const int length = value.property(QStringLiteral("length")).toInt();
            QStringList patterns;
            patterns.reserve(length);
            for (int i = 0; i < length; ++i) {
                const QJSValue entry = value.property(i);
                patterns.push_back(extractPathFromEntry(QVariant::fromValue(entry)));
            }
            return patterns;
        }

        if (value.isQObject())
            return extractPatternsFromModelObject(value.toQObject());
    }

    if (routes.canConvert<QVariantList>()) {
        const QVariantList entries = routes.toList();
        QStringList patterns;
        patterns.reserve(entries.size());
        for (const QVariant &entry : entries)
            patterns.push_back(extractPathFromEntry(entry));
        return patterns;
    }

    if (routes.canConvert<QObject *>())
        return extractPatternsFromModelObject(routes.value<QObject *>());

    return QStringList();
}

QStringList RouteResolver::extractPatternsFromModelObject(QObject *model) const
{
    if (!model)
        return QStringList();

    const QVariant countValue = model->property("count");
    if (!countValue.isValid())
        return QStringList();

    const int count = qMax(0, countValue.toInt());
    QStringList patterns;
    patterns.reserve(count);

    for (int index = 0; index < count; ++index) {
        QVariant entry;
        const bool invoked = QMetaObject::invokeMethod(model,
                                                       "get",
                                                       Q_RETURN_ARG(QVariant, entry),
                                                       Q_ARG(int, index));
        if (!invoked)
            entry = QVariant();

        patterns.push_back(extractPathFromEntry(entry));
    }

    return patterns;
}

QString RouteResolver::extractPathFromEntry(const QVariant &entry) const
{
    if (!entry.isValid() || entry.isNull())
        return QString();

    if (entry.userType() == qMetaTypeId<QJSValue>()) {
        const QJSValue value = entry.value<QJSValue>();
        return normalizedPathToken(value.property(QStringLiteral("path")).toVariant());
    }

    if (entry.canConvert<QVariantMap>()) {
        const QVariantMap map = entry.toMap();
        return normalizedPathToken(map.value(QStringLiteral("path")));
    }

    if (entry.canConvert<QObject *>()) {
        QObject *object = entry.value<QObject *>();
        if (!object)
            return QString();
        return normalizedPathToken(object->property("path"));
    }

    return QString();
}

void RouteResolver::touchCacheKey(const QString &key)
{
    m_cacheOrder.removeAll(key);
    m_cacheOrder.push_back(key);
}

void RouteResolver::putCacheEntry(const QString &key, const QVariantMap &entry)
{
    m_cache.insert(key, entry);
    touchCacheKey(key);

    while (m_cacheOrder.size() > m_cacheCapacity) {
        const QString oldest = m_cacheOrder.takeFirst();
        m_cache.remove(oldest);
    }
}
