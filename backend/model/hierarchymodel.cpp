#include "backend/model/hierarchymodel.h"

#include <QJSValue>
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
