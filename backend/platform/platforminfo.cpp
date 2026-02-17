#include "backend/platform/platforminfo.h"

#include <QSysInfo>
#include <QtGui/qtgui-config.h>

namespace {

const QString &kPlatformMacos()
{
    static const QString value = QStringLiteral("macos");
    return value;
}

const QString &kPlatformLinux()
{
    static const QString value = QStringLiteral("linux");
    return value;
}

const QString &kPlatformWindows()
{
    static const QString value = QStringLiteral("windows");
    return value;
}

const QString &kPlatformIos()
{
    static const QString value = QStringLiteral("ios");
    return value;
}

const QString &kPlatformAndroid()
{
    static const QString value = QStringLiteral("android");
    return value;
}

const QString &kPlatformWasm()
{
    static const QString value = QStringLiteral("wasm");
    return value;
}

const QStringList &allRuntimeTargetList()
{
    static const QStringList values = {
        kPlatformMacos(),
        kPlatformLinux(),
        kPlatformWindows(),
        kPlatformIos(),
        kPlatformAndroid(),
        kPlatformWasm()
    };
    return values;
}

const QStringList &desktopRuntimeTargetList()
{
    static const QStringList values = {
        kPlatformMacos(),
        kPlatformLinux(),
        kPlatformWindows(),
        kPlatformWasm()
    };
    return values;
}

const QStringList &mobileRuntimeTargetList()
{
    static const QStringList values = {
        kPlatformIos(),
        kPlatformAndroid()
    };
    return values;
}

QString normalizePlatformToken(const QString &value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized.isEmpty())
        return {};

    if (normalized == QStringLiteral("osx")
        || normalized == QStringLiteral("mac")
        || normalized == QStringLiteral("darwin")
        || normalized == QStringLiteral("macosx")) {
        return kPlatformMacos();
    }

    if (normalized == QStringLiteral("win")
        || normalized == QStringLiteral("win32")
        || normalized == QStringLiteral("win64")
        || normalized == QStringLiteral("mingw")) {
        return kPlatformWindows();
    }

    if (normalized == QStringLiteral("gnu/linux")
        || normalized == QStringLiteral("ubuntu")
        || normalized == QStringLiteral("debian")
        || normalized == QStringLiteral("fedora")) {
        return kPlatformLinux();
    }

    if (normalized == QStringLiteral("iphoneos")
        || normalized == QStringLiteral("iphonesimulator")
        || normalized == QStringLiteral("ios-simulator")) {
        return kPlatformIos();
    }

    if (normalized == QStringLiteral("android-arm")
        || normalized == QStringLiteral("android-arm64")
        || normalized == QStringLiteral("android-x86")
        || normalized == QStringLiteral("android-x86_64")) {
        return kPlatformAndroid();
    }

    if (normalized == QStringLiteral("emscripten")
        || normalized == QStringLiteral("webassembly")
        || normalized == QStringLiteral("qtwasm")) {
        return kPlatformWasm();
    }

    return normalized;
}

bool isKnownPlatformToken(const QString &platform)
{
    return allRuntimeTargetList().contains(platform);
}

bool isMobilePlatformToken(const QString &platform)
{
    return mobileRuntimeTargetList().contains(platform);
}

bool isDesktopPlatformToken(const QString &platform)
{
    return desktopRuntimeTargetList().contains(platform);
}

QString currentCanonicalPlatform()
{
#if defined(Q_OS_ANDROID)
    return kPlatformAndroid();
#elif defined(Q_OS_IOS)
    return kPlatformIos();
#elif defined(Q_OS_WASM)
    return kPlatformWasm();
#elif defined(Q_OS_MACOS)
    return kPlatformMacos();
#elif defined(Q_OS_WIN)
    return kPlatformWindows();
#elif defined(Q_OS_LINUX)
    return kPlatformLinux();
#else
    return QStringLiteral("unknown");
#endif
}

QString legacyPlatformName()
{
#if defined(Q_OS_ANDROID)
    return kPlatformAndroid();
#elif defined(Q_OS_IOS)
    return kPlatformIos();
#elif defined(Q_OS_WASM)
    return kPlatformWasm();
#elif defined(Q_OS_MACOS)
    return QStringLiteral("osx");
#elif defined(Q_OS_WIN)
    return kPlatformWindows();
#elif defined(Q_OS_LINUX)
    return kPlatformLinux();
#else
    return QStringLiteral("unknown");
#endif
}

