#include <QtTest>

#include <QScopedPointer>
#include <QQmlEngine>
#include <QWindow>
#include <QtPlugin>

#include "backend/platform/nativewindowstyle.h"
#include "backend/platform/platforminfo.h"
#include "test_utils.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class PlatformIntegrationTests : public QObject
{
    Q_OBJECT

private slots:
    void platform_flags_consistency();
    void platform_runtime_profiles_are_exposed();
    void application_window_and_main_metrics_are_exposed();
    void mobile_theme_scale_contract();
    void native_window_style_mobile_coverage_flags();
};

void PlatformIntegrationTests::platform_flags_consistency()
{
    PlatformInfo platform;
    QVERIFY(!platform.os().isEmpty());
    QVERIFY(!platform.arch().isEmpty());
    QVERIFY(platform.desktop() || platform.mobile());

    const int explicitPlatformCount = (platform.android() ? 1 : 0)
        + (platform.ios() ? 1 : 0)
        + (platform.macos() ? 1 : 0)
        + (platform.windows() ? 1 : 0)
        + (platform.linux() ? 1 : 0)
        + (platform.wasm() ? 1 : 0);
    QCOMPARE(explicitPlatformCount, 1);

    if (platform.macos())
        QCOMPARE(platform.os(), QStringLiteral("osx"));
    if (platform.windows())
        QCOMPARE(platform.os(), QStringLiteral("windows"));
    if (platform.linux())
        QCOMPARE(platform.os(), QStringLiteral("linux"));
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
    QCOMPARE(platform.runtimeTargets(), expectedTargets);
    QCOMPARE(platform.desktopTargets(),
             QStringList({QStringLiteral("macos"), QStringLiteral("linux"), QStringLiteral("windows"), QStringLiteral("wasm")}));
    QCOMPARE(platform.mobileTargets(), QStringList({QStringLiteral("ios"), QStringLiteral("android")}));

    QVERIFY(expectedTargets.contains(platform.canonicalOs()));
    QVERIFY(!platform.graphicsBackend().isEmpty());
    QVERIFY(platform.targetMatchesCurrent(platform.canonicalOs()));
    QVERIFY(platform.supportsTargetGeneration(platform.canonicalOs()));

    QCOMPARE(platform.normalizeTarget(QStringLiteral("osx")), QStringLiteral("macos"));
    QCOMPARE(platform.normalizeTarget(QStringLiteral("win32")), QStringLiteral("windows"));
    QVERIFY(platform.normalizeTarget(QStringLiteral("unknown-target")).isEmpty());
    QVERIFY(!platform.isKnownTarget(QStringLiteral("unknown-target")));

    const QVariantMap currentProfile = platform.runtimeProfile();
    QVERIFY(currentProfile.value(QStringLiteral("known")).toBool());
    QCOMPARE(currentProfile.value(QStringLiteral("target")).toString(), platform.canonicalOs());
    QVERIFY(currentProfile.contains(QStringLiteral("backend")));
    QVERIFY(currentProfile.contains(QStringLiteral("generationSupported")));
    QVERIFY(currentProfile.contains(QStringLiteral("backendFeatureReady")));
    QVERIFY(currentProfile.contains(QStringLiteral("cmakeSystemName")));
    QVERIFY(currentProfile.contains(QStringLiteral("runtimeEventsAutoAttachRecommended")));
    QVERIFY(currentProfile.contains(QStringLiteral("mobileSystemWindowDelegationRecommended")));
    QVERIFY(currentProfile.contains(QStringLiteral("mobileSystemInsetsDelegationRecommended")));
    QVERIFY(currentProfile.contains(QStringLiteral("mobileDisplayCoverageOverrideRecommended")));
    QVERIFY(currentProfile.contains(QStringLiteral("mobileFullscreenVisibilityRecommended")));
    QVERIFY(currentProfile.contains(QStringLiteral("mobileFullscreenGeometryHintRecommended")));
    QVERIFY(currentProfile.contains(QStringLiteral("bootstrapMsaaSamples")));
    QVERIFY(currentProfile.contains(QStringLiteral("bootstrapFramesInFlight")));
    QVERIFY(currentProfile.contains(QStringLiteral("bootstrapPartialUpdateRecommended")));
    QVERIFY(currentProfile.contains(QStringLiteral("bootstrapBatchRenderingRecommended")));
    QVERIFY(currentProfile.contains(QStringLiteral("bootstrapPipelineCacheRecommended")));
    QVERIFY(currentProfile.contains(QStringLiteral("bootstrapTextureAtlasEdge")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveWideBreakpoint")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveNavWidth")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveNavDrawerWidth")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveMobileDesktopMinWidth")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveBottomNavigationMaxItems")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveCompactSpacingBreakpoint")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveNavRailMaxWidthRatio")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveDrawerMarginSafety")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveDrawerEnterDuration")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveDrawerExitDuration")));
    QVERIFY(currentProfile.contains(QStringLiteral("adaptiveAnimatedTransitions")));

    const QVariantList allProfiles = platform.runtimeProfiles();
    QCOMPARE(allProfiles.size(), expectedTargets.size());
    for (const QVariant &profileVariant : allProfiles) {
        const QVariantMap profile = profileVariant.toMap();
        QVERIFY(profile.value(QStringLiteral("known")).toBool());
        QVERIFY(expectedTargets.contains(profile.value(QStringLiteral("target")).toString()));
        QVERIFY(profile.contains(QStringLiteral("backend")));
        QVERIFY(profile.contains(QStringLiteral("directRunSupported")));
    }

    const QVariantMap iosProfile = platform.runtimeProfile(QStringLiteral("ios"));
    QVERIFY(iosProfile.value(QStringLiteral("mobile")).toBool());
    QVERIFY(iosProfile.value(QStringLiteral("ios")).toBool());
    QVERIFY(!iosProfile.value(QStringLiteral("android")).toBool());
    QVERIFY(!iosProfile.value(QStringLiteral("runtimeEventsAutoAttachRecommended")).toBool());
    QVERIFY(!iosProfile.value(QStringLiteral("mobileSystemWindowDelegationRecommended")).toBool());
    QVERIFY(!iosProfile.value(QStringLiteral("mobileSystemInsetsDelegationRecommended")).toBool());
    QVERIFY(iosProfile.value(QStringLiteral("mobileDisplayCoverageOverrideRecommended")).toBool());
    QVERIFY(iosProfile.value(QStringLiteral("mobileFullscreenVisibilityRecommended")).toBool());
    QVERIFY(iosProfile.value(QStringLiteral("mobileFullscreenGeometryHintRecommended")).toBool());
    QCOMPARE(iosProfile.value(QStringLiteral("bootstrapMsaaSamples")).toInt(), 4);
    QCOMPARE(iosProfile.value(QStringLiteral("bootstrapFramesInFlight")).toInt(), 2);
    QVERIFY(iosProfile.value(QStringLiteral("bootstrapPartialUpdateRecommended")).toBool());
    QVERIFY(iosProfile.value(QStringLiteral("bootstrapBatchRenderingRecommended")).toBool());
    QVERIFY(iosProfile.value(QStringLiteral("bootstrapPipelineCacheRecommended")).toBool());
    QCOMPARE(iosProfile.value(QStringLiteral("bootstrapTextureAtlasEdge")).toInt(), 1024);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveWideBreakpoint")).toInt(), 948);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveNavWidth")).toInt(), 216);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveNavDrawerWidth")).toInt(), 264);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveMobileDesktopMinWidth")).toInt(), 1180);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveBottomNavigationMaxItems")).toInt(), 5);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveCompactSpacingBreakpoint")).toInt(), 860);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveNavRailMaxWidthRatio")).toDouble(), 0.33);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveDrawerMarginSafety")).toInt(), 24);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveDrawerEnterDuration")).toInt(), 185);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveDrawerExitDuration")).toInt(), 145);
    QCOMPARE(iosProfile.value(QStringLiteral("adaptiveAnimatedTransitions")).toBool(),
             iosProfile.value(QStringLiteral("backendFeatureReady")).toBool());

    const QVariantMap androidProfile = platform.runtimeProfile(QStringLiteral("android"));
    QVERIFY(androidProfile.value(QStringLiteral("mobile")).toBool());
    QVERIFY(androidProfile.value(QStringLiteral("android")).toBool());
    QVERIFY(!androidProfile.value(QStringLiteral("ios")).toBool());
    QVERIFY(!androidProfile.value(QStringLiteral("runtimeEventsAutoAttachRecommended")).toBool());
    QVERIFY(androidProfile.value(QStringLiteral("mobileSystemWindowDelegationRecommended")).toBool());
    QVERIFY(androidProfile.value(QStringLiteral("mobileSystemInsetsDelegationRecommended")).toBool());
    QVERIFY(androidProfile.value(QStringLiteral("mobileDisplayCoverageOverrideRecommended")).toBool());
    QVERIFY(androidProfile.value(QStringLiteral("mobileFullscreenVisibilityRecommended")).toBool());
    QVERIFY(androidProfile.value(QStringLiteral("mobileFullscreenGeometryHintRecommended")).toBool());
    QCOMPARE(androidProfile.value(QStringLiteral("bootstrapMsaaSamples")).toInt(), 4);
    QCOMPARE(androidProfile.value(QStringLiteral("bootstrapFramesInFlight")).toInt(), 2);
    QVERIFY(androidProfile.value(QStringLiteral("bootstrapPartialUpdateRecommended")).toBool());
    QVERIFY(androidProfile.value(QStringLiteral("bootstrapBatchRenderingRecommended")).toBool());
    QVERIFY(androidProfile.value(QStringLiteral("bootstrapPipelineCacheRecommended")).toBool());
    QCOMPARE(androidProfile.value(QStringLiteral("bootstrapTextureAtlasEdge")).toInt(), 1024);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveWideBreakpoint")).toInt(), 940);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveNavWidth")).toInt(), 208);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveNavDrawerWidth")).toInt(), 252);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveMobileDesktopMinWidth")).toInt(), 1080);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveBottomNavigationMaxItems")).toInt(), 4);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveCompactSpacingBreakpoint")).toInt(), 840);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveNavRailMaxWidthRatio")).toDouble(), 0.34);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveDrawerMarginSafety")).toInt(), 20);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveDrawerEnterDuration")).toInt(), 170);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveDrawerExitDuration")).toInt(), 130);
    QCOMPARE(androidProfile.value(QStringLiteral("adaptiveAnimatedTransitions")).toBool(),
             androidProfile.value(QStringLiteral("backendFeatureReady")).toBool());

    const QVariantMap windowsProfile = platform.runtimeProfile(QStringLiteral("windows"));
    QVERIFY(windowsProfile.value(QStringLiteral("desktop")).toBool());
    QCOMPARE(windowsProfile.value(QStringLiteral("backend")).toString(), QStringLiteral("d3d11"));
    QCOMPARE(windowsProfile.value(QStringLiteral("bootstrapMsaaSamples")).toInt(), 4);
    QCOMPARE(windowsProfile.value(QStringLiteral("bootstrapFramesInFlight")).toInt(), 2);
    QVERIFY(windowsProfile.value(QStringLiteral("bootstrapPartialUpdateRecommended")).toBool());
    QVERIFY(windowsProfile.value(QStringLiteral("bootstrapBatchRenderingRecommended")).toBool());
    QVERIFY(windowsProfile.value(QStringLiteral("bootstrapPipelineCacheRecommended")).toBool());
    QCOMPARE(windowsProfile.value(QStringLiteral("bootstrapTextureAtlasEdge")).toInt(), 2048);
    QCOMPARE(windowsProfile.value(QStringLiteral("adaptiveWideBreakpoint")).toInt(), 1000);
    QCOMPARE(windowsProfile.value(QStringLiteral("adaptiveNavWidth")).toInt(), 224);
    QCOMPARE(windowsProfile.value(QStringLiteral("adaptiveNavDrawerWidth")).toInt(), 248);
    QCOMPARE(windowsProfile.value(QStringLiteral("adaptiveMobileDesktopMinWidth")).toInt(), 1240);
    QCOMPARE(windowsProfile.value(QStringLiteral("adaptiveBottomNavigationMaxItems")).toInt(), 5);
    QCOMPARE(windowsProfile.value(QStringLiteral("adaptiveCompactSpacingBreakpoint")).toInt(), 920);
    QCOMPARE(windowsProfile.value(QStringLiteral("adaptiveNavRailMaxWidthRatio")).toDouble(), 0.31);
    QCOMPARE(windowsProfile.value(QStringLiteral("adaptiveDrawerMarginSafety")).toInt(), 16);
    QCOMPARE(windowsProfile.value(QStringLiteral("adaptiveDrawerEnterDuration")).toInt(), 155);
    QCOMPARE(windowsProfile.value(QStringLiteral("adaptiveDrawerExitDuration")).toInt(), 115);
    QVERIFY(windowsProfile.value(QStringLiteral("adaptiveAnimatedTransitions")).toBool());

    const QVariantMap desktopProfile = platform.runtimeProfile(QStringLiteral("linux"));
    QVERIFY(desktopProfile.value(QStringLiteral("desktop")).toBool());
    QVERIFY(!desktopProfile.value(QStringLiteral("runtimeEventsAutoAttachRecommended")).toBool());
    QVERIFY(!desktopProfile.value(QStringLiteral("mobileSystemWindowDelegationRecommended")).toBool());
    QVERIFY(!desktopProfile.value(QStringLiteral("mobileSystemInsetsDelegationRecommended")).toBool());
    QVERIFY(!desktopProfile.value(QStringLiteral("mobileDisplayCoverageOverrideRecommended")).toBool());
    QCOMPARE(desktopProfile.value(QStringLiteral("bootstrapMsaaSamples")).toInt(), 4);
    QCOMPARE(desktopProfile.value(QStringLiteral("bootstrapFramesInFlight")).toInt(), 2);
    QVERIFY(desktopProfile.value(QStringLiteral("bootstrapPartialUpdateRecommended")).toBool());
    QVERIFY(desktopProfile.value(QStringLiteral("bootstrapBatchRenderingRecommended")).toBool());
    QVERIFY(desktopProfile.value(QStringLiteral("bootstrapPipelineCacheRecommended")).toBool());
    QCOMPARE(desktopProfile.value(QStringLiteral("bootstrapTextureAtlasEdge")).toInt(), 2048);
    QCOMPARE(desktopProfile.value(QStringLiteral("adaptiveWideBreakpoint")).toInt(), 980);
    QCOMPARE(desktopProfile.value(QStringLiteral("adaptiveNavWidth")).toInt(), 220);
    QCOMPARE(desktopProfile.value(QStringLiteral("adaptiveNavDrawerWidth")).toInt(), 240);
    QCOMPARE(desktopProfile.value(QStringLiteral("adaptiveMobileDesktopMinWidth")).toInt(), 1200);
    QCOMPARE(desktopProfile.value(QStringLiteral("adaptiveBottomNavigationMaxItems")).toInt(), 5);
    QCOMPARE(desktopProfile.value(QStringLiteral("adaptiveCompactSpacingBreakpoint")).toInt(), 900);
    QCOMPARE(desktopProfile.value(QStringLiteral("adaptiveNavRailMaxWidthRatio")).toDouble(), 0.32);
    QCOMPARE(desktopProfile.value(QStringLiteral("adaptiveDrawerMarginSafety")).toInt(), 16);
    QCOMPARE(desktopProfile.value(QStringLiteral("adaptiveDrawerEnterDuration")).toInt(), 160);
    QCOMPARE(desktopProfile.value(QStringLiteral("adaptiveDrawerExitDuration")).toInt(), 120);
    QVERIFY(desktopProfile.value(QStringLiteral("adaptiveAnimatedTransitions")).toBool());

    const QVariantMap wasmProfile = platform.runtimeProfile(QStringLiteral("wasm"));
    QVERIFY(wasmProfile.value(QStringLiteral("desktop")).toBool());
    QCOMPARE(wasmProfile.value(QStringLiteral("backend")).toString(), QStringLiteral("default"));
    QCOMPARE(wasmProfile.value(QStringLiteral("bootstrapMsaaSamples")).toInt(), 2);
    QCOMPARE(wasmProfile.value(QStringLiteral("bootstrapFramesInFlight")).toInt(), 1);
    QVERIFY(!wasmProfile.value(QStringLiteral("bootstrapPartialUpdateRecommended")).toBool());
    QVERIFY(!wasmProfile.value(QStringLiteral("bootstrapBatchRenderingRecommended")).toBool());
    QVERIFY(!wasmProfile.value(QStringLiteral("bootstrapPipelineCacheRecommended")).toBool());
    QCOMPARE(wasmProfile.value(QStringLiteral("bootstrapTextureAtlasEdge")).toInt(), 1024);
    QCOMPARE(wasmProfile.value(QStringLiteral("adaptiveWideBreakpoint")).toInt(), 960);
    QCOMPARE(wasmProfile.value(QStringLiteral("adaptiveNavWidth")).toInt(), 216);
    QCOMPARE(wasmProfile.value(QStringLiteral("adaptiveNavDrawerWidth")).toInt(), 236);
    QCOMPARE(wasmProfile.value(QStringLiteral("adaptiveMobileDesktopMinWidth")).toInt(), 1120);
    QCOMPARE(wasmProfile.value(QStringLiteral("adaptiveBottomNavigationMaxItems")).toInt(), 4);
    QCOMPARE(wasmProfile.value(QStringLiteral("adaptiveCompactSpacingBreakpoint")).toInt(), 880);
    QCOMPARE(wasmProfile.value(QStringLiteral("adaptiveNavRailMaxWidthRatio")).toDouble(), 0.34);
    QCOMPARE(wasmProfile.value(QStringLiteral("adaptiveDrawerMarginSafety")).toInt(), 16);
    QCOMPARE(wasmProfile.value(QStringLiteral("adaptiveDrawerEnterDuration")).toInt(), 140);
    QCOMPARE(wasmProfile.value(QStringLiteral("adaptiveDrawerExitDuration")).toInt(), 100);
    QVERIFY(!wasmProfile.value(QStringLiteral("adaptiveAnimatedTransitions")).toBool());
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
        QVERIFY(root);
        QVERIFY(root->property("compactRule").toBool());
        QVERIFY(!root->property("expandedRule").toBool());
        QVERIFY(!root->property("unknownRule").toBool());
        QVERIFY(!root->property("mobileOversizedHeightEnabled").toBool());
        QVERIFY(root->property("backendAdaptiveContractReady").toBool());
        QTRY_VERIFY(root->property("runtimeRunning").toBool());
        QVERIFY(root->property("tokenCompliant").toBool());

        const double effectiveScale = root->property("effectiveSupersampleScale").toDouble();
        QVERIFY(effectiveScale >= 1.0);
        QVERIFY(effectiveScale <= 4.0);

        root->setProperty("width", 1400);
        root->setProperty("height", 1020);
        QTRY_VERIFY(root->property("expandedRule").toBool());
    }

    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        QString mainPath = QFINDTESTDATA("../example/VisualCatalog/qml/Main.qml");
        if (mainPath.isEmpty())
            mainPath = QFINDTESTDATA("../qml/Main.qml");
        QVERIFY2(!mainPath.isEmpty(),
                 "Failed to locate ../example/VisualCatalog/qml/Main.qml");

        QScopedPointer<QObject> root(TestUtils::loadQmlFile(engine, mainPath));
        QVERIFY(root);
        QTRY_VERIFY(root->property("metricsPass").toBool());
        QCOMPARE(root->property("metricsTotalChecks").toInt(), 6);
        QCOMPARE(root->property("metricsPassedChecks").toInt(), 6);
        QVERIFY(root->property("metricsRenderScaleCompliant").toBool());
        QVERIFY(root->property("metricsFontFallbackCompliant").toBool());
        QVERIFY(root->property("metricsThemeTextCompliant").toBool());
        QVERIFY(root->property("metricsRuntimeCompliant").toBool());
        QVERIFY(root->property("metricsSvgCompliant").toBool());
        QVERIFY(root->property("metricsPageCompliant").toBool());
        QVERIFY(root->property("metricsSummary").toString().contains('/'));
        QVERIFY(root->property("runtimeSnapshot").isValid());
        const QVariantMap snapshot = root->property("runtimeSnapshot").toMap();
        QVERIFY(snapshot.contains(QStringLiteral("pid")));
        QVERIFY(snapshot.contains(QStringLiteral("uptimeMs")));
        QVERIFY(snapshot.contains(QStringLiteral("rssBytes")));
        QTRY_VERIFY(root->property("catalogViewportReady").toBool());
        QVERIFY(root->property("catalogSafeAreaEntryReady").toBool());
        QCOMPARE(root->property("catalogComponentCount").toInt(), 54);
        QVERIFY(root->property("catalogDocumentCount").toInt() > root->property("catalogComponentCount").toInt());
        QCOMPARE(root->property("activeEntryKey").toString(), QStringLiteral("catalog-overview"));
        QVERIFY(root->property("activeEntry").isValid());
    }
}

