#include "backend/runtime/renderingmonitor.h"

#include <algorithm>
#include <cmath>
#include <QDateTime>
#include <QQuickWindow>

namespace {

constexpr int kPercentileRecomputeFrameInterval = 8;

} // namespace

RenderingMonitor::RenderingMonitor(QObject *parent)
    : QObject(parent)
{
    m_recentFrameSamplesMs.reserve(m_frameSampleCapacity);
}

void RenderingMonitor::attachWindow(QObject *window)
{
    if (m_window)
        detachWindow();

    auto *quickWindow = qobject_cast<QQuickWindow *>(window);
    if (!quickWindow) {
        resetMetrics();
        setActive(false);
        return;
    }

    m_window = quickWindow;
    connect(m_window, &QObject::destroyed, this, &RenderingMonitor::handleWindowDestroyed);
    connect(m_window, &QQuickWindow::frameSwapped, this, &RenderingMonitor::handleFrameSwapped);

    resetMetrics();
    setActive(true);
}

void RenderingMonitor::start()
{
    if (m_active)
        return;
    setActive(true);
}

void RenderingMonitor::stop()
{
    if (!m_active)
        return;
    setActive(false);
}

void RenderingMonitor::reset()
{
    resetMetrics();
}

QVariantMap RenderingMonitor::performanceSnapshot() const
{
    QVariantMap snapshot;
    snapshot.insert(QStringLiteral("schema"), QStringLiteral("lvrs.performance.v1"));
    snapshot.insert(QStringLiteral("component"), QStringLiteral("RenderMonitor"));
    snapshot.insert(QStringLiteral("epochMs"), QVariant::fromValue(QDateTime::currentMSecsSinceEpoch()));
    snapshot.insert(QStringLiteral("active"), m_active);
    snapshot.insert(QStringLiteral("fps"), m_fps);
    snapshot.insert(QStringLiteral("lastFrameMs"), m_lastFrameMs);
    snapshot.insert(QStringLiteral("avgFrameMs"), m_avgFrameMs);
    snapshot.insert(QStringLiteral("p95FrameMs"), m_p95FrameMs);
    snapshot.insert(QStringLiteral("p99FrameMs"), m_p99FrameMs);
    snapshot.insert(QStringLiteral("frameCount"), QVariant::fromValue(m_frameCount));
    snapshot.insert(QStringLiteral("droppedFrameCount"), QVariant::fromValue(m_droppedFrameCount));
    snapshot.insert(QStringLiteral("droppedFrameThresholdMs"), m_droppedFrameThresholdMs);
    snapshot.insert(QStringLiteral("recentSampleCount"), m_sampleCount);
    snapshot.insert(QStringLiteral("frameSampleCapacity"), m_frameSampleCapacity);
    return snapshot;
}

bool RenderingMonitor::active() const
{
    return m_active;
}

double RenderingMonitor::fps() const
{
    return m_fps;
}

double RenderingMonitor::lastFrameMs() const
{
    return m_lastFrameMs;
}

double RenderingMonitor::avgFrameMs() const
{
    return m_avgFrameMs;
}

double RenderingMonitor::p95FrameMs() const
{
    return m_p95FrameMs;
}

double RenderingMonitor::p99FrameMs() const
{
    return m_p99FrameMs;
}

quint64 RenderingMonitor::droppedFrameCount() const
{
    return m_droppedFrameCount;
}

double RenderingMonitor::droppedFrameThresholdMs() const
{
    return m_droppedFrameThresholdMs;
}

void RenderingMonitor::setDroppedFrameThresholdMs(double value)
{
    const double next = qBound(5.0, value, 100.0);
    if (qFuzzyCompare(m_droppedFrameThresholdMs, next))
        return;

    m_droppedFrameThresholdMs = next;
    m_droppedFrameCount = 0;
    const QVector<double> values = sampleValues();
    for (double sample : values) {
        if (sample >= m_droppedFrameThresholdMs)
            m_droppedFrameCount += 1;
    }

    emit droppedFrameThresholdMsChanged();
    emit statsChanged();
}

int RenderingMonitor::frameSampleCapacity() const
{
    return m_frameSampleCapacity;
}

void RenderingMonitor::setFrameSampleCapacity(int value)
{
    const int next = qBound(16, value, 2000);
    if (m_frameSampleCapacity == next)
        return;

    QVector<double> values = sampleValues();
    if (values.size() > next)
        values = values.mid(values.size() - next);

    m_frameSampleCapacity = next;
    m_recentFrameSamplesMs = values;
    m_recentFrameSamplesMs.reserve(m_frameSampleCapacity);
    m_sampleCount = m_recentFrameSamplesMs.size();
    m_nextSampleIndex = m_sampleCount < m_frameSampleCapacity ? m_sampleCount : 0;

    m_sampleSumMs = 0.0;
    for (double sample : m_recentFrameSamplesMs)
        m_sampleSumMs += sample;
    m_avgFrameMs = m_sampleCount > 0
        ? (m_sampleSumMs / static_cast<double>(m_sampleCount))
        : 0.0;

    m_percentilesDirty = true;
    m_framesSincePercentileUpdate = kPercentileRecomputeFrameInterval;
    updatePercentiles(true);

    emit frameSampleCapacityChanged();
    emit statsChanged();
}

