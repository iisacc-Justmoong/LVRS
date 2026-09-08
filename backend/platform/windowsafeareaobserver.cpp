#include "backend/platform/windowsafeareaobserver.h"

#include <QEvent>
#include <QMargins>
#include <QScreen>
#include <QWindow>
#include <QtGui/private/qwindow_p.h>
#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniObject>
#endif

namespace {

struct SafeAreaSnapshot {
    qreal left = 0.0;
    qreal top = 0.0;
    qreal right = 0.0;
    qreal bottom = 0.0;
    bool resolved = false;
};

SafeAreaSnapshot querySafeAreaSnapshot(QWindow *window)
{
    SafeAreaSnapshot snapshot;
    if (!window)
        return snapshot;

#ifdef Q_OS_ANDROID
    // Qt 6.8's Android QPA returns empty safeAreaMargins, including when Android
    // 15 enforces edge-to-edge windows. Read the actual WindowInsets instead.
    if (QNativeInterface::QAndroidApplication::isActivityContext()) {
        const QJniObject activity = QNativeInterface::QAndroidApplication::context();
        const auto nativeWindow = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        const auto decor = nativeWindow.callObjectMethod("getDecorView", "()Landroid/view/View;");
        const auto insets = decor.callObjectMethod("getRootWindowInsets", "()Landroid/view/WindowInsets;");
        if (insets.isValid()) {
            const qreal scale = qMax(qreal(1), window->devicePixelRatio());
            if (QJniObject::getStaticField<jint>("android/os/Build$VERSION", "SDK_INT") >= 30) {
                const int bars = QJniObject::callStaticMethod<jint>("android/view/WindowInsets$Type", "systemBars");
                const int cutout = QJniObject::callStaticMethod<jint>("android/view/WindowInsets$Type", "displayCutout");
                const auto margins = insets.callObjectMethod("getInsets", "(I)Landroid/graphics/Insets;", bars | cutout);
                snapshot.left = margins.getField<jint>("left") / scale;
                snapshot.top = margins.getField<jint>("top") / scale;
                snapshot.right = margins.getField<jint>("right") / scale;
                snapshot.bottom = margins.getField<jint>("bottom") / scale;
            } else {
                snapshot.left = insets.callMethod<jint>("getSystemWindowInsetLeft") / scale;
                snapshot.top = insets.callMethod<jint>("getSystemWindowInsetTop") / scale;
                snapshot.right = insets.callMethod<jint>("getSystemWindowInsetRight") / scale;
                snapshot.bottom = insets.callMethod<jint>("getSystemWindowInsetBottom") / scale;
            }
            snapshot.resolved = true;
            return snapshot;
        }
    }
#endif

    auto *windowPrivate = QWindowPrivate::get(window);
    if (!windowPrivate || !windowPrivate->platformWindow)
        return snapshot;

    const QMargins margins = windowPrivate->platformWindow->safeAreaMargins();
    snapshot.left = margins.left();
    snapshot.top = margins.top();
    snapshot.right = margins.right();
    snapshot.bottom = margins.bottom();
    snapshot.resolved = true;
    return snapshot;
}

} // namespace

WindowSafeAreaObserver::WindowSafeAreaObserver(QObject *parent)
    : QObject(parent)
{
}

QObject *WindowSafeAreaObserver::window() const
{
    return m_window.data();
}

void WindowSafeAreaObserver::setWindow(QObject *windowObject)
{
    auto *quickWindow = qobject_cast<QWindow *>(windowObject);
    if (m_window == quickWindow)
        return;

    detachWindow();
    attachWindow(quickWindow);
    emit windowChanged();
    refresh();
}

qreal WindowSafeAreaObserver::leftInset() const
{
    return m_leftInset;
}

qreal WindowSafeAreaObserver::topInset() const
{
    return m_topInset;
}

qreal WindowSafeAreaObserver::rightInset() const
{
    return m_rightInset;
}

qreal WindowSafeAreaObserver::bottomInset() const
{
    return m_bottomInset;
}

bool WindowSafeAreaObserver::resolved() const
{
    return m_resolved;
}

void WindowSafeAreaObserver::refresh()
{
    updateMargins();
}

bool WindowSafeAreaObserver::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window) {
        switch (event->type()) {
        case QEvent::PlatformSurface:
        case QEvent::Show:
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::Expose:
        case QEvent::WindowStateChange:
        case QEvent::OrientationChange:
        case QEvent::ScreenChangeInternal:
        case QEvent::FocusIn:
            refresh();
            break;
        default:
            break;
        }
    }

    return QObject::eventFilter(watched, event);
}

