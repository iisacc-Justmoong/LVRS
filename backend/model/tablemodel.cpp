#include "backend/model/tablemodel.h"

#include <QAbstractItemModel>
#include <QJSValue>
#include <QMetaObject>
#include <QtMath>

#include <utility>

namespace {
QVariantMap resultMap(bool accepted, const QString &type, const QVariant &value, const QString &text)
{
    return QVariantMap {
        {QStringLiteral("accepted"), accepted},
        {QStringLiteral("type"), type},
        {QStringLiteral("value"), value},
        {QStringLiteral("text"), text}
    };
}

bool isFiniteNumber(double value)
{
    return std::isfinite(value);
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
    if (getter.isCallable()) {
        QJSValueList args;
        args << QJSValue(index);
        return getter.callWithInstance(value, args).toVariant();
    }

    QJSValue at = value.property(QStringLiteral("at"));
    if (at.isCallable()) {
        QJSValueList args;
        args << QJSValue(index);
        return at.callWithInstance(value, args).toVariant();
    }

    return value.property(static_cast<quint32>(index)).toVariant();
}

QObject *objectFromVariant(const QVariant &value)
{
    QObject *object = nullptr;
    if (value.canConvert<QObject *>())
        object = value.value<QObject *>();
    if (!object)
        object = qvariant_cast<QObject *>(value);
    return object;
}

int roleIdForName(QAbstractItemModel *model, const QByteArray &name)
{
    if (!model)
        return -1;
    const QHash<int, QByteArray> roles = model->roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        if (it.value() == name)
            return it.key();
    }
    return -1;
}

QVariantMap cellMapFromModelIndex(QAbstractItemModel *model, const QModelIndex &index)
{
    if (!model || !index.isValid())
        return {};

    QVariantMap result;
    const QHash<int, QByteArray> roles = model->roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        const QString key = QString::fromUtf8(it.value());
        if (key.isEmpty())
            continue;
        const QVariant roleData = model->data(index, it.key());
        if (roleData.isValid())
            result.insert(key, roleData);
    }

    const QVariant displayValue = model->data(index, Qt::DisplayRole);
    const QVariant editValue = model->data(index, Qt::EditRole);
    if (displayValue.isValid())
        result.insert(QStringLiteral("display"), displayValue);
    if (editValue.isValid())
        result.insert(QStringLiteral("edit"), editValue);
    if (!result.contains(QStringLiteral("value"))) {
        if (editValue.isValid())
            result.insert(QStringLiteral("value"), editValue);
        else if (displayValue.isValid())
            result.insert(QStringLiteral("value"), displayValue);
    }
    if (!result.contains(QStringLiteral("text"))) {
        const QVariant textValue = displayValue.isValid()
            ? displayValue
            : result.value(QStringLiteral("value"));
        if (textValue.isValid() && !textValue.isNull())
            result.insert(QStringLiteral("text"), textValue.toString());
    }
    return result;
}
} // namespace

TableModel::TableModel(QObject *parent)
    : QObject(parent)
    , m_undoStack(this)
{
    m_rows = {
        QVariantList {QStringLiteral("Text"), QStringLiteral("Text"), QStringLiteral("Text")},
        QVariantList {QStringLiteral("Text"), QStringLiteral("Text"), QStringLiteral("Text")},
        QVariantList {QStringLiteral("Text"), QStringLiteral("Text"), QStringLiteral("Text")},
        QVariantList {QStringLiteral("Text"), QStringLiteral("Text"), QStringLiteral("Text")}
    };
    m_rowsSource = m_rows;
    connect(&m_undoStack, &ModelUndoStack::stackChanged, this, &TableModel::undoStackChanged);
}

QVariant TableModel::rows() const
{
    return m_rows;
}

void TableModel::setRows(const QVariant &rows)
{
    const QVariantList next = listFromVariant(rows);
    const bool mutableRows = isMutableListVariant(rows);
    m_rowsSource = rows;
    m_rows = next;
    m_rowsStructureMutable = mutableRows;
    reconnectRowsSourceSignals();
    clearHistory();
    bumpRevision();
    emit rowsChanged();
    emit headersChanged();
    emit geometryChanged();
    emit modelChanged();
}

QVariant TableModel::headerCellItems() const
{
    return m_headerCellItems;
}

void TableModel::setHeaderCellItems(const QVariant &headerCellItems)
{
    if (m_headerCellItems == headerCellItems)
        return;
    m_headerCellItems = headerCellItems;
    clearHistory();
    bumpRevision();
    emit headerCellItemsChanged();
    emit headersChanged();
    emit geometryChanged();
    emit modelChanged();
}

QVariant TableModel::headerColumns() const
{
    return m_headerColumns;
}

void TableModel::setHeaderColumns(const QVariant &headerColumns)
{
    if (m_headerColumns == headerColumns)
        return;
    m_headerColumns = headerColumns;
    clearHistory();
    bumpRevision();
    emit headerColumnsChanged();
    emit headersChanged();
    emit geometryChanged();
    emit modelChanged();
}

QString TableModel::defaultHeaderText() const
{
    return m_defaultHeaderText;
}

void TableModel::setDefaultHeaderText(const QString &value)
{
    if (m_defaultHeaderText == value)
        return;
    m_defaultHeaderText = value;
    emit defaultHeaderTextChanged();
}

QString TableModel::defaultCellText() const
{
    return m_defaultCellText;
}

void TableModel::setDefaultCellText(const QString &value)
{
    if (m_defaultCellText == value)
        return;
    m_defaultCellText = value;
    bumpRevision();
    emit defaultCellTextChanged();
    emit geometryChanged();
    emit modelChanged();
}

bool TableModel::inputable() const
{
    return m_inputable;
}

void TableModel::setInputable(bool value)
{
    if (m_inputable == value)
        return;
    m_inputable = value;
    bumpRevision();
    emit inputableChanged();
    emit geometryChanged();
    emit modelChanged();
}

double TableModel::tableWidth() const
{
    return m_tableWidth;
}

void TableModel::setTableWidth(double value)
{
    const double next = qMax(0.0, value);
    if (qFuzzyCompare(m_tableWidth, next))
        return;
    m_tableWidth = next;
    bumpRevision();
    emit geometryChanged();
}

int TableModel::rowHeight() const
{
    return m_rowHeight;
}

void TableModel::setDefaultRowHeight(int value)
{
    const int next = qMax(1, value);
    if (m_rowHeight == next)
        return;
    m_rowHeight = next;
    bumpRevision();
    emit geometryChanged();
}

int TableModel::cellWidth() const
{
    return m_cellWidth;
}

void TableModel::setCellWidth(int value)
{
    const int next = qMax(0, value);
    if (m_cellWidth == next)
        return;
    m_cellWidth = next;
    bumpRevision();
    emit geometryChanged();
}

QVariant TableModel::columnWidths() const
{
    return m_columnWidths;
}

void TableModel::setColumnWidths(const QVariant &value)
{
    const QVariantList next = listFromVariant(value);
    if (m_columnWidths == next)
        return;
    m_columnWidths = next;
    bumpRevision();
    emit columnWidthsChanged();
    emit geometryChanged();
}

QVariant TableModel::rowHeights() const
{
    return m_rowHeights;
}

void TableModel::setRowHeights(const QVariant &value)
{
    const QVariantList next = listFromVariant(value);
    if (m_rowHeights == next)
        return;
    m_rowHeights = next;
    bumpRevision();
    emit rowHeightsChanged();
    emit geometryChanged();
}

int TableModel::minColumnWidth() const
{
    return m_minColumnWidth;
}

