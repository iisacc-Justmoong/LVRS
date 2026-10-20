#include <QtTest>
#include <QBuffer>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>
#include <utility>
#include "test_utils.h"

class AvatarTests : public QObject
{
    Q_OBJECT
private slots:
    void sizes_and_centered_circle();
    void centered_crop_pixels_data();
    void centered_crop_pixels();
    void image_formats_data();
    void image_formats();
    void caller_resolved_relative_source_url();
    void image_provider_and_http();
    void transparent_image_pixels();
    void source_changes_and_errors();
};

static QQuickItem *createAvatar(QQmlEngine &engine)
{
    engine.addImportPath(TestUtils::qmlImportBase());
    return qobject_cast<QQuickItem *>(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS 1.0 as LV
LV.Avatar { asynchronous: true }
)"));
}

static QByteArray pngData(const QImage &image)
{
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

static QUrl dataUrl(const QImage &image)
{
    return QUrl(QStringLiteral("data:image/png;base64,") + QString::fromLatin1(pngData(image).toBase64()));
}

static QImage stripeImage(bool portrait)
{
    QImage image(portrait ? QSize(40, 120) : QSize(120, 40), QImage::Format_ARGB32);
    QPainter painter(&image);
    const QColor colors[] = {Qt::red, QColor("#22bb55"), Qt::blue};
    for (int index = 0; index < 3; ++index)
        painter.fillRect(portrait ? QRect(0, index * 40, 40, 40) : QRect(index * 40, 0, 40, 40), colors[index]);
    return image;
}

static QImage capture(QQuickItem *item)
{
    const auto grab = item->grabToImage();
    if (!grab)
        return {};
    QSignalSpy ready(grab.data(), &QQuickItemGrabResult::ready);
    if (ready.isEmpty() && !ready.wait(5000))
        return {};
    return grab->image().scaled(item->size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void AvatarTests::sizes_and_centered_circle()
{
    QQmlEngine engine;
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    QVERIFY(avatar);
    QCOMPARE(avatar->size(), QSizeF(64, 64));
    for (const auto &[size, diameter] : {std::pair{0, 24}, {1, 40}, {2, 64}}) {
        QVERIFY(avatar->setProperty("size", size));
        QCOMPARE(avatar->implicitWidth(), diameter);
        QCOMPARE(avatar->implicitHeight(), diameter);
    }
    avatar->setWidth(100);
    avatar->setHeight(80);
    auto *circle = avatar->findChild<QQuickItem *>("avatar_circle");
    QVERIFY(circle);
    QCOMPARE(circle->size(), QSizeF(80, 80));
    QCOMPARE(circle->position(), QPointF(10, 0));
    QCOMPARE(avatar->property("status").toInt(), 0); // Image.Null
}

void AvatarTests::centered_crop_pixels_data()
{
    QTest::addColumn<bool>("portrait");
    QTest::addColumn<int>("diameter");
    for (bool portrait : {false, true}) {
        for (int diameter : {24, 40, 64, 96}) {
            const auto name = QByteArray(portrait ? "portrait-" : "landscape-") + QByteArray::number(diameter);
            QTest::newRow(name.constData()) << portrait << diameter;
        }
    }
}

void AvatarTests::centered_crop_pixels()
{
    QFETCH(bool, portrait);
    QFETCH(int, diameter);
    QQmlEngine engine;
    QQuickWindow window;
    window.resize(diameter, diameter);
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    QVERIFY(avatar);
    avatar->setSize(QSizeF(diameter, diameter));
    avatar->setParentItem(window.contentItem());
    avatar->setProperty("source", dataUrl(stripeImage(portrait)));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    qInfo() << "Avatar graphics API" << window.rendererInterface()->graphicsApi();
    if (qEnvironmentVariableIntValue("LVRS_AVATAR_REQUIRE_GPU"))
        QVERIFY(window.rendererInterface()->graphicsApi() != QSGRendererInterface::Software
            && window.rendererInterface()->graphicsApi() != QSGRendererInterface::Unknown);
    QTRY_COMPARE(avatar->property("status").toInt(), 1); // Image.Ready
    QImage pixels;
    QTRY_VERIFY_WITH_TIMEOUT(!(pixels = capture(avatar.data())).isNull()
        && pixels.pixelColor(diameter / 2, diameter / 2).green() > 150, 5000);
    for (const QPoint point : {QPoint(diameter / 2, diameter / 2), QPoint(diameter / 4, diameter / 2),
             QPoint(diameter * 3 / 4, diameter / 2), QPoint(diameter / 2, diameter / 4),
             QPoint(diameter / 2, diameter * 3 / 4)}) {
        const QColor actual = pixels.pixelColor(point);
        QVERIFY2(qAbs(actual.red() - 34) < 8 && qAbs(actual.green() - 187) < 8
            && qAbs(actual.blue() - 85) < 8, qPrintable(actual.name()));
    }
    for (const QPoint point : {QPoint(0, 0), QPoint(diameter - 1, 0),
             QPoint(0, diameter - 1), QPoint(diameter - 1, diameter - 1)})
        QVERIFY(pixels.pixelColor(point).alpha() < 10);
    const QString directory = qEnvironmentVariable("LVRS_AVATAR_CAPTURE_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QVERIFY(pixels.save(directory + QString("/%1-%2.png").arg(portrait ? "portrait" : "landscape").arg(diameter)));
    }
}

void AvatarTests::image_formats_data()
{
    QTest::addColumn<QByteArray>("format");
    for (const QByteArray &format : {QByteArray("png"), QByteArray("jpeg"), QByteArray("webp"), QByteArray("bmp"), QByteArray("svg"), QByteArray("gif")})
        QTest::newRow(format.constData()) << format;
}

void AvatarTests::image_formats()
{
    QFETCH(QByteArray, format);
    QVERIFY2(QImageReader::supportedImageFormats().contains(format), format.constData());
    QTemporaryDir fixtures(QCoreApplication::applicationDirPath() + "/avatar-fixtures-XXXXXX");
    QVERIFY(fixtures.isValid());
    const QString path = fixtures.filePath("profile." + QString::fromLatin1(format));
    if (format == "svg" || format == "gif") {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        const QByteArray content = format == "svg"
            ? QByteArray("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"120\" height=\"40\"><rect width=\"120\" height=\"40\" fill=\"#22bb55\"/></svg>")
            : QByteArray::fromBase64("R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7");
        QCOMPARE(file.write(content), content.size());
    } else {
        QVERIFY2(QImageWriter::supportedImageFormats().contains(format), format.constData());
        QVERIFY(stripeImage(false).save(path, format.constData()));
    }
    QQmlEngine engine;
    QQuickWindow window;
    window.resize(64, 64);
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    QVERIFY(avatar);
    avatar->setParentItem(window.contentItem());
    avatar->setProperty("source", QUrl::fromLocalFile(path));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_COMPARE(avatar->property("status").toInt(), 1);
    if (format != "gif") {
        QImage pixels;
        QTRY_VERIFY_WITH_TIMEOUT(!(pixels = capture(avatar.data())).isNull()
            && pixels.pixelColor(32, 32).green() > 150, 5000);
        QVERIFY(pixels.pixelColor(0, 0).alpha() < 10);
    }
}

class AvatarImageProvider : public QQuickImageProvider
{
public:
    AvatarImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}
    QImage requestImage(const QString &, QSize *size, const QSize &) override
    {
        auto image = stripeImage(true);
        *size = image.size();
        return image;
    }
};

void AvatarTests::caller_resolved_relative_source_url()
{
    QTemporaryDir fixtures(QCoreApplication::applicationDirPath() + "/avatar-relative-XXXXXX");
    QVERIFY(fixtures.isValid());
    QVERIFY(stripeImage(false).save(fixtures.filePath("profile.png")));
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import LVRS 1.0 as LV
LV.Avatar { source: Qt.resolvedUrl("profile.png"); asynchronous: false }
)", QUrl::fromLocalFile(fixtures.filePath("Main.qml")));
    QScopedPointer<QObject> avatar(component.create());
    QVERIFY2(avatar, qPrintable(component.errorString()));
    QTRY_COMPARE(avatar->property("status").toInt(), 1);
    QCOMPARE(avatar->property("source").toUrl(), QUrl::fromLocalFile(fixtures.filePath("profile.png")));
}

void AvatarTests::image_provider_and_http()
{
    QQmlEngine engine;
    engine.addImageProvider("avatar-test", new AvatarImageProvider);
    QQuickWindow window;
    window.resize(64, 64);
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    QVERIFY(avatar);
    avatar->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    connect(&server, &QTcpServer::newConnection, &server, [&server]() {
        auto *socket = server.nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            socket->readAll();
            const auto bytes = pngData(stripeImage(false));
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: "
                + QByteArray::number(bytes.size()) + "\r\nConnection: close\r\n\r\n" + bytes);
            socket->disconnectFromHost();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    });
    for (const QUrl &source : {QUrl("image://avatar-test/person"),
             QUrl(QString("http://127.0.0.1:%1/profile.png").arg(server.serverPort())),
             QUrl("qrc:/qt/qml/LVRS/resources/images/card/member.svg")}) {
        avatar->setProperty("source", source);
        QTRY_COMPARE(avatar->property("status").toInt(), 1);
        QImage pixels;
        QTRY_VERIFY_WITH_TIMEOUT(!(pixels = capture(avatar.data())).isNull()
            && pixels.pixelColor(32, 32).alpha() > 240, 5000);
        QVERIFY(pixels.pixelColor(0, 0).alpha() < 10);
        if (source.scheme() != "qrc")
            QVERIFY(pixels.pixelColor(32, 32).green() > 150);
    }
}

