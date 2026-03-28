#include <QtTest>

#include <QScopedPointer>
#include <QQmlEngine>
#include <QtPlugin>

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
    Component.onCompleted: LV.Theme.targetOverride = "android-arm64"

    property bool tokenContract:
        LV.Theme.effectiveTarget === "android"
        && LV.Theme.mobileTarget
        && LV.Theme.metricScaleFactor === 1.5
        && LV.Theme.typographyScaleFactor === 1.5
        && LV.Theme.gap8 === 12
        && LV.Theme.dialogMinWidth === 420
        && LV.Theme.textTitle === 39
        && LV.Theme.textBody === 18
        && LV.Theme.textCaption === 17
        && LV.Theme.scaleMetric(17) === 26
        && Math.abs(LV.Theme.scaleRealMetric(1.5) - 2.25) < 0.01
        && Math.abs(LV.Theme.scaleRealMetric(4) - 6.0) < 0.01
        && LV.Theme.scaleTextMetric(13) === 20
        && LV.Theme.isThemeTextStyleCompliant(LV.Theme.textBody, LV.Theme.textBodyWeight, LV.Theme.textBodyStyleName)

    LV.List {
        id: listControl
        visible: false
    }

    LV.MenuItem {
        id: menuItem
        visible: false
    }

    LV.CheckBox {
        id: checkBox
        visible: false
    }

    property bool componentContract:
        listControl.listWidth === 255
        && listControl.minimumListHeight === 335
        && menuItem.itemWidth === 242
        && menuItem.itemHeight === 24
        && menuItem.iconSize === 24
        && checkBox.boxSize === 26
}
)";

        QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
        QVERIFY(root);
        QTRY_VERIFY(root->property("tokenContract").toBool());
        QVERIFY(root->property("componentContract").toBool());
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
        && LV.Theme.textBody === 12
        && LV.Theme.textCaption === 11
}
)";

        QScopedPointer<QObject> root(TestUtils::createFromQml(engine, qml));
        QVERIFY(root);
        QTRY_VERIFY(root->property("desktopContract").toBool());
    }
}

QTEST_MAIN(PlatformIntegrationTests)
#include "tst_platform_integration.moc"
