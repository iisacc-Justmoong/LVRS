#include "CTestSupport.h"
#include "QtEventWait.h"
#include <QGuiApplication>
#include <QTemporaryDir>

#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QRegularExpression>
#include <QSet>
#include <QSvgRenderer>
#include <QTemporaryFile>
#include <QUrl>
#include <QXmlStreamReader>
#include <QtPlugin>

#include "backend/graphics/svgmanager.h"
#include "accent_fixture.h"

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class SvgManagerTests : public QObject
{
public:
    void svg_manager_defaults_to_square_18_logical_pixels();
    void svg_manager_generates_png_and_clamps();
    void svg_manager_error_paths_and_cache_signals();
    void figma_iconset_resources_are_complete_and_renderable();
    void control_variant_resources_are_packaged();
    void provider_metadata_is_not_packaged();
    void translate_object_preserves_image_alpha();
    void smart_select_preserves_image_alpha();
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
    CTEST_CHECK(rasterUrl.startsWith(QStringLiteral("data:image/png;base64,")));

    const qsizetype separator = rasterUrl.indexOf(QLatin1Char(','));
    CTEST_CHECK(separator > 0);
    const QImage raster = QImage::fromData(QByteArray::fromBase64(rasterUrl.mid(separator + 1).toLatin1()));
    CTEST_CHECK(!raster.isNull());
    CTEST_COMPARE(raster.size(), QSize(18, 18));

    QRect paintedBounds;
    for (int y = 0; y < raster.height(); ++y) {
        for (int x = 0; x < raster.width(); ++x) {
            if (raster.pixelColor(x, y).alpha() > 0)
                paintedBounds |= QRect(x, y, 1, 1);
        }
    }
    CTEST_COMPARE(paintedBounds.width(), 18);
    CTEST_CHECK(paintedBounds.height() >= 9);
    CTEST_CHECK(paintedBounds.height() <= 10);
    CTEST_COMPARE(raster.pixelColor(raster.width() / 2, 0).alpha(), 0);
    CTEST_COMPARE(raster.pixelColor(raster.width() / 2, raster.height() - 1).alpha(), 0);
}

void SvgManagerTests::svg_manager_generates_png_and_clamps()
{
    SvgManager manager;
    CTEST_CHECK(manager.vectorFirst());

    manager.setMaximumScale(0.5);
    CTEST_CHECK(qFuzzyCompare(manager.maximumScale(), qreal(1.0)));
    manager.setMinimumScale(0.2);
    CTEST_CHECK(qFuzzyCompare(manager.minimumScale(), qreal(1.0)));
    manager.setMaximumScale(4.0);
    manager.setMinimumScale(3.0);
    CTEST_CHECK(qFuzzyCompare(manager.minimumScale(), qreal(3.0)));
    CTEST_CHECK(qFuzzyCompare(manager.maximumScale(), qreal(4.0)));
    manager.ensureMinimumScale(3.5);
    CTEST_CHECK(qFuzzyCompare(manager.minimumScale(), qreal(3.5)));
    CTEST_CHECK(qFuzzyCompare(manager.maximumScale(), qreal(4.0)));
    manager.ensureMinimumScale(5.0);
    CTEST_CHECK(qFuzzyCompare(manager.minimumScale(), qreal(5.0)));
    CTEST_CHECK(qFuzzyCompare(manager.maximumScale(), qreal(5.0)));

    manager.setCacheSize(10000);
    CTEST_COMPARE(manager.cacheSize(), 4096);
    manager.setCacheSize(-1);
    CTEST_COMPARE(manager.cacheSize(), 0);
    manager.setCacheSize(32);
    CTEST_COMPARE(manager.cacheSize(), 32);

    CTEST_COMPARE(manager.icon(QString(), 16, 3.0), QString());
    CTEST_CHECK(!manager.lastError().isEmpty());

    const QByteArray svg = QByteArrayLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' width='16' height='16'>"
        "<rect x='0' y='0' width='16' height='16' fill='#ff453a'/></svg>");
    const QString svgUrl = QStringLiteral("data:image/svg+xml;base64,") + QString::fromLatin1(svg.toBase64());
    const QString vectorA = manager.icon(svgUrl, 16, 3.0);
    CTEST_COMPARE(vectorA, svgUrl);
    CTEST_CHECK(manager.lastError().isEmpty());

    manager.setVectorFirst(false);
    CTEST_COMPARE(manager.vectorFirst(), false);
    const QString vectorB = manager.icon(svgUrl, 16, 3.0);
    CTEST_CHECK(vectorB.startsWith(QStringLiteral("data:image/png;base64,")));

    const quint64 revisionBeforeClear = manager.revision();
    manager.clearCache();
    CTEST_COMPARE(manager.revision(), revisionBeforeClear + 1);
    CTEST_CHECK(manager.deviceScale() >= 1.0);
}

