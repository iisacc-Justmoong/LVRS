#pragma once

#include <QObject>
#include <QPointer>
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
    Q_PROPERTY(qreal effectiveSupersampleScaleValue READ effectiveSupersampleScale CONSTANT)
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

public:
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

    Q_INVOKABLE qreal effectiveSupersampleScale() const;
    Q_INVOKABLE bool shouldUseSceneSupersampling(int width, int height) const;
    Q_INVOKABLE QSize resolveLayerTextureSize(int width, int height, bool sceneSupersamplingActive = true) const;
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

private:
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
    QPointer<QQuickWindow> m_boundWindow;
    QMetaObject::Connection m_boundWindowWidthConnection;
    QMetaObject::Connection m_boundWindowHeightConnection;
    QMetaObject::Connection m_boundWindowVisibilityConnection;
    QMetaObject::Connection m_boundWindowActiveConnection;
    QMetaObject::Connection m_boundWindowDestroyedConnection;
};
