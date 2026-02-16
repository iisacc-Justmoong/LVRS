#pragma once

#include <QObject>
#include <QtQml/qqml.h>

class RenderQuality : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RenderQuality)
    QML_SINGLETON

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(qreal supersampleScale READ supersampleScale WRITE setSupersampleScale NOTIFY supersampleScaleChanged)
    Q_PROPERTY(qreal minimumSupersampleScale READ minimumSupersampleScale CONSTANT)
    Q_PROPERTY(qreal maximumSupersampleScale READ maximumSupersampleScale CONSTANT)
    Q_PROPERTY(int msaaSamples READ msaaSamples WRITE setMsaaSamples NOTIFY msaaSamplesChanged)
    Q_PROPERTY(bool nativeTextRendering READ nativeTextRendering WRITE setNativeTextRendering NOTIFY nativeTextRenderingChanged)

public:
    static constexpr qreal kForcedSupersampleScale = 3.0;

    explicit RenderQuality(QObject *parent = nullptr);

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

    Q_INVOKABLE qreal effectiveSupersampleScale() const;
    Q_INVOKABLE void applyWindow(QObject *window);
    Q_INVOKABLE void applyGlobalDefaults();

    static void configureGlobalDefaults(int msaaSamples = 8, bool nativeTextRendering = true);

signals:
    void enabledChanged();
    void supersampleScaleChanged();
    void msaaSamplesChanged();
    void nativeTextRenderingChanged();

private:
    bool m_enabled = true;
    qreal m_supersampleScale = kForcedSupersampleScale;
    qreal m_minimumSupersampleScale = kForcedSupersampleScale;
    qreal m_maximumSupersampleScale = kForcedSupersampleScale;
    int m_msaaSamples = 8;
    bool m_nativeTextRendering = true;
};