void TableModel::setMinColumnWidth(int value)
{
    const int next = qMax(1, value);
    if (m_minColumnWidth == next)
        return;
    m_minColumnWidth = next;
    bumpRevision();
    emit geometryChanged();
}

int TableModel::minRowHeight() const
{
    return m_minRowHeight;
}

void TableModel::setMinRowHeight(int value)
{
    const int next = qMax(1, value);
    if (m_minRowHeight == next)
        return;
    m_minRowHeight = next;
    bumpRevision();
    emit geometryChanged();
}

int TableModel::resizingColumnIndex() const
{
    return m_resizingColumnIndex;
}

int TableModel::resizingRowIndex() const
{
    return m_resizingRowIndex;
}

int TableModel::contextRowIndex() const
{
    return m_contextRowIndex;
}

int TableModel::contextColumnIndex() const
{
    return m_contextColumnIndex;
}

int TableModel::rowCount() const
{
    return m_rows.size();
}

int TableModel::headerCount() const
{
    return headerSourceList().size();
}

int TableModel::columnCount() const
{
    int count = qMax(1, headerCount());
    for (const QVariant &row : m_rows)
        count = qMax(count, columnCountForRow(row));
    return count;
}

bool TableModel::structureMutationAvailable() const
{
    return canMutateStructureInternal();
}

bool TableModel::rowsModelBacked() const
{
    return itemModelFromVariant(m_rowsSource) != nullptr;
}

bool TableModel::cellEditingAvailable() const
{
    QAbstractItemModel *model = itemModelFromVariant(m_rowsSource);
    if (!model)
        return canMutateStructureInternal();

    const int rows = model->rowCount(QModelIndex());
    const int columns = model->columnCount(QModelIndex());
    for (int row = 0; row < rows; ++row) {
        for (int column = 0; column < columns; ++column) {
            const QModelIndex index = model->index(row, column, QModelIndex());
            if (index.isValid() && model->flags(index).testFlag(Qt::ItemIsEditable))
                return true;
        }
    }
    return false;
}

int TableModel::revision() const
{
    return m_revision;
}

ModelUndoStack *TableModel::undoStack()
{
    return &m_undoStack;
}

bool TableModel::canUndo() const
{
    return m_undoStack.canUndo();
}

bool TableModel::canRedo() const
{
    return m_undoStack.canRedo();
}

int TableModel::undoDepth() const
{
    return m_undoStack.undoDepth();
}

int TableModel::redoDepth() const
{
    return m_undoStack.redoDepth();
}

QVariant TableModel::resolvedHeaderSource() const
{
    return headerSourceVariant();
}

QVariant TableModel::rowAt(int rowIndex) const
{
    return rowIndex >= 0 && rowIndex < m_rows.size() ? m_rows.at(rowIndex) : QVariant();
}

QVariant TableModel::cellAt(int rowIndex, int columnIndex) const
{
    return entryAt(rowAt(rowIndex), columnIndex);
}

QVariant TableModel::headerAt(int columnIndex) const
{
    return entryAt(headerSourceVariant(), columnIndex);
}

int TableModel::columnCountForRow(const QVariant &rowEntry) const
{
    const int count = listFromVariant(rowEntry).size();
    return count > 0 ? count : qMax(1, headerCount());
}

QString TableModel::normalizeHeaderCellType(const QVariant &value) const
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

QString TableModel::inferredCellType(const QVariant &value) const
{
    if (value.metaType().id() == QMetaType::Bool)
        return QStringLiteral("bool");
    bool ok = false;
    const double numeric = value.toDouble(&ok);
    if (ok)
        return qFloor(numeric) == numeric ? QStringLiteral("int") : QStringLiteral("float");
    return QStringLiteral("string");
}

QString TableModel::headerCellType(int columnIndex) const
{
    const QVariant entry = headerAt(columnIndex);
    const QVariantMap map = mapFromVariant(entry);
    if (!map.isEmpty()) {
        const QVariant type = roleValue(entry,
                                        {QStringLiteral("type"),
                                         QStringLiteral("valueType"),
                                         QStringLiteral("cellType"),
                                         QStringLiteral("dataType")});
        if (type.isValid() && !type.isNull())
            return normalizeHeaderCellType(type);
        const QVariant value = map.value(QStringLiteral("value"));
        if (value.isValid() && !value.isNull())
            return inferredCellType(value);
        return QStringLiteral("string");
    }
    return inferredCellType(entry);
}

QString TableModel::columnType(int columnIndex) const
{
    return headerCellType(columnIndex);
}

QVariant TableModel::cellRawValue(int rowIndex, int columnIndex) const
{
    const QVariant entry = cellAt(rowIndex, columnIndex);
    const QVariantMap map = mapFromVariant(entry);
    if (!map.isEmpty()) {
        const QVariant value = roleValue(entry,
                                        {QStringLiteral("value"),
                                         QStringLiteral("text"),
                                         QStringLiteral("label"),
                                         QStringLiteral("title")});
        return value.isValid() ? value : QVariant();
    }
    return entry;
}

QVariant TableModel::typedDefaultValue(const QString &valueType) const
{
    const QString normalizedType = normalizeHeaderCellType(valueType);
    if (normalizedType == QStringLiteral("int") || normalizedType == QStringLiteral("float"))
        return 0;
    if (normalizedType == QStringLiteral("bool"))
        return false;
    return m_defaultCellText;
}

QVariantMap TableModel::coerceCellValue(const QVariant &value, const QString &valueType) const
{
    const QString type = normalizeHeaderCellType(valueType);
    if (type == QStringLiteral("string")) {
        const QString text = (value.isValid() && !value.isNull()) ? value.toString() : QString();
        return resultMap(true, type, text, text);
    }

    if (type == QStringLiteral("int")) {
        bool ok = false;
        const QString text = (value.isValid() && !value.isNull()) ? value.toString().trimmed() : QString();
        const int intValue = text.toInt(&ok);
        if (ok && QString::number(intValue) == text)
            return resultMap(true, type, intValue, QString::number(intValue));
        return resultMap(false, type, value, text);
    }

    if (type == QStringLiteral("float")) {
        bool ok = false;
        const double floatValue = value.toDouble(&ok);
        if (ok && isFiniteNumber(floatValue))
            return resultMap(true, type, floatValue, QString::number(floatValue));
        return resultMap(false, type, value, (value.isValid() && !value.isNull()) ? value.toString() : QString());
    }

    if (value.metaType().id() == QMetaType::Bool) {
        const bool boolValue = value.toBool();
        return resultMap(true, type, boolValue, boolValue ? QStringLiteral("true") : QStringLiteral("false"));
    }
    bool numberOk = false;
    const int intValue = value.toInt(&numberOk);
    if (numberOk && (intValue == 0 || intValue == 1)) {
        const bool boolValue = intValue == 1;
        return resultMap(true, type, boolValue, boolValue ? QStringLiteral("true") : QStringLiteral("false"));
    }
    const QString text = (value.isValid() && !value.isNull()) ? value.toString().trimmed().toLower() : QString();
    if (text == QStringLiteral("true") || text == QStringLiteral("1") || text == QStringLiteral("yes") || text == QStringLiteral("on"))
        return resultMap(true, type, true, QStringLiteral("true"));
    if (text == QStringLiteral("false") || text == QStringLiteral("0") || text == QStringLiteral("no") || text == QStringLiteral("off"))
        return resultMap(true, type, false, QStringLiteral("false"));
    return resultMap(false, type, value, text);
}

QVariantMap TableModel::validateCellInput(int, int columnIndex, const QVariant &value) const
{
    return coerceCellValue(value, headerCellType(columnIndex));
}