void SvgManagerTests::svg_manager_error_paths_and_cache_signals()
{
    SvgManager manager;
    int errorCount = 0;
    const auto errorConnection = QObject::connect(&manager, &SvgManager::lastErrorChanged, &manager, [&] { ++errorCount; });
    CTEST_CHECK(errorConnection);
    int vectorCount = 0;
    const auto vectorConnection = QObject::connect(&manager, &SvgManager::vectorFirstChanged, &manager, [&] { ++vectorCount; });
    CTEST_CHECK(vectorConnection);
    int minimumCount = 0;
    const auto minimumConnection = QObject::connect(&manager, &SvgManager::minimumScaleChanged, &manager, [&] { ++minimumCount; });
    CTEST_CHECK(minimumConnection);
    int maximumCount = 0;
    const auto maximumConnection = QObject::connect(&manager, &SvgManager::maximumScaleChanged, &manager, [&] { ++maximumCount; });
    CTEST_CHECK(maximumConnection);
    int cacheCount = 0;
    const auto cacheConnection = QObject::connect(&manager, &SvgManager::cacheSizeChanged, &manager, [&] { ++cacheCount; });
    CTEST_CHECK(cacheConnection);
    int revisionCount = 0;
    const auto revisionConnection = QObject::connect(&manager, &SvgManager::revisionChanged, &manager, [&] { ++revisionCount; });
    CTEST_CHECK(revisionConnection);

    manager.setMaximumScale(2.0);
    CTEST_CHECK(qFuzzyCompare(manager.maximumScale(), qreal(2.0)));
    CTEST_CHECK(qFuzzyCompare(manager.minimumScale(), qreal(2.0)));
    CTEST_CHECK(maximumCount >= 1);
    CTEST_CHECK(minimumCount >= 1);

    manager.setMinimumScale(1.5);
    CTEST_CHECK(qFuzzyCompare(manager.minimumScale(), qreal(1.5)));
    manager.setCacheSize(1);
    CTEST_COMPARE(manager.cacheSize(), 1);
    CTEST_COMPARE(cacheCount, 1);

    const QString malformed = manager.icon(QStringLiteral("data:image/svg+xml;base64"), 16, 3.0);
    CTEST_COMPARE(malformed, QString());
    CTEST_COMPARE(manager.lastError(), QStringLiteral("Malformed SVG data URL"));
    CTEST_CHECK(errorCount >= 1);

    const QByteArray svg = QByteArrayLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' width='24' height='24'>"
        "<circle cx='12' cy='12' r='8' fill='#34c759'/></svg>");
    const QString dataUrl = QStringLiteral("data:image/svg+xml;base64,") + QString::fromLatin1(svg.toBase64());
    const QString vectorData = manager.icon(dataUrl, 24, 0.0);
    CTEST_COMPARE(vectorData, dataUrl);
    CTEST_COMPARE(manager.lastError(), QString());

    manager.setVectorFirst(false);
    CTEST_COMPARE(vectorCount, 1);
    CTEST_COMPARE(manager.vectorFirst(), false);
    const QString vectorAgain = manager.icon(dataUrl, 24, 0.0);
    CTEST_CHECK(vectorAgain.startsWith(QStringLiteral("data:image/png;base64,")));
    CTEST_COMPARE(manager.lastError(), QString());

    QTemporaryDir workspace(QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources/ctest-svg-XXXXXX"));
    CTEST_CHECK(workspace.isValid());
    QTemporaryFile file;
    file.setFileTemplate(workspace.filePath("svgmanager-XXXXXX.svg"));
    CTEST_CHECK(file.open());
    CTEST_CHECK(file.write(svg) > 0);
    file.flush();
    const QString fileVector = manager.icon(QUrl::fromLocalFile(file.fileName()).toString(), 18, 2.0);
    CTEST_CHECK(fileVector.startsWith(QStringLiteral("data:image/png;base64,")));

    const quint64 revisionBefore = manager.revision();
    manager.clearCache();
    CTEST_COMPARE(manager.revision(), revisionBefore + 1);

    const quint64 revisionAfterFirstClear = manager.revision();
    manager.clearCache();
    CTEST_COMPARE(manager.revision(), revisionAfterFirstClear);
    CTEST_COMPARE(revisionCount, 2);
}

