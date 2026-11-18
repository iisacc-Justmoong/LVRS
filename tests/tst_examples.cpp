#include "QtEventWait.h"
#include <QGuiApplication>
#include <QFileInfo>
#include <QScopedPointer>
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
public:
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

static QString findExampleFile(const char* relativePath)
{
    const auto path = QDir(QStringLiteral(LVRS_TEST_SOURCE_DIR)).filePath(QString::fromUtf8(relativePath));
    return QFileInfo(path).isFile() ? QDir::cleanPath(path) : QString{};
}

void ExampleSmokeTests::ios_hello_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = findExampleFile("../example/iOSHello/qml/Main.qml");
    CTEST_CHECK_MESSAGE(!path.isEmpty(), "Failed to locate ../example/iOSHello/qml/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    CTEST_CHECK(obj);
    CTEST_WAIT_DEFAULT(obj->property("exampleContractReady").toBool());
}

void ExampleSmokeTests::android_hello_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = findExampleFile("../example/AndroidHello/qml/Main.qml");
    CTEST_CHECK_MESSAGE(!path.isEmpty(), "Failed to locate ../example/AndroidHello/qml/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    CTEST_CHECK(obj);
    CTEST_WAIT_DEFAULT(obj->property("exampleContractReady").toBool());
}

void ExampleSmokeTests::mvvm_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = findExampleFile("../example/mvvm/qml/Main.qml");
    CTEST_CHECK_MESSAGE(!path.isEmpty(), "Failed to locate ../example/mvvm/qml/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    CTEST_CHECK(obj);
    CTEST_WAIT_DEFAULT(obj->property("viewportContractReady").toBool());
}

void ExampleSmokeTests::event_listener_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = findExampleFile("../example/EventListener/Main.qml");
    CTEST_CHECK_MESSAGE(!path.isEmpty(), "Failed to locate ../example/EventListener/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    CTEST_CHECK(obj);
    CTEST_WAIT_DEFAULT(obj->property("viewportContractReady").toBool());
}

void ExampleSmokeTests::typing_practice_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = findExampleFile("../example/TypingPractice/qml/Main.qml");
    CTEST_CHECK_MESSAGE(!path.isEmpty(), "Failed to locate ../example/TypingPractice/qml/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    CTEST_CHECK(obj);
    CTEST_WAIT_DEFAULT(obj->property("viewportContractReady").toBool());
}

void ExampleSmokeTests::visual_catalog_example_loads()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QString path = findExampleFile("../example/VisualCatalog/qml/Main.qml");
    CTEST_CHECK_MESSAGE(!path.isEmpty(), "Failed to locate ../example/VisualCatalog/qml/Main.qml");
    QScopedPointer<QObject> obj(loadFile(engine, path));
    CTEST_CHECK(obj);
    CTEST_WAIT_DEFAULT(obj->property("catalogViewportReady").toBool());
    CTEST_CHECK(obj->property("catalogSafeAreaEntryReady").toBool());
    CTEST_COMPARE(obj->property("catalogComponentCount").toInt(), 92);
    CTEST_CHECK(QMetaObject::invokeMethod(obj.data(), "activateCatalogEntry",
                                     Q_ARG(QVariant, QStringLiteral("sheet"))));
    CTEST_WAIT_COMPARE(obj->property("activeEntryKey").toString(), QStringLiteral("sheet"));
    CTEST_CHECK(QMetaObject::invokeMethod(obj.data(), "activateCatalogEntry",
                                     Q_ARG(QVariant, QStringLiteral("toggle-switch"))));
    CTEST_WAIT_DEFAULT(obj->findChild<QObject *>(QStringLiteral("catalogToggleOn")));
    QObject *slow = obj->findChild<QObject *>(QStringLiteral("catalogToggleSlow"));
    QObject *immediate = obj->findChild<QObject *>(QStringLiteral("catalogToggleImmediate"));
    CTEST_CHECK(slow && immediate);
    CTEST_COMPARE(slow->property("transitionDuration").toInt(), 640);
    CTEST_COMPARE(immediate->property("transitionDuration").toInt(), 0);
    CTEST_CHECK(QMetaObject::invokeMethod(obj.data(), "activateCatalogEntry",
                                     Q_ARG(QVariant, QStringLiteral("tooltip"))));
    CTEST_WAIT_DEFAULT(obj->findChild<QObject *>(QStringLiteral("galleryTooltip")));
    CTEST_CHECK(QMetaObject::invokeMethod(obj.data(), "activateCatalogEntry",
                                     Q_ARG(QVariant, QStringLiteral("window-material"))));
    CTEST_WAIT_DEFAULT(obj->findChild<QObject *>(QStringLiteral("galleryMaterialPopover")));
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    ExampleSmokeTests tests;
    runner.run("ios_hello_example_loads", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.ios_hello_example_loads(); });
    runner.run("android_hello_example_loads", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.android_hello_example_loads(); });
    runner.run("mvvm_example_loads", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.mvvm_example_loads(); });
    runner.run("event_listener_example_loads", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.event_listener_example_loads(); });
    runner.run("typing_practice_example_loads", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.typing_practice_example_loads(); });
    runner.run("visual_catalog_example_loads", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.visual_catalog_example_loads(); });
    return runner.result();
}
