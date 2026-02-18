#include "backend/runtime/renderingmonitor.h"

#include <algorithm>
#include <cmath>
#include <QDateTime>
#include <QQuickWindow>
#include <numeric>

RenderingMonitor::RenderingMonitor(QObject *parent)
    : QObject(parent)
{
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
    snapshot.insert(QStringLiteral("recentSampleCount"), m_recentFrameSamplesMs.size());
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
    for (double sample : m_recentFrameSamplesMs) {
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

    m_frameSampleCapacity = next;
    while (m_recentFrameSamplesMs.size() > m_frameSampleCapacity)
        m_recentFrameSamplesMs.removeFirst();
    updateAggregates();

    emit frameSampleCapacityChanged();
    emit statsChanged();
}

int RenderingMonitor::recentSampleCount() const
{
    return m_recentFrameSamplesMs.size();
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
        m_recentFrameSamplesMs.append(m_lastFrameMs);
        while (m_recentFrameSamplesMs.size() > m_frameSampleCapacity)
            m_recentFrameSamplesMs.removeFirst();
        if (m_lastFrameMs >= m_droppedFrameThresholdMs)
            m_droppedFrameCount += 1;
        updateAggregates();
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

void RenderingMonitor::updateAggregates()
{
    if (m_recentFrameSamplesMs.isEmpty()) {
        m_avgFrameMs = 0.0;
        m_p95FrameMs = 0.0;
        m_p99FrameMs = 0.0;
        return;
    }

    const double total = std::accumulate(m_recentFrameSamplesMs.constBegin(),
                                         m_recentFrameSamplesMs.constEnd(),
                                         0.0);
    m_avgFrameMs = total / static_cast<double>(m_recentFrameSamplesMs.size());
    m_p95FrameMs = percentileValue(m_recentFrameSamplesMs, 95.0);
    m_p99FrameMs = percentileValue(m_recentFrameSamplesMs, 99.0);
}

double RenderingMonitor::percentileValue(QVector<double> values, double percentile)
{
    if (values.isEmpty())
        return 0.0;

    std::sort(values.begin(), values.end());
    const double clamped = qBound(0.0, percentile, 100.0);
    const int index = qBound(0,
                             static_cast<int>(std::ceil((clamped / 100.0) * values.size())) - 1,
                             values.size() - 1);
    return values.at(index);
}
