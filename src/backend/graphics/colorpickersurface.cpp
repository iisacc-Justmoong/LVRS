#include "colorpickersurface.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTouchEvent>
#include <algorithm>
#include <cmath>

namespace {
constexpr double pi = 3.14159265358979323846;
double unit(double v) { return std::clamp(v, 0.0, 1.0); }
double wrapHue(double h) { return std::fmod(h + 360.0, 360.0); }
double spectrumSaturation(double y) { return unit(y / .45); }
double spectrumBrightness(double y) { return 1 - unit((y - .43) / .57); }

double spectrumPosition(double saturation, double brightness)
{
    // Project onto the three linear regions of the two exported Figma overlays.
    const double overlap = (saturation / .45 + (1 / .57 - brightness) / .57)
        / (1 / (.45 * .45) + 1 / (.57 * .57));
    const double candidates[] = {std::clamp(.45 * saturation, 0.0, .43),
                                  std::clamp(overlap, .43, .45),
                                  std::clamp(1 - .57 * brightness, .45, 1.0)};
    const auto distance = [=](double y) {
        const double ds = spectrumSaturation(y) - saturation;
        const double dv = spectrumBrightness(y) - brightness;
        return ds * ds + dv * dv;
    };
    double closest = candidates[0];
    for (double candidate : candidates)
        if (distance(candidate) < distance(closest))
            closest = candidate;
    return closest;
}
const QPointF whiteVertex(74, 43), hueVertex(200, 116), blackVertex(74, 189);

QPointF closestOnLine(QPointF p, QPointF a, QPointF b)
{
    const QPointF edge = b - a;
    return a + edge * unit(QPointF::dotProduct(p - a, edge) / QPointF::dotProduct(edge, edge));
}

QPointF triangleWeights(QPointF point)
{
    double hue = (point.x() - whiteVertex.x()) / 126;
    double black = (point.y() - whiteVertex.y() - 73 * hue) / 146;
    if (hue < 0 || black < 0 || hue + black > 1) {
        const QPointF candidates[] = {closestOnLine(point, whiteVertex, hueVertex),
                                      closestOnLine(point, hueVertex, blackVertex),
                                      closestOnLine(point, blackVertex, whiteVertex)};
        QPointF closest = candidates[0];
        for (const auto &candidate : candidates)
            if (QLineF(point, candidate).length() < QLineF(point, closest).length())
                closest = candidate;
        hue = (closest.x() - whiteVertex.x()) / 126;
        black = (closest.y() - whiteVertex.y() - 73 * hue) / 146;
    }
    return {unit(hue), unit(black)};
}

void selector(QPainter *painter, QPointF center, double diameter)
{
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QColor(0, 0, 0, 190), 3));
    painter->drawEllipse(center, diameter / 2 - 1, diameter / 2 - 1);
    painter->setPen(QPen(QColor(255, 255, 255, 240), 1.5));
    painter->drawEllipse(center, diameter / 2 - 1, diameter / 2 - 1);
}
}

ColorPickerSurface::ColorPickerSurface(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptTouchEvents(true);
    setActiveFocusOnTab(true);
}

void ColorPickerSurface::setModel(ColorPickerModel *model)
{
    if (m_model == model)
        return;
    if (m_model)
        disconnect(m_model, nullptr, this, nullptr);
    m_model = model;
    if (model) {
        connect(model, &ColorPickerModel::channelsChanged, this, [this] { update(); });
        connect(model, &QObject::destroyed, this, [this] { update(); });
    }
    emit modelChanged();
    update();
}

void ColorPickerSurface::setField(int field)
{
    field = std::clamp(field, int(Wheel), int(BrightnessRail));
    if (field == m_field)
        return;
    m_field = field;
    m_dragPart = -1;
    emit fieldChanged();
    update();
}

QPointF ColorPickerSurface::wheelPoint(QPointF point) const
{
    const double scale = std::max(0.001, std::min(width(), height()) / 232);
    return (point - QPointF((width() - 232 * scale) / 2, (height() - 232 * scale) / 2)) / scale;
}

