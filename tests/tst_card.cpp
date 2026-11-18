#include "QtEventWait.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QTimer>
#include <type_traits>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QQmlEngine>
#include <functional>
#include "test_utils.h"

class CardSignalRecorder : public QObject
{
    Q_OBJECT
public:
    int clicked = 0;
    int menu = 0;
    int action = 0;
public slots:
    void recordClicked() { ++clicked; }
    void recordMenu() { ++menu; }
    void recordAction() { ++action; }
};

namespace {
struct CardVariant {
    int type;
    int size;
    int detail;
    int displayState;
    QSize dimensions;
    bool mobile;

    QString tag() const
    {
        return QString("%1-%2-%3-%4-%5")
            .arg(type).arg(size).arg(detail).arg(displayState).arg(mobile);
    }
};

QList<CardVariant> cardVariants()
{
    QList<CardVariant> result;
    for (bool mobile : {false, true}) {
        for (int type = 0; type < 8; ++type) {
            for (int size = 0; size < (type == 0 ? 3 : 1); ++size) {
                for (int detail = 0; detail < (type == 0 ? 2 : 1); ++detail) {
                    for (int state = 1; state <= (type == 1 ? 1 : 3); ++state) {
                        const QSize fileSizes[] = {{140, 160}, {210, 240}, {360, 280}};
                        const QSize dimensions = type == 0 ? fileSizes[size]
                            : type == 1 ? QSize(480, 320) : QSize(256, 280);
                        result.append({type, size, detail, state, dimensions, mobile});
                    }
                }
            }
        }
    }
    return result;
}

template <typename Actual, typename Expected>
bool cardEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual) + 1.0, qreal(expected) + 1.0);
    else
        return actual == expected;
}

void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
    loop.exec();
}

void sendMouse(QQuickWindow *window, QEvent::Type type, const QPoint &point,
               Qt::MouseButton button, Qt::MouseButtons buttons)
{
    static ulong timestamp = 10000;
    timestamp += 16;
    const QPointF local(point);
    QMouseEvent event(type, local, local, QPointF(window->mapToGlobal(point)),
                      button, buttons, Qt::NoModifier);
    event.setTimestamp(timestamp);
    QCoreApplication::sendEvent(window, &event);
    QCoreApplication::processEvents();
}

void mouseClick(QQuickWindow *window, const QPoint &point)
{
    sendMouse(window, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton);
    sendMouse(window, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
}

void keyClick(QQuickWindow *window, Qt::Key key)
{
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}
}

#define CARD_COMPARE(actual, expected) CTEST_CHECK(cardEqual((actual), (expected)))
#define CARD_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(cardEqual((actual), (expected)))

class CardTests
{
public:
    void figma_contract(const CardVariant &variant);
    void mouse_keyboard_and_nested_actions();
    void content_reflows_and_preview_swaps();
    void preview_errors_and_fill_pixels();
    void catalog_renders_all_designs();
};

