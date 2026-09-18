#pragma once

#include "hierarchynode.h"

#include <QString>
#include <QVector>

class HierarchyCodec
{
public:
    bool parse(const QString &rawText, QVector<HierarchyNode> *outNodes, QString *errorMessage) const;
    QString serialize(const QVector<HierarchyNode> &nodes) const;

private:
    static void flattenTreeNode(
        const QVariantMap &treeNode,
        const QString &parentId,
        QVector<HierarchyNode> *outNodes,
        QString *errorMessage,
        bool *ok);
};
