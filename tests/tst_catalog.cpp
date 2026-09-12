#include <QtTest>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlExpression>
#include <QDirIterator>
#include <QQuickItem>
#include <QQuickWindow>
#include "test_utils.h"

class CatalogTests : public QObject
{
    Q_OBJECT
private slots:
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

void CatalogTests::every_qml_type_has_a_recipe_and_live_preview()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(catalog(engine));
    QVERIFY(root);
    const auto keys = evaluate(engine, root.data(),
        "catalogEntries.map(function(entry) { return entry.key }).join('|')").toString().split('|');
    QCOMPARE(keys.size(), 84);
    const QString paths = evaluate(engine, root.data(),
        "catalogEntries.map(function(entry) { return entry.location }).join('|')").toString();
    const QDir source(QStringLiteral(LVRS_TEST_SOURCE_DIR "/.."));
    QDirIterator files(source.filePath("qml"), {"*.qml"}, QDir::Files, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const QString path = source.relativeFilePath(files.next());
        QVERIFY2(paths.split('|').contains(path), qPrintable("Missing catalog type: " + path));
    }
    for (const QString &key : keys) {
        QVERIFY(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry", Q_ARG(QVariant, key)));
        QTRY_COMPARE(root->property("activeEntryKey").toString(), key);
        QCoreApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT(root->property("previewReady").toBool(), 5000);
        const bool guideComplete = evaluate(engine, root.data(),
            "activeMotionGuide !== null && activeMotionGuide.trigger.length > 20 "
            "&& activeMotionGuide.response.length > 20 && activeMotionGuide.observe.length > 20").toBool();
        QVERIFY2(guideComplete, qPrintable("Missing interaction guide: " + key));
        QVERIFY2(root->property("previewError").toString().isEmpty(), qPrintable(key));
    }
}

void CatalogTests::search_and_reset()
{
    QQmlEngine engine;
    QScopedPointer<QObject> root(catalog(engine));
    QVERIFY(root);
    QVERIFY(root->setProperty("searchQuery", "slider"));
    QVERIFY(evaluate(engine, root.data(),
        "filteredCatalogRows.some(function(row) { return row.key === 'slider' })").toBool());
    QVERIFY(root->setProperty("searchQuery", "no-match-1234"));
    QCOMPARE(evaluate(engine, root.data(), "filteredCatalogRows.length").toInt(), 0);
    QVERIFY(root->setProperty("searchQuery", ""));
    QVERIFY(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry", Q_ARG(QVariant, QStringLiteral("motion"))));
    QCoreApplication::processEvents();
    QTRY_VERIFY(root->property("previewReady").toBool());
    auto *loader = root->findChild<QObject *>("catalogPreviewLoader");
    QVERIFY(loader);
    auto *item = qvariant_cast<QObject *>(loader->property("item"));
    QVERIFY(item);
    QVERIFY(item->setProperty("actionCount", 8));
    QVERIFY(QMetaObject::invokeMethod(root.data(), "resetPreview"));
    QCoreApplication::processEvents();
    QTRY_VERIFY(root->property("previewReady").toBool());
    item = qvariant_cast<QObject *>(loader->property("item"));
    QVERIFY(item);
    QCOMPARE(item->property("actionCount").toInt(), 0);
}

void CatalogTests::native_catalog_capture()
{
    const QString directory = qEnvironmentVariable("LVRS_CATALOG_CAPTURE_DIR");
    if (directory.isEmpty())
        QSKIP("Set LVRS_CATALOG_CAPTURE_DIR for native Metal catalog screenshots.");
    QVERIFY(QDir().mkpath(directory));
    QQmlEngine engine;
    QScopedPointer<QObject> root(catalog(engine));
    QVERIFY(root);
    auto *window = qobject_cast<QQuickWindow *>(root.data());
    QVERIFY(window);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    for (const QString &key : {QStringLiteral("help-button"), QStringLiteral("color-picker-button"), QStringLiteral("context-menu-item"), QStringLiteral("menu")}) {
        QVERIFY(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry", Q_ARG(QVariant, key)));
        QCoreApplication::processEvents();
        QTRY_VERIFY(root->property("previewReady").toBool());
        QTest::qWait(600);
        const auto frame = window->grabWindow();
        QVERIFY(!frame.isNull());
        QVERIFY(frame.save(directory + "/" + key + ".png"));
    }
    window->resize(1024, 760);
    QVERIFY(QMetaObject::invokeMethod(root.data(), "activateCatalogEntry", Q_ARG(QVariant, QStringLiteral("check-box"))));
    QTest::qWait(600);
    QVERIFY(window->grabWindow().save(directory + "/compact.png"));
}

QTEST_MAIN(CatalogTests)
#include "tst_catalog.moc"
