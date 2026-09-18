#include "backend/model/modelsource.h"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QHash>
#include <QJSValue>
#include <QMetaObject>
#include <QModelIndex>
#include <QSequentialIterable>
#include <QStringList>

namespace {
constexpr int kInvalidCount = -1;

bool isInvalidOrNull(const QVariant &value)
{
    return !value.isValid() || value.isNull();
}

QVariant jsValueToVariant(const QJSValue &value)
{
    if (value.isUndefined() || value.isNull())
        return QVariant();
    return value.toVariant();
}

int jsLength(const QJSValue &value)
{
    if (!value.isObject())
        return kInvalidCount;
    const QJSValue length = value.property(QStringLiteral("length"));
    if (length.isNumber())
        return qMax(0, length.toInt());
    const QJSValue count = value.property(QStringLiteral("count"));
    if (count.isNumber())
        return qMax(0, count.toInt());
    return kInvalidCount;
}
} // namespace

ModelSource::ModelSource(QObject *parent)
    : QObject(parent)
{
}

QVariant ModelSource::source() const
{
    return m_source;
}

void ModelSource::setSource(const QVariant &source)
{
    if (m_source == source)
        return;

    const int previousCount = count();
    m_source = source;
    reconnectSourceSignals();
    m_revision += 1;
    emit sourceChanged();
    emit revisionChanged();
    if (previousCount != count())
        emit countChanged();
}

int ModelSource::column() const
{
    return m_column;
}

void ModelSource::setColumn(int column)
{
    const int next = qMax(0, column);
    if (m_column == next)
        return;
    m_column = next;
    m_revision += 1;
    emit columnChanged();
    emit revisionChanged();
}

int ModelSource::count() const
{
    return countForVariant(m_source);
}

int ModelSource::revision() const
{
    return m_revision;
}

bool ModelSource::itemModel() const
{
    return itemModelFromVariant(m_source) != nullptr;
}

QVariant ModelSource::at(int index) const
{
    if (index < 0)
        return QVariant();

    if (QAbstractItemModel *model = itemModelFromVariant(m_source))
        return row(index);

    return listValueAt(m_source, index);
}

QVariantMap ModelSource::row(int index) const
{
    QVariantMap result;
    QAbstractItemModel *model = itemModelFromVariant(m_source);
    if (!model)
        return result;

    if (index < 0 || index >= model->rowCount(QModelIndex()))
        return result;

    int column = qMax(0, m_column);
    if (column >= model->columnCount(QModelIndex()))
        column = 0;

    const QModelIndex modelIndex = model->index(index, column, QModelIndex());
    if (!modelIndex.isValid())
        return result;

    const QHash<int, QByteArray> roles = model->roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        if (it.value().isEmpty())
            continue;
        const QVariant value = model->data(modelIndex, it.key());
        if (value.isValid())
            result.insert(QString::fromUtf8(it.value()), value);
    }

    if (!result.contains(QStringLiteral("display"))) {
        const QVariant displayValue = model->data(modelIndex, Qt::DisplayRole);
        if (displayValue.isValid())
            result.insert(QStringLiteral("display"), displayValue);
    }

    if (!result.contains(QStringLiteral("edit"))) {
        const QVariant editValue = model->data(modelIndex, Qt::EditRole);
        if (editValue.isValid())
            result.insert(QStringLiteral("edit"), editValue);
    }

    result.insert(QStringLiteral("index"), index);
    result.insert(QStringLiteral("row"), index);
    result.insert(QStringLiteral("column"), column);
    return result;
}

QVariant ModelSource::roleValue(const QVariant &entry,
                                const QString &roleName,
                                const QVariant &fallbackValue) const
{
    const QString key = roleName.trimmed();
    if (key.isEmpty() || isInvalidOrNull(entry))
        return fallbackValue;

    if (entry.userType() == qMetaTypeId<QJSValue>()) {
        const QJSValue value = entry.value<QJSValue>();
        if (value.isObject()) {
            const QJSValue property = value.property(key);
            if (!property.isUndefined())
                return jsValueToVariant(property);
        }
        return fallbackValue;
    }

    if (entry.canConvert<QVariantMap>()) {
        const QVariantMap map = entry.toMap();
        return map.contains(key) ? map.value(key) : fallbackValue;
    }

    if (entry.canConvert<QObject *>()) {
        if (QObject *object = entry.value<QObject *>()) {
            const QVariant property = object->property(key.toUtf8().constData());
            return property.isValid() ? property : fallbackValue;
        }
    }

    return fallbackValue;
}

QString ModelSource::textValue(const QVariant &entry,
                               const QVariant &roleNames,
                               const QString &fallbackValue) const
{
    if (isInvalidOrNull(entry))
        return fallbackValue;

    if (!entry.canConvert<QVariantMap>()
            && entry.userType() != qMetaTypeId<QJSValue>()
            && !entry.canConvert<QObject *>()) {
        return entry.toString();
    }

    const QStringList roles = roleListFromVariant(roleNames);
    for (const QString &role : roles) {
        const QVariant value = roleValue(entry, role, QVariant());
        if (value.isValid() && !value.isNull())
            return value.toString();
    }

    return fallbackValue;
}

bool ModelSource::boolValue(const QVariant &entry, const QString &roleName, bool fallbackValue) const
{
    const QVariant value = roleValue(entry, roleName, QVariant());
    if (!value.isValid() || value.isNull())
        return fallbackValue;
    if (value.metaType().id() == QMetaType::Bool)
        return value.toBool();
    if (value.canConvert<int>())
        return value.toInt() != 0;
    const QString text = value.toString().trimmed().toLower();
    if (text.isEmpty() || text == QStringLiteral("0") || text == QStringLiteral("false") || text == QStringLiteral("no"))
        return false;
    return true;
}

