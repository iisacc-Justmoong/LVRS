#include "CTestSupport.h"
#include "QtEventWait.h"
#include <QGuiApplication>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlExpression>
#include <QDirIterator>
#include <QQuickItem>
#include <QQuickWindow>
#include "test_utils.h"

class CatalogTests : public QObject
{
public:
    void every_qml_type_has_a_recipe_and_live_preview();
    void search_and_reset();
    void native_catalog_capture();
};

static QObject *catalog(QQmlEngine &engine)
{
    engine.addImportPath(TestUtils::qmlImportBase());
    QQmlComponent component(&engine, QUrl::fromLocalFile(
        QStringLiteral(LVRS_TEST_SOURCE_DIR "/../example/VisualCatalog/qml/Main.qml")));
    auto *result = component.create();
    if (!result) qWarning() << component.errors();
    return result;
}

static QVariant evaluate(QQmlEngine &engine, QObject *scope, const QString &code)
{
    QQmlExpression expression(engine.rootContext(), scope, code);
    const QVariant result = expression.evaluate();
    if (expression.hasError()) qWarning() << expression.error();
    return result;
}

static void processFor(int milliseconds)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    timer.setTimerType(Qt::PreciseTimer);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(milliseconds);
    loop.exec();
}

void CatalogTests::every_qml_type_has_a_recipe_and_live_preview()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(catalog(engine));
    CTEST_CHECK(root);
    const auto keys = evaluate(engine, root.data(),
        "catalogEntries.map(function(entry) { return entry.key }).join('|')").toString().split('|');
    CTEST_CHECK(keys.contains(QStringLiteral("avatar")));
    CTEST_COMPARE(QSet<QString>(keys.begin(), keys.end()).size(), keys.size());
    const QString paths = evaluate(engine, root.data(),
        "catalogEntries.map(function(entry) { return entry.location }).join('|')").toString();
    const QDir source(QStringLiteral(LVRS_TEST_SOURCE_DIR "/.."));
    QDirIterator files(source.filePath("src/qml"), {"*.qml"}, QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const QString path = source.relativeFilePath(files.next());
        CTEST_CHECK_MESSAGE(paths.split('|').contains(path), qPrintable("Missing catalog type: " + path));
    }
    for (const QString &key : keys) {
        CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry", Q_ARG(QVariant, key)));
        CTEST_WAIT_COMPARE(root->property("activeEntryKey").toString(), key);
        QCoreApplication::processEvents();
        CTEST_WAIT_VERIFY(root->property("previewReady").toBool(), 5000);
        const bool guideComplete = evaluate(engine, root.data(),
            "activeMotionGuide !== null && activeMotionGuide.trigger.length > 20 "
            "&& activeMotionGuide.response.length > 20 && activeMotionGuide.observe.length > 20").toBool();
        CTEST_CHECK_MESSAGE(guideComplete, qPrintable("Missing interaction guide: " + key));
        CTEST_CHECK_MESSAGE(root->property("previewError").toString().isEmpty(), qPrintable(key));
    }
}

void CatalogTests::search_and_reset()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(catalog(engine));
    CTEST_CHECK(root);
    CTEST_CHECK(root->setProperty("searchQuery", "slider"));
    CTEST_CHECK(evaluate(engine, root.data(),
        "filteredCatalogRows.some(function(row) { return row.key === 'slider' })").toBool());
    CTEST_CHECK(root->setProperty("searchQuery", "no-match-1234"));
    CTEST_COMPARE(evaluate(engine, root.data(), "filteredCatalogRows.length").toInt(), 0);
    CTEST_CHECK(root->setProperty("searchQuery", ""));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry", Q_ARG(QVariant, QStringLiteral("motion"))));
    QCoreApplication::processEvents();
    CTEST_WAIT_DEFAULT(root->property("previewReady").toBool());
    auto *loader = root->findChild<QObject *>("catalogPreviewLoader");
    CTEST_CHECK(loader);
    auto *item = qvariant_cast<QObject *>(loader->property("item"));
    CTEST_CHECK(item);
    CTEST_CHECK(item->setProperty("actionCount", 8));
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "resetPreview"));
    QCoreApplication::processEvents();
    CTEST_WAIT_DEFAULT(root->property("previewReady").toBool());
    item = qvariant_cast<QObject *>(loader->property("item"));
    CTEST_CHECK(item);
    CTEST_COMPARE(item->property("actionCount").toInt(), 0);
}

void CatalogTests::native_catalog_capture()
{
    const QString directory = qEnvironmentVariable("LVRS_CATALOG_CAPTURE_DIR");
    if (directory.isEmpty())
        lvrs_ctest::skip("Set LVRS_CATALOG_CAPTURE_DIR for native Metal catalog screenshots.");
    CTEST_CHECK(QDir().mkpath(directory));
    QQmlEngine engine;
    QScopedPointer<QObject> root(catalog(engine));
    CTEST_CHECK(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    CTEST_CHECK(window);
    CTEST_CHECK(lvrs_ctest::waitFor([&] { return window->isExposed(); }, 5000));
    for (const QString &key : {QStringLiteral("help-button"), QStringLiteral("color-picker-button"), QStringLiteral("context-menu-item"), QStringLiteral("menu")}) {
        CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry", Q_ARG(QVariant, key)));
        QCoreApplication::processEvents();
        CTEST_WAIT_DEFAULT(root->property("previewReady").toBool());
        processFor(600);
        const auto frame = window->grabWindow();
        CTEST_CHECK(!frame.isNull());
        CTEST_CHECK(frame.save(directory + "/" + key + ".png"));
    }
    window->resize(1024, 760);
    CTEST_CHECK(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry", Q_ARG(QVariant, QStringLiteral("check-box"))));
    processFor(600);
    CTEST_CHECK(window->grabWindow().save(directory + "/compact.png"));
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    CatalogTests tests;
    runner.run("every_qml_type_has_a_recipe_and_live_preview", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.every_qml_type_has_a_recipe_and_live_preview(); });
    runner.run("search_and_reset", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.search_and_reset(); });
    runner.run("native_catalog_capture", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.native_catalog_capture(); });
    return runner.result();
}
