#include "backend/model/tableheadermodel.h"

#include <QJSValue>
#include <QObject>
#include <QMetaObject>
#include <QtMath>

namespace {
QObject *objectFromVariant(const QVariant &value)
{
    QObject *object = nullptr;
    if (value.canConvert<QObject *>())
        object = value.value<QObject *>();
    if (!object)
        object = qvariant_cast<QObject *>(value);
    return object;
}

int jsLength(const QJSValue &value)
{
    const QJSValue length = value.property(QStringLiteral("length"));
    if (length.isNumber())
        return qMax(0, length.toInt());
    const QJSValue count = value.property(QStringLiteral("count"));
    if (count.isNumber())
        return qMax(0, count.toInt());
    return 0;
}

QVariant jsEntryAt(QJSValue value, int index)
{
    if (index < 0)
        return {};
    if (value.isArray())
        return value.property(static_cast<quint32>(index)).toVariant();
    QJSValue getter = value.property(QStringLiteral("get"));
    if (getter.isCallable())
        return getter.callWithInstance(value, QJSValueList {QJSValue(index)}).toVariant();
    return value.property(static_cast<quint32>(index)).toVariant();
}
} // namespace

TableHeaderModel::TableHeaderModel(QObject *parent)
    : QObject(parent)
{
    rebuildDescriptors();
}

QVariant TableHeaderModel::cellItems() const
{
    return m_cellItems;
}

void TableHeaderModel::setCellItems(const QVariant &value)
{
    if (m_cellItems == value)
        return;
    m_cellItems = value;
    emit sourceChanged();
    rebuildDescriptors();
}

QVariant TableHeaderModel::columns() const
{
    return m_columns;
}

void TableHeaderModel::setColumns(const QVariant &value)
{
    if (m_columns == value)
        return;
    m_columns = value;
    emit sourceChanged();
    rebuildDescriptors();
}

double TableHeaderModel::tableWidth() const
{
    return m_tableWidth;
}

void TableHeaderModel::setTableWidth(double value)
{
    const double next = qMax(0.0, value);
    if (qFuzzyCompare(m_tableWidth, next))
        return;
    m_tableWidth = next;
    emit geometryChanged();
    rebuildDescriptors();
}

int TableHeaderModel::rowHeight() const
{
    return m_rowHeight;
}

void TableHeaderModel::setRowHeight(int value)
{
    const int next = qMax(1, value);
    if (m_rowHeight == next)
        return;
    m_rowHeight = next;
    emit geometryChanged();
    rebuildDescriptors();
}

int TableHeaderModel::cellHorizontalPadding() const
{
    return m_cellHorizontalPadding;
}

void TableHeaderModel::setCellHorizontalPadding(int value)
{
    const int next = qMax(0, value);
    if (m_cellHorizontalPadding == next)
        return;
    m_cellHorizontalPadding = next;
    emit geometryChanged();
    rebuildDescriptors();
}

QVariant TableHeaderModel::columnWidths() const
{
    return m_columnWidths;
}

void TableHeaderModel::setColumnWidths(const QVariant &value)
{
    const QVariantList next = listFromVariant(value);
    if (m_columnWidths == next)
        return;
    m_columnWidths = next;
    emit geometryChanged();
    rebuildDescriptors();
}

int TableHeaderModel::fallbackCellWidth() const
{
    return m_fallbackCellWidth;
}

void TableHeaderModel::setFallbackCellWidth(int value)
{
    const int next = qMax(0, value);
    if (m_fallbackCellWidth == next)
        return;
    m_fallbackCellWidth = next;
    emit geometryChanged();
    rebuildDescriptors();
}

int TableHeaderModel::minColumnWidth() const
{
    return m_minColumnWidth;
}

void TableHeaderModel::setMinColumnWidth(int value)
{
    const int next = qMax(1, value);
    if (m_minColumnWidth == next)
        return;
    m_minColumnWidth = next;
    emit geometryChanged();
    rebuildDescriptors();
}

