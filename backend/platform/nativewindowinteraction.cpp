#include "backend/platform/nativewindowinteraction.h"

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
    auto *window = qobject_cast<QWindow *>(windowObject);
    if (!window || !isValidResizeEdges(edges))
        return false;

    return window->startSystemResize(toResizeEdges(edges));
}
