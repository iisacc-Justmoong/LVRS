#pragma once

#include "colorpickermodel.h"
#include <QImage>
#include <QPointer>
#include <QQuickPaintedItem>

// Interactive color domains. Ordinary controls, labels and layout remain in QML.
class ColorPickerSurface : public QQuickPaintedItem
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ColorPickerSurface)
    Q_PROPERTY(ColorPickerModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int field READ field WRITE setField NOTIFY fieldChanged)
public:
    enum Field { Wheel, HueSaturation, SaturationBrightness, Spectrum, HueRail, BrightnessRail };
    Q_ENUM(Field)
    explicit ColorPickerSurface(QQuickItem *parent = nullptr);
    ColorPickerModel *model() const { return m_model; }
    void setModel(ColorPickerModel *model);
    int field() const { return m_field; }
    void setField(int field);
    void paint(QPainter *painter) override;

signals:
    void modelChanged();
    void fieldChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void touchEvent(QTouchEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QPointF wheelPoint(QPointF point) const;
    bool beginEdit(QPointF point);
    void editAt(QPointF point);
    QPointer<ColorPickerModel> m_model;
    int m_field = Wheel;
    int m_dragPart = -1;
    QImage m_ring;
    QImage m_domain;
    QString m_domainKey;
};
