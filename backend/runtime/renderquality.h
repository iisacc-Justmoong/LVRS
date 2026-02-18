#pragma once

#include <QObject>
#include <QPointer>
#include <QElapsedTimer>
#include <QStringList>
#include <QSize>
#include <QtQml/qqml.h>

class QQuickWindow;

class RenderQuality : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RenderQuality)
    QML_SINGLETON

    Q_PROPERTY(bool vectorFirst READ vectorFirst CONSTANT)
    Q_PROPERTY(bool textVectorFirst READ textVectorFirst CONSTANT)
    Q_PROPERTY(bool hiDpiEnabled READ hiDpiEnabled CONSTANT)
    Q_PROPERTY(qreal hiResScale READ hiResScale CONSTANT)
    Q_PROPERTY(qreal effectiveSupersampleScaleValue READ effectiveSupersampleScale NOTIFY effectiveSupersampleScaleChanged)
    Q_PROPERTY(bool supersamplingEnabled READ supersamplingEnabled CONSTANT)
    Q_PROPERTY(bool antialiasingEnabled READ antialiasingEnabled CONSTANT)
    Q_PROPERTY(bool sceneSupersampling READ sceneSupersampling WRITE setSceneSupersampling NOTIFY sceneSupersamplingChanged)
    Q_PROPERTY(bool sceneSupersamplingActive READ sceneSupersamplingActive NOTIFY sceneSupersamplingActiveChanged)
    Q_PROPERTY(int sceneSupersamplePixelBudget READ sceneSupersamplePixelBudget CONSTANT)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(qreal supersampleScale READ supersampleScale WRITE setSupersampleScale NOTIFY supersampleScaleChanged)
    Q_PROPERTY(qreal minimumSupersampleScale READ minimumSupersampleScale CONSTANT)
    Q_PROPERTY(qreal maximumSupersampleScale READ maximumSupersampleScale CONSTANT)
    Q_PROPERTY(int msaaSamples READ msaaSamples WRITE setMsaaSamples NOTIFY msaaSamplesChanged)
    Q_PROPERTY(bool nativeTextRendering READ nativeTextRendering WRITE setNativeTextRendering NOTIFY nativeTextRenderingChanged)
    Q_PROPERTY(int framesInFlight READ framesInFlight WRITE setFramesInFlight NOTIFY framesInFlightChanged)
    Q_PROPERTY(bool partialUpdateEnabled READ partialUpdateEnabled WRITE setPartialUpdateEnabled NOTIFY partialUpdateEnabledChanged)
    Q_PROPERTY(bool batchRenderingEnabled READ batchRenderingEnabled WRITE setBatchRenderingEnabled NOTIFY batchRenderingEnabledChanged)
    Q_PROPERTY(bool inactiveRenderDowngradeEnabled READ inactiveRenderDowngradeEnabled WRITE setInactiveRenderDowngradeEnabled NOTIFY inactiveRenderDowngradeEnabledChanged)
    Q_PROPERTY(int inactiveMsaaSamples READ inactiveMsaaSamples WRITE setInactiveMsaaSamples NOTIFY inactiveMsaaSamplesChanged)
    Q_PROPERTY(bool powerSaveActive READ powerSaveActive NOTIFY powerSaveActiveChanged)
    Q_PROPERTY(bool psoCacheEnabled READ psoCacheEnabled WRITE setPsoCacheEnabled NOTIFY psoCacheEnabledChanged)
    Q_PROPERTY(bool psoCacheLoadEnabled READ psoCacheLoadEnabled WRITE setPsoCacheLoadEnabled NOTIFY psoCacheLoadEnabledChanged)
    Q_PROPERTY(bool psoCacheSaveEnabled READ psoCacheSaveEnabled WRITE setPsoCacheSaveEnabled NOTIFY psoCacheSaveEnabledChanged)
    Q_PROPERTY(QString psoCacheFile READ psoCacheFile WRITE setPsoCacheFile NOTIFY psoCacheFileChanged)
    Q_PROPERTY(bool depthBufferFor2D READ depthBufferFor2D WRITE setDepthBufferFor2D NOTIFY depthBufferFor2DChanged)
    Q_PROPERTY(bool mipmapEnabled READ mipmapEnabled WRITE setMipmapEnabled NOTIFY mipmapEnabledChanged)
    Q_PROPERTY(bool textureCompressionEnabled READ textureCompressionEnabled WRITE setTextureCompressionEnabled NOTIFY textureCompressionEnabledChanged)
    Q_PROPERTY(QStringList compressedTextureExtensions READ compressedTextureExtensions WRITE setCompressedTextureExtensions NOTIFY compressedTextureExtensionsChanged)
    Q_PROPERTY(bool dynamicResolutionEnabled READ dynamicResolutionEnabled WRITE setDynamicResolutionEnabled NOTIFY dynamicResolutionEnabledChanged)
    Q_PROPERTY(qreal dynamicResolutionScale READ dynamicResolutionScale NOTIFY dynamicResolutionScaleChanged)
    Q_PROPERTY(qreal dynamicResolutionMinScale READ dynamicResolutionMinScale WRITE setDynamicResolutionMinScale NOTIFY dynamicResolutionMinScaleChanged)
    Q_PROPERTY(qreal dynamicResolutionMaxScale READ dynamicResolutionMaxScale WRITE setDynamicResolutionMaxScale NOTIFY dynamicResolutionMaxScaleChanged)
    Q_PROPERTY(qreal dynamicResolutionStep READ dynamicResolutionStep WRITE setDynamicResolutionStep NOTIFY dynamicResolutionStepChanged)
    Q_PROPERTY(double dynamicResolutionTargetFrameMs READ dynamicResolutionTargetFrameMs WRITE setDynamicResolutionTargetFrameMs NOTIFY dynamicResolutionTargetFrameMsChanged)
    Q_PROPERTY(double dynamicResolutionHysteresisMs READ dynamicResolutionHysteresisMs WRITE setDynamicResolutionHysteresisMs NOTIFY dynamicResolutionHysteresisMsChanged)
    Q_PROPERTY(int detectedDeviceTier READ detectedDeviceTier CONSTANT)
    Q_PROPERTY(int activeDeviceTier READ activeDeviceTier NOTIFY activeDeviceTierChanged)

