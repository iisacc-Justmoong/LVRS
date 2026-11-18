#include "QtEventWait.h"
#include <QGuiApplication>
#include <QFile>
#include <QSGRendererInterface>
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

class AvatarTests
{
public:
    void sizes_and_centered_circle();
    void centered_crop_pixels(bool portrait, int diameter);
    void image_formats(const QByteArray &format);
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
    bool ready = false;
    const auto captureConnection = QObject::connect(grab.data(), &QQuickItemGrabResult::ready,
                                                     grab.data(), [&ready] { ready = true; });
    if (!captureConnection)
        return {};
    const bool completed = lvrs_ctest::waitFor([&ready] { return ready; }, 5000);
    QObject::disconnect(captureConnection);
    if (!completed)
        return {};
    return grab->image().scaled(item->size().toSize(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void AvatarTests::sizes_and_centered_circle()
{
    QQmlEngine engine;
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    CTEST_CHECK(avatar);
    CTEST_COMPARE(avatar->size(), QSizeF(64, 64));
    for (const auto &[size, diameter] : {std::pair{0, 24}, {1, 40}, {2, 64}}) {
        CTEST_CHECK(avatar->setProperty("size", size));
        CTEST_COMPARE(avatar->implicitWidth(), diameter);
        CTEST_COMPARE(avatar->implicitHeight(), diameter);
    }
    avatar->setWidth(100);
    avatar->setHeight(80);
    auto *circle = avatar->findChild<QQuickItem *>("avatar_circle");
    CTEST_CHECK(circle);
    CTEST_COMPARE(circle->size(), QSizeF(80, 80));
    CTEST_COMPARE(circle->position(), QPointF(10, 0));
    CTEST_COMPARE(avatar->property("status").toInt(), 0); // Image.Null
}

void AvatarTests::centered_crop_pixels(bool portrait, int diameter)
{
    QQmlEngine engine;
    QQuickWindow window;
    window.resize(diameter, diameter);
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    CTEST_CHECK(avatar);
    avatar->setSize(QSizeF(diameter, diameter));
    avatar->setParentItem(window.contentItem());
    avatar->setProperty("source", dataUrl(stripeImage(portrait)));
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    qInfo() << "Avatar graphics API" << window.rendererInterface()->graphicsApi();
    if (qEnvironmentVariableIntValue("LVRS_AVATAR_REQUIRE_GPU"))
        CTEST_CHECK(window.rendererInterface()->graphicsApi() != QSGRendererInterface::Software
            && window.rendererInterface()->graphicsApi() != QSGRendererInterface::Unknown);
    CTEST_WAIT_COMPARE(avatar->property("status").toInt(), 1); // Image.Ready
    QImage pixels;
    CTEST_WAIT_VERIFY(!(pixels = capture(avatar.data())).isNull()
        && pixels.pixelColor(diameter / 2, diameter / 2).green() > 150, 5000);
    for (const QPoint point : {QPoint(diameter / 2, diameter / 2), QPoint(diameter / 4, diameter / 2),
             QPoint(diameter * 3 / 4, diameter / 2), QPoint(diameter / 2, diameter / 4),
             QPoint(diameter / 2, diameter * 3 / 4)}) {
        const QColor actual = pixels.pixelColor(point);
        CTEST_CHECK_MESSAGE(qAbs(actual.red() - 34) < 8 && qAbs(actual.green() - 187) < 8
            && qAbs(actual.blue() - 85) < 8, qPrintable(actual.name()));
    }
    for (const QPoint point : {QPoint(0, 0), QPoint(diameter - 1, 0),
             QPoint(0, diameter - 1), QPoint(diameter - 1, diameter - 1)})
        CTEST_CHECK(pixels.pixelColor(point).alpha() < 10);
    const QString directory = qEnvironmentVariable("LVRS_AVATAR_CAPTURE_DIR");
    if (!directory.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(directory));
        CTEST_CHECK(pixels.save(directory + QString("/%1-%2.png").arg(portrait ? "portrait" : "landscape").arg(diameter)));
    }
}

void AvatarTests::image_formats(const QByteArray &format)
{
    CTEST_CHECK_MESSAGE(QImageReader::supportedImageFormats().contains(format), format.constData());
    const QString resourceRoot = QDir::cleanPath(QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources"));
    CTEST_CHECK(QDir().mkpath(resourceRoot));
    QTemporaryDir fixtures(QDir(resourceRoot).filePath(QStringLiteral("ctest-avatar-XXXXXX")));
    CTEST_CHECK(fixtures.isValid());
    qInfo().noquote() << "Avatar test workspace:" << fixtures.path();
    const QString path = fixtures.filePath("profile." + QString::fromLatin1(format));
    if (format == "svg" || format == "gif") {
        QFile file(path);
        CTEST_CHECK(file.open(QIODevice::WriteOnly));
        const QByteArray content = format == "svg"
            ? QByteArray("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"120\" height=\"40\"><rect width=\"120\" height=\"40\" fill=\"#22bb55\"/></svg>")
            : QByteArray::fromBase64("R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7");
        CTEST_COMPARE(file.write(content), content.size());
    } else {
        CTEST_CHECK_MESSAGE(QImageWriter::supportedImageFormats().contains(format), format.constData());
        CTEST_CHECK(stripeImage(false).save(path, format.constData()));
    }
    QQmlEngine engine;
    QQuickWindow window;
    window.resize(64, 64);
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    CTEST_CHECK(avatar);
    avatar->setParentItem(window.contentItem());
    avatar->setProperty("source", QUrl::fromLocalFile(path));
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    CTEST_WAIT_COMPARE(avatar->property("status").toInt(), 1);
    if (format != "gif") {
        QImage pixels;
        CTEST_WAIT_VERIFY(!(pixels = capture(avatar.data())).isNull()
            && pixels.pixelColor(32, 32).green() > 150, 5000);
        CTEST_CHECK(pixels.pixelColor(0, 0).alpha() < 10);
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
    const QString resourceRoot = QDir::cleanPath(QStringLiteral(LVRS_TEST_SOURCE_DIR "/../resources"));
    CTEST_CHECK(QDir().mkpath(resourceRoot));
    QTemporaryDir fixtures(QDir(resourceRoot).filePath(QStringLiteral("ctest-avatar-XXXXXX")));
    CTEST_CHECK(fixtures.isValid());
    qInfo().noquote() << "Avatar test workspace:" << fixtures.path();
    CTEST_CHECK(stripeImage(false).save(fixtures.filePath("profile.png")));
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import LVRS 1.0 as LV
LV.Avatar { source: Qt.resolvedUrl("profile.png"); asynchronous: false }
)", QUrl::fromLocalFile(fixtures.filePath("Main.qml")));
    QScopedPointer<QObject> avatar(component.create());
    CTEST_CHECK_MESSAGE(avatar, qPrintable(component.errorString()));
    CTEST_WAIT_COMPARE(avatar->property("status").toInt(), 1);
    CTEST_COMPARE(avatar->property("source").toUrl(), QUrl::fromLocalFile(fixtures.filePath("profile.png")));
}

void AvatarTests::image_provider_and_http()
{
    QQmlEngine engine;
    engine.addImageProvider("avatar-test", new AvatarImageProvider);
    QQuickWindow window;
    window.resize(64, 64);
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    CTEST_CHECK(avatar);
    avatar->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    QTcpServer server;
    CTEST_CHECK(server.listen(QHostAddress::LocalHost));
    QObject::connect(&server, &QTcpServer::newConnection, &server, [&server]() {
        auto *socket = server.nextPendingConnection();
        QObject::connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            socket->readAll();
            const auto bytes = pngData(stripeImage(false));
            socket->write("HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nContent-Length: "
                + QByteArray::number(bytes.size()) + "\r\nConnection: close\r\n\r\n" + bytes);
            socket->disconnectFromHost();
        });
        QObject::connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
    });
    for (const QUrl &source : {QUrl("image://avatar-test/person"),
             QUrl(QString("http://127.0.0.1:%1/profile.png").arg(server.serverPort())),
             QUrl("qrc:/qt/qml/LVRS/resources/images/card/member.svg")}) {
        avatar->setProperty("source", source);
        const bool ready = lvrs_ctest::waitFor([&] { return avatar->property("status").toInt() == 1; }, 5000);
        CTEST_CHECK_MESSAGE(ready, qPrintable(QStringLiteral("Avatar source %1 did not become ready; status=%2")
            .arg(source.toString()).arg(avatar->property("status").toInt())));
        QImage pixels;
        CTEST_WAIT_VERIFY(!(pixels = capture(avatar.data())).isNull()
            && pixels.pixelColor(32, 32).alpha() > 240, 5000);
        CTEST_CHECK(pixels.pixelColor(0, 0).alpha() < 10);
        if (source.scheme() != "qrc")
            CTEST_CHECK(pixels.pixelColor(32, 32).green() > 150);
    }
}

void AvatarTests::source_changes_and_errors()
{
    QQmlEngine engine;
    QQuickWindow window;
    window.resize(64, 64);
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    CTEST_CHECK(avatar);
    avatar->setParentItem(window.contentItem());
    avatar->setProperty("backgroundColor", QColor(Qt::transparent));
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    for (QColor color : {QColor(Qt::red), QColor(Qt::blue)}) {
        QImage image(30, 70, QImage::Format_ARGB32);
        image.fill(color);
        avatar->setProperty("source", dataUrl(image));
        CTEST_WAIT_COMPARE(avatar->property("status").toInt(), 1);
        QImage pixels;
        CTEST_WAIT_VERIFY(!(pixels = capture(avatar.data())).isNull()
            && pixels.pixelColor(32, 32) == color, 5000);
    }
    avatar->setProperty("source", QUrl("file:///nonexistent-lvrs-avatar.png"));
    CTEST_WAIT_COMPARE(avatar->property("status").toInt(), 3); // Image.Error
    QImage pixels;
    CTEST_WAIT_DEFAULT(!(pixels = capture(avatar.data())).isNull() && pixels.pixelColor(32, 32).alpha() == 0);
    avatar->setProperty("source", QUrl());
    CTEST_WAIT_COMPARE(avatar->property("status").toInt(), 0);
    CTEST_WAIT_DEFAULT(!(pixels = capture(avatar.data())).isNull() && pixels.pixelColor(32, 32).alpha() == 0);
    avatar->setSize(QSizeF(100, 80));
    avatar->setProperty("source", dataUrl(stripeImage(false)));
    CTEST_WAIT_COMPARE(avatar->property("status").toInt(), 1);
    CTEST_WAIT_DEFAULT(!(pixels = capture(avatar.data())).isNull() && pixels.pixelColor(50, 40).green() > 150);
    CTEST_CHECK(pixels.pixelColor(1, 40).alpha() == 0);
    avatar->setSize(QSizeF(0, 0));
    CTEST_COMPARE(avatar->findChild<QQuickItem *>("avatar_circle")->size(), QSizeF(0, 0));
}

void AvatarTests::transparent_image_pixels()
{
    QQmlEngine engine;
    QQuickWindow window;
    window.resize(64, 64);
    QScopedPointer<QQuickItem> avatar(createAvatar(engine));
    CTEST_CHECK(avatar);
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
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    CTEST_WAIT_COMPARE(avatar->property("status").toInt(), 1);
    QImage pixels;
    CTEST_WAIT_DEFAULT(!(pixels = capture(avatar.data())).isNull() && pixels.pixelColor(32, 32) == QColor(Qt::red));
    CTEST_COMPARE(pixels.pixelColor(10, 32).alpha(), 0);
    CTEST_COMPARE(pixels.pixelColor(0, 0).alpha(), 0);
    avatar->setProperty("backgroundColor", QColor(Qt::yellow));
    CTEST_WAIT_DEFAULT(!(pixels = capture(avatar.data())).isNull() && pixels.pixelColor(10, 32) == QColor(Qt::yellow));
    CTEST_COMPARE(pixels.pixelColor(32, 32), QColor(Qt::red));
    CTEST_COMPARE(pixels.pixelColor(0, 0).alpha(), 0);
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    AvatarTests tests;
    runner.run("sizes_and_centered_circle", [&] { tests.sizes_and_centered_circle(); });
    for (bool portrait : {false, true}) {
        for (int diameter : {24, 40, 64, 96}) {
            const std::string name = std::string("centered_crop_pixels:")
                + (portrait ? "portrait-" : "landscape-") + std::to_string(diameter);
            runner.run(name, [&] { tests.centered_crop_pixels(portrait, diameter); });
        }
    }
    for (const QByteArray &format : {QByteArray("png"), QByteArray("jpeg"), QByteArray("webp"),
                                     QByteArray("bmp"), QByteArray("svg"), QByteArray("gif")}) {
        const std::string name = "image_formats:" + format.toStdString();
        runner.run(name, [&] { tests.image_formats(format); });
    }
    runner.run("caller_resolved_relative_source_url", [&] { tests.caller_resolved_relative_source_url(); });
    runner.run("image_provider_and_http", [&] { tests.image_provider_and_http(); });
    runner.run("transparent_image_pixels", [&] { tests.transparent_image_pixels(); });
    runner.run("source_changes_and_errors", [&] { tests.source_changes_and_errors(); });
    return runner.result();
}
