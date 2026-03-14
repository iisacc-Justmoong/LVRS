#pragma once

#include "backend/runtime/appbootstrap.h"

#include <functional>
#include <QStringList>

class QQmlApplicationEngine;

namespace lvrs {

struct QmlAppLaunchSpec {
    AppBootstrapOptions bootstrap;
    QString moduleUri;
    QString rootObject = QStringLiteral("Main");
    QStringList qmlImportPaths;
    bool includeDefaultRuntimeQmlImportPaths = true;
    std::function<void(QQmlApplicationEngine &engine)> configureEngine;
};

QStringList defaultRuntimeQmlImportPaths(const QString &applicationDirPath);
int runBootstrappedQmlApp(int argc, char *argv[], const QmlAppLaunchSpec &spec);

} // namespace lvrs
