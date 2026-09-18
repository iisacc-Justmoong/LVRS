#pragma once

#include "backend/state/viewmodel.h"

#include <QStringList>
#include <QVariant>
#include <QVariantMap>
#include <QtQml/qqml.h>

class StateModel : public ViewModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(StateModel)

    Q_PROPERTY(QVariantMap values READ values WRITE setValues NOTIFY valuesChanged)
    Q_PROPERTY(QStringList stateKeys READ stateKeys NOTIFY stateKeysChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY valuesChanged)

public:
    explicit StateModel(QObject *parent = nullptr);

    QVariantMap values() const;
    void setValues(const QVariantMap &values);

    QStringList stateKeys() const;
    int revision() const;
    bool empty() const;

    Q_INVOKABLE QVariant value(const QString &key, const QVariant &fallbackValue = QVariant()) const;
    Q_INVOKABLE QVariant valueOr(const QString &key, const QVariant &fallbackValue) const;
    Q_INVOKABLE bool hasValue(const QString &key) const;
    Q_INVOKABLE bool setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE bool removeValue(const QString &key);
    Q_INVOKABLE bool applyPatch(const QVariantMap &patch);
    Q_INVOKABLE void clearValues();
    Q_INVOKABLE QVariantMap stateSnapshot() const;

signals:
    void valuesChanged();
    void stateKeysChanged();
    void revisionChanged();
    void valueChanged(const QString &key, const QVariant &value, const QVariant &previousValue);

private:
    static QString normalizedKey(const QString &key);
    static QVariantMap normalizedValues(const QVariantMap &values);
    void bumpRevision();
    void emitChangedKeys(const QVariantMap &previousValues, const QVariantMap &nextValues);

    QVariantMap m_values;
    int m_revision = 0;
};
