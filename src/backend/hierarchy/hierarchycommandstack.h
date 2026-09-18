#pragma once

#include "hierarchycontroller.h"

#include <QVector>

class HierarchyCommandStack
{
public:
    bool applyMove(
        QVector<HierarchyNode> *nodes,
        const HierarchyMoveIntent &intent,
        QString *errorMessage);

    bool undo(QVector<HierarchyNode> *nodes, QString *errorMessage);
    bool redo(QVector<HierarchyNode> *nodes, QString *errorMessage);

private:
    QVector<QVector<HierarchyNode>> m_undo;
    QVector<QVector<HierarchyNode>> m_redo;
};
