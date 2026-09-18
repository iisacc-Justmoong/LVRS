#pragma once

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QVariantMap>
#include <QtQml/qqml.h>

class QAbstractItemModel;

class ModelSource : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ModelSource)

    Q_PROPERTY(QVariant source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int column READ column WRITE setColumn NOTIFY columnChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(bool itemModel READ itemModel NOTIFY sourceChanged)

public:
    explicit ModelSource(QObject *parent = nullptr);

    QVariant source() const;
    void setSource(const QVariant &source);

    int column() const;
    void setColumn(int column);

    int count() const;
    int revision() const;
    bool itemModel() const;

    Q_INVOKABLE QVariant at(int index) const;
    Q_INVOKABLE QVariantMap row(int index) const;
    Q_INVOKABLE QVariant roleValue(const QVariant &entry,
                                   const QString &roleName,
                                   const QVariant &fallbackValue = QVariant()) const;
    Q_INVOKABLE QString textValue(const QVariant &entry,
                                  const QVariant &roleNames,
                                  const QString &fallbackValue = QString()) const;
    Q_INVOKABLE bool boolValue(const QVariant &entry,
                               const QString &roleName,
                               bool fallbackValue = false) const;
    Q_INVOKABLE int intValue(const QVariant &entry,
                             const QString &roleName,
                             int fallbackValue = 0) const;
    Q_INVOKABLE void invalidate();

signals:
    void sourceChanged();
    void columnChanged();
    void countChanged();
    void revisionChanged();

private:
    static QAbstractItemModel *itemModelFromVariant(const QVariant &value);
    static QObject *objectFromVariant(const QVariant &value);
    static int countForVariant(const QVariant &value);
    static QVariant listValueAt(const QVariant &value, int index);
    static QStringList roleListFromVariant(const QVariant &value);
    void reconnectSourceSignals();

    QVariant m_source;
    QPointer<QObject> m_observedObject;
    QList<QMetaObject::Connection> m_sourceConnections;
    int m_column = 0;
    int m_revision = 0;
};
