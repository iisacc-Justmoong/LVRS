#include "windownoisetexture.h"

#include <QPainter>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {
constexpr int tileSide = 256;

QImage blurTile(const QImage &source, qreal sigma)
{
    if (sigma < 0.01)
        return source;

    const int radius = static_cast<int>(std::ceil(sigma * 3));
    std::vector<qreal> kernel(2 * radius + 1);
    qreal total = 0;
    for (int offset = -radius; offset <= radius; ++offset) {
        const qreal weight = std::exp(-qreal(offset * offset) / (2 * sigma * sigma));
        kernel[offset + radius] = weight;
        total += weight;
    }
    for (qreal &weight : kernel)
        weight /= total;

    // Wrap both passes around the tile, so repetition has no unblurred seams.
    std::vector<std::array<qreal, 3>> horizontal(tileSide * tileSide);
    for (int y = 0; y < tileSide; ++y) {
        const auto *row = reinterpret_cast<const QRgb *>(source.constScanLine(y));
        for (int x = 0; x < tileSide; ++x) {
            auto &pixel = horizontal[y * tileSide + x];
            for (int offset = -radius; offset <= radius; ++offset) {
                const QRgb sample = row[(x + offset + tileSide) % tileSide];
                const qreal weight = kernel[offset + radius];
                pixel[0] += qRed(sample) * weight;
                pixel[1] += qGreen(sample) * weight;
                pixel[2] += qBlue(sample) * weight;
            }
        }
    }
    QImage result(tileSide, tileSide, QImage::Format_RGB32);
    for (int y = 0; y < tileSide; ++y) {
        auto *row = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < tileSide; ++x) {
            std::array<qreal, 3> pixel{};
            for (int offset = -radius; offset <= radius; ++offset) {
                const auto &sample = horizontal[((y + offset + tileSide) % tileSide) * tileSide + x];
                const qreal weight = kernel[offset + radius];
                for (int channel = 0; channel < 3; ++channel)
                    pixel[channel] += sample[channel] * weight;
            }
            row[x] = qRgb(qRound(pixel[0]), qRound(pixel[1]), qRound(pixel[2]));
        }
    }
    return result;
}
}

WindowNoiseTexture::WindowNoiseTexture(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setOpaquePainting(true);
    setAntialiasing(false);
    rebuildTexture();
}

void WindowNoiseTexture::setColor(QColor value)
{
    if (!value.isValid())
        return;
    value.setAlpha(255);
    if (m_color == value)
        return;
    m_color = value;
    rebuildTexture();
    emit colorChanged();
}

void WindowNoiseTexture::setPrimaryColor(QColor value)
{
    if (!value.isValid())
        return;
    value.setAlpha(255);
    if (m_primaryColor == value)
        return;
    m_primaryColor = value;
    rebuildTexture();
    emit primaryColorChanged();
}

void WindowNoiseTexture::setNoiseStrength(qreal value)
{
    if (!std::isfinite(value))
        return;
    value = std::clamp(value, qreal(0), qreal(1));
    if (m_noiseStrength == value)
        return;
    m_noiseStrength = value;
    rebuildTexture();
    emit noiseStrengthChanged();
}

void WindowNoiseTexture::setBlurRadius(qreal value)
{
    if (!std::isfinite(value))
        return;
    value = std::clamp(value, qreal(0), qreal(50));
    if (m_blurRadius == value)
        return;
    m_blurRadius = value;
    rebuildTexture();
    emit blurRadiusChanged();
}

void WindowNoiseTexture::setGrainStrength(qreal value)
{
    if (!std::isfinite(value))
        return;
    value = std::clamp(value, qreal(0), qreal(1));
    if (m_grainStrength == value)
        return;
    m_grainStrength = value;
    rebuildTexture();
    emit grainStrengthChanged();
}

void WindowNoiseTexture::rebuildTexture()
{
    QImage noise(tileSide, tileSide, QImage::Format_RGB32);
    std::uint32_t state = 0x6d2b79f5u;
    for (int y = 0; y < tileSide; ++y) {
        auto *row = reinterpret_cast<QRgb *>(noise.scanLine(y));
        for (int x = 0; x < tileSide; ++x) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            const qreal amount = m_noiseStrength * (0.1 + 0.9 * qreal(state & 0xffffu) / 65535);
            const auto channel = [amount](int base, int accent) {
                return qRound(base + (accent - base) * amount);
            };
            row[x] = qRgb(channel(m_color.red(), m_primaryColor.red()),
                          channel(m_color.green(), m_primaryColor.green()),
                          channel(m_color.blue(), m_primaryColor.blue()));
        }
    }
    m_texture = blurTile(noise, m_blurRadius);
    if (m_grainStrength > 0) {
        // A separate fixed seed keeps the fine grain stable across accent changes.
        state = 0x9e3779b9u;
        for (int y = 0; y < tileSide; ++y) {
            auto *row = reinterpret_cast<QRgb *>(m_texture.scanLine(y));
            for (int x = 0; x < tileSide; ++x) {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                const qreal signedNoise = 2 * qreal(state & 0xffffu) / 65535 - 1;
                const int grain = qRound(signedNoise * m_grainStrength * 255);
                const QRgb pixel = row[x];
                row[x] = qRgb(std::clamp(qRed(pixel) + grain, 0, 255),
                              std::clamp(qGreen(pixel) + grain, 0, 255),
                              std::clamp(qBlue(pixel) + grain, 0, 255));
            }
        }
    }
    update();
}

void WindowNoiseTexture::paint(QPainter *painter)
{
    painter->fillRect(boundingRect(), m_color);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    // QQuickPaintedItem scales its painter for the actual raster resolution.
    // Keep each grain sample one device pixel wide, including on Retina displays.
    const QTransform transform = painter->deviceTransform();
    const qreal scaleX = m_grainStrength > 0
        ? std::max(qreal(1), std::hypot(transform.m11(), transform.m12())) : 1;
    const qreal scaleY = m_grainStrength > 0
        ? std::max(qreal(1), std::hypot(transform.m21(), transform.m22())) : 1;
    const QSizeF tileSize(tileSide / scaleX, tileSide / scaleY);
    for (qreal y = 0; y < height(); y += tileSize.height())
        for (qreal x = 0; x < width(); x += tileSize.width())
            painter->drawImage(QRectF(QPointF(x, y), tileSize), m_texture);
}