void ColorPickerSurface::paint(QPainter *painter)
{
    if (!m_model || width() <= 0 || height() <= 0)
        return;
    const double h = wrapHue(m_model->hue()) / 360;
    const double s = m_model->saturation(), v = m_model->brightness();
    const double dpr = painter->device()->devicePixelRatioF();
    painter->setRenderHint(QPainter::Antialiasing);
    if (m_field == Wheel) {
        const double scale = std::min(width(), height()) / 232;
        painter->translate((width() - 232 * scale) / 2, (height() - 232 * scale) / 2);
        painter->scale(scale, scale);
        if (m_ring.isNull())
            m_ring.load(":/qt/qml/LVRS/resources/images/colorpicker/hue-ring.png");
        painter->drawImage(QRectF(4, 4, 224, 224), m_ring);
    }

    const bool triangle = m_field == Wheel;
    const QSize size(std::max(1, int(std::ceil((triangle ? 126 : width()) * dpr))),
                     std::max(1, int(std::ceil((triangle ? 146 : height()) * dpr))));
    const double dependency = triangle || m_field == SaturationBrightness ? h : m_field == HueSaturation ? v : 0;
    const QString key = QString("%1/%2/%3/%4").arg(m_field).arg(size.width()).arg(size.height()).arg(dependency, 0, 'f', 6);
    if (m_domainKey != key) {
        m_domainKey = key;
        m_domain = QImage(size, QImage::Format_ARGB32_Premultiplied);
        for (int y = 0; y < size.height(); ++y) {
            QRgb *row = reinterpret_cast<QRgb *>(m_domain.scanLine(y));
            const double ny = size.height() == 1 ? 0 : double(y) / (size.height() - 1);
            for (int x = 0; x < size.width(); ++x) {
                const double nx = size.width() == 1 ? 0 : double(x) / (size.width() - 1);
                QColor color;
                switch (m_field) {
                case Wheel: {
                    const double black = ny - nx / 2;
                    const double brightness = unit(1 - black);
                    color = QColor::fromHsvF(h, brightness > 0 ? unit(nx / brightness) : 0, brightness);
                    break;
                }
                case HueSaturation: color = QColor::fromHsvF(nx == 1 ? 0 : nx, 1 - ny, v); break;
                case SaturationBrightness: color = QColor::fromHsvF(h, nx, 1 - ny); break;
                case Spectrum: color = QColor::fromHsvF(nx == 1 ? 0 : nx, spectrumSaturation(ny), spectrumBrightness(ny)); break;
                case HueRail: color = QColor::fromHsvF(ny == 1 ? 0 : ny, 1, 1); break;
                default: color = QColor::fromRgbF(1 - ny, 1 - ny, 1 - ny); break;
                }
                row[x] = color.rgb();
            }
        }
    }

    QPainterPath clip;
    QRectF domain(0, 0, width(), height());
    if (triangle) {
        clip.moveTo(whiteVertex); clip.lineTo(hueVertex); clip.lineTo(blackVertex); clip.closeSubpath();
        domain = QRectF(74, 43, 126, 146);
    } else if (m_field >= HueRail) {
        domain = QRectF((width() - 12) / 2, 0, 12, height());
        clip.addRoundedRect(domain, 4, 4);
    } else {
        clip.addRoundedRect(domain, 4, 4);
    }
    painter->save();
    painter->setClipPath(clip);
    painter->drawImage(domain, m_domain);
    painter->restore();

    if (triangle) {
        const double radians = (90 - h * 360) * pi / 180;
        selector(painter, QPointF(116 + 100 * std::cos(radians), 116 + 100 * std::sin(radians)), 14);
        selector(painter, whiteVertex * ((1 - s) * v) + hueVertex * (s * v) + blackVertex * (1 - v), 12);
    } else if (m_field >= HueRail) {
        const double position = m_field == HueRail ? h : 1 - v;
        const double y = 3.5 + position * std::max(0.0, height() - 7);
        painter->setPen(QPen(QColor(0, 0, 0, 180), 1));
        painter->setBrush(QColor(235, 235, 239));
        painter->drawRoundedRect(QRectF(0.5, y - 3, width() - 1, 6), 3, 3);
    } else {
        const double nx = m_field == SaturationBrightness ? s : h;
        // Use the same white/black overlay stops for the cursor and input domain.
        const double ny = m_field == HueSaturation ? 1 - s : m_field == SaturationBrightness ? 1 - v
            : spectrumPosition(s, v);
        selector(painter, QPointF(std::clamp(nx * width(), 6.0, std::max(6.0, width() - 6)),
                                 std::clamp(ny * height(), 6.0, std::max(6.0, height() - 6))), 12);
    }
}

