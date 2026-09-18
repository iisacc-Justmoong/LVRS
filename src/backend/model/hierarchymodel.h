#pragma once

#include "backend/model/modelsource.h"

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqml.h>

class QAbstractItemModel;
class QJSValue;

class HierarchyModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(HierarchyModel)

    Q_PROPERTY(QVariant source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(int column READ column WRITE setColumn NOTIFY columnChanged)
    Q_PROPERTY(QString itemIdRole READ itemIdRole WRITE setItemIdRole NOTIFY rolesChanged)
    Q_PROPERTY(QString itemKeyRole READ itemKeyRole WRITE setItemKeyRole NOTIFY rolesChanged)
    Q_PROPERTY(QString labelRole READ labelRole WRITE setLabelRole NOTIFY rolesChanged)
    Q_PROPERTY(QString iconNameRole READ iconNameRole WRITE setIconNameRole NOTIFY rolesChanged)
    Q_PROPERTY(QString iconSourceRole READ iconSourceRole WRITE setIconSourceRole NOTIFY rolesChanged)
    Q_PROPERTY(QString iconGlyphRole READ iconGlyphRole WRITE setIconGlyphRole NOTIFY rolesChanged)
    Q_PROPERTY(QString countRole READ countRole WRITE setCountRole NOTIFY rolesChanged)
    Q_PROPERTY(QString enabledRole READ enabledRole WRITE setEnabledRole NOTIFY rolesChanged)
    Q_PROPERTY(QString expandedRole READ expandedRole WRITE setExpandedRole NOTIFY rolesChanged)
    Q_PROPERTY(QString selectedRole READ selectedRole WRITE setSelectedRole NOTIFY rolesChanged)
    Q_PROPERTY(QString activatableRole READ activatableRole WRITE setActivatableRole NOTIFY rolesChanged)
    Q_PROPERTY(QString draggableRole READ draggableRole WRITE setDraggableRole NOTIFY rolesChanged)
    Q_PROPERTY(QString showChevronRole READ showChevronRole WRITE setShowChevronRole NOTIFY rolesChanged)
    Q_PROPERTY(QString depthRole READ depthRole WRITE setDepthRole NOTIFY rolesChanged)
    Q_PROPERTY(QVariantList descriptors READ descriptors NOTIFY descriptorsChanged)
    Q_PROPERTY(int count READ count NOTIFY descriptorsChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(bool hasSource READ hasSource NOTIFY sourceChanged)

public:
    explicit HierarchyModel(QObject *parent = nullptr);

    QVariant source() const;
    void setSource(const QVariant &source);

    int column() const;
    void setColumn(int column);

    QString itemIdRole() const;
    void setItemIdRole(const QString &value);

    QString itemKeyRole() const;
    void setItemKeyRole(const QString &value);

    QString labelRole() const;
    void setLabelRole(const QString &value);

    QString iconNameRole() const;
    void setIconNameRole(const QString &value);

    QString iconSourceRole() const;
    void setIconSourceRole(const QString &value);

    QString iconGlyphRole() const;
    void setIconGlyphRole(const QString &value);

    QString countRole() const;
    void setCountRole(const QString &value);

    QString enabledRole() const;
    void setEnabledRole(const QString &value);

    QString expandedRole() const;
    void setExpandedRole(const QString &value);

    QString selectedRole() const;
    void setSelectedRole(const QString &value);

    QString activatableRole() const;
    void setActivatableRole(const QString &value);

    QString draggableRole() const;
    void setDraggableRole(const QString &value);

    QString showChevronRole() const;
    void setShowChevronRole(const QString &value);

    QString depthRole() const;
    void setDepthRole(const QString &value);

    QVariantList descriptors() const;
    int count() const;
    int revision() const;
    bool hasSource() const;

    Q_INVOKABLE QVariantMap descriptorAt(int index) const;
    Q_INVOKABLE QVariant roleValue(const QVariant &entry,
                                   const QString &roleName,
                                   const QVariant &fallbackValue = QVariant()) const;
    Q_INVOKABLE bool depthArraySupportsEditing(const QVariant &nodes) const;
    Q_INVOKABLE bool sourceSupportsEditing() const;
    Q_INVOKABLE QVariantMap projectInteractionState(const QVariantList &items) const;
    Q_INVOKABLE int descendantRangeEnd(const QVariantList &items, int itemIndex) const;
    Q_INVOKABLE QVariantMap resolveDragTarget(const QVariantList &items,
                                              int sourceStart,
                                              int sourceEnd,
                                              int rawInsertionIndex,
                                              double localX,
                                              double indentStep,
                                              double basePadding) const;
    Q_INVOKABLE QVariantMap moveDescriptors(const QVariantList &items,
                                            int sourceStart,
                                            int sourceEnd,
                                            int targetIndex,
                                            int targetDepth) const;
    Q_INVOKABLE QVariantMap moveSourceRows(int sourceStart,
                                           int sourceEnd,
                                           int targetIndex,
                                           int targetDepth);
    Q_INVOKABLE void invalidate();

signals:
    void sourceChanged();
    void columnChanged();
    void rolesChanged();
    void descriptorsChanged();
    void revisionChanged();

private:
    static bool variantLooksObjectLike(const QVariant &value);
    static bool stringStartsLikeUrl(const QString &value);
    static int normalizedDepth(const QVariant &rawDepth, int fallbackDepth);
    static int descriptorIndent(const QVariantMap &descriptor);
    static bool descriptorCanBecomeActive(const QVariantMap &descriptor);
    static QString descriptorKey(const QVariantMap &descriptor, int index);
    static QString descriptorLabel(const QVariantMap &descriptor, int index);
    static QVariantMap dropDescriptorFor(const QVariantList &remainingItems, int insertionIndex, int depth);
    static QObject *objectFromVariant(const QVariant &value);
    static QAbstractItemModel *itemModelFromVariant(const QVariant &value);

    bool setRole(QString *target, const QString &value);
    QVariant firstRoleValue(const QVariant &entry,
                            const QStringList &roles,
                            const QVariant &fallbackValue = QVariant()) const;
    bool boolRole(const QVariant &entry, const QString &roleName, bool fallbackValue) const;
    int intRole(const QVariant &entry, const QString &roleName, int fallbackValue) const;
    bool jsModelSupportsEditing(const QJSValue &value) const;
    bool itemModelSupportsEditing(QAbstractItemModel *model) const;
    bool objectModelSupportsEditing(QObject *object) const;
    bool descriptorHasWritableDepthRole(const QVariantMap &descriptor) const;
    bool setJsDescriptorState(QJSValue *instance,
                              int row,
                              const QVariantMap &descriptor,
                              const QString &parentItemKey) const;
    bool setItemModelDescriptorState(QAbstractItemModel *model,
                                     int row,
                                     const QVariantMap &descriptor,
                                     const QString &parentItemKey) const;
    bool setObjectDescriptorState(QObject *object,
                                  int row,
                                  const QVariantMap &descriptor,
                                  const QString &parentItemKey) const;
    bool applyMoveToItemModel(QAbstractItemModel *model,
                              const QVariantMap &moveResult,
                              int sourceStart,
                              int sourceEnd) const;
    bool applyMoveToObjectModel(QObject *object,
                                const QVariantMap &moveResult,
                                int sourceStart,
                                int sourceEnd) const;
    bool applyMoveToJsModel(QJSValue value,
                            const QVariantMap &moveResult,
                            int sourceStart,
                            int sourceEnd) const;
    void rebuildDescriptors();

    ModelSource m_source;
    QVariantList m_descriptors;
    int m_revision = 0;

    QString m_itemIdRole = QStringLiteral("itemId");
    QString m_itemKeyRole = QStringLiteral("key");
    QString m_labelRole = QStringLiteral("label");
    QString m_iconNameRole = QStringLiteral("iconName");
    QString m_iconSourceRole = QStringLiteral("iconSource");
    QString m_iconGlyphRole = QStringLiteral("iconGlyph");
    QString m_countRole = QStringLiteral("count");
    QString m_enabledRole = QStringLiteral("enabled");
    QString m_expandedRole = QStringLiteral("expanded");
    QString m_selectedRole = QStringLiteral("selected");
    QString m_activatableRole = QStringLiteral("activatable");
    QString m_draggableRole = QStringLiteral("draggable");
    QString m_showChevronRole = QStringLiteral("showChevron");
    QString m_depthRole = QStringLiteral("depth");
};
