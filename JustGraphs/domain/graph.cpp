#include "domain/graph.h"
#include <algorithm>
#include <limits>
#include <ranges>

int Graph::addNode(QPoint p) {
    if (m_nextNodeId > std::numeric_limits<int>::max()) return 0;
    const int id = int(m_nextNodeId++);
    m_nodes.emplace_back(id, p);
    return id;
}

bool Graph::addNodeWithId(int id, QPoint p) {
    if (id <= 0 || findNodePtr(id)) return false;
    m_nodes.emplace_back(id, p);
    m_nextNodeId = std::max(m_nextNodeId, std::int64_t(id) + 1);
    return true;
}

bool Graph::setNodePosition(int id, QPoint p) {
    Node* n = findNodePtr(id);
    if (!n) return false;
    n->setCoord(p);
    return true;
}

bool Graph::removeNode(int id) {
    auto it = std::ranges::find(m_nodes, id, &Node::getIndex);
    if (it == m_nodes.end()) return false;
    // Remove references before destroying their endpoint object.
    std::erase_if(m_edges, [id](const Edge& e) {
        return e.getFirst().getIndex() == id || e.getSecond().getIndex() == id;
    });
    m_nodes.erase(it);
    return true;
}

bool Graph::removeEdge(int from, int to) {
    const auto removed = std::erase_if(m_edges, [=, this](const Edge& edge) {
        const int u = edge.getFirst().getIndex(), v = edge.getSecond().getIndex();
        return (u == from && v == to) ||
               (graphKind() == GraphKind::Undirected && u == to && v == from);
    });
    return removed != 0;
}

bool Graph::addEdge(const Node& from, const Node& to, int cost) {
    const Node* ownedFrom = findNodePtr(from.getIndex());
    const Node* ownedTo = findNodePtr(to.getIndex());
    if (ownedFrom != &from || ownedTo != &to) return false;
    return addEdge(from.getIndex(), to.getIndex(), cost);
}

void Graph::clear() { m_edges.clear(); m_nodes.clear(); m_nextNodeId = 1; }

const Node* Graph::findNodePtr(int id) const {
    auto it = std::ranges::find(m_nodes, id, &Node::getIndex);
    return it == m_nodes.end() ? nullptr : &*it;
}
Node* Graph::findNodePtr(int id) {
    auto it = std::ranges::find(m_nodes, id, &Node::getIndex);
    return it == m_nodes.end() ? nullptr : &*it;
}
const Node* Graph::findNode(int id) const { return findNodePtr(id); }
const std::list<Node>& Graph::getNodes() const { return m_nodes; }
const std::vector<Edge>& Graph::getEdges() const { return m_edges; }