bool TableModel::cellValueAccepted(int rowIndex, int columnIndex, const QVariant &value) const
{
    return validateCellInput(rowIndex, columnIndex, value).value(QStringLiteral("accepted")).toBool();
}

QString TableModel::cellText(int rowIndex, int columnIndex) const
{
    const QVariant rawValue = cellRawValue(rowIndex, columnIndex);
    if (!rawValue.isValid() || rawValue.isNull())
        return m_defaultCellText;
    const QVariantMap result = validateCellInput(rowIndex, columnIndex, rawValue);
    if (result.value(QStringLiteral("accepted")).toBool())
        return result.value(QStringLiteral("text")).toString();
    return rawValue.toString();
}

bool TableModel::setCellValue(int rowIndex, int columnIndex, const QVariant &value)
{
    const QVariantMap result = validateCellInput(rowIndex, columnIndex, value);
    if (!result.value(QStringLiteral("accepted")).toBool())
        return false;

    if (QAbstractItemModel *model = itemModelFromVariant(m_rowsSource))
        return setItemModelCellValue(model, rowIndex, columnIndex, result);

    if (!canMutateStructureInternal() || rowIndex < 0 || rowIndex >= m_rows.size())
        return false;
    if (columnIndex < 0 || columnIndex >= listFromVariant(m_rows.at(rowIndex)).size())
        return false;
    recordUndoSnapshot();

    QVariantMap cell = normalizeCellObject(rowIndex, columnIndex);
    if (cell.isEmpty())
        return false;
    cell.insert(QStringLiteral("value"), result.value(QStringLiteral("value")));
    cell.insert(QStringLiteral("text"), result.value(QStringLiteral("text")));
    if (!setCellObject(rowIndex, columnIndex, cell))
        return false;
    emitRowsMutated();
    return true;
}

int TableModel::cellRowSpan(int rowIndex, int columnIndex) const
{
    const int availableRows = qMax(1, rowCount() - rowIndex);
    return qMin(availableRows, integerObjectValue(cellAt(rowIndex, columnIndex), QStringLiteral("rowSpan"), QStringLiteral("rowspan"), 1));
}

int TableModel::cellColumnSpan(int rowIndex, int columnIndex) const
{
    const int availableColumns = qMax(1, columnCountForRow(rowAt(rowIndex)) - columnIndex);
    return qMin(availableColumns, integerObjectValue(cellAt(rowIndex, columnIndex), QStringLiteral("columnSpan"), QStringLiteral("colSpan"), 1));
}

QVariantMap TableModel::mergeAnchorForCell(int rowIndex, int columnIndex) const
{
    const QVariant entry = cellAt(rowIndex, columnIndex);
    const QVariantMap map = mapFromVariant(entry);
    if (isMergeCoveredMarker(entry)
            && map.contains(QStringLiteral("_lvrsMergeAnchorRow"))
            && map.contains(QStringLiteral("_lvrsMergeAnchorColumn"))) {
        return QVariantMap {
            {QStringLiteral("rowIndex"), map.value(QStringLiteral("_lvrsMergeAnchorRow")).toInt()},
            {QStringLiteral("columnIndex"), map.value(QStringLiteral("_lvrsMergeAnchorColumn")).toInt()}
        };
    }

    for (int row = 0; row <= rowIndex; ++row) {
        const int count = columnCountForRow(rowAt(row));
        for (int column = 0; column < count; ++column) {
            if (row == rowIndex && column == columnIndex)
                continue;
            const QVariant candidate = cellAt(row, column);
            if (isMergeCoveredMarker(candidate))
                continue;
            const int rowSpan = cellRowSpan(row, column);
            const int columnSpan = cellColumnSpan(row, column);
            if ((rowSpan > 1 || columnSpan > 1)
                    && spanContains(row, column, rowSpan, columnSpan, rowIndex, columnIndex)) {
                return QVariantMap {
                    {QStringLiteral("rowIndex"), row},
                    {QStringLiteral("columnIndex"), column}
                };
            }
        }
    }

    return QVariantMap {
        {QStringLiteral("rowIndex"), rowIndex},
        {QStringLiteral("columnIndex"), columnIndex}
    };
}

bool TableModel::isCoveredCell(int rowIndex, int columnIndex) const
{
    const QVariantMap anchor = mergeAnchorForCell(rowIndex, columnIndex);
    return anchor.value(QStringLiteral("rowIndex")).toInt() != rowIndex
        || anchor.value(QStringLiteral("columnIndex")).toInt() != columnIndex;
}

QVariantList TableModel::visibleCells() const
{
    QVariantList result;
    for (int row = 0; row < rowCount(); ++row) {
        const int count = columnCountForRow(rowAt(row));
        for (int column = 0; column < count; ++column) {
            if (isCoveredCell(row, column))
                continue;
            const int rowSpan = cellRowSpan(row, column);
            const int columnSpan = cellColumnSpan(row, column);
            result.append(QVariantMap {
                {QStringLiteral("rowIndex"), row},
                {QStringLiteral("columnIndex"), column},
                {QStringLiteral("rowSpan"), rowSpan},
                {QStringLiteral("columnSpan"), columnSpan},
                {QStringLiteral("cellData"), cellAt(row, column)},
                {QStringLiteral("text"), cellText(row, column)},
                {QStringLiteral("valueType"), headerCellType(column)},
                {QStringLiteral("x"), columnX(column)},
                {QStringLiteral("y"), rowY(row)},
                {QStringLiteral("width"), columnSpanWidth(column, columnSpan)},
                {QStringLiteral("height"), rowSpanHeight(row, rowSpan)},
                {QStringLiteral("inputable"), cellInputable(row, column)}
            });
        }
    }
    return result;
}

int TableModel::numericSize(const QVariant &value, int fallbackValue, int minimumValue) const
{
    bool ok = false;
    const double parsed = value.toDouble(&ok);
    if (!ok || parsed <= 0)
        return qMax(1, qRound(static_cast<double>(fallbackValue)));
    return qMax(minimumValue, qRound(parsed));
}

int TableModel::autoCellWidth(const QVariant &rowEntry) const
{
    const int count = qMax(1, columnCountForRow(rowEntry));
    return qMax(1, static_cast<int>(qFloor(m_tableWidth / count)));
}

int TableModel::autoColumnWidth() const
{
    const int count = qMax(1, columnCount());
    return qMax(m_minColumnWidth, static_cast<int>(qFloor(m_tableWidth / count)));
}

int TableModel::columnWidth(int columnIndex) const
{
    if (columnIndex >= 0 && columnIndex < m_columnWidths.size()) {
        const int fallback = m_cellWidth > 0 ? m_cellWidth : autoColumnWidth();
        return numericSize(m_columnWidths.at(columnIndex), fallback, m_minColumnWidth);
    }
    if (m_cellWidth > 0)
        return numericSize(m_cellWidth, autoColumnWidth(), m_minColumnWidth);
    return autoColumnWidth();
}

int TableModel::columnX(int columnIndex) const
{
    int xValue = 0;
    for (int column = 0; column < columnIndex; ++column)
        xValue += columnWidth(column);
    return xValue;
}

int TableModel::columnSpanWidth(int columnIndex, int columnSpan) const
{
    const int spanValue = qMax(1, columnSpan);
    int widthValue = 0;
    for (int column = columnIndex; column < columnIndex + spanValue; ++column)
        widthValue += columnWidth(column);
    return widthValue;
}

int TableModel::rowHeightAt(int rowIndex) const
{
    if (rowIndex >= 0 && rowIndex < m_rowHeights.size())
        return numericSize(m_rowHeights.at(rowIndex), m_rowHeight, m_minRowHeight);
    return numericSize(m_rowHeight, m_rowHeight, m_minRowHeight);
}

