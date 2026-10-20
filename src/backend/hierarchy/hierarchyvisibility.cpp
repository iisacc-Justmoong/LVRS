#include "hierarchyvisibility.h"

#include <QHash>

QVector<HierarchyVisibleRow> HierarchyVisibility::project(
    const QVector<HierarchyNode> &nodes,
    const QSet<QString> &expandedIds)
{
    QHash<QString, int> depth;
    QHash<QString, bool> hasChildren;
    QHash<QString, QString> parentById;
    depth.reserve(nodes.size());
    hasChildren.reserve(nodes.size());
    parentById.reserve(nodes.size());

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

    struct Ancestry { int length = 0; bool expanded = true; };
    QHash<QString, Ancestry> ancestry;
    ancestry.reserve(nodes.size());
    QVector<QString> path;
    QSet<QString> visiting;
    // Resolve each ancestor once, without recursion or dependence on input order.
    auto resolve = [&](const QString &start) {
        path.clear();
        visiting.clear();
        QString cursor = start;
        while (!cursor.isEmpty() && !ancestry.contains(cursor))
        {
            if (visiting.contains(cursor))
            {
                // Malformed cyclic branches cannot have a finite visible depth.
                for (const auto &id : path) ancestry.insert(id, {0, false});
                return Ancestry{0, false};
            }
            visiting.insert(cursor);
            path.push_back(cursor);
            cursor = parentById.value(cursor);
        }
        auto info = cursor.isEmpty() ? Ancestry{} : ancestry.value(cursor);
        for (auto it = path.crbegin(); it != path.crend(); ++it)
        {
            ++info.length;
            info.expanded = expandedIds.contains(*it) && info.expanded;
            ancestry.insert(*it, info);
        }
        return info;
    };
    for (const HierarchyNode &node : nodes)
        depth.insert(node.id, resolve(node.parentId).length);

    QVector<HierarchyVisibleRow> rows;
    rows.reserve(nodes.size());
    for (const HierarchyNode &node : nodes)
    {
        if (!resolve(node.parentId).expanded)
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
