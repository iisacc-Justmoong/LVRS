#include "QtEventWait.h"
#include "QtGuiTouch.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QTimer>
#include <type_traits>
#include <QAccessible>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include "test_utils.h"

class TabSignalRecorder : public QObject
{
    Q_OBJECT
public:
    QList<int> values;
public slots:
    void recordActivated(int index) { values.append(index); }
};

namespace {
template <typename Actual, typename Expected>
bool tabEqual(const Actual &actual, const Expected &expected)
{
    if constexpr (std::is_arithmetic_v<Actual> && std::is_arithmetic_v<Expected>
                  && (std::is_floating_point_v<Actual> || std::is_floating_point_v<Expected>))
        return qFuzzyCompare(qreal(actual), qreal(expected));
    else
        return actual == expected;
}

void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(milliseconds);
    loop.exec();
}

void keyClick(QQuickWindow *window, Qt::Key key)
{
    QKeyEvent press(QEvent::KeyPress, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}
}

#define TAB_COMPARE(actual, expected) CTEST_CHECK(tabEqual((actual), (expected)))
#define TAB_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(tabEqual((actual), (expected)))

class TabsTests
{
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
public:
    void icon_is_required(const QString &type)
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        QQmlComponent component(&engine);
        component.setData(QString("import LVRS as LV\nLV.%1 { text: 'Overview' }").arg(type).toUtf8(), QUrl());
        QScopedPointer<QObject> item(component.create());
        CTEST_CHECK_MESSAGE(!item, "A Tab without an explicit iconName must fail creation.");
        CTEST_CHECK_MESSAGE(component.errorString().contains("Required property iconName was not initialized"),
            qPrintable(component.errorString()));
    }

    void icon_is_always_left_of_label()
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        QScopedPointer<QObject> object(TestUtils::createFromQml(engine,
            "import LVRS as LV\nLV.Tab { width: 180; text: 'Overview'; iconName: 'nodesfolder'; badge: '8'; motionEnabled: false }"));
        CTEST_CHECK(object);
        auto *item = qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(240, 100); item->setParentItem(window.contentItem());
        window.show(); CTEST_WAIT_DEFAULT(window.isExposed());
        auto *icon = item->findChild<QQuickItem *>("tabIcon");
        auto *label = item->findChild<QQuickItem *>("tabLabel");
        CTEST_CHECK(icon && label);
        CTEST_WAIT_DEFAULT(icon->isVisible());
        TAB_WAIT_COMPARE(icon->width(), 18.0);
        TAB_WAIT_COMPARE(icon->height(), 18.0);
        TAB_WAIT_COMPARE(label->x() - icon->x() - icon->width(), 8.0);
        TAB_COMPARE(item->metaObject()->indexOfProperty("showIcon"), -1);
        TAB_WAIT_COMPARE(icon->property("status").toInt(), 1); // Image.Ready
        item->setProperty("displayState", 6);
        CTEST_CHECK(icon->isVisible());
        TAB_COMPARE(icon->opacity(), 0.3);
        item->setParentItem(nullptr);
    }

    void empty_icon_cannot_activate()
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        QScopedPointer<QObject> object(TestUtils::createFromQml(engine,
            "import LVRS as LV\nLV.Tab { text: 'Overview'; iconName: '  '; motionEnabled: false }"));
        CTEST_CHECK(object);
        CTEST_CHECK(!object->property("effectiveEnabled").toBool());
    }

    void desktop_variants()
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        for (int style = 0; style < 2; ++style) {
            for (int state = 1; state <= 6; ++state) {
                QScopedPointer<QObject> item(TestUtils::createFromQml(engine, QString(
                    "import LVRS as LV\nLV.Tab { text: 'Overview'; iconName: 'nodesfolder'; tabStyle: %1; displayState: %2; motionEnabled: false }")
                    .arg(style).arg(state).toUtf8()));
                CTEST_CHECK(item);
                TAB_COMPARE(item->property("implicitHeight").toReal(), 32.0);
                TAB_COMPARE(item->property("effectiveSelected").toBool(), state == 4);
                TAB_COMPARE(item->property("effectiveEnabled").toBool(), state != 6);
            }
        }
    }

    void missing_model_icon_is_disabled()
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        for (const auto &type : {"TabBar", "MobileTabBar", "MobileNavigationBar"}) {
            QScopedPointer<QObject> object(TestUtils::createFromQml(engine, QString(
                "import LVRS as LV\nLV.%1 { model: [{text:'Missing'}, {text:'Blank', iconName:'  '}, {text:'Ready', iconName:'nodesfolder'}]; motionEnabled: false }")
                .arg(QString::fromLatin1(type)).toUtf8()));
            CTEST_CHECK_MESSAGE(object, type);
            TAB_WAIT_COMPARE(object->property("count").toInt(), 3);
            CTEST_CHECK(!tab(engine, object.data(), 0)->isEnabled());
            CTEST_CHECK(!tab(engine, object.data(), 1)->isEnabled());
            CTEST_CHECK(tab(engine, object.data(), 2)->isEnabled());
            TAB_WAIT_COMPARE(object->property("currentIndex").toInt(), 2);
            TabSignalRecorder activated;
            const auto activatedConnection = QObject::connect(object.data(), SIGNAL(activated(int)),
                                                               &activated, SLOT(recordActivated(int)));
            CTEST_CHECK(activatedConnection);
            evaluate(engine, object.data(), "activate(0); activate(1)");
            TAB_COMPARE(activated.values.size(), 0);
            evaluate(engine, object.data(), "activate(2)");
            TAB_COMPARE(activated.values.size(), 1);
            processFor(1); // Drain deferred scroll callbacks before destroying the bar.
        }
    }

    void desktop_icon_capture()
    {
        const auto output = qEnvironmentVariable("LVRS_TAB_ICON_SCREENSHOT_PATH");
        if (output.isEmpty()) lvrs_ctest::skip("Set LVRS_TAB_ICON_SCREENSHOT_PATH for a desktop Tab capture.");
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
Rectangle {
    width: 1000; height: 280; color: '#141414'
    Column {
        x: 24; y: 20; width: 952; spacing: 16
        Repeater {
            model: [LV.Tab.Underline, LV.Tab.Surface]
            Row {
                id: row
                required property int modelData
                spacing: 8
                Repeater {
                    model: ['Default', 'Hover', 'Pressed', 'Selected', 'Focus', 'Disabled']
                    LV.Tab {
                        required property int index
                        required property string modelData
                        width: 152; text: modelData; iconName: 'nodesfolder'
                        tabStyle: row.modelData; displayState: index + 1; motionEnabled: false
                    }
                }
            }
        }
        LV.TabBar {
            width: 952; motionEnabled: false
            model: [{text:'Overview',iconName:'nodesfolder'}, {text:'Activity',iconName:'toolwindownotifications',badge:'8'},
                {text:'Members',iconName:'loggedInUser'}, {text:'Settings',iconName:'generalsettings'}]
        }
    }
})"));
        CTEST_CHECK(object);
        auto *content = qobject_cast<QQuickItem *>(object.data()); CTEST_CHECK(content);
        QQuickWindow window; window.resize(1000, 280);
        content->setParentItem(window.contentItem()); window.show();
        CTEST_WAIT_DEFAULT(window.isExposed());
        processFor(200);
        const auto capture = window.grabWindow(); CTEST_CHECK(!capture.isNull());
        CTEST_CHECK(capture.save(output));
        content->setParentItem(nullptr);
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
    model: [{text: 'Overview', iconName: 'nodesfolder'}, {text: 'Disabled', iconName: 'locked', enabled: false},
        {text: 'Settings', iconName: 'generalsettings', badge: '8'}]
})"));
        CTEST_CHECK(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(480, 180); bar->setParentItem(window.contentItem());
        window.show(); CTEST_WAIT_DEFAULT(window.isExposed());
        TAB_WAIT_COMPARE(bar->property("count").toInt(), 3);
        auto *first = tab(engine, bar, 0); auto *last = tab(engine, bar, 2); CTEST_CHECK(first && last);
        TAB_COMPARE(first->property("iconName").toString(), QStringLiteral("nodesfolder"));
        TAB_COMPARE(last->property("iconName").toString(), QStringLiteral("generalsettings"));
        TabSignalRecorder activated;
        const auto activatedConnection = QObject::connect(bar, SIGNAL(activated(int)),
                                                           &activated, SLOT(recordActivated(int)));
        CTEST_CHECK(activatedConnection);
        first->forceActiveFocus(); keyClick(&window, Qt::Key_Right);
        CTEST_WAIT_DEFAULT(last->hasActiveFocus());
        TAB_COMPARE(bar->property("currentIndex").toInt(), 0); // Focus alone is not selection.
        keyClick(&window, Qt::Key_Space);
        TAB_WAIT_COMPARE(bar->property("currentIndex").toInt(), 2);
        TAB_COMPARE(activated.values.size(), 1);
        auto *accessible = QAccessible::queryAccessibleInterface(last); CTEST_CHECK(accessible);
        TAB_COMPARE(accessible->role(), QAccessible::PageTab);
        CTEST_CHECK(accessible->state().selected);
        evaluate(engine, bar, "activate(1)"); TAB_COMPARE(activated.values.size(), 1);
        bar->setProperty("autoSelect", false);
        evaluate(engine, bar, "activate(0)"); TAB_COMPARE(activated.values.size(), 2);
        TAB_COMPARE(bar->property("currentIndex").toInt(), 2);
        bar->setProperty("currentIndex", 0);
        CTEST_CHECK(first->property("selected").toBool()); CTEST_CHECK(!last->property("selected").toBool());
        bar->setProperty("widthPolicy", 2); bar->setWidth(180);
        evaluate(engine, bar, "focusItem(2)");
        auto *viewport = bar->findChild<QQuickItem *>("tabViewport"); CTEST_CHECK(viewport);
        CTEST_WAIT_DEFAULT(viewport->property("contentX").toReal() > 0);
        evaluate(engine, bar, "model = []"); TAB_WAIT_COMPARE(bar->property("count").toInt(), 0);
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
        CTEST_CHECK(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(430, 180); bar->setParentItem(window.contentItem());
        window.show(); CTEST_WAIT_DEFAULT(window.isExposed());
        TAB_WAIT_COMPARE(bar->property("count").toInt(), 5);
        for (int platform : {1, 2}) {
            bar->setProperty("platformStyle", platform);
            for (int count : {3, 4, 5}) {
                evaluate(engine, bar, QString("model = [{text:'Home',iconName:'home'}, {text:'Library',iconName:'nodesfolder'}, {text:'Activity',iconName:'generalsettings'}, {text:'Profile',iconName:'loggedInUser'}, {text:'Search',iconName:'inputFieldSearch'}].slice(0,%1)").arg(count));
                TAB_WAIT_COMPARE(bar->property("count").toInt(), count);
                for (int presentation : {0, 1, 2}) {
                    bar->setProperty("presentation", presentation);
                    for (int width : {320, 360, 412}) {
                        bar->setWidth(width);
                        // Row positions settle during Qt Quick's next polish, after width bindings.
                        CTEST_WAIT_DEFAULT(([&] {
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
                            auto *item = tab(engine, bar, i); CTEST_CHECK(item);
                            if (!item->isVisible()) continue;
                            ++visible; selected += item->property("selected").toBool();
                            const auto bounds = item->mapRectToItem(bar, item->boundingRect());
                            CTEST_CHECK_MESSAGE(bounds.left() >= -0.1 && bounds.right() <= width + 0.1,
                                qPrintable(QString("platform=%1 count=%2 presentation=%3 width=%4 item=%5 bounds=%6..%7")
                                    .arg(platform).arg(count).arg(presentation).arg(width).arg(i).arg(bounds.left()).arg(bounds.right())));
                            CTEST_CHECK(item->width() >= 48 && item->height() >= 48);
                        }
                        TAB_COMPARE(selected, 1);
                        TAB_COMPARE(visible, platform == 1 && presentation == 2 ? 1 : count);
                    }
                }
            }
        }
        bar->setProperty("presentation", 0); bar->setProperty("platformStyle", 1);
        bar->setProperty("currentIndex", 0);
        auto *destination = tab(engine, bar, 2); CTEST_CHECK(destination);
        TAB_WAIT_COMPARE(destination->width(), (bar->width() - 44) / 5);
        TAB_WAIT_COMPARE(destination->x(), destination->width() * 2);
        TabSignalRecorder activated;
        const auto activatedConnection = QObject::connect(bar, SIGNAL(activated(int)),
                                                           &activated, SLOT(recordActivated(int)));
        CTEST_CHECK(activatedConnection);
        auto *touch = lvrs_ctest::createTouchDevice();
        const auto point = destination->mapToScene(QPointF(destination->width()/2, destination->height()/2)).toPoint();
        lvrs_ctest::TouchSequence(&window, touch).press(0, point, &window);
        lvrs_ctest::TouchSequence(&window, touch).release(0, point, &window);
        TAB_WAIT_COMPARE(activated.values.size(), 1);
        TAB_COMPARE(activated.values.first(), 2);
        TAB_WAIT_COMPARE(bar->property("currentIndex").toInt(), 2);
        const qreal baseHeight = bar->implicitHeight(); bar->setProperty("bottomSafeInset", 34);
        TAB_COMPARE(bar->implicitHeight(), baseHeight + 34);
        bar->setParentItem(nullptr);
    }

    void gallery_capture()
    {
        const QString output = qEnvironmentVariable("LVRS_TABS_SCREENSHOT_PATH");
        if (output.isEmpty()) lvrs_ctest::skip("Set LVRS_TABS_SCREENSHOT_PATH for a native-renderer capture.");
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
            QStringLiteral(LVRS_TEST_SOURCE_DIR "/../example/VisualCatalog/qml/TabsGallery.qml")));
        CTEST_CHECK(object);
        auto *gallery = qobject_cast<QQuickItem *>(object.data()); CTEST_CHECK(gallery);
        QQuickWindow window;
        window.setColor(QColor("#141414")); window.resize(1000, 1180);
        gallery->setParentItem(window.contentItem()); gallery->setPosition(QPointF(24, 24));
        gallery->setSize(QSizeF(952, 1132)); window.show();
        CTEST_WAIT_DEFAULT(window.isExposed());
        processFor(400);
        const auto image = window.grabWindow(); CTEST_CHECK(!image.isNull());
        CTEST_CHECK(image.save(output));
        gallery->setParentItem(nullptr);
    }
};
int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    TabsTests tests;
    runner.run("icon_is_required:Tab", [&] { tests.icon_is_required(QStringLiteral("Tab")); });
    runner.run("icon_is_required:MobileTab", [&] { tests.icon_is_required(QStringLiteral("MobileTab")); });
    runner.run("icon_is_required:MobileNavigationTab", [&] { tests.icon_is_required(QStringLiteral("MobileNavigationTab")); });
    runner.run("icon_is_always_left_of_label", [&] { tests.icon_is_always_left_of_label(); });
    runner.run("empty_icon_cannot_activate", [&] { tests.empty_icon_cannot_activate(); });
    runner.run("desktop_variants", [&] { tests.desktop_variants(); });
    runner.run("missing_model_icon_is_disabled", [&] { tests.missing_model_icon_is_disabled(); });
    runner.run("desktop_icon_capture", [&] { tests.desktop_icon_capture(); });
    runner.run("activation_focus_disabled_and_controlled_selection", [&] { tests.activation_focus_disabled_and_controlled_selection(); });
    runner.run("mobile_geometry_touch_and_presentations", [&] { tests.mobile_geometry_touch_and_presentations(); });
    runner.run("gallery_capture", [&] { tests.gallery_capture(); });
    return runner.result();
}
#include "tst_tabs.moc"
