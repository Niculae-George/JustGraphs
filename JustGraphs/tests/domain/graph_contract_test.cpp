#include "domain/directedgraph.h"
#include "domain/undirectedgraph.h"
#include <QCoreApplication>
#include <type_traits>
#include <QTextStream>

namespace { int failures = 0; void check(bool ok, const char* what) { if (!ok) { QTextStream(stderr) << "FAIL: " << what << '\n'; ++failures; } } }

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    static_assert(!std::is_copy_constructible_v<DirectedGraph>);
    static_assert(!std::is_move_constructible_v<DirectedGraph>);
    static_assert(std::is_same_v<decltype(std::declval<const Graph&>().getNodes()), const std::list<Node>&>);
    static_assert(std::is_same_v<decltype(std::declval<const Edge&>().getFirst()), const Node&>);

    DirectedGraph g;
    int a = g.addNode({0, 0}), b = g.addNode({1, 0}), c = g.addNode({2, 0});
    check(a == 1 && b == 2 && c == 3, "new nodes have positive stable IDs");
    check(g.addEdge(a, b), "valid edge accepted");
    check(!g.addEdge(a, b), "duplicate edge rejected");
    check(!g.addEdge(a, a), "self edge rejected");
    Node foreign(b, {0, 0});
    check(!g.addEdge(foreign, *g.findNode(c)), "foreign Node object rejected");
    check(g.addEdge(b, c), "second edge accepted");
    check(g.removeNode(b), "middle node removed");
    check(g.getEdges().empty(), "incident edges removed before node lifetime ends");
    check(g.addNode({3, 0}) == 4, "ID is not reused after deletion");
    check(g.removeNode(4) && g.addNode({4, 0}) == 5, "deleting the highest ID cannot retarget a stale handle");
    check(g.addNodeWithId(20, {0, 1}), "imported ID accepted");
    check(!g.addNodeWithId(20, {0, 2}), "duplicate imported ID rejected");
    check(!g.addNodeWithId(0, {0, 2}), "non-positive imported ID rejected");
    check(g.setNodePosition(a, {9, 9}) && g.findNode(a)->getPos() == QPoint(9, 9), "position mutation by ID");
    check(!g.setNodePosition(999, {0, 0}), "missing position ID rejected");
    check(g.graphKind() == GraphKind::Directed, "typed directed kind");

    UndirectedGraph u;
    const int x = u.addNode({}), y = u.addNode({});
    check(u.graphKind() == GraphKind::Undirected, "typed undirected kind");
    check(u.addEdge(x, y), "undirected edge accepted");
    check(!u.addEdge(y, x), "reverse duplicate undirected edge rejected");
    check(u.removeEdge(y, x) && u.getEdges().empty(), "undirected removal accepts reverse orientation");
    check(!u.removeEdge(x, y), "missing edge removal is a no-op");
    DirectedGraph d;
    d.addNode({}); d.addNode({});
    d.addEdge(1, 2); d.addEdge(2, 1);
    check(d.removeEdge(1, 2) && d.getEdges().size() == 1, "directed removal leaves opposite arc");
    check(d.getEdges().front().getFirst().getIndex() == 2, "opposite direction remains intact");
    check(!d.removeEdge(1, 2) && !d.removeEdge(0, 2), "invalid/missing directed removal is a no-op");
    check(d.removeNode(1) && d.getEdges().empty(), "node removal clears surviving incident arcs");
    return failures ? 1 : 0;
}
