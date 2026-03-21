#include "backend/runtime/appentry.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QQmlApplicationEngine>
#include <QVariantMap>

namespace lvrs {

namespace {

void appendExistingPath(QStringList *paths, const QString &path)
{
    const QString cleanedPath = QDir::cleanPath(path.trimmed());
    if (cleanedPath.isEmpty())
        return;

    const QFileInfo pathInfo(cleanedPath);
    if (!pathInfo.isDir())
        return;
    if (paths->contains(cleanedPath))
        return;

    paths->append(cleanedPath);
}

QString compactJson(const QVariant &value)
{
    const QByteArray json = QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
    if (!json.isEmpty())
        return QString::fromUtf8(json);
    return value.toString();
}

void logBootstrapEvent(const QString &event, const QVariantMap &payload)
{
    qInfo().noquote() << QStringLiteral("LVRS bootstrap.%1 %2").arg(event, compactJson(payload));
}

} // namespace

QStringList defaultRuntimeQmlImportPaths(const QString &applicationDirPath)
{
    QStringList importPaths;
    const QString appDir = QDir::cleanPath(applicationDirPath.trimmed());
    if (appDir.isEmpty())
        return importPaths;

    appendExistingPath(&importPaths, QDir(appDir).filePath(QStringLiteral("lvrs-runtime/qml")));
    appendExistingPath(&importPaths, QDir(appDir).filePath(QStringLiteral("../lib/qt6/qml")));
    appendExistingPath(&importPaths, QDir(appDir).filePath(QStringLiteral("../qml")));
    appendExistingPath(&importPaths,
                       QDir(appDir).filePath(QStringLiteral("../../../../../platforms/linux/lib/qt6/qml")));
    return importPaths;
}

int runBootstrappedQmlApp(int argc, char *argv[], const QmlAppLaunchSpec &spec)
{
    if (spec.moduleUri.trimmed().isEmpty() || spec.rootObject.trimmed().isEmpty()) {
        qCritical().noquote() << "LVRS app entry requires non-empty module URI and root object.";
        return -1;
    }

    const AppBootstrapState bootstrapState = preApplicationBootstrap(spec.bootstrap);
    if (!bootstrapState.ok) {
        qCritical().noquote() << bootstrapState.errorMessage;
        return -1;
    }

    QGuiApplication app(argc, argv);
    postApplicationBootstrap(app, spec.bootstrap);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    QStringList runtimeImportPaths;
    if (spec.includeDefaultRuntimeQmlImportPaths)
        runtimeImportPaths = defaultRuntimeQmlImportPaths(QCoreApplication::applicationDirPath());
    for (const QString &path : spec.qmlImportPaths)
        appendExistingPath(&runtimeImportPaths, path);
    for (const QString &path : runtimeImportPaths)
        engine.addImportPath(path);

    if (spec.bootstrap.logBootstrapDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("applicationDirPath"), QCoreApplication::applicationDirPath());
        payload.insert(QStringLiteral("includeDefaultRuntimeQmlImportPaths"),
                       spec.includeDefaultRuntimeQmlImportPaths);
        payload.insert(QStringLiteral("importPathCount"), runtimeImportPaths.size());
        payload.insert(QStringLiteral("importPaths"), runtimeImportPaths);
        logBootstrapEvent(QStringLiteral("entry.import-paths"), payload);
    }

    if (spec.configureEngine)
        spec.configureEngine(engine);

    if (spec.bootstrap.logBootstrapDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("moduleUri"), spec.moduleUri);
        payload.insert(QStringLiteral("rootObject"), spec.rootObject);
        payload.insert(QStringLiteral("configureEngine"), spec.configureEngine != nullptr);
        payload.insert(QStringLiteral("initialPropertyCount"), spec.initialProperties.size());
        payload.insert(QStringLiteral("initialPropertyKeys"), spec.initialProperties.keys());
        logBootstrapEvent(QStringLiteral("entry.load-request"), payload);
    }

    engine.setInitialProperties(spec.initialProperties);
    engine.loadFromModule(spec.moduleUri, spec.rootObject);
    engine.setInitialProperties({});

    return app.exec();
}

} // namespace lvrs
