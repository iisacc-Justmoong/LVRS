#include <QtTest>

#include "backend/hierarchy/hierarchycommandstack.h"
#include "backend/hierarchy/hierarchyvisibility.h"

class HierarchyInteractionTests : public QObject
{
    Q_OBJECT

private slots:
    void move_intent_before_after_child_root();
    void visibility_projection_respects_expanded_state();
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
