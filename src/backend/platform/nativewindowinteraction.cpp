#include "backend/platform/nativewindowinteraction.h"

#include <QCoreApplication>
#include <QCursor>
#include <QEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QWindow>

namespace {
constexpr int kKnownResizeEdges = int(Qt::LeftEdge)
    | int(Qt::TopEdge)
    | int(Qt::RightEdge)
    | int(Qt::BottomEdge);

int edgeCount(int edges)
{
    int count = 0;
    count += (edges & int(Qt::LeftEdge)) != 0 ? 1 : 0;
    count += (edges & int(Qt::TopEdge)) != 0 ? 1 : 0;
    count += (edges & int(Qt::RightEdge)) != 0 ? 1 : 0;
    count += (edges & int(Qt::BottomEdge)) != 0 ? 1 : 0;
    return count;
}

Qt::Edges toResizeEdges(int edges)
{
    Qt::Edges result;
    if ((edges & int(Qt::LeftEdge)) != 0)
        result |= Qt::LeftEdge;
    if ((edges & int(Qt::TopEdge)) != 0)
        result |= Qt::TopEdge;
    if ((edges & int(Qt::RightEdge)) != 0)
        result |= Qt::RightEdge;
    if ((edges & int(Qt::BottomEdge)) != 0)
        result |= Qt::BottomEdge;
    return result;
}

void constrainMovingEdge(int &movingEdge,
                         int fixedEdge,
                         int minimumLength,
                         int maximumLength,
                         bool movingStartEdge)
{
    const int requestedLength = movingStartEdge
        ? fixedEdge - movingEdge
        : movingEdge - fixedEdge;
    const int constrainedLength = qBound(minimumLength, requestedLength, maximumLength);
    movingEdge = movingStartEdge
        ? fixedEdge - constrainedLength
        : fixedEdge + constrainedLength;
}
}

NativeWindowInteraction::NativeWindowInteraction(QObject *parent)
    : QObject(parent)
{
}

bool NativeWindowInteraction::isValidResizeEdges(int edges) const
{
    if (edges == 0 || (edges & ~kKnownResizeEdges) != 0)
        return false;

    const int count = edgeCount(edges);
    if (count == 1)
        return true;
    if (count != 2)
        return false;

    const bool hasHorizontalEdge = (edges & (int(Qt::LeftEdge) | int(Qt::RightEdge))) != 0;
    const bool hasVerticalEdge = (edges & (int(Qt::TopEdge) | int(Qt::BottomEdge))) != 0;
    const bool hasOpposingHorizontalEdges = (edges & int(Qt::LeftEdge)) != 0
        && (edges & int(Qt::RightEdge)) != 0;
    const bool hasOpposingVerticalEdges = (edges & int(Qt::TopEdge)) != 0
        && (edges & int(Qt::BottomEdge)) != 0;

    return hasHorizontalEdge
        && hasVerticalEdge
        && !hasOpposingHorizontalEdges
        && !hasOpposingVerticalEdges;
}

bool NativeWindowInteraction::requestSystemMove(QObject *windowObject)
{
    auto *window = qobject_cast<QWindow *>(windowObject);
    return window && window->startSystemMove();
}

bool NativeWindowInteraction::requestSystemResize(QObject *windowObject, int edges)
{
    return requestSystemResizeAt(windowObject, edges, QCursor::pos());
}

bool NativeWindowInteraction::requestSystemResizeAt(QObject *windowObject,
                                                     int edges,
                                                     const QPointF &globalPosition)
{
    auto *window = qobject_cast<QWindow *>(windowObject);
    if (!window || !isValidResizeEdges(edges))
        return false;

    const Qt::Edges resizeEdges = toResizeEdges(edges);
    finishManualResize();
    if (window->startSystemResize(resizeEdges))
        return true;

#if defined(Q_OS_MACOS)
    return beginManualResize(window, resizeEdges, globalPosition.toPoint());
#else
    Q_UNUSED(globalPosition)
    return false;
#endif
}

