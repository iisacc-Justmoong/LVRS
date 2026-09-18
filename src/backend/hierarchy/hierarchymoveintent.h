#pragma once

#include <QString>

enum class HierarchyDropMode
{
    Before,
    After,
    Child,
    Root
};

struct HierarchyMoveIntent
{
    QString dragNodeId;
    QString targetNodeId;
    HierarchyDropMode dropMode = HierarchyDropMode::Child;
};
