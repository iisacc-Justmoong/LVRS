#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QCoreApplication>
#include <QDir>
#include <QtPlugin>

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class ExampleSmokeTests : public QObject
{
    Q_OBJECT

private slots:
    void mvvm_example_loads();
    void event_listener_example_loads();
    void visual_catalog_example_loads();
};

static QObject *loadFile(QQmlEngine &engine, const QString &path)
{
    QQmlComponent component(&engine, QUrl::fromLocalFile(path));
    QObject *obj = component.create();
    if (component.isError()) {
        const auto errors = component.errors();
        for (const auto &err : errors)
            qWarning() << err;
    }
    return obj;
}

void ExampleSmokeTests::mvvm_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = QFINDTESTDATA("../example/mvvm/qml/Main.qml");
    QVERIFY2(!path.isEmpty(), "Failed to locate ../example/mvvm/qml/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    QVERIFY(obj);
}

void ExampleSmokeTests::event_listener_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = QFINDTESTDATA("../example/EventListener/Main.qml");
    QVERIFY2(!path.isEmpty(), "Failed to locate ../example/EventListener/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    QVERIFY(obj);
}

void ExampleSmokeTests::visual_catalog_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = QFINDTESTDATA("../example/VisualCatalog/qml/Main.qml");
    QVERIFY2(!path.isEmpty(), "Failed to locate ../example/VisualCatalog/qml/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    QVERIFY(obj);
    QCOMPARE(obj->property("catalogComponentCount").toInt(), 53);
}

QTEST_MAIN(ExampleSmokeTests)
#include "tst_examples.moc"