int TableModel::rowY(int rowIndex) const
{
    int yValue = 0;
    for (int row = 0; row < rowIndex; ++row)
        yValue += rowHeightAt(row);
    return yValue;
}

int TableModel::rowSpanHeight(int rowIndex, int rowSpan) const
{
    const int spanValue = qMax(1, rowSpan);
    int heightValue = 0;
    for (int row = rowIndex; row < rowIndex + spanValue; ++row)
        heightValue += rowHeightAt(row);
    return heightValue;
}

int TableModel::totalBodyHeight() const
{
    int heightValue = 0;
    for (int row = 0; row < rowCount(); ++row)
        heightValue += rowHeightAt(row);
    return heightValue;
}

int TableModel::rowCellWidth(const QVariant &rowEntry) const
{
    return m_cellWidth > 0 ? m_cellWidth : autoCellWidth(rowEntry);
}

double TableModel::rowCellSpacing(const QVariant &rowEntry) const
{
    const int count = columnCountForRow(rowEntry);
    if (count <= 1)
        return 0.0;
    const int widthValue = rowCellWidth(rowEntry);
    return qMax(0.0, (m_tableWidth - (count * widthValue)) / (count - 1));
}

int TableModel::structureCellWidth(int columnIndex) const
{
    return columnWidth(qMax(0, columnIndex));
}

int TableModel::structureCellSpacing() const
{
    return 0;
}

int TableModel::structureColumnX(int columnIndex) const
{
    return columnX(columnIndex);
}

int TableModel::cellX(const QVariant &, int columnIndex) const
{
    return columnX(columnIndex);
}

int TableModel::cellSpanWidth(const QVariant &, int columnSpan) const
{
    return columnSpanWidth(0, columnSpan);
}

QVariantList TableModel::normalizedColumnWidths() const
{
    QVariantList result = m_columnWidths;
    const int count = qMax(1, columnCount());
    while (result.size() < count)
        result.append(columnWidth(result.size()));
    for (int column = 0; column < count; ++column)
        result[column] = numericSize(result.at(column), columnWidth(column), m_minColumnWidth);
    return result;
}

QVariantList TableModel::normalizedRowHeights() const
{
    QVariantList result = m_rowHeights;
    while (result.size() < rowCount())
        result.append(rowHeightAt(result.size()));
    for (int row = 0; row < rowCount(); ++row)
        result[row] = numericSize(result.at(row), rowHeightAt(row), m_minRowHeight);
    return result;
}

bool TableModel::setColumnWidth(int columnIndex, const QVariant &widthValue)
{
    if (columnIndex < 0 || columnIndex >= columnCount())
        return false;
    const int nextWidth = numericSize(widthValue, columnWidth(columnIndex), m_minColumnWidth);
    return applyColumnWidth(columnIndex, nextWidth, true);
}

bool TableModel::setRowHeight(int rowIndex, const QVariant &heightValue)
{
    if (rowIndex < 0 || rowIndex >= rowCount())
        return false;
    const int nextHeight = numericSize(heightValue, rowHeightAt(rowIndex), m_minRowHeight);
    return applyRowHeight(rowIndex, nextHeight, true);
}

bool TableModel::beginColumnResize(int columnIndex, double pointerX)
{
    if (columnIndex < 0 || columnIndex >= columnCount())
        return false;
    m_resizingColumnIndex = columnIndex;
    m_resizeStartPointerX = pointerX;
    m_resizeStartColumnWidth = columnWidth(columnIndex);
    m_resizeHistoryRecorded = false;
    emit resizeStateChanged();
    return true;
}

bool TableModel::updateColumnResize(double pointerX)
{
    if (m_resizingColumnIndex < 0)
        return false;
    const int nextWidth = numericSize(m_resizeStartColumnWidth + (pointerX - m_resizeStartPointerX),
                                      m_resizeStartColumnWidth,
                                      m_minColumnWidth);
    if (!m_resizeHistoryRecorded) {
        recordUndoSnapshot();
        m_resizeHistoryRecorded = true;
    }
    return applyColumnWidth(m_resizingColumnIndex, nextWidth, false);
}

void TableModel::endColumnResize()
{
    if (m_resizingColumnIndex < 0)
        return;
    m_resizingColumnIndex = -1;
    m_resizeStartPointerX = 0.0;
    m_resizeStartColumnWidth = 0;
    m_resizeHistoryRecorded = false;
    emit resizeStateChanged();
}

bool TableModel::beginRowResize(int rowIndex, double pointerY)
{
    if (rowIndex < 0 || rowIndex >= rowCount())
        return false;
    m_resizingRowIndex = rowIndex;
    m_resizeStartPointerY = pointerY;
    m_resizeStartRowHeight = rowHeightAt(rowIndex);
    m_resizeHistoryRecorded = false;
    emit resizeStateChanged();
    return true;
}

bool TableModel::updateRowResize(double pointerY)
{
    if (m_resizingRowIndex < 0)
        return false;
    const int nextHeight = numericSize(m_resizeStartRowHeight + (pointerY - m_resizeStartPointerY),
                                       m_resizeStartRowHeight,
                                       m_minRowHeight);
    if (!m_resizeHistoryRecorded) {
        recordUndoSnapshot();
        m_resizeHistoryRecorded = true;
    }
    return applyRowHeight(m_resizingRowIndex, nextHeight, false);
}

void TableModel::endRowResize()
{
    if (m_resizingRowIndex < 0)
        return;
    m_resizingRowIndex = -1;
    m_resizeStartPointerY = 0.0;
    m_resizeStartRowHeight = 0;
    m_resizeHistoryRecorded = false;
    emit resizeStateChanged();
}

bool TableModel::setContextCell(int rowIndex, int columnIndex)
{
    if (!canMutateStructureInternal() || !isValidBodyCell(rowIndex, columnIndex))
        return false;
    if (m_contextRowIndex == rowIndex && m_contextColumnIndex == columnIndex)
        return true;
    m_contextRowIndex = rowIndex;
    m_contextColumnIndex = columnIndex;
    emit contextCellChanged();
    return true;
}

void TableModel::clearContextCell()
{
    if (m_contextRowIndex == -1 && m_contextColumnIndex == -1)
        return;
    m_contextRowIndex = -1;
    m_contextColumnIndex = -1;
    emit contextCellChanged();
}

QVariantList TableModel::contextMenuDescriptors(int rowIndex, int columnIndex) const
{
    if (!canMutateStructureInternal() || !isValidBodyCell(rowIndex, columnIndex))
        return {};

    return QVariantList {
        QVariantMap {
            {QStringLiteral("label"), QStringLiteral("Delete row")},
            {QStringLiteral("eventName"), QStringLiteral("table.deleteRow")},
            {QStringLiteral("action"), QStringLiteral("deleteRow")},
            {QStringLiteral("rowIndex"), rowIndex},
            {QStringLiteral("columnIndex"), columnIndex},
            {QStringLiteral("enabled"), canDeleteRow(rowIndex)}
        },
        QVariantMap {{QStringLiteral("type"), QStringLiteral("divider")}},
        QVariantMap {
            {QStringLiteral("label"), QStringLiteral("Delete column")},
            {QStringLiteral("eventName"), QStringLiteral("table.deleteColumn")},
            {QStringLiteral("action"), QStringLiteral("deleteColumn")},
            {QStringLiteral("rowIndex"), rowIndex},
            {QStringLiteral("columnIndex"), columnIndex},
            {QStringLiteral("enabled"), canDeleteColumn(columnIndex)}
        }
    };
}

