#include "hierarchycommandstack.h"

bool HierarchyCommandStack::applyMove(
    QVector<HierarchyNode> *nodes,
    const HierarchyMoveIntent &intent,
    QString *errorMessage)
{
    if (!nodes)
    {
        if (errorMessage) *errorMessage = QStringLiteral("nodes is null.");
        return false;
    }

    QVector<HierarchyNode> before = *nodes;
    if (!HierarchyController::applyMove(nodes, intent, errorMessage))
    {
        return false;
    }

    m_undo.push_back(before);
    m_redo.clear();
    return true;
}

bool HierarchyCommandStack::undo(QVector<HierarchyNode> *nodes, QString *errorMessage)
{
    if (!nodes)
    {
        if (errorMessage) *errorMessage = QStringLiteral("nodes is null.");
        return false;
    }
    if (m_undo.isEmpty())
    {
        if (errorMessage) *errorMessage = QStringLiteral("undo stack empty.");
        return false;
    }

    m_redo.push_back(*nodes);
    *nodes = m_undo.takeLast();
    return true;
}

bool HierarchyCommandStack::redo(QVector<HierarchyNode> *nodes, QString *errorMessage)
{
    if (!nodes)
    {
        if (errorMessage) *errorMessage = QStringLiteral("nodes is null.");
        return false;
    }
    if (m_redo.isEmpty())
    {
        if (errorMessage) *errorMessage = QStringLiteral("redo stack empty.");
        return false;
    }

    m_undo.push_back(*nodes);
    *nodes = m_redo.takeLast();
    return true;
}
