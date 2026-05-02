#pragma once

#include "backend/model/modelundostack.h"

#include <QObject>
#include <QMetaObject>
#include <QPointer>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

class QAbstractItemModel;

class TableModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TableModel)

    Q_PROPERTY(QVariant rows READ rows WRITE setRows NOTIFY rowsChanged)
    Q_PROPERTY(QVariant headerCellItems READ headerCellItems WRITE setHeaderCellItems NOTIFY headerCellItemsChanged)
    Q_PROPERTY(QVariant headerColumns READ headerColumns WRITE setHeaderColumns NOTIFY headerColumnsChanged)
    Q_PROPERTY(QString defaultHeaderText READ defaultHeaderText WRITE setDefaultHeaderText NOTIFY defaultHeaderTextChanged)
    Q_PROPERTY(QString defaultCellText READ defaultCellText WRITE setDefaultCellText NOTIFY defaultCellTextChanged)
    Q_PROPERTY(bool inputable READ inputable WRITE setInputable NOTIFY inputableChanged)
    Q_PROPERTY(double tableWidth READ tableWidth WRITE setTableWidth NOTIFY geometryChanged)
    Q_PROPERTY(int rowHeight READ rowHeight WRITE setDefaultRowHeight NOTIFY geometryChanged)
    Q_PROPERTY(int cellWidth READ cellWidth WRITE setCellWidth NOTIFY geometryChanged)
    Q_PROPERTY(QVariant columnWidths READ columnWidths WRITE setColumnWidths NOTIFY columnWidthsChanged)
    Q_PROPERTY(QVariant rowHeights READ rowHeights WRITE setRowHeights NOTIFY rowHeightsChanged)
    Q_PROPERTY(int minColumnWidth READ minColumnWidth WRITE setMinColumnWidth NOTIFY geometryChanged)
    Q_PROPERTY(int minRowHeight READ minRowHeight WRITE setMinRowHeight NOTIFY geometryChanged)
    Q_PROPERTY(int resizingColumnIndex READ resizingColumnIndex NOTIFY resizeStateChanged)
    Q_PROPERTY(int resizingRowIndex READ resizingRowIndex NOTIFY resizeStateChanged)
    Q_PROPERTY(int contextRowIndex READ contextRowIndex NOTIFY contextCellChanged)
    Q_PROPERTY(int contextColumnIndex READ contextColumnIndex NOTIFY contextCellChanged)
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowsChanged)
    Q_PROPERTY(int headerCount READ headerCount NOTIFY headersChanged)
    Q_PROPERTY(int columnCount READ columnCount NOTIFY modelChanged)
    Q_PROPERTY(bool rowsModelBacked READ rowsModelBacked NOTIFY rowsChanged)
    Q_PROPERTY(bool cellEditingAvailable READ cellEditingAvailable NOTIFY rowsChanged)
    Q_PROPERTY(bool structureMutationAvailable READ structureMutationAvailable NOTIFY rowsChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(ModelUndoStack *undoStack READ undoStack CONSTANT)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoStackChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoStackChanged)
    Q_PROPERTY(int undoDepth READ undoDepth NOTIFY undoStackChanged)
    Q_PROPERTY(int redoDepth READ redoDepth NOTIFY undoStackChanged)