void WindowSafeAreaObserver::attachWindow(QWindow *window)
{
    if (!window)
        return;

    m_window = window;
    m_window->installEventFilter(this);

    m_windowWidthChangedConnection = connect(m_window,
                                             &QWindow::widthChanged,
                                             this,
                                             &WindowSafeAreaObserver::refresh);
    m_windowHeightChangedConnection = connect(m_window,
                                              &QWindow::heightChanged,
                                              this,
                                              &WindowSafeAreaObserver::refresh);
    m_windowXChangedConnection = connect(m_window,
                                         &QWindow::xChanged,
                                         this,
                                         &WindowSafeAreaObserver::refresh);
    m_windowYChangedConnection = connect(m_window,
                                         &QWindow::yChanged,
                                         this,
                                         &WindowSafeAreaObserver::refresh);
    m_windowVisibleChangedConnection = connect(m_window,
                                               &QWindow::visibleChanged,
                                               this,
                                               &WindowSafeAreaObserver::refresh);
    m_windowScreenChangedConnection = connect(m_window,
                                              &QWindow::screenChanged,
                                              this,
                                              [this](QScreen *screen) {
                                                  attachScreen(screen);
                                                  refresh();
                                              });
    m_windowActiveChangedConnection = connect(m_window,
                                              &QWindow::activeChanged,
                                              this,
                                              &WindowSafeAreaObserver::refresh);
    m_windowDestroyedConnection = connect(m_window,
                                          &QObject::destroyed,
                                          this,
                                          [this]() {
                                              detachScreen();
                                              m_window.clear();
                                              updateMargins();
                                              emit windowChanged();
                                          });

    attachScreen(m_window->screen());
}

void WindowSafeAreaObserver::detachWindow()
{
    detachScreen();

    if (m_window)
        m_window->removeEventFilter(this);

    disconnect(m_windowWidthChangedConnection);
    disconnect(m_windowHeightChangedConnection);
    disconnect(m_windowXChangedConnection);
    disconnect(m_windowYChangedConnection);
    disconnect(m_windowVisibleChangedConnection);
    disconnect(m_windowScreenChangedConnection);
    disconnect(m_windowActiveChangedConnection);
    disconnect(m_windowDestroyedConnection);

    m_windowWidthChangedConnection = QMetaObject::Connection();
    m_windowHeightChangedConnection = QMetaObject::Connection();
    m_windowXChangedConnection = QMetaObject::Connection();
    m_windowYChangedConnection = QMetaObject::Connection();
    m_windowVisibleChangedConnection = QMetaObject::Connection();
    m_windowScreenChangedConnection = QMetaObject::Connection();
    m_windowActiveChangedConnection = QMetaObject::Connection();
    m_windowDestroyedConnection = QMetaObject::Connection();

    m_window.clear();
    updateMargins();
}

void WindowSafeAreaObserver::attachScreen(QScreen *screen)
{
    if (m_screen == screen)
        return;

    detachScreen();
    if (!screen)
        return;

    m_screen = screen;
    m_screenGeometryChangedConnection = connect(m_screen,
                                                &QScreen::geometryChanged,
                                                this,
                                                [this]() { refresh(); });
    m_screenAvailableGeometryChangedConnection = connect(m_screen,
                                                         &QScreen::availableGeometryChanged,
                                                         this,
                                                         [this]() { refresh(); });
    m_screenOrientationChangedConnection = connect(m_screen,
                                                   &QScreen::orientationChanged,
                                                   this,
                                                   [this]() { refresh(); });
}

void WindowSafeAreaObserver::detachScreen()
{
    disconnect(m_screenGeometryChangedConnection);
    disconnect(m_screenAvailableGeometryChangedConnection);
    disconnect(m_screenOrientationChangedConnection);

    m_screenGeometryChangedConnection = QMetaObject::Connection();
    m_screenAvailableGeometryChangedConnection = QMetaObject::Connection();
    m_screenOrientationChangedConnection = QMetaObject::Connection();

    m_screen.clear();
}

void WindowSafeAreaObserver::updateMargins()
{
    const SafeAreaSnapshot snapshot = querySafeAreaSnapshot(m_window);
    const bool marginsDidChange = snapshot.left != m_leftInset
        || snapshot.top != m_topInset
        || snapshot.right != m_rightInset
        || snapshot.bottom != m_bottomInset;
    const bool resolvedDidChange = snapshot.resolved != m_resolved;

    if (!marginsDidChange && !resolvedDidChange)
        return;

    m_leftInset = snapshot.left;
    m_topInset = snapshot.top;
    m_rightInset = snapshot.right;
    m_bottomInset = snapshot.bottom;
    m_resolved = snapshot.resolved;

    if (resolvedDidChange)
        emit resolvedChanged();
    emit marginsChanged();
}
