#pragma once

#include <QObject>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QtQml/qqml.h>

class ViewStateTracker : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ViewStateTracker)
    QML_SINGLETON

    Q_PROPERTY(QVariantList stack READ stack NOTIFY stackChanged)
    Q_PROPERTY(QStringList loadedViews READ loadedViews NOTIFY stackChanged)
    Q_PROPERTY(QStringList activeViews READ activeViews NOTIFY stackChanged)
    Q_PROPERTY(QStringList inactiveViews READ inactiveViews NOTIFY stackChanged)
    Q_PROPERTY(QStringList disabledViews READ disabledViews NOTIFY stackChanged)
    Q_PROPERTY(QString currentActiveView READ currentActiveView NOTIFY stackChanged)
    Q_PROPERTY(int loadedCount READ loadedCount NOTIFY stackChanged)

public:
    enum ViewState {
        Active = 0,
        Inactive = 1,
        Disabled = 2
    };
    Q_ENUM(ViewState)

    explicit ViewStateTracker(QObject *parent = nullptr);

    QVariantList stack() const;
    QStringList loadedViews() const;
    QStringList activeViews() const;
    QStringList inactiveViews() const;
    QStringList disabledViews() const;
    QString currentActiveView() const;
    int loadedCount() const;

    Q_INVOKABLE void syncStack(const QVariantList &entries);
    Q_INVOKABLE void setViewDisabled(const QString &viewId, bool disabled);
    Q_INVOKABLE void setViewEnabled(const QString &viewId, bool enabled);
    Q_INVOKABLE bool isLoaded(const QString &viewId) const;
    Q_INVOKABLE QString stateOf(const QString &viewId) const;
    Q_INVOKABLE QVariantMap view(const QString &viewId) const;
    Q_INVOKABLE QVariantMap snapshot() const;
    Q_INVOKABLE void clear();

signals:
    void stackChanged();

private:
    struct ViewRecord {
        QString viewId;
        QString path;
        int index = -1;
        bool baseEnabled = true;
        ViewState state = Inactive;
    };

    struct StackInput {
        QString viewId;
        QString path;
        bool enabled = true;
    };

    static QString normalizeId(const QString &rawId);
    static QString stateToString(ViewState state);
    static bool sameStack(const QVector<ViewRecord> &left, const QVector<ViewRecord> &right);

    QVector<StackInput> parseEntries(const QVariantList &entries) const;
    void recalculateStates(QVector<ViewRecord> *records) const;
    bool updateRecords(const QVector<ViewRecord> &nextRecords);
    QVariantMap toMap(const ViewRecord &record) const;
    int findTopMostIndex(const QString &viewId) const;

    QVector<ViewRecord> m_records;
    QSet<QString> m_disabledOverrides;
};
