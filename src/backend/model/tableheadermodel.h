#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

class TableHeaderModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TableHeaderModel)

    Q_PROPERTY(QVariant cellItems READ cellItems WRITE setCellItems NOTIFY sourceChanged)
    Q_PROPERTY(QVariant columns READ columns WRITE setColumns NOTIFY sourceChanged)
    Q_PROPERTY(double tableWidth READ tableWidth WRITE setTableWidth NOTIFY geometryChanged)
    Q_PROPERTY(int rowHeight READ rowHeight WRITE setRowHeight NOTIFY geometryChanged)
    Q_PROPERTY(int cellHorizontalPadding READ cellHorizontalPadding WRITE setCellHorizontalPadding NOTIFY geometryChanged)
    Q_PROPERTY(QVariant columnWidths READ columnWidths WRITE setColumnWidths NOTIFY geometryChanged)
    Q_PROPERTY(int fallbackCellWidth READ fallbackCellWidth WRITE setFallbackCellWidth NOTIFY geometryChanged)
    Q_PROPERTY(int minColumnWidth READ minColumnWidth WRITE setMinColumnWidth NOTIFY geometryChanged)
    Q_PROPERTY(QVariantList descriptors READ descriptors NOTIFY descriptorsChanged)
    Q_PROPERTY(int columnCount READ columnCount NOTIFY descriptorsChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    explicit TableHeaderModel(QObject *parent = nullptr);

    QVariant cellItems() const;
    void setCellItems(const QVariant &value);

    QVariant columns() const;
    void setColumns(const QVariant &value);

    double tableWidth() const;
    void setTableWidth(double value);

    int rowHeight() const;
    void setRowHeight(int value);

    int cellHorizontalPadding() const;
    void setCellHorizontalPadding(int value);

    QVariant columnWidths() const;
    void setColumnWidths(const QVariant &value);

    int fallbackCellWidth() const;
    void setFallbackCellWidth(int value);

    int minColumnWidth() const;
    void setMinColumnWidth(int value);

    QVariantList descriptors() const;
    int columnCount() const;
    int revision() const;

    Q_INVOKABLE QVariant resolvedColumnSource() const;
    Q_INVOKABLE QVariant columnAt(int index) const;
    Q_INVOKABLE QString normalizeColumnType(const QVariant &value) const;
    Q_INVOKABLE QString inferredColumnType(const QVariant &value) const;
    Q_INVOKABLE QString columnType(int index) const;
    Q_INVOKABLE QString columnText(int index) const;
    Q_INVOKABLE int columnPadding(int index) const;
    Q_INVOKABLE int numericWidth(const QVariant &value, int fallbackValue) const;
    Q_INVOKABLE int autoColumnWidth() const;
    Q_INVOKABLE int columnWidth(int index) const;
    Q_INVOKABLE int columnX(int index) const;
    Q_INVOKABLE QVariantMap descriptorAt(int index) const;

signals:
    void sourceChanged();
    void geometryChanged();
    void descriptorsChanged();
    void revisionChanged();

private:
    static QVariantList listFromVariant(const QVariant &value);
    static QVariantMap mapFromVariant(const QVariant &value);
    static QVariant roleValue(const QVariant &entry, const QStringList &keys, const QVariant &fallback = QVariant());
    void rebuildDescriptors();

    QVariant m_cellItems;
    QVariant m_columns = QVariant(QStringList {QStringLiteral("Column"), QStringLiteral("Column"), QStringLiteral("Column")});
    QVariantList m_columnWidths;
    QVariantList m_descriptors;
    double m_tableWidth = 0.0;
    int m_rowHeight = 24;
    int m_cellHorizontalPadding = 8;
    int m_fallbackCellWidth = 0;
    int m_minColumnWidth = 32;
    int m_revision = 0;
};
