#include "backend/model/hierarchymodel.h"

#include <QJSValue>
#include <QHash>
#include <QObject>
#include <QtMath>

#include <cmath>

namespace {
QString normalizedRole(const QString &value)
{
    return value.trimmed();
}

QString textFromVariant(const QVariant &value)
{
    return (value.isValid() && !value.isNull()) ? value.toString() : QString();
}
} // namespace

HierarchyModel::HierarchyModel(QObject *parent)
    : QObject(parent)
    , m_source(this)
{
    connect(&m_source, &ModelSource::revisionChanged, this, &HierarchyModel::rebuildDescriptors);
}

QVariant HierarchyModel::source() const
{
    return m_source.source();
}

void HierarchyModel::setSource(const QVariant &source)
{
    if (m_source.source() == source)
        return;

    m_source.setSource(source);
    emit sourceChanged();
}

int HierarchyModel::column() const
{
    return m_source.column();
}

void HierarchyModel::setColumn(int column)
{
    if (m_source.column() == qMax(0, column))
        return;

    m_source.setColumn(column);
    emit columnChanged();
}

QString HierarchyModel::itemIdRole() const
{
    return m_itemIdRole;
}

void HierarchyModel::setItemIdRole(const QString &value)
{
    setRole(&m_itemIdRole, value);
}

QString HierarchyModel::itemKeyRole() const
{
    return m_itemKeyRole;
}

void HierarchyModel::setItemKeyRole(const QString &value)
{
    setRole(&m_itemKeyRole, value);
}

QString HierarchyModel::labelRole() const
{
    return m_labelRole;
}

void HierarchyModel::setLabelRole(const QString &value)
{
    setRole(&m_labelRole, value);
}

QString HierarchyModel::iconNameRole() const
{
    return m_iconNameRole;
}

void HierarchyModel::setIconNameRole(const QString &value)
{
    setRole(&m_iconNameRole, value);
}

QString HierarchyModel::iconSourceRole() const
{
    return m_iconSourceRole;
}

void HierarchyModel::setIconSourceRole(const QString &value)
{
    setRole(&m_iconSourceRole, value);
}

QString HierarchyModel::iconGlyphRole() const
{
    return m_iconGlyphRole;
}

void HierarchyModel::setIconGlyphRole(const QString &value)
{
    setRole(&m_iconGlyphRole, value);
}

QString HierarchyModel::countRole() const
{
    return m_countRole;
}

void HierarchyModel::setCountRole(const QString &value)
{
    setRole(&m_countRole, value);
}

QString HierarchyModel::enabledRole() const
{
    return m_enabledRole;
}

void HierarchyModel::setEnabledRole(const QString &value)
{
    setRole(&m_enabledRole, value);
}

QString HierarchyModel::expandedRole() const
{
    return m_expandedRole;
}

void HierarchyModel::setExpandedRole(const QString &value)
{
    setRole(&m_expandedRole, value);
}

QString HierarchyModel::selectedRole() const
{
    return m_selectedRole;
}

void HierarchyModel::setSelectedRole(const QString &value)
{
    setRole(&m_selectedRole, value);
}

QString HierarchyModel::activatableRole() const
{
    return m_activatableRole;
}

void HierarchyModel::setActivatableRole(const QString &value)
{
    setRole(&m_activatableRole, value);
}

QString HierarchyModel::draggableRole() const
{
    return m_draggableRole;
}

void HierarchyModel::setDraggableRole(const QString &value)
{
    setRole(&m_draggableRole, value);
}

QString HierarchyModel::showChevronRole() const
{
    return m_showChevronRole;
}

void HierarchyModel::setShowChevronRole(const QString &value)
{
    setRole(&m_showChevronRole, value);
}

QString HierarchyModel::depthRole() const
{
    return m_depthRole;
}

void HierarchyModel::setDepthRole(const QString &value)
{
    setRole(&m_depthRole, value);
}

