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

    if (platform == kPlatformWindows())
        return QStringLiteral("d3d11");

    if (platform == kPlatformAndroid())
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

struct AdaptiveViewProfile {
    int wideBreakpoint = 980;
    int navWidth = 220;
    int navDrawerWidth = 240;
    int mobileDesktopMinWidth = 1200;
    int bottomNavigationMaxItems = 5;
    int compactSpacingBreakpoint = 900;
    double navRailMaxWidthRatio = 0.32;
    int drawerMarginSafety = 16;
    int drawerEnterDuration = 160;
    int drawerExitDuration = 120;
    bool enableAnimatedTransitions = true;
};

AdaptiveViewProfile adaptiveViewProfileForPlatform(const QString &platform, bool backendReady)
{
    AdaptiveViewProfile profile;
    profile.enableAnimatedTransitions = backendReady;

    if (platform == kPlatformIos()) {
        profile.wideBreakpoint = 948;
        profile.navWidth = 216;
        profile.navDrawerWidth = 264;
        profile.mobileDesktopMinWidth = 1180;
        profile.bottomNavigationMaxItems = 5;
        profile.compactSpacingBreakpoint = 860;
        profile.navRailMaxWidthRatio = 0.33;
        profile.drawerMarginSafety = 24;
        profile.drawerEnterDuration = 185;
        profile.drawerExitDuration = 145;
        return profile;
    }

    if (platform == kPlatformAndroid()) {
        profile.wideBreakpoint = 940;
        profile.navWidth = 208;
        profile.navDrawerWidth = 252;
        profile.mobileDesktopMinWidth = 1080;
        profile.bottomNavigationMaxItems = 4;
        profile.compactSpacingBreakpoint = 840;
        profile.navRailMaxWidthRatio = 0.34;
        profile.drawerMarginSafety = 20;
        profile.drawerEnterDuration = 170;
        profile.drawerExitDuration = 130;
        return profile;
    }

    if (platform == kPlatformMacos()) {
        profile.wideBreakpoint = 1040;
        profile.navWidth = 232;
        profile.navDrawerWidth = 256;
        profile.mobileDesktopMinWidth = 1320;
        profile.compactSpacingBreakpoint = 960;
        profile.navRailMaxWidthRatio = 0.30;
        profile.drawerEnterDuration = 180;
        profile.drawerExitDuration = 136;
        return profile;
    }

    if (platform == kPlatformWindows()) {
        profile.wideBreakpoint = 1000;
        profile.navWidth = 224;
        profile.navDrawerWidth = 248;
        profile.mobileDesktopMinWidth = 1240;
        profile.compactSpacingBreakpoint = 920;
        profile.navRailMaxWidthRatio = 0.31;
        profile.drawerEnterDuration = 155;
        profile.drawerExitDuration = 115;
        return profile;
    }

    if (platform == kPlatformLinux()) {
        profile.wideBreakpoint = 980;
        profile.navWidth = 220;
        profile.navDrawerWidth = 240;
        profile.mobileDesktopMinWidth = 1200;
        profile.compactSpacingBreakpoint = 900;
        profile.navRailMaxWidthRatio = 0.32;
        profile.drawerEnterDuration = 160;
        profile.drawerExitDuration = 120;
        return profile;
    }

    if (platform == kPlatformWasm()) {
        profile.wideBreakpoint = 960;
        profile.navWidth = 216;
        profile.navDrawerWidth = 236;
        profile.mobileDesktopMinWidth = 1120;
        profile.bottomNavigationMaxItems = 4;
        profile.compactSpacingBreakpoint = 880;
        profile.navRailMaxWidthRatio = 0.34;
        profile.enableAnimatedTransitions = false;
        profile.drawerEnterDuration = 140;
        profile.drawerExitDuration = 100;
        return profile;
    }

    return profile;
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
    const bool backendReady = known ? backendFeatureReady(backend) : false;
    const bool desktop = known && isDesktopPlatformToken(normalized);
    const bool mobile = known && isMobilePlatformToken(normalized);
    const bool android = known && normalized == kPlatformAndroid();
    const bool ios = known && normalized == kPlatformIos();
    const bool runtimeEventsAutoAttachRecommended = false;
    const bool mobileSystemWindowDelegationRecommended = mobile;
    const bool mobileSystemInsetsDelegationRecommended = mobile;
    const bool mobileDisplayCoverageOverrideRecommended = android;
    const bool mobileFullscreenVisibilityRecommended = android;
    const bool mobileFullscreenGeometryHintRecommended = android;
    int bootstrapMsaaSamples = 4;
    int bootstrapFramesInFlight = 2;
    bool bootstrapPartialUpdateRecommended = true;
    bool bootstrapBatchRenderingRecommended = true;
    bool bootstrapPipelineCacheRecommended = true;
    int bootstrapTextureAtlasEdge = 2048;
    const AdaptiveViewProfile adaptiveView = adaptiveViewProfileForPlatform(normalized, backendReady);

    if (android || ios) {
        bootstrapMsaaSamples = 2;
        bootstrapTextureAtlasEdge = 1024;
    } else if (normalized == kPlatformMacos()) {
        bootstrapFramesInFlight = 3;
    } else if (normalized == kPlatformWasm()) {
        bootstrapMsaaSamples = 2;
        bootstrapFramesInFlight = 1;
        bootstrapPartialUpdateRecommended = false;
        bootstrapBatchRenderingRecommended = false;
        bootstrapPipelineCacheRecommended = false;
        bootstrapTextureAtlasEdge = 1024;
    }

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
    profile.insert(QStringLiteral("backendFeatureReady"), backendReady);
    profile.insert(QStringLiteral("metalRequired"), known && backend == QStringLiteral("metal"));
    profile.insert(QStringLiteral("vulkanRequired"), known && backend == QStringLiteral("vulkan"));
    profile.insert(QStringLiteral("runtimeEventsAutoAttachRecommended"), runtimeEventsAutoAttachRecommended);
    profile.insert(QStringLiteral("mobileSystemWindowDelegationRecommended"), mobileSystemWindowDelegationRecommended);
    profile.insert(QStringLiteral("mobileSystemInsetsDelegationRecommended"), mobileSystemInsetsDelegationRecommended);
    profile.insert(QStringLiteral("mobileDisplayCoverageOverrideRecommended"), mobileDisplayCoverageOverrideRecommended);
    profile.insert(QStringLiteral("mobileFullscreenVisibilityRecommended"), mobileFullscreenVisibilityRecommended);
    profile.insert(QStringLiteral("mobileFullscreenGeometryHintRecommended"), mobileFullscreenGeometryHintRecommended);
    profile.insert(QStringLiteral("bootstrapMsaaSamples"), bootstrapMsaaSamples);
    profile.insert(QStringLiteral("bootstrapFramesInFlight"), bootstrapFramesInFlight);
    profile.insert(QStringLiteral("bootstrapPartialUpdateRecommended"), bootstrapPartialUpdateRecommended);
    profile.insert(QStringLiteral("bootstrapBatchRenderingRecommended"), bootstrapBatchRenderingRecommended);
    profile.insert(QStringLiteral("bootstrapPipelineCacheRecommended"), bootstrapPipelineCacheRecommended);
    profile.insert(QStringLiteral("bootstrapTextureAtlasEdge"), bootstrapTextureAtlasEdge);
    profile.insert(QStringLiteral("adaptiveWideBreakpoint"), adaptiveView.wideBreakpoint);
    profile.insert(QStringLiteral("adaptiveNavWidth"), adaptiveView.navWidth);
    profile.insert(QStringLiteral("adaptiveNavDrawerWidth"), adaptiveView.navDrawerWidth);
    profile.insert(QStringLiteral("adaptiveMobileDesktopMinWidth"), adaptiveView.mobileDesktopMinWidth);
    profile.insert(QStringLiteral("adaptiveBottomNavigationMaxItems"), adaptiveView.bottomNavigationMaxItems);
    profile.insert(QStringLiteral("adaptiveCompactSpacingBreakpoint"), adaptiveView.compactSpacingBreakpoint);
    profile.insert(QStringLiteral("adaptiveNavRailMaxWidthRatio"), adaptiveView.navRailMaxWidthRatio);
    profile.insert(QStringLiteral("adaptiveDrawerMarginSafety"), adaptiveView.drawerMarginSafety);
    profile.insert(QStringLiteral("adaptiveDrawerEnterDuration"), adaptiveView.drawerEnterDuration);
    profile.insert(QStringLiteral("adaptiveDrawerExitDuration"), adaptiveView.drawerExitDuration);
    profile.insert(QStringLiteral("adaptiveAnimatedTransitions"), adaptiveView.enableAnimatedTransitions);
    profile.insert(QStringLiteral("android"), android);
    profile.insert(QStringLiteral("ios"), ios);
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
