#pragma once

#include "hierarchynode.h"

#include <QHash>
#include <QVector>

class HierarchyOps
{
public:
    static bool reparent(
        QVector<HierarchyNode> *nodes,
        const QString &nodeId,
        const QString &newParentId,
        QString *errorMessage);

    static QHash<QString, int> buildDepthMap(const QVector<HierarchyNode> &nodes);
};
