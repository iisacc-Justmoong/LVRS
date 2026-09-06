#include <QtTest>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQmlEngine>
#include <QSignalSpy>

#include "test_utils.h"

class ListCompositeTests : public QObject
{
    Q_OBJECT

private slots:
    void figma_variants_data();
    void figma_variants();
    void mixed_list_uses_each_rows_height();
};

void ListCompositeTests::figma_variants_data()
{
    QTest::addColumn<int>("type");
    QTest::addColumn<int>("width");
    QTest::addColumn<int>("height");
    const char *names[] = {"Mini", "Detail", "Navigation", "Toggle", "Checkable",
                          "Action", "ActionGroup", "Stepper", "Select", "InlineEdit",
                          "DetailActions", "DetailQuantity", "DetailSettings", "Resource",
                          "Media", "Task", "Form"};
    const int widths[] = {170, 194, 280, 280, 280, 400, 400, 400, 400, 400,
                          400, 400, 400, 400, 400, 400, 400};
    const int heights[] = {22, 106, 44, 44, 44, 44, 44, 44, 44, 44,
                           120, 118, 159, 129, 113, 135, 148};
    for (int index = 0; index < 17; ++index)
        QTest::newRow(names[index]) << index << widths[index] << heights[index];
}

void ListCompositeTests::figma_variants()
{
    QFETCH(int, type);
    QFETCH(int, width);
    QFETCH(int, height);
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    const QByteArray qml = "import QtQuick\nimport LVRS as LV\nLV.ListItem { type: "
        + QByteArray::number(type) + " }";
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, qml));
    QVERIFY(object);
    QTRY_COMPARE(object->property("implicitWidth").toInt(), width);
    QTRY_COMPARE(object->property("implicitHeight").toInt(), height);
    QCOMPARE(object->property("size").toInt(), type);
}

void ListCompositeTests::mixed_list_uses_each_rows_height()
{
    QQmlEngine engine;
    engine.addImportPath(TestUtils::qmlImportBase());
    QScopedPointer<QObject> object(TestUtils::createFromQml(engine, R"(
import QtQuick
import LVRS as LV
LV.List {
    width: 400
    listWidth: 400
    minimumListHeight: 0
    footerVisible: false
    expandToContent: true
    items: [
        { type: "Mini", label: "First" },
        { type: "Form", label: "Editable", inputText1: "File name" },
        { type: "Navigation", label: "Next" },
        { type: "Media", label: "Audio" }
    ]
}
)"));
    QVERIFY(object);
    QTRY_COMPARE(object->property("contentHeight").toInt(), 327);
    QTRY_COMPARE(object->property("implicitHeight").toInt(), 327);
    const int heights[] = {22, 148, 44, 113};
    int top = 0;
    for (int i = 0; i < 4; ++i) {
        auto *row = object->findChild<QQuickItem *>(QStringLiteral("list_delegateRoot_%1").arg(i));
        QVERIFY(row);
        QTRY_COMPARE(qRound(row->y()), top);
        QTRY_COMPARE(qRound(row->height()), heights[i]);
        top += heights[i];
    }
}

QTEST_MAIN(ListCompositeTests)
#include "tst_list_composites.moc"
