#include "backend/navigation/modeladapter.h"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>

ModelAdapter::ModelAdapter(QObject *parent)
    : QObject(parent)
{
}

bool ModelAdapter::isItemModel(const QVariant &model) const
{
    return itemModelFromVariant(model) != nullptr;
}

int ModelAdapter::count(const QVariant &model) const
{
    const QAbstractItemModel *itemModel = itemModelFromVariant(model);
    if (!itemModel)
        return 0;

    return qMax(0, itemModel->rowCount(QModelIndex()));
}

QVariantMap ModelAdapter::row(const QVariant &model, int row, int column) const
{
    QVariantMap result;
    QAbstractItemModel *itemModel = itemModelFromVariant(model);
    if (!itemModel)
        return result;

    if (row < 0 || row >= itemModel->rowCount(QModelIndex()))
        return result;

    column = qMax(0, column);
    if (column >= itemModel->columnCount(QModelIndex()))
        column = 0;

    const QModelIndex index = itemModel->index(row, column, QModelIndex());
    if (!index.isValid())
        return result;

    const QHash<int, QByteArray> roles = itemModel->roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        if (it.value().isEmpty())
            continue;
        const QVariant value = itemModel->data(index, it.key());
        if (value.isValid())
            result.insert(QString::fromUtf8(it.value()), value);
    }

    if (!result.contains(QStringLiteral("display"))) {
        const QVariant displayValue = itemModel->data(index, Qt::DisplayRole);
        if (displayValue.isValid())
            result.insert(QStringLiteral("display"), displayValue);
    }

    if (!result.contains(QStringLiteral("edit"))) {
        const QVariant editValue = itemModel->data(index, Qt::EditRole);
        if (editValue.isValid())
            result.insert(QStringLiteral("edit"), editValue);
    }

    result.insert(QStringLiteral("index"), row);
    result.insert(QStringLiteral("row"), row);
    result.insert(QStringLiteral("column"), column);

    return result;
}

QAbstractItemModel *ModelAdapter::itemModelFromVariant(const QVariant &model)
{
    QObject *object = nullptr;
    if (model.canConvert<QObject *>())
        object = model.value<QObject *>();
    if (!object)
        object = qvariant_cast<QObject *>(model);

    return qobject_cast<QAbstractItemModel *>(object);
}
