#pragma once

#include <QObject>
#include <QtQml/qqml.h>

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
};
