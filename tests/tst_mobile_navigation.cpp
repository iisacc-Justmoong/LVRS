#include "QtEventWait.h"
#include "QtGuiTouch.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTimer>
#include <type_traits>
#include <QAccessible>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include "test_utils.h"

class NavigationSignalRecorder : public QObject
{
    Q_OBJECT
public:
    QList<int> activated;
    int searches = 0;
public slots:
    void recordActivated(int index) { activated.append(index); }
    void recordSearch() { ++searches; }
};

namespace {
template <typename Actual, typename Expected>
bool navigationEqual(const Actual &actual, const Expected &expected)
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
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, key, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &release);
    QCoreApplication::processEvents();
}
}

#define NAV_COMPARE(actual, expected) CTEST_CHECK(navigationEqual((actual), (expected)))
#define NAV_WAIT_COMPARE(actual, expected) CTEST_WAIT_DEFAULT(navigationEqual((actual), (expected)))

class MobileNavigationTests
{
    static QVariant eval(QQmlEngine &engine, QObject *object, const QString &code)
    {
        Q_UNUSED(engine);
        QQmlExpression expression(qmlContext(object), object, code);
        const auto value = expression.evaluate();
        if (expression.hasError()) qWarning() << expression.error();
        return value;
    }
    static QQuickItem *item(QQmlEngine &engine, QObject *bar, int index)
    {
        return qobject_cast<QQuickItem *>(eval(engine, bar,
            QString("itemAt(%1)").arg(index)).value<QObject *>());
    }
    static QObject *create(QQmlEngine &engine, bool motion = false, bool withSearch = true)
    {
        engine.addImportPath(TestUtils::qmlImportBase());
        return TestUtils::createFromQml(engine, QString(R"(
import QtQuick
import LVRS as LV
LV.MobileNavigationBar {
    width: 402; motionEnabled: %1
    %2
    model: [{iconName:'home-1', accessibleName:'Home'},
            {iconName:'nodesfolder', accessibleName:'Library'},
            {iconName:'generalsettings', accessibleName:'Settings'}]
})").arg(motion ? "true" : "false", withSearch ? "search: ({})" : "").toUtf8());
    }
public:
    void selected_icons_use_darker_accent_token()
    {
        QQmlEngine engine;
        engine.addImportPath(TestUtils::qmlImportBase());
        QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
Item {
    property color iconAccent: LV.Theme.primaryHover
    property color textAccent: LV.Theme.primary
    LV.MobileNavigationTab { objectName: "navigation"; iconName: "home-1"; selected: true }
    LV.MobileTab { objectName: "mobile"; iconName: "home-1"; selected: true; platformStyle: LV.MobileTab.IOS }
})"));
        CTEST_CHECK(root);
        auto *navigation = root->findChild<QObject *>("navigation");
        auto *mobile = root->findChild<QObject *>("mobile");
        CTEST_CHECK(navigation && mobile);
        NAV_COMPARE(navigation->property("foreground"), root->property("iconAccent"));
        NAV_COMPARE(mobile->property("iconForeground"), root->property("iconAccent"));
        NAV_COMPARE(mobile->property("foreground"), root->property("textAccent"));
        CTEST_CHECK(root->property("iconAccent") != root->property("textAccent"));
    }
    void source_icon_colors_and_artwork_geometry()
    {
        QQmlEngine engine;
        QScopedPointer<QObject> object(create(engine)); CTEST_CHECK(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data());
        CTEST_CHECK(item(engine, bar, 0)->property("preserveIconColors").isValid());
        CTEST_CHECK(!item(engine, bar, 0)->property("preserveIconColors").toBool());
        eval(engine, bar, "model = [{iconName:'collection', preserveIconColors:true, iconArtworkSize:Qt.size(17.5,20.5545)}]");
        auto *tab = item(engine, bar, 0); CTEST_CHECK(tab);
        CTEST_CHECK(tab->property("preserveIconColors").toBool());
        auto *image = tab->findChild<QQuickItem *>("mobileNavigationIcon"); CTEST_CHECK(image);
        NAV_COMPARE(image->width(), 17.5);
        NAV_COMPARE(image->height(), 20.5545);
        NAV_COMPARE(image->opacity(), 1.0);
        eval(engine, bar, "search = ({iconName:'inputFieldSearch', preserveIconColors:true})");
        auto *search = bar->findChild<QQuickItem *>("mobileNavigationSearch"); CTEST_CHECK(search);
        CTEST_CHECK(search->property("preserveIconColors").toBool());
        NAV_COMPARE(search->property("iconArtworkSize").toSizeF(), QSizeF(24, 24));
        bar->setProperty("enabled", false);
        CTEST_CHECK(image->opacity() < 1.0);
    }
    void gallery_capture()
    {
        const QString output = qEnvironmentVariable("LVRS_MOBILE_NAVIGATION_CAPTURE_PATH");
        if (output.isEmpty()) lvrs_ctest::skip("Set LVRS_MOBILE_NAVIGATION_CAPTURE_PATH for a rendered capture.");
        QQmlEngine engine; engine.addImportPath(TestUtils::qmlImportBase());
        QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
            QStringLiteral(LVRS_TEST_SOURCE_DIR "/../example/VisualCatalog/qml/MobileNavigationGallery.qml")));
        CTEST_CHECK(object); auto *gallery = qobject_cast<QQuickItem *>(object.data()); CTEST_CHECK(gallery);
        QQuickWindow window; window.setColor(QColor("#0c0c0d")); window.resize(960, 1020);
        gallery->setParentItem(window.contentItem()); gallery->setPosition(QPointF(24,24)); gallery->setWidth(912);
        window.show(); CTEST_WAIT_DEFAULT(window.isExposed());
        sendMouse(&window, QEvent::MouseMove, QPoint(959,1019), Qt::NoButton, Qt::NoButton); processFor(300);
        const auto capture=window.grabWindow(); CTEST_CHECK(!capture.isNull()); CTEST_CHECK(capture.save(output));
        if (window.rendererInterface()->graphicsApi() != QSGRendererInterface::Software) {
            // Figma binds selected icons to the darker Blue Hover token.
            int primaryPixels = 0;
            for (int y=0;y<capture.height();++y) for (int x=0;x<capture.width();++x) {
                const QColor pixel=capture.pixelColor(x,y);
                if (qAbs(pixel.red()-35)<3 && qAbs(pixel.green()-115)<3 && qAbs(pixel.blue()-205)<3) ++primaryPixels;
            }
            CTEST_CHECK_MESSAGE(primaryPixels > 20, "Selected icons must render at the LVRS icon accent token.");
        }
        gallery->setParentItem(nullptr);
    }
    void left_alignment_search_radius_and_labels()
    {
        QQmlEngine engine;
        QScopedPointer<QObject> object(create(engine)); CTEST_CHECK(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(430, 200); bar->setParentItem(window.contentItem());
        window.show(); CTEST_WAIT_DEFAULT(window.isExposed());
        auto *group = bar->findChild<QQuickItem *>("mobileNavigationGroup");
        auto *search = bar->findChild<QQuickItem *>("mobileNavigationSearch");
        CTEST_CHECK(group && search);
        for (int platform : {1, 2}) {
            bar->setProperty("platformStyle", platform);
            NAV_COMPARE(bar->property("resolvedCornerRadius").toReal(), platform == 1 ? 28.0 : 12.0);
            for (int count = 1; count <= 5; ++count) {
                eval(engine, bar, QString("model = Array.from({length:%1}, (_, i) => ({iconName:'home-1', accessibleName:'Page '+i}))").arg(count));
                NAV_WAIT_COMPARE(bar->property("count").toInt(), count);
                NAV_WAIT_COMPARE(group->width(), qreal(count * 56 + 8));
                NAV_COMPARE(group->mapToItem(bar, QPointF()).x(), 16.0);
                NAV_COMPARE(search->mapToItem(bar, QPointF()).x() + search->width(), 386.0);
                CTEST_CHECK(!item(engine, bar, 0)->property("showLabel").toBool());
            }
        }
        for (qreal radius : {0.0, 7.5, 20.0, 80.0}) {
            bar->setProperty("deviceCornerRadius", radius);
            NAV_COMPARE(bar->property("resolvedCornerRadius").toReal(), qMin(radius, 28.0));
        }
        eval(engine, bar, "model = [{iconName:'home-1', text:'Home', accessibleName:'Go home'}]");
        CTEST_WAIT_DEFAULT(item(engine, bar, 0)->property("showLabel").toBool());
        auto *accessible = QAccessible::queryAccessibleInterface(item(engine, bar, 0));
        CTEST_CHECK(accessible); NAV_COMPARE(accessible->text(QAccessible::Name), QString("Go home"));
        const auto height = bar->implicitHeight(); bar->setProperty("bottomSafeInset", 34);
        NAV_COMPARE(bar->implicitHeight(), height + 34);
        eval(engine, bar, "LayoutMirroring.enabled = true");
        NAV_COMPARE(group->mapToItem(bar, QPointF()).x(), 16.0);
        NAV_COMPARE(search->mapToItem(bar, QPointF()).x() + search->width(), 386.0);
        bar->setParentItem(nullptr);
    }

    void input_disabled_controlled_selection_and_overflow()
    {
        QQmlEngine engine; QScopedPointer<QObject> object(create(engine)); CTEST_CHECK(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(430, 200); bar->setParentItem(window.contentItem());
        window.show(); CTEST_WAIT_DEFAULT(window.isExposed());
        NavigationSignalRecorder recorder;
        const auto activatedConnection = QObject::connect(bar, SIGNAL(activated(int)),
                                                           &recorder, SLOT(recordActivated(int)));
        const auto searchConnection = QObject::connect(bar, SIGNAL(searchRequested()),
                                                        &recorder, SLOT(recordSearch()));
        CTEST_CHECK(activatedConnection);
        CTEST_CHECK(searchConnection);
        auto *search = bar->findChild<QQuickItem *>("mobileNavigationSearch"); CTEST_CHECK(search);
        auto *accessible = QAccessible::queryAccessibleInterface(search); CTEST_CHECK(accessible);
        NAV_COMPARE(accessible->role(), QAccessible::Button);
        NAV_COMPARE(accessible->text(QAccessible::Name), QString("Search"));
        mouseClick(&window, search->mapToScene(QPointF(28,28)).toPoint());
        NAV_WAIT_COMPARE(recorder.searches, 1); NAV_COMPARE(recorder.activated.size(), 0);
        NAV_COMPARE(bar->property("currentIndex").toInt(), 0);
        eval(engine, bar, "model = [{text:'Home', iconName:'home-1'}, {text:'Off', iconName:'locked', enabled:false}, {text:'Library', iconName:'nodesfolder'}]");
        NAV_WAIT_COMPARE(bar->property("count").toInt(), 3);
        item(engine, bar, 0)->forceActiveFocus(); keyClick(&window, Qt::Key_Right);
        CTEST_WAIT_DEFAULT(item(engine, bar, 2)->hasActiveFocus());
        keyClick(&window, Qt::Key_Space);
        NAV_WAIT_COMPARE(bar->property("currentIndex").toInt(), 2);
        NAV_COMPARE(recorder.activated.size(), 1);
        eval(engine, bar, "activate(1)"); NAV_COMPARE(recorder.activated.size(), 1);
        bar->setProperty("autoSelect", false); eval(engine, bar, "activate(0)");
        NAV_COMPARE(recorder.activated.size(), 2); NAV_COMPARE(bar->property("currentIndex").toInt(), 2);
        bar->setProperty("autoSelect", true);
        // Replacing the model recreates delegates; wait for Row's layout pass
        // before mapping a real touch position into the scene.
        NAV_WAIT_COMPARE(item(engine, bar, 2)->x(), 112.0);
        auto *touch = lvrs_ctest::createTouchDevice();
        const auto point = item(engine,bar,0)->mapToScene(QPointF(28,24)).toPoint();
        lvrs_ctest::TouchSequence(&window,touch).press(0,point,&window);
        CTEST_WAIT_DEFAULT(item(engine, bar, 0)->property("down").toBool());
        lvrs_ctest::TouchSequence(&window,touch).release(0,point,&window);
        NAV_WAIT_COMPARE(bar->property("currentIndex").toInt(),0); NAV_COMPARE(recorder.activated.size(),3);
        eval(engine, bar, "search = {enabled:false}");
        mouseClick(&window, search->mapToScene(QPointF(28,28)).toPoint());
        NAV_COMPARE(recorder.searches, 1);
        eval(engine, bar, "model = Array.from({length:5}, (_, i) => ({iconName:'home-1', accessibleName:'Page '+i}))");
        bar->setWidth(320); NAV_WAIT_COMPARE(bar->property("count").toInt(), 5);
        for (int i=0;i<5;++i) CTEST_CHECK(item(engine, bar, i)->width() >= 48);
        eval(engine, bar, "focusItem(4)");
        auto *viewport=bar->findChild<QQuickItem *>("mobileNavigationViewport"); CTEST_CHECK(viewport);
        CTEST_WAIT_DEFAULT(viewport->property("contentX").toReal()>0);
        const auto rect=item(engine,bar,4)->mapRectToItem(viewport,item(engine,bar,4)->boundingRect());
        CTEST_CHECK(rect.left()>=-0.1 && rect.right()<=viewport->width()+0.1);
        eval(engine, bar, "model=[]"); NAV_WAIT_COMPARE(bar->property("count").toInt(), 0);
        CTEST_CHECK(search->isVisible());
        bar->setParentItem(nullptr);
    }

    void optional_search_reclaims_space_and_skips_focus()
    {
        QQmlEngine engine;
        QScopedPointer<QObject> object(create(engine, false, false)); CTEST_CHECK(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data()); CTEST_CHECK(bar);
        bar->setWidth(320);
        eval(engine, bar, "model = Array.from({length:5}, (_, i) => ({iconName:'home-1', accessibleName:'Page '+i}))");
        QQuickWindow window; window.resize(430, 200); bar->setParentItem(window.contentItem());
        window.show(); CTEST_WAIT_DEFAULT(window.isExposed());
        auto *group = bar->findChild<QQuickItem *>("mobileNavigationGroup");
        auto *search = bar->findChild<QQuickItem *>("mobileNavigationSearch");
        auto *viewport = bar->findChild<QQuickItem *>("mobileNavigationViewport");
        auto *indicator = bar->findChild<QQuickItem *>("mobileNavigationIndicator");
        CTEST_CHECK(group && search && viewport && indicator);
        NavigationSignalRecorder recorder;
        const auto searchConnection = QObject::connect(bar, SIGNAL(searchRequested()),
                                                        &recorder, SLOT(recordSearch()));
        CTEST_CHECK(searchConnection);

        // Omitting the argument hides Search and returns all available width.
        CTEST_CHECK(!search->isVisible()); CTEST_CHECK(!search->isEnabled());
        NAV_WAIT_COMPARE(group->width(), 288.0);
        NAV_COMPARE(group->mapToItem(bar, QPointF()).x(), 16.0);
        NAV_COMPARE(item(engine, bar, 0)->width(), 56.0);
        NAV_WAIT_COMPARE(viewport->property("contentWidth").toReal(), viewport->width());
        CTEST_CHECK(!eval(engine, bar, "focusItem(count)").toBool());
        CTEST_CHECK(eval(engine, bar, "focusItem(4)").toBool());
        keyClick(&window, Qt::Key_Right);
        CTEST_WAIT_DEFAULT(item(engine, bar, 0)->hasActiveFocus());
        keyClick(&window, Qt::Key_End);
        CTEST_WAIT_DEFAULT(item(engine, bar, 4)->hasActiveFocus());
        keyClick(&window, Qt::Key_Space);
        NAV_WAIT_COMPARE(bar->property("currentIndex").toInt(), 4);
        NAV_WAIT_COMPARE(indicator->x(), 224.0);

        // An empty configuration opts in without affecting tab selection.
        eval(engine, bar, "search = ({})");
        CTEST_WAIT_DEFAULT(search->isVisible()); CTEST_CHECK(search->isEnabled());
        NAV_WAIT_COMPARE(group->width(), 220.0);
        NAV_COMPARE(search->mapToItem(bar, QPointF()).x() + search->width(), 304.0);
        NAV_WAIT_COMPARE(indicator->x(), 192.0);
        CTEST_CHECK(eval(engine, bar, "focusItem(count)").toBool());
        CTEST_WAIT_DEFAULT(search->hasActiveFocus());
        keyClick(&window, Qt::Key_Return);
        NAV_WAIT_COMPARE(recorder.searches, 1);
        NAV_COMPARE(bar->property("currentIndex").toInt(), 4);

        for (const auto &value : {"null", "undefined", "({text:'Search', visible:false})"}) {
            eval(engine, bar, "search = ({})");
            CTEST_CHECK(eval(engine, bar, "focusItem(count)").toBool());
            eval(engine, bar, QString("search = %1").arg(value));
            CTEST_WAIT_DEFAULT(!search->isVisible()); CTEST_CHECK(!search->isEnabled());
            CTEST_WAIT_DEFAULT(!search->hasActiveFocus());
            NAV_WAIT_COMPARE(group->width(), 288.0);
            NAV_COMPARE(group->mapToItem(bar, QPointF()).x(), 16.0);
            NAV_COMPARE(bar->property("currentIndex").toInt(), 4);
            NAV_WAIT_COMPARE(indicator->x(), 224.0);
            NAV_WAIT_COMPARE(viewport->property("contentX").toReal(), 0.0);
            CTEST_CHECK(!eval(engine, bar, "focusItem(count)").toBool());
            CTEST_CHECK(QMetaObject::invokeMethod(search, "clicked"));
            NAV_COMPARE(recorder.searches, 1);
        }
        eval(engine, bar, "model = []");
        CTEST_WAIT_DEFAULT(!group->isVisible()); CTEST_CHECK(!search->isVisible());
        CTEST_CHECK(!eval(engine, bar, "focusItem(0)").toBool());
        eval(engine, bar, "moveFocus(0, 1)");
        bar->setParentItem(nullptr);
    }

    void shared_indicator_stretches_retargets_and_reduces_motion()
    {
        QQmlEngine engine; QScopedPointer<QObject> object(create(engine, true)); CTEST_CHECK(object);
        auto *bar=qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(430,200); bar->setParentItem(window.contentItem());
        window.show(); CTEST_WAIT_DEFAULT(window.isExposed());
        auto *indicator=bar->findChild<QQuickItem *>("mobileNavigationIndicator"); CTEST_CHECK(indicator);
        NAV_WAIT_COMPARE(indicator->width(), 56.0);
        processFor(50); bar->setProperty("currentIndex",1);
        CTEST_WAIT_VERIFY(indicator->width()>60, 250);
        CTEST_CHECK(indicator->height()<48);
        bar->setProperty("currentIndex",2);
        CTEST_WAIT_VERIFY(qAbs(indicator->x()-112)<0.1 && qAbs(indicator->width()-56)<0.1, 1000);
        bar->setProperty("currentIndex",0); processFor(70);
        eval(engine,bar,"LV.Motion.reducedMotion=true");
        CTEST_CHECK(eval(engine,bar,"LV.Motion.reducedMotion").toBool());
        NAV_WAIT_COMPARE(indicator->x(),0.0); NAV_COMPARE(indicator->width(),56.0);
        bar->setProperty("currentIndex",2); NAV_WAIT_COMPARE(indicator->x(),112.0);
        eval(engine,bar,"LV.Motion.reducedMotion=false");
        bar->setProperty("currentIndex",0); processFor(50);
        bar->setProperty("motionEnabled",false);
        NAV_WAIT_COMPARE(indicator->x(),0.0);
        eval(engine,bar,"model=[]"); CTEST_WAIT_DEFAULT(!indicator->isVisible());
        bar->setParentItem(nullptr);
    }
};
int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc > 1 ? argv[1] : "");
    MobileNavigationTests tests;
    runner.run("selected_icons_use_darker_accent_token", [&] { tests.selected_icons_use_darker_accent_token(); });
    runner.run("source_icon_colors_and_artwork_geometry", [&] { tests.source_icon_colors_and_artwork_geometry(); });
    runner.run("gallery_capture", [&] { tests.gallery_capture(); });
    runner.run("left_alignment_search_radius_and_labels", [&] { tests.left_alignment_search_radius_and_labels(); });
    runner.run("input_disabled_controlled_selection_and_overflow", [&] { tests.input_disabled_controlled_selection_and_overflow(); });
    runner.run("optional_search_reclaims_space_and_skips_focus", [&] { tests.optional_search_reclaims_space_and_skips_focus(); });
    runner.run("shared_indicator_stretches_retargets_and_reduces_motion", [&] { tests.shared_indicator_stretches_retargets_and_reduces_motion(); });
    return runner.result();
}
#include "tst_mobile_navigation.moc"
