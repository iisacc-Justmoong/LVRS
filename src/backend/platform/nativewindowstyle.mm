#include "backend/platform/nativewindowstyle.h"

#include <QGuiApplication>
#include <QQuickWindow>
#include <QSurfaceFormat>
#include <QWindow>

#import <AppKit/AppKit.h>
#import <objc/runtime.h>

static const NSWindowButton titleBarButtons[] = {
    NSWindowCloseButton, NSWindowMiniaturizeButton, NSWindowZoomButton
};
static char titleBarLayoutKey;

// Keep AppKit's real controls, targets, accessibility and fullscreen behavior.
// The window owns this observer; no application delegate or private API is replaced.
@interface LVRSWindowTitleBarLayout : NSObject {
    NSWindow *_targetWindow;
    CGFloat _originalTitleBarHeight;
    CGFloat _originalContainerHeight;
    NSPoint _originalButtonOrigins[3];
    BOOL _updating;
    BOOL _fullscreenTransition;
}
@property CGFloat height;
@property CGFloat leftMargin;
- (instancetype)initWithWindow:(NSWindow *)window;
- (void)updateLayout:(NSNotification *)notification;
- (void)restoreLayout;
@end

@implementation LVRSWindowTitleBarLayout
- (instancetype)initWithWindow:(NSWindow *)window
{
    if ((self = [super init])) {
        _targetWindow = window;
        NSView *titleBar = [window standardWindowButton:NSWindowCloseButton].superview;
        _originalTitleBarHeight = titleBar.frame.size.height;
        _originalContainerHeight = titleBar.superview.frame.size.height;
        for (int i = 0; i < 3; ++i)
            _originalButtonOrigins[i] = [window standardWindowButton:titleBarButtons[i]].frame.origin;
        NSNotificationCenter *center = NSNotificationCenter.defaultCenter;
        for (NSNotificationName name in @[NSWindowDidResizeNotification, NSWindowDidBecomeKeyNotification,
                                           NSWindowDidDeminiaturizeNotification])
            [center addObserver:self selector:@selector(updateLayout:) name:name object:window];
        [center addObserver:self selector:@selector(willEnterFullScreen:) name:NSWindowWillEnterFullScreenNotification object:window];
        [center addObserver:self selector:@selector(didExitFullScreen:) name:NSWindowDidExitFullScreenNotification object:window];
    }
    return self;
}
- (void)resizeTitleBar:(CGFloat)barHeight containerHeight:(CGFloat)containerHeight
{
    NSView *titleBar = [_targetWindow standardWindowButton:NSWindowCloseButton].superview;
    NSView *container = titleBar.superview;
    NSRect frame = container.frame;
    frame.origin.y += frame.size.height - containerHeight;
    frame.size.height = containerHeight;
    container.frame = frame;
    frame = titleBar.frame;
    frame.size.height = barHeight;
    titleBar.frame = frame;
}
- (void)updateLayout:(NSNotification *)notification
{
    Q_UNUSED(notification);
    if (_updating || _fullscreenTransition || self.height <= 0
            || (_targetWindow.styleMask & NSWindowStyleMaskFullScreen))
        return;
    _updating = YES;
    [self resizeTitleBar:self.height containerHeight:self.height];
    for (int i = 0; i < 3; ++i) {
        NSButton *button = [_targetWindow standardWindowButton:titleBarButtons[i]];
        NSRect frame = button.frame;
        frame.origin.x = self.leftMargin + _originalButtonOrigins[i].x - _originalButtonOrigins[0].x;
        frame.origin.y = (self.height - frame.size.height) * 0.5;
        button.frame = frame;
    }
    _updating = NO;
}
- (void)restoreLayout
{
    _updating = YES;
    [self resizeTitleBar:_originalTitleBarHeight containerHeight:_originalContainerHeight];
    for (int i = 0; i < 3; ++i) {
        NSButton *button = [_targetWindow standardWindowButton:titleBarButtons[i]];
        [button setFrameOrigin:_originalButtonOrigins[i]];
    }
    _updating = NO;
}
- (void)willEnterFullScreen:(NSNotification *)notification
{
    Q_UNUSED(notification);
    _fullscreenTransition = YES;
    [self restoreLayout];
}
- (void)didExitFullScreen:(NSNotification *)notification
{
    _fullscreenTransition = NO;
    [self updateLayout:notification];
}
- (void)dealloc
{
    [NSNotificationCenter.defaultCenter removeObserver:self];
#if !__has_feature(objc_arc)
    [super dealloc];
#endif
}
@end

// The effect is a sibling below Qt's render view, never above its controls.
// Returning nil keeps window movement, resize handles and input owned by Qt.
@interface LVRSWindowBackdropView : NSVisualEffectView
@property(strong) NSColor *originalBackgroundColor;
@property BOOL originalOpaque;
@end

