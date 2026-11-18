#include "QtEventWait.h"
#include <QGuiApplication>
#include <type_traits>

#include <QCursor>
#include <QDir>
#include <QFileInfo>
#include <QMouseEvent>
#include <QScopedPointer>
#include <QQmlEngine>
#include <QWindow>
#include <QtPlugin>

#include "backend/platform/nativewindowinteraction.h"
#include "backend/platform/nativewindowstyle.h"
#include "backend/platform/platforminfo.h"
#include "backend/platform/windowsafeareaobserver.h"
#include "test_utils.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

namespace {
template <typename Actual, typename Expected>
bool platformEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual) + 1.0, qreal(expected) + 1.0);
    else
        return actual == expected;
}
}
#define PLATFORM_COMPARE(actual, expected) CTEST_CHECK(platformEqual((actual), (expected)))

class PlatformIntegrationTests
{
public:
    void safe_area_tracks_window_lifetime();
    void platform_flags_consistency();
    void platform_runtime_profiles_are_exposed();
    void application_window_and_main_metrics_are_exposed();
    void mobile_theme_scale_contract();
    void native_window_interaction_resizes_manually_when_system_resize_is_unavailable();
    void native_window_interaction_validates_system_resize_edges();
    void native_window_style_mobile_coverage_flags();
};

void PlatformIntegrationTests::safe_area_tracks_window_lifetime()
{
    WindowSafeAreaObserver observer;
    CTEST_CHECK(!observer.resolved());
    PLATFORM_COMPARE(observer.bottomCornerRadius(), -1.0);
    QWindow window;
    window.resize(400, 700);
    observer.setWindow(&window);
    window.show();
    CTEST_WAIT_DEFAULT(observer.resolved());
    CTEST_CHECK(observer.leftInset() >= 0 && observer.topInset() >= 0);
    CTEST_CHECK(observer.rightInset() >= 0 && observer.bottomInset() >= 0);
    CTEST_CHECK(observer.topInset() + observer.bottomInset() < window.height());
    CTEST_CHECK(observer.bottomCornerRadius() >= -1.0);
#ifndef Q_OS_ANDROID
    PLATFORM_COMPARE(observer.bottomCornerRadius(), -1.0);
#endif
    window.resize(700, 400);
    QCoreApplication::processEvents();
    CTEST_CHECK(observer.topInset() + observer.bottomInset() < window.height());
    observer.setWindow(nullptr);
    CTEST_CHECK(!observer.resolved());
    PLATFORM_COMPARE(observer.topInset(), 0);
    PLATFORM_COMPARE(observer.bottomInset(), 0);
    PLATFORM_COMPARE(observer.bottomCornerRadius(), -1.0);
}

void PlatformIntegrationTests::platform_flags_consistency()
{
    PlatformInfo platform;
    CTEST_CHECK(!platform.os().isEmpty());
    CTEST_CHECK(!platform.arch().isEmpty());
    CTEST_CHECK(platform.desktop() || platform.mobile());

    const int explicitPlatformCount = (platform.android() ? 1 : 0)
        + (platform.ios() ? 1 : 0)
        + (platform.macos() ? 1 : 0)
        + (platform.windows() ? 1 : 0)
        + (platform.linux() ? 1 : 0)
        + (platform.wasm() ? 1 : 0);
    PLATFORM_COMPARE(explicitPlatformCount, 1);

    if (platform.macos())
        PLATFORM_COMPARE(platform.os(), QStringLiteral("osx"));
    if (platform.windows())
        PLATFORM_COMPARE(platform.os(), QStringLiteral("windows"));
    if (platform.linux())
        PLATFORM_COMPARE(platform.os(), QStringLiteral("linux"));
}

