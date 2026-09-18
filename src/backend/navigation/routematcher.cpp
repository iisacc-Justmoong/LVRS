#include "backend/navigation/routematcher.h"

#include <QStringList>

RouteMatcher::RouteMatcher(QObject *parent)
    : QObject(parent)
{
}

QString RouteMatcher::normalizePath(const QString &path) const
{
    QString value = path.trimmed();
    if (value.isEmpty())
        value = QStringLiteral("/");

    if (!value.startsWith(QLatin1Char('/')))
        value.prepend(QLatin1Char('/'));

    while (value.length() > 1 && value.endsWith(QLatin1Char('/')))
        value.chop(1);

    return value;
}

QVariantMap RouteMatcher::match(const QString &path, const QString &routePath) const
{
    QVariantMap result;
    result.insert(QStringLiteral("matched"), false);
    result.insert(QStringLiteral("params"), QVariantMap());

    const QString normalizedPath = normalizePath(path);
    const QString normalizedRoute = normalizePath(routePath);

    if (normalizedRoute == QStringLiteral("/")) {
        const bool matchedRoot = normalizedPath == QStringLiteral("/");
        result.insert(QStringLiteral("matched"), matchedRoot);
        return result;
    }

    const QStringList pathSegments = splitSegments(normalizedPath);
    const QStringList routeSegments = splitSegments(normalizedRoute);

    QVariantMap params;
    int pathIndex = 0;
    for (int routeIndex = 0; routeIndex < routeSegments.size(); ++routeIndex) {
        const QString segment = routeSegments.at(routeIndex);
        const bool isParam = segment.startsWith(QLatin1Char('[')) && segment.endsWith(QLatin1Char(']'));
        if (!isParam) {
            if (pathIndex >= pathSegments.size() || pathSegments.at(pathIndex) != segment)
                return result;
            pathIndex += 1;
            continue;
        }

        QString key = segment.mid(1, segment.size() - 2);
        if (key.startsWith(QStringLiteral("..."))) {
            key = key.mid(3);
            params.insert(key, pathSegments.mid(pathIndex).join(QLatin1Char('/')));
            pathIndex = pathSegments.size();
            result.insert(QStringLiteral("matched"), true);
            result.insert(QStringLiteral("params"), params);
            return result;
        }

        if (pathIndex >= pathSegments.size())
            return result;
        params.insert(key, pathSegments.at(pathIndex));
        pathIndex += 1;
    }

    if (pathIndex != pathSegments.size())
        return result;

    result.insert(QStringLiteral("matched"), true);
    result.insert(QStringLiteral("params"), params);
    return result;
}

QStringList RouteMatcher::splitSegments(const QString &normalizedPath)
{
    if (normalizedPath == QStringLiteral("/"))
        return QStringList();
    return normalizedPath.mid(1).split(QLatin1Char('/'), Qt::KeepEmptyParts);
}