QString graphicsBackendForPlatformToken(const QString &platform)
{
    if (platform == kPlatformMacos() || platform == kPlatformIos())
        return QStringLiteral("metal");

    if (platform == kPlatformWindows() || platform == kPlatformAndroid())
        return QStringLiteral("vulkan");

    return QStringLiteral("default");
}

bool isMetalFeatureReady()
{
#if defined(QT_FEATURE_metal) && QT_FEATURE_metal > 0
    return true;
#else
    return false;
#endif
}

bool isVulkanFeatureReady()
{
#if defined(QT_FEATURE_vulkan) && QT_FEATURE_vulkan > 0
    return true;
#else
    return false;
#endif
}

bool backendFeatureReady(const QString &backendName)
{
    if (backendName == QStringLiteral("metal"))
        return isMetalFeatureReady();
    if (backendName == QStringLiteral("vulkan"))
        return isVulkanFeatureReady();
    return true;
}

QString cmakeSystemNameForPlatformToken(const QString &platform)
{
    if (platform == kPlatformMacos())
        return QStringLiteral("Darwin");
    if (platform == kPlatformLinux())
        return QStringLiteral("Linux");
    if (platform == kPlatformWindows())
        return QStringLiteral("Windows");
    if (platform == kPlatformIos())
        return QStringLiteral("iOS");
    if (platform == kPlatformAndroid())
        return QStringLiteral("Android");
    if (platform == kPlatformWasm())
        return QStringLiteral("Emscripten");
    return QStringLiteral("Unknown");
}

QString executableSuffixForPlatformToken(const QString &platform)
{
    if (platform == kPlatformWindows())
        return QStringLiteral(".exe");
    if (platform == kPlatformWasm())
        return QStringLiteral(".html");
    return {};
}

QString sharedLibrarySuffixForPlatformToken(const QString &platform)
{
    if (platform == kPlatformWindows())
        return QStringLiteral(".dll");
    if (platform == kPlatformMacos() || platform == kPlatformIos())
        return QStringLiteral(".dylib");
    if (platform == kPlatformLinux() || platform == kPlatformAndroid())
        return QStringLiteral(".so");
    return {};
}

QVariantMap buildRuntimeProfile(const QString &requested, const QString &hostCanonical)
{
    const QString normalized = normalizePlatformToken(requested.isEmpty() ? hostCanonical : requested);
    const bool known = isKnownPlatformToken(normalized);
    const QString backend = known ? graphicsBackendForPlatformToken(normalized) : QStringLiteral("default");
    const bool desktop = known && isDesktopPlatformToken(normalized);
    const bool mobile = known && isMobilePlatformToken(normalized);

    QVariantMap profile;
    profile.insert(QStringLiteral("requested"), requested);
    profile.insert(QStringLiteral("target"), known ? normalized : QStringLiteral("unknown"));
    profile.insert(QStringLiteral("known"), known);
    profile.insert(QStringLiteral("host"), hostCanonical);
    profile.insert(QStringLiteral("current"), known && normalized == hostCanonical);
    profile.insert(QStringLiteral("desktop"), desktop);
    profile.insert(QStringLiteral("mobile"), mobile);
    profile.insert(QStringLiteral("backend"), backend);
    profile.insert(QStringLiteral("generationSupported"), known);
    profile.insert(QStringLiteral("backendFeatureReady"), known ? backendFeatureReady(backend) : false);
    profile.insert(QStringLiteral("metalRequired"), known && backend == QStringLiteral("metal"));
    profile.insert(QStringLiteral("vulkanRequired"), known && backend == QStringLiteral("vulkan"));
    profile.insert(QStringLiteral("cmakeSystemName"), known ? cmakeSystemNameForPlatformToken(normalized) : QStringLiteral("Unknown"));
    profile.insert(QStringLiteral("executableSuffix"), known ? executableSuffixForPlatformToken(normalized) : QString());
    profile.insert(QStringLiteral("sharedLibrarySuffix"), known ? sharedLibrarySuffixForPlatformToken(normalized) : QString());
    profile.insert(QStringLiteral("directRunSupported"), desktop && normalized != kPlatformWasm());
    return profile;
}

} // namespace