void PlatformIntegrationTests::platform_runtime_profiles_are_exposed()
{
    PlatformInfo platform;

    const QStringList expectedTargets = {
        QStringLiteral("macos"),
        QStringLiteral("linux"),
        QStringLiteral("windows"),
        QStringLiteral("ios"),
        QStringLiteral("android"),
        QStringLiteral("wasm")
    };
    PLATFORM_COMPARE(platform.runtimeTargets(), expectedTargets);
    PLATFORM_COMPARE(platform.desktopTargets(),
             QStringList({QStringLiteral("macos"), QStringLiteral("linux"), QStringLiteral("windows"), QStringLiteral("wasm")}));
    PLATFORM_COMPARE(platform.mobileTargets(), QStringList({QStringLiteral("ios"), QStringLiteral("android")}));

    CTEST_CHECK(expectedTargets.contains(platform.canonicalOs()));
    CTEST_CHECK(!platform.graphicsBackend().isEmpty());
    CTEST_CHECK(platform.targetMatchesCurrent(platform.canonicalOs()));
    CTEST_CHECK(platform.supportsTargetGeneration(platform.canonicalOs()));

    PLATFORM_COMPARE(platform.normalizeTarget(QStringLiteral("osx")), QStringLiteral("macos"));
    PLATFORM_COMPARE(platform.normalizeTarget(QStringLiteral("win32")), QStringLiteral("windows"));
    CTEST_CHECK(platform.normalizeTarget(QStringLiteral("unknown-target")).isEmpty());
    CTEST_CHECK(!platform.isKnownTarget(QStringLiteral("unknown-target")));

    const QVariantMap currentProfile = platform.runtimeProfile();
    CTEST_CHECK(currentProfile.value(QStringLiteral("known")).toBool());
    PLATFORM_COMPARE(currentProfile.value(QStringLiteral("target")).toString(), platform.canonicalOs());
    CTEST_CHECK(currentProfile.contains(QStringLiteral("backend")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("generationSupported")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("backendFeatureReady")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("cmakeSystemName")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("runtimeEventsAutoAttachRecommended")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("mobileSystemWindowDelegationRecommended")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("mobileSystemInsetsDelegationRecommended")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("mobileDisplayCoverageOverrideRecommended")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("mobileFullscreenVisibilityRecommended")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("mobileFullscreenGeometryHintRecommended")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("bootstrapMsaaSamples")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("bootstrapFramesInFlight")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("bootstrapPartialUpdateRecommended")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("bootstrapBatchRenderingRecommended")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("bootstrapPipelineCacheRecommended")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("bootstrapTextureAtlasEdge")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveWideBreakpoint")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveNavWidth")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveNavDrawerWidth")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveMobileDesktopMinWidth")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveBottomNavigationMaxItems")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveCompactSpacingBreakpoint")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveNavRailMaxWidthRatio")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveDrawerMarginSafety")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveDrawerEnterDuration")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveDrawerExitDuration")));
    CTEST_CHECK(currentProfile.contains(QStringLiteral("adaptiveAnimatedTransitions")));

    const QVariantList allProfiles = platform.runtimeProfiles();
    PLATFORM_COMPARE(allProfiles.size(), expectedTargets.size());
    for (const QVariant &profileVariant : allProfiles) {
        const QVariantMap profile = profileVariant.toMap();
        CTEST_CHECK(profile.value(QStringLiteral("known")).toBool());
        CTEST_CHECK(expectedTargets.contains(profile.value(QStringLiteral("target")).toString()));
        CTEST_CHECK(profile.contains(QStringLiteral("backend")));
        CTEST_CHECK(profile.contains(QStringLiteral("directRunSupported")));
    }

    const QVariantMap iosProfile = platform.runtimeProfile(QStringLiteral("ios"));
    CTEST_CHECK(iosProfile.value(QStringLiteral("mobile")).toBool());
    CTEST_CHECK(iosProfile.value(QStringLiteral("ios")).toBool());
    CTEST_CHECK(!iosProfile.value(QStringLiteral("android")).toBool());
    CTEST_CHECK(!iosProfile.value(QStringLiteral("runtimeEventsAutoAttachRecommended")).toBool());
    CTEST_CHECK(!iosProfile.value(QStringLiteral("mobileSystemWindowDelegationRecommended")).toBool());
    CTEST_CHECK(!iosProfile.value(QStringLiteral("mobileSystemInsetsDelegationRecommended")).toBool());
    CTEST_CHECK(iosProfile.value(QStringLiteral("mobileDisplayCoverageOverrideRecommended")).toBool());
    CTEST_CHECK(iosProfile.value(QStringLiteral("mobileFullscreenVisibilityRecommended")).toBool());
    CTEST_CHECK(iosProfile.value(QStringLiteral("mobileFullscreenGeometryHintRecommended")).toBool());
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("bootstrapMsaaSamples")).toInt(), 4);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("bootstrapFramesInFlight")).toInt(), 2);
    CTEST_CHECK(iosProfile.value(QStringLiteral("bootstrapPartialUpdateRecommended")).toBool());
    CTEST_CHECK(iosProfile.value(QStringLiteral("bootstrapBatchRenderingRecommended")).toBool());
    CTEST_CHECK(iosProfile.value(QStringLiteral("bootstrapPipelineCacheRecommended")).toBool());
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("bootstrapTextureAtlasEdge")).toInt(), 1024);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveWideBreakpoint")).toInt(), 948);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveNavWidth")).toInt(), 216);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveNavDrawerWidth")).toInt(), 264);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveMobileDesktopMinWidth")).toInt(), 1180);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveBottomNavigationMaxItems")).toInt(), 5);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveCompactSpacingBreakpoint")).toInt(), 860);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveNavRailMaxWidthRatio")).toDouble(), 0.33);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveDrawerMarginSafety")).toInt(), 24);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveDrawerEnterDuration")).toInt(), 185);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveDrawerExitDuration")).toInt(), 145);
    PLATFORM_COMPARE(iosProfile.value(QStringLiteral("adaptiveAnimatedTransitions")).toBool(),
             iosProfile.value(QStringLiteral("backendFeatureReady")).toBool());

    const QVariantMap androidProfile = platform.runtimeProfile(QStringLiteral("android"));
    CTEST_CHECK(androidProfile.value(QStringLiteral("mobile")).toBool());
    CTEST_CHECK(androidProfile.value(QStringLiteral("android")).toBool());
    CTEST_CHECK(!androidProfile.value(QStringLiteral("ios")).toBool());
    CTEST_CHECK(!androidProfile.value(QStringLiteral("runtimeEventsAutoAttachRecommended")).toBool());
    CTEST_CHECK(androidProfile.value(QStringLiteral("mobileSystemWindowDelegationRecommended")).toBool());
    CTEST_CHECK(androidProfile.value(QStringLiteral("mobileSystemInsetsDelegationRecommended")).toBool());
    CTEST_CHECK(androidProfile.value(QStringLiteral("mobileDisplayCoverageOverrideRecommended")).toBool());
    CTEST_CHECK(androidProfile.value(QStringLiteral("mobileFullscreenVisibilityRecommended")).toBool());
    CTEST_CHECK(androidProfile.value(QStringLiteral("mobileFullscreenGeometryHintRecommended")).toBool());
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("bootstrapMsaaSamples")).toInt(), 4);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("bootstrapFramesInFlight")).toInt(), 2);
    CTEST_CHECK(androidProfile.value(QStringLiteral("bootstrapPartialUpdateRecommended")).toBool());
    CTEST_CHECK(androidProfile.value(QStringLiteral("bootstrapBatchRenderingRecommended")).toBool());
    CTEST_CHECK(androidProfile.value(QStringLiteral("bootstrapPipelineCacheRecommended")).toBool());
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("bootstrapTextureAtlasEdge")).toInt(), 1024);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveWideBreakpoint")).toInt(), 940);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveNavWidth")).toInt(), 208);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveNavDrawerWidth")).toInt(), 252);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveMobileDesktopMinWidth")).toInt(), 1080);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveBottomNavigationMaxItems")).toInt(), 4);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveCompactSpacingBreakpoint")).toInt(), 840);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveNavRailMaxWidthRatio")).toDouble(), 0.34);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveDrawerMarginSafety")).toInt(), 20);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveDrawerEnterDuration")).toInt(), 170);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveDrawerExitDuration")).toInt(), 130);
    PLATFORM_COMPARE(androidProfile.value(QStringLiteral("adaptiveAnimatedTransitions")).toBool(),
             androidProfile.value(QStringLiteral("backendFeatureReady")).toBool());

    const QVariantMap windowsProfile = platform.runtimeProfile(QStringLiteral("windows"));
    CTEST_CHECK(windowsProfile.value(QStringLiteral("desktop")).toBool());
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("backend")).toString(), QStringLiteral("d3d11"));
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("bootstrapMsaaSamples")).toInt(), 4);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("bootstrapFramesInFlight")).toInt(), 2);
    CTEST_CHECK(windowsProfile.value(QStringLiteral("bootstrapPartialUpdateRecommended")).toBool());
    CTEST_CHECK(windowsProfile.value(QStringLiteral("bootstrapBatchRenderingRecommended")).toBool());
    CTEST_CHECK(windowsProfile.value(QStringLiteral("bootstrapPipelineCacheRecommended")).toBool());
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("bootstrapTextureAtlasEdge")).toInt(), 2048);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("adaptiveWideBreakpoint")).toInt(), 1000);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("adaptiveNavWidth")).toInt(), 224);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("adaptiveNavDrawerWidth")).toInt(), 248);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("adaptiveMobileDesktopMinWidth")).toInt(), 1240);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("adaptiveBottomNavigationMaxItems")).toInt(), 5);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("adaptiveCompactSpacingBreakpoint")).toInt(), 920);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("adaptiveNavRailMaxWidthRatio")).toDouble(), 0.31);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("adaptiveDrawerMarginSafety")).toInt(), 16);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("adaptiveDrawerEnterDuration")).toInt(), 155);
    PLATFORM_COMPARE(windowsProfile.value(QStringLiteral("adaptiveDrawerExitDuration")).toInt(), 115);
    CTEST_CHECK(windowsProfile.value(QStringLiteral("adaptiveAnimatedTransitions")).toBool());

    const QVariantMap desktopProfile = platform.runtimeProfile(QStringLiteral("linux"));
    CTEST_CHECK(desktopProfile.value(QStringLiteral("desktop")).toBool());
    CTEST_CHECK(!desktopProfile.value(QStringLiteral("runtimeEventsAutoAttachRecommended")).toBool());
    CTEST_CHECK(!desktopProfile.value(QStringLiteral("mobileSystemWindowDelegationRecommended")).toBool());
    CTEST_CHECK(!desktopProfile.value(QStringLiteral("mobileSystemInsetsDelegationRecommended")).toBool());
    CTEST_CHECK(!desktopProfile.value(QStringLiteral("mobileDisplayCoverageOverrideRecommended")).toBool());
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("bootstrapMsaaSamples")).toInt(), 4);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("bootstrapFramesInFlight")).toInt(), 2);
    CTEST_CHECK(desktopProfile.value(QStringLiteral("bootstrapPartialUpdateRecommended")).toBool());
    CTEST_CHECK(desktopProfile.value(QStringLiteral("bootstrapBatchRenderingRecommended")).toBool());
    CTEST_CHECK(desktopProfile.value(QStringLiteral("bootstrapPipelineCacheRecommended")).toBool());
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("bootstrapTextureAtlasEdge")).toInt(), 2048);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("adaptiveWideBreakpoint")).toInt(), 980);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("adaptiveNavWidth")).toInt(), 220);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("adaptiveNavDrawerWidth")).toInt(), 240);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("adaptiveMobileDesktopMinWidth")).toInt(), 1200);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("adaptiveBottomNavigationMaxItems")).toInt(), 5);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("adaptiveCompactSpacingBreakpoint")).toInt(), 900);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("adaptiveNavRailMaxWidthRatio")).toDouble(), 0.32);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("adaptiveDrawerMarginSafety")).toInt(), 16);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("adaptiveDrawerEnterDuration")).toInt(), 160);
    PLATFORM_COMPARE(desktopProfile.value(QStringLiteral("adaptiveDrawerExitDuration")).toInt(), 120);
    CTEST_CHECK(desktopProfile.value(QStringLiteral("adaptiveAnimatedTransitions")).toBool());

    const QVariantMap wasmProfile = platform.runtimeProfile(QStringLiteral("wasm"));
    CTEST_CHECK(wasmProfile.value(QStringLiteral("desktop")).toBool());
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("backend")).toString(), QStringLiteral("default"));
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("bootstrapMsaaSamples")).toInt(), 2);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("bootstrapFramesInFlight")).toInt(), 1);
    CTEST_CHECK(!wasmProfile.value(QStringLiteral("bootstrapPartialUpdateRecommended")).toBool());
    CTEST_CHECK(!wasmProfile.value(QStringLiteral("bootstrapBatchRenderingRecommended")).toBool());
    CTEST_CHECK(!wasmProfile.value(QStringLiteral("bootstrapPipelineCacheRecommended")).toBool());
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("bootstrapTextureAtlasEdge")).toInt(), 1024);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("adaptiveWideBreakpoint")).toInt(), 960);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("adaptiveNavWidth")).toInt(), 216);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("adaptiveNavDrawerWidth")).toInt(), 236);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("adaptiveMobileDesktopMinWidth")).toInt(), 1120);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("adaptiveBottomNavigationMaxItems")).toInt(), 4);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("adaptiveCompactSpacingBreakpoint")).toInt(), 880);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("adaptiveNavRailMaxWidthRatio")).toDouble(), 0.34);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("adaptiveDrawerMarginSafety")).toInt(), 16);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("adaptiveDrawerEnterDuration")).toInt(), 140);
    PLATFORM_COMPARE(wasmProfile.value(QStringLiteral("adaptiveDrawerExitDuration")).toInt(), 100);
    CTEST_CHECK(!wasmProfile.value(QStringLiteral("adaptiveAnimatedTransitions")).toBool());
}

