#include <QtTest>

#include "backend/hierarchy/hierarchycodec.h"
#include "backend/hierarchy/hierarchyops.h"

class HierarchyCodecTests : public QObject
{
    Q_OBJECT

private slots:
    void parse_nodes_schema_succeeds();
    void reparent_prevents_cycle();
};

void HierarchyCodecTests::parse_nodes_schema_succeeds()
{
    HierarchyCodec codec;
    QVector<HierarchyNode> nodes;
    QString error;

    const QString input = QStringLiteral(R"JSON({
        "schema": "lvrs.hierarchy.v1",
        "nodes": [
            { "id": "root", "label": "Root" },
            { "id": "child", "label": "Child", "parentId": "root" }
        ]
    })JSON");

    QVERIFY2(codec.parse(input, &nodes, &error), qPrintable(error));
    QCOMPARE(nodes.size(), 2);
    QCOMPARE(nodes.at(1).parentId, QStringLiteral("root"));
}

void HierarchyCodecTests::reparent_prevents_cycle()
{
    QVector<HierarchyNode> nodes = {
        {QStringLiteral("a"), QStringLiteral("A"), QString(), {}},
        {QStringLiteral("b"), QStringLiteral("B"), QStringLiteral("a"), {}},
        {QStringLiteral("c"), QStringLiteral("C"), QStringLiteral("b"), {}}
    };

    QString error;
    QVERIFY(!HierarchyOps::reparent(&nodes, QStringLiteral("a"), QStringLiteral("c"), &error));
    QVERIFY(error.contains(QStringLiteral("cycle")));
}

QTEST_MAIN(HierarchyCodecTests)
#include "tst_hierarchy_codec.moc"
