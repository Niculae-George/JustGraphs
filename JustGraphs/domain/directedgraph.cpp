#include "domain/directedgraph.h"

DirectedGraph::DirectedGraph() : Graph() {}

bool DirectedGraph::addEdge(int from, int to, int cost) {
    const Node* f = findNodePtr(from);
    const Node* s = findNodePtr(to);
    if (!f || !s || from == to) return false;
    for (const auto& e : m_edges)
        if (e.getFirst().getIndex() == from && e.getSecond().getIndex() == to) return false;
    m_edges.emplace_back(f, s, cost);
    return true;
}
