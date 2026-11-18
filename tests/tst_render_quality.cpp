#include "CTestSupport.h"

#include <QDir>
#include <QGuiApplication>
#include <QScopeGuard>

#include <QQuickWindow>
#include <QSize>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QFile>
#include <QQuickGraphicsConfiguration>
#include <QUrl>
#include <QtPlugin>
#include <functional>

#include "backend/runtime/renderquality.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class RenderQualityTests
{
public:
    void render_quality_bounds_and_window_apply();
    void render_quality_signal_and_global_defaults();
    void render_quality_gpu_policy_pso_texture_and_drs_contract();
};

void RenderQualityTests::render_quality_bounds_and_window_apply()
{
    RenderQuality quality;
    CTEST_CHECK(quality.vectorFirst());
    CTEST_CHECK(quality.textVectorFirst());
    CTEST_CHECK(quality.hiDpiEnabled());
    CTEST_CHECK(qFuzzyCompare(quality.hiResScale(), RenderQuality::kForcedSupersampleScale));
    CTEST_CHECK(quality.supersamplingEnabled());
    CTEST_CHECK(quality.antialiasingEnabled());
    CTEST_CHECK(quality.sceneSupersampling());
    CTEST_COMPARE(quality.framesInFlight(), 2);
    CTEST_CHECK(quality.partialUpdateEnabled());
    CTEST_CHECK(quality.batchRenderingEnabled());
    CTEST_CHECK(quality.inactiveRenderDowngradeEnabled());
    CTEST_COMPARE(quality.inactiveMsaaSamples(), 2);
    CTEST_CHECK(!quality.powerSaveActive());
    CTEST_CHECK(quality.psoCacheEnabled());
    CTEST_CHECK(quality.psoCacheLoadEnabled());
    CTEST_CHECK(quality.psoCacheSaveEnabled());
    CTEST_CHECK(!quality.psoCacheFile().isEmpty());
    CTEST_CHECK(!quality.depthBufferFor2D());
    CTEST_CHECK(quality.mipmapEnabled());
    CTEST_CHECK(quality.textureCompressionEnabled());
    CTEST_CHECK(!quality.dynamicResolutionEnabled());
    CTEST_CHECK(qFuzzyCompare(quality.dynamicResolutionScale(), RenderQuality::kForcedSupersampleScale));
    CTEST_CHECK(quality.detectedDeviceTier() >= static_cast<int>(RenderQuality::LowTier));
    CTEST_CHECK(quality.detectedDeviceTier() <= static_cast<int>(RenderQuality::HighTier));
    CTEST_COMPARE(quality.activeDeviceTier(), static_cast<int>(RenderQuality::BalancedTier));
    CTEST_CHECK(quality.sceneSupersamplePixelBudget() > 0);
    CTEST_CHECK(!quality.sceneSupersamplingActive());
    CTEST_CHECK(quality.shouldUseSceneSupersampling(640, 360));
    CTEST_CHECK(quality.shouldUseSceneSupersampling(1480, 980));
    CTEST_CHECK(!quality.shouldUseSceneSupersampling(5000, 4000));
    CTEST_COMPARE(quality.resolveLayerTextureSize(640, 360, true),
             QSize(1920, 1080));
    CTEST_COMPARE(quality.resolveLayerTextureSize(640, 360, false),
             QSize(640, 360));
    CTEST_COMPARE(quality.resolveLayerTextureSize(0, 0, true),
             QSize(3, 3));

    QQuickWindow boundWindow;
    boundWindow.resize(640, 360);
    quality.setInactiveRenderDowngradeEnabled(false);
    quality.bindWindow(&boundWindow);
    CTEST_CHECK(quality.sceneSupersamplingActive());
    boundWindow.resize(1480, 980);
    CTEST_CHECK(quality.sceneSupersamplingActive());
    boundWindow.resize(5000, 4000);
    CTEST_CHECK(!quality.sceneSupersamplingActive());
    quality.unbindWindow();
    CTEST_CHECK(!quality.sceneSupersamplingActive());

    quality.setSupersampleScale(999.0);
    CTEST_CHECK(qFuzzyCompare(quality.supersampleScale(), quality.maximumSupersampleScale()));
    quality.setSupersampleScale(0.01);
    CTEST_CHECK(qFuzzyCompare(quality.supersampleScale(), quality.minimumSupersampleScale()));

    quality.setEnabled(false);
    CTEST_COMPARE(quality.enabled(), false);
    CTEST_CHECK(qFuzzyCompare(quality.effectiveSupersampleScale(), 1.0));
    quality.setEnabled(true);
    quality.setSupersampleScale(1.0);
    CTEST_CHECK(qFuzzyCompare(quality.effectiveSupersampleScale(), RenderQuality::kForcedSupersampleScale));

    quality.setMsaaSamples(-4);
    CTEST_COMPARE(quality.msaaSamples(), 2);
    quality.setMsaaSamples(48);
    CTEST_COMPARE(quality.msaaSamples(), 16);

    quality.setMsaaSamples(8);
    quality.setNativeTextRendering(true);
    quality.setFramesInFlight(9);
    CTEST_COMPARE(quality.framesInFlight(), 3);
    quality.setFramesInFlight(0);
    CTEST_COMPARE(quality.framesInFlight(), 1);
    quality.setInactiveMsaaSamples(-1);
    CTEST_COMPARE(quality.inactiveMsaaSamples(), 0);
    quality.setInactiveMsaaSamples(6);
    CTEST_COMPARE(quality.inactiveMsaaSamples(), 6);
    quality.setInactiveRenderDowngradeEnabled(false);
    CTEST_CHECK(!quality.inactiveRenderDowngradeEnabled());
    quality.setSceneSupersampling(false);
    CTEST_CHECK(!quality.shouldUseSceneSupersampling(640, 360));
    QQuickWindow window;
    window.resize(640, 360);
    quality.applyWindow(&window);
    CTEST_CHECK(window.format().samples() >= 8);
    const QQuickGraphicsConfiguration configuration = window.graphicsConfiguration();
    CTEST_CHECK(configuration.isAutomaticPipelineCacheEnabled());
    CTEST_CHECK(!configuration.pipelineCacheLoadFile().isEmpty());
    CTEST_CHECK(!configuration.pipelineCacheSaveFile().isEmpty());
    CTEST_COMPARE(configuration.isDepthBufferEnabledFor2D(), quality.depthBufferFor2D());
    CTEST_COMPARE(QQuickWindow::textRenderType(), QQuickWindow::NativeTextRendering);
}

