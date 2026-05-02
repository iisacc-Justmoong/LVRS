#pragma once

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

class NavigationStackModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(NavigationStackModel)

    Q_PROPERTY(QVariant path READ path WRITE setPath NOTIFY pathChanged)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentChanged)
    Q_PROPERTY(QVariant currentParams READ currentParams NOTIFY currentChanged)
    Q_PROPERTY(QVariantList viewTrackingEntries READ viewTrackingEntries NOTIFY viewTrackingEntriesChanged)
    Q_PROPERTY(QStringList trackedViewIds READ trackedViewIds NOTIFY trackedViewIdsChanged)
    Q_PROPERTY(int depth READ depth NOTIFY pathChanged)

public:
    explicit NavigationStackModel(QObject *parent = nullptr);

    QVariant path() const;
    void setPath(const QVariant &value);

    QString currentPath() const;
    QVariant currentParams() const;
    QVariantList viewTrackingEntries() const;
    QStringList trackedViewIds() const;
    int depth() const;

    Q_INVOKABLE QString normalizePath(const QVariant &pathValue) const;
    Q_INVOKABLE QVariantMap createPathEntry(const QVariant &pathValue, const QVariant &params = QVariant()) const;
    Q_INVOKABLE QVariantMap createComponentPathEntry(const QVariant &component, const QVariant &params = QVariant()) const;
    Q_INVOKABLE QVariantList stackAfterPathOperation(const QVariant &pathValue,
                                                     const QVariant &params,
                                                     const QString &mode) const;
    Q_INVOKABLE QVariantList stackAfterComponentOperation(const QVariant &component,
                                                          const QVariant &params,
                                                          const QString &mode) const;
    Q_INVOKABLE QVariantList stackAfterPop() const;
    Q_INVOKABLE QVariantList stackAfterPopToRoot() const;
    Q_INVOKABLE void applyPathOperation(const QVariant &pathValue, const QVariant &params, const QString &mode);
    Q_INVOKABLE void applyComponentOperation(const QVariant &component, const QVariant &params, const QString &mode);
    Q_INVOKABLE void pop();
    Q_INVOKABLE void popToRoot();
    Q_INVOKABLE QVariantMap currentEntryDescriptor() const;
    Q_INVOKABLE QVariantMap createViewTrackingEntry(const QVariant &entry, int index) const;
    Q_INVOKABLE QVariantList buildViewTrackingEntries(const QVariant &pathValue = QVariant()) const;
    Q_INVOKABLE QStringList updateTrackedViewIds(const QVariantList &entries);

signals:
    void pathChanged();
    void currentChanged();
    void viewTrackingEntriesChanged();
    void trackedViewIdsChanged();

private:
    static QVariantList listFromVariant(const QVariant &value);
    static QVariantMap mapFromVariant(const QVariant &value);
    static QVariant paramsOrEmpty(const QVariant &params);
    void replacePath(const QVariantList &nextPath);
    void refreshDerivedState();

    QVariantList m_path;
    QString m_currentPath;
    QVariant m_currentParams = QVariantMap();
    QVariantList m_viewTrackingEntries;
    QStringList m_trackedViewIds;
};
