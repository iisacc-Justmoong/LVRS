#include <QtTest>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QScopeGuard>
#include "backend/platform/nativewindowstyle.h"
#include "test_utils.h"

#import <AppKit/AppKit.h>

namespace {
NSArray<NSView *> *backdrops(NSView *view)
{
    NSMutableArray<NSView *> *result = [NSMutableArray array];
    for (NSView *sibling in view.superview.subviews)
        if ([sibling.identifier isEqualToString:@"LVRSWindowBackdrop"])
            [result addObject:sibling];
    return result;
}
}

class NativeWindowBlurTests : public QObject
{
    Q_OBJECT
private slots:
    void lifecycle_and_foreground();
    void unified_titlebar_controls();
};

void NativeWindowBlurTests::lifecycle_and_foreground()
{
    NativeWindowStyle style;
    QObject invalid;
    QVERIFY(!style.applyBackgroundBlur(&invalid));
    if (QGuiApplication::platformName() != QStringLiteral("cocoa")) {
        QVERIFY(!style.backgroundBlurSupported());
        QSKIP("Run with QT_QPA_PLATFORM=cocoa and native Metal for AppKit verification.");
    }
    QVERIFY(style.backgroundBlurSupported());
    QVERIFY(QQuickWindow::hasDefaultAlphaBuffer());
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    id: testWindow
    width: 900; height: 600; visible: true
    property int clicks: 0
    LV.LabelButton {
        objectName: "foregroundButton"; x: 200; y: 180; text: "Continue"
        tone: LV.AbstractButton.Primary
        onClicked: testWindow.clicks++
    }
})"));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTRY_VERIFY(root->property("backgroundBlurActive").toBool());
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    QCOMPARE(backdrops(view).count, NSUInteger(1));
    NSVisualEffectView *effect = static_cast<NSVisualEffectView *>(backdrops(view).firstObject);
    QCOMPARE(effect.blendingMode, NSVisualEffectBlendingModeBehindWindow);
    QCOMPARE(effect.material, NSVisualEffectMaterialUnderWindowBackground);
    QCOMPARE(effect.state, NSVisualEffectStateActive);
    QVERIFY(!view.window.opaque);
    QCOMPARE(view.window.alphaValue, CGFloat(1.0));
    QCOMPARE(window->color().alpha(), 0);
    QCOMPARE(window->opacity(), 1.0);
    QVERIFY([effect hitTest:NSMakePoint(200, 200)] == nil);
    QVERIFY(view.window.contentView == view);
    for (const QSize size : {QSize(1100, 720), QSize(900, 600)}) {
        window->resize(size);
        QTRY_VERIFY(NSEqualRects(effect.frame, view.frame));
        QVERIFY(style.applyBackgroundBlur(window));
        QVERIFY(style.applySolidChrome(window, QColor("#141414")));
        QCOMPARE(backdrops(view).count, NSUInteger(1));
        QVERIFY(!view.window.opaque);
    }
    auto *button = root->findChild<QQuickItem *>("foregroundButton");
    QVERIFY(button);
    const QPoint point = button->mapToScene(QPointF(button->width()/2, button->height()/2)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point);
    QTRY_COMPARE(root->property("clicks").toInt(), 1);
    const QColor buttonPixel = window->grabWindow().pixelColor(204 * window->devicePixelRatio(), 190 * window->devicePixelRatio());
    QCOMPARE(buttonPixel.alpha(), 255);
    QVERIFY(root->setProperty("backgroundBlurEnabled", false));
    QTRY_VERIFY(!root->property("backgroundBlurActive").toBool());
    QCOMPARE(backdrops(view).count, NSUInteger(0));
    QVERIFY(view.window.opaque);
    QCOMPARE(window->color(), root->property("windowColor").value<QColor>());
    QVERIFY(root->setProperty("backgroundBlurEnabled", true));
    QTRY_COMPARE(backdrops(view).count, NSUInteger(1));
    window->hide();
    window->destroy();
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    view = reinterpret_cast<NSView *>(window->winId());
    QTRY_COMPARE(backdrops(view).count, NSUInteger(1));
    QVERIFY(!view.window.opaque);
}