QVariantList HierarchyModel::descriptors() const
{
    return m_descriptors;
}

int HierarchyModel::count() const
{
    return m_descriptors.size();
}

int HierarchyModel::revision() const
{
    return m_revision;
}

bool HierarchyModel::hasSource() const
{
    return m_source.count() > 0;
}

QVariantMap HierarchyModel::descriptorAt(int index) const
{
    if (index < 0 || index >= m_descriptors.size())
        return {};
    return m_descriptors.at(index).toMap();
}

QVariant HierarchyModel::roleValue(const QVariant &entry,
                                   const QString &roleName,
                                   const QVariant &fallbackValue) const
{
    return m_source.roleValue(entry, roleName, fallbackValue);
}

bool HierarchyModel::depthArraySupportsEditing(const QVariant &nodes) const
{
    QVariantList list;
    if (nodes.userType() == qMetaTypeId<QJSValue>())
        list = nodes.value<QJSValue>().toVariant().toList();
    else if (nodes.canConvert<QVariantList>())
        list = nodes.toList();
    else
        return false;

    for (const QVariant &node : list) {
        if (!variantLooksObjectLike(node) || node.canConvert<QVariantList>())
            return false;
        const QVariantMap map = node.toMap();
        if (map.value(QStringLiteral("children")).canConvert<QVariantList>()
                || map.value(QStringLiteral("items")).canConvert<QVariantList>()
                || map.value(QStringLiteral("nodes")).canConvert<QVariantList>()) {
            return false;
        }
    }
    return true;
}

