#include "backend/runtime/renderquality.h"

#include <QDir>
#include <QFileInfo>
#include <QQuickGraphicsConfiguration>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QThread>
#include <QUrl>
#include <QWindow>
#include <QtGlobal>

#include <cmath>

namespace {

constexpr int kFramesInFlightMin = 1;
constexpr int kFramesInFlightMax = 3;
constexpr int kInactiveMsaaSamplesMax = 16;
constexpr qreal kDynamicScaleMin = 1.0;
constexpr qreal kDynamicScaleMax = 4.0;
constexpr qreal kDynamicScaleStepMin = 0.05;
constexpr qreal kDynamicScaleStepMax = 1.0;
constexpr double kDynamicFrameTargetMinMs = 8.0;
constexpr double kDynamicFrameTargetMaxMs = 50.0;
constexpr double kDynamicFrameHysteresisMinMs = 0.5;
constexpr double kDynamicFrameHysteresisMaxMs = 10.0;
constexpr int kDynamicDownshiftTriggerFrames = 3;
constexpr int kDynamicUpshiftTriggerFrames = 30;

QString normalizeTextureExtension(const QString &value)
{
    QString normalized = value.trimmed().toLower();
    while (normalized.startsWith(QLatin1Char('.')))
        normalized.remove(0, 1);
    return normalized;
}

bool extensionListContains(const QStringList &extensions, const QString &suffix)
{
    const QString normalizedSuffix = normalizeTextureExtension(suffix);
    if (normalizedSuffix.isEmpty())
        return false;

    for (const QString &item : extensions) {
        if (normalizeTextureExtension(item) == normalizedSuffix)
            return true;
    }
    return false;
}

QString pathWithoutSuffix(const QString &path)
{
    const int slashIndex = path.lastIndexOf(QLatin1Char('/'));
    const int dotIndex = path.lastIndexOf(QLatin1Char('.'));
    if (dotIndex <= slashIndex)
        return QString();
    return path.left(dotIndex);
}

QString resolveDefaultPsoCacheFilePath()
{
    const QString explicitFile = qEnvironmentVariable("LVRS_PSO_CACHE_FILE").trimmed();
    if (!explicitFile.isEmpty())
        return explicitFile;

    QString cacheDir = qEnvironmentVariable("LVRS_PSO_CACHE_DIR").trimmed();
    if (cacheDir.isEmpty())
        cacheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheDir.isEmpty())
        cacheDir = QDir::tempPath() + QStringLiteral("/lvrs-cache");

    QDir dir(cacheDir);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));
    return dir.filePath(QStringLiteral("lvrs_scenegraph_pso.cache"));
}

RenderQuality::DeviceTier inferDeviceTier()
{
    int threads = QThread::idealThreadCount();
    if (threads <= 0)
        threads = 4;

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    if (threads <= 6)
        return RenderQuality::LowTier;
    if (threads <= 8)
        return RenderQuality::BalancedTier;
    return RenderQuality::HighTier;
#else
    if (threads <= 4)
        return RenderQuality::LowTier;
    if (threads <= 10)
        return RenderQuality::BalancedTier;
    return RenderQuality::HighTier;
#endif
}

} // namespace

RenderQuality::RenderQuality(QObject *parent)
    : QObject(parent)
{
    m_psoCacheFile = resolvedPsoCacheFile();
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
    if (m_enabled == value)
        return;
    m_enabled = value;
    emit enabledChanged();
    emit effectiveSupersampleScaleChanged();
    updateSceneSupersamplingActive();
}

qreal RenderQuality::supersampleScale() const
{
    return m_supersampleScale;
}

void RenderQuality::setSupersampleScale(qreal value)
{
    if (!qIsFinite(value))
        return;
    const qreal next = qBound(m_minimumSupersampleScale, value, m_maximumSupersampleScale);
    if (qFuzzyCompare(m_supersampleScale, next))
        return;
    m_supersampleScale = next;
    emit supersampleScaleChanged();
    emit effectiveSupersampleScaleChanged();
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
    updateWindowPowerMode();
}

bool RenderQuality::nativeTextRendering() const
{
    return m_nativeTextRendering;
}

