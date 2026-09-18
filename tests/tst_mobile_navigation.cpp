#include <QtTest>
#include <QAccessible>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include "test_utils.h"

class MobileNavigationTests : public QObject
{
    Q_OBJECT
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
private slots:
    void gallery_capture()
    {
        const QString output = qEnvironmentVariable("LVRS_MOBILE_NAVIGATION_CAPTURE_PATH");
        if (output.isEmpty()) QSKIP("Set LVRS_MOBILE_NAVIGATION_CAPTURE_PATH for a rendered capture.");
        QQmlEngine engine; engine.addImportPath(TestUtils::qmlImportBase());
        QScopedPointer<QObject> object(TestUtils::loadQmlFile(engine,
            QStringLiteral(LVRS_TEST_SOURCE_DIR "/../example/VisualCatalog/qml/MobileNavigationGallery.qml")));
        QVERIFY(object); auto *gallery = qobject_cast<QQuickItem *>(object.data()); QVERIFY(gallery);
        QQuickWindow window; window.setColor(QColor("#0c0c0d")); window.resize(960, 1020);
        gallery->setParentItem(window.contentItem()); gallery->setPosition(QPointF(24,24)); gallery->setWidth(912);
        window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTest::mouseMove(&window, QPoint(959,1019)); QTest::qWait(300);
        const auto capture=window.grabWindow(); QVERIFY(!capture.isNull()); QVERIFY(capture.save(output));
        if (window.rendererInterface()->graphicsApi() != QSGRendererInterface::Software) {
            // Figma applies a solid primary tint, not luminance-preserving colorization.
            int primaryPixels = 0;
            for (int y=0;y<capture.height();++y) for (int x=0;x<capture.width();++x) {
                const QColor pixel=capture.pixelColor(x,y);
                if (qAbs(pixel.red()-10)<3 && qAbs(pixel.green()-132)<3 && pixel.blue()>252) ++primaryPixels;
            }
            QVERIFY2(primaryPixels > 20, "Selected icons must render at the LVRS primary color.");
        }
        gallery->setParentItem(nullptr);
    }
    void left_alignment_search_radius_and_labels()
    {
        QQmlEngine engine;
        QScopedPointer<QObject> object(create(engine)); QVERIFY(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(430, 200); bar->setParentItem(window.contentItem());
        window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *group = bar->findChild<QQuickItem *>("mobileNavigationGroup");
        auto *search = bar->findChild<QQuickItem *>("mobileNavigationSearch");
        QVERIFY(group && search);
        for (int platform : {1, 2}) {
            bar->setProperty("platformStyle", platform);
            QCOMPARE(bar->property("resolvedCornerRadius").toReal(), platform == 1 ? 28.0 : 12.0);
            for (int count = 1; count <= 5; ++count) {
                eval(engine, bar, QString("model = Array.from({length:%1}, (_, i) => ({iconName:'home-1', accessibleName:'Page '+i}))").arg(count));
                QTRY_COMPARE(bar->property("count").toInt(), count);
                QTRY_COMPARE(group->width(), qreal(count * 56 + 8));
                QCOMPARE(group->mapToItem(bar, QPointF()).x(), 16.0);
                QCOMPARE(search->mapToItem(bar, QPointF()).x() + search->width(), 386.0);
                QVERIFY(!item(engine, bar, 0)->property("showLabel").toBool());
            }
        }
        for (qreal radius : {0.0, 7.5, 20.0, 80.0}) {
            bar->setProperty("deviceCornerRadius", radius);
            QCOMPARE(bar->property("resolvedCornerRadius").toReal(), qMin(radius, 28.0));
        }
        eval(engine, bar, "model = [{iconName:'home-1', text:'Home', accessibleName:'Go home'}]");
        QTRY_VERIFY(item(engine, bar, 0)->property("showLabel").toBool());
        auto *accessible = QAccessible::queryAccessibleInterface(item(engine, bar, 0));
        QVERIFY(accessible); QCOMPARE(accessible->text(QAccessible::Name), QString("Go home"));
        const auto height = bar->implicitHeight(); bar->setProperty("bottomSafeInset", 34);
        QCOMPARE(bar->implicitHeight(), height + 34);
        eval(engine, bar, "LayoutMirroring.enabled = true");
        QCOMPARE(group->mapToItem(bar, QPointF()).x(), 16.0);
        QCOMPARE(search->mapToItem(bar, QPointF()).x() + search->width(), 386.0);
        bar->setParentItem(nullptr);
    }

    void input_disabled_controlled_selection_and_overflow()
    {
        QQmlEngine engine; QScopedPointer<QObject> object(create(engine)); QVERIFY(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(430, 200); bar->setParentItem(window.contentItem());
        window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        QSignalSpy activated(bar, SIGNAL(activated(int))), searched(bar, SIGNAL(searchRequested()));
        auto *search = bar->findChild<QQuickItem *>("mobileNavigationSearch"); QVERIFY(search);
        auto *accessible = QAccessible::queryAccessibleInterface(search); QVERIFY(accessible);
        QCOMPARE(accessible->role(), QAccessible::Button);
        QCOMPARE(accessible->text(QAccessible::Name), QString("Search"));
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, search->mapToScene(QPointF(28,28)).toPoint());
        QTRY_COMPARE(searched.size(), 1); QCOMPARE(activated.size(), 0);
        QCOMPARE(bar->property("currentIndex").toInt(), 0);
        eval(engine, bar, "model = [{text:'Home'}, {text:'Off', enabled:false}, {text:'Library'}]");
        QTRY_COMPARE(bar->property("count").toInt(), 3);
        item(engine, bar, 0)->forceActiveFocus(); QTest::keyClick(&window, Qt::Key_Right);
        QTRY_VERIFY(item(engine, bar, 2)->hasActiveFocus());
        QTest::keyClick(&window, Qt::Key_Space);
        QTRY_COMPARE(bar->property("currentIndex").toInt(), 2);
        QCOMPARE(activated.size(), 1);
        eval(engine, bar, "activate(1)"); QCOMPARE(activated.size(), 1);
        bar->setProperty("autoSelect", false); eval(engine, bar, "activate(0)");
        QCOMPARE(activated.size(), 2); QCOMPARE(bar->property("currentIndex").toInt(), 2);
        bar->setProperty("autoSelect", true);
        // Replacing the model recreates delegates; wait for Row's layout pass
        // before mapping a real touch position into the scene.
        QTRY_COMPARE(item(engine, bar, 2)->x(), 112.0);
        auto *touch = QTest::createTouchDevice();
        const auto point = item(engine,bar,0)->mapToScene(QPointF(28,24)).toPoint();
        QTest::touchEvent(&window,touch).press(0,point,&window);
        QTRY_VERIFY(item(engine, bar, 0)->property("down").toBool());
        QTest::touchEvent(&window,touch).release(0,point,&window);
        QTRY_COMPARE(bar->property("currentIndex").toInt(),0); QCOMPARE(activated.size(),3);
        eval(engine, bar, "search = {enabled:false}");
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, search->mapToScene(QPointF(28,28)).toPoint());
        QCOMPARE(searched.size(), 1);
        eval(engine, bar, "model = Array.from({length:5}, (_, i) => ({iconName:'home-1', accessibleName:'Page '+i}))");
        bar->setWidth(320); QTRY_COMPARE(bar->property("count").toInt(), 5);
        for (int i=0;i<5;++i) QVERIFY(item(engine, bar, i)->width() >= 48);
        eval(engine, bar, "focusItem(4)");
        auto *viewport=bar->findChild<QQuickItem *>("mobileNavigationViewport"); QVERIFY(viewport);
        QTRY_VERIFY(viewport->property("contentX").toReal()>0);
        const auto rect=item(engine,bar,4)->mapRectToItem(viewport,item(engine,bar,4)->boundingRect());
        QVERIFY(rect.left()>=-0.1 && rect.right()<=viewport->width()+0.1);
        eval(engine, bar, "model=[]"); QTRY_COMPARE(bar->property("count").toInt(), 0);
        QVERIFY(search->isVisible());
        bar->setParentItem(nullptr);
    }

