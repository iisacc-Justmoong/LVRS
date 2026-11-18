#include "CTestSupport.h"
#include <QGuiApplication>

#include "backend/hierarchy/hierarchycodec.h"
#include "backend/hierarchy/hierarchyops.h"

class HierarchyCodecTests
{
public:
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

    CTEST_CHECK_MESSAGE(codec.parse(input, &nodes, &error), qPrintable(error));
    CTEST_COMPARE(nodes.size(), 2);
    CTEST_COMPARE(nodes.at(1).parentId, QStringLiteral("root"));
}

void HierarchyCodecTests::reparent_prevents_cycle()
{
    QVector<HierarchyNode> nodes = {
        {QStringLiteral("a"), QStringLiteral("A"), QString(), {}},
        {QStringLiteral("b"), QStringLiteral("B"), QStringLiteral("a"), {}},
        {QStringLiteral("c"), QStringLiteral("C"), QStringLiteral("b"), {}}
    };

    QString error;
    CTEST_CHECK(!HierarchyOps::reparent(&nodes, QStringLiteral("a"), QStringLiteral("c"), &error));
    CTEST_CHECK(error.contains(QStringLiteral("cycle")));
}

int main(int argc, char** argv)
{
    QGuiApplication application(argc, argv);
    lvrs_ctest::CTestRunner runner;
    HierarchyCodecTests tests;
    runner.run("parse_nodes_schema_succeeds", [&] { tests.parse_nodes_schema_succeeds(); });
    runner.run("reparent_prevents_cycle", [&] { tests.reparent_prevents_cycle(); });
    return runner.result();
}
