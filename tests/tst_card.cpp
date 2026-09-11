#include <QtTest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickItemGrabResult>
#include <QQmlEngine>
#include <QSignalSpy>
#include <functional>
#include "test_utils.h"

class CardTests : public QObject
{
    Q_OBJECT
private slots:
    void figma_contract_data();
    void figma_contract();
    void mouse_keyboard_and_nested_actions();
    void content_reflows_and_preview_swaps();
    void preview_errors_and_fill_pixels();
    void catalog_renders_all_designs();
};

void CardTests::figma_contract_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("size");
    QTest::addColumn<int>("detail");
    QTest::addColumn<int>("displayState");
    QTest::addColumn<QSize>("dimensions");
    QTest::addColumn<bool>("mobile");
    for (bool mobile : {false, true}) {
        for (int type = 0; type < 8; ++type) {
            for (int size = 0; size < (type == 0 ? 3 : 1); ++size) {
                for (int detail = 0; detail < (type == 0 ? 2 : 1); ++detail) {
                    for (int state = 1; state <= (type == 1 ? 1 : 3); ++state) {
                        const QSize fileSizes[] = {{192, 192}, {256, 320}, {480, 280}};
                        const QSize dimensions = type == 0 ? fileSizes[size]
                            : type == 1 ? QSize(480, 320) : QSize(256, 280);
                        const QByteArray name = QString("%1-%2-%3-%4-%5")
                            .arg(type).arg(size).arg(detail).arg(state).arg(mobile).toLatin1();
                        QTest::newRow(name.constData()) << type << size << detail << state << dimensions << mobile;
                    }
                }
            }
        }
    }
}

void CardTests::figma_contract()
{
    QFETCH(int, type); QFETCH(int, size); QFETCH(int, detail);
    QFETCH(int, displayState); QFETCH(QSize, dimensions); QFETCH(bool, mobile);
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
    QVERIFY(object);
    auto *card = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(card);
    QTRY_COMPARE(card->implicitWidth(), dimensions.width());
    QTRY_COMPARE(card->implicitHeight(), dimensions.height());
    QCOMPARE(card->property("effectiveSelected").toBool(), type != 1 && displayState == 3);
    QCOMPARE(card->property("borderWidth").toReal(), type == 1 ? 0.0 : displayState == 3 ? 2.0 : 1.0);
    if (type == 0) {
        auto *caption = card->findChild<QQuickItem *>("card_caption");
        QVERIFY(caption);
        const int briefHeights[] = {37, 39, 39};
        const int detailedHeights[] = {75, 99, 81};
        QTRY_COMPARE(caption->height(), detail ? detailedHeights[size] : briefHeights[size]);
        QCOMPARE(caption->x(), (size == 0 ? 12 : 18) + (displayState == 3 ? 2.0 : 1.0));
        auto *title = card->findChild<QQuickItem *>("card_filename");
        QVERIFY(title);
        QCOMPARE(title->property("font").value<QFont>().pixelSize(), size == 0 ? 13 : 15);
        QCOMPARE(title->property("font").value<QFont>().weight(), QFont::DemiBold);
    } else if (type >= 2) {
        auto *header = card->findChild<QQuickItem *>("card_header");
        QVERIFY(header);
        QTRY_COMPARE(header->height(), 36.0);
        auto *footer = card->findChild<QQuickItem *>("card_footer");
        QVERIFY(footer);
        QTRY_COMPARE(footer->mapToItem(card, QPointF()).y(), displayState == 3 ? 238.0 : 239.0);
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
    QVERIFY(object);
    auto *card = qobject_cast<QQuickItem *>(object.data());
    card->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QSignalSpy clicked(card, SIGNAL(clicked()));
    QSignalSpy menu(card, SIGNAL(menuRequested()));
    QSignalSpy action(card, SIGNAL(actionTriggered()));
    QTest::mouseMove(&window, QPoint(100, 100));
    QTRY_VERIFY(card->property("effectiveHovered").toBool());
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QCOMPARE(clicked.count(), 1);
    QVERIFY(card->property("selected").toBool());
    for (const QString &name : {QString("card_menu"), QString("card_action")}) {
        auto *button = card->findChild<QQuickItem *>(name);
        QVERIFY(button);
        const QPoint point = button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint();
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, point);
        QCOMPARE(clicked.count(), 1);
        QVERIFY(card->property("selected").toBool());
    }
    QCOMPARE(menu.count(), 1);
    QCOMPARE(action.count(), 1);
    card->forceActiveFocus();
    QTest::keyClick(&window, Qt::Key_Space);
    QTRY_COMPARE(clicked.count(), 2);
    QVERIFY(!card->property("selected").toBool());
    card->setEnabled(false);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTest::keyClick(&window, Qt::Key_Space);
    QCOMPARE(clicked.count(), 2);
    QCOMPARE(menu.count(), 1);
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
    QVERIFY(object);
    auto *card = qobject_cast<QQuickItem *>(object.data());
    auto *preview = card->findChild<QQuickItem *>("customPreview");
    QVERIFY(preview);
    QTRY_COMPARE(preview->size(), QSizeF(360, 240));
    auto *caption = card->findChild<QQuickItem *>("card_caption");
    QVERIFY(caption);
    QTRY_COMPARE(caption->size(), QSizeF(320, 81));
    card->setWidth(240);
    QTRY_COMPARE(preview->width(), 240.0);
    QTRY_COMPARE(caption->width(), 200.0);
    for (const QString &name : {QString("card_filename"), QString("card_details"), QString("card_metadata")}) {
        auto *label = card->findChild<QQuickItem *>(name);
        QVERIFY(label);
        const QPointF bottom = label->mapToItem(card, QPointF(label->width(), label->height()));
        QVERIFY(bottom.x() <= card->width() && bottom.y() <= card->height());
    }
    card->setProperty("type", 3); // Project
    card->setHeight(280);
    card->setProperty("progress", 2.0);
    auto *bar = card->findChild<QQuickItem *>("card_progress");
    QVERIFY(bar);
    QTRY_COMPARE(bar->property("progress").toDouble(), 1.0);
    card->setProperty("progress", -1.0);
    QTRY_COMPARE(bar->property("progress").toDouble(), 0.0);
    card->setProperty("showMenu", false);
    card->setProperty("showAction", false);
    QVERIFY(!card->findChild<QQuickItem *>("card_menu")->isVisible());
    QVERIFY(!card->findChild<QQuickItem *>("card_action")->isVisible());
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
    QVERIFY(object);
    auto *card = qobject_cast<QQuickItem *>(object.data());
    card->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_COMPARE(card->property("previewStatus").toInt(), 3); // Image.Error
    const QString fixture = QStringLiteral(LVRS_TEST_SOURCE_DIR) + "/../example/VisualCatalog/assets/cards/architecture.png";
    QVERIFY(QFileInfo::exists(fixture));
    card->setProperty("previewSource", QUrl::fromLocalFile(fixture));
    QTRY_COMPARE(card->property("previewStatus").toInt(), 1); // Image.Ready
    auto *image = card->findChild<QQuickItem *>("card_previewImage");
    QVERIFY(image);
    QCOMPARE(image->size(), card->size());
    QCOMPARE(image->property("fillMode").toInt(), 2); // PreserveAspectCrop
    qInfo() << "Decoded preview" << image->property("sourceSize") << image->implicitWidth() << image->implicitHeight();
    QTest::qWait(100);
    const auto capture = card->grabToImage(QSize(192, 192));
    QVERIFY(capture);
    QSignalSpy ready(capture.data(), &QQuickItemGrabResult::ready);
    QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty(), 5000);
    const QImage pixels = capture->image();
    QVERIFY(!pixels.isNull());
    const QString captureDir = qEnvironmentVariable("LVRS_CARD_CAPTURE_DIR");
    if (!captureDir.isEmpty()) {
        QVERIFY(QDir().mkpath(captureDir));
        QVERIFY(pixels.save(captureDir + "/file-small.png"));
    }
    qInfo() << "Card graphics API" << window.rendererInterface()->graphicsApi()
            << "preview pixel" << pixels.pixelColor(96, 30);
    QVERIFY(pixels.pixelColor(0, 0).alpha() < 20); // Rounded corner really clips.
    QVERIFY(pixels.pixelColor(96, 30).lightness() > 45); // Photo, not a blank rectangle.
    QVERIFY(pixels.pixelColor(96, 180).lightness() < pixels.pixelColor(96, 30).lightness());
}

