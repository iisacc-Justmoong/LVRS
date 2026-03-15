#pragma once

#include <QString>

struct HierarchySelectionState
{
    QString selectedNodeId;
    QString activeNodeId;
    QString editingNodeId;
    QString anchorNodeId;
};