QVariantMap TableModel::triggerContextAction(const QString &action, int rowIndex, int columnIndex)
{
    const QString normalizedAction = action.trimmed();
    QVariantMap result {
        {QStringLiteral("accepted"), false},
        {QStringLiteral("action"), normalizedAction},
        {QStringLiteral("rowIndex"), rowIndex},
        {QStringLiteral("columnIndex"), columnIndex}
    };

    if (normalizedAction == QStringLiteral("deleteRow")) {
        if (!deleteRow(rowIndex))
            return result;
        clearContextCell();
        result.insert(QStringLiteral("accepted"), true);
        return result;
    }

    if (normalizedAction == QStringLiteral("deleteColumn")) {
        if (!deleteColumn(columnIndex))
            return result;
        clearContextCell();
        result.insert(QStringLiteral("accepted"), true);
        return result;
    }

    return result;
}

bool TableModel::canInsertRow(int rowIndex) const
{
    return canMutateStructureInternal() && rowIndex >= 0 && rowIndex <= rowCount();
}

bool TableModel::insertRow(int rowIndex)
{
    if (!canInsertRow(rowIndex))
        return false;
    recordUndoSnapshot();
    QVariantList rowHeights = m_rowHeights.isEmpty() ? QVariantList() : normalizedRowHeights();
    normalizeStructureForMutation();
    const int count = qMax(1, columnCount());
    QVariantList row;
    for (int column = 0; column < count; ++column)
        row.append(createDefaultCell(rowIndex, column));
    m_rows.insert(rowIndex, row);
    if (!rowHeights.isEmpty()) {
        rowHeights.insert(rowIndex, defaultRowHeight());
        applyRowHeights(rowHeights);
    }
    emitRowsMutated();
    return true;
}

bool TableModel::appendRow()
{
    return insertRow(rowCount());
}

bool TableModel::canDeleteRow(int rowIndex) const
{
    return canMutateStructureInternal() && rowIndex >= 0 && rowIndex < rowCount();
}

bool TableModel::deleteRow(int rowIndex)
{
    if (!canDeleteRow(rowIndex))
        return false;
    recordUndoSnapshot();
    QVariantList rowHeights = m_rowHeights.isEmpty() ? QVariantList() : normalizedRowHeights();
    normalizeStructureForMutation();
    m_rows.removeAt(rowIndex);
    if (!rowHeights.isEmpty() && rowIndex < rowHeights.size()) {
        rowHeights.removeAt(rowIndex);
        applyRowHeights(rowHeights);
    }
    emitRowsMutated();
    return true;
}

bool TableModel::removeRow(int rowIndex)
{
    return deleteRow(rowIndex);
}

bool TableModel::canInsertColumn(int columnIndex) const
{
    return canMutateStructureInternal() && columnIndex >= 0 && columnIndex <= columnCount();
}

bool TableModel::insertColumn(int columnIndex)
{
    if (!canInsertColumn(columnIndex))
        return false;
    recordUndoSnapshot();
    QVariantList columnWidths = m_columnWidths.isEmpty() ? QVariantList() : normalizedColumnWidths();
    normalizeStructureForMutation();
    const int currentColumnCount = qMax(1, columnCount());
    for (int row = 0; row < m_rows.size(); ++row) {
        QVariantList rowEntry = listFromVariant(m_rows.at(row));
        while (rowEntry.size() < currentColumnCount)
            rowEntry.append(createDefaultCell(row, rowEntry.size()));
        rowEntry.insert(columnIndex, createDefaultCell(row, columnIndex));
        m_rows[row] = rowEntry;
    }

    const bool headerCellsMutable = isMutableListVariant(m_headerCellItems);
    const bool headerColumnsMutable = !headerCellsMutable && isMutableListVariant(m_headerColumns);
    QVariantList header = headerSourceList();
    if (headerCellsMutable || headerColumnsMutable) {
        while (header.size() < currentColumnCount)
            header.append(createDefaultHeaderCell(header.size()));
        header.insert(columnIndex, createDefaultHeaderCell(columnIndex));
        if (headerCellsMutable)
            m_headerCellItems = header;
        else
            m_headerColumns = header;
        emitHeadersMutated(headerCellsMutable, headerColumnsMutable);
    }

    if (!columnWidths.isEmpty()) {
        columnWidths.insert(columnIndex, defaultColumnWidth());
        applyColumnWidths(columnWidths);
    }
    emitRowsMutated();
    return true;
}

bool TableModel::appendColumn()
{
    return insertColumn(columnCount());
}

bool TableModel::canDeleteColumn(int columnIndex) const
{
    return canMutateStructureInternal() && columnCount() > 1 && columnIndex >= 0 && columnIndex < columnCount();
}

bool TableModel::deleteColumn(int columnIndex)
{
    if (!canDeleteColumn(columnIndex))
        return false;
    recordUndoSnapshot();
    QVariantList columnWidths = m_columnWidths.isEmpty() ? QVariantList() : normalizedColumnWidths();
    normalizeStructureForMutation();
    for (int row = 0; row < m_rows.size(); ++row) {
        QVariantList rowEntry = listFromVariant(m_rows.at(row));
        if (columnIndex < rowEntry.size())
            rowEntry.removeAt(columnIndex);
        m_rows[row] = rowEntry;
    }

    const bool headerCellsMutable = isMutableListVariant(m_headerCellItems);
    const bool headerColumnsMutable = !headerCellsMutable && isMutableListVariant(m_headerColumns);
    QVariantList header = headerSourceList();
    if ((headerCellsMutable || headerColumnsMutable) && columnIndex < header.size()) {
        header.removeAt(columnIndex);
        if (headerCellsMutable)
            m_headerCellItems = header;
        else
            m_headerColumns = header;
        emitHeadersMutated(headerCellsMutable, headerColumnsMutable);
    }

    if (!columnWidths.isEmpty() && columnIndex < columnWidths.size()) {
        columnWidths.removeAt(columnIndex);
        applyColumnWidths(columnWidths);
    }
    emitRowsMutated();
    return true;
}

bool TableModel::removeColumn(int columnIndex)
{
    return deleteColumn(columnIndex);
}

bool TableModel::canMergeCells(int rowIndex, int columnIndex, int rowSpan, int columnSpan) const
{
    if (!canMutateStructureInternal() || rowIndex < 0 || columnIndex < 0 || rowSpan < 1 || columnSpan < 1)
        return false;
    if (rowIndex + rowSpan > rowCount())
        return false;
    for (int row = rowIndex; row < rowIndex + rowSpan; ++row) {
        const QVariantList rowEntry = listFromVariant(m_rows.at(row));
        if (columnIndex + columnSpan > rowEntry.size())
            return false;
    }
    return true;
}

