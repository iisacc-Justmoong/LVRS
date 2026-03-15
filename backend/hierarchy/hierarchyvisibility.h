#pragma once

#include "hierarchynode.h"

#include <QSet>
#include <QVector>

struct HierarchyVisibleRow
{
    HierarchyNode node;
    int depth = 0;
    bool hasChildren = false;
};

class HierarchyVisibility
{
public:
    static QVector<HierarchyVisibleRow> project(
        const QVector<HierarchyNode> &nodes,
        const QSet<QString> &expandedIds);
};
