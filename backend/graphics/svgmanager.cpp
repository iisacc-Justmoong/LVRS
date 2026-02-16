#include "backend/graphics/svgmanager.h"

#include <QBuffer>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QScreen>
#include <QSvgRenderer>
#include <QTimer>
#include <QUrl>

SvgManager::SvgManager(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

QString SvgManager::icon(const QString &svgUrl, int logicalSize, qreal scale)
{
    setLastError(QString());

    const QString trimmedUrl = svgUrl.trimmed();
    if (trimmedUrl.isEmpty()) {
        setLastError(QStringLiteral("Empty SVG URL"));
        return QString();
    }

    if (isVectorSource(trimmedUrl)) {
        if (trimmedUrl.startsWith(QStringLiteral("data:image/svg"), Qt::CaseInsensitive)
            && !trimmedUrl.contains(QLatin1Char(','))) {
            setLastError(QStringLiteral("Malformed SVG data URL"));
            return QString();
        }
        return trimmedUrl;
    }

    const int targetLogicalSize = qBound(1, logicalSize, 2048);
    const qreal targetScale = resolveScale(scale);
    const QString cacheKey = makeCacheKey(trimmedUrl, targetLogicalSize, targetScale);

    const auto it = m_cache.constFind(cacheKey);
    if (it != m_cache.constEnd()) {
        touchCacheKey(cacheKey);
        return *it;
    }

    const QByteArray svgPayload = loadSvgPayload(trimmedUrl);
    if (svgPayload.isEmpty()) {
        if (m_lastError.isEmpty())
            setLastError(QStringLiteral("Failed to load SVG payload"));
        return QString();
    }

    QSvgRenderer renderer(svgPayload);
    if (!renderer.isValid()) {
        setLastError(QStringLiteral("Invalid SVG payload"));
        return QString();
    }

    const int rasterSize = qMax(1, qRound(static_cast<qreal>(targetLogicalSize) * targetScale));
    QImage image(rasterSize, rasterSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&painter, QRectF(0.0, 0.0, rasterSize, rasterSize));
    painter.end();

    QByteArray pngData;
    QBuffer buffer(&pngData);
    if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) {
        setLastError(QStringLiteral("Failed to encode SVG as PNG"));
        return QString();
    }

    const QString dataUrl = QStringLiteral("data:image/png;base64,")
        + QString::fromLatin1(pngData.toBase64());
    insertCache(cacheKey, dataUrl);
    return dataUrl;
}

qreal SvgManager::deviceScale() const
{
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return 1.0;
    return qMax(1.0, screen->devicePixelRatio());
}

void SvgManager::clearCache()
{
    if (m_cache.isEmpty() && m_cacheOrder.isEmpty() && m_sourcePayloadCache.isEmpty())
        return;
    m_cache.clear();
    m_cacheOrder.clear();
    m_sourcePayloadCache.clear();
    m_revision += 1;
    emit revisionChanged();
}

void SvgManager::ensureMinimumScale(qreal value)
{
    const qreal requested = qMax(1.0, value);
    if (requested > m_maximumScale) {
        m_maximumScale = requested;
        emit maximumScaleChanged();
    }
    if (requested > m_minimumScale) {
        m_minimumScale = requested;
        emit minimumScaleChanged();
    }
}

QString SvgManager::lastError() const
{
    return m_lastError;
}

bool SvgManager::vectorFirst() const
{
    return m_vectorFirst;
}

void SvgManager::setVectorFirst(bool value)
{
    Q_UNUSED(value)

    // SVG rendering is enforced as vector-only policy.
    const bool next = true;
    if (m_vectorFirst == next)
        return;
    m_vectorFirst = next;
    m_cache.clear();
    m_cacheOrder.clear();
    m_sourcePayloadCache.clear();
    m_pendingSourceUrls.clear();
    m_revision += 1;
    emit revisionChanged();
    emit vectorFirstChanged();
}

qreal SvgManager::minimumScale() const
{
    return m_minimumScale;
}

void SvgManager::setMinimumScale(qreal value)
{
    const qreal next = qBound(1.0, value, m_maximumScale);
    if (qFuzzyCompare(m_minimumScale, next))
        return;
    m_minimumScale = next;
    emit minimumScaleChanged();
}

qreal SvgManager::maximumScale() const
{
    return m_maximumScale;
}

void SvgManager::setMaximumScale(qreal value)
{
    const qreal next = qMax(1.0, value);
    if (qFuzzyCompare(m_maximumScale, next))
        return;
    m_maximumScale = next;
    if (m_minimumScale > m_maximumScale) {
        m_minimumScale = m_maximumScale;
        emit minimumScaleChanged();
    }
    emit maximumScaleChanged();
}

int SvgManager::cacheSize() const
{
    return m_cacheSize;
}

void SvgManager::setCacheSize(int value)
{
    const int next = qBound(0, value, 4096);
    if (m_cacheSize == next)
        return;
    m_cacheSize = next;

    if (m_cacheSize == 0) {
        clearCache();
    } else {
        while (m_cacheOrder.size() > m_cacheSize) {
            const QString oldest = m_cacheOrder.takeFirst();
            m_cache.remove(oldest);
        }
    }

    emit cacheSizeChanged();
}

quint64 SvgManager::revision() const
{
    return m_revision;
}

