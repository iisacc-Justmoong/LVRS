#pragma once

#include <QObject>
#include <QStringList>
#include <QtQml/qqml.h>

class PageMonitor : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PageMonitor)
    QML_SINGLETON

    Q_PROPERTY(QStringList history READ history NOTIFY historyChanged)
    Q_PROPERTY(int count READ count NOTIFY historyChanged)
    Q_PROPERTY(QString current READ current NOTIFY historyChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)

public:
    explicit PageMonitor(QObject *parent = nullptr);

    Q_INVOKABLE void record(const QString &path);
    Q_INVOKABLE QString undo();
    Q_INVOKABLE void clear();

    QStringList history() const;
    int count() const;
    QString current() const;
    bool canUndo() const;

signals:
    void historyChanged();

private:
    void emitIfChanged(bool changed);

    QStringList m_history;
};
