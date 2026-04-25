#include "backend/state/viewmodel.h"

ViewModel::ViewModel(QObject *parent)
    : QObject(parent)
{
}

QString ViewModel::key() const
{
    return m_key;
}

void ViewModel::setKey(const QString &value)
{
    const QString next = value.trimmed();
    if (m_key == next)
        return;
    m_key = next;
    emit keyChanged();
}

QString ViewModel::displayName() const
{
    return m_displayName;
}

void ViewModel::setDisplayName(const QString &value)
{
    const QString next = value.trimmed();
    if (m_displayName == next)
        return;
    m_displayName = next;
    emit displayNameChanged();
}

bool ViewModel::busy() const
{
    return m_busy;
}

void ViewModel::setBusy(bool value)
{
    if (m_busy == value)
        return;
    m_busy = value;
    emit busyChanged();
}

QString ViewModel::error() const
{
    return m_error;
}

void ViewModel::setError(const QString &value)
{
    const QString next = value.trimmed();
    if (m_error == next)
        return;
    m_error = next;
    emit errorChanged();
}

bool ViewModel::hasError() const
{
    return !m_error.isEmpty();
}

QVariantMap ViewModel::metadata() const
{
    return m_metadata;
}

void ViewModel::setMetadata(const QVariantMap &value)
{
    if (m_metadata == value)
        return;
    m_metadata = value;
    emit metadataChanged();
}

void ViewModel::clearError()
{
    setError(QString());
}

QVariantMap ViewModel::snapshot() const
{
    QVariantMap map;
    map.insert(QStringLiteral("key"), m_key);
    map.insert(QStringLiteral("displayName"), m_displayName);
    map.insert(QStringLiteral("busy"), m_busy);
    map.insert(QStringLiteral("error"), m_error);
    map.insert(QStringLiteral("hasError"), hasError());
    map.insert(QStringLiteral("metadata"), m_metadata);
    return map;
}