bool SvgManager::isVectorSource(const QString &sourceUrl) const
{
    const QString trimmed = sourceUrl.trimmed();
    if (trimmed.isEmpty())
        return false;

    if (trimmed.startsWith(QStringLiteral("data:image/svg"), Qt::CaseInsensitive))
        return true;

    if (trimmed.startsWith(QStringLiteral(":/")))
        return trimmed.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)
            || trimmed.endsWith(QStringLiteral(".svgz"), Qt::CaseInsensitive);

    const QUrl parsed = QUrl::fromUserInput(trimmed);
    if (!parsed.isValid())
        return false;

    if (parsed.scheme() == QStringLiteral("qrc")) {
        const QString qrcPath = parsed.path();
        return qrcPath.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)
            || qrcPath.endsWith(QStringLiteral(".svgz"), Qt::CaseInsensitive);
    }

    if (parsed.isLocalFile()) {
        const QString localPath = parsed.toLocalFile();
        return localPath.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)
            || localPath.endsWith(QStringLiteral(".svgz"), Qt::CaseInsensitive);
    }

    const QString path = parsed.path();
    return path.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)
        || path.endsWith(QStringLiteral(".svgz"), Qt::CaseInsensitive);
}

qreal SvgManager::resolveScale(qreal requestedScale) const
{
    if (requestedScale > 0.0)
        return qBound(1.0, requestedScale, m_maximumScale);

    const qreal preferred = qMax(deviceScale(), m_minimumScale);
    return qBound(1.0, preferred, m_maximumScale);
}

QByteArray SvgManager::loadSvgPayload(const QString &sourceUrl)
{
    if (sourceUrl.startsWith(QStringLiteral(":/"))) {
        QFile file(sourceUrl);
        if (!file.open(QIODevice::ReadOnly)) {
            setLastError(file.errorString());
            return QByteArray();
        }
        return file.readAll();
    }

    const QUrl parsed = QUrl::fromUserInput(sourceUrl);
    if (!parsed.isValid()) {
        setLastError(QStringLiteral("Invalid SVG URL"));
        return QByteArray();
    }

    if (parsed.scheme() == QStringLiteral("qrc")) {
        const QString qrcPath = QStringLiteral(":") + parsed.path();
        QFile file(qrcPath);
        if (!file.open(QIODevice::ReadOnly)) {
            setLastError(file.errorString());
            return QByteArray();
        }
        return file.readAll();
    }

    if (parsed.isLocalFile()) {
        QFile file(parsed.toLocalFile());
        if (!file.open(QIODevice::ReadOnly)) {
            setLastError(file.errorString());
            return QByteArray();
        }
        return file.readAll();
    }

    if (parsed.scheme() == QStringLiteral("data")) {
        const QString text = parsed.toString();
        const int commaIndex = text.indexOf(',');
        if (commaIndex < 0) {
            setLastError(QStringLiteral("Malformed SVG data URL"));
            return QByteArray();
        }

        const QString header = text.left(commaIndex);
        const QByteArray payload = text.mid(commaIndex + 1).toLatin1();
        if (header.contains(QStringLiteral(";base64"), Qt::CaseInsensitive))
            return QByteArray::fromBase64(payload);
        return QUrl::fromPercentEncoding(payload).toUtf8();
    }

    const QString remoteKey = parsed.toString(QUrl::FullyEncoded);
    const auto payloadIt = m_sourcePayloadCache.constFind(remoteKey);
    if (payloadIt != m_sourcePayloadCache.constEnd())
        return *payloadIt;

    requestSvgFromUrl(parsed, remoteKey);
    if (m_pendingSourceUrls.contains(remoteKey) && m_lastError.isEmpty())
        setLastError(QStringLiteral("SVG request pending"));
    return QByteArray();
}

void SvgManager::requestSvgFromUrl(const QUrl &url, const QString &cacheKey)
{
    if (m_pendingSourceUrls.contains(cacheKey))
        return;

    if (!m_network) {
        setLastError(QStringLiteral("Network manager is unavailable"));
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    request.setTransferTimeout(5000);
#endif

    QNetworkReply *reply = m_network->get(request);
    m_pendingSourceUrls.insert(cacheKey);

    QTimer::singleShot(6000, reply, [this, reply]() {
        if (!reply->isFinished()) {
            reply->abort();
            setLastError(QStringLiteral("SVG request timed out"));
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, cacheKey]() {
        m_pendingSourceUrls.remove(cacheKey);

        if (reply->error() != QNetworkReply::NoError) {
            if (reply->error() != QNetworkReply::OperationCanceledError)
                setLastError(reply->errorString());
            reply->deleteLater();
            return;
        }

        const QByteArray bytes = reply->readAll();
        reply->deleteLater();
        if (bytes.isEmpty()) {
            setLastError(QStringLiteral("Received empty SVG payload"));
            return;
        }

        m_sourcePayloadCache.insert(cacheKey, bytes);
        setLastError(QString());
        m_revision += 1;
        emit revisionChanged();
    });
}

void SvgManager::insertCache(const QString &key, const QString &value)
{
    if (m_cacheSize == 0)
        return;

    if (m_cache.contains(key)) {
        m_cache.insert(key, value);
        touchCacheKey(key);
        return;
    }

    m_cache.insert(key, value);
    m_cacheOrder.append(key);

    while (m_cacheOrder.size() > m_cacheSize) {
        const QString oldest = m_cacheOrder.takeFirst();
        m_cache.remove(oldest);
    }
}

void SvgManager::touchCacheKey(const QString &key)
{
    const int index = m_cacheOrder.indexOf(key);
    if (index < 0)
        return;
    m_cacheOrder.removeAt(index);
    m_cacheOrder.append(key);
}

QString SvgManager::makeCacheKey(const QString &sourceUrl, int logicalSize, qreal scale) const
{
    return sourceUrl + QStringLiteral("|")
        + QString::number(logicalSize) + QStringLiteral("|")
        + QString::number(scale, 'f', 3);
}

void SvgManager::setLastError(const QString &message)
{
    if (m_lastError == message)
        return;
    m_lastError = message;
    emit lastErrorChanged();
}