QVariantMap HierarchyModel::projectInteractionState(const QVariantList &items) const
{
    QVariantList visibilityFlags;
    QVariantList visibleIndices;
    QVariantList visibleEnabledIndices;
    QVariantList metadata;
    QVariantMap idIndexMap;
    QVariantMap keyIndexMap;
    visibilityFlags.reserve(items.size());
    metadata.reserve(items.size());

    QVector<bool> visibleByDepth;
    QVector<bool> expandedByDepth;
    QVector<int> ancestorStack;
    QVector<int> nextSiblingByDepth;
    QVector<int> nextVisibleSiblingByDepth;
    QHash<int, int> siblingCountByParent;
    QHash<int, int> visibleSiblingCountByParent;
    int visibleIndexCounter = 0;

    QVariantList pathLabels;
    QVariantList pathKeys;
    QVariantList pathLabelArrays;
    pathLabels.reserve(items.size());
    pathKeys.reserve(items.size());
    pathLabelArrays.reserve(items.size());

    for (int index = 0; index < items.size(); ++index) {
        const QVariantMap item = items.at(index).toMap();
        const QString idLookup = QStringLiteral("int:%1").arg(item.value(QStringLiteral("itemId"), index).toInt());
        if (!idIndexMap.contains(idLookup))
            idIndexMap.insert(idLookup, index);
        const QString key = descriptorKey(item, index);
        if (!key.isEmpty() && !keyIndexMap.contains(key))
            keyIndexMap.insert(key, index);

        const int indent = descriptorIndent(item);
        if (visibleByDepth.size() > indent) {
            visibleByDepth.resize(indent);
            expandedByDepth.resize(indent);
        }

        bool rowVisible = true;
        if (indent > 0 && expandedByDepth.size() >= indent)
            rowVisible = visibleByDepth.at(indent - 1) && expandedByDepth.at(indent - 1);

        const bool hasChildren = index < items.size() - 1
            && descriptorIndent(items.at(index + 1).toMap()) > indent;
        const bool expandable = item.value(QStringLiteral("showChevron"), true).toBool() && hasChildren;
        visibilityFlags.append(rowVisible);
        visibleByDepth.append(rowVisible);
        expandedByDepth.append(!expandable || item.value(QStringLiteral("expanded")).toBool());

        if (rowVisible) {
            visibleIndices.append(index);
            if (descriptorCanBecomeActive(item))
                visibleEnabledIndices.append(index);
        }

        if (ancestorStack.size() > indent)
            ancestorStack.resize(indent);
        if (nextSiblingByDepth.size() > indent + 1)
            nextSiblingByDepth.resize(indent + 1);
        if (nextVisibleSiblingByDepth.size() > indent + 1)
            nextVisibleSiblingByDepth.resize(indent + 1);

        const int parentIndex = indent > 0 && ancestorStack.size() >= indent ? ancestorStack.at(indent - 1) : -1;
        const QString label = descriptorLabel(item, index);
        const QString parentPathLabel = parentIndex >= 0 ? pathLabels.at(parentIndex).toString() : QString();
        const QString pathLabel = parentPathLabel.isEmpty() ? label : parentPathLabel + QStringLiteral(" / ") + label;
        pathLabels.append(pathLabel);

        const QVariantList ancestorKeys = parentIndex >= 0 ? pathKeys.at(parentIndex).toList() : QVariantList();
        const QVariantList ancestorLabels = parentIndex >= 0 ? pathLabelArrays.at(parentIndex).toList() : QVariantList();
        QVariantList nextPathKeys = ancestorKeys;
        nextPathKeys.append(key);
        QVariantList nextPathLabels = ancestorLabels;
        nextPathLabels.append(label);
        pathKeys.append(QVariant::fromValue(nextPathKeys));
        pathLabelArrays.append(QVariant::fromValue(nextPathLabels));

        const int siblingIndex = nextSiblingByDepth.size() > indent ? nextSiblingByDepth.at(indent) : 0;
        if (nextSiblingByDepth.size() <= indent)
            nextSiblingByDepth.resize(indent + 1);
        nextSiblingByDepth[indent] = siblingIndex + 1;
        siblingCountByParent[parentIndex] += 1;

        int visibleSiblingIndex = -1;
        if (rowVisible) {
            visibleSiblingIndex = nextVisibleSiblingByDepth.size() > indent ? nextVisibleSiblingByDepth.at(indent) : 0;
            if (nextVisibleSiblingByDepth.size() <= indent)
                nextVisibleSiblingByDepth.resize(indent + 1);
            nextVisibleSiblingByDepth[indent] = visibleSiblingIndex + 1;
            visibleSiblingCountByParent[parentIndex] += 1;
        }

        ancestorStack.append(index);
        if (ancestorStack.size() > indent + 1)
            ancestorStack.resize(indent + 1);

        metadata.append(QVariantMap {
            {QStringLiteral("hasChildren"), hasChildren},
            {QStringLiteral("visible"), rowVisible},
            {QStringLiteral("flatIndex"), index},
            {QStringLiteral("visibleIndex"), rowVisible ? visibleIndexCounter++ : -1},
            {QStringLiteral("siblingIndex"), siblingIndex},
            {QStringLiteral("visibleSiblingIndex"), visibleSiblingIndex},
            {QStringLiteral("parentIndex"), parentIndex},
            {QStringLiteral("parentItemKey"), parentIndex >= 0 ? descriptorKey(items.at(parentIndex).toMap(), parentIndex) : QString()},
            {QStringLiteral("parentLabel"), parentIndex >= 0 ? descriptorLabel(items.at(parentIndex).toMap(), parentIndex) : QString()},
            {QStringLiteral("parentPathLabel"), parentPathLabel},
            {QStringLiteral("pathLabel"), pathLabel},
            {QStringLiteral("ancestorItemKeys"), ancestorKeys},
            {QStringLiteral("ancestorLabels"), ancestorLabels},
            {QStringLiteral("pathItemKeys"), nextPathKeys},
            {QStringLiteral("pathItemLabels"), nextPathLabels},
            {QStringLiteral("siblingCount"), 0},
            {QStringLiteral("visibleSiblingCount"), 0},
            {QStringLiteral("childCount"), 0},
            {QStringLiteral("visibleChildCount"), 0},
            {QStringLiteral("descendantCount"), 0},
            {QStringLiteral("visibleDescendantCount"), 0},
            {QStringLiteral("childItemKeys"), QVariantList()},
            {QStringLiteral("childItemLabels"), QVariantList()}
        });
    }

    for (int index = 0; index < items.size(); ++index) {
        QVariantMap row = metadata.at(index).toMap();
        const int parentIndex = row.value(QStringLiteral("parentIndex")).toInt();
        row.insert(QStringLiteral("siblingCount"), siblingCountByParent.value(parentIndex));
        row.insert(QStringLiteral("visibleSiblingCount"), visibleSiblingCountByParent.value(parentIndex));

        int childCount = 0;
        int visibleChildCount = 0;
        int descendantCount = 0;
        int visibleDescendantCount = 0;
        QVariantList childKeys;
        QVariantList childLabels;
        const int indent = descriptorIndent(items.at(index).toMap());
        for (int child = index + 1; child < items.size(); ++child) {
            const int childIndent = descriptorIndent(items.at(child).toMap());
            if (childIndent <= indent)
                break;
            descendantCount += 1;
            if (visibilityFlags.at(child).toBool())
                visibleDescendantCount += 1;
            if (childIndent == indent + 1) {
                childCount += 1;
                if (visibilityFlags.at(child).toBool())
                    visibleChildCount += 1;
                childKeys.append(descriptorKey(items.at(child).toMap(), child));
                childLabels.append(descriptorLabel(items.at(child).toMap(), child));
            }
        }
        row.insert(QStringLiteral("childCount"), childCount);
        row.insert(QStringLiteral("visibleChildCount"), visibleChildCount);
        row.insert(QStringLiteral("descendantCount"), descendantCount);
        row.insert(QStringLiteral("visibleDescendantCount"), visibleDescendantCount);
        row.insert(QStringLiteral("childItemKeys"), childKeys);
        row.insert(QStringLiteral("childItemLabels"), childLabels);
        metadata[index] = row;
    }

    return QVariantMap {
        {QStringLiteral("visibilityFlags"), visibilityFlags},
        {QStringLiteral("visibleItemIndices"), visibleIndices},
        {QStringLiteral("visibleEnabledItemIndices"), visibleEnabledIndices},
        {QStringLiteral("metadata"), metadata},
        {QStringLiteral("idIndexMap"), idIndexMap},
        {QStringLiteral("keyIndexMap"), keyIndexMap},
        {QStringLiteral("itemCount"), items.size()},
        {QStringLiteral("visibleItemCount"), visibleIndices.size()}
    };
}