void RenderQuality::setNativeTextRendering(bool value)
{
    if (m_nativeTextRendering == value)
        return;
    m_nativeTextRendering = value;
    emit nativeTextRenderingChanged();
    QQuickWindow::setTextRenderType(m_nativeTextRendering
                                        ? QQuickWindow::NativeTextRendering
                                        : QQuickWindow::QtTextRendering);
}

int RenderQuality::framesInFlight() const
{
    return m_framesInFlight;
}

void RenderQuality::setFramesInFlight(int value)
{
    const int next = qBound(kFramesInFlightMin, value, kFramesInFlightMax);
    if (m_framesInFlight == next)
        return;
    m_framesInFlight = next;
    emit framesInFlightChanged();
}

bool RenderQuality::partialUpdateEnabled() const
{
    return m_partialUpdateEnabled;
}

void RenderQuality::setPartialUpdateEnabled(bool value)
{
    if (m_partialUpdateEnabled == value)
        return;
    m_partialUpdateEnabled = value;
    emit partialUpdateEnabledChanged();
}

bool RenderQuality::batchRenderingEnabled() const
{
    return m_batchRenderingEnabled;
}

void RenderQuality::setBatchRenderingEnabled(bool value)
{
    if (m_batchRenderingEnabled == value)
        return;
    m_batchRenderingEnabled = value;
    emit batchRenderingEnabledChanged();
}

bool RenderQuality::inactiveRenderDowngradeEnabled() const
{
    return m_inactiveRenderDowngradeEnabled;
}

void RenderQuality::setInactiveRenderDowngradeEnabled(bool value)
{
    if (m_inactiveRenderDowngradeEnabled == value)
        return;
    m_inactiveRenderDowngradeEnabled = value;
    emit inactiveRenderDowngradeEnabledChanged();
    updateWindowPowerMode();
    updateSceneSupersamplingActive();
}

int RenderQuality::inactiveMsaaSamples() const
{
    return m_inactiveMsaaSamples;
}

void RenderQuality::setInactiveMsaaSamples(int value)
{
    const int next = qBound(0, value, kInactiveMsaaSamplesMax);
    if (m_inactiveMsaaSamples == next)
        return;
    m_inactiveMsaaSamples = next;
    emit inactiveMsaaSamplesChanged();
    updateWindowPowerMode();
}

bool RenderQuality::powerSaveActive() const
{
    return m_powerSaveActive;
}

bool RenderQuality::psoCacheEnabled() const
{
    return m_psoCacheEnabled;
}

void RenderQuality::setPsoCacheEnabled(bool value)
{
    if (m_psoCacheEnabled == value)
        return;
    m_psoCacheEnabled = value;
    emit psoCacheEnabledChanged();
    applyGraphicsConfiguration(m_boundWindow);
}

bool RenderQuality::psoCacheLoadEnabled() const
{
    return m_psoCacheLoadEnabled;
}

void RenderQuality::setPsoCacheLoadEnabled(bool value)
{
    if (m_psoCacheLoadEnabled == value)
        return;
    m_psoCacheLoadEnabled = value;
    emit psoCacheLoadEnabledChanged();
    applyGraphicsConfiguration(m_boundWindow);
}

bool RenderQuality::psoCacheSaveEnabled() const
{
    return m_psoCacheSaveEnabled;
}

void RenderQuality::setPsoCacheSaveEnabled(bool value)
{
    if (m_psoCacheSaveEnabled == value)
        return;
    m_psoCacheSaveEnabled = value;
    emit psoCacheSaveEnabledChanged();
    applyGraphicsConfiguration(m_boundWindow);
}

QString RenderQuality::psoCacheFile() const
{
    return m_psoCacheFile;
}

void RenderQuality::setPsoCacheFile(const QString &value)
{
    const QString next = value.trimmed().isEmpty() ? resolvedPsoCacheFile() : value.trimmed();
    if (m_psoCacheFile == next)
        return;
    m_psoCacheFile = next;
    emit psoCacheFileChanged();
    applyGraphicsConfiguration(m_boundWindow);
}

