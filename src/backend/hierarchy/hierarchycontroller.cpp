#include "hierarchycontroller.h"

namespace
{
    int indexOfNode(const QVector<HierarchyNode> &nodes, const QString &id)
    {
        for (int i = 0; i < nodes.size(); ++i)
        {
            if (nodes.at(i).id == id)
            {
                return i;
            }
        }
        return -1;
    }
}

bool HierarchyController::applyMove(
    QVector<HierarchyNode> *nodes,
    const HierarchyMoveIntent &intent,
    QString *errorMessage)
{
    if (!nodes)
    {
        if (errorMessage) *errorMessage = QStringLiteral("nodes is null.");
        return false;
    }

    const int dragIndex = indexOfNode(*nodes, intent.dragNodeId);
    if (dragIndex < 0)
    {
        if (errorMessage) *errorMessage = QStringLiteral("drag node not found.");
        return false;
    }

    QString newParentId;
    int insertIndex = nodes->size();

    if (intent.dropMode == HierarchyDropMode::Root)
    {
        newParentId = QString();
        insertIndex = nodes->size();
    }
    else
    {
        const int targetIndex = indexOfNode(*nodes, intent.targetNodeId);
        if (targetIndex < 0)
        {
            if (errorMessage) *errorMessage = QStringLiteral("target node not found.");
            return false;
        }

        if (intent.dropMode == HierarchyDropMode::Child)
        {
            newParentId = intent.targetNodeId;
            insertIndex = targetIndex + 1;
        }
        else
        {
            newParentId = nodes->at(targetIndex).parentId;
            insertIndex = targetIndex + (intent.dropMode == HierarchyDropMode::After ? 1 : 0);
        }
    }

    QHash<QString, QString> parents;
    for (const HierarchyNode &node : *nodes)
    {
        parents.insert(node.id, node.parentId);
    }
    if (!validateNoCycle(parents, intent.dragNodeId, newParentId))
    {
        if (errorMessage) *errorMessage = QStringLiteral("move would create cycle.");
        return false;
    }

    HierarchyNode moved = nodes->at(dragIndex);
    moved.parentId = newParentId;

    nodes->removeAt(dragIndex);
    if (insertIndex > dragIndex)
    {
        --insertIndex;
    }
    if (insertIndex < 0) insertIndex = 0;
    if (insertIndex > nodes->size()) insertIndex = nodes->size();
    nodes->insert(insertIndex, moved);
    return true;
}

bool HierarchyController::validateNoCycle(const QHash<QString, QString> &parents, const QString &nodeId, const QString &newParentId)
{
    QString cursor = newParentId;
    while (!cursor.isEmpty())
    {
        if (cursor == nodeId)
        {
            return false;
        }
        cursor = parents.value(cursor);
    }
    return true;
}

QVector<QString> HierarchyController::collectSubtree(const QVector<HierarchyNode> &nodes, const QString &rootId)
{
    Q_UNUSED(nodes)
    Q_UNUSED(rootId)
    return {};
}
