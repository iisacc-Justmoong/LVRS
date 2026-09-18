#pragma once

#include "hierarchymoveintent.h"
#include "hierarchynode.h"

#include <QHash>
#include <QSet>
#include <QString>
#include <QVector>

class HierarchyController
{
public:
    static bool applyMove(
        QVector<HierarchyNode> *nodes,
        const HierarchyMoveIntent &intent,
        QString *errorMessage);

private:
    static bool validateNoCycle(const QHash<QString, QString> &parents, const QString &nodeId, const QString &newParentId);
    static QVector<QString> collectSubtree(const QVector<HierarchyNode> &nodes, const QString &rootId);
};
