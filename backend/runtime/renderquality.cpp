#include "backend/runtime/renderquality.h"

#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QtGlobal>

RenderQuality::RenderQuality(QObject *parent)
    : QObject(parent)
{
}

bool RenderQuality::vectorFirst() const
{
    return kVectorFirstEnabled;
}

bool RenderQuality::textVectorFirst() const
{
    return kTextVectorFirstEnabled;
}

bool RenderQuality::hiDpiEnabled() const
{
    return kHiDpiEnabled;
}

qreal RenderQuality::hiResScale() const
{
    return kForcedSupersampleScale;
}

bool RenderQuality::supersamplingEnabled() const
{
    return kSupersamplingEnabled;
}

bool RenderQuality::antialiasingEnabled() const
{
    return kAntialiasingEnabled;
}

bool RenderQuality::sceneSupersampling() const
{
    return m_sceneSupersampling;
}

void RenderQuality::setSceneSupersampling(bool value)
{
    if (m_sceneSupersampling == value)
        return;
    m_sceneSupersampling = value;
    emit sceneSupersamplingChanged();
    updateSceneSupersamplingActive();
}

bool RenderQuality::sceneSupersamplingActive() const
{
    return m_sceneSupersamplingActive;
}

int RenderQuality::sceneSupersamplePixelBudget() const
{
    return m_sceneSupersamplePixelBudget;
}

bool RenderQuality::enabled() const
{
    return m_enabled;
}

void RenderQuality::setEnabled(bool value)
{
    Q_UNUSED(value)

    // HiRes is enforced framework-wide at renderer policy level.
    const bool next = true;
    if (m_enabled == next)
        return;
    m_enabled = next;
    emit enabledChanged();
    updateSceneSupersamplingActive();
}

qreal RenderQuality::supersampleScale() const
{
    return m_supersampleScale;
}

void RenderQuality::setSupersampleScale(qreal value)
{
    Q_UNUSED(value)

    // Supersample scale is fixed to @3x.
    const qreal next = kForcedSupersampleScale;
    if (qFuzzyCompare(m_supersampleScale, next))
        return;
    m_supersampleScale = next;
    emit supersampleScaleChanged();
    updateSceneSupersamplingActive();
}

qreal RenderQuality::minimumSupersampleScale() const
{
    return m_minimumSupersampleScale;
}

qreal RenderQuality::maximumSupersampleScale() const
{
    return m_maximumSupersampleScale;
}

int RenderQuality::msaaSamples() const
{
    return m_msaaSamples;
}

void RenderQuality::setMsaaSamples(int value)
{
    const int minimumSamples = kAntialiasingEnabled ? 2 : 0;
    const int next = qBound(minimumSamples, value, 16);
    if (m_msaaSamples == next)
        return;
    m_msaaSamples = next;
    emit msaaSamplesChanged();
}

bool RenderQuality::nativeTextRendering() const
{
    return m_nativeTextRendering;
}

void RenderQuality::setNativeTextRendering(bool value)
{
    Q_UNUSED(value)

    const bool next = kTextVectorFirstEnabled;
    if (m_nativeTextRendering == next)
        return;
    m_nativeTextRendering = next;
    emit nativeTextRenderingChanged();
}

qreal RenderQuality::effectiveSupersampleScale() const
{
    if (!kSupersamplingEnabled || !m_enabled)
        return 1.0;
    return kForcedSupersampleScale;
}

bool RenderQuality::shouldUseSceneSupersampling(int width, int height) const
{
    if (!m_enabled || !kSupersamplingEnabled || !m_sceneSupersampling)
        return false;
    if (width <= 0 || height <= 0)
        return false;

    const qreal scale = effectiveSupersampleScale();
    if (scale <= 1.0)
        return false;

    const qreal pixelCost = static_cast<qreal>(width) * static_cast<qreal>(height) * scale * scale;
    return pixelCost <= static_cast<qreal>(m_sceneSupersamplePixelBudget);
}

QSize RenderQuality::resolveLayerTextureSize(int width, int height, bool sceneSupersamplingActive) const
{
    const int baseWidth = qMax(1, width);
    const int baseHeight = qMax(1, height);
    if (!sceneSupersamplingActive)
        return QSize(baseWidth, baseHeight);

    const qreal scale = effectiveSupersampleScale();
    if (scale <= 1.0)
        return QSize(baseWidth, baseHeight);

    const int scaledWidth = qMax(1, qRound(static_cast<qreal>(baseWidth) * scale));
    const int scaledHeight = qMax(1, qRound(static_cast<qreal>(baseHeight) * scale));
    return QSize(scaledWidth, scaledHeight);
}

