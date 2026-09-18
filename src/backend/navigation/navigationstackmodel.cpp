#include "backend/navigation/navigationstackmodel.h"

#include "backend/navigation/routematcher.h"

#include <QJSValue>

NavigationStackModel::NavigationStackModel(QObject *parent)
    : QObject(parent)
{
    refreshDerivedState();
}

QVariant NavigationStackModel::path() const
{
    return m_path;
}

void NavigationStackModel::setPath(const QVariant &value)
{
    replacePath(listFromVariant(value));
}

QString NavigationStackModel::currentPath() const
{
    return m_currentPath;
}

QVariant NavigationStackModel::currentParams() const
{
    return m_currentParams;
}

QVariantList NavigationStackModel::viewTrackingEntries() const
{
    return m_viewTrackingEntries;
}

QStringList NavigationStackModel::trackedViewIds() const
{
    return m_trackedViewIds;
}

int NavigationStackModel::depth() const
{
    return m_path.size();
}

QString NavigationStackModel::normalizePath(const QVariant &pathValue) const
{
    RouteMatcher matcher;
    return matcher.normalizePath(pathValue.isValid() && !pathValue.isNull() ? pathValue.toString() : QString());
}

QVariantMap NavigationStackModel::createPathEntry(const QVariant &pathValue, const QVariant &params) const
{
    const QVariantMap pathMap = mapFromVariant(pathValue);
    if (!pathMap.isEmpty() && pathMap.contains(QStringLiteral("path"))) {
        return QVariantMap {
            {QStringLiteral("path"), pathMap.value(QStringLiteral("path")).toString().isEmpty()
                ? QString()
                : normalizePath(pathMap.value(QStringLiteral("path")))},
            {QStringLiteral("params"), pathMap.contains(QStringLiteral("params"))
                ? pathMap.value(QStringLiteral("params"))
                : QVariantMap()}
        };
    }

    if (!pathValue.isValid() || pathValue.isNull() || pathValue.toString().isEmpty()) {
        return QVariantMap {
            {QStringLiteral("path"), QString()},
            {QStringLiteral("params"), paramsOrEmpty(params)}
        };
    }

    return QVariantMap {
        {QStringLiteral("path"), normalizePath(pathValue)},
        {QStringLiteral("params"), paramsOrEmpty(params)}
    };
}

QVariantMap NavigationStackModel::createComponentPathEntry(const QVariant &component, const QVariant &params) const
{
    return QVariantMap {
        {QStringLiteral("path"), QString()},
        {QStringLiteral("params"), paramsOrEmpty(params)},
        {QStringLiteral("component"), component}
    };
}

QVariantList NavigationStackModel::stackAfterPathOperation(const QVariant &pathValue,
                                                           const QVariant &params,
                                                           const QString &mode) const
{
    QVariantList next = m_path;
    const QVariantMap nextEntry = createPathEntry(pathValue, params);
    const QString normalizedMode = mode.trimmed().toLower();
    if (normalizedMode == QStringLiteral("set") || next.isEmpty()) {
        return QVariantList {nextEntry};
    }
    if (normalizedMode == QStringLiteral("replace")) {
        next[next.size() - 1] = nextEntry;
        return next;
    }
    next.append(nextEntry);
    return next;
}

QVariantList NavigationStackModel::stackAfterComponentOperation(const QVariant &component,
                                                                const QVariant &params,
                                                                const QString &mode) const
{
    QVariantList next = m_path;
    const QVariantMap nextEntry = createComponentPathEntry(component, params);
    const QString normalizedMode = mode.trimmed().toLower();
    if (normalizedMode == QStringLiteral("set") || next.isEmpty())
        return QVariantList {nextEntry};
    if (normalizedMode == QStringLiteral("replace")) {
        next[next.size() - 1] = nextEntry;
        return next;
    }
    next.append(nextEntry);
    return next;
}

QVariantList NavigationStackModel::stackAfterPop() const
{
    if (m_path.size() <= 1)
        return m_path;
    return m_path.mid(0, m_path.size() - 1);
}

QVariantList NavigationStackModel::stackAfterPopToRoot() const
{
    if (m_path.size() <= 1)
        return m_path;
    return QVariantList {m_path.first()};
}

void NavigationStackModel::applyPathOperation(const QVariant &pathValue, const QVariant &params, const QString &mode)
{
    replacePath(stackAfterPathOperation(pathValue, params, mode));
}

void NavigationStackModel::applyComponentOperation(const QVariant &component, const QVariant &params, const QString &mode)
{
    replacePath(stackAfterComponentOperation(component, params, mode));
}

void NavigationStackModel::pop()
{
    replacePath(stackAfterPop());
}

void NavigationStackModel::popToRoot()
{
    replacePath(stackAfterPopToRoot());
}

