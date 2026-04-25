#pragma once

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QHash>
#include <QMetaObject>
#include <QVariant>
#include <QVariantMap>
#include <QtQml/qqml.h>

class ViewModel;

class ViewModelRegistry : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ViewModels)
    QML_SINGLETON

    Q_PROPERTY(QStringList keys READ keys NOTIFY keysChanged)
    Q_PROPERTY(QStringList views READ views NOTIFY viewsChanged)
    Q_PROPERTY(QVariantMap bindings READ bindings NOTIFY viewsChanged)
    Q_PROPERTY(QVariantMap owners READ owners NOTIFY ownershipChanged)
    Q_PROPERTY(QVariantMap descriptors READ descriptors NOTIFY descriptorsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit ViewModelRegistry(QObject *parent = nullptr);

    Q_INVOKABLE QObject *get(const QString &key) const;
    Q_INVOKABLE void set(const QString &key, QObject *object);
    Q_INVOKABLE bool registerViewModel(QObject *object, const QString &fallbackKey = QString());
    Q_INVOKABLE void remove(const QString &key);
    Q_INVOKABLE void clear();

    Q_INVOKABLE bool bindView(const QString &viewId, const QString &key, bool writable = false);
    Q_INVOKABLE void unbindView(const QString &viewId);
    Q_INVOKABLE QObject *getForView(const QString &viewId) const;
    Q_INVOKABLE QString keyForView(const QString &viewId) const;
    Q_INVOKABLE bool claimOwnership(const QString &viewId, const QString &key);
    Q_INVOKABLE bool releaseOwnership(const QString &viewId, const QString &key = QString());
    Q_INVOKABLE bool canWrite(const QString &viewId, const QString &key = QString()) const;
    Q_INVOKABLE QString ownerOf(const QString &key) const;
    Q_INVOKABLE bool updateProperty(const QString &viewId, const QString &property, const QVariant &value);
    Q_INVOKABLE bool updatePropertyByKey(const QString &viewId,
                                         const QString &key,
                                         const QString &property,
                                         const QVariant &value);
    Q_INVOKABLE QVariant readProperty(const QString &viewId, const QString &property) const;
    Q_INVOKABLE QVariantMap descriptor(const QString &key) const;
    Q_INVOKABLE bool bindRouteViewModel(const QVariant &pathValue,
                                        const QVariant &routeEntry,
                                        const QVariant &paramsValue,
                                        int fallbackIndex = 0);

    QStringList keys() const;
    QStringList views() const;
    QVariantMap bindings() const;
    QVariantMap owners() const;
    QVariantMap descriptors() const;
    QString lastError() const;

signals:
    void keysChanged();
    void viewsChanged();
    void ownershipChanged();
    void descriptorsChanged();
    void lastErrorChanged();

private:
    static QString normalizeToken(const QString &value);

    void setLastError(const QString &message);
    void pruneBindingsAndOwners();
    QObject *objectForKey(const QString &normalizedKey) const;
    QString resolveKeyForWrite(const QString &viewId, const QString &key) const;

    void prune();
    bool hasReference(QObject *object, const QString &exceptKey = QString()) const;
    void maybeDisposeOwned(QObject *object, const QString &exceptKey = QString());
    void observeDescriptorObject(QObject *object);
    void releaseDescriptorObjectIfUnreferenced(QObject *object);

    QHash<QString, QPointer<QObject>> m_entries;
    QHash<QString, QString> m_viewBindings;
    QHash<QString, QString> m_owners;
    QHash<QObject *, QList<QMetaObject::Connection>> m_descriptorConnections;
    QString m_lastError;
};