int HierarchyModel::descendantRangeEnd(const QVariantList &items, int itemIndex) const
{
    if (itemIndex < 0 || itemIndex >= items.size())
        return itemIndex;
    const int parentIndent = descriptorIndent(items.at(itemIndex).toMap());
    int descendantEnd = itemIndex;
    for (int index = itemIndex + 1; index < items.size(); ++index) {
        if (descriptorIndent(items.at(index).toMap()) <= parentIndent)
            break;
        descendantEnd = index;
    }
    return descendantEnd;
}

QVariantMap HierarchyModel::resolveDragTarget(const QVariantList &items,
                                              int sourceStart,
                                              int sourceEnd,
                                              int rawInsertionIndex,
                                              double localX,
                                              double indentStep,
                                              double basePadding) const
{
    if (sourceStart < 0 || sourceEnd < sourceStart)
        return {};
    const int blockSize = sourceEnd - sourceStart + 1;
    int insertionIndex = rawInsertionIndex;
    if (rawInsertionIndex > sourceStart)
        insertionIndex = rawInsertionIndex >= sourceEnd + 1 ? rawInsertionIndex - blockSize : -1;
    if (insertionIndex < 0)
        return {};

    QVariantList remainingItems;
    for (int index = 0; index < items.size(); ++index) {
        if (index >= sourceStart && index <= sourceEnd)
            continue;
        remainingItems.append(items.at(index));
    }
    insertionIndex = qMax(0, qMin(insertionIndex, remainingItems.size()));

    const double safeStep = indentStep > 0 ? indentStep : 8.0;
    const int desiredDepth = qMax(0, static_cast<int>(qFloor((localX - basePadding + safeStep * 0.5) / safeStep)));
    const QVariantMap previousItem = insertionIndex > 0 ? remainingItems.at(insertionIndex - 1).toMap() : QVariantMap();
    const QVariantMap nextItem = insertionIndex < remainingItems.size() ? remainingItems.at(insertionIndex).toMap() : QVariantMap();
    const int minDepth = nextItem.isEmpty() ? 0 : descriptorIndent(nextItem);
    const int maxDepth = previousItem.isEmpty() ? 0 : descriptorIndent(previousItem) + 1;
    const int depth = qMax(minDepth, qMin(maxDepth, desiredDepth));
    QVariantMap drop = dropDescriptorFor(remainingItems, insertionIndex, depth);
    drop.insert(QStringLiteral("insertionIndex"), insertionIndex);
    drop.insert(QStringLiteral("depth"), depth);
    drop.insert(QStringLiteral("minDepth"), minDepth);
    drop.insert(QStringLiteral("maxDepth"), maxDepth);
    return drop;
}

