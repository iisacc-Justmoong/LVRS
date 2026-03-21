#include "backend/runtime/appbootstrap.h"

#include "backend/fonts/fontpolicy.h"
#include "backend/runtime/renderquality.h"

#include <QDebug>
#include <QGuiApplication>
#include <QQuickStyle>

namespace {

struct RenderQualityBootstrapProfile {
    int msaaSamples = 4;
    bool nativeTextRendering = true;
    int framesInFlight = 2;
    bool partialUpdateEnabled = true;
    bool batchRenderingEnabled = true;
    bool pipelineCacheEnabled = true;
    int textureAtlasEdge = 2048;
};

void seedBootstrapBooleanEnvironment(const char *name, bool enabled)
{
    if (enabled) {
        if (qEnvironmentVariableIsEmpty(name))
            qputenv(name, QByteArrayLiteral("1"));
        return;
    }

    qputenv(name, QByteArrayLiteral("0"));
}

RenderQualityBootstrapProfile resolveRenderQualityBootstrapProfile()
{
    RenderQualityBootstrapProfile profile;

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    profile.msaaSamples = 2;
    profile.framesInFlight = 2;
    profile.textureAtlasEdge = 1024;
#elif defined(Q_OS_WASM)
    profile.msaaSamples = 2;
    profile.framesInFlight = 1;
    profile.partialUpdateEnabled = false;
    profile.batchRenderingEnabled = false;
    profile.pipelineCacheEnabled = false;
    profile.textureAtlasEdge = 1024;
#elif defined(Q_OS_MACOS)
    profile.msaaSamples = 4;
    profile.framesInFlight = 3;
#elif defined(Q_OS_WIN)
    profile.msaaSamples = 4;
    profile.framesInFlight = 2;
#else
    profile.msaaSamples = 4;
    profile.framesInFlight = 2;
#endif

    return profile;
}

void applyRenderQualityBootstrapEnvironment(const RenderQualityBootstrapProfile &profile)
{
    seedBootstrapBooleanEnvironment("QSG_RHI_PIPELINE_CACHE_LOAD", profile.pipelineCacheEnabled);
    seedBootstrapBooleanEnvironment("QSG_RHI_PIPELINE_CACHE_SAVE", profile.pipelineCacheEnabled);

    if (profile.batchRenderingEnabled && profile.textureAtlasEdge > 0) {
        const QByteArray atlasEdge = QByteArray::number(profile.textureAtlasEdge);
        if (qEnvironmentVariableIsEmpty("QSG_ATLAS_WIDTH"))
            qputenv("QSG_ATLAS_WIDTH", atlasEdge);
        if (qEnvironmentVariableIsEmpty("QSG_ATLAS_HEIGHT"))
            qputenv("QSG_ATLAS_HEIGHT", atlasEdge);
    }
}

void logGraphicsBackend(const lvrs::GraphicsBackendBootstrapResult &backend)
{
    if (backend.loaderName.isEmpty()) {
        qInfo() << "LVRS graphics backend:" << backend.backendName;
        return;
    }
    qInfo() << "LVRS graphics backend:" << backend.backendName
            << ", loader =" << backend.loaderName;
}

} // namespace

namespace lvrs {

AppBootstrapState preApplicationBootstrap(const AppBootstrapOptions &options)
{
    AppBootstrapState state;

    if (options.configureRenderQualityDefaults) {
        const RenderQualityBootstrapProfile profile = resolveRenderQualityBootstrapProfile();
        applyRenderQualityBootstrapEnvironment(profile);
        RenderQuality::configureGlobalDefaults(profile.msaaSamples,
                                              profile.nativeTextRendering,
                                              profile.framesInFlight,
                                              profile.partialUpdateEnabled,
                                              profile.batchRenderingEnabled);
    }

    const QString quickStyleName = options.quickStyleName.trimmed();
    if (!quickStyleName.isEmpty())
        QQuickStyle::setStyle(quickStyleName);

    if (options.bootstrapGraphicsBackend) {
        state.graphicsBackend = bootstrapPreferredGraphicsBackend();
        if (!state.graphicsBackend.available) {
            state.ok = false;
            state.errorMessage = state.graphicsBackend.errorMessage;
            return state;
        }
        if (options.logGraphicsBackend)
            logGraphicsBackend(state.graphicsBackend);
    }

    return state;
}

void postApplicationBootstrap(QGuiApplication &app, const AppBootstrapOptions &options)
{
    const QString appName = options.applicationName.trimmed();
    if (!appName.isEmpty())
        app.setApplicationName(appName);

    if (options.installBundledFonts)
        FontPolicy::loadBundledFonts();

    if (options.installPretendardFallbacks)
        FontPolicy::installPretendardFallbacks();

    if (options.enforcePretendardFallback && !FontPolicy::enforcePretendardFallback())
        qWarning() << "Pretendard fallback could not be enforced.";
}

} // namespace lvrs