bool RenderQuality::depthBufferFor2D() const
{
    return m_depthBufferFor2D;
}

void RenderQuality::setDepthBufferFor2D(bool value)
{
    if (m_depthBufferFor2D == value)
        return;
    m_depthBufferFor2D = value;
    emit depthBufferFor2DChanged();
    applyGraphicsConfiguration(m_boundWindow);
}

bool RenderQuality::mipmapEnabled() const
{
    return m_mipmapEnabled;
}

void RenderQuality::setMipmapEnabled(bool value)
{
    if (m_mipmapEnabled == value)
        return;
    m_mipmapEnabled = value;
    emit mipmapEnabledChanged();
}

bool RenderQuality::textureCompressionEnabled() const
{
    return m_textureCompressionEnabled;
}

void RenderQuality::setTextureCompressionEnabled(bool value)
{
    if (m_textureCompressionEnabled == value)
        return;
    m_textureCompressionEnabled = value;
    emit textureCompressionEnabledChanged();
}

QStringList RenderQuality::compressedTextureExtensions() const
{
    return m_compressedTextureExtensions;
}

void RenderQuality::setCompressedTextureExtensions(const QStringList &value)
{
    QStringList next;
    next.reserve(value.size());
    for (const QString &raw : value) {
        const QString normalized = normalizeTextureExtension(raw);
        if (normalized.isEmpty())
            continue;
        if (!next.contains(normalized))
            next.append(normalized);
    }

    if (next.isEmpty()) {
        next = { QStringLiteral("ktx2"), QStringLiteral("ktx"), QStringLiteral("dds") };
    }

    if (m_compressedTextureExtensions == next)
        return;

    m_compressedTextureExtensions = next;
    emit compressedTextureExtensionsChanged();
}

bool RenderQuality::dynamicResolutionEnabled() const
{
    return m_dynamicResolutionEnabled;
}

void RenderQuality::setDynamicResolutionEnabled(bool value)
{
    if (m_dynamicResolutionEnabled == value)
        return;
    m_dynamicResolutionEnabled = value;
    resetDynamicResolutionController();
    if (!m_dynamicResolutionEnabled)
        setDynamicResolutionScaleInternal(m_dynamicResolutionMaxScale);
    emit dynamicResolutionEnabledChanged();
}

qreal RenderQuality::dynamicResolutionScale() const
{
    return m_dynamicResolutionScale;
}

qreal RenderQuality::dynamicResolutionMinScale() const
{
    return m_dynamicResolutionMinScale;
}

void RenderQuality::setDynamicResolutionMinScale(qreal value)
{
    const qreal next = qBound(kDynamicScaleMin, value, kDynamicScaleMax);
    if (qFuzzyCompare(m_dynamicResolutionMinScale, next))
        return;

    m_dynamicResolutionMinScale = next;
    if (m_dynamicResolutionMaxScale < m_dynamicResolutionMinScale) {
        m_dynamicResolutionMaxScale = m_dynamicResolutionMinScale;
        emit dynamicResolutionMaxScaleChanged();
    }
    emit dynamicResolutionMinScaleChanged();
    setDynamicResolutionScaleInternal(m_dynamicResolutionScale);
}

qreal RenderQuality::dynamicResolutionMaxScale() const
{
    return m_dynamicResolutionMaxScale;
}

void RenderQuality::setDynamicResolutionMaxScale(qreal value)
{
    const qreal next = qBound(kDynamicScaleMin, value, kDynamicScaleMax);
    if (qFuzzyCompare(m_dynamicResolutionMaxScale, next))
        return;

    m_dynamicResolutionMaxScale = next;
    if (m_dynamicResolutionMinScale > m_dynamicResolutionMaxScale) {
        m_dynamicResolutionMinScale = m_dynamicResolutionMaxScale;
        emit dynamicResolutionMinScaleChanged();
    }
    emit dynamicResolutionMaxScaleChanged();
    setDynamicResolutionScaleInternal(m_dynamicResolutionScale);
}

qreal RenderQuality::dynamicResolutionStep() const
{
    return m_dynamicResolutionStep;
}