    void optional_search_reclaims_space_and_skips_focus()
    {
        QQmlEngine engine;
        QScopedPointer<QObject> object(create(engine, false, false)); QVERIFY(object);
        auto *bar = qobject_cast<QQuickItem *>(object.data()); QVERIFY(bar);
        bar->setWidth(320);
        eval(engine, bar, "model = Array.from({length:5}, (_, i) => ({iconName:'home-1', accessibleName:'Page '+i}))");
        QQuickWindow window; window.resize(430, 200); bar->setParentItem(window.contentItem());
        window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *group = bar->findChild<QQuickItem *>("mobileNavigationGroup");
        auto *search = bar->findChild<QQuickItem *>("mobileNavigationSearch");
        auto *viewport = bar->findChild<QQuickItem *>("mobileNavigationViewport");
        auto *indicator = bar->findChild<QQuickItem *>("mobileNavigationIndicator");
        QVERIFY(group && search && viewport && indicator);
        QSignalSpy searched(bar, SIGNAL(searchRequested()));

        // Omitting the argument hides Search and returns all available width.
        QVERIFY(!search->isVisible()); QVERIFY(!search->isEnabled());
        QTRY_COMPARE(group->width(), 288.0);
        QCOMPARE(group->mapToItem(bar, QPointF()).x(), 16.0);
        QCOMPARE(item(engine, bar, 0)->width(), 56.0);
        QTRY_COMPARE(viewport->property("contentWidth").toReal(), viewport->width());
        QVERIFY(!eval(engine, bar, "focusItem(count)").toBool());
        QVERIFY(eval(engine, bar, "focusItem(4)").toBool());
        QTest::keyClick(&window, Qt::Key_Right);
        QTRY_VERIFY(item(engine, bar, 0)->hasActiveFocus());
        QTest::keyClick(&window, Qt::Key_End);
        QTRY_VERIFY(item(engine, bar, 4)->hasActiveFocus());
        QTest::keyClick(&window, Qt::Key_Space);
        QTRY_COMPARE(bar->property("currentIndex").toInt(), 4);
        QTRY_COMPARE(indicator->x(), 224.0);

        // An empty configuration opts in without affecting tab selection.
        eval(engine, bar, "search = ({})");
        QTRY_VERIFY(search->isVisible()); QVERIFY(search->isEnabled());
        QTRY_COMPARE(group->width(), 220.0);
        QCOMPARE(search->mapToItem(bar, QPointF()).x() + search->width(), 304.0);
        QTRY_COMPARE(indicator->x(), 192.0);
        QVERIFY(eval(engine, bar, "focusItem(count)").toBool());
        QTRY_VERIFY(search->hasActiveFocus());
        QTest::keyClick(&window, Qt::Key_Return);
        QTRY_COMPARE(searched.size(), 1);
        QCOMPARE(bar->property("currentIndex").toInt(), 4);

        for (const auto &value : {"null", "undefined", "({text:'Search', visible:false})"}) {
            eval(engine, bar, "search = ({})");
            QVERIFY(eval(engine, bar, "focusItem(count)").toBool());
            eval(engine, bar, QString("search = %1").arg(value));
            QTRY_VERIFY(!search->isVisible()); QVERIFY(!search->isEnabled());
            QTRY_VERIFY(!search->hasActiveFocus());
            QTRY_COMPARE(group->width(), 288.0);
            QCOMPARE(group->mapToItem(bar, QPointF()).x(), 16.0);
            QCOMPARE(bar->property("currentIndex").toInt(), 4);
            QTRY_COMPARE(indicator->x(), 224.0);
            QTRY_COMPARE(viewport->property("contentX").toReal(), 0.0);
            QVERIFY(!eval(engine, bar, "focusItem(count)").toBool());
            QVERIFY(QMetaObject::invokeMethod(search, "clicked"));
            QCOMPARE(searched.size(), 1);
        }
        eval(engine, bar, "model = []");
        QTRY_VERIFY(!group->isVisible()); QVERIFY(!search->isVisible());
        QVERIFY(!eval(engine, bar, "focusItem(0)").toBool());
        eval(engine, bar, "moveFocus(0, 1)");
        bar->setParentItem(nullptr);
    }

