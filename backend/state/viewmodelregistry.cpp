#include "backend/state/viewmodelregistry.h"

#include "backend/state/statemodel.h"
#include "backend/state/viewmodel.h"

#include <QByteArray>
#include <QJSValue>
#include <QSet>
#include <QVariantList>

#include <utility>

namespace {

QVariant entryValueByKey(const QVariant &entry, const QString &key)
{
    if (!entry.isValid() || entry.isNull() || key.isEmpty())
        return QVariant();

    if (entry.userType() == qMetaTypeId<QJSValue>()) {
        const QJSValue value = entry.value<QJSValue>();
        if (value.isObject())
            return value.property(key).toVariant();
        return QVariant();
    }

    if (entry.canConvert<QVariantMap>())
        return entry.toMap().value(key);

    if (entry.canConvert<QObject *>()) {
        QObject *object = entry.value<QObject *>();
        if (!object)
            return QVariant();
        return object->property(key.toUtf8().constData());
    }

    return QVariant();
}

QString firstNonEmptyToken(const QVariant &entry, const QStringList &keys)
{
    for (const QString &key : keys) {
        const QString token = entryValueByKey(entry, key).toString().trimmed();
        if (!token.isEmpty())
            return token;
    }
    return QString();
}

bool firstBoolToken(const QVariant &entry, const QStringList &keys, bool *value)
{
    for (const QString &key : keys) {
        const QVariant candidate = entryValueByKey(entry, key);
        if (!candidate.isValid() || candidate.isNull())
            continue;
        if (value)
            *value = candidate.toBool();
        return true;
    }
    return false;
}

} // namespace

ViewModelRegistry::ViewModelRegistry(QObject *parent)
    : QObject(parent)
{
}

QObject *ViewModelRegistry::get(const QString &key) const
{
    const QString normalized = normalizeToken(key);
    if (normalized.isEmpty())
        return nullptr;

    return objectForKey(normalized);
}

void ViewModelRegistry::set(const QString &key, QObject *object)
{
    setLastError(QString());
    const QString normalized = normalizeToken(key);
    if (normalized.isEmpty()) {
        setLastError(QStringLiteral("Empty view model key"));
        return;
    }

    QObject *previous = nullptr;
    auto existing = m_entries.constFind(normalized);
    if (existing != m_entries.constEnd())
        previous = existing.value();

    if (object && !object->parent())
        object->setParent(const_cast<ViewModelRegistry *>(this));
    if (auto *viewModel = qobject_cast<ViewModel *>(object)) {
        if (viewModel->key().isEmpty())
            viewModel->setKey(normalized);
        observeDescriptorObject(viewModel);
    } else {
        observeDescriptorObject(object);
    }

    bool changed = false;
    auto it = m_entries.find(normalized);
    if (it == m_entries.end()) {
        m_entries.insert(normalized, object);
        changed = true;
    } else if (it.value() != object) {
        it.value() = object;
        changed = true;
    }

    if (changed)
        emit keysChanged();
    if (changed)
        emit descriptorsChanged();

    if (changed && previous != object) {
        releaseDescriptorObjectIfUnreferenced(previous);
        maybeDisposeOwned(previous, normalized);
    }
    prune();
}

bool ViewModelRegistry::registerViewModel(QObject *object, const QString &fallbackKey)
{
    setLastError(QString());
    if (!object) {
        setLastError(QStringLiteral("ViewModel object is null"));
        return false;
    }

    QString key = normalizeToken(fallbackKey);
    if (auto *viewModel = qobject_cast<ViewModel *>(object)) {
        if (key.isEmpty())
            key = normalizeToken(viewModel->key());
        if (key.isEmpty()) {
            setLastError(QStringLiteral("ViewModel key is empty"));
            return false;
        }
        if (viewModel->key() != key)
            viewModel->setKey(key);
    }

    if (key.isEmpty()) {
        setLastError(QStringLiteral("ViewModel key is empty"));
        return false;
    }

    set(key, object);
    return lastError().isEmpty();
}

void ViewModelRegistry::remove(const QString &key)
{
    setLastError(QString());
    const QString normalized = normalizeToken(key);
    if (normalized.isEmpty())
        return;

    auto it = m_entries.find(normalized);
    if (it == m_entries.end())
        return;

    QObject *object = it.value();
    m_entries.erase(it);
    emit keysChanged();
    emit descriptorsChanged();
    pruneBindingsAndOwners();
    releaseDescriptorObjectIfUnreferenced(object);
    maybeDisposeOwned(object);
}

