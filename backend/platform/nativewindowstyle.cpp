#include "backend/platform/nativewindowstyle.h"

#include <QWindow>
#include <QtGlobal>

NativeWindowStyle::NativeWindowStyle(QObject *parent)
    : QObject(parent)
{
}

bool NativeWindowStyle::titleBarColorSupported() const
{
    return false;
}

bool NativeWindowStyle::solidChromeSupported() const
{
    return false;
}

bool NativeWindowStyle::applyTitleBarColor(QObject *window, const QColor &color, bool darkAppearance)
{
    Q_UNUSED(window);
    Q_UNUSED(color);
    Q_UNUSED(darkAppearance);
    return false;
}

bool NativeWindowStyle::applySolidChrome(QObject *window, const QColor &color, bool darkAppearance)
{
    Q_UNUSED(window);
    Q_UNUSED(color);
    Q_UNUSED(darkAppearance);
    return false;
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
