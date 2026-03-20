#include "backend/runtime/appentry.h"

#include <QtPlugin>
#include <QVariantMap>

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

int main(int argc, char *argv[])
{
    lvrs::QmlAppLaunchSpec launchSpec;
    launchSpec.bootstrap.applicationName = QStringLiteral("LVRSExampleAndroidHello");
    launchSpec.bootstrap.quickStyleName = QStringLiteral("Basic");
    launchSpec.moduleUri = QStringLiteral("ExampleAndroidHello");
    launchSpec.rootObject = QStringLiteral("Main");
    launchSpec.initialProperties = QVariantMap{
        {QStringLiteral("bootstrapTitle"), QStringLiteral("Android Hello")},
        {QStringLiteral("bootstrapSubtitle"), QStringLiteral("LVRS Example")},
        {QStringLiteral("bootstrapMessage"),
         QStringLiteral("This app verifies LVRS Android bootstrap, safe-area defaults, runtime attach, and first-frame page-stack initialization.")},
        {QStringLiteral("platformLabel"), QStringLiteral("Android")},
        {QStringLiteral("initialRoutePath"), QStringLiteral("/")}
    };

    return lvrs::runBootstrappedQmlApp(argc, argv, launchSpec);
}