void ViewModelRegistry::clear()
{
    if (m_entries.isEmpty() && m_viewBindings.isEmpty() && m_owners.isEmpty())
        return;

    const auto entries = m_entries;
    const bool hadEntries = !m_entries.isEmpty();
    const bool hadBindings = !m_viewBindings.isEmpty();
    const bool hadOwners = !m_owners.isEmpty();

    m_entries.clear();
    m_viewBindings.clear();
    m_owners.clear();
    for (const QList<QMetaObject::Connection> &connections : std::as_const(m_descriptorConnections)) {
        for (const QMetaObject::Connection &connection : connections)
            disconnect(connection);
    }
    m_descriptorConnections.clear();

    if (hadEntries)
        emit keysChanged();
    if (hadBindings)
        emit viewsChanged();
    if (hadOwners)
        emit ownershipChanged();
    if (hadEntries || hadBindings || hadOwners)
        emit descriptorsChanged();

    QSet<QObject *> processed;
    for (auto it = entries.begin(); it != entries.end(); ++it) {
        QObject *object = it.value();
        if (!object || processed.contains(object))
            continue;
        processed.insert(object);
        maybeDisposeOwned(object);
    }
}

QStringList ViewModelRegistry::keys() const
{
    return m_entries.keys();
}

bool ViewModelRegistry::bindView(const QString &viewId, const QString &key, bool writable)
{
    setLastError(QString());
    prune();

    const QString normalizedView = normalizeToken(viewId);
    if (normalizedView.isEmpty()) {
        setLastError(QStringLiteral("Empty view id"));
        return false;
    }

    const QString normalizedKey = normalizeToken(key);
    if (normalizedKey.isEmpty()) {
        setLastError(QStringLiteral("Empty view model key"));
        return false;
    }

    if (!objectForKey(normalizedKey)) {
        setLastError(QStringLiteral("ViewModel key is not registered"));
        return false;
    }

    const QString previousKey = m_viewBindings.value(normalizedView);
    const QString currentOwner = m_owners.value(normalizedKey);

    if (writable && !currentOwner.isEmpty() && currentOwner != normalizedView) {
        setLastError(QStringLiteral("ViewModel is already owned by another view"));
        return false;
    }

    bool ownershipChangedFlag = false;
    if (!previousKey.isEmpty() && previousKey != normalizedKey) {
        auto previousOwnerIt = m_owners.find(previousKey);
        if (previousOwnerIt != m_owners.end() && previousOwnerIt.value() == normalizedView) {
            m_owners.erase(previousOwnerIt);
            ownershipChangedFlag = true;
        }
    }

    if (writable) {
        if (currentOwner != normalizedView) {
            m_owners.insert(normalizedKey, normalizedView);
            ownershipChangedFlag = true;
        }
    } else {
        auto ownerIt = m_owners.find(normalizedKey);
        if (ownerIt != m_owners.end() && ownerIt.value() == normalizedView) {
            m_owners.erase(ownerIt);
            ownershipChangedFlag = true;
        }
    }

    bool bindingsChanged = false;
    if (previousKey != normalizedKey) {
        m_viewBindings.insert(normalizedView, normalizedKey);
        bindingsChanged = true;
    }

    if (bindingsChanged)
        emit viewsChanged();
    if (ownershipChangedFlag)
        emit ownershipChanged();
    if (bindingsChanged || ownershipChangedFlag)
        emit descriptorsChanged();
    return true;
}

void ViewModelRegistry::unbindView(const QString &viewId)
{
    const QString normalizedView = normalizeToken(viewId);
    if (normalizedView.isEmpty())
        return;

    const bool bindingsChanged = m_viewBindings.remove(normalizedView) > 0;

    bool ownershipChangedFlag = false;
    for (auto it = m_owners.begin(); it != m_owners.end(); ) {
        if (it.value() == normalizedView) {
            it = m_owners.erase(it);
            ownershipChangedFlag = true;
        } else {
            ++it;
        }
    }

    if (bindingsChanged)
        emit viewsChanged();
    if (ownershipChangedFlag)
        emit ownershipChanged();
    if (bindingsChanged || ownershipChangedFlag)
        emit descriptorsChanged();
}