void PlatformIntegrationTests::application_window_and_main_metrics_are_exposed()
{
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());

        const QByteArray qml = R"(
import QtQuick
import LVRS as LV

LV.ApplicationWindow {
    id: root
    width: 520
    height: 560
    autoAttachRuntimeEvents: true
    desktopMinWidth: 0
    desktopMinHeight: 0
    mobileMinWidth: 0
    mobileMinHeight: 0
    visible: false
    title: "MetricsWindow"

    property bool compactRule: matchesMedia("compact")
    property bool expandedRule: matchesMedia("expanded")
    property bool unknownRule: matchesMedia("unknown")
    property bool runtimeRunning: LV.RuntimeEvents.running
    property bool backendAdaptiveContractReady:
        backendWideBreakpoint === Number(backendRuntimeProfile.adaptiveWideBreakpoint)
        && navWidth === Number(backendRuntimeProfile.adaptiveNavWidth)
        && navDrawerWidth === Number(backendRuntimeProfile.adaptiveNavDrawerWidth)
        && scaffoldMobileDesktopMinWidth === Number(backendRuntimeProfile.adaptiveMobileDesktopMinWidth)
        && scaffoldBottomNavigationMaxItems === Number(backendRuntimeProfile.adaptiveBottomNavigationMaxItems)
        && scaffoldCompactSpacingBreakpoint === Number(backendRuntimeProfile.adaptiveCompactSpacingBreakpoint)
        && Math.abs(backendNavRailMaxWidthRatio - Number(backendRuntimeProfile.adaptiveNavRailMaxWidthRatio)) < 0.0001
        && backendDrawerMarginSafety === Number(backendRuntimeProfile.adaptiveDrawerMarginSafety)
        && backendDrawerEnterDuration === Number(backendRuntimeProfile.adaptiveDrawerEnterDuration)
        && backendDrawerExitDuration === Number(backendRuntimeProfile.adaptiveDrawerExitDuration)
        && backendAnimatedTransitions === (backendRuntimeProfile.adaptiveAnimatedTransitions === true)
    property bool tokenCompliant:
        LV.Theme.isThemeTextStyleCompliant(LV.Theme.textTitle, LV.Theme.textTitleWeight, LV.Theme.textTitleStyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textTitle2, LV.Theme.textTitle2Weight, LV.Theme.textTitle2StyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textHeader, LV.Theme.textHeaderWeight, LV.Theme.textHeaderStyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textHeader2, LV.Theme.textHeader2Weight, LV.Theme.textHeader2StyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textBody, LV.Theme.textBodyWeight, LV.Theme.textBodyStyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textDescription, LV.Theme.textDescriptionWeight, LV.Theme.textDescriptionStyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textCaption, LV.Theme.textCaptionWeight, LV.Theme.textCaptionStyleName)
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textDisabled, LV.Theme.textDisabledWeight, LV.Theme.textDisabledStyleName)
}
)";

        QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
        CTEST_CHECK(root);
        CTEST_CHECK(root->property("compactRule").toBool());
        CTEST_CHECK(!root->property("expandedRule").toBool());
        CTEST_CHECK(!root->property("unknownRule").toBool());
        CTEST_CHECK(!root->property("mobileOversizedHeightEnabled").toBool());
        CTEST_CHECK(root->property("backendAdaptiveContractReady").toBool());
        CTEST_WAIT_DEFAULT(root->property("runtimeRunning").toBool());
        CTEST_CHECK(root->property("tokenCompliant").toBool());

        const double effectiveScale = root->property("effectiveSupersampleScale").toDouble();
        CTEST_CHECK(effectiveScale >= 1.0);
        CTEST_CHECK(effectiveScale <= 4.0);

        root->setProperty("width", 1400);
        root->setProperty("height", 1020);
        CTEST_WAIT_DEFAULT(root->property("expandedRule").toBool());
    }

    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        const QDir testSource(QStringLiteral(LVRS_TEST_SOURCE_DIR));
        QString mainPath = testSource.filePath("../example/VisualCatalog/qml/Main.qml");
        if (!QFileInfo::exists(mainPath))
            mainPath = testSource.filePath("../src/qml/Main.qml");
        if (!QFileInfo::exists(mainPath))
            mainPath.clear();
        CTEST_CHECK_MESSAGE(!mainPath.isEmpty(),
                 "Failed to locate ../example/VisualCatalog/qml/Main.qml");

        QScopedPointer<QObject> root(TestUtils::loadQmlFile(engine, mainPath));
        CTEST_CHECK(root);
        CTEST_WAIT_DEFAULT(root->property("metricsPass").toBool());
        PLATFORM_COMPARE(root->property("metricsTotalChecks").toInt(), 6);
        PLATFORM_COMPARE(root->property("metricsPassedChecks").toInt(), 6);
        CTEST_CHECK(root->property("metricsRenderScaleCompliant").toBool());
        CTEST_CHECK(root->property("metricsFontFallbackCompliant").toBool());
        CTEST_CHECK(root->property("metricsThemeTextCompliant").toBool());
        CTEST_CHECK(root->property("metricsRuntimeCompliant").toBool());
        CTEST_CHECK(root->property("metricsSvgCompliant").toBool());
        CTEST_CHECK(root->property("metricsPageCompliant").toBool());
        CTEST_CHECK(root->property("metricsSummary").toString().contains('/'));
        CTEST_CHECK(root->property("runtimeSnapshot").isValid());
        const QVariantMap snapshot = root->property("runtimeSnapshot").toMap();
        CTEST_CHECK(snapshot.contains(QStringLiteral("pid")));
        CTEST_CHECK(snapshot.contains(QStringLiteral("uptimeMs")));
        CTEST_CHECK(snapshot.contains(QStringLiteral("rssBytes")));
        CTEST_WAIT_DEFAULT(root->property("catalogViewportReady").toBool());
        CTEST_CHECK(root->property("catalogSafeAreaEntryReady").toBool());
    PLATFORM_COMPARE(root->property("catalogComponentCount").toInt(), 92);
        CTEST_CHECK(root->property("catalogDocumentCount").toInt() > root->property("catalogComponentCount").toInt());
        PLATFORM_COMPARE(root->property("activeEntryKey").toString(), QStringLiteral("motion"));
        CTEST_CHECK(root->property("activeEntry").isValid());
    }
}