bool TableModel::mergeCells(int rowIndex, int columnIndex, int rowSpan, int columnSpan)
{
    if (!canMergeCells(rowIndex, columnIndex, rowSpan, columnSpan))
        return false;

    recordUndoSnapshot();
    splitCellsIntersecting(rowIndex, columnIndex, rowSpan, columnSpan);

    QVariantMap anchorCell = normalizeCellObject(rowIndex, columnIndex);
    anchorCell.insert(QStringLiteral("rowSpan"), rowSpan);
    anchorCell.insert(QStringLiteral("columnSpan"), columnSpan);
    anchorCell.remove(QStringLiteral("_lvrsMerged"));
    anchorCell.remove(QStringLiteral("_lvrsMergeAnchorRow"));
    anchorCell.remove(QStringLiteral("_lvrsMergeAnchorColumn"));
    setCellObject(rowIndex, columnIndex, anchorCell);

    for (int row = rowIndex; row < rowIndex + rowSpan; ++row) {
        for (int column = columnIndex; column < columnIndex + columnSpan; ++column) {
            if (row == rowIndex && column == columnIndex)
                continue;
            QVariantMap cell = normalizeCellObject(row, column);
            cell.insert(QStringLiteral("_lvrsMerged"), true);
            cell.insert(QStringLiteral("_lvrsMergeAnchorRow"), rowIndex);
            cell.insert(QStringLiteral("_lvrsMergeAnchorColumn"), columnIndex);
            if (cell.contains(QStringLiteral("rowSpan")))
                cell.insert(QStringLiteral("rowSpan"), 1);
            if (cell.contains(QStringLiteral("columnSpan")))
                cell.insert(QStringLiteral("columnSpan"), 1);
            setCellObject(row, column, cell);
        }
    }

    emitRowsMutated();
    return true;
}

bool TableModel::splitCell(int rowIndex, int columnIndex)
{
    if (!canMutateStructureInternal() || rowIndex < 0 || columnIndex < 0 || rowIndex >= rowCount())
        return false;
    if (columnIndex >= columnCountForRow(rowAt(rowIndex)))
        return false;
    const QVariantMap anchor = mergeAnchorForCell(rowIndex, columnIndex);
    recordUndoSnapshot();
    return splitAnchorCell(anchor.value(QStringLiteral("rowIndex")).toInt(),
                           anchor.value(QStringLiteral("columnIndex")).toInt(),
                           true);
}

bool TableModel::rowInputable(const QVariant &rowEntry) const
{
    const QVariantMap map = mapFromVariant(rowEntry);
    if (map.contains(QStringLiteral("inputable")))
        return map.value(QStringLiteral("inputable")).toBool();
    return m_inputable;
}

bool TableModel::cellInputable(int rowIndex, int columnIndex) const
{
    const QVariantMap map = mapFromVariant(cellAt(rowIndex, columnIndex));
    if (map.contains(QStringLiteral("inputable")))
        return map.value(QStringLiteral("inputable")).toBool();
    return rowInputable(rowAt(rowIndex));
}

bool TableModel::undo()
{
    if (!m_undoStack.canUndo())
        return false;

    const QVariant target = m_undoStack.takeUndoSnapshot(snapshot());
    restoreSnapshot(target);
    return true;
}

bool TableModel::redo()
{
    if (!m_undoStack.canRedo())
        return false;

    const QVariant target = m_undoStack.takeRedoSnapshot(snapshot());
    restoreSnapshot(target);
    return true;
}

void TableModel::clearUndoStack()
{
    clearHistory();
}

QVariantList TableModel::listFromVariant(const QVariant &value)
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
    if (QAbstractItemModel *model = itemModelFromVariant(value))
        return rowsFromItemModel(model);
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
            if (QMetaObject::invokeMethod(object,
                                          "get",
                                          Q_RETURN_ARG(QVariant, returned),
                                          Q_ARG(int, index))) {
                result.append(returned);
                continue;
            }
            if (QMetaObject::invokeMethod(object,
                                          "at",
                                          Q_RETURN_ARG(QVariant, returned),
                                          Q_ARG(int, index))) {
                result.append(returned);
            }
        }
        return result;
    }
    return {};
}

QVariantMap TableModel::mapFromVariant(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return {};
    if (value.userType() == qMetaTypeId<QJSValue>())
        return value.value<QJSValue>().toVariant().toMap();
    if (value.canConvert<QVariantMap>())
        return value.toMap();
    return {};
}

QAbstractItemModel *TableModel::itemModelFromVariant(const QVariant &value)
{
    return qobject_cast<QAbstractItemModel *>(objectFromVariant(value));
}

QVariantList TableModel::rowsFromItemModel(QAbstractItemModel *model)
{
    if (!model)
        return {};

    const QModelIndex parent;
    const int rows = qMax(0, model->rowCount(parent));
    const int columns = qMax(0, model->columnCount(parent));
    QVariantList result;
    result.reserve(rows);
    for (int row = 0; row < rows; ++row) {
        QVariantList rowItems;
        rowItems.reserve(columns);
        for (int column = 0; column < columns; ++column)
            rowItems.append(cellMapFromModelIndex(model, model->index(row, column, parent)));
        result.append(QVariant(rowItems));
    }
    return result;
}

bool TableModel::isListLike(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return false;
    if (value.userType() == qMetaTypeId<QJSValue>())
        return value.value<QJSValue>().isArray() || jsLength(value.value<QJSValue>()) > 0;
    if (value.canConvert<QVariantList>())
        return true;
    if (value.canConvert<QStringList>())
        return true;
    if (QObject *object = objectFromVariant(value))
        return object->property("count").isValid() || object->property("length").isValid();
    return false;
}

bool TableModel::isMutableListVariant(const QVariant &value)
{
    if (!value.isValid() || value.isNull())
        return false;
    if (value.userType() == qMetaTypeId<QJSValue>())
        return value.value<QJSValue>().isArray();
    if (itemModelFromVariant(value))
        return false;
    return value.canConvert<QVariantList>() || value.canConvert<QStringList>();
}

QVariant TableModel::entryAt(const QVariant &value, int index)
{
    const QVariantList list = listFromVariant(value);
    return index >= 0 && index < list.size() ? list.at(index) : QVariant();
}

QVariant TableModel::roleValue(const QVariant &entry, const QStringList &keys, const QVariant &fallback)
{
    const QVariantMap map = mapFromVariant(entry);
    for (const QString &key : keys) {
        if (map.contains(key))
            return map.value(key);
    }
    return fallback;
}

int TableModel::integerObjectValue(const QVariant &entry,
                                   const QString &primaryKey,
                                   const QString &alternateKey,
                                   int fallbackValue)
{
    const QVariant value = roleValue(entry, {primaryKey, alternateKey});
    bool ok = false;
    const int parsed = value.toInt(&ok);
    return !ok || parsed < 1 ? fallbackValue : parsed;
}

bool TableModel::isMergeCoveredMarker(const QVariant &entry)
{
    return mapFromVariant(entry).value(QStringLiteral("_lvrsMerged")).toBool();
}

void TableModel::clearMergeMetadata(QVariantMap *entry)
{
    if (!entry)
        return;
    entry->remove(QStringLiteral("_lvrsMerged"));
    entry->remove(QStringLiteral("_lvrsMergeAnchorRow"));
    entry->remove(QStringLiteral("_lvrsMergeAnchorColumn"));
    if (entry->contains(QStringLiteral("rowSpan")))
        entry->insert(QStringLiteral("rowSpan"), 1);
    if (entry->contains(QStringLiteral("rowspan")))
        entry->insert(QStringLiteral("rowspan"), 1);
    if (entry->contains(QStringLiteral("columnSpan")))
        entry->insert(QStringLiteral("columnSpan"), 1);
    if (entry->contains(QStringLiteral("colSpan")))
        entry->insert(QStringLiteral("colSpan"), 1);
}

bool TableModel::spanContains(int anchorRow,
                              int anchorColumn,
                              int rowSpan,
                              int columnSpan,
                              int rowIndex,
                              int columnIndex)
{
    return rowIndex >= anchorRow
        && rowIndex < anchorRow + rowSpan
        && columnIndex >= anchorColumn
        && columnIndex < anchorColumn + columnSpan;
}

QVariant TableModel::headerSourceVariant() const
{
    if (m_headerCellItems.isValid() && !m_headerCellItems.isNull())
        return m_headerCellItems;
    if (QAbstractItemModel *model = itemModelFromVariant(m_rowsSource)) {
        if (usingDefaultHeaderColumns())
            return headersFromItemModel(model);
    }
    return m_headerColumns;
}