QVariantList TableHeaderModel::descriptors() const
{
    return m_descriptors;
}

int TableHeaderModel::columnCount() const
{
    return listFromVariant(resolvedColumnSource()).size();
}

int TableHeaderModel::revision() const
{
    return m_revision;
}

QVariant TableHeaderModel::resolvedColumnSource() const
{
    if (m_cellItems.isValid() && !m_cellItems.isNull())
        return m_cellItems;
    return m_columns;
}

QVariant TableHeaderModel::columnAt(int index) const
{
    const QVariantList source = listFromVariant(resolvedColumnSource());
    return index >= 0 && index < source.size() ? source.at(index) : QVariant();
}

QString TableHeaderModel::normalizeColumnType(const QVariant &value) const
{
    if (!value.isValid() || value.isNull())
        return QStringLiteral("string");
    const QString name = value.toString().trimmed().toLower();
    if (name == QStringLiteral("int") || name == QStringLiteral("integer"))
        return QStringLiteral("int");
    if (name == QStringLiteral("float") || name == QStringLiteral("real") || name == QStringLiteral("double")
            || name == QStringLiteral("number") || name == QStringLiteral("decimal")) {
        return QStringLiteral("float");
    }
    if (name == QStringLiteral("bool") || name == QStringLiteral("boolean"))
        return QStringLiteral("bool");
    return QStringLiteral("string");
}

QString TableHeaderModel::inferredColumnType(const QVariant &value) const
{
    if (value.metaType().id() == QMetaType::Bool)
        return QStringLiteral("bool");
    bool ok = false;
    const double numeric = value.toDouble(&ok);
    if (ok)
        return qFloor(numeric) == numeric ? QStringLiteral("int") : QStringLiteral("float");
    return QStringLiteral("string");
}

QString TableHeaderModel::columnType(int index) const
{
    const QVariant entry = columnAt(index);
    const QVariantMap map = mapFromVariant(entry);
    if (!map.isEmpty()) {
        const QVariant type = roleValue(entry,
                                        {QStringLiteral("type"),
                                         QStringLiteral("valueType"),
                                         QStringLiteral("cellType"),
                                         QStringLiteral("dataType")});
        if (type.isValid() && !type.isNull())
            return normalizeColumnType(type);
        const QVariant value = map.value(QStringLiteral("value"));
        if (value.isValid() && !value.isNull())
            return inferredColumnType(value);
        return QStringLiteral("string");
    }
    return inferredColumnType(entry);
}

QString TableHeaderModel::columnText(int index) const
{
    const QVariant entry = columnAt(index);
    if (entry.metaType().id() == QMetaType::QString
            || entry.metaType().id() == QMetaType::Int
            || entry.metaType().id() == QMetaType::Double
            || entry.metaType().id() == QMetaType::Bool) {
        return entry.toString();
    }
    const QVariant value = roleValue(entry,
                                     {QStringLiteral("label"),
                                      QStringLiteral("text"),
                                      QStringLiteral("title"),
                                      QStringLiteral("value")},
                                     QStringLiteral("Column"));
    return value.toString();
}

int TableHeaderModel::columnPadding(int index) const
{
    const QVariant entry = columnAt(index);
    const QVariant spacing = roleValue(entry, {QStringLiteral("contentSpacing"), QStringLiteral("horizontalPadding")});
    bool ok = false;
    const int padding = spacing.toInt(&ok);
    return ok && padding >= 0 ? padding : m_cellHorizontalPadding;
}

int TableHeaderModel::numericWidth(const QVariant &value, int fallbackValue) const
{
    bool ok = false;
    const double parsed = value.toDouble(&ok);
    if (!ok || parsed <= 0)
        return fallbackValue;
    return qMax(m_minColumnWidth, qRound(parsed));
}

int TableHeaderModel::autoColumnWidth() const
{
    const int count = qMax(1, columnCount());
    return qMax(m_minColumnWidth, static_cast<int>(qFloor(m_tableWidth / count)));
}