void PlatformIntegrationTests::mobile_theme_scale_contract()
{
    for (const QByteArray &target : {QByteArray("android-arm64"), QByteArray("ios")}) {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());

        QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    property Item listDelegateProbe: null

    Component.onCompleted: {
        LV.Theme.targetOverride = "@TARGET@"
        listDelegateProbe = listControl.createDelegateItem(root, listControl.itemDelegate, {
            "index": 0,
            "entry": "Fixed Body List",
            "label": "Fixed Body List",
            "enabled": true,
            "selected": false,
            "trigger": function() {}
        })
    }

    property bool tokenContract:
        LV.Theme.effectiveTarget === LV.Platform.normalizeTarget("@TARGET@")
        && LV.Theme.mobileTarget
        && LV.Theme.metricScaleFactor === 1.0
        && LV.Theme.typographyScaleFactor === 1.0
        && LV.Theme.gap8 === 8
        && LV.Theme.dialogMinWidth === 280
        && LV.Theme.textTitle === 26
        && LV.Theme.textTitle2 === 22
        && LV.Theme.textHeader === 17
        && LV.Theme.textHeader2 === 15
        && LV.Theme.textBody === 13
        && LV.Theme.textDescription === 12
        && LV.Theme.textCaption === 11
        && LV.Theme.scaleMetric(17) === 17
        && Math.abs(LV.Theme.scaleRealMetric(1.5) - 1.5) < 0.01
        && Math.abs(LV.Theme.scaleRealMetric(4) - 4.0) < 0.01
        && LV.Theme.scaleTextMetric(13) === 13
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textBody, LV.Theme.textBodyWeight, LV.Theme.textBodyStyleName)

    LV.List {
        id: listControl
        visible: false
        footerVisible: false
        items: ["Fixed Body List"]
    }

    LV.ListItem {
        id: miniListItem
        visible: false
    }

    LV.ListItem {
        id: detailListItem
        visible: false
        size: LV.ListItem.Detail
    }

    LV.ListFooter {
        id: listFooter
        visible: false
    }

    LV.LabelButton {
        id: figmaLabelButton
        text: "Button"
        visible: false
    }

    LV.IconButton {
        id: figmaIconButton
        visible: false
    }

    LV.LabelMenuButton {
        id: figmaLabelMenuButton
        text: "Open"
        visible: false
    }

    LV.IconMenuButton {
        id: figmaIconMenuButton
        visible: false
    }

    LV.LabelSegmentedControl {
        id: figmaLabelSegment
        visible: false

        LV.LabelButton { id: labelSegmentButton0; text: "Button" }
        LV.LabelButton { id: labelSegmentButton1; text: "Button" }
    }

    LV.IconSegmentedControl {
        id: figmaIconSegment
        visible: false

        LV.IconButton { id: iconSegmentButton0; iconName: "projectStructure" }
        LV.IconButton { id: iconSegmentButton1; iconName: "projectStructure" }
    }

    LV.Label {
        id: bodyLabel
        visible: false
        style: body
        text: "Fixed Body Label"
    }

    LV.TextEditor {
        id: textEditor
        visible: false
        filePath: ""
    }

    LV.CodeEditor {
        id: codeEditor
        visible: false
    }

    LV.MenuItem {
        id: menuItem
        visible: false
    }

    LV.MenuDivider {
        id: menuDivider
        visible: false
    }

    LV.HierarchyToolbar {
        id: hierarchyToolbar
        visible: false
        buttonItems: [
            { id: "slot0", iconName: "projectStructure", selected: true },
            { id: "slot1", iconName: "projectStructure" },
            { id: "slot2", iconName: "projectStructure" },
            { id: "slot3", iconName: "projectStructure" }
        ]
    }

    LV.HierarchyItem {
        id: hierarchyItem
        visible: false
    }

    LV.HierarchyList {
        id: hierarchyList
        visible: false
    }

    LV.Hierarchy {
        id: hierarchyPanel
        visible: false
    }

    LV.CheckBox {
        id: checkBox
        text: "Label"
        opacity: 0
    }

    LV.RadioButton {
        id: radioButton
        text: "Label"
        checked: true
        opacity: 0
    }

    LV.Table {
        id: table
        opacity: 0
    }

    LV.TableHeader {
        id: tableHeader
        visible: false
    }

    LV.TableRow {
        id: tableRow
        visible: false
    }

    LV.TableCellItem {
        id: tableCell
        visible: false
    }

    LV.Stepper {
        id: stepper
        visible: false
        arrow: LV.Stepper.UpDown
    }

    LV.ComboBox {
        id: comboBox
        visible: false
    }

    function findListLabel(item) {
        if (!item)
            return null
        if (item.objectName === "listItem_miniLabel")
            return item
        for (let i = 0; i < item.children.length; ++i) {
            const label = findListLabel(item.children[i])
            if (label)
                return label
        }
        return null
    }
    readonly property Item listDelegateBodyLabel: findListLabel(listDelegateProbe)
    readonly property int listDelegateBodyPixelSize: listDelegateBodyLabel
        ? listDelegateBodyLabel.font.pixelSize : -1

    property bool componentContract:
        listControl.listWidth === 170
        && listControl.minimumListHeight === 140
        && listControl.itemHeight === 22
        && listDelegateProbe !== null
        && listDelegateProbe.contentItem.children.length > 0
        && listDelegateBodyPixelSize === 13
        && miniListItem.iconSize === 18
        && miniListItem.rowHorizontalPadding === 4
        && miniListItem.rowVerticalPadding === 2
        && miniListItem.implicitWidth === 170
        && miniListItem.implicitHeight === 22
        && detailListItem.detailItemWidth === 194
        && detailListItem.detailContentWidth === 170
        && detailListItem.detailTopHeight === 24
        && detailListItem.detailMiddleHeight === 12
        && detailListItem.detailBottomHeight === 38
        && detailListItem.implicitWidth === 194
        && detailListItem.implicitHeight === 106
        && listFooter.horizontalPadding === 2
        && listFooter.verticalPadding === 2
        && listFooter.stockButtonPadding === 2
        && listFooter.stockButtonHeight === 22
        && listFooter.stockMenuButtonSpacing === -2
        && listFooter.implicitWidth === 88
        && listFooter.implicitHeight === 26
        && figmaLabelButton.tone === LV.AbstractButton.Primary
        && figmaIconButton.tone === LV.AbstractButton.Primary
        && figmaLabelMenuButton.tone === LV.AbstractButton.Primary
        && figmaIconMenuButton.tone === LV.AbstractButton.Primary
        && figmaLabelButton.figmaButtonHeight === 22
        && figmaIconButton.figmaButtonHeight === 22
        && figmaLabelMenuButton.figmaButtonHeight === 22
        && figmaIconMenuButton.figmaButtonHeight === 22
        && figmaLabelButton.horizontalPadding === 8
        && Math.abs(figmaLabelButton.verticalPadding - 4.5) < 0.01
        && figmaIconButton.horizontalPadding === 2
        && figmaIconButton.verticalPadding === 2
        && figmaLabelMenuButton.horizontalPadding === 8
        && figmaLabelMenuButton.verticalPadding === 2
        && figmaLabelMenuButton.spacing === 0
        && figmaLabelMenuButton.rightPadding === 2
        && figmaIconMenuButton.horizontalPadding === 4
        && figmaIconMenuButton.rightPadding === 2
        && figmaIconMenuButton.verticalPadding === 2
        && figmaIconMenuButton.spacing === -2
        && figmaLabelButton.implicitWidth === 56
        && figmaIconButton.implicitWidth === 22
        && figmaLabelMenuButton.implicitWidth === 60
        && figmaIconMenuButton.implicitWidth === 40
        && figmaLabelButton.implicitHeight === 22
        && figmaIconButton.implicitHeight === 22
        && figmaLabelMenuButton.implicitHeight === 22
        && figmaIconMenuButton.implicitHeight === 22
        && figmaLabelSegment.segmentCount === 2
        && figmaLabelSegment.horizontalPadding === 4
        && Math.abs(figmaLabelSegment.verticalPadding - 3.5) < 0.01
        && figmaLabelSegment.spacing === 2
        && figmaLabelSegment.borderWidth === 2
        && figmaLabelSegment.cornerRadius === 12
        && figmaLabelSegment.implicitWidth === 122
        && figmaLabelSegment.width === 122
        && Math.abs(figmaLabelSegment.implicitHeight - 29.0) < 0.01
        && Math.abs(figmaLabelSegment.height - 29.0) < 0.01
        && labelSegmentButton0.width === 56
        && labelSegmentButton0.height === 22
        && labelSegmentButton1.x === 58
        && figmaIconSegment.segmentCount === 2
        && figmaIconSegment.horizontalPadding === 4
        && figmaIconSegment.verticalPadding === 4
        && figmaIconSegment.spacing === 2
        && figmaIconSegment.borderWidth === 2
        && figmaIconSegment.cornerRadius === 12
        && figmaIconSegment.implicitWidth === 54
        && figmaIconSegment.width === 54
        && figmaIconSegment.implicitHeight === 30
        && figmaIconSegment.height === 30
        && iconSegmentButton0.width === 22
        && iconSegmentButton0.height === 22
        && iconSegmentButton1.x === 24
        && bodyLabel.stylePixelSize === 13
        && bodyLabel.styleLineHeight === 13
        && bodyLabel.font.pixelSize === 13
        && textEditor.fontPixelSize === 13
        && textEditor.textLineHeight === 13
        && codeEditor.fontPixelSize === 13
        && codeEditor.textLineHeight === 13
        && menuItem.itemWidth === 161
        && menuItem.itemHeight === 24
        && menuItem.iconSize === 18
        && menuItem.chevronSize === 16
        && menuItem.topPadding === 3
        && menuItem.bottomPadding === 3
        && menuDivider.lineLength === 220
        && menuDivider.linePadding === 0
        && Math.abs(menuDivider.thickness - 1.0) < 0.01
        && Math.abs(menuDivider.implicitWidth - 220.0) < 0.01
        && Math.abs(menuDivider.implicitHeight - 3.0) < 0.01
        && hierarchyToolbar.minimumToolbarWidth === 200
        && hierarchyToolbar.horizontalPadding === 8
        && hierarchyToolbar.verticalPadding === 2
        && hierarchyToolbar.slotSize === 22
        && hierarchyToolbar.spacing === 0
        && !hierarchyToolbar.distributeSpacing
        && hierarchyToolbar.implicitWidth === 200
        && hierarchyToolbar.implicitHeight === 26
        && hierarchyItem.rowHeight === 20
        && hierarchyItem.itemWidth === 200
        && hierarchyItem.iconSize === 18
        && hierarchyItem.chevronSize === 16
        && hierarchyItem.baseLeftPadding === 8
        && hierarchyItem.rowRightPadding === 8
        && hierarchyItem.leadingSpacing === 2
        && hierarchyItem.cornerRadius === 5
        && hierarchyItem.implicitWidth === 200
        && hierarchyItem.implicitHeight === 20
        && hierarchyList.generatedIndentStep === 8
        && hierarchyList.generatedRowHeight === 20
        && hierarchyList.generatedItemWidth === 200
        && hierarchyList.generatedIconSize === 18
        && hierarchyList.generatedChevronSize === 16
        && hierarchyPanel.minimumPanelWidth === 200
        && hierarchyPanel.minimumPanelHeight === 530
        && hierarchyPanel.implicitWidth === 200
        && hierarchyPanel.implicitHeight === 530
        && checkBox.boxSize === 17
        && Math.abs(checkBox.framePadding - 0.5) < 0.01
        && Math.abs(checkBox.boxRadius - 3.5) < 0.01
        && Math.abs(checkBox.boxBorderWidthUncheckedEnabled - 0.5) < 0.01
        && checkBox.contentItem.spacing === 6
        && checkBox.implicitWidth === 57
        && checkBox.implicitHeight === 18
        && checkBox.useFigmaCheckedAssets
        && checkBox.checkedAssetSourceEnabled.toString() === LV.Theme.iconPath("checkboxCheckedEnabled").toString()
        && checkBox.checkedAssetSourceDisabled.toString() === LV.Theme.iconPath("checkboxCheckedDisabled").toString()
        && radioButton.indicatorSize === 18
        && radioButton.dotSize === 8
        && radioButton.indicatorRadius === 9
        && radioButton.dotRadius === 4
        && radioButton.contentItem.spacing === 8
        && radioButton.implicitWidth === 59
        && radioButton.implicitHeight === 18
        && radioButton.contentItem.children[0].x === 0
        && radioButton.contentItem.children[0].y === 0
        && radioButton.contentItem.children[0].children[0].x === 5
        && radioButton.contentItem.children[0].children[0].y === 5
        && Math.abs(radioButton.contentItem.children[1].x - 26.0) < 0.01
        && Math.abs(radioButton.contentItem.children[1].y - 2.5) < 0.01
        && radioButton.contentItem.children[1].font.pixelSize === 13
        && table.implicitWidth === 528
        && table.implicitHeight === 121
        && table.rowHeight === 24
        && table.borderWidth === 1
        && String(table.backgroundColor) === "#1e1e1e"
        && table.borderColor === LV.Theme.panelBackground10
        && table.rowDividerColor === LV.Theme.panelBackground10
        && !table.structureControlsVisible
        && tableHeader.implicitWidth === 717
        && tableHeader.implicitHeight === 25
        && tableHeader.rowHeight === 24
        && tableHeader.separatorHeight === 1
        && tableHeader.cellHorizontalPadding === 8
        && tableRow.implicitWidth === 717
        && tableRow.implicitHeight === 24
        && tableRow.cellWidth === 234
        && tableRow.cellHeight === 24
        && tableRow.contentSpacing === 8
        && tableRow.dividerColor === LV.Theme.panelBackground10
        && tableCell.implicitWidth === 234
        && tableCell.implicitHeight === 24
        && tableCell.resolvedContentSpacing === 8
        && LV.Theme.textBody === 13
        && stepper.width === 18
        && stepper.height === 18
        && Math.abs(stepper.iconWidth - (18 * (6.43604 / 18.0))) < 0.01
        && Math.abs(stepper.iconHeight - (18 * (11.1455 / 18.0))) < 0.01
        && comboBox.width === 97
        && comboBox.height === 20
        && Math.abs(comboBox.labelBounds.x - 8.0) < 0.01
        && Math.abs(comboBox.labelBounds.y - 3.5) < 0.01
        && Math.abs(comboBox.indicatorBounds.x - 78.0) < 0.01
        && Math.abs(comboBox.indicatorBounds.y - 1.0) < 0.01
        && comboBox.indicatorBounds.width === 18
        && comboBox.indicatorBounds.height === 18


    property string componentDebug: JSON.stringify({
        listWidth: listControl.listWidth,
        listMinHeight: listControl.minimumListHeight,
        listItemHeight: listControl.itemHeight,
        bodyPixelSize: listDelegateBodyPixelSize,
        miniPaddingX: miniListItem.rowHorizontalPadding,
        miniPaddingY: miniListItem.rowVerticalPadding,
        miniWidth: miniListItem.implicitWidth,
        miniHeight: miniListItem.implicitHeight,
        detailItemWidth: detailListItem.detailItemWidth,
        detailContentWidth: detailListItem.detailContentWidth,
        detailTopHeight: detailListItem.detailTopHeight,
        detailMiddleHeight: detailListItem.detailMiddleHeight,
        detailBottomHeight: detailListItem.detailBottomHeight,
        detailWidth: detailListItem.implicitWidth,
        detailHeight: detailListItem.implicitHeight,
        footerPaddingX: listFooter.horizontalPadding,
        footerStockPadding: listFooter.stockButtonPadding,
        footerButtonHeight: listFooter.stockButtonHeight,
        footerWidth: listFooter.implicitWidth,
        footerHeight: listFooter.implicitHeight,
        labelButton: [figmaLabelButton.implicitWidth, figmaLabelButton.implicitHeight,
            figmaLabelButton.horizontalPadding, figmaLabelButton.verticalPadding,
            figmaLabelButton.tone],
        iconButton: [figmaIconButton.implicitWidth, figmaIconButton.implicitHeight,
            figmaIconButton.horizontalPadding, figmaIconButton.verticalPadding,
            figmaIconButton.tone],
        labelMenuButton: [figmaLabelMenuButton.implicitWidth,
            figmaLabelMenuButton.implicitHeight, figmaLabelMenuButton.spacing,
            figmaLabelMenuButton.tone],
        iconMenuButton: [figmaIconMenuButton.implicitWidth,
            figmaIconMenuButton.implicitHeight, figmaIconMenuButton.spacing,
            figmaIconMenuButton.tone],
        labelSegment: [figmaLabelSegment.implicitWidth,
            figmaLabelSegment.implicitHeight, figmaLabelSegment.horizontalPadding,
            figmaLabelSegment.verticalPadding, figmaLabelSegment.spacing,
            labelSegmentButton0.width, labelSegmentButton0.height,
            labelSegmentButton0.implicitWidth, labelSegmentButton0.implicitHeight,
            labelSegmentButton1.x],
        iconSegment: [figmaIconSegment.implicitWidth,
            figmaIconSegment.implicitHeight, figmaIconSegment.horizontalPadding,
            figmaIconSegment.verticalPadding, figmaIconSegment.spacing,
            iconSegmentButton0.width, iconSegmentButton0.height,
            iconSegmentButton0.implicitWidth, iconSegmentButton0.implicitHeight,
            iconSegmentButton1.x],
        hierarchyToolbar: [hierarchyToolbar.minimumToolbarWidth,
            hierarchyToolbar.horizontalPadding, hierarchyToolbar.verticalPadding,
            hierarchyToolbar.slotSize, hierarchyToolbar.implicitWidth,
            hierarchyToolbar.implicitHeight],
        hierarchyItem: [hierarchyItem.rowHeight, hierarchyItem.itemWidth,
            hierarchyItem.iconSize, hierarchyItem.chevronSize,
            hierarchyItem.baseLeftPadding, hierarchyItem.rowRightPadding,
            hierarchyItem.leadingSpacing, hierarchyItem.cornerRadius,
            hierarchyItem.implicitWidth, hierarchyItem.implicitHeight],
        hierarchyList: [hierarchyList.generatedIndentStep,
            hierarchyList.generatedRowHeight, hierarchyList.generatedItemWidth,
            hierarchyList.generatedIconSize, hierarchyList.generatedChevronSize],
        hierarchyPanel: [hierarchyPanel.minimumPanelWidth,
            hierarchyPanel.minimumPanelHeight, hierarchyPanel.implicitWidth,
            hierarchyPanel.implicitHeight],
        checkBox: [checkBox.boxSize, checkBox.framePadding, checkBox.boxRadius,
            checkBox.boxBorderWidthUncheckedEnabled, checkBox.contentItem.spacing,
            checkBox.implicitWidth, checkBox.implicitHeight],
        radioButton: [radioButton.indicatorSize, radioButton.dotSize,
            radioButton.indicatorRadius, radioButton.dotRadius,
            radioButton.contentItem.spacing, radioButton.implicitWidth,
            radioButton.implicitHeight],
        table: [table.implicitWidth, table.implicitHeight, table.rowHeight,
            table.borderWidth, table.structureControlsVisible,
            table.resolvedRowCount, table.resolvedBodyHeight,
            table.rowHeightAt(0), table.rowHeights, table.minRowHeight],
        tableHeader: [tableHeader.implicitWidth, tableHeader.implicitHeight,
            tableHeader.rowHeight, tableHeader.separatorHeight,
            tableHeader.cellHorizontalPadding],
        tableRow: [tableRow.implicitWidth, tableRow.implicitHeight,
            tableRow.cellWidth, tableRow.cellHeight, tableRow.contentSpacing],
        tableCell: [tableCell.implicitWidth, tableCell.implicitHeight,
            tableCell.resolvedContentSpacing]
    })
}
)";

        qml.replace("@TARGET@", target);
        QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
        CTEST_CHECK(root);
        CTEST_WAIT_DEFAULT(root->property("tokenContract").toBool());
        PLATFORM_COMPARE(root->property("listDelegateBodyPixelSize").toInt(), 13);
        CTEST_CHECK_MESSAGE(root->property("componentContract").toBool(),
                 qPrintable(root->property("componentDebug").toString()));
    }

    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());

        const QByteArray qml = R"(
