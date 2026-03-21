#include "backend/runtime/appbootstrap.h"

#include "backend/fonts/fontpolicy.h"
#include "backend/runtime/renderquality.h"

#include <QDebug>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QQuickStyle>
#include <QSurfaceFormat>
#include <QVariantList>
#include <QVariantMap>

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

struct EnvironmentValue {
    bool isSet = false;
    QString value;
};

using EnvironmentSnapshot = QMap<QString, EnvironmentValue>;

QString compactJson(const QVariant &value)
{
    const QByteArray json = QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact);
    if (!json.isEmpty())
        return QString::fromUtf8(json);
    return value.toString();
}

void logBootstrapEvent(const QString &event, const QVariantMap &payload, QtMsgType type = QtInfoMsg)
{
    const QString line = QStringLiteral("LVRS bootstrap.%1 %2").arg(event, compactJson(payload));
    switch (type) {
    case QtWarningMsg:
        qWarning().noquote() << line;
        break;
    case QtCriticalMsg:
        qCritical().noquote() << line;
        break;
    default:
        qInfo().noquote() << line;
        break;
    }
}

QString bootstrapPlatformTag()
{
#if defined(Q_OS_ANDROID)
    return QStringLiteral("android");
#elif defined(Q_OS_IOS)
    return QStringLiteral("ios");
#elif defined(Q_OS_WASM)
    return QStringLiteral("wasm");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#elif defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux");
#else
    return QStringLiteral("unknown");
#endif
}

QStringList bootstrapEnvironmentKeys()
{
    return QStringList({
        QStringLiteral("QSG_RHI_PIPELINE_CACHE_LOAD"),
        QStringLiteral("QSG_RHI_PIPELINE_CACHE_SAVE"),
        QStringLiteral("QSG_PARTIAL_UPDATE"),
        QStringLiteral("QSG_NO_FULL_REDRAW"),
        QStringLiteral("QSG_BATCH_RENDERER"),
        QStringLiteral("QSG_ATLAS_WIDTH"),
        QStringLiteral("QSG_ATLAS_HEIGHT"),
        QStringLiteral("QSG_RHI_BACKEND"),
        QStringLiteral("QT_VULKAN_LIB")
    });
}

EnvironmentSnapshot captureEnvironmentSnapshot(const QStringList &keys)
{
    EnvironmentSnapshot snapshot;
    for (const QString &key : keys) {
        const QByteArray name = key.toUtf8();
        EnvironmentValue value;
        value.isSet = qEnvironmentVariableIsSet(name.constData());
        if (value.isSet)
            value.value = qEnvironmentVariable(name.constData());
        snapshot.insert(key, value);
    }
    return snapshot;
}

QVariantList describeEnvironmentChanges(const EnvironmentSnapshot &before, const QStringList &keys)
{
    QVariantList description;
    for (const QString &key : keys) {
        const QByteArray name = key.toUtf8();
        const EnvironmentValue previous = before.value(key);
        const bool isSet = qEnvironmentVariableIsSet(name.constData());
        const QString value = isSet ? qEnvironmentVariable(name.constData()) : QString();

        QVariantMap item;
        item.insert(QStringLiteral("name"), key);
        item.insert(QStringLiteral("wasSet"), previous.isSet);
        if (previous.isSet)
            item.insert(QStringLiteral("previousValue"), previous.value);
        item.insert(QStringLiteral("isSet"), isSet);
        if (isSet)
            item.insert(QStringLiteral("value"), value);
        item.insert(QStringLiteral("changed"), previous.isSet != isSet || previous.value != value);
        item.insert(QStringLiteral("preservedExisting"), previous.isSet && previous.value == value);
        description.append(item);
    }
    return description;
}