QVariantMap HierarchyModel::moveDescriptors(const QVariantList &items,
                                            int sourceStart,
                                            int sourceEnd,
                                            int targetIndex,
                                            int targetDepth) const
{
    QVariantMap result {{QStringLiteral("accepted"), false}};
    if (sourceStart < 0 || sourceEnd < sourceStart || sourceEnd >= items.size())
        return result;

    QVariantList block;
    QVariantList remaining;
    for (int index = 0; index < items.size(); ++index) {
        if (index >= sourceStart && index <= sourceEnd)
            block.append(items.at(index));
        else
            remaining.append(items.at(index));
    }
    if (block.isEmpty())
        return result;

    const int clampedTarget = qMax(0, qMin(targetIndex, remaining.size()));
    const int sourceDepth = descriptorIndent(block.first().toMap());
    const int depthDelta = targetDepth - sourceDepth;
    if (clampedTarget == sourceStart && depthDelta == 0)
        return result;

    for (int index = 0; index < block.size(); ++index) {
        QVariantMap descriptor = block.at(index).toMap();
        descriptor.insert(QStringLiteral("indentLevel"),
                          qMax(0, descriptorIndent(descriptor) + depthDelta));
        block[index] = descriptor;
    }

    QVariantList reordered;
    for (int index = 0; index < clampedTarget; ++index)
        reordered.append(remaining.at(index));
    for (const QVariant &descriptor : block)
        reordered.append(descriptor);
    for (int index = clampedTarget; index < remaining.size(); ++index)
        reordered.append(remaining.at(index));

    result.insert(QStringLiteral("accepted"), true);
    result.insert(QStringLiteral("reorderedDescriptors"), reordered);
    result.insert(QStringLiteral("movedDescriptor"), block.first());
    result.insert(QStringLiteral("fromIndex"), sourceStart);
    result.insert(QStringLiteral("toIndex"), clampedTarget);
    result.insert(QStringLiteral("depth"), qMax(0, targetDepth));
    result.insert(QStringLiteral("drop"), dropDescriptorFor(remaining, clampedTarget, qMax(0, targetDepth)));
    return result;
}

void HierarchyModel::invalidate()
{
    m_source.invalidate();
}

bool HierarchyModel::variantLooksObjectLike(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return false;
    if (value.canConvert<QVariantMap>())
        return true;
    if (value.canConvert<QObject *>())
        return value.value<QObject *>() != nullptr;
    if (value.userType() == qMetaTypeId<QJSValue>())
        return value.value<QJSValue>().isObject();
    return false;
}

