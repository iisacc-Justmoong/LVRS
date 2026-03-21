#include "backend/runtime/vulkanbootstrap.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLibrary>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSet>
#include <QStringList>
#include <QVariantMap>
#include <QtGlobal>
#include <QtGui/qtgui-config.h>

namespace {

struct RuntimeLoaderProbeResult {
    bool available = false;
    QString resolvedLoaderName;
    QString lastErrorMessage;
    QStringList candidates;
};

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

void configureGraphicsBackend(const QByteArray &backendName, QSGRendererInterface::GraphicsApi graphicsApi)
{
    if (backendName.isEmpty())
        qunsetenv("QSG_RHI_BACKEND");
    else
        qputenv("QSG_RHI_BACKEND", backendName);
    QQuickWindow::setGraphicsApi(graphicsApi);
}

lvrs::GraphicsBackendBootstrapResult makeGraphicsBackendResult(const QString &backendName,
                                                              const QString &loaderName = QString(),
                                                              const QString &requestedBackendName = QString(),
                                                              bool fallbackUsed = false,
                                                              const QString &fallbackReason = QString(),
                                                              const QStringList &probeCandidates = {})
{
    lvrs::GraphicsBackendBootstrapResult result;
    result.available = true;
    result.backendName = backendName;
    result.loaderName = loaderName;
    result.requestedBackendName = requestedBackendName;
    result.fallbackUsed = fallbackUsed;
    result.fallbackReason = fallbackReason;
    result.probeCandidates = probeCandidates;
    return result;
}

lvrs::GraphicsBackendBootstrapResult makeGraphicsBackendFailure(const QString &errorMessage,
                                                               const QString &requestedBackendName = QString(),
                                                               const QStringList &probeCandidates = {})
{
    lvrs::GraphicsBackendBootstrapResult result;
    result.errorMessage = errorMessage;
    result.requestedBackendName = requestedBackendName;
    result.probeCandidates = probeCandidates;
    return result;
}

void appendIfExists(QStringList &candidates, const QString &path)
{
    if (path.isEmpty())
        return;
    if (QFileInfo::exists(path))
        candidates.append(path);
}

void appendCellarCandidates(QStringList &candidates, const QString &cellarRoot)
{
    QDir cellarDir(cellarRoot);
    if (!cellarDir.exists())
        return;

    const QStringList versions = cellarDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (auto it = versions.crbegin(); it != versions.crend(); ++it)
        appendIfExists(candidates, cellarDir.filePath(*it + "/lib/libMoltenVK.dylib"));
}

QStringList buildVulkanLoaderCandidates()
{
    QStringList candidates;

    const QString envVulkanLib = qEnvironmentVariable("QT_VULKAN_LIB");
    if (!envVulkanLib.isEmpty())
        candidates.append(envVulkanLib);

#if defined(Q_OS_MACOS)
    const QString brewPrefix = qEnvironmentVariable("HOMEBREW_PREFIX");
    if (!brewPrefix.isEmpty()) {
        appendIfExists(candidates, brewPrefix + "/lib/libMoltenVK.dylib");
        appendIfExists(candidates, brewPrefix + "/opt/molten-vk/lib/libMoltenVK.dylib");
        appendCellarCandidates(candidates, brewPrefix + "/Cellar/molten-vk");
    }

    appendIfExists(candidates, "/opt/homebrew/lib/libMoltenVK.dylib");
    appendIfExists(candidates, "/opt/homebrew/opt/molten-vk/lib/libMoltenVK.dylib");
    appendCellarCandidates(candidates, "/opt/homebrew/Cellar/molten-vk");

    appendIfExists(candidates, "/usr/local/lib/libMoltenVK.dylib");
    appendIfExists(candidates, "/usr/local/opt/molten-vk/lib/libMoltenVK.dylib");
    appendCellarCandidates(candidates, "/usr/local/Cellar/molten-vk");

    candidates.append("libMoltenVK.dylib");
    candidates.append("MoltenVK");
    candidates.append("vulkan");
    candidates.append("libvulkan.1");
    candidates.append("libMoltenVK");
#elif defined(Q_OS_ANDROID)
    candidates.append("libvulkan.so");
    candidates.append("libvulkan.so.1");
    candidates.append("vulkan");
#elif defined(Q_OS_WIN)
    candidates.append("vulkan-1");
#else
    candidates.append("vulkan");
    candidates.append("libvulkan.so");
    candidates.append("libvulkan.so.1");
#endif

    QStringList unique;
    QSet<QString> seen;
    for (const QString &candidate : candidates) {
        if (candidate.isEmpty())
            continue;
        if (seen.contains(candidate))
            continue;
        seen.insert(candidate);
        unique.append(candidate);
    }
    return unique;
}

RuntimeLoaderProbeResult probeVulkanRuntime()
{
    RuntimeLoaderProbeResult result;
    result.candidates = buildVulkanLoaderCandidates();
    for (const QString &candidate : result.candidates) {
        QLibrary loader(candidate);
        if (loader.load()) {
            loader.unload();
            qputenv("QT_VULKAN_LIB", candidate.toUtf8());
            result.available = true;
            result.resolvedLoaderName = candidate;
            return result;
        }
        if (!loader.errorString().isEmpty())
            result.lastErrorMessage = loader.errorString();
    }
    return result;
}

QStringList buildD3D11LoaderCandidates()
{
    QStringList candidates;
    candidates.append(QStringLiteral("d3d11"));
    candidates.append(QStringLiteral("d3d11.dll"));

    QStringList unique;
    QSet<QString> seen;
    for (const QString &candidate : candidates) {
        if (candidate.isEmpty())
            continue;
        if (seen.contains(candidate))
            continue;
        seen.insert(candidate);
        unique.append(candidate);
    }
    return unique;
}

RuntimeLoaderProbeResult probeD3D11Runtime()
{
    RuntimeLoaderProbeResult result;
    result.candidates = buildD3D11LoaderCandidates();
    for (const QString &candidate : result.candidates) {
        QLibrary loader(candidate);
        if (!loader.load()) {
            if (!loader.errorString().isEmpty())
                result.lastErrorMessage = loader.errorString();
            continue;
        }

        if (loader.resolve("D3D11CreateDevice")) {
            loader.unload();
            result.available = true;
            result.resolvedLoaderName = candidate;
            return result;
        }

        result.lastErrorMessage = QStringLiteral("D3D11CreateDevice symbol is unavailable in %1").arg(candidate);
        loader.unload();
    }
    return result;
}

lvrs::GraphicsBackendBootstrapResult bootstrapMetalBackend(bool logDiagnostics)
{
#if defined(QT_FEATURE_metal) && QT_FEATURE_metal > 0
    if (logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requestedBackend"), QStringLiteral("metal"));
        payload.insert(QStringLiteral("selectedBackend"), QStringLiteral("metal"));
        payload.insert(QStringLiteral("fallbackUsed"), false);
        payload.insert(QStringLiteral("qtFeatureMetal"), true);
        logBootstrapEvent(QStringLiteral("graphics.selected"), payload);
    }
    configureGraphicsBackend(QByteArrayLiteral("metal"), QSGRendererInterface::Metal);
    return makeGraphicsBackendResult(QStringLiteral("metal"),
                                     QString(),
                                     QStringLiteral("metal"));
#else
    const QString errorMessage =
        QStringLiteral("Metal backend is required on macOS/iOS, but this Qt build has no Metal support.");
    if (logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requestedBackend"), QStringLiteral("metal"));
        payload.insert(QStringLiteral("selectedBackend"), QString());
        payload.insert(QStringLiteral("fallbackUsed"), false);
        payload.insert(QStringLiteral("qtFeatureMetal"), false);
        payload.insert(QStringLiteral("error"), errorMessage);
        logBootstrapEvent(QStringLiteral("graphics.failure"), payload, QtWarningMsg);
    }
    return makeGraphicsBackendFailure(errorMessage, QStringLiteral("metal"));
#endif
}

lvrs::GraphicsBackendBootstrapResult bootstrapWindowsGraphicsBackend(bool logDiagnostics)
{
#if defined(Q_OS_WIN)
    const RuntimeLoaderProbeResult probe = probeD3D11Runtime();
    if (logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requestedBackend"), QStringLiteral("d3d11"));
        payload.insert(QStringLiteral("candidateCount"), probe.candidates.size());
        payload.insert(QStringLiteral("candidates"), probe.candidates);
        logBootstrapEvent(QStringLiteral("graphics.probe"), payload);
    }

    if (probe.available) {
        QString loaderName = probe.resolvedLoaderName;
        configureGraphicsBackend(QByteArrayLiteral("d3d11"), QSGRendererInterface::Direct3D11);
        if (loaderName.isEmpty())
            loaderName = QStringLiteral("system");
        if (logDiagnostics) {
            QVariantMap payload;
            payload.insert(QStringLiteral("requestedBackend"), QStringLiteral("d3d11"));
            payload.insert(QStringLiteral("selectedBackend"), QStringLiteral("d3d11"));
            payload.insert(QStringLiteral("loader"), loaderName);
            payload.insert(QStringLiteral("fallbackUsed"), false);
            payload.insert(QStringLiteral("candidateCount"), probe.candidates.size());
            logBootstrapEvent(QStringLiteral("graphics.selected"), payload);
        }
        return makeGraphicsBackendResult(QStringLiteral("d3d11"),
                                         loaderName,
                                         QStringLiteral("d3d11"),
                                         false,
                                         QString(),
                                         probe.candidates);
    }
    if (logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requestedBackend"), QStringLiteral("d3d11"));
        payload.insert(QStringLiteral("selectedBackend"), QStringLiteral("opengl"));
        payload.insert(QStringLiteral("loader"), QStringLiteral("windows-fallback"));
        payload.insert(QStringLiteral("fallbackUsed"), true);
        payload.insert(QStringLiteral("reason"), probe.lastErrorMessage);
        payload.insert(QStringLiteral("candidateCount"), probe.candidates.size());
        payload.insert(QStringLiteral("candidates"), probe.candidates);
        logBootstrapEvent(QStringLiteral("graphics.fallback"), payload, QtWarningMsg);
    }

    configureGraphicsBackend(QByteArrayLiteral("opengl"), QSGRendererInterface::OpenGL);
    return makeGraphicsBackendResult(QStringLiteral("opengl"),
                                     QStringLiteral("windows-fallback"),
                                     QStringLiteral("d3d11"),
                                     true,
                                     probe.lastErrorMessage,
                                     probe.candidates);
#else
    return makeGraphicsBackendFailure(
        QStringLiteral("Direct3D11 bootstrap is only supported on Windows targets."),
        QStringLiteral("d3d11"));
#endif
}

lvrs::GraphicsBackendBootstrapResult bootstrapAndroidGraphicsBackend(bool logDiagnostics)
{
#if defined(QT_FEATURE_vulkan) && QT_FEATURE_vulkan > 0
    const RuntimeLoaderProbeResult probe = probeVulkanRuntime();
    if (logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requestedBackend"), QStringLiteral("vulkan"));
        payload.insert(QStringLiteral("candidateCount"), probe.candidates.size());
        payload.insert(QStringLiteral("candidates"), probe.candidates);
        payload.insert(QStringLiteral("qtFeatureVulkan"), true);
        logBootstrapEvent(QStringLiteral("graphics.probe"), payload);
    }

    if (probe.available) {
        QString loaderName = probe.resolvedLoaderName;
        configureGraphicsBackend(QByteArrayLiteral("vulkan"), QSGRendererInterface::Vulkan);
        if (loaderName.isEmpty())
            loaderName = QStringLiteral("system");
        if (logDiagnostics) {
            QVariantMap payload;
            payload.insert(QStringLiteral("requestedBackend"), QStringLiteral("vulkan"));
            payload.insert(QStringLiteral("selectedBackend"), QStringLiteral("vulkan"));
            payload.insert(QStringLiteral("loader"), loaderName);
            payload.insert(QStringLiteral("fallbackUsed"), false);
            payload.insert(QStringLiteral("candidateCount"), probe.candidates.size());
            logBootstrapEvent(QStringLiteral("graphics.selected"), payload);
        }
        return makeGraphicsBackendResult(QStringLiteral("vulkan"),
                                         loaderName,
                                         QStringLiteral("vulkan"),
                                         false,
                                         QString(),
                                         probe.candidates);
    }
    if (logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requestedBackend"), QStringLiteral("vulkan"));
        payload.insert(QStringLiteral("selectedBackend"), QStringLiteral("opengl"));
        payload.insert(QStringLiteral("loader"), QStringLiteral("android-fallback"));
        payload.insert(QStringLiteral("fallbackUsed"), true);
        payload.insert(QStringLiteral("reason"), probe.lastErrorMessage);
        payload.insert(QStringLiteral("candidateCount"), probe.candidates.size());
        payload.insert(QStringLiteral("candidates"), probe.candidates);
        payload.insert(QStringLiteral("qtFeatureVulkan"), true);
        logBootstrapEvent(QStringLiteral("graphics.fallback"), payload, QtWarningMsg);
    }
#else
    if (logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requestedBackend"), QStringLiteral("vulkan"));
        payload.insert(QStringLiteral("selectedBackend"), QStringLiteral("opengl"));
        payload.insert(QStringLiteral("loader"), QStringLiteral("android-fallback"));
        payload.insert(QStringLiteral("fallbackUsed"), true);
        payload.insert(QStringLiteral("reason"),
                       QStringLiteral("Qt Vulkan support is unavailable in this build."));
        payload.insert(QStringLiteral("candidateCount"), 0);
        payload.insert(QStringLiteral("candidates"), QStringList());
        payload.insert(QStringLiteral("qtFeatureVulkan"), false);
        logBootstrapEvent(QStringLiteral("graphics.fallback"), payload, QtWarningMsg);
    }
#endif

    configureGraphicsBackend(QByteArrayLiteral("opengl"), QSGRendererInterface::OpenGL);
    return makeGraphicsBackendResult(QStringLiteral("opengl"),
                                     QStringLiteral("android-fallback"),
                                     QStringLiteral("vulkan"),
                                     true,
#if defined(QT_FEATURE_vulkan) && QT_FEATURE_vulkan > 0
                                     probe.lastErrorMessage,
                                     probe.candidates
#else
                                     QStringLiteral("Qt Vulkan support is unavailable in this build."),
                                     {}
#endif
    );
}
}

namespace lvrs {
GraphicsBackendBootstrapResult bootstrapPreferredGraphicsBackend(bool logDiagnostics)
{
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    return bootstrapMetalBackend(logDiagnostics);

#elif defined(Q_OS_ANDROID)
    return bootstrapAndroidGraphicsBackend(logDiagnostics);

#elif defined(Q_OS_WIN)
    return bootstrapWindowsGraphicsBackend(logDiagnostics);

#else
    if (logDiagnostics) {
        QVariantMap payload;
        payload.insert(QStringLiteral("requestedBackend"), QStringLiteral("default"));
        payload.insert(QStringLiteral("selectedBackend"), QStringLiteral("default"));
        payload.insert(QStringLiteral("fallbackUsed"), false);
        logBootstrapEvent(QStringLiteral("graphics.selected"), payload);
    }
    configureGraphicsBackend(QByteArray(), QSGRendererInterface::Unknown);
    return makeGraphicsBackendResult(QStringLiteral("default"),
                                     QString(),
                                     QStringLiteral("default"));
#endif
}
}
