#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqml.h>

#include <functional>

namespace lvrs {

enum class QmlTypeRegistrationKind {
    Creatable,
    Uncreatable,
    Singleton,
    Custom
};

QString qmlTypeRegistrationKindName(QmlTypeRegistrationKind kind);

struct QmlTypeRegistration {
    QString uri;
    int majorVersion = 1;
    int minorVersion = 0;
    QString qmlName;
    QString diagnosticName;
    QmlTypeRegistrationKind kind = QmlTypeRegistrationKind::Custom;
    bool required = true;
    std::function<int()> registerCallback;

    QString qualifiedName() const;
};

struct QmlTypeRegistrationResult {
    QString uri;
    int majorVersion = 1;
    int minorVersion = 0;
    QString qmlName;
    QString diagnosticName;
    QmlTypeRegistrationKind kind = QmlTypeRegistrationKind::Custom;
    int typeId = -1;
    bool ok = false;
    bool skipped = false;
    QString error;

    QString qualifiedName() const;
    QVariantMap toVariantMap() const;
};

struct QmlTypeRegistrationReport {
    bool ok = true;
    QList<QmlTypeRegistrationResult> results;
    QStringList errors;

    QString errorMessage() const;
    QVariantList diagnostics() const;
};

QmlTypeRegistration qmlCustomTypeRegistration(const QString &uri,
                                              int majorVersion,
                                              int minorVersion,
                                              const QString &qmlName,
                                              QmlTypeRegistrationKind kind,
                                              std::function<int()> registerCallback,
                                              const QString &diagnosticName = QString(),
                                              bool required = true);

QmlTypeRegistrationReport registerQmlTypes(const QList<QmlTypeRegistration> &manifest);

template <typename T>
QmlTypeRegistration qmlCreatableType(const QString &uri,
                                     int majorVersion,
                                     int minorVersion,
                                     const QString &qmlName,
                                     const QString &diagnosticName = QString(),
                                     bool required = true)
{
    return qmlCustomTypeRegistration(uri,
                                     majorVersion,
                                     minorVersion,
                                     qmlName,
                                     QmlTypeRegistrationKind::Creatable,
                                     [uri, majorVersion, minorVersion, qmlName]() {
                                         const QByteArray uriBytes = uri.toUtf8();
                                         const QByteArray qmlNameBytes = qmlName.toUtf8();
                                         return qmlRegisterType<T>(uriBytes.constData(),
                                                                   majorVersion,
                                                                   minorVersion,
                                                                   qmlNameBytes.constData());
                                     },
                                     diagnosticName,
                                     required);
}

template <typename T>
QmlTypeRegistration qmlUncreatableType(const QString &uri,
                                       int majorVersion,
                                       int minorVersion,
                                       const QString &qmlName,
                                       const QString &reason,
                                       const QString &diagnosticName = QString(),
                                       bool required = true)
{
    return qmlCustomTypeRegistration(uri,
                                     majorVersion,
                                     minorVersion,
                                     qmlName,
                                     QmlTypeRegistrationKind::Uncreatable,
                                     [uri, majorVersion, minorVersion, qmlName, reason]() {
                                         const QByteArray uriBytes = uri.toUtf8();
                                         const QByteArray qmlNameBytes = qmlName.toUtf8();
                                         const QByteArray reasonBytes = reason.toUtf8();
                                         return qmlRegisterUncreatableType<T>(uriBytes.constData(),
                                                                              majorVersion,
                                                                              minorVersion,
                                                                              qmlNameBytes.constData(),
                                                                              reasonBytes.constData());
                                     },
                                     diagnosticName,
                                     required);
}

} // namespace lvrs