bool ColorPickerSurface::beginEdit(QPointF point)
{
    if (!m_model || !isEnabled())
        return false;
    m_dragPart = 0;
    if (m_field == Wheel) {
        const QPointF local = wheelPoint(point);
        const double radius = QLineF(local, QPointF(116, 116)).length();
        const double hueWeight = (local.x() - 74) / 126;
        const double blackWeight = (local.y() - 43 - 73 * hueWeight) / 146;
        if (radius >= 86 && radius <= 116)
            m_dragPart = 1;
        else if (hueWeight < 0 || blackWeight < 0 || hueWeight + blackWeight > 1)
            m_dragPart = -1;
    }
    if (m_dragPart < 0)
        return false;
    forceActiveFocus(Qt::MouseFocusReason);
    editAt(point);
    return true;
}

void ColorPickerSurface::editAt(QPointF point)
{
    if (!m_model || m_dragPart < 0)
        return;
    const double x = unit(point.x() / std::max(1.0, width()));
    const double y = unit(point.y() / std::max(1.0, height()));
    const double h = m_model->hue(), s = m_model->saturation(), v = m_model->brightness();
    switch (m_field) {
    case Wheel: {
        const QPointF local = wheelPoint(point);
        if (m_dragPart == 1) {
            const QPointF relative = local - QPointF(116, 116);
            m_model->editHsv(wrapHue(90 - std::atan2(relative.y(), relative.x()) * 180 / pi), s, v);
        } else {
            const QPointF weights = triangleWeights(local);
            const double brightness = 1 - weights.y();
            m_model->editHsv(h, brightness > 0 ? unit(weights.x() / brightness) : s, brightness);
        }
        break;
    }
    case HueSaturation: m_model->editHsv(x * 360, 1 - y, v); break;
    case SaturationBrightness: m_model->editHsv(h, x, 1 - y); break;
    case Spectrum: m_model->editHsv(x * 360, spectrumSaturation(y), spectrumBrightness(y)); break;
    case HueRail: m_model->editHsv(unit((point.y() - 3.5) / std::max(1.0, height() - 7)) * 360, s, v); break;
    case BrightnessRail: m_model->editHsv(h, s, 1 - unit((point.y() - 3.5) / std::max(1.0, height() - 7))); break;
    }
}

void ColorPickerSurface::mousePressEvent(QMouseEvent *event) { event->setAccepted(beginEdit(event->position())); }
void ColorPickerSurface::mouseMoveEvent(QMouseEvent *event) { editAt(event->position()); event->accept(); }
void ColorPickerSurface::mouseReleaseEvent(QMouseEvent *event) { editAt(event->position()); m_dragPart = -1; event->accept(); }

void ColorPickerSurface::touchEvent(QTouchEvent *event)
{
    if (event->type() == QEvent::TouchCancel) {
        m_dragPart = -1;
    } else if (!event->points().isEmpty()) {
        const auto &point = event->points().first();
        if (event->type() == QEvent::TouchBegin) {
            event->setAccepted(beginEdit(point.position()));
            return;
        }
        editAt(point.position());
        if (event->type() == QEvent::TouchEnd)
            m_dragPart = -1;
    }
    event->accept();
}

void ColorPickerSurface::keyPressEvent(QKeyEvent *event)
{
    if (!m_model || (event->key() != Qt::Key_Left && event->key() != Qt::Key_Right
                     && event->key() != Qt::Key_Up && event->key() != Qt::Key_Down)) {
        QQuickPaintedItem::keyPressEvent(event);
        return;
    }
    const bool horizontal = event->key() == Qt::Key_Left || event->key() == Qt::Key_Right;
    const double direction = event->key() == Qt::Key_Right || event->key() == Qt::Key_Up ? 1 : -1;
    const bool shifted = event->modifiers().testFlag(Qt::ShiftModifier);
    double h = m_model->hue(), s = m_model->saturation(), v = m_model->brightness();
    if (m_field == HueRail)
        h = wrapHue(h - direction * (shifted ? 10 : 1));
    else if (m_field == BrightnessRail)
        v += direction * (shifted ? .1 : .01);
    else if (horizontal) {
        if (m_field == SaturationBrightness)
            s += direction * (shifted ? .1 : .01);
        else
            h = wrapHue(h + direction * (shifted ? 10 : 1));
    } else if (m_field == HueSaturation || (m_field == Wheel && shifted)) {
        s += direction * .01;
    } else if (m_field == Spectrum) {
        const double y = unit(spectrumPosition(s, v) - direction * (shifted ? .05 : .005));
        s = spectrumSaturation(y); v = spectrumBrightness(y);
    } else {
        v += direction * .01;
    }
    m_model->editHsv(h, s, v);
    event->accept();
}