bool NativeWindowInteraction::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_manualResizeWindow) {
        finishManualResize();
        return QObject::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::MouseMove: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->buttons().testFlag(Qt::LeftButton))
            updateManualResize(mouseEvent->globalPosition().toPoint());
        else
            finishManualResize();
        break;
    }
    case QEvent::MouseButtonRelease: {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            updateManualResize(mouseEvent->globalPosition().toPoint());
            finishManualResize();
        }
        break;
    }
    case QEvent::UngrabMouse:
    case QEvent::ApplicationDeactivate:
        finishManualResize();
        break;
    case QEvent::Hide:
    case QEvent::Close:
    case QEvent::Destroy:
        if (watched == m_manualResizeWindow)
            finishManualResize();
        break;
    default:
        break;
    }

    return QObject::eventFilter(watched, event);
}

bool NativeWindowInteraction::beginManualResize(QWindow *window,
                                                Qt::Edges edges,
                                                const QPoint &globalPosition)
{
#if defined(Q_OS_MACOS)
    auto *application = QGuiApplication::instance();
    if (!application || !window || !window->isVisible())
        return false;

    const bool horizontalResizeRequested = edges.testFlag(Qt::LeftEdge)
        || edges.testFlag(Qt::RightEdge);
    const bool verticalResizeRequested = edges.testFlag(Qt::TopEdge)
        || edges.testFlag(Qt::BottomEdge);
    const bool horizontalResizeAvailable = window->minimumWidth() < window->maximumWidth();
    const bool verticalResizeAvailable = window->minimumHeight() < window->maximumHeight();
    if ((!horizontalResizeRequested || !horizontalResizeAvailable)
        && (!verticalResizeRequested || !verticalResizeAvailable)) {
        return false;
    }

    m_manualResizeWindow = window;
    m_manualResizeInitialGeometry = window->geometry();
    m_manualResizeInitialPointer = globalPosition;
    m_manualResizeEdges = edges;
    application->installEventFilter(this);
    return true;
#else
    Q_UNUSED(window)
    Q_UNUSED(edges)
    Q_UNUSED(globalPosition)
    return false;
#endif
}

void NativeWindowInteraction::updateManualResize(const QPoint &globalPosition)
{
    if (!m_manualResizeWindow)
        return;

    const QPoint delta = globalPosition - m_manualResizeInitialPointer;
    int left = m_manualResizeInitialGeometry.left();
    int top = m_manualResizeInitialGeometry.top();
    int right = left + m_manualResizeInitialGeometry.width();
    int bottom = top + m_manualResizeInitialGeometry.height();

    if (m_manualResizeEdges.testFlag(Qt::LeftEdge))
        left += delta.x();
    if (m_manualResizeEdges.testFlag(Qt::TopEdge))
        top += delta.y();
    if (m_manualResizeEdges.testFlag(Qt::RightEdge))
        right += delta.x();
    if (m_manualResizeEdges.testFlag(Qt::BottomEdge))
        bottom += delta.y();

    const int minimumWidth = qMax(1, m_manualResizeWindow->minimumWidth());
    const int minimumHeight = qMax(1, m_manualResizeWindow->minimumHeight());
    const int maximumWidth = qMax(minimumWidth, m_manualResizeWindow->maximumWidth());
    const int maximumHeight = qMax(minimumHeight, m_manualResizeWindow->maximumHeight());

    if (m_manualResizeEdges.testFlag(Qt::LeftEdge))
        constrainMovingEdge(left, right, minimumWidth, maximumWidth, true);
    else if (m_manualResizeEdges.testFlag(Qt::RightEdge))
        constrainMovingEdge(right, left, minimumWidth, maximumWidth, false);

    if (m_manualResizeEdges.testFlag(Qt::TopEdge))
        constrainMovingEdge(top, bottom, minimumHeight, maximumHeight, true);
    else if (m_manualResizeEdges.testFlag(Qt::BottomEdge))
        constrainMovingEdge(bottom, top, minimumHeight, maximumHeight, false);

    m_manualResizeWindow->setGeometry(left, top, right - left, bottom - top);
}

void NativeWindowInteraction::finishManualResize()
{
    if (auto *application = QCoreApplication::instance())
        application->removeEventFilter(this);

    m_manualResizeWindow.clear();
    m_manualResizeInitialGeometry = QRect();
    m_manualResizeInitialPointer = QPoint();
    m_manualResizeEdges = Qt::Edges();
}
