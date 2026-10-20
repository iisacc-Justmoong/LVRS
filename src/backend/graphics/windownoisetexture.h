#pragma once

#include "lvrs_export.h"

#include <QColor>
#include <QImage>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

// A cached, opaque noise tile with optional sharp monochrome grain.
// Only the colored noise is blurred, never app content or the fine grain.
class LVRS_EXPORT WindowNoiseTexture : public QQuickPaintedItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(WindowNoiseTexture)

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QColor primaryColor READ primaryColor WRITE setPrimaryColor NOTIFY primaryColorChanged)
    Q_PROPERTY(qreal noiseStrength READ noiseStrength WRITE setNoiseStrength NOTIFY noiseStrengthChanged)
    Q_PROPERTY(qreal blurRadius READ blurRadius WRITE setBlurRadius NOTIFY blurRadiusChanged)
    Q_PROPERTY(qreal grainStrength READ grainStrength WRITE setGrainStrength NOTIFY grainStrengthChanged)

public:
    explicit WindowNoiseTexture(QQuickItem *parent = nullptr);

    QColor color() const { return m_color; }
    QColor primaryColor() const { return m_primaryColor; }
    qreal noiseStrength() const { return m_noiseStrength; }
    qreal blurRadius() const { return m_blurRadius; }
    qreal grainStrength() const { return m_grainStrength; }

    void setColor(QColor value);
    void setPrimaryColor(QColor value);
    void setNoiseStrength(qreal value);
    void setBlurRadius(qreal value);
    void setGrainStrength(qreal value);
    void paint(QPainter *painter) override;

signals:
    void colorChanged();
    void primaryColorChanged();
    void noiseStrengthChanged();
    void blurRadiusChanged();
    void grainStrengthChanged();

private:
    void rebuildTexture();

    QColor m_color = QColor("#141414");
    QColor m_primaryColor = QColor("#1582FF");
    qreal m_noiseStrength = 0.32;
    qreal m_blurRadius = 1.5;
    qreal m_grainStrength = 0;
    QImage m_texture;
};