void RenderQuality::setDynamicResolutionStep(qreal value)
{
    const qreal next = qBound(kDynamicScaleStepMin, value, kDynamicScaleStepMax);
    if (qFuzzyCompare(m_dynamicResolutionStep, next))
        return;
    m_dynamicResolutionStep = next;
    emit dynamicResolutionStepChanged();
}

double RenderQuality::dynamicResolutionTargetFrameMs() const
{
    return m_dynamicResolutionTargetFrameMs;
}

void RenderQuality::setDynamicResolutionTargetFrameMs(double value)
{
    const double next = qBound(kDynamicFrameTargetMinMs, value, kDynamicFrameTargetMaxMs);
    if (qFuzzyCompare(m_dynamicResolutionTargetFrameMs, next))
        return;
    m_dynamicResolutionTargetFrameMs = next;
    emit dynamicResolutionTargetFrameMsChanged();
}

double RenderQuality::dynamicResolutionHysteresisMs() const
{
    return m_dynamicResolutionHysteresisMs;
}

void RenderQuality::setDynamicResolutionHysteresisMs(double value)
{
    const double next = qBound(kDynamicFrameHysteresisMinMs, value, kDynamicFrameHysteresisMaxMs);
    if (qFuzzyCompare(m_dynamicResolutionHysteresisMs, next))
        return;
    m_dynamicResolutionHysteresisMs = next;
    emit dynamicResolutionHysteresisMsChanged();
}

int RenderQuality::detectedDeviceTier() const
{
    return m_detectedDeviceTier;
}

int RenderQuality::activeDeviceTier() const
{
    return m_activeDeviceTier;
}

qreal RenderQuality::effectiveSupersampleScale() const
{
    if (!kSupersamplingEnabled || !m_enabled)
        return 1.0;

    qreal effectiveScale = m_supersampleScale;
    if (m_dynamicResolutionEnabled)
        effectiveScale = qMin(effectiveScale, m_dynamicResolutionScale);
    return qBound(1.0, effectiveScale, m_supersampleScale);
}

qreal RenderQuality::resolveSceneSupersampleScaleForSize(int width, int height) const
{
    if (!m_enabled || !kSupersamplingEnabled || !m_sceneSupersampling)
        return 1.0;

    const int resolvedWidth = qMax(1, width);
    const int resolvedHeight = qMax(1, height);
    const qreal requestedScale = effectiveSupersampleScale();
    if (requestedScale <= 1.0)
        return 1.0;

    const qreal pixelArea = static_cast<qreal>(resolvedWidth) * static_cast<qreal>(resolvedHeight);
    const qreal budget = static_cast<qreal>(m_sceneSupersamplePixelBudget);
    if (pixelArea <= 0.0 || budget <= pixelArea)
        return 1.0;

    const qreal budgetScale = std::sqrt(budget / pixelArea);
    if (!qIsFinite(budgetScale) || budgetScale <= 1.0)
        return 1.0;

    const qreal clampedScale = qBound(1.0, qMin(requestedScale, budgetScale), requestedScale);
    if (clampedScale >= kTextCompensationMinScale)
        return clampedScale;

    if (budgetScale >= kTextCompensationMinScale)
        return qMin(requestedScale, kTextCompensationMinScale);

    return clampedScale;
}

bool RenderQuality::shouldUseSceneSupersampling(int width, int height) const
{
    return resolveSceneSupersampleScaleForSize(width, height) > 1.0;
}

QSize RenderQuality::resolveLayerTextureSize(int width, int height, bool sceneSupersamplingActive) const
{
    const int baseWidth = qMax(1, width);
    const int baseHeight = qMax(1, height);
    if (!sceneSupersamplingActive)
        return QSize(baseWidth, baseHeight);

    const qreal scale = resolveSceneSupersampleScaleForSize(baseWidth, baseHeight);
    if (scale <= 1.0)
        return QSize(baseWidth, baseHeight);

    const int scaledWidth = qMax(1, qRound(static_cast<qreal>(baseWidth) * scale));
    const int scaledHeight = qMax(1, qRound(static_cast<qreal>(baseHeight) * scale));
    return QSize(scaledWidth, scaledHeight);
}

