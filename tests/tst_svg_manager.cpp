#include <QtTest>

#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QSet>
#include <QSignalSpy>
#include <QSvgRenderer>
#include <QTemporaryFile>
#include <QUrl>
#include <QXmlStreamReader>
#include <QtPlugin>

#include "backend/graphics/svgmanager.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class SvgManagerTests : public QObject
{
    Q_OBJECT

private slots:
    void svg_manager_defaults_to_square_18_logical_pixels();
    void svg_manager_generates_png_and_clamps();
    void svg_manager_error_paths_and_cache_signals();
    void figma_iconset_resources_are_complete_and_renderable();
    void all_svg_icons_scale_and_center();
};

void SvgManagerTests::svg_manager_defaults_to_square_18_logical_pixels()
{
    SvgManager manager;
    manager.setMaximumScale(1.0);
    manager.setMinimumScale(1.0);
    manager.setVectorFirst(false);

    const QByteArray svg = QByteArrayLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' width='16' height='8' viewBox='0 0 16 8'>"
        "<rect x='0' y='0' width='16' height='8' fill='#ff453a'/></svg>");
    const QString svgUrl = QStringLiteral("data:image/svg+xml;base64,") + QString::fromLatin1(svg.toBase64());
    const QString rasterUrl = manager.icon(svgUrl);
    QVERIFY(rasterUrl.startsWith(QStringLiteral("data:image/png;base64,")));

    const qsizetype separator = rasterUrl.indexOf(QLatin1Char(','));
    QVERIFY(separator > 0);
    const QImage raster = QImage::fromData(QByteArray::fromBase64(rasterUrl.mid(separator + 1).toLatin1()));
    QVERIFY(!raster.isNull());
    QCOMPARE(raster.size(), QSize(18, 18));

    QRect paintedBounds;
    for (int y = 0; y < raster.height(); ++y) {
        for (int x = 0; x < raster.width(); ++x) {
            if (raster.pixelColor(x, y).alpha() > 0)
                paintedBounds |= QRect(x, y, 1, 1);
        }
    }
    QCOMPARE(paintedBounds.width(), 18);
    QVERIFY(paintedBounds.height() >= 9);
    QVERIFY(paintedBounds.height() <= 10);
    QCOMPARE(raster.pixelColor(raster.width() / 2, 0).alpha(), 0);
    QCOMPARE(raster.pixelColor(raster.width() / 2, raster.height() - 1).alpha(), 0);
}

void SvgManagerTests::svg_manager_generates_png_and_clamps()
{
    SvgManager manager;
    QVERIFY(manager.vectorFirst());

    manager.setMaximumScale(0.5);
    QCOMPARE(manager.maximumScale(), 1.0);
    manager.setMinimumScale(0.2);
    QCOMPARE(manager.minimumScale(), 1.0);
    manager.setMaximumScale(4.0);
    manager.setMinimumScale(3.0);
    QCOMPARE(manager.minimumScale(), 3.0);
    QCOMPARE(manager.maximumScale(), 4.0);
    manager.ensureMinimumScale(3.5);
    QCOMPARE(manager.minimumScale(), 3.5);
    QCOMPARE(manager.maximumScale(), 4.0);
    manager.ensureMinimumScale(5.0);
    QCOMPARE(manager.minimumScale(), 5.0);
    QCOMPARE(manager.maximumScale(), 5.0);

    manager.setCacheSize(10000);
    QCOMPARE(manager.cacheSize(), 4096);
    manager.setCacheSize(-1);
    QCOMPARE(manager.cacheSize(), 0);
    manager.setCacheSize(32);
    QCOMPARE(manager.cacheSize(), 32);

    QCOMPARE(manager.icon(QString(), 16, 3.0), QString());
    QVERIFY(!manager.lastError().isEmpty());

    const QByteArray svg = QByteArrayLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16'>"
        "<rect x='0' y='0' width='16' height='16' fill='#ff453a'/></svg>");
    const QString svgUrl = QStringLiteral("data:image/svg+xml;base64,") + QString::fromLatin1(svg.toBase64());
    const QString vectorA = manager.icon(svgUrl, 16, 3.0);
    QCOMPARE(vectorA, svgUrl);
    QVERIFY(manager.lastError().isEmpty());

    manager.setVectorFirst(false);
    QCOMPARE(manager.vectorFirst(), false);
    const QString vectorB = manager.icon(svgUrl, 16, 3.0);
    QVERIFY(vectorB.startsWith(QStringLiteral("data:image/png;base64,")));

    const quint64 revisionBeforeClear = manager.revision();
    manager.clearCache();
    QCOMPARE(manager.revision(), revisionBeforeClear + 1);
    QVERIFY(manager.deviceScale() >= 1.0);
}