QVariantMap renderProfileSummary(const RenderQualityBootstrapProfile &profile)
{
    QVariantMap summary;
    summary.insert(QStringLiteral("platform"), bootstrapPlatformTag());
    summary.insert(QStringLiteral("msaaSamples"), profile.msaaSamples);
    summary.insert(QStringLiteral("nativeTextRendering"), profile.nativeTextRendering);
    summary.insert(QStringLiteral("framesInFlight"), profile.framesInFlight);
    summary.insert(QStringLiteral("partialUpdateEnabled"), profile.partialUpdateEnabled);
    summary.insert(QStringLiteral("batchRenderingEnabled"), profile.batchRenderingEnabled);
    summary.insert(QStringLiteral("pipelineCacheEnabled"), profile.pipelineCacheEnabled);
    summary.insert(QStringLiteral("textureAtlasEdge"), profile.textureAtlasEdge);

    const QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    QVariantMap surfaceFormat;
    surfaceFormat.insert(QStringLiteral("samples"), format.samples());
    surfaceFormat.insert(QStringLiteral("depthBufferSize"), format.depthBufferSize());
    surfaceFormat.insert(QStringLiteral("stencilBufferSize"), format.stencilBufferSize());
    summary.insert(QStringLiteral("surfaceFormat"), surfaceFormat);
    return summary;
}

QVariantMap bootstrapOptionsSummary(const lvrs::AppBootstrapOptions &options)
{
    QVariantMap summary;
    summary.insert(QStringLiteral("platform"), bootstrapPlatformTag());
    summary.insert(QStringLiteral("applicationName"), options.applicationName);
    summary.insert(QStringLiteral("quickStyleName"), options.quickStyleName);
    summary.insert(QStringLiteral("configureRenderQualityDefaults"), options.configureRenderQualityDefaults);
    summary.insert(QStringLiteral("bootstrapGraphicsBackend"), options.bootstrapGraphicsBackend);
    summary.insert(QStringLiteral("logBootstrapDiagnostics"), options.logBootstrapDiagnostics);
    summary.insert(QStringLiteral("logGraphicsBackend"), options.logGraphicsBackend);
    summary.insert(QStringLiteral("installBundledFonts"), options.installBundledFonts);
    summary.insert(QStringLiteral("installPretendardFallbacks"), options.installPretendardFallbacks);
    summary.insert(QStringLiteral("enforcePretendardFallback"), options.enforcePretendardFallback);
    summary.insert(QStringLiteral("qtVersion"), QString::fromLatin1(qVersion()));
    return summary;
}

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
    } else {
        qInfo() << "LVRS graphics backend:" << backend.backendName
                << ", loader =" << backend.loaderName;
    }

    QVariantMap payload;
    payload.insert(QStringLiteral("requestedBackend"),
                   backend.requestedBackendName.isEmpty() ? backend.backendName : backend.requestedBackendName);
    payload.insert(QStringLiteral("selectedBackend"), backend.backendName);
    payload.insert(QStringLiteral("loader"), backend.loaderName);
    payload.insert(QStringLiteral("fallbackUsed"), backend.fallbackUsed);
    if (!backend.fallbackReason.isEmpty())
        payload.insert(QStringLiteral("fallbackReason"), backend.fallbackReason);
    if (!backend.probeCandidates.isEmpty()) {
        payload.insert(QStringLiteral("candidateCount"), backend.probeCandidates.size());
        payload.insert(QStringLiteral("candidates"), backend.probeCandidates);
    }
    logBootstrapEvent(QStringLiteral("pre.graphics-backend"), payload);
}

} // namespace

namespace lvrs {

AppBootstrapState preApplicationBootstrap(const AppBootstrapOptions &options)
{
    AppBootstrapState state;
    const QStringList environmentKeys = bootstrapEnvironmentKeys();

    if (options.logBootstrapDiagnostics)
        logBootstrapEvent(QStringLiteral("pre.options"), bootstrapOptionsSummary(options));

    if (options.configureRenderQualityDefaults) {
        const EnvironmentSnapshot environmentBefore = captureEnvironmentSnapshot(environmentKeys);
        const RenderQualityBootstrapProfile profile = resolveRenderQualityBootstrapProfile();
        applyRenderQualityBootstrapEnvironment(profile);
        RenderQuality::configureGlobalDefaults(profile.msaaSamples,
                                              profile.nativeTextRendering,
                                              profile.framesInFlight,
                                              profile.partialUpdateEnabled,
                                              profile.batchRenderingEnabled);
        if (options.logBootstrapDiagnostics) {
            QVariantMap payload = renderProfileSummary(profile);
            payload.insert(QStringLiteral("environment"), describeEnvironmentChanges(environmentBefore, environmentKeys));
            logBootstrapEvent(QStringLiteral("pre.render-quality"), payload);
        }
    } else if (options.logBootstrapDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("platform"), bootstrapPlatformTag());
        payload.insert(QStringLiteral("configured"), false);
        logBootstrapEvent(QStringLiteral("pre.render-quality"), payload);
    }

