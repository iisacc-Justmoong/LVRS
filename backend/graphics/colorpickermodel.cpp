#include "colorpickermodel.h"

#include <QRegularExpression>
#include <algorithm>
#include <cmath>

namespace {
double unit(double value) { return std::clamp(value, 0.0, 1.0); }
double hueUnit(double degrees) { return degrees == 360 ? 0 : unit(degrees / 360.0); }
}

ColorPickerModel::ColorPickerModel(QObject *parent) : QObject(parent)
{
    updateColor(m_color, false);
}

void ColorPickerModel::updateColor(const QColor &color, bool edited, bool keepHsv, bool keepCmyk)
{
    if (!color.isValid())
        return;
    const QColor rgb = color.toRgb();
    const bool changed = m_color != rgb;
    m_color = rgb;
    if (!keepHsv) {
        // Achromatic colors have no hue. Keep the last chosen hue for resaturation.
        if (rgb.hsvHueF() >= 0)
            m_hue = rgb.hsvHueF() * 360;
        m_saturation = rgb.hsvSaturationF();
        m_brightness = rgb.valueF();
    }
    if (!keepCmyk) {
        const QColor cmyk = rgb.toCmyk();
        m_cmyk = {cmyk.cyanF(), cmyk.magentaF(), cmyk.yellowF(), cmyk.blackF()};
    }
    ++m_revision;
    if (changed)
        emit colorChanged();
    emit channelsChanged();
    if (edited && changed)
        emit colorEdited(m_color);
}

void ColorPickerModel::setColor(const QColor &color)
{
    if (!color.isValid() || color.toRgb() == m_color)
        return;
    if (!m_explicitPrevious) {
        m_previousColor = color.toRgb();
        emit previousColorChanged();
    }
    updateColor(color, false);
}

void ColorPickerModel::setPreviousColor(const QColor &color)
{
    if (!color.isValid())
        return;
    m_explicitPrevious = true;
    if (m_previousColor == color.toRgb())
        return;
    m_previousColor = color.toRgb();
    emit previousColorChanged();
}

double ColorPickerModel::channelValue(int channel) const
{
    switch (channel) {
    case Hue: return m_hue;
    case Saturation: return m_saturation * 100;
    case Brightness: return m_brightness * 100;
    case Red: return m_color.redF() * 255;
    case Green: return m_color.greenF() * 255;
    case Blue: return m_color.blueF() * 255;
    case Cyan: case Magenta: case Yellow: case Black: return m_cmyk[channel - Cyan] * 100;
    case Alpha: return m_color.alphaF() * 100;
    case Gray: return (1 - qGray(m_color.rgb()) / 255.0) * 100;
    default: return 0;
    }
}

QColor ColorPickerModel::channelColor(int channel, double position) const
{
    if (!std::isfinite(position))
        return m_color;
    const double p = unit(position);
    switch (channel) {
    case Hue: return QColor::fromHsvF(p == 1 ? 0 : p, 1, 1);
    case Saturation: return QColor::fromHsvF(hueUnit(m_hue), p, m_brightness);
    case Brightness: return QColor::fromHsvF(hueUnit(m_hue), m_saturation, p);
    case Red: return QColor::fromRgbF(p, m_color.greenF(), m_color.blueF());
    case Green: return QColor::fromRgbF(m_color.redF(), p, m_color.blueF());
    case Blue: return QColor::fromRgbF(m_color.redF(), m_color.greenF(), p);
    case Cyan: case Magenta: case Yellow: case Black: {
        auto cmyk = m_cmyk;
        cmyk[channel - Cyan] = p;
        return QColor::fromCmykF(cmyk[0], cmyk[1], cmyk[2], cmyk[3]);
    }
    case Alpha: { QColor result = m_color; result.setAlphaF(p); return result; }
    case Gray: return QColor::fromRgbF(1 - p, 1 - p, 1 - p);
    default: return m_color;
    }
}

void ColorPickerModel::editChannel(int channel, double value)
{
    if (!std::isfinite(value) || channel < Hue || channel > Gray)
        return;
    const double maximum = channel == Hue ? 360 : channel >= Red && channel <= Blue ? 255 : 100;
    const double p = unit(value / maximum);
    if (channel <= Brightness) {
        editHsv(channel == Hue ? p * 360 : m_hue,
                channel == Saturation ? p : m_saturation,
                channel == Brightness ? p : m_brightness);
        return;
    }
    QColor result = channelColor(channel, p);
    result.setAlphaF(channel == Alpha ? p : m_color.alphaF());
    const bool isCmyk = channel >= Cyan && channel <= Black;
    if (isCmyk)
        m_cmyk[channel - Cyan] = p;
    updateColor(result, true, channel == Alpha, isCmyk || channel == Alpha);
}

void ColorPickerModel::editHsv(double hue, double saturation, double brightness)
{
    if (!std::isfinite(hue) || !std::isfinite(saturation) || !std::isfinite(brightness))
        return;
    m_hue = std::clamp(hue, 0.0, 360.0);
    m_saturation = unit(saturation);
    m_brightness = unit(brightness);
    updateColor(QColor::fromHsvF(hueUnit(m_hue), m_saturation, m_brightness, m_color.alphaF()), true, true);
}

void ColorPickerModel::editColor(const QColor &color)
{
    updateColor(color, true);
}

bool ColorPickerModel::editHex(const QString &text)
{
    QString value = text.trimmed();
    if (value.startsWith('#'))
        value.remove(0, 1);
    static const QRegularExpression pattern(QStringLiteral("^(?:[0-9a-fA-F]{6}|[0-9a-fA-F]{8})$"));
    if (!pattern.match(value).hasMatch())
        return false;
    QColor color("#" + value.left(6));
    // Eight-digit input is explicitly RRGGBBAA, not QColor's AARRGGBB notation.
    color.setAlphaF(value.size() == 8 ? value.mid(6, 2).toInt(nullptr, 16) / 255.0 : m_color.alphaF());
    updateColor(color, true);
    return true;
}

void ColorPickerModel::accept()
{
    if (m_previousColor != m_color) {
        m_previousColor = m_color;
        emit previousColorChanged();
    }
}

void ColorPickerModel::cancel()
{
    updateColor(m_previousColor, true);
}
