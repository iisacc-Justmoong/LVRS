#include <QtTest>

#include <QQuickWindow>
#include <QSignalSpy>
#include <QSize>
#include <QSurfaceFormat>
#include <QtPlugin>

#include "backend/runtime/renderquality.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class RenderQualityTests : public QObject
{
    Q_OBJECT

private slots:
    void render_quality_bounds_and_window_apply();
    void render_quality_signal_and_global_defaults();
};

void RenderQualityTests::render_quality_bounds_and_window_apply()
{
    RenderQuality quality;
    QVERIFY(quality.vectorFirst());
    QVERIFY(quality.textVectorFirst());
    QVERIFY(quality.hiDpiEnabled());
    QCOMPARE(quality.hiResScale(), RenderQuality::kForcedSupersampleScale);
    QVERIFY(quality.supersamplingEnabled());
    QVERIFY(quality.antialiasingEnabled());
    QVERIFY(quality.sceneSupersampling());
    QCOMPARE(quality.framesInFlight(), 2);
    QVERIFY(quality.partialUpdateEnabled());
    QVERIFY(quality.batchRenderingEnabled());
    QVERIFY(quality.inactiveRenderDowngradeEnabled());
    QCOMPARE(quality.inactiveMsaaSamples(), 2);
    QVERIFY(!quality.powerSaveActive());
    QVERIFY(quality.sceneSupersamplePixelBudget() > 0);
    QVERIFY(!quality.sceneSupersamplingActive());
    QVERIFY(quality.shouldUseSceneSupersampling(640, 360));
    QVERIFY(!quality.shouldUseSceneSupersampling(1480, 980));
    QCOMPARE(quality.resolveLayerTextureSize(640, 360, true),
             QSize(1920, 1080));
    QCOMPARE(quality.resolveLayerTextureSize(640, 360, false),
             QSize(640, 360));
    QCOMPARE(quality.resolveLayerTextureSize(0, 0, true),
             QSize(3, 3));

    QQuickWindow boundWindow;
    boundWindow.resize(640, 360);
    quality.setInactiveRenderDowngradeEnabled(false);
    quality.bindWindow(&boundWindow);
    QVERIFY(quality.sceneSupersamplingActive());
    boundWindow.resize(1480, 980);
    QVERIFY(!quality.sceneSupersamplingActive());
    quality.unbindWindow();
    QVERIFY(!quality.sceneSupersamplingActive());

    quality.setSupersampleScale(999.0);
    QCOMPARE(quality.supersampleScale(), quality.maximumSupersampleScale());
    quality.setSupersampleScale(0.01);
    QCOMPARE(quality.supersampleScale(), quality.minimumSupersampleScale());

    quality.setEnabled(false);
    QCOMPARE(quality.enabled(), true);
    QCOMPARE(quality.effectiveSupersampleScale(), RenderQuality::kForcedSupersampleScale);
    quality.setEnabled(true);
    quality.setSupersampleScale(1.0);
    QCOMPARE(quality.effectiveSupersampleScale(), RenderQuality::kForcedSupersampleScale);

    quality.setMsaaSamples(-4);
    QCOMPARE(quality.msaaSamples(), 2);
    quality.setMsaaSamples(48);
    QCOMPARE(quality.msaaSamples(), 16);

    quality.setMsaaSamples(8);
    quality.setNativeTextRendering(true);
    quality.setFramesInFlight(9);
    QCOMPARE(quality.framesInFlight(), 3);
    quality.setFramesInFlight(0);
    QCOMPARE(quality.framesInFlight(), 1);
    quality.setInactiveMsaaSamples(-1);
    QCOMPARE(quality.inactiveMsaaSamples(), 0);
    quality.setInactiveMsaaSamples(6);
    QCOMPARE(quality.inactiveMsaaSamples(), 6);
    quality.setInactiveRenderDowngradeEnabled(false);
    QVERIFY(!quality.inactiveRenderDowngradeEnabled());
    quality.setSceneSupersampling(false);
    QVERIFY(!quality.shouldUseSceneSupersampling(640, 360));
    QQuickWindow window;
    window.resize(640, 360);
    quality.applyWindow(&window);
    QVERIFY(window.format().samples() >= 8);
    QCOMPARE(QQuickWindow::textRenderType(), QQuickWindow::NativeTextRendering);
}

