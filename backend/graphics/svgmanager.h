#pragma once

#include <QObject>
#include <QtQml/qqml.h>

#include <QHash>
#include <QSet>
#include <QStringList>

#include <list>

class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

class SvgManager : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SvgManager)
    QML_SINGLETON

    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool vectorFirst READ vectorFirst WRITE setVectorFirst NOTIFY vectorFirstChanged)
    Q_PROPERTY(qreal minimumScale READ minimumScale WRITE setMinimumScale NOTIFY minimumScaleChanged)
    Q_PROPERTY(qreal maximumScale READ maximumScale WRITE setMaximumScale NOTIFY maximumScaleChanged)
    Q_PROPERTY(int cacheSize READ cacheSize WRITE setCacheSize NOTIFY cacheSizeChanged)
    Q_PROPERTY(quint64 revision READ revision NOTIFY revisionChanged)

public:
    explicit SvgManager(QObject *parent = nullptr);

    Q_INVOKABLE QString icon(const QString &svgUrl, int logicalSize = 18, qreal scale = 0.0);
    Q_INVOKABLE qreal deviceScale() const;
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE void ensureMinimumScale(qreal value);

    QString lastError() const;

    bool vectorFirst() const;
    void setVectorFirst(bool value);

    qreal minimumScale() const;
    void setMinimumScale(qreal value);

    qreal maximumScale() const;
    void setMaximumScale(qreal value);

    int cacheSize() const;
    void setCacheSize(int value);

    quint64 revision() const;

signals:
    void lastErrorChanged();
    void vectorFirstChanged();
    void minimumScaleChanged();
    void maximumScaleChanged();
    void cacheSizeChanged();
    void revisionChanged();

private:
    bool isVectorSource(const QString &sourceUrl) const;
    qreal resolveScale(qreal requestedScale) const;
    QByteArray loadSvgPayload(const QString &sourceUrl);
    void requestSvgFromUrl(const QUrl &url, const QString &cacheKey);
    void insertCache(const QString &key, const QString &value);
    void touchCacheKey(const QString &key);
    QString makeCacheKey(const QString &sourceUrl, int logicalSize, qreal scale) const;
    void setLastError(const QString &message);

    QNetworkAccessManager *m_network = nullptr;
    QHash<QString, QString> m_cache;
    std::list<QString> m_cacheOrder;
    QHash<QString, std::list<QString>::iterator> m_cacheOrderIter;
    QHash<QString, QByteArray> m_sourcePayloadCache;
    QSet<QString> m_pendingSourceUrls;
    QString m_lastError;
    bool m_vectorFirst = true;
    qreal m_minimumScale = 3.0;
    qreal m_maximumScale = 4.0;
    int m_cacheSize = 256;
    quint64 m_revision = 0;
};
