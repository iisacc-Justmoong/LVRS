#include "QtEventWait.h"
#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlProperty>
#include <QQuickItem>
#include <QCoreApplication>
#include <QDir>
#include <QtPlugin>

#if defined(LVRS_USE_STATIC_QML_PLUGIN)
Q_IMPORT_PLUGIN(LVRSPlugin)
#endif

class LayoutTests : public QObject
{
public:
    void vstack_default_spacing();
    void hstack_default_spacing();
    void vstack_alignment_name();
    void hstack_alignment_name();
    void spacer_min_length_vertical();
};

static QObject *createFromQml(QQmlEngine &engine, const QByteArray &qml)
{
    QQmlComponent component(&engine);
    component.setData(qml, QUrl());
    QObject *obj = component.create();
    if (component.isError()) {
        const auto errors = component.errors();
        for (const auto &err : errors)
            qWarning() << err;
    }
    return obj;
}

void LayoutTests::vstack_default_spacing()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import QtQuick.Layouts
import LVRS 1.0 as LV

Item {
    LV.VStack {
        id: stack
        spacing: -1
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);

    const auto children = root->children().first()->children();
    QObject *layout = nullptr;
    for (QObject *child : children) {
        const QString cls = QString::fromLatin1(child->metaObject()->className());
        if (cls.contains("ColumnLayout")) {
            layout = child;
            break;
        }
    }
    CTEST_CHECK(layout);
    CTEST_COMPARE(layout->property("spacing").toInt(), 8);
}

void LayoutTests::hstack_default_spacing()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import QtQuick.Layouts
import LVRS 1.0 as LV

Item {
    LV.HStack {
        id: stack
        spacing: -1
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);

    const auto children = root->children().first()->children();
    QObject *layout = nullptr;
    for (QObject *child : children) {
        const QString cls = QString::fromLatin1(child->metaObject()->className());
        if (cls.contains("RowLayout")) {
            layout = child;
            break;
        }
    }
    CTEST_CHECK(layout);
    CTEST_COMPARE(layout->property("spacing").toInt(), 8);
}

void LayoutTests::vstack_alignment_name()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import QtQuick.Layouts
import LVRS 1.0 as LV

Item {
    property int childAlign: child.Layout.alignment

    LV.VStack {
        alignmentName: "leading"
        Rectangle {
            id: child
            width: 10; height: 10
        }
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    const int align = root->property("childAlign").toInt();
    CTEST_COMPARE(align, int(Qt::AlignLeft));
}

void LayoutTests::hstack_alignment_name()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import QtQuick.Layouts
import LVRS 1.0 as LV

Item {
    property int childAlign: child.Layout.alignment

    LV.HStack {
        alignmentName: "bottom"
        Rectangle {
            id: child
            width: 10; height: 10
        }
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    const int align = root->property("childAlign").toInt();
    CTEST_COMPARE(align, int(Qt::AlignBottom));
}

void LayoutTests::spacer_min_length_vertical()
{
    QQmlEngine engine;
    const QString importBase = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/..");
    engine.addImportPath(importBase);
    const QByteArray qml = R"(
import QtQuick
import QtQuick.Layouts
import LVRS 1.0 as LV

Item {
    property bool fillHeight: spacer.Layout.fillHeight
    property int minHeight: spacer.Layout.minimumHeight

    LV.VStack {
        LV.Spacer {
            id: spacer
            minLength: 12
        }
    }
}
)";

    QScopedPointer<QObject> root(createFromQml(engine, qml));
    CTEST_CHECK(root);
    CTEST_WAIT_DEFAULT(root->property("fillHeight").toBool());
    CTEST_WAIT_COMPARE(root->property("minHeight").toInt(), 12);
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner(argc == 2 ? argv[1] : "");
    LayoutTests tests;
    runner.run("vstack_default_spacing", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.vstack_default_spacing(); });
    runner.run("hstack_default_spacing", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.hstack_default_spacing(); });
    runner.run("vstack_alignment_name", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.vstack_alignment_name(); });
    runner.run("hstack_alignment_name", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.hstack_alignment_name(); });
    runner.run("spacer_min_length_vertical", [&] { lvrs_ctest::ScopedDeferredDeletes cleanup; tests.spacer_min_length_vertical(); });
    return runner.result();
}