int ModelSource::intValue(const QVariant &entry, const QString &roleName, int fallbackValue) const
{
    bool ok = false;
    const QVariant value = roleValue(entry, roleName, QVariant());
    const int result = value.toInt(&ok);
    return ok ? result : fallbackValue;
}

void ModelSource::invalidate()
{
    m_revision += 1;
    emit revisionChanged();
    emit countChanged();
}

QAbstractItemModel *ModelSource::itemModelFromVariant(const QVariant &value)
{
    return qobject_cast<QAbstractItemModel *>(objectFromVariant(value));
}

QObject *ModelSource::objectFromVariant(const QVariant &value)
{
    QObject *object = nullptr;
    if (value.canConvert<QObject *>())
        object = value.value<QObject *>();
    if (!object)
        object = qvariant_cast<QObject *>(value);
    return object;
}

int ModelSource::countForVariant(const QVariant &value)
{
    if (isInvalidOrNull(value))
        return 0;

    if (QAbstractItemModel *model = itemModelFromVariant(value))
        return qMax(0, model->rowCount(QModelIndex()));

    if (value.userType() == qMetaTypeId<QJSValue>()) {
        const int count = jsLength(value.value<QJSValue>());
        if (count >= 0)
            return count;
    }

    if (value.canConvert<QVariantList>())
        return value.toList().size();
    if (value.canConvert<QStringList>())
        return value.toStringList().size();

    if (value.canConvert<QSequentialIterable>()) {
        int count = 0;
        const QSequentialIterable iterable = value.value<QSequentialIterable>();
        for (auto it = iterable.begin(); it != iterable.end(); ++it)
            count += 1;
        return count;
    }

    if (QObject *object = objectFromVariant(value)) {
        const QVariant count = object->property("count");
        if (count.isValid())
            return qMax(0, count.toInt());
        const QVariant length = object->property("length");
        if (length.isValid())
            return qMax(0, length.toInt());
    }

    return 0;
}

QVariant ModelSource::listValueAt(const QVariant &value, int index)
{
    if (index < 0 || isInvalidOrNull(value))
        return QVariant();

    if (value.userType() == qMetaTypeId<QJSValue>()) {
        QJSValue instance = value.value<QJSValue>();
        if (instance.isArray())
            return jsValueToVariant(instance.property(static_cast<quint32>(index)));

        QJSValue getter = instance.property(QStringLiteral("get"));
        if (getter.isCallable()) {
            QJSValueList args;
            args << QJSValue(index);
            return jsValueToVariant(getter.callWithInstance(instance, args));
        }

        const QJSValue entry = instance.property(static_cast<quint32>(index));
        return jsValueToVariant(entry);
    }

    if (value.canConvert<QVariantList>()) {
        const QVariantList list = value.toList();
        return index >= 0 && index < list.size() ? list.at(index) : QVariant();
    }
    if (value.canConvert<QStringList>()) {
        const QStringList list = value.toStringList();
        return index >= 0 && index < list.size() ? QVariant(list.at(index)) : QVariant();
    }

    if (QObject *object = objectFromVariant(value)) {
        QVariant returned;
        if (QMetaObject::invokeMethod(object,
                                      "get",
                                      Q_RETURN_ARG(QVariant, returned),
                                      Q_ARG(int, index))) {
            return returned;
        }
        if (QMetaObject::invokeMethod(object,
                                      "at",
                                      Q_RETURN_ARG(QVariant, returned),
                                      Q_ARG(int, index))) {
            return returned;
        }
    }

    return QVariant();
}

QStringList ModelSource::roleListFromVariant(const QVariant &value)
{
    QStringList result;
    if (value.userType() == qMetaTypeId<QJSValue>()) {
        const QVariant converted = value.value<QJSValue>().toVariant();
        return roleListFromVariant(converted);
    }
    if (value.canConvert<QStringList>())
        return value.toStringList();
    if (value.canConvert<QVariantList>()) {
        const QVariantList list = value.toList();
        result.reserve(list.size());
        for (const QVariant &entry : list) {
            const QString role = entry.toString().trimmed();
            if (!role.isEmpty())
                result.append(role);
        }
        return result;
    }
    const QString single = value.toString().trimmed();
    if (!single.isEmpty())
        result.append(single);
    return result;
}

void ModelSource::reconnectSourceSignals()
{
    for (const QMetaObject::Connection &connection : std::as_const(m_sourceConnections))
        disconnect(connection);
    m_sourceConnections.clear();
    m_observedObject.clear();

    QAbstractItemModel *model = itemModelFromVariant(m_source);
    if (!model)
        return;

    m_observedObject = model;
    auto invalidateModel = [this]() {
        invalidate();
    };
    m_sourceConnections.append(connect(model, &QAbstractItemModel::rowsInserted, this, invalidateModel));
    m_sourceConnections.append(connect(model, &QAbstractItemModel::rowsRemoved, this, invalidateModel));
    m_sourceConnections.append(connect(model, &QAbstractItemModel::rowsMoved, this, invalidateModel));
    m_sourceConnections.append(connect(model, &QAbstractItemModel::modelReset, this, invalidateModel));
    m_sourceConnections.append(connect(model, &QAbstractItemModel::layoutChanged, this, invalidateModel));
    m_sourceConnections.append(connect(model, &QAbstractItemModel::dataChanged, this, invalidateModel));
    m_sourceConnections.append(connect(model, &QObject::destroyed, this, [this]() {
        m_observedObject.clear();
        invalidate();
    }));
}
