#include <QtTest>
#include <QAccessible>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include "test_utils.h"

class TabsTests : public QObject
{
    Q_OBJECT
private:
    static QVariant evaluate(QQmlEngine &engine, QObject *object, const QString &code)
    {
        QQmlExpression expression(engine.rootContext(), object, code);
        const QVariant result = expression.evaluate();
        if (expression.hasError()) qWarning() << expression.error();
        return result;
    }
    static QQuickItem *tab(QQmlEngine &engine, QObject *bar, int index)
    {
        return qobject_cast<QQuickItem *>(evaluate(engine, bar, QString("itemAt(%1)").arg(index)).value<QObject *>());
    }
private slots:
    void desktop_variants()
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        for (int style = 0; style < 2; ++style) {
            for (int state = 1; state <= 6; ++state) {
                QScopedPointer<QObject> item(TestUtils::createFromQml(engine, QString(
                    "import LVRS as LV\nLV.Tab { text: 'Overview'; tabStyle: %1; displayState: %2; motionEnabled: false }")
                    .arg(style).arg(state).toUtf8()));
                QVERIFY(item);
                QCOMPARE(item->property("implicitHeight").toReal(), 32.0);
                QCOMPARE(item->property("effectiveSelected").toBool(), state == 4);
                QCOMPARE(item->property("effectiveEnabled").toBool(), state != 6);
            }
        }
    }

    void activation_focus_disabled_and_controlled_selection()
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.TabBar {
    width: 480; motionEnabled: false
    model: [{text: 'Overview'}, {text: 'Disabled', enabled: false}, {text: 'Settings', badge: '8'}]
})"));
        QVERIFY(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(480, 180); bar->setParentItem(window.contentItem());
        window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_COMPARE(bar->property("count").toInt(), 3);
        auto *first = tab(engine, bar, 0); auto *last = tab(engine, bar, 2); QVERIFY(first && last);
        QSignalSpy activated(bar, SIGNAL(activated(int))); QVERIFY(activated.isValid());
        first->forceActiveFocus(); QTest::keyClick(&window, Qt::Key_Right);
        QTRY_VERIFY(last->hasActiveFocus());
        QCOMPARE(bar->property("currentIndex").toInt(), 0); // Focus alone is not selection.
        QTest::keyClick(&window, Qt::Key_Space);
        QTRY_COMPARE(bar->property("currentIndex").toInt(), 2);
        QCOMPARE(activated.size(), 1);
        auto *accessible = QAccessible::queryAccessibleInterface(last); QVERIFY(accessible);
        QCOMPARE(accessible->role(), QAccessible::PageTab);
        QVERIFY(accessible->state().selected);
        evaluate(engine, bar, "activate(1)"); QCOMPARE(activated.size(), 1);
        bar->setProperty("autoSelect", false);
        evaluate(engine, bar, "activate(0)"); QCOMPARE(activated.size(), 2);
        QCOMPARE(bar->property("currentIndex").toInt(), 2);
        bar->setProperty("currentIndex", 0);
        QVERIFY(first->property("selected").toBool()); QVERIFY(!last->property("selected").toBool());
        bar->setProperty("widthPolicy", 2); bar->setWidth(180);
        evaluate(engine, bar, "focusItem(2)");
        auto *viewport = bar->findChild<QQuickItem *>("tabViewport"); QVERIFY(viewport);
        QTRY_VERIFY(viewport->property("contentX").toReal() > 0);
        evaluate(engine, bar, "model = []"); QTRY_COMPARE(bar->property("count").toInt(), 0);
        bar->setParentItem(nullptr);
    }

    void mobile_geometry_touch_and_presentations()
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.MobileTabBar {
    motionEnabled: false
    model: [{text: 'Home', iconName: 'home'}, {text: 'Library', iconName: 'nodesfolder'},
        {text: 'Activity', iconName: 'generalsettings'}, {text: 'Profile', iconName: 'loggedInUser'},
        {text: 'Search', iconName: 'inputFieldSearch'}]
})"));
        QVERIFY(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(430, 180); bar->setParentItem(window.contentItem());
        window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_COMPARE(bar->property("count").toInt(), 5);
        for (int platform : {1, 2}) {
            bar->setProperty("platformStyle", platform);
            for (int count : {3, 4, 5}) {
                evaluate(engine, bar, QString("model = [{text:'Home'}, {text:'Library'}, {text:'Activity'}, {text:'Profile'}, {text:'Search'}].slice(0,%1)").arg(count));
                QTRY_COMPARE(bar->property("count").toInt(), count);
                for (int presentation : {0, 1, 2}) {
                    bar->setProperty("presentation", presentation);
                    for (int width : {320, 360, 412}) {
                        bar->setWidth(width);
                        // Row positions settle during Qt Quick's next polish, after width bindings.
                        QTRY_VERIFY(([&] {
                            for (int i = 0; i < count; ++i) {
                                auto *item = tab(engine, bar, i);
                                if (!item || !item->isVisible()) continue;
                                const auto bounds = item->mapRectToItem(bar, item->boundingRect());
                                if (bounds.left() < -0.1 || bounds.right() > width + 0.1) return false;
                            }
                            return true;
                        })());
                        int visible = 0, selected = 0;
                        for (int i = 0; i < count; ++i) {
                            auto *item = tab(engine, bar, i); QVERIFY(item);
                            if (!item->isVisible()) continue;
                            ++visible; selected += item->property("selected").toBool();
                            const auto bounds = item->mapRectToItem(bar, item->boundingRect());
                            QVERIFY2(bounds.left() >= -0.1 && bounds.right() <= width + 0.1,
                                qPrintable(QString("platform=%1 count=%2 presentation=%3 width=%4 item=%5 bounds=%6..%7")
                                    .arg(platform).arg(count).arg(presentation).arg(width).arg(i).arg(bounds.left()).arg(bounds.right())));
                            QVERIFY(item->width() >= 48 && item->height() >= 48);
                        }
                        QCOMPARE(selected, 1);
                        QCOMPARE(visible, platform == 1 && presentation == 2 ? 1 : count);
                    }
                }
            }
        }
        bar->setProperty("presentation", 0); bar->setProperty("platformStyle", 1);
        bar->setProperty("currentIndex", 0);
        auto *destination = tab(engine, bar, 2); QVERIFY(destination);
        QTRY_COMPARE(destination->width(), (bar->width() - 44) / 5);
        QTRY_COMPARE(destination->x(), destination->width() * 2);
        QSignalSpy activated(bar, SIGNAL(activated(int)));
        auto *touch = QTest::createTouchDevice();
        const auto point = destination->mapToScene(QPointF(destination->width()/2, destination->height()/2)).toPoint();
        QTest::touchEvent(&window, touch).press(0, point, &window);
        QTest::touchEvent(&window, touch).release(0, point, &window);
        QTRY_COMPARE(activated.size(), 1);
        QCOMPARE(activated.first().first().toInt(), 2);
        QTRY_COMPARE(bar->property("currentIndex").toInt(), 2);
        const qreal baseHeight = bar->implicitHeight(); bar->setProperty("bottomSafeInset", 34);
        QCOMPARE(bar->implicitHeight(), baseHeight + 34);
        bar->setParentItem(nullptr);
    }

    void gallery_capture()
    {
        const QString output = qEnvironmentVariable("LVRS_TABS_SCREENSHOT_PATH");
        if (output.isEmpty()) QSKIP("Set LVRS_TABS_SCREENSHOT_PATH for a native-renderer capture.");
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
            QStringLiteral(LVRS_TEST_SOURCE_DIR "/../example/VisualCatalog/qml/TabsGallery.qml")));
        QVERIFY(object);
        auto *gallery = qobject_cast<QQuickItem *>(object.data()); QVERIFY(gallery);
        QQuickWindow window;
        window.setColor(QColor("#141414")); window.resize(1000, 1180);
        gallery->setParentItem(window.contentItem()); gallery->setPosition(QPointF(24, 24));
        gallery->setSize(QSizeF(952, 1132)); window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::qWait(400);
        const auto image = window.grabWindow(); QVERIFY(!image.isNull());
        QVERIFY(image.save(output));
        gallery->setParentItem(nullptr);
    }
};
QTEST_MAIN(TabsTests)
#include "tst_tabs.moc"