void RenderQualityTests::render_quality_signal_and_global_defaults()
{
    RenderQuality quality;
    int enabledCount = 0;
    const auto enabledConnection = QObject::connect(&quality, &RenderQuality::enabledChanged,
        &quality, [&] { ++enabledCount; });
    CTEST_CHECK(enabledConnection);
    int scaleCount = 0;
    const auto scaleConnection = QObject::connect(&quality, &RenderQuality::supersampleScaleChanged,
        &quality, [&] { ++scaleCount; });
    CTEST_CHECK(scaleConnection);
    int sceneCount = 0;
    const auto sceneConnection = QObject::connect(&quality, &RenderQuality::sceneSupersamplingChanged,
        &quality, [&] { ++sceneCount; });
    CTEST_CHECK(sceneConnection);
    int msaaCount = 0;
    const auto msaaConnection = QObject::connect(&quality, &RenderQuality::msaaSamplesChanged,
        &quality, [&] { ++msaaCount; });
    CTEST_CHECK(msaaConnection);
    int textCount = 0;
    const auto textConnection = QObject::connect(&quality, &RenderQuality::nativeTextRenderingChanged,
        &quality, [&] { ++textCount; });
    CTEST_CHECK(textConnection);
    int framesCount = 0;
    const auto framesConnection = QObject::connect(&quality, &RenderQuality::framesInFlightChanged,
        &quality, [&] { ++framesCount; });
    CTEST_CHECK(framesConnection);
    int partialCount = 0;
    const auto partialConnection = QObject::connect(&quality, &RenderQuality::partialUpdateEnabledChanged,
        &quality, [&] { ++partialCount; });
    CTEST_CHECK(partialConnection);
    int batchCount = 0;
    const auto batchConnection = QObject::connect(&quality, &RenderQuality::batchRenderingEnabledChanged,
        &quality, [&] { ++batchCount; });
    CTEST_CHECK(batchConnection);
    int inactiveDowngradeCount = 0;
    const auto inactiveDowngradeConnection = QObject::connect(&quality, &RenderQuality::inactiveRenderDowngradeEnabledChanged,
        &quality, [&] { ++inactiveDowngradeCount; });
    CTEST_CHECK(inactiveDowngradeConnection);
    int inactiveMsaaCount = 0;
    const auto inactiveMsaaConnection = QObject::connect(&quality, &RenderQuality::inactiveMsaaSamplesChanged,
        &quality, [&] { ++inactiveMsaaCount; });
    CTEST_CHECK(inactiveMsaaConnection);
    const auto disconnectSignals = qScopeGuard([&] {
        QObject::disconnect(enabledConnection);
        QObject::disconnect(scaleConnection);
        QObject::disconnect(sceneConnection);
        QObject::disconnect(msaaConnection);
        QObject::disconnect(textConnection);
        QObject::disconnect(framesConnection);
        QObject::disconnect(partialConnection);
        QObject::disconnect(batchConnection);
        QObject::disconnect(inactiveDowngradeConnection);
        QObject::disconnect(inactiveMsaaConnection);
    });

    quality.setEnabled(true);
    CTEST_COMPARE(enabledCount, 0);
    quality.setEnabled(false);
    CTEST_COMPARE(enabledCount, 1);

    quality.setSupersampleScale(3.0);
    CTEST_COMPARE(scaleCount, 0);
    quality.setSupersampleScale(2.5);
    CTEST_COMPARE(scaleCount, 0);

    quality.setSceneSupersampling(true);
    CTEST_COMPARE(sceneCount, 0);
    quality.setSceneSupersampling(false);
    CTEST_COMPARE(sceneCount, 1);
    quality.setSceneSupersampling(true);
    CTEST_COMPARE(sceneCount, 2);

    quality.setMsaaSamples(8);
    CTEST_COMPARE(msaaCount, 1);
    quality.setMsaaSamples(12);
    CTEST_COMPARE(msaaCount, 2);

    quality.setNativeTextRendering(true);
    CTEST_COMPARE(textCount, 0);
    quality.setNativeTextRendering(false);
    CTEST_COMPARE(textCount, 1);
    CTEST_COMPARE(quality.nativeTextRendering(), false);

    quality.setFramesInFlight(3);
    CTEST_COMPARE(framesCount, 1);
    quality.setFramesInFlight(3);
    CTEST_COMPARE(framesCount, 1);

    quality.setPartialUpdateEnabled(false);
    CTEST_COMPARE(partialCount, 1);
    quality.setBatchRenderingEnabled(false);
    CTEST_COMPARE(batchCount, 1);
    quality.setInactiveRenderDowngradeEnabled(false);
    CTEST_COMPARE(inactiveDowngradeCount, 1);
    quality.setInactiveMsaaSamples(0);
    CTEST_COMPARE(inactiveMsaaCount, 1);

    qunsetenv("QSG_RHI_FRAMES_IN_FLIGHT");
    qunsetenv("QSG_PARTIAL_UPDATE");
    qunsetenv("QSG_NO_FULL_REDRAW");
    qunsetenv("QSG_BATCH_RENDERER");
    qunsetenv("QSG_RHI_PIPELINE_CACHE_LOAD");
    qunsetenv("QSG_RHI_PIPELINE_CACHE_SAVE");

    quality.applyWindow(nullptr);
    quality.applyGlobalDefaults();

    const QSurfaceFormat applied = QSurfaceFormat::defaultFormat();
    CTEST_CHECK(applied.samples() >= 12);
    CTEST_CHECK(applied.depthBufferSize() >= 24);
    CTEST_CHECK(applied.stencilBufferSize() >= 8);
    CTEST_COMPARE(QQuickWindow::textRenderType(), QQuickWindow::QtTextRendering);
    CTEST_COMPARE(qEnvironmentVariableIntValue("QSG_RHI_FRAMES_IN_FLIGHT"), 3);
    CTEST_COMPARE(qEnvironmentVariableIntValue("QSG_RHI_PIPELINE_CACHE_LOAD"), 1);
    CTEST_COMPARE(qEnvironmentVariableIntValue("QSG_RHI_PIPELINE_CACHE_SAVE"), 1);
    CTEST_COMPARE(qEnvironmentVariableIntValue("QSG_PARTIAL_UPDATE"), 0);
    CTEST_COMPARE(qEnvironmentVariableIntValue("QSG_NO_FULL_REDRAW"), 0);
    CTEST_COMPARE(qEnvironmentVariableIntValue("QSG_BATCH_RENDERER"), 0);

}

