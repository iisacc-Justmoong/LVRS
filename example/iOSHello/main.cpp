#include "backend/runtime/appentry.h"

#include <QtPlugin>
#include <QVariantMap>

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

int main(int argc, char *argv[])
{
    lvrs::QmlAppLaunchSpec launchSpec;
    launchSpec.bootstrap.applicationName = QStringLiteral("LVRSExampleIOSHello");
    launchSpec.bootstrap.quickStyleName = QStringLiteral("Basic");
    launchSpec.moduleUri = QStringLiteral("ExampleIOSHello");
    launchSpec.rootObject = QStringLiteral("Main");
    launchSpec.initialProperties = QVariantMap{
        {QStringLiteral("bootstrapTitle"), QStringLiteral("iOS Hello")},
        {QStringLiteral("bootstrapSubtitle"), QStringLiteral("LVRS Example")},
        {QStringLiteral("bootstrapMessage"),
         QStringLiteral("This app verifies LVRS iOS bootstrap, safe-area defaults, runtime attach, and first-frame page-stack initialization.")},
        {QStringLiteral("platformLabel"), QStringLiteral("iOS")},
        {QStringLiteral("initialRoutePath"), QStringLiteral("/")}
    };

    return lvrs::runBootstrappedQmlApp(argc, argv, launchSpec);
}