import QtQuick
import LVRS as LV

QtObject {
    Component.onCompleted: LV.Theme.targetOverride = "windows"

    property bool desktopContract:
        LV.Theme.effectiveTarget === "windows"
        && !LV.Theme.mobileTarget
        && LV.Theme.metricScaleFactor === 1.0
        && LV.Theme.typographyScaleFactor === 1.0
        && LV.Theme.gap8 === 8
        && LV.Theme.textBody === 13
        && LV.Theme.textCaption === 11
}
)";

        QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
        CTEST_CHECK(root);
        CTEST_WAIT_DEFAULT(root->property("desktopContract").toBool());
    }
}

void PlatformIntegrationTests::native_window_style_mobile_coverage_flags()
{
    NativeWindowStyle nativeWindowStyle;
    QWindow window;

    CTEST_CHECK(nativeWindowStyle.applyMobileCoverageFlags(&window, true, true));
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    CTEST_CHECK(window.flags().testFlag(Qt::ExpandedClientAreaHint));
    CTEST_CHECK(window.flags().testFlag(Qt::NoTitleBarBackgroundHint));
#endif
    CTEST_CHECK(window.flags().testFlag(Qt::MaximizeUsingFullscreenGeometryHint));

    CTEST_CHECK(nativeWindowStyle.applyMobileCoverageFlags(&window, false, false));
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    CTEST_CHECK(!window.flags().testFlag(Qt::ExpandedClientAreaHint));
    CTEST_CHECK(!window.flags().testFlag(Qt::NoTitleBarBackgroundHint));
#endif
    CTEST_CHECK(!window.flags().testFlag(Qt::MaximizeUsingFullscreenGeometryHint));
}

