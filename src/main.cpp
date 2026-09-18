#include "backend/runtime/appentry.h"

#include <QDebug>
#include <QStringList>
#include <QtPlugin>

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

namespace {

struct LaunchConfig {
    QString appName = QStringLiteral("LVRSApp");
    QString quickStyle = QStringLiteral("Basic");
    QString moduleUri = QStringLiteral("YourAppModule");
    QString rootObject = QStringLiteral("Main");
    bool printHelp = false;
};

QString readEnvironmentOrDefault(const char *envName, const QString &fallback)
{
    const QString value = qEnvironmentVariable(envName).trimmed();
    return value.isEmpty() ? fallback : value;
}

void printUsage(const QString &programName)
{
    const QString usage = QStringLiteral(
                              "LVRS template launcher\n"
                              "Usage: %1 [options]\n"
                              "Options:\n"
                              "  --module <uri>        QML module URI to load (default: YourAppModule)\n"
                              "  --root <type>         Root QML type name (default: Main)\n"
                              "  --app-name <name>     Application name\n"
                              "  --style <name>        Qt Quick Controls style (default: Basic)\n"
                              "  --help                Show this help\n"
                              "Environment fallback:\n"
                              "  LVRS_APP_MODULE_URI, LVRS_APP_ROOT_OBJECT, LVRS_APP_NAME, LVRS_QUICK_STYLE\n"
                              "Static plugin:\n"
                              "  LVRS_USE_STATIC_QML_PLUGIN is provided automatically for static builds.")
                              .arg(programName);
    qInfo().noquote() << usage;
}

bool parseSingleValueOption(
    const QStringList &args,
    int *index,
    const QString &argument,
    const QString &optionName,
    QString *value,
    bool *matched)
{
    *matched = false;
    const QString inlinePrefix = optionName + QStringLiteral("=");

    if (argument.startsWith(inlinePrefix)) {
        *value = argument.sliced(inlinePrefix.size()).trimmed();
        *matched = true;
        return true;
    }

    if (argument == optionName) {
        const int nextIndex = *index + 1;
        if (nextIndex >= args.size()) {
            qCritical().noquote() << "Missing value for option:" << optionName;
            return false;
        }
        *value = args.at(nextIndex).trimmed();
        *index = nextIndex;
        *matched = true;
    }

    return true;
}

bool parseLaunchConfig(const QStringList &args, LaunchConfig *config)
{
    for (int i = 1; i < args.size(); ++i) {
        const QString argument = args.at(i).trimmed();
        if (argument.isEmpty())
            continue;

        if (argument == QStringLiteral("--help") || argument == QStringLiteral("-h")) {
            config->printHelp = true;
            continue;
        }

        bool matched = false;

        if (!parseSingleValueOption(args, &i, argument, QStringLiteral("--module"), &config->moduleUri, &matched))
            return false;
        if (matched)
            continue;

        if (!parseSingleValueOption(args, &i, argument, QStringLiteral("--root"), &config->rootObject, &matched))
            return false;
        if (matched)
            continue;

        if (!parseSingleValueOption(args, &i, argument, QStringLiteral("--app-name"), &config->appName, &matched))
            return false;
        if (matched)
            continue;

        if (!parseSingleValueOption(args, &i, argument, QStringLiteral("--style"), &config->quickStyle, &matched))
            return false;
        if (matched)
            continue;

        qWarning().noquote() << "Ignoring unknown option:" << argument;
    }

    return true;
}

} // namespace

/*
 * LVRS downstream app template entrypoint.
 * This file is not built by the LVRS framework CMake target.
 */
int main(int argc, char *argv[])
{
    QStringList args;
    args.reserve(argc);
    for (int i = 0; i < argc; ++i)
        args.append(QString::fromLocal8Bit(argv[i]));

    LaunchConfig launchConfig;
    launchConfig.appName = readEnvironmentOrDefault("LVRS_APP_NAME", launchConfig.appName);
    launchConfig.quickStyle = readEnvironmentOrDefault("LVRS_QUICK_STYLE", launchConfig.quickStyle);
    launchConfig.moduleUri = readEnvironmentOrDefault("LVRS_APP_MODULE_URI", launchConfig.moduleUri);
    launchConfig.rootObject = readEnvironmentOrDefault("LVRS_APP_ROOT_OBJECT", launchConfig.rootObject);

    if (!parseLaunchConfig(args, &launchConfig))
        return -1;

    if (launchConfig.printHelp) {
        printUsage(args.isEmpty() ? QStringLiteral("app") : args.first());
        return 0;
    }

    lvrs::QmlAppLaunchSpec launchSpec;
    launchSpec.bootstrap.applicationName = launchConfig.appName;
    launchSpec.bootstrap.quickStyleName = launchConfig.quickStyle;
    launchSpec.moduleUri = launchConfig.moduleUri;
    launchSpec.rootObject = launchConfig.rootObject;

    return lvrs::runBootstrappedQmlApp(argc, argv, launchSpec);
}
