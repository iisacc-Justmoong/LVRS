#include <QtTest>

#include <QDir>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QUrl>
#include <QtPlugin>

#include "backend/graphics/svgmanager.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class SvgManagerTests : public QObject
{
    Q_OBJECT

private slots:
    void svg_manager_generates_png_and_clamps();
    void svg_manager_error_paths_and_cache_signals();
};

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
    QCOMPARE(manager.vectorFirst(), true);
    const QString vectorB = manager.icon(svgUrl, 16, 3.0);
    QCOMPARE(vectorB, svgUrl);

    const quint64 revisionBeforeClear = manager.revision();
    manager.clearCache();
    QCOMPARE(manager.revision(), revisionBeforeClear);
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
    QCOMPARE(vectorSpy.count(), 0);
    QCOMPARE(manager.vectorFirst(), true);
    const QString vectorAgain = manager.icon(dataUrl, 24, 0.0);
    QCOMPARE(vectorAgain, dataUrl);
    QCOMPARE(manager.lastError(), QString());

    QTemporaryFile file;
    file.setFileTemplate(QDir::tempPath() + "/svgmanager-XXXXXX.svg");
    QVERIFY(file.open());
    QVERIFY(file.write(svg) > 0);
    file.flush();
    const QString fileVector = manager.icon(QUrl::fromLocalFile(file.fileName()).toString(), 18, 2.0);
    QVERIFY(fileVector.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive));

    const quint64 revisionBefore = manager.revision();
    manager.clearCache();
    QCOMPARE(manager.revision(), revisionBefore);

    const quint64 revisionAfterFirstClear = manager.revision();
    manager.clearCache();
    QCOMPARE(manager.revision(), revisionAfterFirstClear);
    QCOMPARE(revisionSpy.count(), 0);
}

QTEST_MAIN(SvgManagerTests)
#include "tst_svg_manager.moc"