void PlatformIntegrationTests::mobile_theme_scale_contract()
{
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());

        const QByteArray qml = R"(
import QtQuick
import LVRS as LV

Item {
    id: root
    property Item listDelegateProbe: null

    Component.onCompleted: {
        LV.Theme.targetOverride = "android-arm64"
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
        LV.Theme.effectiveTarget === "android"
        && LV.Theme.mobileTarget
        && LV.Theme.metricScaleFactor === 2.0
        && LV.Theme.typographyScaleFactor === 2.0
        && LV.Theme.gap8 === 16
        && LV.Theme.dialogMinWidth === 560
        && LV.Theme.textTitle === 52
        && LV.Theme.textTitle2 === 44
        && LV.Theme.textHeader === 34
        && LV.Theme.textHeader2 === 30
        && LV.Theme.textBody === 13
        && LV.Theme.textDescription === 24
        && LV.Theme.textCaption === 22
        && LV.Theme.scaleMetric(17) === 34
        && Math.abs(LV.Theme.scaleRealMetric(1.5) - 3.0) < 0.01
        && Math.abs(LV.Theme.scaleRealMetric(4) - 8.0) < 0.01
        && LV.Theme.scaleTextMetric(13) === 26
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

    readonly property int listDelegateBodyPixelSize: listDelegateProbe !== null
        && listDelegateProbe.contentItem.children.length > 0
        && listDelegateProbe.contentItem.children[0].children.length > 1
        ? listDelegateProbe.contentItem.children[0].children[1].font.pixelSize
        : -1

    property bool componentContract:
        listControl.listWidth === 340
        && listControl.minimumListHeight === 280
        && listControl.itemHeight === 44
        && listDelegateProbe !== null
        && listDelegateProbe.contentItem.children.length > 0
        && listDelegateBodyPixelSize === 13
        && miniListItem.iconSize === 36
        && miniListItem.rowHorizontalPadding === 8
        && miniListItem.rowVerticalPadding === 4
        && miniListItem.implicitWidth === 340
        && miniListItem.implicitHeight === 44
        && detailListItem.detailItemWidth === 388
        && detailListItem.detailContentWidth === 340
        && detailListItem.detailTopHeight === 48
        && detailListItem.detailMiddleHeight === 24
        && detailListItem.detailBottomHeight === 76
        && detailListItem.implicitWidth === 388
        && detailListItem.implicitHeight === 212
        && listFooter.horizontalPadding === 4
        && listFooter.verticalPadding === 4
        && listFooter.stockButtonPadding === 4
        && listFooter.stockButtonHeight === 44
        && listFooter.stockMenuButtonSpacing === -4
        && listFooter.implicitWidth === 172
        && listFooter.implicitHeight === 52
        && figmaLabelButton.tone === LV.AbstractButton.Primary
        && figmaIconButton.tone === LV.AbstractButton.Primary
        && figmaLabelMenuButton.tone === LV.AbstractButton.Primary
        && figmaIconMenuButton.tone === LV.AbstractButton.Primary
        && figmaLabelButton.figmaButtonHeight === 44
        && figmaIconButton.figmaButtonHeight === 44
        && figmaLabelMenuButton.figmaButtonHeight === 44
        && figmaIconMenuButton.figmaButtonHeight === 44
        && figmaLabelButton.horizontalPadding === 16
        && Math.abs(figmaLabelButton.verticalPadding - 15.5) < 0.01
        && figmaIconButton.horizontalPadding === 4
        && figmaIconButton.verticalPadding === 4
        && figmaLabelMenuButton.horizontalPadding === 16
        && figmaLabelMenuButton.verticalPadding === 4
        && figmaLabelMenuButton.spacing === -4
        && figmaIconMenuButton.horizontalPadding === 4
        && figmaIconMenuButton.verticalPadding === 4
        && figmaIconMenuButton.spacing === -4
        && figmaLabelButton.implicitWidth === 72
        && figmaIconButton.implicitWidth === 44
        && figmaLabelMenuButton.implicitWidth === 96
        && figmaIconMenuButton.implicitWidth === 76
        && figmaLabelButton.implicitHeight === 44
        && figmaIconButton.implicitHeight === 44
        && figmaLabelMenuButton.implicitHeight === 44
        && figmaIconMenuButton.implicitHeight === 44
        && figmaLabelSegment.segmentCount === 2
        && figmaLabelSegment.horizontalPadding === 8
        && Math.abs(figmaLabelSegment.verticalPadding - 7.0) < 0.01
        && figmaLabelSegment.spacing === 4
        && figmaLabelSegment.borderWidth === 4
        && figmaLabelSegment.cornerRadius === 16
        && figmaLabelSegment.implicitWidth === 164
        && figmaLabelSegment.width === 164
        && Math.abs(figmaLabelSegment.implicitHeight - 58.0) < 0.01
        && Math.abs(figmaLabelSegment.height - 58.0) < 0.01
        && labelSegmentButton0.width === 72
        && labelSegmentButton0.height === 44
        && labelSegmentButton1.x === 76
        && figmaIconSegment.segmentCount === 2
        && figmaIconSegment.horizontalPadding === 8
        && figmaIconSegment.verticalPadding === 8
        && figmaIconSegment.spacing === 4
        && figmaIconSegment.borderWidth === 4
        && figmaIconSegment.cornerRadius === 16
        && figmaIconSegment.implicitWidth === 108
        && figmaIconSegment.width === 108
        && figmaIconSegment.implicitHeight === 60
        && figmaIconSegment.height === 60
        && iconSegmentButton0.width === 44
        && iconSegmentButton0.height === 44
        && iconSegmentButton1.x === 48
        && bodyLabel.stylePixelSize === 13
        && bodyLabel.styleLineHeight === 13
        && bodyLabel.font.pixelSize === 13
        && textEditor.fontPixelSize === 13
        && textEditor.textLineHeight === 13
        && codeEditor.fontPixelSize === 13
        && codeEditor.textLineHeight === 13
        && menuItem.itemWidth === 322
        && menuItem.itemHeight === 48
        && menuItem.iconSize === 36
        && menuItem.chevronSize === 32
        && menuItem.topPadding === 6
        && menuItem.bottomPadding === 6
        && menuDivider.lineLength === 440
        && menuDivider.linePadding === 0
        && Math.abs(menuDivider.thickness - 2.0) < 0.01
        && Math.abs(menuDivider.implicitWidth - 440.0) < 0.01
        && Math.abs(menuDivider.implicitHeight - 6.0) < 0.01
        && hierarchyToolbar.minimumToolbarWidth === 400
        && hierarchyToolbar.horizontalPadding === 16
        && hierarchyToolbar.verticalPadding === 4
        && hierarchyToolbar.slotSize === 44
        && hierarchyToolbar.spacing === 0
        && !hierarchyToolbar.distributeSpacing
        && hierarchyToolbar.implicitWidth === 400
        && hierarchyToolbar.implicitHeight === 52
        && hierarchyItem.rowHeight === 40
        && hierarchyItem.itemWidth === 400
        && hierarchyItem.iconSize === 36
        && hierarchyItem.chevronSize === 32
        && hierarchyItem.baseLeftPadding === 16
        && hierarchyItem.rowRightPadding === 16
        && hierarchyItem.leadingSpacing === 4
        && hierarchyItem.cornerRadius === 10
        && hierarchyItem.implicitWidth === 400
        && hierarchyItem.implicitHeight === 40
        && hierarchyList.generatedIndentStep === 16
        && hierarchyList.generatedRowHeight === 40
        && hierarchyList.generatedItemWidth === 400
        && hierarchyList.generatedIconSize === 36
        && hierarchyList.generatedChevronSize === 32
        && hierarchyPanel.minimumPanelWidth === 400
        && hierarchyPanel.minimumPanelHeight === 1060
        && hierarchyPanel.implicitWidth === 400
        && hierarchyPanel.implicitHeight === 1060
        && checkBox.boxSize === 34
        && Math.abs(checkBox.framePadding - 1.0) < 0.01
        && Math.abs(checkBox.boxRadius - 7.0) < 0.01
        && Math.abs(checkBox.boxBorderWidthUncheckedEnabled - 1.0) < 0.01
        && checkBox.contentItem.spacing === 12
        && checkBox.implicitWidth === 81
        && checkBox.implicitHeight === 36
        && checkBox.useFigmaCheckedAssets
        && checkBox.checkedAssetSourceEnabled.toString() === LV.Theme.iconPath("checkboxCheckedEnabled").toString()
        && checkBox.checkedAssetSourceDisabled.toString() === LV.Theme.iconPath("checkboxCheckedDisabled").toString()
        && radioButton.indicatorSize === 36
        && radioButton.dotSize === 16
        && radioButton.indicatorRadius === 18
        && radioButton.dotRadius === 8
        && radioButton.contentItem.spacing === 16
        && radioButton.implicitWidth === 85
        && radioButton.implicitHeight === 36
        && radioButton.contentItem.children[0].x === 0
        && radioButton.contentItem.children[0].y === 0
        && radioButton.contentItem.children[0].children[0].x === 10
        && radioButton.contentItem.children[0].children[0].y === 10
        && Math.abs(radioButton.contentItem.children[1].x - 52.0) < 0.01
        && Math.abs(radioButton.contentItem.children[1].y - 11.5) < 0.01
        && radioButton.contentItem.children[1].font.pixelSize === 13
        && table.implicitWidth === 1056
        && table.implicitHeight === 242
        && table.rowHeight === 48
        && table.borderWidth === 2
        && String(table.backgroundColor) === "#1e1e1e"
        && table.borderColor === LV.Theme.panelBackground10
        && table.rowDividerColor === LV.Theme.panelBackground10
        && !table.structureControlsVisible
        && tableHeader.implicitWidth === 1434
        && tableHeader.implicitHeight === 50
        && tableHeader.rowHeight === 48
        && tableHeader.separatorHeight === 2
        && tableHeader.cellHorizontalPadding === 16
        && tableRow.implicitWidth === 1434
        && tableRow.implicitHeight === 48
        && tableRow.cellWidth === 468
        && tableRow.cellHeight === 48
        && tableRow.contentSpacing === 16
        && tableRow.dividerColor === LV.Theme.panelBackground10
        && tableCell.implicitWidth === 468
        && tableCell.implicitHeight === 48
        && tableCell.resolvedContentSpacing === 16
        && LV.Theme.textBody === 13
        && stepper.width === 36
        && stepper.height === 36
        && Math.abs(stepper.iconWidth - (36 * (6.43604 / 18.0))) < 0.01
        && Math.abs(stepper.iconHeight - (36 * (11.1455 / 18.0))) < 0.01
        && comboBox.width === 194
        && comboBox.height === 40
        && Math.abs(comboBox.labelBounds.x - 16.0) < 0.01
        && Math.abs(comboBox.labelBounds.y - 13.5) < 0.01
        && Math.abs(comboBox.indicatorBounds.x - 156.0) < 0.01
        && Math.abs(comboBox.indicatorBounds.y - 2.0) < 0.01
        && comboBox.indicatorBounds.width === 36
        && comboBox.indicatorBounds.height === 36

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

        QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
        QVERIFY(root);
        QTRY_VERIFY(root->property("tokenContract").toBool());
        QCOMPARE(root->property("listDelegateBodyPixelSize").toInt(), 13);
        QVERIFY2(root->property("componentContract").toBool(),
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
        QVERIFY(root);
        QTRY_VERIFY(root->property("desktopContract").toBool());
    }
}

void PlatformIntegrationTests::native_window_style_mobile_coverage_flags()
{
    NativeWindowStyle nativeWindowStyle;
    QWindow window;

    QVERIFY(nativeWindowStyle.applyMobileCoverageFlags(&window, true, true));
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    QVERIFY(window.flags().testFlag(Qt::ExpandedClientAreaHint));
    QVERIFY(window.flags().testFlag(Qt::NoTitleBarBackgroundHint));
#endif
    QVERIFY(window.flags().testFlag(Qt::MaximizeUsingFullscreenGeometryHint));

    QVERIFY(nativeWindowStyle.applyMobileCoverageFlags(&window, false, false));
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    QVERIFY(!window.flags().testFlag(Qt::ExpandedClientAreaHint));
    QVERIFY(!window.flags().testFlag(Qt::NoTitleBarBackgroundHint));
#endif
    QVERIFY(!window.flags().testFlag(Qt::MaximizeUsingFullscreenGeometryHint));
}

QTEST_MAIN(PlatformIntegrationTests)
#include "tst_platform_integration.moc"