QString RenderQuality::resolveTextureSource(const QString &source) const
{
    const QString trimmed = source.trimmed();
    if (trimmed.isEmpty() || !m_textureCompressionEnabled || m_compressedTextureExtensions.isEmpty())
        return trimmed;

    auto resolveCandidatePath = [this](const QString &path) {
        const QFileInfo info(path);
        const QString suffix = normalizeTextureExtension(info.suffix());
        if (suffix.isEmpty() || extensionListContains(m_compressedTextureExtensions, suffix))
            return QString();

        const QString base = pathWithoutSuffix(path);
        if (base.isEmpty())
            return QString();

        for (const QString &rawExtension : m_compressedTextureExtensions) {
            const QString extension = normalizeTextureExtension(rawExtension);
            if (extension.isEmpty())
                continue;
            const QString candidate = base + QStringLiteral(".") + extension;
            if (QFileInfo::exists(candidate))
                return candidate;
        }
        return QString();
    };

    if (trimmed.startsWith(QStringLiteral(":/"))) {
        const QString candidate = resolveCandidatePath(trimmed);
        return candidate.isEmpty() ? trimmed : candidate;
    }

    if (trimmed.startsWith(QStringLiteral("qrc:/"))) {
        const QString qrcPath = QStringLiteral(":") + trimmed.mid(4);
        const QString candidate = resolveCandidatePath(qrcPath);
        if (candidate.isEmpty())
            return trimmed;
        return QStringLiteral("qrc") + candidate.mid(1);
    }

    const QUrl parsed = QUrl::fromUserInput(trimmed);
    if (parsed.isValid() && parsed.isLocalFile()) {
        const QString candidate = resolveCandidatePath(parsed.toLocalFile());
        return candidate.isEmpty() ? trimmed : QUrl::fromLocalFile(candidate).toString();
    }

    if (parsed.isValid() && !parsed.scheme().isEmpty())
        return trimmed;

    const QString candidate = resolveCandidatePath(trimmed);
    return candidate.isEmpty() ? trimmed : candidate;
}

void RenderQuality::sampleFrameTime(double frameMs)
{
    if (!m_dynamicResolutionEnabled || !m_enabled || m_powerSaveActive)
        return;
    if (!std::isfinite(frameMs) || frameMs <= 0.0)
        return;

    const double upperBound = m_dynamicResolutionTargetFrameMs + m_dynamicResolutionHysteresisMs;
    const double lowerBound = qMax(1.0, m_dynamicResolutionTargetFrameMs - m_dynamicResolutionHysteresisMs);

    if (frameMs > upperBound) {
        m_dynamicOverBudgetStreak += 1;
        m_dynamicUnderBudgetStreak = 0;
    } else if (frameMs < lowerBound) {
        m_dynamicUnderBudgetStreak += 1;
        m_dynamicOverBudgetStreak = 0;
    } else {
        m_dynamicOverBudgetStreak = 0;
        m_dynamicUnderBudgetStreak = 0;
        return;
    }

    if (m_dynamicOverBudgetStreak >= kDynamicDownshiftTriggerFrames) {
        setDynamicResolutionScaleInternal(m_dynamicResolutionScale - m_dynamicResolutionStep);
        m_dynamicOverBudgetStreak = 0;
    } else if (m_dynamicUnderBudgetStreak >= kDynamicUpshiftTriggerFrames) {
        setDynamicResolutionScaleInternal(m_dynamicResolutionScale + m_dynamicResolutionStep);
        m_dynamicUnderBudgetStreak = 0;
    }
}

