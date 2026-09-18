#include "hierarchyops.h"

namespace
{
    bool createsCycle(const QHash<QString, QString> &parentById, const QString &nodeId, QString cursor)
    {
        while (!cursor.isEmpty())
        {
            if (cursor == nodeId)
            {
                return true;
            }
            cursor = parentById.value(cursor);
        }
        return false;
    }
}

bool HierarchyOps::reparent(
    QVector<HierarchyNode> *nodes,
    const QString &nodeId,
    const QString &newParentId,
    QString *errorMessage)
{
    if (!nodes)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("nodes is null.");
        }
        return false;
    }

    int nodeIndex = -1;
    QHash<QString, QString> parentById;
    for (int i = 0; i < nodes->size(); ++i)
    {
        const HierarchyNode &node = nodes->at(i);
        parentById.insert(node.id, node.parentId);
        if (node.id == nodeId)
        {
            nodeIndex = i;
        }
    }

    if (nodeIndex < 0)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("nodeId not found: %1").arg(nodeId);
        }
        return false;
    }

    if (!newParentId.isEmpty() && !parentById.contains(newParentId))
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("newParentId not found: %1").arg(newParentId);
        }
        return false;
    }

    if (createsCycle(parentById, nodeId, newParentId))
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("reparent would create a cycle.");
        }
        return false;
    }

    (*nodes)[nodeIndex].parentId = newParentId;
    return true;
}

QHash<QString, int> HierarchyOps::buildDepthMap(const QVector<HierarchyNode> &nodes)
{
    QHash<QString, QString> parentById;
    for (const HierarchyNode &node : nodes)
    {
        parentById.insert(node.id, node.parentId);
    }

    QHash<QString, int> depthMap;
    for (const HierarchyNode &node : nodes)
    {
        int depth = 0;
        QString cursor = node.parentId;
        while (!cursor.isEmpty())
        {
            ++depth;
            cursor = parentById.value(cursor);
        }
        depthMap.insert(node.id, depth);
    }
    return depthMap;
}