void CardTests::figma_contract(const CardVariant &variant)
{
    const auto &[type, size, detail, displayState, dimensions, mobile] = variant;
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, QString(R"(
import QtQuick
import LVRS as LV
LV.Card {
    type: %1; size: %2; detail: %3; displayState: %4
    title: "Coastal house.png"
    description: "Everything for the next release"
    details: "Warm limestone, curved walls and a quiet view of the sea."
    metadata: "PNG · 1536 × 1024"
    summary: "24 items · 1.8 GB"
    footnote: "Updated today"
    rows: progressCard ? [{label: "Next", value: "Review"}] : [{label: "Guidelines", value: "12 files"}, {label: "Photography", value: "8 files"}, {label: "Templates", value: "4 files"}]
    Component.onCompleted: LV.Theme.targetOverride = "%5"
})").arg(type).arg(size).arg(detail).arg(displayState).arg(mobile ? "ios" : "macos").toUtf8()));
    CTEST_CHECK(object);
    auto *card = qobject_cast<QQuickItem *>(object.data());
    CTEST_CHECK(card);
    CARD_WAIT_COMPARE(card->implicitWidth(), dimensions.width());
    CARD_WAIT_COMPARE(card->implicitHeight(), dimensions.height());
    CARD_WAIT_COMPARE(card->size(), QSizeF(dimensions));
    CARD_COMPARE(card->property("effectiveSelected").toBool(), type != 1 && displayState == 3);
    CARD_COMPARE(card->property("borderWidth").toReal(), type == 1 ? 0.0 : displayState == 3 ? 2.0 : 1.0);
    CARD_COMPARE(card->property("resolvedCornerRadius").toReal(), type == 0 ? 12.0 : 0.0);
    if (type >= 2)
        CARD_COMPARE(card->property("surfaceColor").value<QColor>().alpha(), 0);
    else
        CTEST_CHECK(card->property("surfaceColor").value<QColor>().alpha() > 0);
    if (type == 0) {
        auto *caption = card->findChild<QQuickItem *>("card_caption");
        CTEST_CHECK(caption);
        const int briefHeights[] = {37, 39, 39};
        const int detailedHeights[] = {75, 99, 81};
        CARD_WAIT_COMPARE(caption->height(), detail ? detailedHeights[size] : briefHeights[size]);
        CARD_COMPARE(caption->x(), (size == 0 ? 12 : 18) + (displayState == 3 ? 2.0 : 1.0));
        CARD_WAIT_COMPARE(caption->width(), card->width() - 2 * caption->x());
        CARD_WAIT_COMPARE(caption->y() + caption->height(), card->height() - caption->x());
        auto *preview = card->findChild<QQuickItem *>("card_previewImage");
        auto *scrim = card->findChild<QQuickItem *>("card_scrim");
        CTEST_CHECK(preview && scrim);
        CARD_WAIT_COMPARE(preview->size(), QSizeF(dimensions));
        const int briefScrimHeights[] = {113, 121, 121};
        const int detailedScrimHeights[] = {151, 181, 163};
        CARD_WAIT_COMPARE(scrim->height(), detail ? detailedScrimHeights[size] : briefScrimHeights[size]);
        auto *title = card->findChild<QQuickItem *>("card_filename");
        CTEST_CHECK(title);
        CARD_COMPARE(title->property("font").value<QFont>().pixelSize(), size == 0 ? 13 : 15);
        CARD_COMPARE(title->property("font").value<QFont>().weight(), QFont::DemiBold);
    } else if (type >= 2) {
        auto *header = card->findChild<QQuickItem *>("card_header");
        CTEST_CHECK(header);
        CARD_WAIT_COMPARE(header->height(), 36.0);
        auto *footer = card->findChild<QQuickItem *>("card_footer");
        CTEST_CHECK(footer);
        CARD_WAIT_COMPARE(footer->mapToItem(card, QPointF()).y(), displayState == 3 ? 238.0 : 239.0);
    }
}

void CardTests::mouse_keyboard_and_nested_actions()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(400, 400);
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.Card { type: LV.Card.Folder; title: "Brand assets"; statusText: "Shared"; footnote: "Updated today" }
)"));
    CTEST_CHECK(object);
    auto *card = qobject_cast<QQuickItem *>(object.data());
    card->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    CardSignalRecorder observations;
    const auto clickedConnection = QObject::connect(card, SIGNAL(clicked()), &observations, SLOT(recordClicked()));
    const auto menuConnection = QObject::connect(card, SIGNAL(menuRequested()), &observations, SLOT(recordMenu()));
    const auto actionConnection = QObject::connect(card, SIGNAL(actionTriggered()), &observations, SLOT(recordAction()));
    CTEST_CHECK(clickedConnection && menuConnection && actionConnection);
#ifdef Q_OS_WIN
    // Exposure alone does not make the native Windows input window active.
    window.requestActivate();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isActive(); }, 5000));
#endif
    sendMouse(&window, QEvent::MouseMove, QPoint(380, 380), Qt::NoButton, Qt::NoButton);
    sendMouse(&window, QEvent::MouseMove, QPoint(100, 100), Qt::NoButton, Qt::NoButton);
    CTEST_WAIT_DEFAULT(card->property("effectiveHovered").toBool());
    mouseClick(&window, QPoint(100, 100));
    CARD_COMPARE(observations.clicked, 1);
    CTEST_CHECK(card->property("selected").toBool());
    for (const QString &name : {QString("card_menu"), QString("card_action")}) {
        auto *button = card->findChild<QQuickItem *>(name);
        CTEST_CHECK(button);
        const QPoint point = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();
        mouseClick(&window, point);
        CARD_COMPARE(observations.clicked, 1);
        CTEST_CHECK(card->property("selected").toBool());
    }
    CARD_COMPARE(observations.menu, 1);
    CARD_COMPARE(observations.action, 1);
    card->forceActiveFocus();
    keyClick(&window, Qt::Key_Space);
    CARD_WAIT_COMPARE(observations.clicked, 2);
    CTEST_CHECK(!card->property("selected").toBool());
    card->setEnabled(false);
    mouseClick(&window, QPoint(100, 100));
    keyClick(&window, Qt::Key_Space);
    CARD_COMPARE(observations.clicked, 2);
    CARD_COMPARE(observations.menu, 1);
}

