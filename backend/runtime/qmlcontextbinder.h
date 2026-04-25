#pragma once

#include <QList>
#include <QObject>
#include <QStringList>
#include <QVariantMap>

class QQmlApplicationEngine;

namespace lvrs {

struct QmlContextObjectBinding {
    QString contextName;
    QObject *object = nullptr;
    bool required = true;
};

struct QmlViewModelBinding {
    QString key;
    QObject *object = nullptr;
    QString contextName;
    QString displayName;
    QVariantMap metadata;
    QString viewId;
    bool writable = false;
    bool required = true;
};

struct QmlContextBindPlan {
    QList<QmlContextObjectBinding> contextObjects;
    QList<QmlViewModelBinding> viewModels;
};

struct QmlContextBindResult {
    bool ok = true;
    QStringList errors;
    QStringList contextNames;
    QStringList viewModelKeys;

    QString errorMessage() const;
};

QmlContextBindResult applyQmlContextBindPlan(QQmlApplicationEngine &engine,
                                             const QmlContextBindPlan &plan);

} // namespace lvrs