QVariantList TableModel::headerSourceList() const
{
    return listFromVariant(headerSourceVariant());
}

QVariantList TableModel::headersFromItemModel(QAbstractItemModel *model) const
{
    if (!model)
        return {};

    const QModelIndex parent;
    const int columns = qMax(0, model->columnCount(parent));
    QVariantList result;
    result.reserve(columns);
    for (int column = 0; column < columns; ++column) {
        const QVariant displayValue = model->headerData(column, Qt::Horizontal, Qt::DisplayRole);
        const QVariant labelValue = displayValue.isValid() && !displayValue.isNull()
            ? displayValue
            : QVariant(m_defaultHeaderText);
        result.append(QVariantMap {
            {QStringLiteral("label"), labelValue.toString()}
        });
    }
    return result;
}

bool TableModel::usingDefaultHeaderColumns() const
{
    if (m_headerCellItems.isValid() && !m_headerCellItems.isNull())
        return false;

    const QVariantList headers = listFromVariant(m_headerColumns);
    if (headers.size() != 3)
        return false;

    for (const QVariant &entry : headers) {
        const QString text = entry.toString();
        if (text != QStringLiteral("Column") && text != m_defaultHeaderText)
            return false;
    }
    return true;
}

QVariantMap TableModel::snapshot() const
{
    return QVariantMap {
        {QStringLiteral("rows"), m_rows},
        {QStringLiteral("rowsSource"), m_rowsSource},
        {QStringLiteral("headerCellItems"), m_headerCellItems},
        {QStringLiteral("headerColumns"), m_headerColumns},
        {QStringLiteral("columnWidths"), m_columnWidths},
        {QStringLiteral("rowHeights"), m_rowHeights},
        {QStringLiteral("rowsStructureMutable"), m_rowsStructureMutable}
    };
}

void TableModel::restoreSnapshot(const QVariant &snapshot)
{
    const QVariantMap map = snapshot.toMap();
    if (map.isEmpty())
        return;

    m_rows = map.value(QStringLiteral("rows")).toList();
    m_rowsSource = map.value(QStringLiteral("rowsSource"), m_rows);
    m_headerCellItems = map.value(QStringLiteral("headerCellItems"));
    m_headerColumns = map.value(QStringLiteral("headerColumns"));
    m_columnWidths = map.value(QStringLiteral("columnWidths")).toList();
    m_rowHeights = map.value(QStringLiteral("rowHeights")).toList();
    m_rowsStructureMutable = map.value(QStringLiteral("rowsStructureMutable"), true).toBool();
    reconnectRowsSourceSignals();
    if (rowsModelBacked()) {
        m_rows = listFromVariant(m_rowsSource);
        m_rowsStructureMutable = isMutableListVariant(m_rowsSource);
    }

    m_resizingColumnIndex = -1;
    m_resizingRowIndex = -1;
    m_resizeHistoryRecorded = false;
    bumpRevision();
    emit rowsChanged();
    emit headerCellItemsChanged();
    emit headerColumnsChanged();
    emit headersChanged();
    emit columnWidthsChanged();
    emit rowHeightsChanged();
    emit resizeStateChanged();
    emit geometryChanged();
    emit modelChanged();
}

void TableModel::clearHistory()
{
    m_undoStack.clear();
}

void TableModel::recordUndoSnapshot()
{
    m_undoStack.pushSnapshot(snapshot());
}

void TableModel::bumpRevision()
{
    ++m_revision;
    emit revisionChanged();
}

void TableModel::reconnectRowsSourceSignals()
{
    for (const QMetaObject::Connection &connection : std::as_const(m_rowsSourceConnections))
        QObject::disconnect(connection);
    m_rowsSourceConnections.clear();
    m_rowsSourceObject = objectFromVariant(m_rowsSource);

    QAbstractItemModel *model = itemModelFromVariant(m_rowsSource);
    if (!model)
        return;

    auto refreshRows = [this]() {
        refreshRowsFromSource(true);
    };
    m_rowsSourceConnections.append(connect(model,
                                           &QAbstractItemModel::dataChanged,
                                           this,
                                           [this](const QModelIndex &, const QModelIndex &, const QList<int> &) {
                                               refreshRowsFromSource(true);
                                           }));
    m_rowsSourceConnections.append(connect(model, &QAbstractItemModel::rowsInserted, this, refreshRows));
    m_rowsSourceConnections.append(connect(model, &QAbstractItemModel::rowsRemoved, this, refreshRows));
    m_rowsSourceConnections.append(connect(model, &QAbstractItemModel::rowsMoved, this, refreshRows));
    m_rowsSourceConnections.append(connect(model, &QAbstractItemModel::columnsInserted, this, refreshRows));
    m_rowsSourceConnections.append(connect(model, &QAbstractItemModel::columnsRemoved, this, refreshRows));
    m_rowsSourceConnections.append(connect(model, &QAbstractItemModel::columnsMoved, this, refreshRows));
    m_rowsSourceConnections.append(connect(model, &QAbstractItemModel::layoutChanged, this, refreshRows));
    m_rowsSourceConnections.append(connect(model, &QAbstractItemModel::modelReset, this, refreshRows));
    m_rowsSourceConnections.append(connect(model,
                                           &QAbstractItemModel::headerDataChanged,
                                           this,
                                           [this](Qt::Orientation, int, int) {
                                               refreshRowsFromSource(true);
                                           }));
    m_rowsSourceConnections.append(connect(model, &QObject::destroyed, this, [this]() {
        m_rowsSourceConnections.clear();
        m_rowsSourceObject.clear();
        m_rowsSource = QVariant();
        m_rows.clear();
        m_rowsStructureMutable = false;
        clearHistory();
        bumpRevision();
        emit rowsChanged();
        emit headersChanged();
        emit geometryChanged();
        emit modelChanged();
    }));
}

void TableModel::refreshRowsFromSource(bool emitSignals)
{
    if (m_refreshingRowsSource)
        return;

    m_refreshingRowsSource = true;
    m_rows = listFromVariant(m_rowsSource);
    m_rowsStructureMutable = isMutableListVariant(m_rowsSource);
    m_refreshingRowsSource = false;

    if (!emitSignals)
        return;

    clearHistory();
    bumpRevision();
    emit rowsChanged();
    emit headersChanged();
    emit geometryChanged();
    emit modelChanged();
}

bool TableModel::canMutateStructureInternal() const
{
    if (!m_rowsStructureMutable)
        return false;
    if (m_rows.isEmpty())
        return true;
    for (const QVariant &row : m_rows) {
        if (!isListLike(row))
            return false;
    }
    return true;
}

bool TableModel::setItemModelCellValue(QAbstractItemModel *model,
                                       int rowIndex,
                                       int columnIndex,
                                       const QVariantMap &coercedValue)
{
    if (!model || rowIndex < 0 || columnIndex < 0)
        return false;

    const QModelIndex parent;
    if (rowIndex >= model->rowCount(parent) || columnIndex >= model->columnCount(parent))
        return false;

    const QModelIndex index = model->index(rowIndex, columnIndex, parent);
    if (!index.isValid())
        return false;

    const QVariant value = coercedValue.value(QStringLiteral("value"));
    const int valueRole = roleIdForName(model, QByteArrayLiteral("value"));
    const int editRole = roleIdForName(model, QByteArrayLiteral("edit"));
    const int textRole = roleIdForName(model, QByteArrayLiteral("text"));
    QList<int> roles {Qt::EditRole};
    const QList<int> fallbackRoles {
        valueRole,
        editRole,
        textRole,
        static_cast<int>(Qt::DisplayRole)
    };
    for (int role : fallbackRoles) {
        if (role >= 0 && !roles.contains(role))
            roles.append(role);
    }

    bool accepted = false;
    m_refreshingRowsSource = true;
    for (int role : roles) {
        if (model->setData(index, value, role)) {
            accepted = true;
            break;
        }
    }
    m_refreshingRowsSource = false;

    if (!accepted)
        return false;

    refreshRowsFromSource(false);
    emitRowsMutated();
    return true;
}