void PlatformIntegrationTests::native_window_interaction_validates_system_resize_edges()
{
    NativeWindowInteraction interaction;

    CTEST_CHECK(interaction.isValidResizeEdges(int(Qt::LeftEdge)));
    CTEST_CHECK(interaction.isValidResizeEdges(int(Qt::TopEdge)));
    CTEST_CHECK(interaction.isValidResizeEdges(int(Qt::RightEdge)));
    CTEST_CHECK(interaction.isValidResizeEdges(int(Qt::BottomEdge)));
    CTEST_CHECK(interaction.isValidResizeEdges(int(Qt::LeftEdge | Qt::TopEdge)));
    CTEST_CHECK(interaction.isValidResizeEdges(int(Qt::TopEdge | Qt::RightEdge)));
    CTEST_CHECK(interaction.isValidResizeEdges(int(Qt::RightEdge | Qt::BottomEdge)));
    CTEST_CHECK(interaction.isValidResizeEdges(int(Qt::BottomEdge | Qt::LeftEdge)));

    CTEST_CHECK(!interaction.isValidResizeEdges(0));
    CTEST_CHECK(!interaction.isValidResizeEdges(int(Qt::LeftEdge | Qt::RightEdge)));
    CTEST_CHECK(!interaction.isValidResizeEdges(int(Qt::TopEdge | Qt::BottomEdge)));
    CTEST_CHECK(!interaction.isValidResizeEdges(int(Qt::LeftEdge | Qt::TopEdge | Qt::RightEdge)));
    CTEST_CHECK(!interaction.isValidResizeEdges(1 << 20));

    QObject notAWindow;
    CTEST_CHECK(!interaction.requestSystemMove(&notAWindow));
    CTEST_CHECK(!interaction.requestSystemResize(&notAWindow, int(Qt::LeftEdge)));

    QWindow window;
    CTEST_CHECK(!interaction.requestSystemResize(&window, 0));
    CTEST_CHECK(!interaction.requestSystemResize(&window, int(Qt::LeftEdge | Qt::RightEdge)));
}