void SvgManagerTests::svg_manager_error_paths_and_cache_signals()
{
    SvgManager manager;
    QSignalSpy errorSpy(&manager, &SvgManager::lastErrorChanged);
    QSignalSpy vectorSpy(&manager, &SvgManager::vectorFirstChanged);
    QSignalSpy minimumSpy(&manager, &SvgManager::minimumScaleChanged);
    QSignalSpy maximumSpy(&manager, &SvgManager::maximumScaleChanged);
    QSignalSpy cacheSpy(&manager, &SvgManager::cacheSizeChanged);
    QSignalSpy revisionSpy(&manager, &SvgManager::revisionChanged);
    QVERIFY(errorSpy.isValid());
    QVERIFY(vectorSpy.isValid());
    QVERIFY(minimumSpy.isValid());
    QVERIFY(maximumSpy.isValid());
    QVERIFY(cacheSpy.isValid());
    QVERIFY(revisionSpy.isValid());

    manager.setMaximumScale(2.0);
    QCOMPARE(manager.maximumScale(), 2.0);
    QCOMPARE(manager.minimumScale(), 2.0);
    QVERIFY(maximumSpy.count() >= 1);
    QVERIFY(minimumSpy.count() >= 1);

    manager.setMinimumScale(1.5);
    QCOMPARE(manager.minimumScale(), 1.5);
    manager.setCacheSize(1);
    QCOMPARE(manager.cacheSize(), 1);
    QCOMPARE(cacheSpy.count(), 1);

    const QString malformed = manager.icon(QStringLiteral("data:image/svg+xml;base64"), 16, 3.0);
    QCOMPARE(malformed, QString());
    QCOMPARE(manager.lastError(), QStringLiteral("Malformed SVG data URL"));
    QVERIFY(errorSpy.count() >= 1);

    const QByteArray svg = QByteArrayLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24'>"
        "<circle cx='12' cy='12' r='8' fill='#34c759'/></svg>");
    const QString dataUrl = QStringLiteral("data:image/svg+xml;base64,") + QString::fromLatin1(svg.toBase64());
    const QString vectorData = manager.icon(dataUrl, 24, 0.0);
    QCOMPARE(vectorData, dataUrl);
    QCOMPARE(manager.lastError(), QString());

    manager.setVectorFirst(false);
    QCOMPARE(vectorSpy.count(), 1);
    QCOMPARE(manager.vectorFirst(), false);
    const QString vectorAgain = manager.icon(dataUrl, 24, 0.0);
    QVERIFY(vectorAgain.startsWith(QStringLiteral("data:image/png;base64,")));
    QCOMPARE(manager.lastError(), QString());

    QTemporaryFile file;
    file.setFileTemplate(QDir::tempPath() + "/svgmanager-XXXXXX.svg");
    QVERIFY(file.open());
    QVERIFY(file.write(svg) > 0);
    file.flush();
    const QString fileVector = manager.icon(QUrl::fromLocalFile(file.fileName()).toString(), 18, 2.0);
    QVERIFY(fileVector.startsWith(QStringLiteral("data:image/png;base64,")));

    const quint64 revisionBefore = manager.revision();
    manager.clearCache();
    QCOMPARE(manager.revision(), revisionBefore + 1);

    const quint64 revisionAfterFirstClear = manager.revision();
    manager.clearCache();
    QCOMPARE(manager.revision(), revisionAfterFirstClear);
    QCOMPARE(revisionSpy.count(), 2);
}

