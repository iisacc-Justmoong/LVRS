#pragma once

#include <QObject>
#include <QPointer>
#include <QElapsedTimer>
#include <QVector>
#include <QVariantMap>
#include <QtQml/qqml.h>

class QQuickWindow;

class RenderingMonitor : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RenderMonitor)
    QML_SINGLETON

    Q_PROPERTY(bool active READ active NOTIFY activeChanged)
    Q_PROPERTY(double fps READ fps NOTIFY statsChanged)
    Q_PROPERTY(double lastFrameMs READ lastFrameMs NOTIFY statsChanged)
    Q_PROPERTY(double avgFrameMs READ avgFrameMs NOTIFY statsChanged)
    Q_PROPERTY(double p95FrameMs READ p95FrameMs NOTIFY statsChanged)
    Q_PROPERTY(double p99FrameMs READ p99FrameMs NOTIFY statsChanged)
    Q_PROPERTY(quint64 droppedFrameCount READ droppedFrameCount NOTIFY statsChanged)
    Q_PROPERTY(double droppedFrameThresholdMs READ droppedFrameThresholdMs WRITE setDroppedFrameThresholdMs NOTIFY droppedFrameThresholdMsChanged)
    Q_PROPERTY(int frameSampleCapacity READ frameSampleCapacity WRITE setFrameSampleCapacity NOTIFY frameSampleCapacityChanged)
    Q_PROPERTY(int recentSampleCount READ recentSampleCount NOTIFY statsChanged)
    Q_PROPERTY(quint64 frameCount READ frameCount NOTIFY statsChanged)

public:
    explicit RenderingMonitor(QObject *parent = nullptr);

    Q_INVOKABLE void attachWindow(QObject *window);
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reset();
    Q_INVOKABLE QVariantMap performanceSnapshot() const;

    bool active() const;
    double fps() const;
    double lastFrameMs() const;
    double avgFrameMs() const;
    double p95FrameMs() const;
    double p99FrameMs() const;
    quint64 droppedFrameCount() const;
    double droppedFrameThresholdMs() const;
    void setDroppedFrameThresholdMs(double value);
    int frameSampleCapacity() const;
    void setFrameSampleCapacity(int value);
    int recentSampleCount() const;
    quint64 frameCount() const;

signals:
    void activeChanged();
    void statsChanged();
    void droppedFrameThresholdMsChanged();
    void frameSampleCapacityChanged();

private slots:
    void handleFrameSwapped();
    void handleWindowDestroyed();

private:
    void setActive(bool next);
    void resetMetrics();
    void detachWindow();
    void updateAggregates();
    static double percentileValue(QVector<double> values, double percentile);

    QPointer<QQuickWindow> m_window;
    QElapsedTimer m_frameTimer;
    bool m_active = false;
    double m_fps = 0.0;
    double m_lastFrameMs = 0.0;
    double m_avgFrameMs = 0.0;
    double m_p95FrameMs = 0.0;
    double m_p99FrameMs = 0.0;
    quint64 m_droppedFrameCount = 0;
    double m_droppedFrameThresholdMs = 20.0;
    int m_frameSampleCapacity = 240;
    QVector<double> m_recentFrameSamplesMs;
    quint64 m_frameCount = 0;
};