    const QString quickStyleName = options.quickStyleName.trimmed();
    if (!quickStyleName.isEmpty())
        QQuickStyle::setStyle(quickStyleName);
    if (options.logBootstrapDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requested"), !quickStyleName.isEmpty());
        payload.insert(QStringLiteral("style"), quickStyleName);
        logBootstrapEvent(QStringLiteral("pre.quick-style"), payload);
    }

    if (options.bootstrapGraphicsBackend) {
        state.graphicsBackend = bootstrapPreferredGraphicsBackend(options.logBootstrapDiagnostics);
        if (!state.graphicsBackend.available) {
            state.ok = false;
            state.errorMessage = state.graphicsBackend.errorMessage;
            if (options.logBootstrapDiagnostics) {
                QVariantMap payload;
                payload.insert(QStringLiteral("ok"), false);
                payload.insert(QStringLiteral("error"), state.errorMessage);
                payload.insert(QStringLiteral("requestedBackend"), state.graphicsBackend.requestedBackendName);
                logBootstrapEvent(QStringLiteral("pre.complete"), payload, QtCriticalMsg);
            }
            return state;
        }
        if (options.logGraphicsBackend)
            logGraphicsBackend(state.graphicsBackend);
    } else if (options.logBootstrapDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requested"), false);
        logBootstrapEvent(QStringLiteral("pre.graphics-backend"), payload);
    }

    if (options.logBootstrapDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("ok"), true);
        if (state.graphicsBackend.available) {
            payload.insert(QStringLiteral("requestedBackend"), state.graphicsBackend.requestedBackendName);
            payload.insert(QStringLiteral("selectedBackend"), state.graphicsBackend.backendName);
            payload.insert(QStringLiteral("fallbackUsed"), state.graphicsBackend.fallbackUsed);
        }
        logBootstrapEvent(QStringLiteral("pre.complete"), payload);
    }

    return state;
}

void postApplicationBootstrap(QGuiApplication &app, const AppBootstrapOptions &options)
{
    const QString appName = options.applicationName.trimmed();
    if (!appName.isEmpty())
        app.setApplicationName(appName);
    if (options.logBootstrapDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requestedApplicationName"), appName);
        payload.insert(QStringLiteral("effectiveApplicationName"), app.applicationName());
        logBootstrapEvent(QStringLiteral("post.application"), payload);
    }

    if (options.installBundledFonts)
        FontPolicy::loadBundledFonts();

    if (options.installPretendardFallbacks)
        FontPolicy::installPretendardFallbacks();

    const bool fallbackEnforced = !options.enforcePretendardFallback || FontPolicy::enforcePretendardFallback();
    if (options.enforcePretendardFallback && !fallbackEnforced)
        qWarning() << "Pretendard fallback could not be enforced.";

    if (options.logBootstrapDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("installBundledFonts"), options.installBundledFonts);
        payload.insert(QStringLiteral("installPretendardFallbacks"), options.installPretendardFallbacks);
        payload.insert(QStringLiteral("enforcePretendardFallback"), options.enforcePretendardFallback);
        payload.insert(QStringLiteral("pretendardFallbackEnforced"), fallbackEnforced);
        logBootstrapEvent(QStringLiteral("post.font-policy"),
                          payload,
                          options.enforcePretendardFallback && !fallbackEnforced ? QtWarningMsg : QtInfoMsg);
    }
}

} // namespace lvrs