public:
    explicit TableModel(QObject *parent = nullptr);

    QVariant rows() const;
    void setRows(const QVariant &rows);

    QVariant headerCellItems() const;
    void setHeaderCellItems(const QVariant &headerCellItems);

    QVariant headerColumns() const;
    void setHeaderColumns(const QVariant &headerColumns);

    QString defaultHeaderText() const;
    void setDefaultHeaderText(const QString &value);

    QString defaultCellText() const;
    void setDefaultCellText(const QString &value);

    bool inputable() const;
    void setInputable(bool value);

    double tableWidth() const;
    void setTableWidth(double value);

    int rowHeight() const;
    void setDefaultRowHeight(int value);

    int cellWidth() const;
    void setCellWidth(int value);

    QVariant columnWidths() const;
    void setColumnWidths(const QVariant &value);

    QVariant rowHeights() const;
    void setRowHeights(const QVariant &value);

    int minColumnWidth() const;
    void setMinColumnWidth(int value);

    int minRowHeight() const;
    void setMinRowHeight(int value);

    int resizingColumnIndex() const;
    int resizingRowIndex() const;
    int contextRowIndex() const;
    int contextColumnIndex() const;

    int rowCount() const;
    int headerCount() const;
    int columnCount() const;
    bool rowsModelBacked() const;
    bool cellEditingAvailable() const;
    bool structureMutationAvailable() const;
    int revision() const;
    ModelUndoStack *undoStack();
    bool canUndo() const;
    bool canRedo() const;
    int undoDepth() const;
    int redoDepth() const;

    Q_INVOKABLE QVariant resolvedHeaderSource() const;
    Q_INVOKABLE QVariant rowAt(int rowIndex) const;
    Q_INVOKABLE QVariant cellAt(int rowIndex, int columnIndex) const;
    Q_INVOKABLE QVariant headerAt(int columnIndex) const;
    Q_INVOKABLE int columnCountForRow(const QVariant &rowEntry) const;
    Q_INVOKABLE QString normalizeHeaderCellType(const QVariant &value) const;
    Q_INVOKABLE QString inferredCellType(const QVariant &value) const;
    Q_INVOKABLE QString headerCellType(int columnIndex) const;
    Q_INVOKABLE QString columnType(int columnIndex) const;
    Q_INVOKABLE QVariant cellRawValue(int rowIndex, int columnIndex) const;
    Q_INVOKABLE QVariant typedDefaultValue(const QString &valueType) const;
    Q_INVOKABLE QVariantMap coerceCellValue(const QVariant &value, const QString &valueType) const;
    Q_INVOKABLE QVariantMap validateCellInput(int rowIndex, int columnIndex, const QVariant &value) const;
    Q_INVOKABLE bool cellValueAccepted(int rowIndex, int columnIndex, const QVariant &value) const;
    Q_INVOKABLE QString cellText(int rowIndex, int columnIndex) const;
    Q_INVOKABLE bool setCellValue(int rowIndex, int columnIndex, const QVariant &value);
    Q_INVOKABLE int cellRowSpan(int rowIndex, int columnIndex) const;
    Q_INVOKABLE int cellColumnSpan(int rowIndex, int columnIndex) const;
    Q_INVOKABLE QVariantMap mergeAnchorForCell(int rowIndex, int columnIndex) const;
    Q_INVOKABLE bool isCoveredCell(int rowIndex, int columnIndex) const;
    Q_INVOKABLE QVariantList visibleCells() const;
    Q_INVOKABLE int numericSize(const QVariant &value, int fallbackValue, int minimumValue) const;
    Q_INVOKABLE int autoCellWidth(const QVariant &rowEntry) const;
    Q_INVOKABLE int autoColumnWidth() const;
    Q_INVOKABLE int columnWidth(int columnIndex) const;
    Q_INVOKABLE int columnX(int columnIndex) const;
    Q_INVOKABLE int columnSpanWidth(int columnIndex, int columnSpan) const;
    Q_INVOKABLE int rowHeightAt(int rowIndex) const;
    Q_INVOKABLE int rowY(int rowIndex) const;
    Q_INVOKABLE int rowSpanHeight(int rowIndex, int rowSpan) const;
    Q_INVOKABLE int totalBodyHeight() const;
    Q_INVOKABLE int rowCellWidth(const QVariant &rowEntry) const;
    Q_INVOKABLE double rowCellSpacing(const QVariant &rowEntry) const;
    Q_INVOKABLE int structureCellWidth(int columnIndex) const;
    Q_INVOKABLE int structureCellSpacing() const;
    Q_INVOKABLE int structureColumnX(int columnIndex) const;
    Q_INVOKABLE int cellX(const QVariant &rowEntry, int columnIndex) const;
    Q_INVOKABLE int cellSpanWidth(const QVariant &rowEntry, int columnSpan) const;
    Q_INVOKABLE QVariantList normalizedColumnWidths() const;
    Q_INVOKABLE QVariantList normalizedRowHeights() const;
    Q_INVOKABLE bool setColumnWidth(int columnIndex, const QVariant &widthValue);
    Q_INVOKABLE bool setRowHeight(int rowIndex, const QVariant &heightValue);
    Q_INVOKABLE bool beginColumnResize(int columnIndex, double pointerX);
    Q_INVOKABLE bool updateColumnResize(double pointerX);
    Q_INVOKABLE void endColumnResize();
    Q_INVOKABLE bool beginRowResize(int rowIndex, double pointerY);
    Q_INVOKABLE bool updateRowResize(double pointerY);
    Q_INVOKABLE void endRowResize();
    Q_INVOKABLE bool setContextCell(int rowIndex, int columnIndex);
    Q_INVOKABLE void clearContextCell();
    Q_INVOKABLE QVariantList contextMenuDescriptors(int rowIndex, int columnIndex) const;
    Q_INVOKABLE QVariantMap triggerContextAction(const QString &action, int rowIndex, int columnIndex);
    Q_INVOKABLE bool canInsertRow(int rowIndex) const;
    Q_INVOKABLE bool insertRow(int rowIndex);
    Q_INVOKABLE bool appendRow();
    Q_INVOKABLE bool canDeleteRow(int rowIndex) const;
    Q_INVOKABLE bool deleteRow(int rowIndex);
    Q_INVOKABLE bool removeRow(int rowIndex);
    Q_INVOKABLE bool canInsertColumn(int columnIndex) const;
    Q_INVOKABLE bool insertColumn(int columnIndex);
    Q_INVOKABLE bool appendColumn();
    Q_INVOKABLE bool canDeleteColumn(int columnIndex) const;
    Q_INVOKABLE bool deleteColumn(int columnIndex);
    Q_INVOKABLE bool removeColumn(int columnIndex);
    Q_INVOKABLE bool canMergeCells(int rowIndex, int columnIndex, int rowSpan, int columnSpan) const;
    Q_INVOKABLE bool mergeCells(int rowIndex, int columnIndex, int rowSpan, int columnSpan);
    Q_INVOKABLE bool splitCell(int rowIndex, int columnIndex);
    Q_INVOKABLE bool rowInputable(const QVariant &rowEntry) const;
    Q_INVOKABLE bool cellInputable(int rowIndex, int columnIndex) const;
    Q_INVOKABLE bool undo();
    Q_INVOKABLE bool redo();
    Q_INVOKABLE void clearUndoStack();

