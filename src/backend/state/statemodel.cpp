#include "backend/state/statemodel.h"

#include <QSet>

StateModel::StateModel(QObject *parent)
    : ViewModel(parent)
{
}

QVariantMap StateModel::values() const
{
    return m_values;
}

void StateModel::setValues(const QVariantMap &values)
{
    const QVariantMap next = normalizedValues(values);
    if (m_values == next)
        return;

    const QStringList previousKeys = stateKeys();
    const QVariantMap previous = m_values;
    m_values = next;
    bumpRevision();
    emit valuesChanged();
    if (previousKeys != stateKeys())
        emit stateKeysChanged();
    emitChangedKeys(previous, m_values);
}

QStringList StateModel::stateKeys() const
{
    QStringList keys = m_values.keys();
    keys.sort();
    return keys;
}

int StateModel::revision() const
{
    return m_revision;
}

bool StateModel::empty() const
{
    return m_values.isEmpty();
}

QVariant StateModel::value(const QString &key, const QVariant &fallbackValue) const
{
    return valueOr(key, fallbackValue);
}

QVariant StateModel::valueOr(const QString &key, const QVariant &fallbackValue) const
{
    const QString normalized = normalizedKey(key);
    if (normalized.isEmpty())
        return fallbackValue;
    return m_values.value(normalized, fallbackValue);
}

bool StateModel::hasValue(const QString &key) const
{
    const QString normalized = normalizedKey(key);
    return !normalized.isEmpty() && m_values.contains(normalized);
}

bool StateModel::setValue(const QString &key, const QVariant &value)
{
    const QString normalized = normalizedKey(key);
    if (normalized.isEmpty()) {
        setError(QStringLiteral("Empty state key"));
        return false;
    }

    setError(QString());
    const QVariant previous = m_values.value(normalized);
    const bool hadKey = m_values.contains(normalized);
    if (hadKey && previous == value)
        return true;

    const QStringList previousKeys = stateKeys();
    m_values.insert(normalized, value);
    bumpRevision();
    emit valuesChanged();
    if (previousKeys != stateKeys())
        emit stateKeysChanged();
    emit valueChanged(normalized, value, previous);
    return true;
}

bool StateModel::removeValue(const QString &key)
{
    const QString normalized = normalizedKey(key);
    if (normalized.isEmpty()) {
        setError(QStringLiteral("Empty state key"));
        return false;
    }

    setError(QString());
    if (!m_values.contains(normalized))
        return false;

    const QVariant previous = m_values.value(normalized);
    m_values.remove(normalized);
    bumpRevision();
    emit valuesChanged();
    emit stateKeysChanged();
    emit valueChanged(normalized, QVariant(), previous);
    return true;
}

bool StateModel::applyPatch(const QVariantMap &patch)
{
    const QVariantMap normalizedPatch = normalizedValues(patch);
    if (normalizedPatch.isEmpty())
        return false;

    QVariantMap next = m_values;
    bool changed = false;
    for (auto it = normalizedPatch.constBegin(); it != normalizedPatch.constEnd(); ++it) {
        if (next.value(it.key()) == it.value() && next.contains(it.key()))
            continue;
        next.insert(it.key(), it.value());
        changed = true;
    }

    if (!changed)
        return true;

    setValues(next);
    return true;
}

void StateModel::clearValues()
{
    if (m_values.isEmpty())
        return;

    const QVariantMap previous = m_values;
    m_values.clear();
    bumpRevision();
    emit valuesChanged();
    emit stateKeysChanged();
    emitChangedKeys(previous, m_values);
}

QVariantMap StateModel::stateSnapshot() const
{
    QVariantMap snapshot = ViewModel::snapshot();
    snapshot.insert(QStringLiteral("values"), m_values);
    snapshot.insert(QStringLiteral("stateKeys"), stateKeys());
    snapshot.insert(QStringLiteral("revision"), m_revision);
    snapshot.insert(QStringLiteral("empty"), empty());
    return snapshot;
}

QString StateModel::normalizedKey(const QString &key)
{
    return key.trimmed();
}

QVariantMap StateModel::normalizedValues(const QVariantMap &values)
{
    QVariantMap normalized;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        const QString key = normalizedKey(it.key());
        if (!key.isEmpty())
            normalized.insert(key, it.value());
    }
    return normalized;
}

void StateModel::bumpRevision()
{
    m_revision += 1;
    emit revisionChanged();
}

void StateModel::emitChangedKeys(const QVariantMap &previousValues, const QVariantMap &nextValues)
{
    QSet<QString> keys;
    for (auto it = previousValues.constBegin(); it != previousValues.constEnd(); ++it)
        keys.insert(it.key());
    for (auto it = nextValues.constBegin(); it != nextValues.constEnd(); ++it)
        keys.insert(it.key());

    QStringList sortedKeys = keys.values();
    sortedKeys.sort();
    for (const QString &key : sortedKeys) {
        const QVariant previous = previousValues.value(key);
        const QVariant next = nextValues.value(key);
        if (previousValues.contains(key) == nextValues.contains(key) && previous == next)
            continue;
        emit valueChanged(key, next, previous);
    }
}
