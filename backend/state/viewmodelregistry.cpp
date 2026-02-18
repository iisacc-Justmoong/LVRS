#include "backend/state/viewmodelregistry.h"

#include <QByteArray>
#include <QSet>

ViewModelRegistry::ViewModelRegistry(QObject *parent)
    : QObject(parent)
{
}

QObject *ViewModelRegistry::get(const QString &key) const
{
    const QString normalized = normalizeToken(key);
    if (normalized.isEmpty())
        return nullptr;

    auto it = m_entries.constFind(normalized);
    if (it == m_entries.constEnd())
        return nullptr;
    return it.value();
}

void ViewModelRegistry::set(const QString &key, QObject *object)
{
    const QString normalized = normalizeToken(key);
    if (normalized.isEmpty())
        return;

    QObject *previous = nullptr;
    auto existing = m_entries.constFind(normalized);
    if (existing != m_entries.constEnd())
        previous = existing.value();

    if (object && !object->parent())
        object->setParent(const_cast<ViewModelRegistry *>(this));

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

    if (changed && previous != object)
        maybeDisposeOwned(previous, normalized);
    prune();
}

void ViewModelRegistry::remove(const QString &key)
{
    const QString normalized = normalizeToken(key);
    if (normalized.isEmpty())
        return;

    auto it = m_entries.find(normalized);
    if (it == m_entries.end())
        return;

    QObject *object = it.value();
    m_entries.erase(it);
    emit keysChanged();
    pruneBindingsAndOwners(m_entries.keys());
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

    if (hadEntries)
        emit keysChanged();
    if (hadBindings)
        emit viewsChanged();
    if (hadOwners)
        emit ownershipChanged();

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

    if (!keyExists(normalizedKey)) {
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
    if (!keyExists(targetKey))
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

    if (!canWrite(normalizedView, targetKey)) {
        setLastError(QStringLiteral("View has no write permission for the model"));
        return false;
    }

    QObject *object = get(targetKey);
    if (!object) {
        setLastError(QStringLiteral("ViewModel key is not registered"));
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

    QObject *object = getForView(viewId);
    if (!object)
        return QVariant();

    const QByteArray propertyName = normalizedProperty.toUtf8();
    return object->property(propertyName.constData());
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

void ViewModelRegistry::pruneBindingsAndOwners(const QStringList &validKeys)
{
    bool bindingsChanged = false;
    for (auto it = m_viewBindings.begin(); it != m_viewBindings.end(); ) {
        if (!validKeys.contains(it.value())) {
            it = m_viewBindings.erase(it);
            bindingsChanged = true;
        } else {
            ++it;
        }
    }

    bool ownershipChangedFlag = false;
    for (auto it = m_owners.begin(); it != m_owners.end(); ) {
        if (!validKeys.contains(it.key())) {
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
}

bool ViewModelRegistry::keyExists(const QString &key) const
{
    auto it = m_entries.constFind(key);
    return it != m_entries.constEnd() && it.value();
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
    pruneBindingsAndOwners(m_entries.keys());
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
