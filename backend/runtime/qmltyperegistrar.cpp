#include "backend/runtime/qmltyperegistrar.h"

#include <QSet>

namespace {

QString normalizeToken(const QString &value)
{
    return value.trimmed();
}

QString registrationKey(const lvrs::QmlTypeRegistration &registration)
{
    return QStringLiteral("%1|%2|%3|%4")
        .arg(normalizeToken(registration.uri),
             QString::number(registration.majorVersion),
             QString::number(registration.minorVersion),
             normalizeToken(registration.qmlName));
}

QString qualifiedName(const QString &uri, int majorVersion, int minorVersion, const QString &qmlName)
{
    return QStringLiteral("%1 %2.%3.%4")
        .arg(normalizeToken(uri),
             QString::number(majorVersion),
             QString::number(minorVersion),
             normalizeToken(qmlName));
}

QString diagnosticNameFor(const lvrs::QmlTypeRegistration &registration)
{
    const QString explicitName = normalizeToken(registration.diagnosticName);
    if (!explicitName.isEmpty())
        return explicitName;
    return qualifiedName(registration.uri,
                         registration.majorVersion,
                         registration.minorVersion,
                         registration.qmlName);
}

lvrs::QmlTypeRegistrationResult resultFor(const lvrs::QmlTypeRegistration &registration)
{
    lvrs::QmlTypeRegistrationResult result;
    result.uri = normalizeToken(registration.uri);
    result.majorVersion = registration.majorVersion;
    result.minorVersion = registration.minorVersion;
    result.qmlName = normalizeToken(registration.qmlName);
    result.diagnosticName = diagnosticNameFor(registration);
    result.kind = registration.kind;
    return result;
}

void appendError(lvrs::QmlTypeRegistrationReport *report,
                 lvrs::QmlTypeRegistrationResult *result,
                 const QString &message)
{
    if (result) {
        result->ok = false;
        result->error = message;
    }
    if (report) {
        report->ok = false;
        report->errors.append(message);
    }
}

QString validationError(const lvrs::QmlTypeRegistration &registration)
{
    if (normalizeToken(registration.uri).isEmpty())
        return QStringLiteral("QML type registration has an empty URI.");
    if (registration.majorVersion < 0 || registration.minorVersion < 0)
        return QStringLiteral("QML type '%1' has an invalid version.").arg(diagnosticNameFor(registration));
    if (normalizeToken(registration.qmlName).isEmpty())
        return QStringLiteral("QML type registration has an empty QML name.");
    if (!registration.registerCallback)
        return QStringLiteral("QML type '%1' has no registration callback.").arg(diagnosticNameFor(registration));
    return QString();
}

void markOptionalSkip(lvrs::QmlTypeRegistrationResult *result, const QString &message)
{
    if (!result)
        return;
    result->ok = true;
    result->skipped = true;
    result->error = message;
}

} // namespace

namespace lvrs {

QString qmlTypeRegistrationKindName(QmlTypeRegistrationKind kind)
{
    switch (kind) {
    case QmlTypeRegistrationKind::Creatable:
        return QStringLiteral("creatable");
    case QmlTypeRegistrationKind::Uncreatable:
        return QStringLiteral("uncreatable");
    case QmlTypeRegistrationKind::Singleton:
        return QStringLiteral("singleton");
    case QmlTypeRegistrationKind::Custom:
        return QStringLiteral("custom");
    }
    return QStringLiteral("custom");
}

QString QmlTypeRegistration::qualifiedName() const
{
    return ::qualifiedName(uri, majorVersion, minorVersion, qmlName);
}

QString QmlTypeRegistrationResult::qualifiedName() const
{
    return ::qualifiedName(uri, majorVersion, minorVersion, qmlName);
}

QVariantMap QmlTypeRegistrationResult::toVariantMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("uri"), uri);
    map.insert(QStringLiteral("majorVersion"), majorVersion);
    map.insert(QStringLiteral("minorVersion"), minorVersion);
    map.insert(QStringLiteral("qmlName"), qmlName);
    map.insert(QStringLiteral("qualifiedName"), qualifiedName());
    map.insert(QStringLiteral("diagnosticName"), diagnosticName);
    map.insert(QStringLiteral("kind"), qmlTypeRegistrationKindName(kind));
    map.insert(QStringLiteral("typeId"), typeId);
    map.insert(QStringLiteral("ok"), ok);
    map.insert(QStringLiteral("skipped"), skipped);
    map.insert(QStringLiteral("error"), error);
    return map;
}

QString QmlTypeRegistrationReport::errorMessage() const
{
    return errors.join(QStringLiteral("; "));
}

QVariantList QmlTypeRegistrationReport::diagnostics() const
{
    QVariantList list;
    list.reserve(results.size());
    for (const QmlTypeRegistrationResult &result : results)
        list.append(result.toVariantMap());
    return list;
}

QmlTypeRegistration qmlCustomTypeRegistration(const QString &uri,
                                              int majorVersion,
                                              int minorVersion,
                                              const QString &qmlName,
                                              QmlTypeRegistrationKind kind,
                                              std::function<int()> registerCallback,
                                              const QString &diagnosticName,
                                              bool required)
{
    QmlTypeRegistration registration;
    registration.uri = uri;
    registration.majorVersion = majorVersion;
    registration.minorVersion = minorVersion;
    registration.qmlName = qmlName;
    registration.kind = kind;
    registration.registerCallback = std::move(registerCallback);
    registration.diagnosticName = diagnosticName;
    registration.required = required;
    return registration;
}

QmlTypeRegistrationReport registerQmlTypes(const QList<QmlTypeRegistration> &manifest)
{
    QmlTypeRegistrationReport report;
    QSet<QString> seenKeys;

    for (const QmlTypeRegistration &registration : manifest) {
        QmlTypeRegistrationResult result = resultFor(registration);
        const QString error = validationError(registration);
        if (!error.isEmpty()) {
            if (registration.required)
                appendError(&report, &result, error);
            else
                markOptionalSkip(&result, error);
            report.results.append(result);
            continue;
        }

        const QString key = registrationKey(registration);
        if (seenKeys.contains(key)) {
            const QString duplicateError =
                QStringLiteral("QML type '%1' is declared more than once in the registration manifest.")
                    .arg(result.diagnosticName);
            if (registration.required)
                appendError(&report, &result, duplicateError);
            else
                markOptionalSkip(&result, duplicateError);
            report.results.append(result);
            continue;
        }
        seenKeys.insert(key);

        const int typeId = registration.registerCallback();
        result.typeId = typeId;
        if (typeId < 0) {
            const QString registrationError =
                QStringLiteral("QML type '%1' registration failed.").arg(result.diagnosticName);
            if (registration.required)
                appendError(&report, &result, registrationError);
            else
                markOptionalSkip(&result, registrationError);
        } else {
            result.ok = true;
        }

        report.results.append(result);
    }

    return report;
}

} // namespace lvrs
