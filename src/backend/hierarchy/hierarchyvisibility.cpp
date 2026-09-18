#include "hierarchyvisibility.h"

#include <QHash>

QVector<HierarchyVisibleRow> HierarchyVisibility::project(
    const QVector<HierarchyNode> &nodes,
    const QSet<QString> &expandedIds)
{
    QHash<QString, int> depth;
    QHash<QString, bool> hasChildren;
    QHash<QString, QString> parentById;

    for (const HierarchyNode &node : nodes)
    {
        parentById.insert(node.id, node.parentId);
    }
    for (const HierarchyNode &node : nodes)
    {
        if (!node.parentId.isEmpty())
        {
            hasChildren[node.parentId] = true;
        }
    }

    for (const HierarchyNode &node : nodes)
    {
        int d = 0;
        QString cursor = node.parentId;
        while (!cursor.isEmpty())
        {
            ++d;
            cursor = parentById.value(cursor);
        }
        depth.insert(node.id, d);
    }

    QVector<HierarchyVisibleRow> rows;
    for (const HierarchyNode &node : nodes)
    {
        bool visible = true;
        QString cursor = node.parentId;
        while (!cursor.isEmpty())
        {
            if (!expandedIds.contains(cursor))
            {
                visible = false;
                break;
            }
            cursor = parentById.value(cursor);
        }
        if (!visible)
        {
            continue;
        }

        HierarchyVisibleRow row;
        row.node = node;
        row.depth = depth.value(node.id, 0);
        row.hasChildren = hasChildren.value(node.id, false);
        rows.push_back(row);
    }

    return rows;
}