QObject *ViewModelRegistry::getForView(const QString &viewId) const
{
    return get(keyForView(viewId));
}

QString ViewModelRegistry::keyForView(const QString &viewId) const
{
    const QString normalizedView = normalizeToken(viewId);
    if (normalizedView.isEmpty())
        return QString();
    return m_viewBindings.value(normalizedView);
}

bool ViewModelRegistry::claimOwnership(const QString &viewId, const QString &key)
{
    return bindView(viewId, key, true);
}

bool ViewModelRegistry::releaseOwnership(const QString &viewId, const QString &key)
{
    setLastError(QString());

    const QString normalizedView = normalizeToken(viewId);
    if (normalizedView.isEmpty()) {
        setLastError(QStringLiteral("Empty view id"));
        return false;
    }

    const QString normalizedKey = normalizeToken(key);
    bool changed = false;

    if (normalizedKey.isEmpty()) {
        for (auto it = m_owners.begin(); it != m_owners.end(); ) {
            if (it.value() == normalizedView) {
                it = m_owners.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }
    } else {
        auto it = m_owners.find(normalizedKey);
        if (it != m_owners.end() && it.value() == normalizedView) {
            m_owners.erase(it);
            changed = true;
        }
    }

    if (!changed) {
        setLastError(QStringLiteral("Ownership not found"));
        return false;
    }

    emit ownershipChanged();
    emit descriptorsChanged();
    return true;
}

bool ViewModelRegistry::canWrite(const QString &viewId, const QString &key) const
{
    const QString normalizedView = normalizeToken(viewId);
    if (normalizedView.isEmpty())
        return false;

    const QString targetKey = resolveKeyForWrite(normalizedView, key);
    if (targetKey.isEmpty())
        return false;
    if (!objectForKey(targetKey))
        return false;

    const QString owner = m_owners.value(targetKey);
    return !owner.isEmpty() && owner == normalizedView;
}

QString ViewModelRegistry::ownerOf(const QString &key) const
{
    const QString normalizedKey = normalizeToken(key);
    if (normalizedKey.isEmpty())
        return QString();
    return m_owners.value(normalizedKey);
}

bool ViewModelRegistry::updateProperty(const QString &viewId, const QString &property, const QVariant &value)
{
    return updatePropertyByKey(viewId, QString(), property, value);
}

bool ViewModelRegistry::updatePropertyByKey(const QString &viewId,
                                            const QString &key,
                                            const QString &property,
                                            const QVariant &value)
{
    setLastError(QString());

    const QString normalizedView = normalizeToken(viewId);
    if (normalizedView.isEmpty()) {
        setLastError(QStringLiteral("Empty view id"));
        return false;
    }

    const QString targetKey = resolveKeyForWrite(normalizedView, key);
    if (targetKey.isEmpty()) {
        setLastError(QStringLiteral("View is not bound to a model"));
        return false;
    }

    QObject *object = objectForKey(targetKey);
    if (!object || m_owners.value(targetKey) != normalizedView) {
        setLastError(QStringLiteral("View has no write permission for the model"));
        return false;
    }

    const QString normalizedProperty = normalizeToken(property);
    if (normalizedProperty.isEmpty()) {
        setLastError(QStringLiteral("Empty property name"));
        return false;
    }

    const QByteArray propertyName = normalizedProperty.toUtf8();
    if (!object->setProperty(propertyName.constData(), value)) {
        setLastError(QStringLiteral("Failed to update model property"));
        return false;
    }
    return true;
}

QVariant ViewModelRegistry::readProperty(const QString &viewId, const QString &property) const
{
    const QString normalizedProperty = normalizeToken(property);
    if (normalizedProperty.isEmpty())
        return QVariant();

    const QString normalizedView = normalizeToken(viewId);
    if (normalizedView.isEmpty())
        return QVariant();

    const QString targetKey = m_viewBindings.value(normalizedView);
    if (targetKey.isEmpty())
        return QVariant();

    QObject *object = objectForKey(targetKey);
    if (!object)
        return QVariant();

    const QByteArray propertyName = normalizedProperty.toUtf8();
    return object->property(propertyName.constData());
}

bool ViewModelRegistry::bindRouteViewModel(const QVariant &pathValue,
                                           const QVariant &routeEntry,
                                           const QVariant &paramsValue,
                                           int fallbackIndex)
{
    const QString routeKey = firstNonEmptyToken(routeEntry,
                                                {QStringLiteral("viewModelKey"),
                                                 QStringLiteral("modelKey")});
    const QString paramsKey = firstNonEmptyToken(paramsValue,
                                                 {QStringLiteral("viewModelKey"),
                                                  QStringLiteral("modelKey")});
    const QString targetKey = !routeKey.isEmpty() ? routeKey : paramsKey;
    if (targetKey.isEmpty())
        return false;

    QString viewId = firstNonEmptyToken(routeEntry, {QStringLiteral("viewId")});
    if (viewId.isEmpty())
        viewId = firstNonEmptyToken(paramsValue, {QStringLiteral("viewId")});
    if (viewId.isEmpty())
        viewId = normalizeToken(pathValue.toString());
    if (viewId.isEmpty())
        viewId = QStringLiteral("_component_%1").arg(qMax(0, fallbackIndex));

    bool writable = false;
    bool hasWritable = firstBoolToken(routeEntry,
                                      {QStringLiteral("writable"),
                                       QStringLiteral("modelWritable")},
                                      &writable);
    if (!hasWritable) {
        firstBoolToken(paramsValue,
                       {QStringLiteral("writable"),
                        QStringLiteral("modelWritable")},
                       &writable);
    }

    return bindView(viewId, targetKey, writable);
}

QStringList ViewModelRegistry::views() const
{
    return m_viewBindings.keys();
}

QVariantMap ViewModelRegistry::bindings() const
{
    QVariantMap map;
    for (auto it = m_viewBindings.constBegin(); it != m_viewBindings.constEnd(); ++it)
        map.insert(it.key(), it.value());
    return map;
}

QVariantMap ViewModelRegistry::owners() const
{
    QVariantMap map;
    for (auto it = m_owners.constBegin(); it != m_owners.constEnd(); ++it)
        map.insert(it.key(), it.value());
    return map;
}

QVariantMap ViewModelRegistry::descriptor(const QString &key) const
{
    const QString normalizedKey = normalizeToken(key);
    QVariantMap map;
    if (normalizedKey.isEmpty())
        return map;

    QObject *object = objectForKey(normalizedKey);
    if (!object)
        return map;

    map.insert(QStringLiteral("key"), normalizedKey);
    map.insert(QStringLiteral("className"), QString::fromUtf8(object->metaObject()->className()));
    map.insert(QStringLiteral("owner"), m_owners.value(normalizedKey));

    QVariantList boundViews;
    for (auto it = m_viewBindings.constBegin(); it != m_viewBindings.constEnd(); ++it) {
        if (it.value() == normalizedKey)
            boundViews.append(it.key());
    }
    map.insert(QStringLiteral("views"), boundViews);

    if (auto *viewModel = qobject_cast<ViewModel *>(object)) {
        map.insert(QStringLiteral("viewModel"), true);
        map.insert(QStringLiteral("viewModelKey"), viewModel->key());
        map.insert(QStringLiteral("displayName"), viewModel->displayName());
        map.insert(QStringLiteral("busy"), viewModel->busy());
        map.insert(QStringLiteral("error"), viewModel->error());
        map.insert(QStringLiteral("hasError"), viewModel->hasError());
        map.insert(QStringLiteral("metadata"), viewModel->metadata());
    } else {
        map.insert(QStringLiteral("viewModel"), false);
    }

    if (auto *stateModel = qobject_cast<StateModel *>(object)) {
        map.insert(QStringLiteral("stateModel"), true);
        map.insert(QStringLiteral("values"), stateModel->values());
        map.insert(QStringLiteral("stateKeys"), stateModel->stateKeys());
        map.insert(QStringLiteral("revision"), stateModel->revision());
        map.insert(QStringLiteral("empty"), stateModel->empty());
    } else {
        map.insert(QStringLiteral("stateModel"), false);
    }

    return map;
}

QVariantMap ViewModelRegistry::descriptors() const
{
    QVariantMap map;
    for (const QString &key : m_entries.keys())
        map.insert(key, descriptor(key));
    return map;
}

QString ViewModelRegistry::lastError() const
{
    return m_lastError;
}

QString ViewModelRegistry::normalizeToken(const QString &value)
{
    return value.trimmed();
}

void ViewModelRegistry::setLastError(const QString &message)
{
    if (m_lastError == message)
        return;
    m_lastError = message;
    emit lastErrorChanged();
}

void ViewModelRegistry::pruneBindingsAndOwners()
{
    bool bindingsChanged = false;
    for (auto it = m_viewBindings.begin(); it != m_viewBindings.end(); ) {
        if (!objectForKey(it.value())) {
            it = m_viewBindings.erase(it);
            bindingsChanged = true;
        } else {
            ++it;
        }
    }

    bool ownershipChangedFlag = false;
    for (auto it = m_owners.begin(); it != m_owners.end(); ) {
        if (!objectForKey(it.key())) {
            it = m_owners.erase(it);
            ownershipChangedFlag = true;
        } else {
            ++it;
        }
    }

    if (bindingsChanged)
        emit viewsChanged();
    if (ownershipChangedFlag)
        emit ownershipChanged();
    if (bindingsChanged || ownershipChangedFlag)
        emit descriptorsChanged();
}

QString ViewModelRegistry::resolveKeyForWrite(const QString &viewId, const QString &key) const
{
    const QString normalizedKey = normalizeToken(key);
    if (!normalizedKey.isEmpty())
        return normalizedKey;

    const QString normalizedView = normalizeToken(viewId);
    if (normalizedView.isEmpty())
        return QString();
    return m_viewBindings.value(normalizedView);
}

void ViewModelRegistry::prune()
{
    bool changed = false;
    for (auto it = m_entries.begin(); it != m_entries.end(); ) {
        if (!it.value()) {
            it = m_entries.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }
    if (changed)
        emit keysChanged();
    if (changed)
        emit descriptorsChanged();
    pruneBindingsAndOwners();
}

QObject *ViewModelRegistry::objectForKey(const QString &normalizedKey) const
{
    auto it = m_entries.constFind(normalizedKey);
    if (it == m_entries.constEnd())
        return nullptr;
    return it.value();
}

bool ViewModelRegistry::hasReference(QObject *object, const QString &exceptKey) const
{
    if (!object)
        return false;
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        if (!exceptKey.isEmpty() && it.key() == exceptKey)
            continue;
        if (it.value() == object)
            return true;
    }
    return false;
}

void ViewModelRegistry::maybeDisposeOwned(QObject *object, const QString &exceptKey)
{
    if (!object)
        return;
    if (hasReference(object, exceptKey))
        return;
    if (object->parent() == this)
        object->deleteLater();
}

void ViewModelRegistry::observeDescriptorObject(QObject *object)
{
    if (!object || m_descriptorConnections.contains(object))
        return;

    QList<QMetaObject::Connection> connections;
    connections.append(connect(object, &QObject::destroyed, this, [this, object]() {
        m_descriptorConnections.remove(object);
        emit descriptorsChanged();
    }));

    if (auto *viewModel = qobject_cast<ViewModel *>(object)) {
        connections.append(connect(viewModel, &ViewModel::keyChanged, this, &ViewModelRegistry::descriptorsChanged));
        connections.append(connect(viewModel,
                                   &ViewModel::displayNameChanged,
                                   this,
                                   &ViewModelRegistry::descriptorsChanged));
        connections.append(connect(viewModel, &ViewModel::busyChanged, this, &ViewModelRegistry::descriptorsChanged));
        connections.append(connect(viewModel, &ViewModel::errorChanged, this, &ViewModelRegistry::descriptorsChanged));
        connections.append(connect(viewModel,
                                   &ViewModel::metadataChanged,
                                   this,
                                   &ViewModelRegistry::descriptorsChanged));
    }

    if (auto *stateModel = qobject_cast<StateModel *>(object)) {
        connections.append(connect(stateModel, &StateModel::valuesChanged, this, &ViewModelRegistry::descriptorsChanged));
        connections.append(connect(stateModel, &StateModel::stateKeysChanged, this, &ViewModelRegistry::descriptorsChanged));
        connections.append(connect(stateModel, &StateModel::revisionChanged, this, &ViewModelRegistry::descriptorsChanged));
    }

    m_descriptorConnections.insert(object, connections);
}

void ViewModelRegistry::releaseDescriptorObjectIfUnreferenced(QObject *object)
{
    if (!object || hasReference(object))
        return;

    const QList<QMetaObject::Connection> connections = m_descriptorConnections.take(object);
    for (const QMetaObject::Connection &connection : connections)
        disconnect(connection);
}
