#include "backend/model/modelundostack.h"

#include <QtGlobal>

ModelUndoStack::ModelUndoStack(QObject *parent)
    : QObject(parent)
{
}

int ModelUndoStack::limit() const
{
    return m_limit;
}

void ModelUndoStack::setLimit(int value)
{
    const int next = qMax(1, value);
    if (m_limit == next)
        return;

    m_limit = next;
    trimToLimit();
    emit limitChanged();
}

int ModelUndoStack::undoDepth() const
{
    return m_undo.size();
}

int ModelUndoStack::redoDepth() const
{
    return m_redo.size();
}

bool ModelUndoStack::canUndo() const
{
    return !m_undo.isEmpty();
}

bool ModelUndoStack::canRedo() const
{
    return !m_redo.isEmpty();
}

void ModelUndoStack::clear()
{
    if (m_undo.isEmpty() && m_redo.isEmpty())
        return;

    m_undo.clear();
    m_redo.clear();
    emit stackChanged();
}

void ModelUndoStack::pushSnapshot(const QVariant &snapshot)
{
    if (!snapshot.isValid())
        return;
    if (!m_undo.isEmpty() && m_undo.last() == snapshot)
        return;

    m_undo.append(snapshot);
    trimToLimit();
    m_redo.clear();
    emit stackChanged();
}

QVariant ModelUndoStack::takeUndoSnapshot(const QVariant &currentSnapshot)
{
    if (m_undo.isEmpty())
        return {};

    const QVariant snapshot = m_undo.takeLast();
    if (currentSnapshot.isValid())
        m_redo.append(currentSnapshot);
    emit stackChanged();
    return snapshot;
}

QVariant ModelUndoStack::takeRedoSnapshot(const QVariant &currentSnapshot)
{
    if (m_redo.isEmpty())
        return {};

    const QVariant snapshot = m_redo.takeLast();
    if (currentSnapshot.isValid()) {
        m_undo.append(currentSnapshot);
        trimToLimit();
    }
    emit stackChanged();
    return snapshot;
}

void ModelUndoStack::trimToLimit()
{
    while (m_undo.size() > m_limit)
        m_undo.removeFirst();
    while (m_redo.size() > m_limit)
        m_redo.removeFirst();
}