bool HierarchyModel::stringStartsLikeUrl(const QString &value)
{
    return value.startsWith(QStringLiteral("qrc:"))
        || value.startsWith(QStringLiteral(":/"))
        || value.contains(QStringLiteral("://"));
}

int HierarchyModel::normalizedDepth(const QVariant &rawDepth, int fallbackDepth)
{
    bool ok = false;
    const double numericDepth = rawDepth.toDouble(&ok);
    if (ok && std::isfinite(numericDepth))
        return qMax(0, static_cast<int>(qFloor(numericDepth)));
    return qMax(0, fallbackDepth);
}

int HierarchyModel::descriptorIndent(const QVariantMap &descriptor)
{
    return normalizedDepth(descriptor.value(QStringLiteral("indentLevel")), 0);
}

bool HierarchyModel::descriptorCanBecomeActive(const QVariantMap &descriptor)
{
    return descriptor.value(QStringLiteral("enabled"), true).toBool()
        && descriptor.value(QStringLiteral("activatable"), true).toBool();
}

QString HierarchyModel::descriptorKey(const QVariantMap &descriptor, int index)
{
    const QString explicitKey = descriptor.value(QStringLiteral("itemKey")).toString().trimmed();
    if (!explicitKey.isEmpty())
        return explicitKey;
    const int itemId = descriptor.value(QStringLiteral("itemId"), -1).toInt();
    return itemId >= 0 ? QString::number(itemId) : QString::number(index);
}

QString HierarchyModel::descriptorLabel(const QVariantMap &descriptor, int index)
{
    const QString label = descriptor.value(QStringLiteral("label")).toString();
    return !label.isEmpty() ? label : descriptorKey(descriptor, index);
}

QVariantMap HierarchyModel::dropDescriptorFor(const QVariantList &remainingItems, int insertionIndex, int depth)
{
    const QVariantMap previousItem = insertionIndex > 0 ? remainingItems.at(insertionIndex - 1).toMap() : QVariantMap();
    const QVariantMap nextItem = insertionIndex < remainingItems.size() ? remainingItems.at(insertionIndex).toMap() : QVariantMap();
    QVariantMap parentItem;
    if (depth > 0) {
        for (int index = insertionIndex - 1; index >= 0; --index) {
            const QVariantMap candidate = remainingItems.at(index).toMap();
            if (descriptorIndent(candidate) == depth - 1) {
                parentItem = candidate;
                break;
            }
        }
    }

    QString modeName = QStringLiteral("root");
    QVariantMap anchorItem;
    if (!parentItem.isEmpty() && !previousItem.isEmpty() && parentItem == previousItem
            && depth == descriptorIndent(previousItem) + 1) {
        modeName = QStringLiteral("child");
        anchorItem = parentItem;
    } else if (!nextItem.isEmpty() && descriptorIndent(nextItem) == depth) {
        modeName = QStringLiteral("before");
        anchorItem = nextItem;
    } else if (!previousItem.isEmpty() && descriptorIndent(previousItem) == depth) {
        modeName = QStringLiteral("after");
        anchorItem = previousItem;
    } else if (!parentItem.isEmpty()) {
        modeName = QStringLiteral("child");
        anchorItem = parentItem;
    } else if (!nextItem.isEmpty() && descriptorIndent(nextItem) == 0) {
        modeName = QStringLiteral("before");
        anchorItem = nextItem;
    } else if (!previousItem.isEmpty() && descriptorIndent(previousItem) == 0) {
        modeName = QStringLiteral("after");
        anchorItem = previousItem;
    }

    const int parentIndex = parentItem.isEmpty() ? -1 : remainingItems.indexOf(parentItem);
    const int anchorIndex = anchorItem.isEmpty() ? -1 : remainingItems.indexOf(anchorItem);
    return QVariantMap {
        {QStringLiteral("modeName"), modeName},
        {QStringLiteral("parentItemKey"), parentIndex >= 0 ? descriptorKey(parentItem, parentIndex) : QString()},
        {QStringLiteral("parentLabel"), parentIndex >= 0 ? descriptorLabel(parentItem, parentIndex) : QString()},
        {QStringLiteral("parentPathLabel"), parentItem.value(QStringLiteral("pathLabel")).toString()},
        {QStringLiteral("anchorItemKey"), anchorIndex >= 0 ? descriptorKey(anchorItem, anchorIndex) : QString()},
        {QStringLiteral("anchorLabel"), anchorIndex >= 0 ? descriptorLabel(anchorItem, anchorIndex) : QString()}
    };
}

