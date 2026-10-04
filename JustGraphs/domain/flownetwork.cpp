#include "domain/flownetwork.h"

#include <cmath>
#include <limits>

bool FlowNetwork::validNode(int node) const {
    return node >= 1 && node <= getNumNodes();
}

bool FlowNetwork::validEndpoints(int from, int to) const {
    return validNode(from) && validNode(to);
}

void FlowNetwork::topologyChanged() { ++m_topologyRevision; }

int FlowNetwork::addNode(const QPointF& pos) {
    if (!std::isfinite(pos.x()) || !std::isfinite(pos.y())) return -1;
    const int n = getNumNodes() + 1;
    m_nodePositions.push_back(pos);
    for (auto& row : m_capacity) row.resize(size_t(n), 0);
    m_capacity.emplace_back(size_t(n), 0);
    for (auto& row : m_cost) row.resize(size_t(n), 0);
    m_cost.emplace_back(size_t(n), 0);
    topologyChanged();
    return n;
}

bool FlowNetwork::setNodePosition(int node, const QPointF& pos) {
    if (!validNode(node) || !std::isfinite(pos.x()) || !std::isfinite(pos.y())) return false;
    m_nodePositions[size_t(node - 1)] = pos;
    return true;
}

bool FlowNetwork::removeNode(int node) {
    if (!validNode(node)) return false;
    const size_t index = size_t(node - 1);
    m_nodePositions.erase(m_nodePositions.begin() + static_cast<std::ptrdiff_t>(index));
    for (auto* matrix : {&m_capacity, &m_cost}) {
        matrix->erase(matrix->begin() + static_cast<std::ptrdiff_t>(index));
        for (auto& row : *matrix) row.erase(row.begin() + static_cast<std::ptrdiff_t>(index));
    }
    auto remap = [node](int id) { return id == node ? -1 : id > node ? id - 1 : id; };
    m_source = remap(m_source);
    m_sink = remap(m_sink);
    topologyChanged();
    return true;
}

bool FlowNetwork::addEdge(int from, int to, int capacity) {
    if (!validEndpoints(from, to) || from == to || capacity <= 0) return false;
    const size_t u = size_t(from - 1), v = size_t(to - 1);
    if (m_capacity[u][v] == capacity) return true;
    m_capacity[u][v] = capacity;
    topologyChanged();
    return true;
}

bool FlowNetwork::removeEdge(int from, int to) {
    if (!validEndpoints(from, to) || getCapacity(from, to) <= 0) return false;
    const size_t u = size_t(from - 1), v = size_t(to - 1);
    m_capacity[u][v] = 0;
    // Preserve paired costs for a surviving reverse edge and its residual arc.
    if (m_capacity[v][u] == 0) m_cost[u][v] = m_cost[v][u] = 0;
    topologyChanged();
    return true;
}

bool FlowNetwork::setCapacity(int from, int to, int capacity) {
    return addEdge(from, to, capacity);
}

bool FlowNetwork::setEdgeCost(int from, int to, int cost) {
    if (!validEndpoints(from, to) || from == to || cost == std::numeric_limits<int>::min()) return false;
    const size_t u = size_t(from - 1), v = size_t(to - 1);
    if (m_cost[u][v] == cost && m_cost[v][u] == -cost) return true;
    m_cost[u][v] = cost;
    m_cost[v][u] = -cost;
    topologyChanged();
    return true;
}

void FlowNetwork::clear() {
    if (m_nodePositions.empty() && m_source == -1 && m_sink == -1) return;
    m_nodePositions.clear();
    m_capacity.clear();
    m_cost.clear();
    m_source = m_sink = -1;
    topologyChanged();
}

int FlowNetwork::getNumNodes() const { return static_cast<int>(m_nodePositions.size()); }
const std::vector<QPointF>& FlowNetwork::getNodePositions() const { return m_nodePositions; }

int FlowNetwork::getCapacity(int from, int to) const {
    if (!validEndpoints(from, to)) return 0;
    return m_capacity[size_t(from - 1)][size_t(to - 1)];
}

int FlowNetwork::getEdgeCost(int from, int to) const {
    if (!validEndpoints(from, to)) return 0;
    return m_cost[size_t(from - 1)][size_t(to - 1)];
}

bool FlowNetwork::setSource(int source) {
    if (source != -1 && !validNode(source)) return false;
    if (source != -1 && source == m_sink) return false;
    if (m_source != source) { m_source = source; topologyChanged(); }
    return true;
}

bool FlowNetwork::setSink(int sink) {
    if (sink != -1 && !validNode(sink)) return false;
    if (sink != -1 && sink == m_source) return false;
    if (m_sink != sink) { m_sink = sink; topologyChanged(); }
    return true;
}

int FlowNetwork::getSource() const { return m_source; }
int FlowNetwork::getSink() const { return m_sink; }
bool FlowNetwork::hasValidEndpoints() const {
    return validNode(m_source) && validNode(m_sink) && m_source != m_sink;
}
unsigned long long FlowNetwork::topologyRevision() const { return m_topologyRevision; }
