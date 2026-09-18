#pragma once

#include <QString>
#include <QVariantMap>

struct HierarchyNode
{
    QString id;
    QString label;
    QString parentId;
    QVariantMap meta;
};