bool TableModel::isValidBodyCell(int rowIndex, int columnIndex) const
{
    return rowIndex >= 0
        && rowIndex < rowCount()
        && columnIndex >= 0
        && columnIndex < columnCountForRow(rowAt(rowIndex));
}

QVariantMap TableModel::normalizeCellObject(int rowIndex, int columnIndex)
{
    if (!canMutateStructureInternal() || rowIndex < 0 || rowIndex >= m_rows.size())
        return {};
    QVariantList row = listFromVariant(m_rows.at(rowIndex));
    if (columnIndex < 0 || columnIndex >= row.size())
        return {};
    QVariantMap cell = mapFromVariant(row.at(columnIndex));
    if (!cell.isEmpty())
        return cell;
    const QVariant current = row.at(columnIndex);
    cell.insert(QStringLiteral("text"), (current.isValid() && !current.isNull()) ? current.toString() : QString());
    row[columnIndex] = cell;
    m_rows[rowIndex] = row;
    return cell;
}

bool TableModel::setCellObject(int rowIndex, int columnIndex, const QVariantMap &cell)
{
    if (rowIndex < 0 || rowIndex >= m_rows.size())
        return false;
    QVariantList row = listFromVariant(m_rows.at(rowIndex));
    if (columnIndex < 0 || columnIndex >= row.size())
        return false;
    row[columnIndex] = cell;
    m_rows[rowIndex] = row;
    return true;
}

bool TableModel::setRowList(int rowIndex, const QVariantList &row)
{
    if (rowIndex < 0 || rowIndex >= m_rows.size())
        return false;
    m_rows[rowIndex] = row;
    return true;
}

bool TableModel::normalizeStructureForMutation()
{
    if (!canMutateStructureInternal())
        return false;
    for (int row = 0; row < m_rows.size(); ++row) {
        QVariantList rowEntry = listFromVariant(m_rows.at(row));
        for (int column = 0; column < rowEntry.size(); ++column) {
            QVariantMap cell = mapFromVariant(rowEntry.at(column));
            if (cell.isEmpty())
                continue;
            clearMergeMetadata(&cell);
            rowEntry[column] = cell;
        }
        setRowList(row, rowEntry);
    }
    return true;
}

void TableModel::splitCellsIntersecting(int rowIndex, int columnIndex, int rowSpan, int columnSpan)
{
    for (int row = 0; row < rowCount(); ++row) {
        const int count = columnCountForRow(rowAt(row));
        for (int column = 0; column < count; ++column) {
            if (isMergeCoveredMarker(cellAt(row, column)))
                continue;
            const int candidateRowSpan = cellRowSpan(row, column);
            const int candidateColumnSpan = cellColumnSpan(row, column);
            const bool intersects = row < rowIndex + rowSpan
                && row + candidateRowSpan > rowIndex
                && column < columnIndex + columnSpan
                && column + candidateColumnSpan > columnIndex;
            if (intersects && (candidateRowSpan > 1 || candidateColumnSpan > 1))
                splitAnchorCell(row, column, false);
        }
    }
}

bool TableModel::splitAnchorCell(int anchorRow, int anchorColumn, bool emitChange)
{
    QVariantMap anchorCell = normalizeCellObject(anchorRow, anchorColumn);
    if (anchorCell.isEmpty())
        return false;
    const int rowSpan = cellRowSpan(anchorRow, anchorColumn);
    const int columnSpan = cellColumnSpan(anchorRow, anchorColumn);
    for (int row = anchorRow; row < anchorRow + rowSpan; ++row) {
        for (int column = anchorColumn; column < anchorColumn + columnSpan; ++column) {
            QVariantMap cell = normalizeCellObject(row, column);
            clearMergeMetadata(&cell);
            setCellObject(row, column, cell);
        }
    }
    clearMergeMetadata(&anchorCell);
    setCellObject(anchorRow, anchorColumn, anchorCell);
    if (emitChange)
        emitRowsMutated();
    return true;
}

QVariantMap TableModel::createDefaultCell(int, int columnIndex) const
{
    const QString type = headerCellType(columnIndex);
    const QVariantMap result = coerceCellValue(typedDefaultValue(type), type);
    return QVariantMap {
        {QStringLiteral("value"), result.value(QStringLiteral("value"))},
        {QStringLiteral("text"), result.value(QStringLiteral("text"))}
    };
}

QVariantMap TableModel::createDefaultHeaderCell(int) const
{
    return QVariantMap {{QStringLiteral("label"), m_defaultHeaderText}};
}

int TableModel::defaultColumnWidth() const
{
    if (m_cellWidth > 0)
        return numericSize(m_cellWidth, autoColumnWidth(), m_minColumnWidth);
    return autoColumnWidth();
}

int TableModel::defaultRowHeight() const
{
    return numericSize(m_rowHeight, m_rowHeight, m_minRowHeight);
}

bool TableModel::applyColumnWidths(const QVariantList &widths)
{
    if (m_columnWidths == widths)
        return false;
    m_columnWidths = widths;
    bumpRevision();
    emit columnWidthsChanged();
    emit geometryChanged();
    return true;
}

bool TableModel::applyRowHeights(const QVariantList &heights)
{
    if (m_rowHeights == heights)
        return false;
    m_rowHeights = heights;
    bumpRevision();
    emit rowHeightsChanged();
    emit geometryChanged();
    return true;
}

bool TableModel::applyColumnWidth(int columnIndex, int widthValue, bool recordHistory)
{
    if (columnIndex < 0 || columnIndex >= columnCount())
        return false;
    QVariantList widths = normalizedColumnWidths();
    const int nextWidth = numericSize(widthValue, columnWidth(columnIndex), m_minColumnWidth);
    if (widths.at(columnIndex).toInt() == nextWidth)
        return true;
    if (recordHistory)
        recordUndoSnapshot();
    widths[columnIndex] = nextWidth;
    return applyColumnWidths(widths);
}

bool TableModel::applyRowHeight(int rowIndex, int heightValue, bool recordHistory)
{
    if (rowIndex < 0 || rowIndex >= rowCount())
        return false;
    QVariantList heights = normalizedRowHeights();
    const int nextHeight = numericSize(heightValue, rowHeightAt(rowIndex), m_minRowHeight);
    if (heights.at(rowIndex).toInt() == nextHeight)
        return true;
    if (recordHistory)
        recordUndoSnapshot();
    heights[rowIndex] = nextHeight;
    return applyRowHeights(heights);
}

void TableModel::emitRowsMutated()
{
    if (!rowsModelBacked())
        m_rowsSource = m_rows;
    bumpRevision();
    emit rowsChanged();
    emit geometryChanged();
    emit modelChanged();
}

void TableModel::emitHeadersMutated(bool cellItemsChanged, bool columnsChanged)
{
    bumpRevision();
    if (cellItemsChanged)
        emit headerCellItemsChanged();
    if (columnsChanged)
        emit headerColumnsChanged();
    emit headersChanged();
    emit geometryChanged();
    emit modelChanged();
}