@implementation LVRSWindowBackdropView
- (NSView *)hitTest:(NSPoint)point
{
    Q_UNUSED(point);
    return nil;
}
#if !__has_feature(objc_arc)
- (void)dealloc
{
    [_originalBackgroundColor release];
    [super dealloc];
}
#endif
@end

namespace {
void enableWindowAlphaBuffers()
{
    // Qt requires this before the first Quick window. It also permits toggling
    // the native material without destroying an existing render surface.
    QQuickWindow::setDefaultAlphaBuffer(true);
}
Q_COREAPP_STARTUP_FUNCTION(enableWindowAlphaBuffers)

LVRSWindowBackdropView *backdropForView(NSView *view)
{
    for (NSView *sibling in view.superview.subviews) {
        if ([sibling isKindOfClass:[LVRSWindowBackdropView class]])
            return static_cast<LVRSWindowBackdropView *>(sibling);
    }
    return nil;
}

NSColor *toNativeColor(const QColor &color)
{
    return [NSColor colorWithSRGBRed:color.redF()
                               green:color.greenF()
                                blue:color.blueF()
                               alpha:color.alphaF()];
}

void applyNativeBackground(NSView *view, const QColor &color)
{
    LVRSWindowBackdropView *backdrop = backdropForView(view);
    if (backdrop)
        backdrop.originalBackgroundColor = toNativeColor(color);
    view.window.backgroundColor = backdrop ? NSColor.clearColor : toNativeColor(color);
    view.window.opaque = !backdrop && color.alphaF() >= 1.0;
}
}

NativeWindowStyle::NativeWindowStyle(QObject *parent)
    : QObject(parent)
{
}

bool NativeWindowStyle::titleBarColorSupported() const
{
    return true;
}

bool NativeWindowStyle::solidChromeSupported() const
{
    return true;
}

bool NativeWindowStyle::backgroundBlurSupported() const
{
    return qGuiApp && QGuiApplication::platformName() == QStringLiteral("cocoa");
}