int RenderingMonitor::recentSampleCount() const
{
    return m_sampleCount;
}

quint64 RenderingMonitor::frameCount() const
{
    return m_frameCount;
}

void RenderingMonitor::handleFrameSwapped()
{
    if (!m_active)
        return;

    if (!m_frameTimer.isValid()) {
        m_frameTimer.start();
        m_frameCount = 0;
    }

    const qint64 elapsed = m_frameTimer.restart();
    if (elapsed > 0) {
        m_lastFrameMs = static_cast<double>(elapsed);
        m_fps = 1000.0 / m_lastFrameMs;
        appendFrameSample(m_lastFrameMs);
        if (m_lastFrameMs >= m_droppedFrameThresholdMs)
            m_droppedFrameCount += 1;
        updatePercentiles(false);
    }
    m_frameCount += 1;

    emit statsChanged();
}

void RenderingMonitor::handleWindowDestroyed()
{
    detachWindow();
    setActive(false);
}

void RenderingMonitor::setActive(bool next)
{
    if (m_active == next)
        return;
    m_active = next;
    emit activeChanged();
}

void RenderingMonitor::resetMetrics()
{
    m_frameTimer.invalidate();
    m_fps = 0.0;
    m_lastFrameMs = 0.0;
    m_avgFrameMs = 0.0;
    m_p95FrameMs = 0.0;
    m_p99FrameMs = 0.0;
    m_droppedFrameCount = 0;
    m_recentFrameSamplesMs.clear();
    m_recentFrameSamplesMs.reserve(m_frameSampleCapacity);
    m_sampleCount = 0;
    m_nextSampleIndex = 0;
    m_sampleSumMs = 0.0;
    m_framesSincePercentileUpdate = 0;
    m_percentilesDirty = false;
    m_frameCount = 0;
    emit statsChanged();
}

void RenderingMonitor::detachWindow()
{
    if (!m_window)
        return;
    m_window->disconnect(this);
    m_window.clear();
}

void RenderingMonitor::appendFrameSample(double sampleMs)
{
    if (m_frameSampleCapacity <= 0)
        return;

    if (m_recentFrameSamplesMs.size() < m_frameSampleCapacity) {
        m_recentFrameSamplesMs.append(sampleMs);
        m_sampleCount = m_recentFrameSamplesMs.size();
        m_nextSampleIndex = m_sampleCount < m_frameSampleCapacity ? m_sampleCount : 0;
        m_sampleSumMs += sampleMs;
    } else {
        const double replaced = m_recentFrameSamplesMs.at(m_nextSampleIndex);
        m_recentFrameSamplesMs[m_nextSampleIndex] = sampleMs;
        m_nextSampleIndex = (m_nextSampleIndex + 1) % m_frameSampleCapacity;
        m_sampleCount = m_frameSampleCapacity;
        m_sampleSumMs += (sampleMs - replaced);
    }

    m_avgFrameMs = m_sampleCount > 0
        ? (m_sampleSumMs / static_cast<double>(m_sampleCount))
        : 0.0;
    m_percentilesDirty = true;
    m_framesSincePercentileUpdate += 1;
}

QVector<double> RenderingMonitor::sampleValues() const
{
    QVector<double> values;
    if (m_sampleCount <= 0)
        return values;

    values.reserve(m_sampleCount);
    if (m_sampleCount < m_frameSampleCapacity || m_recentFrameSamplesMs.size() < m_frameSampleCapacity) {
        for (int i = 0; i < m_sampleCount && i < m_recentFrameSamplesMs.size(); ++i)
            values.append(m_recentFrameSamplesMs.at(i));
        return values;
    }

    for (int i = 0; i < m_sampleCount; ++i) {
        const int index = (m_nextSampleIndex + i) % m_frameSampleCapacity;
        values.append(m_recentFrameSamplesMs.at(index));
    }
    return values;
}

void RenderingMonitor::updatePercentiles(bool force)
{
    if (!m_percentilesDirty && !force)
        return;

    if (!force && m_framesSincePercentileUpdate < kPercentileRecomputeFrameInterval)
        return;

    QVector<double> values = sampleValues();
    if (values.isEmpty()) {
        m_p95FrameMs = 0.0;
        m_p99FrameMs = 0.0;
        m_percentilesDirty = false;
        m_framesSincePercentileUpdate = 0;
        return;
    }

    std::sort(values.begin(), values.end());
    m_p95FrameMs = percentileValueFromSorted(values, 95.0);
    m_p99FrameMs = percentileValueFromSorted(values, 99.0);
    m_percentilesDirty = false;
    m_framesSincePercentileUpdate = 0;
}

double RenderingMonitor::percentileValueFromSorted(const QVector<double> &sortedValues, double percentile)
{
    if (sortedValues.isEmpty())
        return 0.0;

    const double clamped = qBound(0.0, percentile, 100.0);
    const int index = qBound(0,
                             static_cast<int>(std::ceil((clamped / 100.0) * sortedValues.size())) - 1,
                             sortedValues.size() - 1);
    return sortedValues.at(index);
}