void NativeWindowBlurTests::unified_titlebar_controls()
{
    if (QGuiApplication::platformName() != QStringLiteral("cocoa"))
        QSKIP("Native title-bar geometry requires Cocoa.");
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    width: 900; height: 600; visible: true
    nativeTitleBarHeight: 56
})"));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window && QTest::qWaitForWindowExposed(window));
    const auto controlsRect = [window] {
        NSView *view = reinterpret_cast<NSView *>(window->winId());
        NSRect result = NSZeroRect;
        for (NSWindowButton kind : {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton}) {
            NSButton *button = [view.window standardWindowButton:kind];
            const NSRect frame = [view convertRect:button.bounds fromView:button];
            result = NSIsEmptyRect(result) ? frame : NSUnionRect(result, frame);
        }
        return QRectF(result.origin.x, view.isFlipped ? result.origin.y : view.bounds.size.height - NSMaxY(result),
                      result.size.width, result.size.height);
    };
    for (const QSize size : {QSize(900, 600), QSize(640, 480), QSize(1100, 720)}) {
        window->resize(size);
        QTRY_VERIFY(qAbs(controlsRect().center().y() - 28.0) < 0.5);
        QCOMPARE(root->property("nativeTitleBarControlsRect").toRectF(), controlsRect());
        NSView *view = reinterpret_cast<NSView *>(window->winId());
        NSButton *close = [view.window standardWindowButton:NSWindowCloseButton];
        NSPoint center = [close.superview convertPoint:NSMakePoint(NSMidX(close.bounds), NSMidY(close.bounds)) fromView:close];
        QVERIFY([close hitTest:center] == close);
    }
    QVERIFY(root->setProperty("nativeTitleBarHeight", 72));
    QTRY_VERIFY(qAbs(controlsRect().center().y() - 36.0) < 0.5);
    QVERIFY(root->setProperty("nativeTitleBarHeight", 0));
    QTRY_VERIFY(controlsRect().center().y() < 28.0);
    QVERIFY(root->property("nativeTitleBarControlsRect").toRectF().isEmpty());
    QVERIFY(root->setProperty("nativeTitleBarHeight", 56));
    __block bool enteredFullScreen = false;
    __block bool exitedFullScreen = false;
    NSWindow *nativeWindow = reinterpret_cast<NSView *>(window->winId()).window;
    NSNotificationCenter *notifications = NSNotificationCenter.defaultCenter;
    id enteredObserver = [notifications addObserverForName:NSWindowDidEnterFullScreenNotification
        object:nativeWindow queue:nil usingBlock:^(NSNotification *) { enteredFullScreen = true; }];
    id exitedObserver = [notifications addObserverForName:NSWindowDidExitFullScreenNotification
        object:nativeWindow queue:nil usingBlock:^(NSNotification *) { exitedFullScreen = true; }];
    const auto removeObservers = qScopeGuard([&] {
        [notifications removeObserver:enteredObserver];
        [notifications removeObserver:exitedObserver];
    });
    window->showFullScreen();
    QTRY_VERIFY_WITH_TIMEOUT(enteredFullScreen, 10000);
    QVERIFY(root->property("nativeTitleBarControlsRect").toRectF().isEmpty());
    window->showNormal();
    QTRY_VERIFY_WITH_TIMEOUT(exitedFullScreen, 10000);
    QTRY_VERIFY(qAbs(controlsRect().center().y() - 28.0) < 0.5);
    window->hide();
    window->destroy();
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    QTRY_VERIFY(qAbs(controlsRect().center().y() - 28.0) < 0.5);
}

QTEST_MAIN(NativeWindowBlurTests)
#include "tst_nativewindowblur.moc"