void CardTests::content_reflows_and_preview_swaps()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.Card {
    width: 360; height: 240
    size: LV.Card.Large; detail: LV.Card.Detailed; selected: true; showMenu: true
    title: "A very long filename that must truncate without pushing the menu out of the card.png"
    details: "A long description that wraps over multiple lines and continues well past the available two lines in this landscape card. Additional text must remain within the caption and never overlap metadata."
    metadata: "PNG · 1536 × 1024"
    previewComponent: Component { Rectangle { objectName: "customPreview"; color: "#cc5500" } }
})"));
    CTEST_CHECK(object);
    auto *card = qobject_cast<QQuickItem *>(object.data());
    auto *preview = card->findChild<QQuickItem *>("customPreview");
    CTEST_CHECK(preview);
    CARD_WAIT_COMPARE(preview->size(), QSizeF(360, 240));
    auto *caption = card->findChild<QQuickItem *>("card_caption");
    CTEST_CHECK(caption);
    CARD_WAIT_COMPARE(caption->size(), QSizeF(320, 81));
    card->setWidth(240);
    CARD_WAIT_COMPARE(preview->width(), 240.0);
    CARD_WAIT_COMPARE(caption->width(), 200.0);
    for (const QString &name : {QString("card_filename"), QString("card_details"), QString("card_metadata")}) {
        auto *label = card->findChild<QQuickItem *>(name);
        CTEST_CHECK(label);
        const QPointF bottom = label->mapToItem(card, QPointF(label->width(), label->height()));
        CTEST_CHECK(bottom.x() <= card->width() && bottom.y() <= card->height());
    }
    card->setProperty("type", 3); // Project
    card->setHeight(280);
    card->setProperty("progress", 2.0);
    auto *bar = card->findChild<QQuickItem *>("card_progress");
    CTEST_CHECK(bar);
    CARD_WAIT_COMPARE(bar->property("progress").toDouble(), 1.0);
    card->setProperty("progress", -1.0);
    CARD_WAIT_COMPARE(bar->property("progress").toDouble(), 0.0);
    card->setProperty("showMenu", false);
    card->setProperty("showAction", false);
    CTEST_CHECK(!card->findChild<QQuickItem *>("card_menu")->isVisible());
    CTEST_CHECK(!card->findChild<QQuickItem *>("card_action")->isVisible());
}

void CardTests::preview_errors_and_fill_pixels()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(240, 240);
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.Card { size: LV.Card.Small; previewSource: "file:///nonexistent-lvrs-card.png"; title: "Missing file" }
)"));
    CTEST_CHECK(object);
    auto *card = qobject_cast<QQuickItem *>(object.data());
    card->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    CARD_WAIT_COMPARE(card->property("previewStatus").toInt(), 3); // Image.Error
    const QString fixture = QStringLiteral(LVRS_TEST_SOURCE_DIR) + "/../example/VisualCatalog/assets/cards/architecture.png";
    CTEST_CHECK(QFileInfo::exists(fixture));
    card->setProperty("previewSource", QUrl::fromLocalFile(fixture));
    CARD_WAIT_COMPARE(card->property("previewStatus").toInt(), 1); // Image.Ready
    auto *image = card->findChild<QQuickItem *>("card_previewImage");
    CTEST_CHECK(image);
    CARD_COMPARE(image->size(), card->size());
    CARD_COMPARE(image->property("fillMode").toInt(), 2); // PreserveAspectCrop
    qInfo() << "Decoded preview" << image->property("sourceSize") << image->implicitWidth() << image->implicitHeight();
    processFor(100);
    const auto capture = card->grabToImage(card->size().toSize());
    CTEST_CHECK(capture);
    int readyCount = 0;
    const auto captureConnection = QObject::connect(capture.data(), &QQuickItemGrabResult::ready,
                                                     [&] { ++readyCount; });
    CTEST_CHECK(captureConnection);
    const auto disconnectCapture = qScopeGuard([&] { QObject::disconnect(captureConnection); });
    CTEST_WAIT_VERIFY(readyCount > 0, 5000);
    const QImage pixels = capture->image();
    CTEST_CHECK(!pixels.isNull());
    const qreal pixelScale = window.devicePixelRatio();
    CARD_COMPARE(card->size(), QSizeF(140, 160));
    CARD_COMPARE(pixels.size(), QSize(140, 160) * pixelScale);
    const QString captureDir = qEnvironmentVariable("LVRS_CARD_CAPTURE_DIR");
    if (!captureDir.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(captureDir));
        CTEST_CHECK(pixels.save(captureDir + "/file-small.png"));
    }
    const int centerX = pixels.width() / 2;
    const int photoY = qRound(30 * pixelScale);
    const int scrimY = pixels.height() - qRound(12 * pixelScale);
    qInfo() << "Card graphics API" << window.rendererInterface()->graphicsApi()
            << "preview pixel" << pixels.pixelColor(centerX, photoY);
    CTEST_CHECK(pixels.pixelColor(0, 0).alpha() < 20); // Rounded corner really clips.
    CTEST_CHECK(pixels.pixelColor(centerX, photoY).lightness() > 45); // Photo, not a blank rectangle.
    CTEST_CHECK(pixels.pixelColor(centerX, scrimY).lightness() < pixels.pixelColor(centerX, photoY).lightness());
}

