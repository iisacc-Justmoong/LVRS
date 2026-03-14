#include "backend/runtime/appentry.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

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

    if (spec.configureEngine)
        spec.configureEngine(engine);

    engine.loadFromModule(spec.moduleUri, spec.rootObject);

    return app.exec();
}

} // namespace lvrs
