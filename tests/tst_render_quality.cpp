#include <QtTest>

#include <QQuickWindow>
#include <QSignalSpy>
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
    QCOMPARE(quality.msaaSamples(), 0);
    quality.setMsaaSamples(48);
    QCOMPARE(quality.msaaSamples(), 16);

    quality.setMsaaSamples(8);
    quality.setNativeTextRendering(true);
    QQuickWindow window;
    quality.applyWindow(&window);
    QVERIFY(window.format().samples() >= 8);
    QCOMPARE(QQuickWindow::textRenderType(), QQuickWindow::NativeTextRendering);
}

void RenderQualityTests::render_quality_signal_and_global_defaults()
{
    RenderQuality quality;
    QSignalSpy enabledSpy(&quality, &RenderQuality::enabledChanged);
    QSignalSpy scaleSpy(&quality, &RenderQuality::supersampleScaleChanged);
    QSignalSpy msaaSpy(&quality, &RenderQuality::msaaSamplesChanged);
    QSignalSpy textSpy(&quality, &RenderQuality::nativeTextRenderingChanged);
    QVERIFY(enabledSpy.isValid());
    QVERIFY(scaleSpy.isValid());
    QVERIFY(msaaSpy.isValid());
    QVERIFY(textSpy.isValid());

    quality.setEnabled(true);
    QCOMPARE(enabledSpy.count(), 0);
    quality.setEnabled(false);
    QCOMPARE(enabledSpy.count(), 0);

    quality.setSupersampleScale(3.0);
    QCOMPARE(scaleSpy.count(), 0);
    quality.setSupersampleScale(2.5);
    QCOMPARE(scaleSpy.count(), 0);

    quality.setMsaaSamples(8);
    QCOMPARE(msaaSpy.count(), 0);
    quality.setMsaaSamples(12);
    QCOMPARE(msaaSpy.count(), 1);

    quality.setNativeTextRendering(true);
    QCOMPARE(textSpy.count(), 0);
    quality.setNativeTextRendering(false);
    QCOMPARE(textSpy.count(), 1);

    const QSurfaceFormat previousFormat = QSurfaceFormat::defaultFormat();
    const QQuickWindow::TextRenderType previousTextType = QQuickWindow::textRenderType();

    quality.applyWindow(nullptr);
    QTest::ignoreMessage(QtWarningMsg,
                         "setHighDpiScaleFactorRoundingPolicy must be called before creating the QGuiApplication instance");
    quality.applyGlobalDefaults();

    const QSurfaceFormat applied = QSurfaceFormat::defaultFormat();
    QVERIFY(applied.samples() >= 12);
    QVERIFY(applied.depthBufferSize() >= 24);
    QVERIFY(applied.stencilBufferSize() >= 8);
    QCOMPARE(QQuickWindow::textRenderType(), QQuickWindow::QtTextRendering);

    QSurfaceFormat::setDefaultFormat(previousFormat);
    QQuickWindow::setTextRenderType(previousTextType);
}

QTEST_MAIN(RenderQualityTests)
#include "tst_render_quality.moc"
