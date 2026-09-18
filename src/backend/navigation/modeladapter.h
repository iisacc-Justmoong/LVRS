#pragma once

#include <QObject>
#include <QVariant>
#include <QVariantMap>
#include <QtQml/qqml.h>

class QAbstractItemModel;

class ModelAdapter : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ModelAdapter)
    QML_SINGLETON

public:
    explicit ModelAdapter(QObject *parent = nullptr);

    Q_INVOKABLE bool isItemModel(const QVariant &model) const;
    Q_INVOKABLE int count(const QVariant &model) const;
    Q_INVOKABLE QVariantMap row(const QVariant &model, int row, int column = 0) const;

private:
    static QAbstractItemModel *itemModelFromVariant(const QVariant &model);
};