void RenderQuality::applyDeviceTierPreset(int tier)
{
    int resolvedTier = tier;
    if (resolvedTier < static_cast<int>(LowTier) || resolvedTier > static_cast<int>(HighTier))
        resolvedTier = m_detectedDeviceTier;

    switch (static_cast<DeviceTier>(resolvedTier)) {
    case LowTier:
        setMsaaSamples(2);
        setFramesInFlight(1);
        setPartialUpdateEnabled(true);
        setBatchRenderingEnabled(true);
        setInactiveMsaaSamples(0);
        setMipmapEnabled(false);
        setTextureCompressionEnabled(true);
        setDepthBufferFor2D(false);
        setDynamicResolutionMinScale(1.25);
        setDynamicResolutionMaxScale(2.0);
        setDynamicResolutionStep(0.20);
        setDynamicResolutionTargetFrameMs(20.0);
        setDynamicResolutionHysteresisMs(2.5);
        setDynamicResolutionEnabled(true);
        break;
    case HighTier:
        setMsaaSamples(8);
        setFramesInFlight(3);
        setPartialUpdateEnabled(true);
        setBatchRenderingEnabled(true);
        setInactiveMsaaSamples(2);
        setMipmapEnabled(true);
        setTextureCompressionEnabled(true);
        setDepthBufferFor2D(false);
        setDynamicResolutionMinScale(2.0);
        setDynamicResolutionMaxScale(3.0);
        setDynamicResolutionStep(0.25);
        setDynamicResolutionTargetFrameMs(16.6);
        setDynamicResolutionHysteresisMs(2.0);
        setDynamicResolutionEnabled(false);
        break;
    case BalancedTier:
    default:
        setMsaaSamples(4);
        setFramesInFlight(2);
        setPartialUpdateEnabled(true);
        setBatchRenderingEnabled(true);
        setInactiveMsaaSamples(1);
        setMipmapEnabled(true);
        setTextureCompressionEnabled(true);
        setDepthBufferFor2D(false);
        setDynamicResolutionMinScale(1.5);
        setDynamicResolutionMaxScale(3.0);
        setDynamicResolutionStep(0.25);
        setDynamicResolutionTargetFrameMs(16.6);
        setDynamicResolutionHysteresisMs(2.0);
        setDynamicResolutionEnabled(true);
        break;
    }

    if (m_activeDeviceTier != resolvedTier) {
        m_activeDeviceTier = resolvedTier;
        emit activeDeviceTierChanged();
    }

    if (m_boundWindow)
        applyWindow(m_boundWindow);
}

void RenderQuality::bindWindow(QObject *window)
{
    auto *quickWindow = qobject_cast<QQuickWindow *>(window);
    if (!quickWindow) {
        detachWindowBinding();
        return;
    }

    if (m_boundWindow == quickWindow) {
        updateWindowPowerMode();
        updateSceneSupersamplingActive();
        return;
    }

    detachWindowBinding();
    m_boundWindow = quickWindow;
    m_boundWindowWidthConnection = connect(m_boundWindow,
                                           &QQuickWindow::widthChanged,
                                           this,
                                           [this]() {
                                               updateWindowPowerMode();
                                               updateSceneSupersamplingActive();
                                           });
    m_boundWindowHeightConnection = connect(m_boundWindow,
                                            &QQuickWindow::heightChanged,
                                            this,
                                            [this]() {
                                                updateWindowPowerMode();
                                                updateSceneSupersamplingActive();
                                            });
    m_boundWindowVisibilityConnection = connect(m_boundWindow,
                                                &QWindow::visibilityChanged,
                                                this,
                                                [this](QWindow::Visibility) {
                                                    updateWindowPowerMode();
                                                    updateSceneSupersamplingActive();
                                                });
    m_boundWindowActiveConnection = connect(m_boundWindow,
                                            &QWindow::activeChanged,
                                            this,
                                            [this]() {
                                                updateWindowPowerMode();
                                                updateSceneSupersamplingActive();
                                            });
    m_boundWindowFrameSwappedConnection = connect(m_boundWindow,
                                                  &QQuickWindow::frameSwapped,
                                                  this,
                                                  [this]() {
                                                      if (!m_dynamicResolutionEnabled || m_powerSaveActive)
                                                          return;
                                                      if (!m_dynamicFrameTimer.isValid()) {
                                                          m_dynamicFrameTimer.start();
                                                          return;
                                                      }

                                                      const qint64 elapsedMs = m_dynamicFrameTimer.restart();
                                                      if (elapsedMs > 0)
                                                          sampleFrameTime(static_cast<double>(elapsedMs));
                                                  });
    m_boundWindowDestroyedConnection = connect(m_boundWindow,
                                               &QObject::destroyed,
                                               this,
                                               [this]() { detachWindowBinding(); });
    updateWindowPowerMode();
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

    applyGraphicsConfiguration(quickWindow);

    QQuickWindow::setTextRenderType(m_nativeTextRendering
                                        ? QQuickWindow::NativeTextRendering
                                        : QQuickWindow::QtTextRendering);
    updateWindowPowerMode();
    updateSceneSupersamplingActive();
}

