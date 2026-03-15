#include "hierarchycodec.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace
{
    QString normalizeId(QString value)
    {
        return value.trimmed();
    }

    QString normalizeLabel(QString value)
    {
        const QString trimmed = value.trimmed();
        return trimmed.isEmpty() ? QStringLiteral("Untitled") : trimmed;
    }
}

bool HierarchyCodec::parse(const QString &rawText, QVector<HierarchyNode> *outNodes, QString *errorMessage) const
{
    if (!outNodes)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("outNodes is null.");
        }
        return false;
    }

    outNodes->clear();
    const QByteArray jsonBytes = rawText.toUtf8();
    if (jsonBytes.trimmed().isEmpty())
    {
        return true;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(jsonBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("Invalid hierarchy JSON: %1").arg(parseError.errorString());
        }
        return false;
    }

    const QJsonObject root = document.object();
    QVector<HierarchyNode> parsed;

    const QJsonValue nodesValue = root.value(QStringLiteral("nodes"));
    if (nodesValue.isArray())
    {
        for (const QJsonValue &entry : nodesValue.toArray())
        {
            if (!entry.isObject())
            {
                continue;
            }
            const QJsonObject object = entry.toObject();
            const QString id = normalizeId(object.value(QStringLiteral("id")).toString());
            if (id.isEmpty())
            {
                continue;
            }
            HierarchyNode node;
            node.id = id;
            node.label = normalizeLabel(object.value(QStringLiteral("label")).toString());
            node.parentId = normalizeId(object.value(QStringLiteral("parentId")).toString());
            node.meta = object.value(QStringLiteral("meta")).toObject().toVariantMap();
            parsed.push_back(node);
        }
    }

    const QJsonValue treeValue = root.value(QStringLiteral("tree"));
    if (parsed.isEmpty() && treeValue.isArray())
    {
        bool ok = true;
        QString flattenError;
        for (const QJsonValue &entry : treeValue.toArray())
        {
            flattenTreeNode(entry.toObject().toVariantMap(), QString(), &parsed, &flattenError, &ok);
            if (!ok)
            {
                if (errorMessage)
                {
                    *errorMessage = flattenError;
                }
                return false;
            }
        }
    }

    QSet<QString> ids;
    for (const HierarchyNode &node : parsed)
    {
        ids.insert(node.id);
    }
    for (const HierarchyNode &node : parsed)
    {
        if (!node.parentId.isEmpty() && !ids.contains(node.parentId))
        {
            if (errorMessage)
            {
                *errorMessage = QStringLiteral("Orphan parentId detected: %1 -> %2").arg(node.id, node.parentId);
            }
            return false;
        }
    }

    *outNodes = parsed;
    return true;
}

QString HierarchyCodec::serialize(const QVector<HierarchyNode> &nodes) const
{
    QJsonArray array;
    for (const HierarchyNode &node : nodes)
    {
        QJsonObject object;
        object.insert(QStringLiteral("id"), node.id);
        object.insert(QStringLiteral("label"), node.label);
        if (!node.parentId.isEmpty())
        {
            object.insert(QStringLiteral("parentId"), node.parentId);
        }
        if (!node.meta.isEmpty())
        {
            object.insert(QStringLiteral("meta"), QJsonObject::fromVariantMap(node.meta));
        }
        array.push_back(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("schema"), QStringLiteral("lvrs.hierarchy.v1"));
    root.insert(QStringLiteral("nodes"), array);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void HierarchyCodec::flattenTreeNode(
    const QVariantMap &treeNode,
    const QString &parentId,
    QVector<HierarchyNode> *outNodes,
    QString *errorMessage,
    bool *ok)
{
    const QString id = normalizeId(treeNode.value(QStringLiteral("id")).toString());
    if (id.isEmpty())
    {
        *ok = false;
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("Tree node id is empty.");
        }
        return;
    }

    HierarchyNode node;
    node.id = id;
    node.label = normalizeLabel(treeNode.value(QStringLiteral("label")).toString());
    node.parentId = parentId;
    outNodes->push_back(node);

    const QVariantList children = treeNode.value(QStringLiteral("children")).toList();
    for (const QVariant &child : children)
    {
        flattenTreeNode(child.toMap(), id, outNodes, errorMessage, ok);
        if (!*ok)
        {
            return;
        }
    }
}