PlatformInfo::PlatformInfo(QObject *parent)
    : QObject(parent)
{
}

QString PlatformInfo::os() const
{
    return legacyPlatformName();
}

QString PlatformInfo::canonicalOs() const
{
    return currentCanonicalPlatform();
}

QString PlatformInfo::arch() const
{
    return QSysInfo::currentCpuArchitecture();
}

QString PlatformInfo::graphicsBackend() const
{
    return graphicsBackendForPlatformToken(canonicalOs());
}

bool PlatformInfo::mobile() const
{
    return isMobilePlatformToken(canonicalOs());
}

bool PlatformInfo::desktop() const
{
    return isDesktopPlatformToken(canonicalOs());
}

bool PlatformInfo::android() const
{
#if defined(Q_OS_ANDROID)
    return true;
#else
    return false;
#endif
}

bool PlatformInfo::ios() const
{
#if defined(Q_OS_IOS)
    return true;
#else
    return false;
#endif
}

bool PlatformInfo::macos() const
{
#if defined(Q_OS_MACOS)
    return true;
#else
    return false;
#endif
}

bool PlatformInfo::windows() const
{
#if defined(Q_OS_WIN)
    return true;
#else
    return false;
#endif
}

bool PlatformInfo::linux() const
{
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    return true;
#else
    return false;
#endif
}

bool PlatformInfo::wasm() const
{
#if defined(Q_OS_WASM)
    return true;
#else
    return false;
#endif
}

bool PlatformInfo::metalSupported() const
{
    return isMetalFeatureReady();
}

bool PlatformInfo::vulkanSupported() const
{
    return isVulkanFeatureReady();
}

QStringList PlatformInfo::runtimeTargets() const
{
    return allRuntimeTargetList();
}

QStringList PlatformInfo::desktopTargets() const
{
    return desktopRuntimeTargetList();
}

QStringList PlatformInfo::mobileTargets() const
{
    return mobileRuntimeTargetList();
}

QVariantList PlatformInfo::runtimeProfiles() const
{
    QVariantList profiles;
    profiles.reserve(allRuntimeTargetList().size());
    for (const QString &target : allRuntimeTargetList())
        profiles.append(runtimeProfile(target));
    return profiles;
}

QString PlatformInfo::normalizeTarget(const QString &target) const
{
    const QString normalized = normalizePlatformToken(target);
    if (!isKnownPlatformToken(normalized))
        return {};
    return normalized;
}

bool PlatformInfo::isKnownTarget(const QString &target) const
{
    return isKnownPlatformToken(normalizePlatformToken(target));
}

bool PlatformInfo::targetMatchesCurrent(const QString &target) const
{
    const QString normalized = normalizePlatformToken(target);
    return isKnownPlatformToken(normalized) && normalized == canonicalOs();
}

bool PlatformInfo::targetIsMobile(const QString &target) const
{
    return isMobilePlatformToken(normalizePlatformToken(target));
}

bool PlatformInfo::targetIsDesktop(const QString &target) const
{
    return isDesktopPlatformToken(normalizePlatformToken(target));
}

bool PlatformInfo::supportsTargetGeneration(const QString &target) const
{
    return isKnownTarget(target);
}

bool PlatformInfo::backendFeatureReadyFor(const QString &target) const
{
    const QString normalized = normalizePlatformToken(target);
    if (!isKnownPlatformToken(normalized))
        return false;
    return backendFeatureReady(graphicsBackendForPlatformToken(normalized));
}

QString PlatformInfo::graphicsBackendFor(const QString &target) const
{
    const QString normalized = normalizePlatformToken(target);
    if (target.trimmed().isEmpty())
        return graphicsBackendForPlatformToken(canonicalOs());
    if (!isKnownPlatformToken(normalized))
        return QStringLiteral("default");
    return graphicsBackendForPlatformToken(normalized);
}

QVariantMap PlatformInfo::runtimeProfile(const QString &target) const
{
    return buildRuntimeProfile(target.trimmed(), canonicalOs());
}