void RenderQuality::applyGlobalDefaults()
{
    configureGlobalDefaults(m_msaaSamples,
                            m_nativeTextRendering,
                            m_framesInFlight,
                            m_partialUpdateEnabled,
                            m_batchRenderingEnabled);
}

void RenderQuality::configureGlobalDefaults(int msaaSamples,
                                            bool nativeTextRendering,
                                            int framesInFlight,
                                            bool partialUpdateEnabled,
                                            bool batchRenderingEnabled)
{
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

    if (qEnvironmentVariableIsEmpty("QSG_RHI_FRAMES_IN_FLIGHT")) {
        const int clampedFrames = qBound(kFramesInFlightMin, framesInFlight, kFramesInFlightMax);
        qputenv("QSG_RHI_FRAMES_IN_FLIGHT", QByteArray::number(clampedFrames));
    }

    if (qEnvironmentVariableIsEmpty("QSG_RHI_PIPELINE_CACHE_LOAD"))
        qputenv("QSG_RHI_PIPELINE_CACHE_LOAD", QByteArrayLiteral("1"));
    if (qEnvironmentVariableIsEmpty("QSG_RHI_PIPELINE_CACHE_SAVE"))
        qputenv("QSG_RHI_PIPELINE_CACHE_SAVE", QByteArrayLiteral("1"));

    if (partialUpdateEnabled) {
        if (qEnvironmentVariableIsEmpty("QSG_PARTIAL_UPDATE"))
            qputenv("QSG_PARTIAL_UPDATE", QByteArrayLiteral("1"));
        if (qEnvironmentVariableIsEmpty("QSG_NO_FULL_REDRAW"))
            qputenv("QSG_NO_FULL_REDRAW", QByteArrayLiteral("1"));
    }

    if (batchRenderingEnabled) {
        if (qEnvironmentVariableIsEmpty("QSG_BATCH_RENDERER"))
            qputenv("QSG_BATCH_RENDERER", QByteArrayLiteral("1"));
        if (qEnvironmentVariableIsEmpty("QSG_ATLAS_WIDTH"))
            qputenv("QSG_ATLAS_WIDTH", QByteArrayLiteral("2048"));
        if (qEnvironmentVariableIsEmpty("QSG_ATLAS_HEIGHT"))
            qputenv("QSG_ATLAS_HEIGHT", QByteArrayLiteral("2048"));
    }

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

    QQuickWindow::setTextRenderType(nativeTextRendering
                                        ? QQuickWindow::NativeTextRendering
                                        : QQuickWindow::QtTextRendering);
}

void RenderQuality::updateWindowPowerMode()
{
    if (!m_boundWindow) {
        if (m_powerSaveActive) {
            m_powerSaveActive = false;
            emit powerSaveActiveChanged();
        }
        return;
    }

    bool nextPowerSave = false;
    if (m_inactiveRenderDowngradeEnabled) {
        const QWindow::Visibility visibility = m_boundWindow->visibility();
        nextPowerSave = (visibility == QWindow::Hidden || visibility == QWindow::Minimized);
    }

    if (m_powerSaveActive != nextPowerSave) {
        m_powerSaveActive = nextPowerSave;
        emit powerSaveActiveChanged();
        resetDynamicResolutionController();
    }

    const int minimumActiveSamples = kAntialiasingEnabled ? 2 : 0;
    const int activeSamples = qBound(minimumActiveSamples, m_msaaSamples, 16);
    const int inactiveSamples = qBound(0, m_inactiveMsaaSamples, 16);
    const int targetSamples = m_powerSaveActive ? qMin(activeSamples, inactiveSamples) : activeSamples;

    QSurfaceFormat format = m_boundWindow->format();
    if (format.samples() != targetSamples) {
        format.setSamples(targetSamples);
        m_boundWindow->setFormat(format);
    }

    m_boundWindow->setPersistentGraphics(!m_powerSaveActive);
    m_boundWindow->setPersistentSceneGraph(!m_powerSaveActive);
}