void SvgManagerTests::figma_iconset_resources_are_complete_and_renderable()
{
    QFile manifest(QStringLiteral(LVRS_TEST_SOURCE_DIR "/../docs/figma-iconset-manifest.json"));
    QVERIFY(manifest.open(QIODevice::ReadOnly));
    const auto inventory = QJsonDocument::fromJson(manifest.readAll()).object();
    const auto icons = inventory.value(QStringLiteral("icons")).toArray();
    QCOMPARE(icons.size(), 2863);
    QCOMPARE(icons.size(), inventory.value(QStringLiteral("count")).toInt());

    QSet<QString> filenames;
    for (const auto &value : icons) {
        const auto icon = value.toObject();
        const QString filename = icon.value(QStringLiteral("filename")).toString();
        QVERIFY2(!filename.contains('/') && !filename.contains('\\')
                     && filename.endsWith(QStringLiteral(".svg"))
                     && !filenames.contains(filename.toCaseFolded()), qPrintable(filename));
        filenames.insert(filename.toCaseFolded());

        QFile resource(QStringLiteral(":/qt/qml/LVRS/resources/iconset/") + filename);
        QVERIFY2(resource.open(QIODevice::ReadOnly), qPrintable(filename));
        const QByteArray svg = resource.readAll();
        QCOMPARE(QString::fromLatin1(QCryptographicHash::hash(svg, QCryptographicHash::Sha256).toHex()),
                 icon.value(QStringLiteral("sha256")).toString());

        QSvgRenderer renderer(svg);
        QVERIFY2(renderer.isValid(), qPrintable(filename));
        QVERIFY2(!renderer.defaultSize().isEmpty(), qPrintable(filename));
        QImage image(36, 36, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        renderer.render(&painter);
        painter.end();
        bool painted = false;
        for (int y = 0; y < image.height() && !painted; ++y)
            for (int x = 0; x < image.width() && !painted; ++x)
                painted = qAlpha(image.pixel(x, y)) > 0;
        QVERIFY2(painted, qPrintable(filename));
    }
}

void SvgManagerTests::all_svg_icons_scale_and_center()
{
    QDirIterator files(QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources"),
                       {QStringLiteral("*.svg")}, QDir::Files, QDirIterator::Subdirectories);
    int count = 0;
    while (files.hasNext()) {
        QFile file(files.next());
        const QByteArray context = file.fileName().toUtf8();
        QVERIFY2(file.open(QIODevice::ReadOnly), context.constData());
        const QByteArray svg = file.readAll();
        QXmlStreamReader xml(svg);
        QVERIFY2(xml.readNextStartElement(), context.constData());
        const auto attributes = xml.attributes();
        QVERIFY2(attributes.value("width") == "100%", context.constData());
        QVERIFY2(attributes.value("height") == "100%", context.constData());
        QVERIFY2(attributes.value("preserveAspectRatio") == "xMidYMid meet", context.constData());
        QSvgRenderer renderer(svg);
        QVERIFY2(renderer.isValid(), context.constData());
        QVERIFY2(renderer.viewBoxF().width() > 0, context.constData());
        QCOMPARE(renderer.viewBoxF().width(), renderer.viewBoxF().height());
        for (int size : {18, 36, 72, 144}) {
            QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            renderer.render(&painter);
            painter.end();
            QRect bounds;
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                    if (qAlpha(image.pixel(x, y)) > 0) bounds |= QRect(x, y, 1, 1);
            QVERIFY2(!bounds.isEmpty(), context.constData());
            // Up to one raster pixel per edge accounts for antialiasing.
            QVERIFY2(qAbs(bounds.left() - (size - 1 - bounds.right())) <= 2, context.constData());
            QVERIFY2(qAbs(bounds.top() - (size - 1 - bounds.bottom())) <= 2, context.constData());
        }
        ++count;
    }
    QCOMPARE(count, 2876);
}

QTEST_MAIN(SvgManagerTests)
#include "tst_svg_manager.moc"
