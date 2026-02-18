#pragma once

#include <QObject>
#include <QVariantMap>
#include <QtQml/qqml.h>

class RouteMatcher : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RouteMatcher)
    QML_SINGLETON

public:
    explicit RouteMatcher(QObject *parent = nullptr);

    Q_INVOKABLE QString normalizePath(const QString &path) const;
    Q_INVOKABLE QVariantMap match(const QString &path, const QString &routePath) const;

private:
    static QStringList splitSegments(const QString &normalizedPath);
};