void RenderQuality::updateSceneSupersamplingActive()
{
    bool next = false;
    if (m_boundWindow && !m_powerSaveActive)
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
    if (m_boundWindowVisibilityConnection) {
        disconnect(m_boundWindowVisibilityConnection);
        m_boundWindowVisibilityConnection = QMetaObject::Connection();
    }
    if (m_boundWindowActiveConnection) {
        disconnect(m_boundWindowActiveConnection);
        m_boundWindowActiveConnection = QMetaObject::Connection();
    }
    if (m_boundWindowFrameSwappedConnection) {
        disconnect(m_boundWindowFrameSwappedConnection);
        m_boundWindowFrameSwappedConnection = QMetaObject::Connection();
    }
    if (m_boundWindowDestroyedConnection) {
        disconnect(m_boundWindowDestroyedConnection);
        m_boundWindowDestroyedConnection = QMetaObject::Connection();
    }
    m_boundWindow.clear();
    if (m_powerSaveActive) {
        m_powerSaveActive = false;
        emit powerSaveActiveChanged();
    }
    if (m_sceneSupersamplingActive) {
        m_sceneSupersamplingActive = false;
        emit sceneSupersamplingActiveChanged();
    }
    resetDynamicResolutionController();
}

RenderQuality::DeviceTier RenderQuality::detectDeviceTierForSystem()
{
    return inferDeviceTier();
}

void RenderQuality::applyGraphicsConfiguration(QQuickWindow *window)
{
    if (!window)
        return;

    QQuickGraphicsConfiguration configuration = window->graphicsConfiguration();
    configuration.setDepthBufferFor2D(m_depthBufferFor2D);
    configuration.setDebugLayer(false);
    configuration.setDebugMarkers(false);
    configuration.setTimestamps(false);
    configuration.setPreferSoftwareDevice(false);
    configuration.setAutomaticPipelineCache(m_psoCacheEnabled);

    if (m_psoCacheEnabled) {
        const QString cacheFile = resolvedPsoCacheFile();
        configuration.setPipelineCacheLoadFile(m_psoCacheLoadEnabled ? cacheFile : QString());
        configuration.setPipelineCacheSaveFile(m_psoCacheSaveEnabled ? cacheFile : QString());
    } else {
        configuration.setPipelineCacheLoadFile(QString());
        configuration.setPipelineCacheSaveFile(QString());
    }

    window->setGraphicsConfiguration(configuration);
}

qreal RenderQuality::clampedDynamicScale(qreal value) const
{
    const qreal minScale = qBound(kDynamicScaleMin, m_dynamicResolutionMinScale, kDynamicScaleMax);
    const qreal maxScale = qBound(minScale, m_dynamicResolutionMaxScale, kDynamicScaleMax);
    return qBound(minScale, value, maxScale);
}

void RenderQuality::setDynamicResolutionScaleInternal(qreal value)
{
    const qreal next = clampedDynamicScale(value);
    if (qFuzzyCompare(m_dynamicResolutionScale, next))
        return;

    m_dynamicResolutionScale = next;
    emit dynamicResolutionScaleChanged();
    emit effectiveSupersampleScaleChanged();
    updateSceneSupersamplingActive();
}

void RenderQuality::resetDynamicResolutionController()
{
    m_dynamicFrameTimer.invalidate();
    m_dynamicOverBudgetStreak = 0;
    m_dynamicUnderBudgetStreak = 0;
}

QString RenderQuality::resolvedPsoCacheFile() const
{
    const QString configured = m_psoCacheFile.trimmed();
    return configured.isEmpty() ? resolveDefaultPsoCacheFilePath() : configured;
}