    void shared_indicator_stretches_retargets_and_reduces_motion()
    {
        QQmlEngine engine; QScopedPointer<QObject> object(create(engine, true)); QVERIFY(object);
        auto *bar=qobject_cast<QQuickItem *>(object.data());
        QQuickWindow window; window.resize(430,200); bar->setParentItem(window.contentItem());
        window.show(); QVERIFY(QTest::qWaitForWindowExposed(&window));
        auto *indicator=bar->findChild<QQuickItem *>("mobileNavigationIndicator"); QVERIFY(indicator);
        QTRY_COMPARE(indicator->width(), 56.0);
        QTest::qWait(50); bar->setProperty("currentIndex",1);
        QTRY_VERIFY_WITH_TIMEOUT(indicator->width()>60, 250);
        QVERIFY(indicator->height()<48);
        bar->setProperty("currentIndex",2);
        QTRY_VERIFY_WITH_TIMEOUT(qAbs(indicator->x()-112)<0.1 && qAbs(indicator->width()-56)<0.1, 1000);
        bar->setProperty("currentIndex",0); QTest::qWait(70);
        eval(engine,bar,"LV.Motion.reducedMotion=true");
        QVERIFY(eval(engine,bar,"LV.Motion.reducedMotion").toBool());
        QTRY_COMPARE(indicator->x(),0.0); QCOMPARE(indicator->width(),56.0);
        bar->setProperty("currentIndex",2); QTRY_COMPARE(indicator->x(),112.0);
        eval(engine,bar,"LV.Motion.reducedMotion=false");
        bar->setProperty("currentIndex",0); QTest::qWait(50);
        bar->setProperty("motionEnabled",false);
        QTRY_COMPARE(indicator->x(),0.0);
        eval(engine,bar,"model=[]"); QTRY_VERIFY(!indicator->isVisible());
        bar->setParentItem(nullptr);
    }
};
QTEST_MAIN(MobileNavigationTests)
#include "tst_mobile_navigation.moc"