public:
    enum DeviceTier {
        LowTier = 0,
        BalancedTier = 1,
        HighTier = 2
    };
    Q_ENUM(DeviceTier)

    static constexpr bool kVectorFirstEnabled = true;
    static constexpr bool kTextVectorFirstEnabled = true;
    static constexpr bool kHiDpiEnabled = true;
    static constexpr qreal kForcedSupersampleScale = 3.0;
    static constexpr bool kSupersamplingEnabled = true;
    static constexpr bool kAntialiasingEnabled = true;
    static constexpr int kSceneSupersamplePixelBudget = 6000000;

    explicit RenderQuality(QObject *parent = nullptr);

    bool vectorFirst() const;
    bool textVectorFirst() const;
    bool hiDpiEnabled() const;
    qreal hiResScale() const;
    bool supersamplingEnabled() const;
    bool antialiasingEnabled() const;
    bool sceneSupersampling() const;
    void setSceneSupersampling(bool value);
    bool sceneSupersamplingActive() const;
    int sceneSupersamplePixelBudget() const;

    bool enabled() const;
    void setEnabled(bool value);

    qreal supersampleScale() const;
    void setSupersampleScale(qreal value);

    qreal minimumSupersampleScale() const;
    qreal maximumSupersampleScale() const;

    int msaaSamples() const;
    void setMsaaSamples(int value);

    bool nativeTextRendering() const;
    void setNativeTextRendering(bool value);

    int framesInFlight() const;
    void setFramesInFlight(int value);
    bool partialUpdateEnabled() const;
    void setPartialUpdateEnabled(bool value);
    bool batchRenderingEnabled() const;
    void setBatchRenderingEnabled(bool value);
    bool inactiveRenderDowngradeEnabled() const;
    void setInactiveRenderDowngradeEnabled(bool value);
    int inactiveMsaaSamples() const;
    void setInactiveMsaaSamples(int value);
    bool powerSaveActive() const;
    bool psoCacheEnabled() const;
    void setPsoCacheEnabled(bool value);
    bool psoCacheLoadEnabled() const;
    void setPsoCacheLoadEnabled(bool value);
    bool psoCacheSaveEnabled() const;
    void setPsoCacheSaveEnabled(bool value);
    QString psoCacheFile() const;
    void setPsoCacheFile(const QString &value);
    bool depthBufferFor2D() const;
    void setDepthBufferFor2D(bool value);
    bool mipmapEnabled() const;
    void setMipmapEnabled(bool value);
    bool textureCompressionEnabled() const;
    void setTextureCompressionEnabled(bool value);
    QStringList compressedTextureExtensions() const;
    void setCompressedTextureExtensions(const QStringList &value);
    bool dynamicResolutionEnabled() const;
    void setDynamicResolutionEnabled(bool value);
    qreal dynamicResolutionScale() const;
    qreal dynamicResolutionMinScale() const;
    void setDynamicResolutionMinScale(qreal value);
    qreal dynamicResolutionMaxScale() const;
    void setDynamicResolutionMaxScale(qreal value);
    qreal dynamicResolutionStep() const;
    void setDynamicResolutionStep(qreal value);
    double dynamicResolutionTargetFrameMs() const;
    void setDynamicResolutionTargetFrameMs(double value);
    double dynamicResolutionHysteresisMs() const;
    void setDynamicResolutionHysteresisMs(double value);
    int detectedDeviceTier() const;
    int activeDeviceTier() const;

    Q_INVOKABLE qreal effectiveSupersampleScale() const;
    Q_INVOKABLE bool shouldUseSceneSupersampling(int width, int height) const;
    Q_INVOKABLE QSize resolveLayerTextureSize(int width, int height, bool sceneSupersamplingActive = true) const;
    Q_INVOKABLE QString resolveTextureSource(const QString &source) const;
    Q_INVOKABLE void sampleFrameTime(double frameMs);
    Q_INVOKABLE void applyDeviceTierPreset(int tier = -1);
    Q_INVOKABLE void bindWindow(QObject *window);
    Q_INVOKABLE void unbindWindow();
    Q_INVOKABLE void applyWindow(QObject *window);
    Q_INVOKABLE void applyGlobalDefaults();

    static void configureGlobalDefaults(int msaaSamples = 4,
                                        bool nativeTextRendering = true,
                                        int framesInFlight = 2,
                                        bool partialUpdateEnabled = true,
                                        bool batchRenderingEnabled = true);