void PlatformIntegrationTests::native_window_interaction_resizes_manually_when_system_resize_is_unavailable()
{
#if defined(Q_OS_MACOS)
    QWindow window;
    window.setGeometry(100, 100, 480, 360);
    window.setMinimumSize(QSize(360, 240));
    window.show();
    CTEST_WAIT_DEFAULT(window.isVisible());

    const QRect initialGeometry = window.geometry();
    const QPoint initialPointer(400, 300);
    QCursor::setPos(QPoint(20, 20));

    NativeWindowInteraction interaction;
    CTEST_CHECK(interaction.requestSystemResizeAt(&window,
                                              int(Qt::RightEdge | Qt::BottomEdge),
                                              initialPointer));

    const QPoint resizePointer = initialPointer + QPoint(120, 90);
    QMouseEvent resizeMove(QEvent::MouseMove,
                           QPointF(window.width() - 1, window.height() - 1),
                           QPointF(resizePointer),
                           Qt::NoButton,
                           Qt::LeftButton,
                           Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &resizeMove);

    PLATFORM_COMPARE(window.geometry().topLeft(), initialGeometry.topLeft());
    PLATFORM_COMPARE(window.size(), initialGeometry.size() + QSize(120, 90));

    QMouseEvent resizeRelease(QEvent::MouseButtonRelease,
                              QPointF(window.width() - 1, window.height() - 1),
                              QPointF(resizePointer),
                              Qt::LeftButton,
                              Qt::NoButton,
                              Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &resizeRelease);

    const QRect releasedGeometry = window.geometry();
    QMouseEvent postReleaseMove(QEvent::MouseMove,
                                QPointF(window.width() - 1, window.height() - 1),
                                QPointF(resizePointer + QPoint(40, 30)),
                                Qt::NoButton,
                                Qt::LeftButton,
                                Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &postReleaseMove);
    PLATFORM_COMPARE(window.geometry(), releasedGeometry);

    window.setGeometry(initialGeometry);
    QCursor::setPos(QPoint(400, 300));
    const QPoint constrainedInitialPointer = QCursor::pos();
    CTEST_CHECK(interaction.requestSystemResize(&window, int(Qt::LeftEdge | Qt::TopEdge)));

    const QPoint constrainedPointer = constrainedInitialPointer + QPoint(200, 180);
    QMouseEvent constrainedMove(QEvent::MouseMove,
                                QPointF(0, 0),
                                QPointF(constrainedPointer),
                                Qt::NoButton,
                                Qt::LeftButton,
                                Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &constrainedMove);

    PLATFORM_COMPARE(window.geometry().topLeft(), initialGeometry.topLeft() + QPoint(120, 120));
    PLATFORM_COMPARE(window.size(), QSize(360, 240));

    QMouseEvent constrainedRelease(QEvent::MouseButtonRelease,
                                   QPointF(0, 0),
                                   QPointF(constrainedPointer),
                                   Qt::LeftButton,
                                   Qt::NoButton,
                                   Qt::NoModifier);
    QCoreApplication::sendEvent(&window, &constrainedRelease);
#else
    lvrs_ctest::skip("The manual fallback is specific to the Cocoa platform gap.");
#endif
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    PlatformIntegrationTests tests;
    runner.run("safe_area_tracks_window_lifetime", [&] { tests.safe_area_tracks_window_lifetime(); });
    runner.run("platform_flags_consistency", [&] { tests.platform_flags_consistency(); });
    runner.run("platform_runtime_profiles_are_exposed", [&] { tests.platform_runtime_profiles_are_exposed(); });
    runner.run("application_window_and_main_metrics_are_exposed", [&] { tests.application_window_and_main_metrics_are_exposed(); });
    runner.run("mobile_theme_scale_contract", [&] { tests.mobile_theme_scale_contract(); });
    runner.run("native_window_interaction_resizes_manually_when_system_resize_is_unavailable", [&] { tests.native_window_interaction_resizes_manually_when_system_resize_is_unavailable(); });
    runner.run("native_window_interaction_validates_system_resize_edges", [&] { tests.native_window_interaction_validates_system_resize_edges(); });
    runner.run("native_window_style_mobile_coverage_flags", [&] { tests.native_window_style_mobile_coverage_flags(); });
    return runner.result();
}
