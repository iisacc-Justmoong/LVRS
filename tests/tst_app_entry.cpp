#include <QtTest>

#include <QDir>
#include <QTemporaryDir>

#include "backend/runtime/appentry.h"

class AppEntryTests : public QObject
{
    Q_OBJECT

private slots:
    void default_runtime_qml_import_paths_include_staged_bundle_dirs();
    void default_runtime_qml_import_paths_include_snapshot_platform_dirs();
};

void AppEntryTests::default_runtime_qml_import_paths_include_staged_bundle_dirs()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString rootDir = tempDir.path();
    const QString appDir = QDir(rootDir).filePath(QStringLiteral("bin"));
    const QString bundledQmlDir = QDir(appDir).filePath(QStringLiteral("lvrs-runtime/qml/LVRS"));
    const QString installedQmlDir = QDir(appDir).filePath(QStringLiteral("../lib/qt6/qml/LVRS"));

    QVERIFY(QDir().mkpath(bundledQmlDir));
    QVERIFY(QDir().mkpath(installedQmlDir));

    const QStringList importPaths = lvrs::defaultRuntimeQmlImportPaths(appDir);
    QCOMPARE(importPaths,
             QStringList({
                 QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("lvrs-runtime/qml"))),
                 QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("../lib/qt6/qml")))
             }));
}

void AppEntryTests::default_runtime_qml_import_paths_include_snapshot_platform_dirs()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const QString installRoot = QDir(tempDir.path()).filePath(QStringLiteral("prefix"));
    const QString appDir = QDir(installRoot).filePath(QStringLiteral("src/LVRS/example/VisualCatalog/bin"));
    const QString snapshotQmlDir =
        QDir(installRoot).filePath(QStringLiteral("platforms/linux/lib/qt6/qml/LVRS"));

    QVERIFY(QDir().mkpath(appDir));
    QVERIFY(QDir().mkpath(snapshotQmlDir));

    const QStringList importPaths = lvrs::defaultRuntimeQmlImportPaths(appDir);
    QCOMPARE(importPaths,
             QStringList({
                 QDir::cleanPath(
                     QDir(appDir).filePath(QStringLiteral("../../../../../platforms/linux/lib/qt6/qml")))
             }));
}

QTEST_MAIN(AppEntryTests)
#include "tst_app_entry.moc"
