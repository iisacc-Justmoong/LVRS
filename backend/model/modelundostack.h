#pragma once

#include <QObject>
#include <QVariant>
#include <QVector>
#include <QtQml/qqml.h>

class ModelUndoStack : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ModelUndoStack)

    Q_PROPERTY(int limit READ limit WRITE setLimit NOTIFY limitChanged)
    Q_PROPERTY(int undoDepth READ undoDepth NOTIFY stackChanged)
    Q_PROPERTY(int redoDepth READ redoDepth NOTIFY stackChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY stackChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY stackChanged)

public:
    explicit ModelUndoStack(QObject *parent = nullptr);

    int limit() const;
    void setLimit(int value);

    int undoDepth() const;
    int redoDepth() const;
    bool canUndo() const;
    bool canRedo() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void pushSnapshot(const QVariant &snapshot);
    Q_INVOKABLE QVariant takeUndoSnapshot(const QVariant &currentSnapshot);
    Q_INVOKABLE QVariant takeRedoSnapshot(const QVariant &currentSnapshot);

signals:
    void limitChanged();
    void stackChanged();

private:
    void trimToLimit();

    QVector<QVariant> m_undo;
    QVector<QVariant> m_redo;
    int m_limit = 128;
};