bool HierarchyModel::setRole(QString *target, const QString &value)
{
    if (!target)
        return false;

    const QString next = normalizedRole(value);
    if (*target == next)
        return false;

    *target = next;
    emit rolesChanged();
    rebuildDescriptors();
    return true;
}

QVariant HierarchyModel::firstRoleValue(const QVariant &entry,
                                        const QStringList &roles,
                                        const QVariant &fallbackValue) const
{
    for (const QString &role : roles) {
        const QString key = normalizedRole(role);
        if (key.isEmpty())
            continue;
        const QVariant value = m_source.roleValue(entry, key, QVariant());
        if (value.isValid() && !value.isNull())
            return value;
    }
    return fallbackValue;
}

bool HierarchyModel::boolRole(const QVariant &entry, const QString &roleName, bool fallbackValue) const
{
    if (!variantLooksObjectLike(entry))
        return fallbackValue;
    return m_source.boolValue(entry, roleName, fallbackValue);
}

int HierarchyModel::intRole(const QVariant &entry, const QString &roleName, int fallbackValue) const
{
    if (!variantLooksObjectLike(entry))
        return fallbackValue;
    return m_source.intValue(entry, roleName, fallbackValue);
}

void HierarchyModel::rebuildDescriptors()
{
    const int previousCount = m_descriptors.size();
    QVariantList nextDescriptors;
    nextDescriptors.reserve(m_source.count());

    for (int index = 0; index < m_source.count(); ++index) {
        const QVariant node = m_source.at(index);
        if (!node.isValid() || node.isNull())
            continue;

        const bool objectNode = variantLooksObjectLike(node);
        const QString primitiveLabel = objectNode ? QString() : node.toString();
        const QVariant labelRaw = objectNode
            ? firstRoleValue(node,
                             {m_labelRole,
                              QStringLiteral("text"),
                              QStringLiteral("title"),
                              QStringLiteral("name"),
                              QStringLiteral("display"),
                              QStringLiteral("edit")},
                             QString())
            : primitiveLabel;
        const QString label = textFromVariant(labelRaw);

        QString iconName;
        QString iconSource;
        if (objectNode) {
            const QVariant iconToken = firstRoleValue(node, {m_iconNameRole, QStringLiteral("icon")}, QString());
            const QVariantMap iconMap = iconToken.toMap();
            if (!iconMap.isEmpty()) {
                iconName = textFromVariant(iconMap.value(QStringLiteral("name"))).trimmed();
                iconSource = textFromVariant(firstRoleValue(iconMap,
                                                           {QStringLiteral("source"), QStringLiteral("url")},
                                                           QString())).trimmed();
            } else {
                const QString iconText = textFromVariant(iconToken).trimmed();
                if (stringStartsLikeUrl(iconText))
                    iconSource = iconText;
                else
                    iconName = iconText;
            }

            const QString explicitIconSource = textFromVariant(firstRoleValue(node, {m_iconSourceRole}, QString())).trimmed();
            if (!explicitIconSource.isEmpty())
                iconSource = explicitIconSource;
        }

        const QString iconGlyph = objectNode
            ? textFromVariant(firstRoleValue(node, {m_iconGlyphRole}, QString()))
            : QString();

        const int countValue = intRole(node, m_countRole, intRole(node, QStringLiteral("count"), -1));
        const int itemId = intRole(node, m_itemIdRole, intRole(node, QStringLiteral("id"), -1));

        const QString explicitKey = objectNode
            ? textFromVariant(firstRoleValue(node, {m_itemKeyRole}, QString())).trimmed()
            : QString();
        const QString fallbackKey = QString::number(index);
        const QString itemKey = !explicitKey.isEmpty()
            ? explicitKey
            : itemId >= 0 ? QString::number(itemId) : fallbackKey;
        const QString displayLabel = !label.isEmpty() ? label : itemKey;

        const QVariant explicitChevron = objectNode
            ? firstRoleValue(node, {m_showChevronRole}, QVariant())
            : QVariant();
        const bool showChevron = explicitChevron.isValid() && !explicitChevron.isNull()
            ? m_source.boolValue(node, m_showChevronRole, true)
            : true;

        const QVariant explicitIndentLevel = objectNode
            ? firstRoleValue(node, {QStringLiteral("indentLevel")}, QVariant())
            : QVariant();
        const QVariant explicitDepth = objectNode
            ? firstRoleValue(node, {m_depthRole}, QVariant())
            : QVariant();
        const int indentLevel = explicitIndentLevel.isValid() && !explicitIndentLevel.isNull()
            ? normalizedDepth(explicitIndentLevel, 0)
            : explicitDepth.isValid() && !explicitDepth.isNull()
                ? normalizedDepth(explicitDepth, 0)
                : 0;

        const QString pathLabelRaw = objectNode
            ? textFromVariant(firstRoleValue(node, {QStringLiteral("pathLabel")}, QString())).trimmed()
            : QString();
        const QString pathLabel = !pathLabelRaw.isEmpty() ? pathLabelRaw : displayLabel;

        const QString parentItemKey = objectNode
            ? textFromVariant(firstRoleValue(node,
                                            {QStringLiteral("parentItemKey"), QStringLiteral("parentKey")},
                                            QString())).trimmed()
            : QString();

        const bool activatable = boolRole(node,
                                          m_activatableRole,
                                          boolRole(node,
                                                   QStringLiteral("selectable"),
                                                   boolRole(node, QStringLiteral("activatable"), true)));
        const bool draggable = boolRole(node,
                                        m_draggableRole,
                                        boolRole(node,
                                                 QStringLiteral("dragAllowed"),
                                                 boolRole(node, QStringLiteral("draggable"), true)));

        nextDescriptors.append(QVariantMap {
            {QStringLiteral("itemId"), itemId},
            {QStringLiteral("itemKey"), itemKey},
            {QStringLiteral("parentItemKey"), parentItemKey},
            {QStringLiteral("pathLabel"), pathLabel},
            {QStringLiteral("nodeData"), node},
            {QStringLiteral("label"), displayLabel},
            {QStringLiteral("iconName"), iconName},
            {QStringLiteral("iconSource"), iconSource},
            {QStringLiteral("iconGlyph"), iconGlyph},
            {QStringLiteral("count"), countValue},
            {QStringLiteral("showChevron"), showChevron},
            {QStringLiteral("hasChildren"), boolRole(node, QStringLiteral("hasChildItems"), false)},
            {QStringLiteral("expanded"), boolRole(node, m_expandedRole, false)},
            {QStringLiteral("selected"), boolRole(node, m_selectedRole, false)},
            {QStringLiteral("enabled"), boolRole(node, m_enabledRole, true)},
            {QStringLiteral("activatable"), activatable},
            {QStringLiteral("draggable"), draggable},
            {QStringLiteral("indentLevel"), indentLevel}
        });
    }

    if (m_descriptors == nextDescriptors)
        return;

    m_descriptors = nextDescriptors;
    m_revision += 1;
    emit descriptorsChanged();
    emit revisionChanged();
    if (previousCount != m_descriptors.size())
        emit sourceChanged();
}