signals:
    void effectiveSupersampleScaleChanged();
    void enabledChanged();
    void supersampleScaleChanged();
    void sceneSupersamplingChanged();
    void sceneSupersamplingActiveChanged();
    void msaaSamplesChanged();
    void nativeTextRenderingChanged();
    void framesInFlightChanged();
    void partialUpdateEnabledChanged();
    void batchRenderingEnabledChanged();
    void inactiveRenderDowngradeEnabledChanged();
    void inactiveMsaaSamplesChanged();
    void powerSaveActiveChanged();
    void psoCacheEnabledChanged();
    void psoCacheLoadEnabledChanged();
    void psoCacheSaveEnabledChanged();
    void psoCacheFileChanged();
    void depthBufferFor2DChanged();
    void mipmapEnabledChanged();
    void textureCompressionEnabledChanged();
    void compressedTextureExtensionsChanged();
    void dynamicResolutionEnabledChanged();
    void dynamicResolutionScaleChanged();
    void dynamicResolutionMinScaleChanged();
    void dynamicResolutionMaxScaleChanged();
    void dynamicResolutionStepChanged();
    void dynamicResolutionTargetFrameMsChanged();
    void dynamicResolutionHysteresisMsChanged();
    void activeDeviceTierChanged();

private:
    static DeviceTier detectDeviceTierForSystem();
    void applyGraphicsConfiguration(QQuickWindow *window);
    qreal clampedDynamicScale(qreal value) const;
    void setDynamicResolutionScaleInternal(qreal value);
    void resetDynamicResolutionController();
    QString resolvedPsoCacheFile() const;
    void updateWindowPowerMode();
    void updateSceneSupersamplingActive();
    void detachWindowBinding();

    bool m_enabled = true;
    qreal m_supersampleScale = kForcedSupersampleScale;
    qreal m_minimumSupersampleScale = kForcedSupersampleScale;
    qreal m_maximumSupersampleScale = kForcedSupersampleScale;
    bool m_sceneSupersampling = true;
    bool m_sceneSupersamplingActive = false;
    int m_sceneSupersamplePixelBudget = kSceneSupersamplePixelBudget;
    int m_msaaSamples = 4;
    bool m_nativeTextRendering = true;
    int m_framesInFlight = 2;
    bool m_partialUpdateEnabled = true;
    bool m_batchRenderingEnabled = true;
    bool m_inactiveRenderDowngradeEnabled = true;
    int m_inactiveMsaaSamples = 2;
    bool m_powerSaveActive = false;
    bool m_psoCacheEnabled = true;
    bool m_psoCacheLoadEnabled = true;
    bool m_psoCacheSaveEnabled = true;
    QString m_psoCacheFile;
    bool m_depthBufferFor2D = false;
    bool m_mipmapEnabled = true;
    bool m_textureCompressionEnabled = true;
    QStringList m_compressedTextureExtensions = { QStringLiteral("ktx2"),
                                                  QStringLiteral("ktx"),
                                                  QStringLiteral("dds") };
    bool m_dynamicResolutionEnabled = false;
    qreal m_dynamicResolutionScale = kForcedSupersampleScale;
    qreal m_dynamicResolutionMinScale = 1.5;
    qreal m_dynamicResolutionMaxScale = kForcedSupersampleScale;
    qreal m_dynamicResolutionStep = 0.25;
    double m_dynamicResolutionTargetFrameMs = 16.6;
    double m_dynamicResolutionHysteresisMs = 2.0;
    int m_dynamicOverBudgetStreak = 0;
    int m_dynamicUnderBudgetStreak = 0;
    QElapsedTimer m_dynamicFrameTimer;
    int m_detectedDeviceTier = static_cast<int>(detectDeviceTierForSystem());
    int m_activeDeviceTier = static_cast<int>(BalancedTier);
    QPointer<QQuickWindow> m_boundWindow;
    QMetaObject::Connection m_boundWindowWidthConnection;
    QMetaObject::Connection m_boundWindowHeightConnection;
    QMetaObject::Connection m_boundWindowVisibilityConnection;
    QMetaObject::Connection m_boundWindowActiveConnection;
    QMetaObject::Connection m_boundWindowFrameSwappedConnection;
    QMetaObject::Connection m_boundWindowDestroyedConnection;
};