void CardTests::catalog_renders_all_designs()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(1100, 1800);
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
        QStringLiteral(LVRS_TEST_SOURCE_DIR) + "/../example/VisualCatalog/qml/CardGallery.qml"));
    CTEST_CHECK(object);
    auto *gallery = qobject_cast<QQuickItem *>(object.data());
    CTEST_CHECK(gallery);
    gallery->setWidth(1100);
    gallery->setParentItem(window.contentItem());
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    CTEST_WAIT_DEFAULT(gallery->height() > 0);
    QList<QQuickItem *> cards;
    std::function<void(QQuickItem *)> collect = [&](QQuickItem *item) {
        if (item->property("variantName").isValid())
            cards.append(item);
        for (auto *child : item->childItems())
            collect(child);
    };
    collect(gallery);
    CARD_COMPARE(cards.size(), 15); // Six File cards, three previews, six information cards.
    QSet<int> types;
    for (auto *card : cards) {
        const int type = card->property("type").toInt();
        types.insert(type);
        if (type < 2)
            CARD_WAIT_COMPARE(card->property("previewStatus").toInt(), 1);
        const QPointF bottom = card->mapToItem(gallery, QPointF(card->width(), card->height()));
        CTEST_CHECK(bottom.x() <= gallery->width() && bottom.y() <= gallery->height());
    }
    CARD_COMPARE(types, (QSet<int>{0, 1, 2, 3, 4, 5, 6, 7}));
    processFor(200);
    const auto capture = gallery->grabToImage(QSize(1100, qCeil(gallery->height())));
    CTEST_CHECK(capture);
    int readyCount = 0;
    const auto captureConnection = QObject::connect(capture.data(), &QQuickItemGrabResult::ready,
                                                     [&] { ++readyCount; });
    CTEST_CHECK(captureConnection);
    const auto disconnectCapture = qScopeGuard([&] { QObject::disconnect(captureConnection); });
    CTEST_WAIT_VERIFY(readyCount > 0, 5000);
    const QString directory = qEnvironmentVariable("LVRS_CARD_CAPTURE_DIR");
    if (!directory.isEmpty()) {
        CTEST_CHECK(QDir().mkpath(directory));
        CTEST_CHECK(capture->saveToFile(directory + "/card-gallery.png"));
    }
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    CardTests tests;
    for (const CardVariant &variant : cardVariants()) {
        const auto caseName = QStringLiteral("figma_contract:") + variant.tag();
        runner.run(caseName.toStdString(), [&] { tests.figma_contract(variant); });
    }
    runner.run("mouse_keyboard_and_nested_actions", [&] { tests.mouse_keyboard_and_nested_actions(); });
    runner.run("content_reflows_and_preview_swaps", [&] { tests.content_reflows_and_preview_swaps(); });
    runner.run("preview_errors_and_fill_pixels", [&] { tests.preview_errors_and_fill_pixels(); });
    runner.run("catalog_renders_all_designs", [&] { tests.catalog_renders_all_designs(); });
    return runner.result();
}
#include "tst_card.moc"