int TableHeaderModel::columnWidth(int index) const
{
    if (index >= 0 && index < m_columnWidths.size()) {
        const int fallback = m_fallbackCellWidth > 0 ? m_fallbackCellWidth : autoColumnWidth();
        return numericWidth(m_columnWidths.at(index), fallback);
    }
    if (m_fallbackCellWidth > 0)
        return numericWidth(m_fallbackCellWidth, autoColumnWidth());
    return autoColumnWidth();
}

int TableHeaderModel::columnX(int index) const
{
    int xValue = 0;
    for (int column = 0; column < index; ++column)
        xValue += columnWidth(column);
    return xValue;
}

QVariantMap TableHeaderModel::descriptorAt(int index) const
{
    if (index < 0)
        return {};
    if (index >= m_descriptors.size()) {
        return QVariantMap {
            {QStringLiteral("index"), index},
            {QStringLiteral("sourceData"), QVariant()},
            {QStringLiteral("text"), QStringLiteral("Column")},
            {QStringLiteral("valueType"), QStringLiteral("string")},
            {QStringLiteral("x"), columnX(index)},
            {QStringLiteral("width"), columnWidth(index)},
            {QStringLiteral("height"), m_rowHeight},
            {QStringLiteral("padding"), m_cellHorizontalPadding}
        };
    }
    return m_descriptors.at(index).toMap();
}

QVariantList TableHeaderModel::listFromVariant(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return {};
    if (value.userType() == qMetaTypeId<QJSValue>()) {
        QJSValue jsValue = value.value<QJSValue>();
        const QVariant converted = jsValue.toVariant();
        if (converted.canConvert<QVariantList>())
            return converted.toList();
        QVariantList result;
        const int count = jsLength(jsValue);
        result.reserve(count);
        for (int index = 0; index < count; ++index)
            result.append(jsEntryAt(jsValue, index));
        return result;
    }
    if (value.canConvert<QVariantList>())
        return value.toList();
    if (value.canConvert<QStringList>()) {
        QVariantList result;
        const QStringList strings = value.toStringList();
        result.reserve(strings.size());
        for (const QString &entry : strings)
            result.append(entry);
        return result;
    }
    if (QObject *object = objectFromVariant(value)) {
        const int count = qMax(0, object->property("count").isValid()
            ? object->property("count").toInt()
            : object->property("length").toInt());
        QVariantList result;
        result.reserve(count);
        for (int index = 0; index < count; ++index) {
            QVariant returned;
            if (QMetaObject::invokeMethod(object, "get", Q_RETURN_ARG(QVariant, returned), Q_ARG(int, index)))
                result.append(returned);
        }
        return result;
    }
    return {};
}

QVariantMap TableHeaderModel::mapFromVariant(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return {};
    if (value.userType() == qMetaTypeId<QJSValue>())
        return value.value<QJSValue>().toVariant().toMap();
    if (value.canConvert<QVariantMap>())
        return value.toMap();
    return {};
}

QVariant TableHeaderModel::roleValue(const QVariant &entry, const QStringList &keys, const QVariant &fallback)
{
    const QVariantMap map = mapFromVariant(entry);
    for (const QString &key : keys) {
        if (map.contains(key))
            return map.value(key);
    }
    return fallback;
}

void TableHeaderModel::rebuildDescriptors()
{
    QVariantList next;
    const int count = columnCount();
    next.reserve(count);
    for (int index = 0; index < count; ++index) {
        next.append(QVariantMap {
            {QStringLiteral("index"), index},
            {QStringLiteral("sourceData"), columnAt(index)},
            {QStringLiteral("text"), columnText(index)},
            {QStringLiteral("valueType"), columnType(index)},
            {QStringLiteral("x"), columnX(index)},
            {QStringLiteral("width"), columnWidth(index)},
            {QStringLiteral("height"), m_rowHeight},
            {QStringLiteral("padding"), columnPadding(index)}
        });
    }

    if (m_descriptors == next)
        return;
    m_descriptors = next;
    ++m_revision;
    emit descriptorsChanged();
    emit revisionChanged();
}
