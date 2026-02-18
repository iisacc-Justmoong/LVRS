#include <QtTest>

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QDebug>
#include <QElapsedTimer>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QtPlugin>

#include <cmath>

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

namespace {

int envInt(const char *name, int fallback, int minimum, int maximum)
{
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(name, &ok);
    if (!ok)
        return qBound(minimum, fallback, maximum);
    return qBound(minimum, value, maximum);
}

double envDouble(const char *name, double fallback, double minimum, double maximum)
{
    bool ok = false;
    const double value = qEnvironmentVariable(name).toDouble(&ok);
    if (!ok)
        return qBound(minimum, fallback, maximum);
    return qBound(minimum, value, maximum);
}

QImage normalizeImage(const QImage &input, const QSize &targetSize)
{
    if (input.isNull())
        return input;

    QImage normalized = input.convertToFormat(QImage::Format_RGBA8888);
    if (normalized.size() != targetSize)
        normalized = normalized.scaled(targetSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    return normalized;
}

double mismatchRatio(const QImage &actual, const QImage &baseline, int channelTolerance)
{
    if (actual.isNull() || baseline.isNull() || actual.size() != baseline.size())
        return 1.0;

    const int width = actual.width();
    const int height = actual.height();
    int mismatchCount = 0;
    const int pixelCount = width * height;

    for (int y = 0; y < height; ++y) {
        const QRgb *aLine = reinterpret_cast<const QRgb *>(actual.constScanLine(y));
        const QRgb *bLine = reinterpret_cast<const QRgb *>(baseline.constScanLine(y));
        for (int x = 0; x < width; ++x) {
            const QRgb a = aLine[x];
            const QRgb b = bLine[x];
            const bool differs = std::abs(qRed(a) - qRed(b)) > channelTolerance
                || std::abs(qGreen(a) - qGreen(b)) > channelTolerance
                || std::abs(qBlue(a) - qBlue(b)) > channelTolerance
                || std::abs(qAlpha(a) - qAlpha(b)) > channelTolerance;
            if (differs)
                mismatchCount += 1;
        }
    }

    if (pixelCount <= 0)
        return 1.0;
    return static_cast<double>(mismatchCount) / static_cast<double>(pixelCount);
}

QImage renderReferenceScene()
{
    const QSize targetSize(320, 200);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
Item {
    width: 320
    height: 200

    Rectangle {
        anchors.fill: parent
        color: "#111827"
    }

    Rectangle {
        x: 16
        y: 16
        width: 120
        height: 72
        radius: 12
        color: "#FF5A5F"
    }

    Rectangle {
        x: 168
        y: 24
        width: 134
        height: 52
        radius: 8
        color: "#4CC9F0"
    }

    Rectangle {
        x: 94
        y: 112
        width: 184
        height: 68
        radius: 14
        color: "#4361EE"
    }
}
)", QUrl(QStringLiteral("inmemory:/visual_baseline_scene.qml")));

    if (component.status() == QQmlComponent::Loading) {
        QElapsedTimer waitTimer;
        waitTimer.start();
        while (component.status() == QQmlComponent::Loading && waitTimer.elapsed() < 5000)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }

    if (component.status() != QQmlComponent::Ready) {
        const auto errors = component.errors();
        for (const auto &err : errors)
            qWarning() << err;
        return QImage();
    }

    QObject *created = component.create();
    if (!created)
        return QImage();

    auto *rootItem = qobject_cast<QQuickItem *>(created);
    if (!rootItem)
        return QImage();

    QQuickWindow window;
    window.setColor(Qt::transparent);
    window.resize(targetSize);

    rootItem->setParentItem(window.contentItem());
    rootItem->setParent(&window);

    window.show();
    QCoreApplication::processEvents();
    QTest::qWait(120);
    QCoreApplication::processEvents();

    const QSharedPointer<QQuickItemGrabResult> grabResult = rootItem->grabToImage(targetSize);
    if (!grabResult)
        return QImage();

    QSignalSpy readySpy(grabResult.data(), &QQuickItemGrabResult::ready);
    if (!readySpy.isValid())
        return QImage();

    QElapsedTimer waitTimer;
    waitTimer.start();
    while (readySpy.count() < 1 && waitTimer.elapsed() < 5000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    if (readySpy.count() < 1)
        return QImage();

    const QImage frame = grabResult->image();
    return normalizeImage(frame, targetSize);
}

} // namespace

class VisualRegressionTests : public QObject
{
    Q_OBJECT

private slots:
    void reference_scene_matches_golden();
};

void VisualRegressionTests::reference_scene_matches_golden()
{
    const QString sourceRoot = QStringLiteral(LVRS_TEST_SOURCE_DIR);
    const QString goldenPath = sourceRoot + QStringLiteral("/golden/visual_baseline_scene.png");
    const QString diffPath = sourceRoot + QStringLiteral("/golden/visual_baseline_scene.diff.png");

    const QImage actual = renderReferenceScene();
    QVERIFY2(!actual.isNull(), "Failed to render reference scene for visual regression.");

    if (qEnvironmentVariableIsSet("LVRS_UPDATE_GOLDEN")) {
        QDir().mkpath(QFileInfo(goldenPath).absolutePath());
        QVERIFY2(actual.save(goldenPath), qPrintable(QStringLiteral("Failed to write golden image: %1").arg(goldenPath)));
        QFile::remove(diffPath);
        return;
    }

    const QImage baseline = normalizeImage(QImage(goldenPath), actual.size());
    QVERIFY2(!baseline.isNull(),
             qPrintable(QStringLiteral("Golden image is missing. Set LVRS_UPDATE_GOLDEN=1 and re-run. Path: %1").arg(goldenPath)));

    const int channelTolerance = envInt("LVRS_VISUAL_DIFF_CHANNEL_TOLERANCE", 8, 0, 64);
    const double ratioLimit = envDouble("LVRS_VISUAL_DIFF_RATIO_MAX", 0.001, 0.0, 1.0);
    const double ratio = mismatchRatio(actual, baseline, channelTolerance);

    if (ratio > ratioLimit) {
        QImage diff(actual.size(), QImage::Format_RGBA8888);
        diff.fill(Qt::transparent);
        for (int y = 0; y < actual.height(); ++y) {
            const QRgb *aLine = reinterpret_cast<const QRgb *>(actual.constScanLine(y));
            const QRgb *bLine = reinterpret_cast<const QRgb *>(baseline.constScanLine(y));
            QRgb *dLine = reinterpret_cast<QRgb *>(diff.scanLine(y));
            for (int x = 0; x < actual.width(); ++x) {
                const QRgb a = aLine[x];
                const QRgb b = bLine[x];
                const bool differs = std::abs(qRed(a) - qRed(b)) > channelTolerance
                    || std::abs(qGreen(a) - qGreen(b)) > channelTolerance
                    || std::abs(qBlue(a) - qBlue(b)) > channelTolerance
                    || std::abs(qAlpha(a) - qAlpha(b)) > channelTolerance;
                dLine[x] = differs ? qRgba(255, 0, 0, 255) : qRgba(0, 0, 0, 0);
            }
        }
        QDir().mkpath(QFileInfo(diffPath).absolutePath());
        diff.save(diffPath);
    } else {
        QFile::remove(diffPath);
    }

    qInfo().noquote() << "LVRS visual diff ratio=" << ratio
                      << "limit=" << ratioLimit
                      << "tolerance=" << channelTolerance;

    QVERIFY2(ratio <= ratioLimit,
             qPrintable(QStringLiteral("Visual regression gate failed. ratio=%1 limit=%2 diff=%3")
                            .arg(ratio, 0, 'f', 6)
                            .arg(ratioLimit, 0, 'f', 6)
                            .arg(diffPath)));
}

QTEST_MAIN(VisualRegressionTests)
#include "tst_visual_regression.moc"
