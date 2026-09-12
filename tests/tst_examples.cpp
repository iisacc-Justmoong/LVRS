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
    void ios_hello_example_loads();
    void android_hello_example_loads();
    void mvvm_example_loads();
    void event_listener_example_loads();
    void typing_practice_example_loads();
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

void ExampleSmokeTests::ios_hello_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = QFINDTESTDATA("../example/iOSHello/qml/Main.qml");
    QVERIFY2(!path.isEmpty(), "Failed to locate ../example/iOSHello/qml/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    QVERIFY(obj);
    QTRY_VERIFY(obj->property("exampleContractReady").toBool());
}

void ExampleSmokeTests::android_hello_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = QFINDTESTDATA("../example/AndroidHello/qml/Main.qml");
    QVERIFY2(!path.isEmpty(), "Failed to locate ../example/AndroidHello/qml/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    QVERIFY(obj);
    QTRY_VERIFY(obj->property("exampleContractReady").toBool());
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
    QTRY_VERIFY(obj->property("viewportContractReady").toBool());
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
    QTRY_VERIFY(obj->property("viewportContractReady").toBool());
}

void ExampleSmokeTests::typing_practice_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = QFINDTESTDATA("../example/TypingPractice/qml/Main.qml");
    QVERIFY2(!path.isEmpty(), "Failed to locate ../example/TypingPractice/qml/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    QVERIFY(obj);
    QTRY_VERIFY(obj->property("viewportContractReady").toBool());
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
    QTRY_VERIFY(obj->property("catalogViewportReady").toBool());
    QVERIFY(obj->property("catalogSafeAreaEntryReady").toBool());
    QCOMPARE(obj->property("catalogComponentCount").toInt(), 84);
    QVERIFY(QMetaObject::invokeMethod(obj.data(), "activateCatalogEntry",
                                     Q_ARG(QVariant, QStringLiteral("sheet"))));
    QTRY_COMPARE(obj->property("activeEntryKey").toString(), QStringLiteral("sheet"));
    QVERIFY(QMetaObject::invokeMethod(obj.data(), "activateCatalogEntry",
                                     Q_ARG(QVariant, QStringLiteral("toggle-switch"))));
    QTRY_VERIFY(obj->findChild<QObject *>(QStringLiteral("catalogToggleOn")));
    QObject *slow = obj->findChild<QObject *>(QStringLiteral("catalogToggleSlow"));
    QObject *immediate = obj->findChild<QObject *>(QStringLiteral("catalogToggleImmediate"));
    QVERIFY(slow && immediate);
    QCOMPARE(slow->property("transitionDuration").toInt(), 640);
    QCOMPARE(immediate->property("transitionDuration").toInt(), 0);
    QVERIFY(QMetaObject::invokeMethod(obj.data(), "activateCatalogEntry",
                                     Q_ARG(QVariant, QStringLiteral("tooltip"))));
    QTRY_VERIFY(obj->findChild<QObject *>(QStringLiteral("galleryTooltip")));
    QVERIFY(QMetaObject::invokeMethod(obj.data(), "activateCatalogEntry",
                                     Q_ARG(QVariant, QStringLiteral("window-material"))));
    QTRY_VERIFY(obj->findChild<QObject *>(QStringLiteral("galleryMaterialPopover")));
}

QTEST_MAIN(ExampleSmokeTests)
#include "tst_examples.moc"
