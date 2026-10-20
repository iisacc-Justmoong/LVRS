#include <QtTest>

#include "backend/hierarchy/hierarchycommandstack.h"
#include "backend/hierarchy/hierarchyvisibility.h"

class HierarchyInteractionTests : public QObject
{
    Q_OBJECT

private slots:
    void move_intent_before_after_child_root();
    void visibility_projection_respects_expanded_state();
    void visibility_projection_matches_reference();
    void visibility_projection_handles_deep_and_cyclic_branches();
    void undo_redo_roundtrip();
};

void HierarchyInteractionTests::move_intent_before_after_child_root()
{
    QVector<HierarchyNode> nodes = {
        {QStringLiteral("a"), QStringLiteral("A"), QString(), {}},
        {QStringLiteral("b"), QStringLiteral("B"), QString(), {}},
        {QStringLiteral("c"), QStringLiteral("C"), QString(), {}}
    };

    QString error;
    QVERIFY(HierarchyController::applyMove(&nodes, {QStringLiteral("c"), QStringLiteral("a"), HierarchyDropMode::Before}, &error));
    QCOMPARE(nodes.at(0).id, QStringLiteral("c"));

    QVERIFY(HierarchyController::applyMove(&nodes, {QStringLiteral("c"), QStringLiteral("b"), HierarchyDropMode::After}, &error));
    QCOMPARE(nodes.at(2).id, QStringLiteral("c"));

    QVERIFY(HierarchyController::applyMove(&nodes, {QStringLiteral("c"), QStringLiteral("a"), HierarchyDropMode::Child}, &error));
    QCOMPARE(nodes.at(1).parentId, QStringLiteral("a"));

    QVERIFY(HierarchyController::applyMove(&nodes, {QStringLiteral("c"), QString(), HierarchyDropMode::Root}, &error));
    QVERIFY(nodes.at(2).parentId.isEmpty());
}

void HierarchyInteractionTests::visibility_projection_respects_expanded_state()
{
    QVector<HierarchyNode> nodes = {
        {QStringLiteral("root"), QStringLiteral("Root"), QString(), {}},
        {QStringLiteral("child"), QStringLiteral("Child"), QStringLiteral("root"), {}},
        {QStringLiteral("leaf"), QStringLiteral("Leaf"), QStringLiteral("child"), {}}
    };

    {
        const QVector<HierarchyVisibleRow> rows = HierarchyVisibility::project(nodes, {});
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows.at(0).node.id, QStringLiteral("root"));
    }

    {
        const QVector<HierarchyVisibleRow> rows = HierarchyVisibility::project(nodes, {QStringLiteral("root")});
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows.at(1).node.id, QStringLiteral("child"));
    }
}

void HierarchyInteractionTests::visibility_projection_matches_reference()
{
    QVector<HierarchyNode> nodes;
    for (int i=0;i<100;++i) {
        const auto id=QString::number(i);
        nodes.push_back({id,id,i ? QString::number((i-1)/3) : QString{}, {}});
    }
    nodes.push_back({QStringLiteral("orphan"),{},QStringLiteral("missing"),{}});
    nodes.push_back({QStringLiteral("duplicate"),{},QStringLiteral("0"),{}});
    nodes.push_back({QStringLiteral("duplicate"),{},QStringLiteral("1"),{}});
    std::reverse(nodes.begin(),nodes.end());
    for (int pattern=0;pattern<8;++pattern) {
        QSet<QString> expanded;
        for (int i=0;i<100;++i) if (pattern==0 || i%(pattern+1)!=0) expanded.insert(QString::number(i));
        if (pattern%2) expanded.insert(QStringLiteral("missing"));
        QHash<QString,QString> parents;
        QHash<QString,int> depths;
        QSet<QString> hasChildren;
        for (const auto& n:nodes) {parents.insert(n.id,n.parentId);if (!n.parentId.isEmpty()) hasChildren.insert(n.parentId);}
        for (const auto& n:nodes) {
            int depth=0;auto cursor=n.parentId;
            while (!cursor.isEmpty()) {++depth;cursor=parents.value(cursor);}
            depths.insert(n.id,depth);
        }
        QVector<HierarchyVisibleRow> expected;
        for (const auto& n:nodes) {
            bool visible=true;auto cursor=n.parentId;
            while (!cursor.isEmpty()) {if (!expanded.contains(cursor)) {visible=false;break;}cursor=parents.value(cursor);}
            if (visible) expected.push_back({n,depths.value(n.id),hasChildren.contains(n.id)});
        }
        const auto actual=HierarchyVisibility::project(nodes,expanded);
        QCOMPARE(actual.size(),expected.size());
        for (qsizetype i=0;i<actual.size();++i) {
            QCOMPARE(actual[i].node.id,expected[i].node.id);
            QCOMPARE(actual[i].node.parentId,expected[i].node.parentId);
            QCOMPARE(actual[i].depth,expected[i].depth);
            QCOMPARE(actual[i].hasChildren,expected[i].hasChildren);
        }
    }
}

void HierarchyInteractionTests::visibility_projection_handles_deep_and_cyclic_branches()
{
    QVector<HierarchyNode> nodes;
    QSet<QString> expanded;
    for (int i=0;i<10000;++i) {
        const auto id=QString::number(i);
        nodes.push_back({id,id,i ? QString::number(i-1) : QString{}, {}});
        expanded.insert(id);
    }
    std::reverse(nodes.begin(),nodes.end());
    const auto rows=HierarchyVisibility::project(nodes,expanded);
    QCOMPARE(rows.size(),nodes.size());
    QCOMPARE(rows.first().depth,9999);
    QCOMPARE(rows.last().depth,0);
    nodes={{"a",{},"b",{}},{"b",{},"a",{}},{"root",{},{},{}}};
    const auto safe=HierarchyVisibility::project(nodes,{"a","b"});
    QCOMPARE(safe.size(),1);
    QCOMPARE(safe.first().node.id,QStringLiteral("root"));
}

void HierarchyInteractionTests::undo_redo_roundtrip()
{
    QVector<HierarchyNode> nodes = {
        {QStringLiteral("a"), QStringLiteral("A"), QString(), {}},
        {QStringLiteral("b"), QStringLiteral("B"), QString(), {}}
    };

    HierarchyCommandStack stack;
    QString error;
    QVERIFY(stack.applyMove(&nodes, {QStringLiteral("b"), QStringLiteral("a"), HierarchyDropMode::Before}, &error));
    QCOMPARE(nodes.at(0).id, QStringLiteral("b"));

    QVERIFY(stack.undo(&nodes, &error));
    QCOMPARE(nodes.at(0).id, QStringLiteral("a"));

    QVERIFY(stack.redo(&nodes, &error));
    QCOMPARE(nodes.at(0).id, QStringLiteral("b"));
}

QTEST_MAIN(HierarchyInteractionTests)
#include "tst_hierarchy_interaction.moc"
