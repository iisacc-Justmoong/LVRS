#include "QtEventWait.h"
#include <QGuiApplication>
#include <QMouseEvent>
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

class NativeWindowBlurTests
{
public:
    void lifecycle_and_foreground();
    void explicit_native_blur_remains_opt_in();
    void unified_titlebar_controls();
    void unified_titlebar_survives_native_update();
    void solid_chrome_disables_background_movement();
};

void NativeWindowBlurTests::lifecycle_and_foreground()
{
    NativeWindowStyle style;
    QObject invalid;
    CTEST_CHECK(!style.applyBackgroundBlur(&invalid));
    if (QGuiApplication::platformName() != QStringLiteral("cocoa")) {
        CTEST_CHECK(!style.backgroundBlurSupported());
        lvrs_ctest::skip("Run with QT_QPA_PLATFORM=cocoa and native Metal for AppKit verification.");
    }
    CTEST_CHECK(style.backgroundBlurSupported());
    CTEST_CHECK(QQuickWindow::hasDefaultAlphaBuffer());
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
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    CTEST_CHECK(!root->property("backgroundBlurActive").toBool());
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    CTEST_WAIT_COMPARE(backdrops(view).count, NSUInteger(0));
    CTEST_WAIT_DEFAULT(view.window.opaque);
    CTEST_COMPARE(window->format().alphaBufferSize(), 0);
    CTEST_COMPARE(view.window.alphaValue, CGFloat(1.0));
    CTEST_COMPARE(window->color().alpha(), 255);
    CTEST_COMPARE(window->opacity(), 1.0);
    CTEST_CHECK(view.window.contentView == view);
    for (const QSize size : {QSize(1100, 720), QSize(900, 600)}) {
        window->resize(size);
        CTEST_CHECK(style.applySolidChrome(window, QColor("#141414")));
        CTEST_COMPARE(backdrops(view).count, NSUInteger(0));
        CTEST_CHECK(view.window.opaque);
    }
    auto *button = root->findChild<QQuickItem *>("foregroundButton");
    CTEST_CHECK(button);
    const QPoint point = button->mapToScene(QPointF(button->width()/2, button->height()/2)).toPoint();
    const QPointF localPoint(point);
    const QPointF globalPoint(window->mapToGlobal(point));
    QMouseEvent press(QEvent::MouseButtonPress, localPoint, localPoint, globalPoint,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, localPoint, localPoint, globalPoint,
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &release);
    CTEST_WAIT_COMPARE(root->property("clicks").toInt(), 1);
    const QColor buttonPixel = window->grabWindow().pixelColor(204 * window->devicePixelRatio(), 190 * window->devicePixelRatio());
    CTEST_COMPARE(buttonPixel.alpha(), 255);
    for (const bool enabled : {false, true}) {
        CTEST_CHECK(root->setProperty("backgroundBlurEnabled", enabled));
        CTEST_CHECK(!root->property("backgroundBlurActive").toBool());
        CTEST_COMPARE(backdrops(view).count, NSUInteger(0));
        CTEST_CHECK(view.window.opaque);
        CTEST_COMPARE(window->color().alpha(), 255);
    }
    CTEST_CHECK(root->setProperty("windowColor", QColor("#33202124")));
    CTEST_WAIT_DEFAULT(view.window.opaque);
    CTEST_COMPARE(window->color(), QColor("#202124"));
    window->hide();
    window->destroy();
    window->show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    view = reinterpret_cast<NSView *>(window->winId());
    CTEST_WAIT_COMPARE(backdrops(view).count, NSUInteger(0));
    CTEST_WAIT_DEFAULT(view.window.opaque);
    CTEST_CHECK(!view.window.movableByWindowBackground);
}