void RenderQualityTests::render_quality_gpu_policy_pso_texture_and_drs_contract()
{
    RenderQuality quality;
    QTemporaryDir tempDir(QDir::cleanPath(
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources/ctest-render-quality-cache-XXXXXX")));
    CTEST_CHECK(tempDir.isValid());
    qInfo().noquote() << "render quality cache workspace:" << tempDir.path();

    const QString psoCacheFile = tempDir.filePath(QStringLiteral("pipeline.cache"));
    quality.setPsoCacheFile(psoCacheFile);
    quality.setPsoCacheEnabled(true);
    quality.setPsoCacheLoadEnabled(true);
    quality.setPsoCacheSaveEnabled(true);
    quality.setDepthBufferFor2D(true);

    QQuickWindow window;
    quality.applyWindow(&window);
    const QQuickGraphicsConfiguration configuration = window.graphicsConfiguration();
    CTEST_CHECK(configuration.isAutomaticPipelineCacheEnabled());
    CTEST_COMPARE(configuration.pipelineCacheLoadFile(), psoCacheFile);
    CTEST_COMPARE(configuration.pipelineCacheSaveFile(), psoCacheFile);
    CTEST_CHECK(configuration.isDepthBufferEnabledFor2D());

    const QString pngPath = tempDir.filePath(QStringLiteral("icon.png"));
    const QString compressedPath = tempDir.filePath(QStringLiteral("icon.ktx2"));

    QFile pngFile(pngPath);
    CTEST_CHECK(pngFile.open(QIODevice::WriteOnly));
    CTEST_CHECK(pngFile.write("png") > 0);
    pngFile.close();

    QFile compressedFile(compressedPath);
    CTEST_CHECK(compressedFile.open(QIODevice::WriteOnly));
    CTEST_CHECK(compressedFile.write("ktx2") > 0);
    compressedFile.close();

    const QString pngUrl = QUrl::fromLocalFile(pngPath).toString();
    const QString compressedUrl = QUrl::fromLocalFile(compressedPath).toString();
    CTEST_COMPARE(quality.resolveTextureSource(pngUrl), compressedUrl);
    quality.setTextureCompressionEnabled(false);
    CTEST_COMPARE(quality.resolveTextureSource(pngUrl), pngUrl);
    quality.setTextureCompressionEnabled(true);

    quality.applyDeviceTierPreset(static_cast<int>(RenderQuality::LowTier));
    quality.setInactiveRenderDowngradeEnabled(false);
    CTEST_COMPARE(quality.activeDeviceTier(), static_cast<int>(RenderQuality::LowTier));
    CTEST_COMPARE(quality.framesInFlight(), 1);
    CTEST_CHECK(quality.dynamicResolutionEnabled());
#if defined(Q_OS_IOS)
    CTEST_COMPARE(quality.msaaSamples(), 4);
#else
    CTEST_COMPARE(quality.msaaSamples(), 2);
#endif
    QQuickWindow lowTierWindow;
    quality.applyWindow(&lowTierWindow);
#if defined(Q_OS_IOS)
    CTEST_COMPARE(lowTierWindow.format().samples(), 4);
#else
    CTEST_COMPARE(lowTierWindow.format().samples(), 2);
#endif

    const qreal initialScale = quality.dynamicResolutionScale();
    for (int i = 0; i < 9; ++i)
        quality.sampleFrameTime(48.0);
#if defined(Q_OS_IOS)
    CTEST_CHECK(qFuzzyCompare(quality.dynamicResolutionScale(), 2.0));
#else
    CTEST_CHECK(quality.dynamicResolutionScale() < initialScale);
#endif
    const qreal downgradedScale = quality.dynamicResolutionScale();
    for (int i = 0; i < 120; ++i)
        quality.sampleFrameTime(8.0);
    CTEST_CHECK(quality.dynamicResolutionScale() >= downgradedScale);
    CTEST_CHECK(quality.dynamicResolutionScale() <= quality.dynamicResolutionMaxScale());

    quality.applyDeviceTierPreset(-1);
    CTEST_COMPARE(quality.activeDeviceTier(), quality.detectedDeviceTier());

    quality.applyDeviceTierPreset(static_cast<int>(RenderQuality::HighTier));
    CTEST_COMPARE(quality.activeDeviceTier(), static_cast<int>(RenderQuality::HighTier));
    CTEST_COMPARE(quality.framesInFlight(), 3);
    CTEST_CHECK(!quality.dynamicResolutionEnabled());
    QQuickWindow highTierWindow;
    quality.applyWindow(&highTierWindow);
    CTEST_COMPARE(highTierWindow.format().samples(), 12);

    quality.applyDeviceTierPreset(static_cast<int>(RenderQuality::UltraTier));
    CTEST_COMPARE(quality.activeDeviceTier(), static_cast<int>(RenderQuality::UltraTier));
    CTEST_COMPARE(quality.msaaSamples(), 16);
    CTEST_COMPARE(quality.framesInFlight(), 3);
    CTEST_CHECK(!quality.dynamicResolutionEnabled());
    CTEST_CHECK(!quality.textureCompressionEnabled());
    CTEST_CHECK(quality.depthBufferFor2D());
    CTEST_CHECK(!quality.inactiveRenderDowngradeEnabled());
}

namespace {
void withIsolatedWorkspace(const std::function<void()> &test)
{
    QTemporaryDir workspace(QDir::cleanPath(
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources/ctest-render-quality-XXXXXX")));
    CTEST_CHECK(workspace.isValid());
    qInfo().noquote() << "render quality workspace:" << workspace.path();

    struct SavedEnvironment { QByteArray key; bool wasSet; QByteArray value; };
    QList<SavedEnvironment> savedEnvironment;
    for (const QByteArray &key : {QByteArray("LVRS_PSO_CACHE_FILE"),
             QByteArray("QSG_RHI_FRAMES_IN_FLIGHT"), QByteArray("QSG_PARTIAL_UPDATE"),
             QByteArray("QSG_NO_FULL_REDRAW"), QByteArray("QSG_BATCH_RENDERER"),
             QByteArray("QSG_RHI_PIPELINE_CACHE_LOAD"), QByteArray("QSG_RHI_PIPELINE_CACHE_SAVE")}) {
        savedEnvironment.append({key, qEnvironmentVariableIsSet(key.constData()), qgetenv(key.constData())});
    }
    const QSurfaceFormat previousFormat = QSurfaceFormat::defaultFormat();
    const QQuickWindow::TextRenderType previousTextType = QQuickWindow::textRenderType();
    const auto restore = qScopeGuard([&] {
        QSurfaceFormat::setDefaultFormat(previousFormat);
        QQuickWindow::setTextRenderType(previousTextType);
        for (const auto &entry : savedEnvironment) {
            if (entry.wasSet) qputenv(entry.key.constData(), entry.value);
            else qunsetenv(entry.key.constData());
        }
    });
    qputenv("LVRS_PSO_CACHE_FILE", workspace.filePath(QStringLiteral("default-pipeline.cache")).toUtf8());
    test();
}
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    RenderQualityTests tests;
    runner.run("render_quality_bounds_and_window_apply", [&] { withIsolatedWorkspace([&] { tests.render_quality_bounds_and_window_apply(); }); });
    runner.run("render_quality_signal_and_global_defaults", [&] { withIsolatedWorkspace([&] { tests.render_quality_signal_and_global_defaults(); }); });
    runner.run("render_quality_gpu_policy_pso_texture_and_drs_contract", [&] { withIsolatedWorkspace([&] { tests.render_quality_gpu_policy_pso_texture_and_drs_contract(); }); });
    return runner.result();
}