void CardTests::catalog_renders_all_designs()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QQuickWindow window;
    window.resize(1100, 1800);
    QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
        QStringLiteral(LVRS_TEST_SOURCE_DIR) + "/../example/VisualCatalog/qml/CardGallery.qml"));
    QVERIFY(object);
    auto *gallery = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(gallery);
    gallery->setWidth(1100);
    gallery->setParentItem(window.contentItem());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QTRY_VERIFY(gallery->height() > 0);
    QList<QQuickItem *> cards;
    std::function<void(QQuickItem *)> collect = [&](QQuickItem *item) {
        if (item->property("variantName").isValid())
            cards.append(item);
        for (auto *child : item->childItems())
            collect(child);
    };
    collect(gallery);
    QCOMPARE(cards.size(), 15); // Six File cards, three previews, six information cards.
    QSet<int> types;
    for (auto *card : cards) {
        const int type = card->property("type").toInt();
        types.insert(type);
        if (type < 2)
            QTRY_COMPARE(card->property("previewStatus").toInt(), 1);
        const QPointF bottom = card->mapToItem(gallery, QPointF(card->width(), card->height()));
        QVERIFY(bottom.x() <= gallery->width() && bottom.y() <= gallery->height());
    }
    QCOMPARE(types, (QSet<int>{0, 1, 2, 3, 4, 5, 6, 7}));
    QTest::qWait(200);
    const auto capture = gallery->grabToImage(QSize(1100, qCeil(gallery->height())));
    QVERIFY(capture);
    QSignalSpy ready(capture.data(), &QQuickItemGrabResult::ready);
    QTRY_VERIFY_WITH_TIMEOUT(!ready.isEmpty(), 5000);
    const QString directory = qEnvironmentVariable("LVRS_CARD_CAPTURE_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QVERIFY(capture->saveToFile(directory + "/card-gallery.png"));
    }
}

QTEST_MAIN(CardTests)
#include "tst_card.moc"