QRectF NativeWindowStyle::layoutTitleBar(QObject *windowObject, qreal height, qreal leftMargin)
{
    auto *window = qobject_cast<QWindow *>(windowObject);
    if (!window || !qGuiApp || QGuiApplication::platformName() != QStringLiteral("cocoa")
            || !qIsFinite(height) || !qIsFinite(leftMargin) || !window->handle())
        return {};
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    NSWindow *nativeWindow = view.window;
    NSButton *close = [nativeWindow standardWindowButton:NSWindowCloseButton];
    if (!close || !close.superview.superview)
        return {};
    LVRSWindowTitleBarLayout *layout = objc_getAssociatedObject(nativeWindow, &titleBarLayoutKey);
    if (height <= 0) {
        if (layout) {
            [layout restoreLayout];
            objc_setAssociatedObject(nativeWindow, &titleBarLayoutKey, nil, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
        }
        return {};
    }
    if (!layout) {
        layout = [[LVRSWindowTitleBarLayout alloc] initWithWindow:nativeWindow];
        objc_setAssociatedObject(nativeWindow, &titleBarLayoutKey, layout, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
#if !__has_feature(objc_arc)
        [layout release];
#endif
    }
    layout.height = qMax(height, qreal(close.frame.size.height));
    layout.leftMargin = qMax(qreal(0), leftMargin);
    [layout updateLayout:nil];
    NSRect bounds = NSZeroRect;
    for (NSWindowButton kind : titleBarButtons) {
        NSButton *button = [nativeWindow standardWindowButton:kind];
        const NSRect frame = [view convertRect:button.bounds fromView:button];
        bounds = NSIsEmptyRect(bounds) ? frame : NSUnionRect(bounds, frame);
    }
    return QRectF(bounds.origin.x, view.isFlipped ? bounds.origin.y : view.bounds.size.height - NSMaxY(bounds),
                  bounds.size.width, bounds.size.height);
}

bool NativeWindowStyle::applyBackgroundBlur(QObject *windowObject, bool enabled)
{
    auto *window = qobject_cast<QWindow *>(windowObject);
    if (!window || !backgroundBlurSupported())
        return false;
    if (!window->handle()) {
        QSurfaceFormat format = window->format();
        format.setAlphaBufferSize(8);
        window->setFormat(format);
        window->create();
    }
    NSView *view = reinterpret_cast<NSView *>(window->winId());
    if (!view || !view.window || !view.superview)
        return false;
    LVRSWindowBackdropView *backdrop = backdropForView(view);
    if (!enabled) {
        if (backdrop) {
            view.window.backgroundColor = backdrop.originalBackgroundColor;
            view.window.opaque = backdrop.originalOpaque;
            [backdrop removeFromSuperview];
        }
        return true;
    }
    if (!backdrop) {
        backdrop = [[LVRSWindowBackdropView alloc] initWithFrame:view.frame];
        backdrop.identifier = @"LVRSWindowBackdrop";
        backdrop.originalBackgroundColor = view.window.backgroundColor;
        backdrop.originalOpaque = view.window.opaque;
        backdrop.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [view.superview addSubview:backdrop positioned:NSWindowBelow relativeTo:view];
#if !__has_feature(objc_arc)
        [backdrop release];
#endif
    }
    backdrop.frame = view.frame;
    // AppKit's thick, frosted window material uses the WindowServer backdrop.
    // Its kernel is system controlled; no private blur-radius API is used.
    backdrop.material = NSVisualEffectMaterialUnderWindowBackground;
    backdrop.blendingMode = NSVisualEffectBlendingModeBehindWindow;
    backdrop.state = NSVisualEffectStateActive;
    view.window.backgroundColor = NSColor.clearColor;
    view.window.opaque = NO;
    return true;
}

bool NativeWindowStyle::applyTitleBarColor(QObject *windowObject, const QColor &color, bool darkAppearance)
{
    auto *window = qobject_cast<QWindow *>(windowObject);
    if (!window)
        return false;

    if (!qGuiApp)
        return false;

    const QString platformName = QGuiApplication::platformName();
    if (platformName.compare(QStringLiteral("cocoa"), Qt::CaseInsensitive) != 0)
        return false;

    if (!window->handle())
        window->create();
    if (!window->handle())
        return false;

    NSView *view = reinterpret_cast<NSView *>(window->winId());
    if (!view)
        return false;

    NSWindow *nativeWindow = view.window;
    if (!nativeWindow)
        return false;

    const bool isDark = darkAppearance;
    applyNativeBackground(view, color);
    [nativeWindow setTitlebarAppearsTransparent:YES];

    if (@available(macOS 11.0, *)) {
        [nativeWindow setToolbarStyle:isDark ? NSWindowToolbarStyleUnifiedCompact : NSWindowToolbarStyleUnified];
        [nativeWindow setTitlebarSeparatorStyle:NSTitlebarSeparatorStyleNone];
    }

    if (@available(macOS 10.14, *)) {
        [nativeWindow setAppearance:[NSAppearance appearanceNamed:isDark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua]];
    }

    return true;
}

bool NativeWindowStyle::applySolidChrome(QObject *windowObject, const QColor &color, bool darkAppearance)
{
    auto *window = qobject_cast<QWindow *>(windowObject);
    if (!window)
        return false;

    if (!qGuiApp)
        return false;

    const QString platformName = QGuiApplication::platformName();
    if (platformName.compare(QStringLiteral("cocoa"), Qt::CaseInsensitive) != 0)
        return false;

    if (!window->handle())
        window->create();
    if (!window->handle())
        return false;

    NSView *view = reinterpret_cast<NSView *>(window->winId());
    if (!view)
        return false;

    NSWindow *nativeWindow = view.window;
    if (!nativeWindow)
        return false;

    const bool isDark = darkAppearance;
    applyNativeBackground(view, color);
    [nativeWindow setTitlebarAppearsTransparent:YES];
    [nativeWindow setTitleVisibility:NSWindowTitleHidden];
    [nativeWindow setMovableByWindowBackground:YES];
    nativeWindow.styleMask |= NSWindowStyleMaskFullSizeContentView;

    if (@available(macOS 11.0, *)) {
        [nativeWindow setToolbarStyle:isDark ? NSWindowToolbarStyleUnifiedCompact : NSWindowToolbarStyleUnified];
        [nativeWindow setTitlebarSeparatorStyle:NSTitlebarSeparatorStyleNone];
    }

    if (@available(macOS 10.14, *)) {
        [nativeWindow setAppearance:[NSAppearance appearanceNamed:isDark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua]];
    }

    return true;
}

bool NativeWindowStyle::applyMobileCoverageFlags(QObject *windowObject,
                                                 bool expandedClientArea,
                                                 bool fullscreenGeometryHint)
{
    auto *window = qobject_cast<QWindow *>(windowObject);
    if (!window)
        return false;

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    window->setFlag(Qt::ExpandedClientAreaHint, expandedClientArea);
    window->setFlag(Qt::NoTitleBarBackgroundHint, expandedClientArea);
#else
    Q_UNUSED(expandedClientArea);
#endif

    window->setFlag(Qt::MaximizeUsingFullscreenGeometryHint, fullscreenGeometryHint);
    return true;
}
