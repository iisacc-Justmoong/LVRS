#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QDebug>
#include <memory>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport LVRS 1.0 as LV\nLV.VStack { width: 64; height: 64 }", QUrl());
    std::unique_ptr<QObject> object(component.create());
    if (!object) {
        qCritical() << component.errors();
        return 1;
    }
    return 0;
}