signals:
    void rowsChanged();
    void headerCellItemsChanged();
    void headerColumnsChanged();
    void headersChanged();
    void defaultHeaderTextChanged();
    void defaultCellTextChanged();
    void inputableChanged();
    void geometryChanged();
    void columnWidthsChanged();
    void rowHeightsChanged();
    void resizeStateChanged();
    void contextCellChanged();
    void modelChanged();
    void revisionChanged();
    void undoStackChanged();

private:
    static QVariantList listFromVariant(const QVariant &value);
    static QVariantMap mapFromVariant(const QVariant &value);
    static QAbstractItemModel *itemModelFromVariant(const QVariant &value);
    static QVariantList rowsFromItemModel(QAbstractItemModel *model);
    static bool isListLike(const QVariant &value);
    static bool isMutableListVariant(const QVariant &value);
    static QVariant entryAt(const QVariant &value, int index);
    static QVariant roleValue(const QVariant &entry, const QStringList &keys, const QVariant &fallback = QVariant());
    static int integerObjectValue(const QVariant &entry,
                                  const QString &primaryKey,
                                  const QString &alternateKey,
                                  int fallbackValue);
    static bool isMergeCoveredMarker(const QVariant &entry);
    static void clearMergeMetadata(QVariantMap *entry);
    static bool spanContains(int anchorRow,
                             int anchorColumn,
                             int rowSpan,
                             int columnSpan,
                             int rowIndex,
                             int columnIndex);

    QVariant headerSourceVariant() const;
    QVariantList headerSourceList() const;
    QVariantList headersFromItemModel(QAbstractItemModel *model) const;
    bool usingDefaultHeaderColumns() const;
    QVariantMap snapshot() const;
    void restoreSnapshot(const QVariant &snapshot);
    void clearHistory();
    void recordUndoSnapshot();
    void bumpRevision();
    void reconnectRowsSourceSignals();
    void refreshRowsFromSource(bool emitSignals);
    bool canMutateStructureInternal() const;
    bool setItemModelCellValue(QAbstractItemModel *model, int rowIndex, int columnIndex, const QVariantMap &coercedValue);
    bool isValidBodyCell(int rowIndex, int columnIndex) const;
    QVariantMap normalizeCellObject(int rowIndex, int columnIndex);
    bool setCellObject(int rowIndex, int columnIndex, const QVariantMap &cell);
    bool setRowList(int rowIndex, const QVariantList &row);
    bool normalizeStructureForMutation();
    void splitCellsIntersecting(int rowIndex, int columnIndex, int rowSpan, int columnSpan);
    bool splitAnchorCell(int anchorRow, int anchorColumn, bool emitChange);
    QVariantMap createDefaultCell(int rowIndex, int columnIndex) const;
    QVariantMap createDefaultHeaderCell(int columnIndex) const;
    int defaultColumnWidth() const;
    int defaultRowHeight() const;
    bool applyColumnWidths(const QVariantList &widths);
    bool applyRowHeights(const QVariantList &heights);
    bool applyColumnWidth(int columnIndex, int widthValue, bool recordHistory);
    bool applyRowHeight(int rowIndex, int heightValue, bool recordHistory);
    void emitRowsMutated();
    void emitHeadersMutated(bool cellItemsChanged, bool columnsChanged);

    QVariantList m_rows;
    QVariant m_rowsSource;
    QPointer<QObject> m_rowsSourceObject;
    QList<QMetaObject::Connection> m_rowsSourceConnections;
    QVariant m_headerCellItems;
    QVariant m_headerColumns = QVariant(QStringList {QStringLiteral("Column"), QStringLiteral("Column"), QStringLiteral("Column")});
    QVariantList m_columnWidths;
    QVariantList m_rowHeights;
    ModelUndoStack m_undoStack;
    QString m_defaultHeaderText = QStringLiteral("Column");
    QString m_defaultCellText = QStringLiteral("Text");
    double m_tableWidth = 0.0;
    int m_rowHeight = 24;
    int m_cellWidth = 0;
    int m_minColumnWidth = 32;
    int m_minRowHeight = 18;
    int m_resizingColumnIndex = -1;
    int m_resizingRowIndex = -1;
    double m_resizeStartPointerX = 0.0;
    double m_resizeStartPointerY = 0.0;
    int m_resizeStartColumnWidth = 0;
    int m_resizeStartRowHeight = 0;
    bool m_resizeHistoryRecorded = false;
    int m_contextRowIndex = -1;
    int m_contextColumnIndex = -1;
    int m_revision = 0;
    bool m_inputable = false;
    bool m_rowsStructureMutable = true;
    bool m_refreshingRowsSource = false;
};
