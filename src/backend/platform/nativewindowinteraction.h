#pragma once

#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QtQml/qqml.h>

class QEvent;
class QWindow;

class NativeWindowInteraction : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(NativeWindowInteraction)
    QML_SINGLETON

public:
    explicit NativeWindowInteraction(QObject *parent = nullptr);

    Q_INVOKABLE bool isValidResizeEdges(int edges) const;
    Q_INVOKABLE bool requestSystemMove(QObject *windowObject);
    Q_INVOKABLE bool requestSystemResize(QObject *windowObject, int edges);
    Q_INVOKABLE bool requestSystemResizeAt(QObject *windowObject,
                                           int edges,
                                           const QPointF &globalPosition);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool beginManualResize(QWindow *window,
                           Qt::Edges edges,
                           const QPoint &globalPosition);
    void updateManualResize(const QPoint &globalPosition);
    void finishManualResize();

    QPointer<QWindow> m_manualResizeWindow;
    QRect m_manualResizeInitialGeometry;
    QPoint m_manualResizeInitialPointer;
    Qt::Edges m_manualResizeEdges;
};