void NativeWindowBlurTests::explicit_native_blur_remains_opt_in()
{
    if (QGuiApplication::platformName() != QStringLiteral("cocoa"))
        lvrs_ctest::skip("Explicit native blur requires Cocoa.");
    QQuickWindow window;
    window.setColor(QColor("#141414"));
    window.resize(640, 400);
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    NativeWindowStyle style;
    CTEST_CHECK(style.applySolidChrome(&window, QColor("#141414")));
    NSView *view = reinterpret_cast<NSView *>(window.winId());
    CTEST_COMPARE(backdrops(view).count, NSUInteger(0));
    CTEST_CHECK(style.applyBackgroundBlur(&window, true));
    CTEST_COMPARE(window.format().alphaBufferSize(), 8);
    CTEST_COMPARE(backdrops(view).count, NSUInteger(1));
    auto *effect = static_cast<NSVisualEffectView *>(backdrops(view).firstObject);
    CTEST_COMPARE(effect.blendingMode, NSVisualEffectBlendingModeBehindWindow);
    CTEST_CHECK([effect hitTest:NSMakePoint(100, 100)] == nil);
    CTEST_CHECK(!view.window.opaque);
    CTEST_CHECK(style.applyBackgroundBlur(&window, false));
    CTEST_COMPARE(window.format().alphaBufferSize(), 0);
    CTEST_COMPARE(backdrops(view).count, NSUInteger(0));
    CTEST_CHECK(view.window.opaque);
}

void NativeWindowBlurTests::solid_chrome_disables_background_movement()
{
    if (QGuiApplication::platformName() != QStringLiteral("cocoa"))
        lvrs_ctest::skip("Native window movement requires Cocoa.");
    QQuickWindow window;
    window.resize(900, 600);
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    const auto nativeWindow = [&window] {
        return reinterpret_cast<NSView *>(window.winId()).window;
    };
    NativeWindowStyle style;
    CTEST_CHECK(style.applySolidChrome(&window, QColor("#141414")));
    CTEST_CHECK(!nativeWindow().movableByWindowBackground);
    // Style/theme refreshes must not restore whole-window background movement.
    nativeWindow().movableByWindowBackground = YES;
    CTEST_CHECK(style.applySolidChrome(&window, QColor("#242424")));
    CTEST_CHECK(!nativeWindow().movableByWindowBackground);
    window.hide();
    window.destroy();
    window.show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window.isExposed(); }, 5000));
    CTEST_CHECK(style.applySolidChrome(&window, QColor("#141414")));
    CTEST_CHECK(!nativeWindow().movableByWindowBackground);
}

void NativeWindowBlurTests::unified_titlebar_controls()
{
    if (QGuiApplication::platformName() != QStringLiteral("cocoa"))
        lvrs_ctest::skip("Native title-bar geometry requires Cocoa.");
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    width: 900; height: 600; visible: true
    nativeTitleBarHeight: 56
})"));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window && lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
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
        CTEST_WAIT_DEFAULT(qAbs(controlsRect().center().y() - 28.0) < 0.5);
        CTEST_COMPARE(root->property("nativeTitleBarControlsRect").toRectF(), controlsRect());
        NSView *view = reinterpret_cast<NSView *>(window->winId());
        NSButton *close = [view.window standardWindowButton:NSWindowCloseButton];
        NSPoint center = [close.superview convertPoint:NSMakePoint(NSMidX(close.bounds), NSMidY(close.bounds)) fromView:close];
        CTEST_CHECK([close hitTest:center] == close);
    }
    CTEST_CHECK(root->setProperty("nativeTitleBarHeight", 72));
    CTEST_WAIT_DEFAULT(qAbs(controlsRect().center().y() - 36.0) < 0.5);
    CTEST_CHECK(root->setProperty("nativeTitleBarHeight", 0));
    CTEST_WAIT_DEFAULT(controlsRect().center().y() < 28.0);
    CTEST_CHECK(root->property("nativeTitleBarControlsRect").toRectF().isEmpty());
    CTEST_CHECK(root->setProperty("nativeTitleBarHeight", 56));
    bool enteredFullScreen = false;
    bool exitedFullScreen = false;
    bool *const enteredFlag = &enteredFullScreen;
    bool *const exitedFlag = &exitedFullScreen;
    NSWindow *nativeWindow = reinterpret_cast<NSView *>(window->winId()).window;
    NSNotificationCenter *notifications = NSNotificationCenter.defaultCenter;
    id enteredObserver = [notifications addObserverForName:NSWindowDidEnterFullScreenNotification
        object:nativeWindow queue:nil usingBlock:^(NSNotification *) { *enteredFlag = true; }];
    id exitedObserver = [notifications addObserverForName:NSWindowDidExitFullScreenNotification
        object:nativeWindow queue:nil usingBlock:^(NSNotification *) { *exitedFlag = true; }];
    const auto removeObservers = qScopeGuard([&] {
        [notifications removeObserver:enteredObserver];
        [notifications removeObserver:exitedObserver];
    });
    window->showFullScreen();
    CTEST_WAIT_VERIFY(enteredFullScreen, 10000);
    CTEST_CHECK(root->property("nativeTitleBarControlsRect").toRectF().isEmpty());
    window->showNormal();
    CTEST_WAIT_VERIFY(exitedFullScreen, 10000);
    CTEST_WAIT_DEFAULT(qAbs(controlsRect().center().y() - 28.0) < 0.5);
    window->hide();
    window->destroy();
    window->show();
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    CTEST_WAIT_DEFAULT(qAbs(controlsRect().center().y() - 28.0) < 0.5);
}

