#include "backend/runtime/vulkanbootstrap.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLibrary>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSet>
#include <QStringList>
#include <QtGlobal>
#include <QtGui/qtgui-config.h>

namespace {

void configureGraphicsBackend(const QByteArray &backendName, QSGRendererInterface::GraphicsApi graphicsApi)
{
    if (backendName.isEmpty())
        qunsetenv("QSG_RHI_BACKEND");
    else
        qputenv("QSG_RHI_BACKEND", backendName);
    QQuickWindow::setGraphicsApi(graphicsApi);
}

lvrs::GraphicsBackendBootstrapResult makeGraphicsBackendResult(const QString &backendName,
                                                              const QString &loaderName = QString())
{
    lvrs::GraphicsBackendBootstrapResult result;
    result.available = true;
    result.backendName = backendName;
    result.loaderName = loaderName;
    return result;
}

lvrs::GraphicsBackendBootstrapResult makeGraphicsBackendFailure(const QString &errorMessage)
{
    lvrs::GraphicsBackendBootstrapResult result;
    result.errorMessage = errorMessage;
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

bool tryLoadVulkanRuntime(QString *resolvedLoaderName, QString *lastErrorMessage)
{
    const QStringList candidates = buildVulkanLoaderCandidates();
    QString lastError;
    for (const QString &candidate : candidates) {
        QLibrary loader(candidate);
        if (loader.load()) {
            loader.unload();
            qputenv("QT_VULKAN_LIB", candidate.toUtf8());
            if (resolvedLoaderName)
                *resolvedLoaderName = candidate;
            return true;
        }
        if (!loader.errorString().isEmpty())
            lastError = loader.errorString();
    }

    if (lastErrorMessage)
        *lastErrorMessage = lastError;
    return false;
}

lvrs::GraphicsBackendBootstrapResult bootstrapMetalBackend()
{
#if defined(QT_FEATURE_metal) && QT_FEATURE_metal > 0
    configureGraphicsBackend(QByteArrayLiteral("metal"), QSGRendererInterface::Metal);
    return makeGraphicsBackendResult(QStringLiteral("metal"));
#else
    return makeGraphicsBackendFailure(
        QStringLiteral("Metal backend is required on macOS/iOS, but this Qt build has no Metal support."));
#endif
}

lvrs::GraphicsBackendBootstrapResult bootstrapDesktopVulkanBackend()
{
#if defined(QT_FEATURE_vulkan) && QT_FEATURE_vulkan > 0
    QString lastError;
    QString loaderName;
    if (tryLoadVulkanRuntime(&loaderName, &lastError)) {
        configureGraphicsBackend(QByteArrayLiteral("vulkan"), QSGRendererInterface::Vulkan);
        return makeGraphicsBackendResult(QStringLiteral("vulkan"), loaderName);
    }

    QString errorMessage =
        QStringLiteral("Vulkan backend is required on this platform. Install Vulkan runtime and set QT_VULKAN_LIB appropriately.");
    if (!lastError.isEmpty())
        errorMessage += QStringLiteral(" Last loader error: %1").arg(lastError);
    return makeGraphicsBackendFailure(errorMessage);
#else
    return makeGraphicsBackendFailure(
        QStringLiteral("Vulkan backend is required on this platform, but this Qt build has no Vulkan support."));
#endif
}

lvrs::GraphicsBackendBootstrapResult bootstrapAndroidGraphicsBackend()
{
#if defined(QT_FEATURE_vulkan) && QT_FEATURE_vulkan > 0
    QString lastError;
    QString loaderName;
    if (tryLoadVulkanRuntime(&loaderName, &lastError)) {
        configureGraphicsBackend(QByteArrayLiteral("vulkan"), QSGRendererInterface::Vulkan);
        if (loaderName.isEmpty())
            loaderName = QStringLiteral("system");
        return makeGraphicsBackendResult(QStringLiteral("vulkan"), loaderName);
    }

    qWarning().noquote()
        << "LVRS Android graphics bootstrap: Vulkan probe failed; falling back to OpenGL."
        << (lastError.isEmpty() ? QString() : QStringLiteral("Reason: %1").arg(lastError));
#else
    qWarning().noquote()
        << "LVRS Android graphics bootstrap: Qt Vulkan support is unavailable; falling back to OpenGL.";
#endif

    configureGraphicsBackend(QByteArrayLiteral("opengl"), QSGRendererInterface::OpenGL);
    return makeGraphicsBackendResult(QStringLiteral("opengl"), QStringLiteral("android-fallback"));
}
}

namespace lvrs {
GraphicsBackendBootstrapResult bootstrapPreferredGraphicsBackend()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_IOS)
    return bootstrapMetalBackend();

#elif defined(Q_OS_ANDROID)
    return bootstrapAndroidGraphicsBackend();

#elif defined(Q_OS_WIN)
    return bootstrapDesktopVulkanBackend();

#else
    configureGraphicsBackend(QByteArray(), QSGRendererInterface::Unknown);
    return makeGraphicsBackendResult(QStringLiteral("default"));
#endif
}
}