void RenderQuality::bindWindow(QObject *window)
{
    auto *quickWindow = qobject_cast<QQuickWindow *>(window);
    if (!quickWindow) {
        detachWindowBinding();
        return;
    }

    if (m_boundWindow == quickWindow) {
        updateSceneSupersamplingActive();
        return;
    }

    detachWindowBinding();
    m_boundWindow = quickWindow;
    m_boundWindowWidthConnection = connect(m_boundWindow,
                                           &QQuickWindow::widthChanged,
                                           this,
                                           [this]() { updateSceneSupersamplingActive(); });
    m_boundWindowHeightConnection = connect(m_boundWindow,
                                            &QQuickWindow::heightChanged,
                                            this,
                                            [this]() { updateSceneSupersamplingActive(); });
    m_boundWindowDestroyedConnection = connect(m_boundWindow,
                                               &QObject::destroyed,
                                               this,
                                               [this]() { detachWindowBinding(); });
    updateSceneSupersamplingActive();
}

void RenderQuality::unbindWindow()
{
    detachWindowBinding();
}

void RenderQuality::applyWindow(QObject *window)
{
    auto *quickWindow = qobject_cast<QQuickWindow *>(window);
    if (!quickWindow)
        return;

    bindWindow(quickWindow);

    const int minimumSamples = kAntialiasingEnabled ? 2 : 0;
    const int samples = qBound(minimumSamples, m_msaaSamples, 16);
    QSurfaceFormat format = quickWindow->format();
    if (format.samples() < samples) {
        format.setSamples(samples);
        quickWindow->setFormat(format);
    }

    quickWindow->setPersistentGraphics(true);
    quickWindow->setPersistentSceneGraph(true);
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
    updateSceneSupersamplingActive();
}

void RenderQuality::applyGlobalDefaults()
{
    configureGlobalDefaults(m_msaaSamples, kTextVectorFirstEnabled);
}

void RenderQuality::configureGlobalDefaults(int msaaSamples, bool nativeTextRendering)
{
    Q_UNUSED(nativeTextRendering)

    if (kHiDpiEnabled) {
        qputenv("QT_ENABLE_HIGHDPI_SCALING", QByteArrayLiteral("1"));
        qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", QByteArrayLiteral("PassThrough"));
    }

    if (kAntialiasingEnabled)
        qputenv("QSG_ANTIALIASING_METHOD", QByteArrayLiteral("msaa"));

    if (qEnvironmentVariableIsEmpty("QSG_RHI_PREFER_SOFTWARE_RENDERER"))
        qputenv("QSG_RHI_PREFER_SOFTWARE_RENDERER", QByteArrayLiteral("0"));
    if (qEnvironmentVariableIsEmpty("QSG_RENDER_LOOP"))
        qputenv("QSG_RENDER_LOOP", QByteArrayLiteral("threaded"));

    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    const int minimumSamples = kAntialiasingEnabled ? 2 : 0;
    const int samples = qBound(minimumSamples, msaaSamples, 16);
    if (format.samples() < samples)
        format.setSamples(samples);
    if (format.depthBufferSize() < 24)
        format.setDepthBufferSize(24);
    if (format.stencilBufferSize() < 8)
        format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);
}

void RenderQuality::updateSceneSupersamplingActive()
{
    bool next = false;
    if (m_boundWindow)
        next = shouldUseSceneSupersampling(m_boundWindow->width(), m_boundWindow->height());
    if (m_sceneSupersamplingActive == next)
        return;
    m_sceneSupersamplingActive = next;
    emit sceneSupersamplingActiveChanged();
}

void RenderQuality::detachWindowBinding()
{
    if (m_boundWindowWidthConnection) {
        disconnect(m_boundWindowWidthConnection);
        m_boundWindowWidthConnection = QMetaObject::Connection();
    }
    if (m_boundWindowHeightConnection) {
        disconnect(m_boundWindowHeightConnection);
        m_boundWindowHeightConnection = QMetaObject::Connection();
    }
    if (m_boundWindowDestroyedConnection) {
        disconnect(m_boundWindowDestroyedConnection);
        m_boundWindowDestroyedConnection = QMetaObject::Connection();
    }
    m_boundWindow.clear();
    if (m_sceneSupersamplingActive) {
        m_sceneSupersamplingActive = false;
        emit sceneSupersamplingActiveChanged();
    }
}
