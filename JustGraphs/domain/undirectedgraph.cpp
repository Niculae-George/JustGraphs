#include "domain/undirectedgraph.h"

UndirectedGraph::UndirectedGraph() = default;

bool UndirectedGraph::addEdge(int from, int to, int cost)
{
    const Node* f = findNodePtr(from);
    const Node* s = findNodePtr(to);
    if (!f || !s || from == to) return false;
    for (const auto& e : m_edges) {
        const int u = e.getFirst().getIndex(), v = e.getSecond().getIndex();
        if ((u == from && v == to) || (u == to && v == from)) return false;
    }
    m_edges.emplace_back(f, s, cost);
    return true;
}