void AvatarTests::source_changes_and_errors()
{
    QQmlEngine engine;
    QQuickWindow window;
    window.resize(64, 64);
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    QVERIFY(avatar);
    avatar->setParentItem(window.contentItem());
    avatar->setProperty("backgroundColor", QColor(Qt::transparent));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    for (QColor color : {QColor(Qt::red), QColor(Qt::blue)}) {
        QImage image(30, 70, QImage::Format_ARGB32);
        image.fill(color);
        avatar->setProperty("source", dataUrl(image));
        QTRY_COMPARE(avatar->property("status").toInt(), 1);
        QImage pixels;
        QTRY_VERIFY_WITH_TIMEOUT(!(pixels = capture(avatar.data())).isNull()
            && pixels.pixelColor(32, 32) == color, 5000);
    }
    avatar->setProperty("source", QUrl("file:///nonexistent-lvrs-avatar.png"));
    QTRY_COMPARE(avatar->property("status").toInt(), 3); // Image.Error
    QImage pixels;
    QTRY_VERIFY(!(pixels = capture(avatar.data())).isNull() && pixels.pixelColor(32, 32).alpha() == 0);
    avatar->setProperty("source", QUrl());
    QTRY_COMPARE(avatar->property("status").toInt(), 0);
    QTRY_VERIFY(!(pixels = capture(avatar.data())).isNull() && pixels.pixelColor(32, 32).alpha() == 0);
    avatar->setSize(QSizeF(100, 80));
    avatar->setProperty("source", dataUrl(stripeImage(false)));
    QTRY_COMPARE(avatar->property("status").toInt(), 1);
    QTRY_VERIFY(!(pixels = capture(avatar.data())).isNull() && pixels.pixelColor(50, 40).green() > 150);
    QVERIFY(pixels.pixelColor(1, 40).alpha() == 0);
    avatar->setSize(QSizeF(0, 0));
    QCOMPARE(avatar->findChild<QQuickItem *>("avatar_circle")->size(), QSizeF(0, 0));
}