void SvgManagerTests::figma_iconset_resources_are_complete_and_renderable()
{
    QFile manifest(QStringLiteral(LVRS_TEST_SOURCE_DIR "/../docs/figma-iconset-manifest.json"));
    CTEST_CHECK(manifest.open(QIODevice::ReadOnly));
    const auto inventory = QJsonDocument::fromJson(manifest.readAll()).object();
    const auto icons = inventory.value(QStringLiteral("icons")).toArray();
    CTEST_COMPARE(icons.size(), 2846);
    CTEST_COMPARE(icons.size(), inventory.value(QStringLiteral("count")).toInt());
    CTEST_COMPARE(inventory.value(QStringLiteral("componentCount")).toInt(), 2844);
    CTEST_COMPARE(inventory.value(QStringLiteral("standaloneVectorCount")).toInt(), 2);

    QSet<QString> filenames;
    QSet<QString> nodeIds;
    int accentedIcons = 0;
    for (const auto &value : icons) {
        const auto icon = value.toObject();
        const QString filename = icon.value(QStringLiteral("filename")).toString();
        CTEST_CHECK_MESSAGE(!filename.contains('/') && !filename.contains('\\')
                     && filename.endsWith(QStringLiteral(".svg"))
                     && !filenames.contains(filename.toCaseFolded()), qPrintable(filename));
        filenames.insert(filename.toCaseFolded());
        const QString nodeId = icon.value(QStringLiteral("nodeId")).toString();
        CTEST_CHECK_MESSAGE(!nodeId.isEmpty() && !nodeIds.contains(nodeId), qPrintable(filename));
        nodeIds.insert(nodeId);

        QFile resource(QStringLiteral(":/qt/qml/LVRS/resources/iconset/") + filename);
        CTEST_CHECK_MESSAGE(resource.open(QIODevice::ReadOnly), qPrintable(filename));
        const QByteArray svg = resource.readAll();
        CTEST_COMPARE(QString::fromLatin1(QCryptographicHash::hash(svg, QCryptographicHash::Sha256).toHex()),
                 icon.value(QStringLiteral("sha256")).toString());

        QSvgRenderer renderer(svg);
        CTEST_CHECK_MESSAGE(renderer.isValid(), qPrintable(filename));
        CTEST_CHECK_MESSAGE(!renderer.defaultSize().isEmpty(), qPrintable(filename));
        QImage image(36, 36, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        renderer.render(&painter);
        painter.end();
        bool painted = false;
        bool colored = false;
        for (int y = 0; y < image.height() && !painted; ++y)
            for (int x = 0; x < image.width() && !painted; ++x)
                painted = qAlpha(image.pixel(x, y)) > 0;
        CTEST_CHECK_MESSAGE(painted, qPrintable(filename));
        if (icon.contains(QStringLiteral("accentVariableId"))) {
            for (int y = 0; y < image.height() && !colored; ++y) {
                for (int x = 0; x < image.width() && !colored; ++x) {
                    const QColor color = image.pixelColor(x, y);
                    colored = color.alpha() > 32
                        && qMax(color.red(), qMax(color.green(), color.blue()))
                            - qMin(color.red(), qMin(color.green(), color.blue())) > 16;
                }
            }
            CTEST_CHECK_MESSAGE(colored, qPrintable(filename + " lost its Figma accent"));
            ++accentedIcons;
        }
    }
    CTEST_COMPARE(accentedIcons, 892);

    // Old resource URLs resolve to the current artwork, including new accents.
    for (const auto &value : inventory.value(QStringLiteral("legacyAliases")).toArray()) {
        const auto alias = value.toObject();
        const QString name = alias.value(QStringLiteral("filename")).toString();
        QFile oldResource(QStringLiteral(":/qt/qml/LVRS/resources/iconset/") + name);
        QFile currentResource(QStringLiteral(":/qt/qml/LVRS/resources/iconset/")
                              + alias.value(QStringLiteral("target")).toString());
        CTEST_CHECK_MESSAGE(oldResource.open(QIODevice::ReadOnly), qPrintable(name));
        CTEST_CHECK_MESSAGE(currentResource.open(QIODevice::ReadOnly), qPrintable(name));
        CTEST_COMPARE(oldResource.readAll(), currentResource.readAll());
    }
}

void SvgManagerTests::control_variant_resources_are_packaged()
{
    const QStringList names = {
        QStringLiteral("StepperChevron.svg"),
        QStringLiteral("StepperDownBorderless.svg"),
        QStringLiteral("StepperDownPrimary.svg"),
        QStringLiteral("StepperUpBorderless.svg"),
        QStringLiteral("StepperUpDownBorderless.svg"),
        QStringLiteral("StepperUpDownChevron.svg"),
        QStringLiteral("StepperUpDownPrimary.svg"),
        QStringLiteral("StepperUpPrimary.svg"),
        QStringLiteral("checkboxCheckedDisabled.svg"),
        QStringLiteral("checkboxCheckedEnabled.svg"),
        QStringLiteral("generalchevronDownAccent.svg"),
        QStringLiteral("generalchevronDownBorderless.svg"),
        QStringLiteral("generalchevronDownDisabled.svg"),
        QStringLiteral("inputFieldSearch.svg"),
    };
    for (const auto &name : names) {
        QFile asset(QStringLiteral(":/qt/qml/LVRS/resources/iconset/") + name);
        CTEST_CHECK_MESSAGE(asset.open(QIODevice::ReadOnly), qPrintable(name));
        QSvgRenderer renderer(asset.readAll());
        CTEST_CHECK_MESSAGE(renderer.isValid(), qPrintable(name));
    }
}

void SvgManagerTests::provider_metadata_is_not_packaged()
{
    QDirIterator files(QStringLiteral(":/qt/qml/LVRS/resources"),
                       {QStringLiteral("*.iisacc-preview"), QStringLiteral(".iisacc-temporary.lock")},
                       QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    CTEST_CHECK_MESSAGE(!files.hasNext(), "iiFileProvider sidecars or locks were packaged as LVRS resources");
}

void SvgManagerTests::translate_object_preserves_image_alpha()
{
    QFile resource(QStringLiteral(":/qt/qml/LVRS/resources/iconset/translateObject.svg"));
    CTEST_CHECK(resource.open(QIODevice::ReadOnly));
    const QByteArray svg = resource.readAll();
    const QRegularExpression embedded(QStringLiteral("data:image/png;base64,([^\"]+)"));
    const auto match = embedded.match(QString::fromUtf8(svg));
    CTEST_CHECK(match.hasMatch());
    const QImage tinted = QImage::fromData(QByteArray::fromBase64(match.captured(1).toLatin1()));
    const QImage original(QStringLiteral(LVRS_TEST_SOURCE_DIR "/fixtures/translate-object-mask.png"));
    CTEST_CHECK(!tinted.isNull());
    CTEST_CHECK(!original.isNull());
    CTEST_COMPARE(tinted.size(), original.size());
    const QColor cyan = AccentFixture::role("Cyan", "Hover");
    bool visible = false;
    bool exactColor = false;
    for (int y = 0; y < original.height(); ++y) {
        for (int x = 0; x < original.width(); ++x) {
            CTEST_COMPARE(tinted.pixelColor(x, y).alpha(), original.pixelColor(x, y).alpha());
            if (original.pixelColor(x, y).alpha() > 0) {
                const QColor color = tinted.pixelColor(x, y);
                // Native PNG export preserves alpha exactly; opaque pixels
                // verify the authored color without unpremultiplication noise.
                if (color.alpha() == 255) {
                    CTEST_CHECK(qAbs(color.red() - cyan.red()) <= 1);
                    CTEST_CHECK(qAbs(color.green() - cyan.green()) <= 1);
                    CTEST_CHECK(qAbs(color.blue() - cyan.blue()) <= 1);
                    exactColor |= color.red() == cyan.red() && color.green() == cyan.green()
                        && color.blue() == cyan.blue();
                }
                visible = true;
            }
        }
    }
    CTEST_CHECK(visible);
    CTEST_CHECK(exactColor);
}

void SvgManagerTests::smart_select_preserves_image_alpha()
{
    QFile resource(QStringLiteral(":/qt/qml/LVRS/resources/iconset/smartSelect.svg"));
    CTEST_CHECK(resource.open(QIODevice::ReadOnly));
    const QRegularExpression embedded(QStringLiteral("data:image/png;base64,([^\"]+)"));
    const auto match = embedded.match(QString::fromUtf8(resource.readAll()));
    CTEST_CHECK(match.hasMatch());
    const QImage image = QImage::fromData(QByteArray::fromBase64(match.captured(1).toLatin1()));
    CTEST_COMPARE(image.size(), QSize(144, 144));
    QByteArray alpha;
    alpha.reserve(image.width() * image.height());
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            alpha.append(char(image.pixelColor(x, y).alpha()));
    CTEST_COMPARE(QCryptographicHash::hash(alpha, QCryptographicHash::Sha256).toHex(),
             QByteArrayLiteral("ece9cf38b3eabed8d8fb2b47306ddc220d8631e85cec81298a961aec2c6d12da"));
}

void SvgManagerTests::all_svg_icons_scale_and_center()
{
    QDirIterator files(QStringLiteral(":/lvrs-test-source-svg"),
                       {QStringLiteral("*.svg")}, QDir::Files, QDirIterator::Subdirectories);
    int count = 0;
    while (files.hasNext()) {
        QFile file(files.next());
        const QByteArray context = file.fileName().toUtf8();
        CTEST_CHECK_MESSAGE(file.open(QIODevice::ReadOnly), context.constData());
        const QByteArray svg = file.readAll();
        QXmlStreamReader xml(svg);
        CTEST_CHECK_MESSAGE(xml.readNextStartElement(), context.constData());
        const auto attributes = xml.attributes();
        CTEST_CHECK_MESSAGE(attributes.value("width") == "100%", context.constData());
        CTEST_CHECK_MESSAGE(attributes.value("height") == "100%", context.constData());
        CTEST_CHECK_MESSAGE(attributes.value("preserveAspectRatio") == "xMidYMid meet", context.constData());
        QSvgRenderer renderer(svg);
        CTEST_CHECK_MESSAGE(renderer.isValid(), context.constData());
        CTEST_CHECK_MESSAGE(renderer.viewBoxF().width() > 0, context.constData());
        CTEST_CHECK(qFuzzyCompare(renderer.viewBoxF().width(), renderer.viewBoxF().height()));
        for (int size : {18, 36, 72, 144}) {
            QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::transparent);
            QPainter painter(&image);
            renderer.render(&painter);
            painter.end();
            int left = size, top = size, right = -1, bottom = -1;
            for (int y = 0; y < size; ++y) {
                const auto *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
                for (int x = 0; x < size; ++x) {
                    if (qAlpha(row[x]) > 0) {
                        left = qMin(left, x); right = qMax(right, x);
                        top = qMin(top, y); bottom = qMax(bottom, y);
                    }
                }
            }
            const QRect bounds = right >= 0 ? QRect(left, top, right-left+1, bottom-top+1) : QRect{};
            CTEST_CHECK_MESSAGE(!bounds.isEmpty(), context.constData());
            // Up to one raster pixel per edge accounts for antialiasing.
            CTEST_CHECK_MESSAGE(qAbs(bounds.left() - (size - 1 - bounds.right())) <= 2, context.constData());
            CTEST_CHECK_MESSAGE(qAbs(bounds.top() - (size - 1 - bounds.bottom())) <= 2, context.constData());
        }
        ++count;
    }
    CTEST_COMPARE(count, 2873);
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    SvgManagerTests tests;
    runner.run("svg_manager_defaults_to_square_18_logical_pixels", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.svg_manager_defaults_to_square_18_logical_pixels(); });
    runner.run("svg_manager_generates_png_and_clamps", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.svg_manager_generates_png_and_clamps(); });
    runner.run("svg_manager_error_paths_and_cache_signals", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.svg_manager_error_paths_and_cache_signals(); });
    runner.run("figma_iconset_resources_are_complete_and_renderable", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.figma_iconset_resources_are_complete_and_renderable(); });
    runner.run("control_variant_resources_are_packaged", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.control_variant_resources_are_packaged(); });
    runner.run("provider_metadata_is_not_packaged", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.provider_metadata_is_not_packaged(); });
    runner.run("translate_object_preserves_image_alpha", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.translate_object_preserves_image_alpha(); });
    runner.run("smart_select_preserves_image_alpha", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.smart_select_preserves_image_alpha(); });
    runner.run("all_svg_icons_scale_and_center", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.all_svg_icons_scale_and_center(); });
    return runner.result();
}
