#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QDebug>

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import LVRS 1.0\nEventListener {}", QUrl());
    const auto object = component.create();
    if (!object) qCritical() << component.errors();
    delete object;
    return object ? 0 : 1;
}
