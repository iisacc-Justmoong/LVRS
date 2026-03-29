#include "backend/platform/nativewindowstyle.h"

#include <QGuiApplication>
#include <QWindow>

#import <AppKit/AppKit.h>

namespace {
NSColor *toNativeColor(const QColor &color)
{
    return [NSColor colorWithSRGBRed:color.redF()
                               green:color.greenF()
                                blue:color.blueF()
                               alpha:color.alphaF()];
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
    [nativeWindow setBackgroundColor:toNativeColor(color)];
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
    [nativeWindow setBackgroundColor:toNativeColor(color)];
    [nativeWindow setOpaque:YES];
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