void NativeWindowBlurTests::unified_titlebar_survives_native_update()
{
    if (QGuiApplication::platformName() != QStringLiteral("cocoa"))
        lvrs_ctest::skip("Native title-bar updates require Cocoa.");
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> root(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.ApplicationWindow {
    width: 900; height: 600; visible: true
    nativeTitleBarHeight: 56
})"));
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window && lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *nativeWindow = view.window;
    QEventLoop pause;
    QTimer::singleShot(100, &pause, &QEventLoop::quit);
    pause.exec();
    // AppKit may reflow its controls after LVRS handled resize/activation.
    // A later window update must restore their center before drawing.
    for (int pass = 0; pass < 3; ++pass) {
        for (NSWindowButton kind : {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton}) {
            NSButton *button = [nativeWindow standardWindowButton:kind];
            [button setFrameOrigin:NSMakePoint(button.frame.origin.x, button.frame.origin.y + 12)];
        }
        [nativeWindow update];
        for (NSWindowButton kind : {NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton}) {
            NSButton *button = [nativeWindow standardWindowButton:kind];
            const NSRect frame = [view convertRect:button.bounds fromView:button];
            const qreal center = view.isFlipped ? NSMidY(frame) : view.bounds.size.height - NSMidY(frame);
            CTEST_CHECK_MESSAGE(qAbs(center - 28.0) < 0.5, qPrintable(QString("Native control center: %1").arg(center)));
            NSPoint hit = [button.superview convertPoint:NSMakePoint(NSMidX(button.bounds), NSMidY(button.bounds)) fromView:button];
            CTEST_CHECK([button hitTest:hit] == button);
        }
    }
    CTEST_CHECK(root->setProperty("nativeTitleBarHeight", 0));
    NSButton *close = [nativeWindow standardWindowButton:NSWindowCloseButton];
    const NSRect originalFrame = close.frame;
    [nativeWindow update];
    CTEST_CHECK(NSEqualRects(close.frame, originalFrame));
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner;
    NativeWindowBlurTests tests;
    runner.run("lifecycle_and_foreground", [&] { tests.lifecycle_and_foreground(); });
    runner.run("explicit_native_blur_remains_opt_in", [&] { tests.explicit_native_blur_remains_opt_in(); });
    runner.run("unified_titlebar_controls", [&] { tests.unified_titlebar_controls(); });
    runner.run("unified_titlebar_survives_native_update", [&] { tests.unified_titlebar_survives_native_update(); });
    runner.run("solid_chrome_disables_background_movement", [&] { tests.solid_chrome_disables_background_movement(); });
    return runner.result();
}