QVariantMap NavigationStackModel::currentEntryDescriptor() const
{
    if (m_path.isEmpty())
        return QVariantMap {{QStringLiteral("path"), QString()}, {QStringLiteral("params"), QVariantMap()}};
    const QVariant entry = m_path.last();
    if (entry.metaType().id() == QMetaType::QString)
        return QVariantMap {{QStringLiteral("path"), normalizePath(entry)}, {QStringLiteral("params"), QVariantMap()}};
    const QVariantMap map = mapFromVariant(entry);
    return QVariantMap {
        {QStringLiteral("path"), map.value(QStringLiteral("path")).toString().isEmpty()
            ? QString()
            : normalizePath(map.value(QStringLiteral("path")))},
        {QStringLiteral("params"), map.value(QStringLiteral("params"), QVariantMap())},
        {QStringLiteral("component"), map.value(QStringLiteral("component"))}
    };
}

QVariantMap NavigationStackModel::createViewTrackingEntry(const QVariant &entry, int index) const
{
    if (entry.metaType().id() == QMetaType::QString) {
        const QString normalized = normalizePath(entry);
        return QVariantMap {
            {QStringLiteral("viewId"), normalized},
            {QStringLiteral("path"), normalized},
            {QStringLiteral("enabled"), true}
        };
    }

    const QVariantMap map = mapFromVariant(entry);
    if (map.isEmpty())
        return {};

    QString pathValue;
    if (map.contains(QStringLiteral("path")) && !map.value(QStringLiteral("path")).toString().isEmpty())
        pathValue = normalizePath(map.value(QStringLiteral("path")));

    QString viewId = map.value(QStringLiteral("viewId")).toString().trimmed();
    if (viewId.isEmpty() && !pathValue.isEmpty())
        viewId = pathValue;
    if (viewId.isEmpty())
        viewId = QStringLiteral("_component_%1").arg(index);

    bool enabled = true;
    if (map.contains(QStringLiteral("enabled")))
        enabled = map.value(QStringLiteral("enabled")).toBool();
    if (map.value(QStringLiteral("disabled")).toBool())
        enabled = false;
    const QVariantMap params = map.value(QStringLiteral("params")).toMap();
    if (params.value(QStringLiteral("disabled")).toBool())
        enabled = false;

    return QVariantMap {
        {QStringLiteral("viewId"), viewId},
        {QStringLiteral("path"), pathValue},
        {QStringLiteral("enabled"), enabled}
    };
}

QVariantList NavigationStackModel::buildViewTrackingEntries(const QVariant &pathValue) const
{
    const QVariantList source = pathValue.isValid() && !pathValue.isNull()
        ? listFromVariant(pathValue)
        : m_path;
    QVariantList entries;
    entries.reserve(source.size());
    for (int index = 0; index < source.size(); ++index) {
        const QVariantMap entry = createViewTrackingEntry(source.at(index), index);
        if (!entry.isEmpty())
            entries.append(entry);
    }
    return entries;
}

QStringList NavigationStackModel::updateTrackedViewIds(const QVariantList &entries)
{
    QStringList nextIds;
    for (const QVariant &entryValue : entries) {
        const QString viewId = entryValue.toMap().value(QStringLiteral("viewId")).toString();
        if (!viewId.isEmpty() && !nextIds.contains(viewId))
            nextIds.append(viewId);
    }
    const QStringList previousIds = m_trackedViewIds;
    if (m_trackedViewIds != nextIds) {
        m_trackedViewIds = nextIds;
        emit trackedViewIdsChanged();
    }

    QStringList removedIds;
    for (const QString &id : previousIds) {
        if (!nextIds.contains(id))
            removedIds.append(id);
    }
    return removedIds;
}

QVariantList NavigationStackModel::listFromVariant(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return {};
    if (value.userType() == qMetaTypeId<QJSValue>())
        return value.value<QJSValue>().toVariant().toList();
    if (value.canConvert<QVariantList>())
        return value.toList();
    return {};
}

QVariantMap NavigationStackModel::mapFromVariant(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return {};
    if (value.userType() == qMetaTypeId<QJSValue>())
        return value.value<QJSValue>().toVariant().toMap();
    if (value.canConvert<QVariantMap>())
        return value.toMap();
    return {};
}

QVariant NavigationStackModel::paramsOrEmpty(const QVariant &params)
{
    return params.isValid() && !params.isNull() ? params : QVariantMap();
}

void NavigationStackModel::replacePath(const QVariantList &nextPath)
{
    if (m_path == nextPath)
        return;
    m_path = nextPath;
    emit pathChanged();
    refreshDerivedState();
}

void NavigationStackModel::refreshDerivedState()
{
    const QString previousCurrentPath = m_currentPath;
    const QVariant previousCurrentParams = m_currentParams;
    const QVariantList previousEntries = m_viewTrackingEntries;

    const QVariantMap current = currentEntryDescriptor();
    m_currentPath = current.value(QStringLiteral("path")).toString();
    m_currentParams = current.value(QStringLiteral("params"), QVariantMap());
    m_viewTrackingEntries = buildViewTrackingEntries();

    if (previousCurrentPath != m_currentPath || previousCurrentParams != m_currentParams)
        emit currentChanged();
    if (previousEntries != m_viewTrackingEntries)
        emit viewTrackingEntriesChanged();
}
