#pragma once

#include <QObject>
#include <QVariantMap>
#include <QtQml/qqml.h>

class ViewModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ViewModel)
    QML_UNCREATABLE("ViewModel is a C++ base type")

    Q_PROPERTY(QString key READ key WRITE setKey NOTIFY keyChanged)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(bool busy READ busy WRITE setBusy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error WRITE setError NOTIFY errorChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QVariantMap metadata READ metadata WRITE setMetadata NOTIFY metadataChanged)

public:
    explicit ViewModel(QObject *parent = nullptr);

    QString key() const;
    void setKey(const QString &value);

    QString displayName() const;
    void setDisplayName(const QString &value);

    bool busy() const;
    void setBusy(bool value);

    QString error() const;
    void setError(const QString &value);
    bool hasError() const;

    QVariantMap metadata() const;
    void setMetadata(const QVariantMap &value);

    Q_INVOKABLE void clearError();
    Q_INVOKABLE QVariantMap snapshot() const;

signals:
    void keyChanged();
    void displayNameChanged();
    void busyChanged();
    void errorChanged();
    void metadataChanged();

private:
    QString m_key;
    QString m_displayName;
    bool m_busy = false;
    QString m_error;
    QVariantMap m_metadata;
};