void RenderQualityTests::render_quality_signal_and_global_defaults()
{
    RenderQuality quality;
    QSignalSpy enabledSpy(&quality, &RenderQuality::enabledChanged);
    QSignalSpy scaleSpy(&quality, &RenderQuality::supersampleScaleChanged);
    QSignalSpy sceneSpy(&quality, &RenderQuality::sceneSupersamplingChanged);
    QSignalSpy msaaSpy(&quality, &RenderQuality::msaaSamplesChanged);
    QSignalSpy textSpy(&quality, &RenderQuality::nativeTextRenderingChanged);
    QSignalSpy framesSpy(&quality, &RenderQuality::framesInFlightChanged);
    QSignalSpy partialSpy(&quality, &RenderQuality::partialUpdateEnabledChanged);
    QSignalSpy batchSpy(&quality, &RenderQuality::batchRenderingEnabledChanged);
    QSignalSpy inactiveDowngradeSpy(&quality, &RenderQuality::inactiveRenderDowngradeEnabledChanged);
    QSignalSpy inactiveMsaaSpy(&quality, &RenderQuality::inactiveMsaaSamplesChanged);
    QVERIFY(enabledSpy.isValid());
    QVERIFY(scaleSpy.isValid());
    QVERIFY(sceneSpy.isValid());
    QVERIFY(msaaSpy.isValid());
    QVERIFY(textSpy.isValid());
    QVERIFY(framesSpy.isValid());
    QVERIFY(partialSpy.isValid());
    QVERIFY(batchSpy.isValid());
    QVERIFY(inactiveDowngradeSpy.isValid());
    QVERIFY(inactiveMsaaSpy.isValid());

    quality.setEnabled(true);
    QCOMPARE(enabledSpy.count(), 0);
    quality.setEnabled(false);
    QCOMPARE(enabledSpy.count(), 0);

    quality.setSupersampleScale(3.0);
    QCOMPARE(scaleSpy.count(), 0);
    quality.setSupersampleScale(2.5);
    QCOMPARE(scaleSpy.count(), 0);

    quality.setSceneSupersampling(true);
    QCOMPARE(sceneSpy.count(), 0);
    quality.setSceneSupersampling(false);
    QCOMPARE(sceneSpy.count(), 1);
    quality.setSceneSupersampling(true);
    QCOMPARE(sceneSpy.count(), 2);

    quality.setMsaaSamples(8);
    QCOMPARE(msaaSpy.count(), 1);
    quality.setMsaaSamples(12);
    QCOMPARE(msaaSpy.count(), 2);

    quality.setNativeTextRendering(true);
    QCOMPARE(textSpy.count(), 0);
    quality.setNativeTextRendering(false);
    QCOMPARE(textSpy.count(), 0);
    QCOMPARE(quality.nativeTextRendering(), true);

    quality.setFramesInFlight(3);
    QCOMPARE(framesSpy.count(), 1);
    quality.setFramesInFlight(3);
    QCOMPARE(framesSpy.count(), 1);

    quality.setPartialUpdateEnabled(false);
    QCOMPARE(partialSpy.count(), 1);
    quality.setBatchRenderingEnabled(false);
    QCOMPARE(batchSpy.count(), 1);
    quality.setInactiveRenderDowngradeEnabled(false);
    QCOMPARE(inactiveDowngradeSpy.count(), 1);
    quality.setInactiveMsaaSamples(0);
    QCOMPARE(inactiveMsaaSpy.count(), 1);

    qunsetenv("QSG_RHI_FRAMES_IN_FLIGHT");
    qunsetenv("QSG_PARTIAL_UPDATE");
    qunsetenv("QSG_NO_FULL_REDRAW");
    qunsetenv("QSG_BATCH_RENDERER");

    const QSurfaceFormat previousFormat = QSurfaceFormat::defaultFormat();
    const QQuickWindow::TextRenderType previousTextType = QQuickWindow::textRenderType();

    quality.applyWindow(nullptr);
    quality.applyGlobalDefaults();

    const QSurfaceFormat applied = QSurfaceFormat::defaultFormat();
    QVERIFY(applied.samples() >= 12);
    QVERIFY(applied.depthBufferSize() >= 24);
    QVERIFY(applied.stencilBufferSize() >= 8);
    QCOMPARE(QQuickWindow::textRenderType(), QQuickWindow::NativeTextRendering);
    QCOMPARE(qEnvironmentVariableIntValue("QSG_RHI_FRAMES_IN_FLIGHT"), 3);
    QVERIFY(!qEnvironmentVariableIsSet("QSG_PARTIAL_UPDATE"));
    QVERIFY(!qEnvironmentVariableIsSet("QSG_BATCH_RENDERER"));

    QSurfaceFormat::setDefaultFormat(previousFormat);
    QQuickWindow::setTextRenderType(previousTextType);
}

QTEST_MAIN(RenderQualityTests)
#include "tst_render_quality.moc"