void AvatarTests::transparent_image_pixels()
{
    QQmlEngine engine;
    QQuickWindow window;
    window.resize(64, 64);
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    QVERIFY(avatar);
    avatar->setParentItem(window.contentItem());
    avatar->setProperty("backgroundColor", QColor(Qt::transparent));
    QImage image(64, 64, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    {
        QPainter painter(&image);
        painter.fillRect(QRect(24, 0, 16, 64), Qt::red);
    }
    avatar->setProperty("source", dataUrl(image));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_COMPARE(avatar->property("status").toInt(), 1);
    QImage pixels;
    QTRY_VERIFY(!(pixels = capture(avatar.data())).isNull() && pixels.pixelColor(32, 32) == QColor(Qt::red));
    QCOMPARE(pixels.pixelColor(10, 32).alpha(), 0);
    QCOMPARE(pixels.pixelColor(0, 0).alpha(), 0);
    avatar->setProperty("backgroundColor", QColor(Qt::yellow));
    QTRY_VERIFY(!(pixels = capture(avatar.data())).isNull() && pixels.pixelColor(10, 32) == QColor(Qt::yellow));
    QCOMPARE(pixels.pixelColor(32, 32), QColor(Qt::red));
    QCOMPARE(pixels.pixelColor(0, 0).alpha(), 0);
}

QTEST_MAIN(AvatarTests)
#include "tst_avatar.moc"
