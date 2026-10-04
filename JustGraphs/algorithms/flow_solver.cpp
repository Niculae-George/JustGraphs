#include "algorithms/flow_solver.h"

#include "domain/flownetwork.h"

#include <algorithm>
#include <limits>
#include <queue>

FlowSolver::FlowSolver(const FlowNetwork* network) { setNetwork(network); }

void FlowSolver::setNetwork(const FlowNetwork* network) {
    m_network = network;
    m_revision = network ? network->topologyRevision() : 0;
    reset();
}

void FlowSolver::reset() {
    resetCache();
}

void FlowSolver::resetCache() const {
    m_residual.clear();
    m_iterations.clear();
    m_minCutEdges.clear();
    m_totalFlow = 0;
}

bool FlowSolver::synchronizeRevision() const {
    if (!m_network) { resetCache(); return false; }
    if (m_revision != m_network->topologyRevision()) {
        m_revision = m_network->topologyRevision();
        resetCache();
    }
    return true;
}

void FlowSolver::initializeAlgorithm() {
    if (!synchronizeRevision() || !m_network->hasValidEndpoints()) return;
    const int n = m_network->getNumNodes();
    m_residual.assign(size_t(n), std::vector<std::int64_t>(size_t(n), 0));
    for (int i = 1; i <= n; ++i)
        for (int j = 1; j <= n; ++j)
            m_residual[size_t(i - 1)][size_t(j - 1)] = m_network->getCapacity(i, j);
    m_totalFlow = 0;
    m_minCutEdges.clear();
    m_iterations.clear();
    FlowIterationResult state;
    state.kind = FlowIterationKind::Initial;
    state.residualCapacity = m_residual;
    m_iterations.push_back(std::move(state));
}

bool FlowSolver::bfs(const std::vector<std::vector<std::int64_t>>& residual, int source, int sink,
                     std::vector<int>& parent) const {
    const int n = static_cast<int>(residual.size());
    std::fill(parent.begin(), parent.end(), -1);
    if (source < 0 || sink < 0 || source >= n || sink >= n) return false;
    std::vector<bool> visited(size_t(n), false);
    std::queue<int> queue;
    queue.push(source);
    visited[size_t(source)] = true;
    while (!queue.empty()) {
        const int u = queue.front(); queue.pop();
        for (int v = 0; v < n; ++v) {
            if (!visited[size_t(v)] && residual[size_t(u)][size_t(v)] > 0) {
                parent[size_t(v)] = u;
                if (v == sink) return true;
                visited[size_t(v)] = true;
                queue.push(v);
            }
        }
    }
    return false;
}

bool FlowSolver::performStep() {
    if (!synchronizeRevision() || !m_network->hasValidEndpoints() || m_residual.empty()) return false;
    const int source = m_network->getSource() - 1;
    const int sink = m_network->getSink() - 1;
    std::vector<int> parent(m_residual.size());
    if (!bfs(m_residual, source, sink, parent)) return false;

    std::int64_t pathFlow = std::numeric_limits<std::int64_t>::max();
    std::vector<int> path;
    for (int v = sink; v != source; v = parent[size_t(v)]) {
        const int u = parent[size_t(v)];
        pathFlow = std::min(pathFlow, m_residual[size_t(u)][size_t(v)]);
        path.push_back(v + 1);
    }
    path.push_back(source + 1);
    std::reverse(path.begin(), path.end());
    for (int v = sink; v != source; v = parent[size_t(v)]) {
        const int u = parent[size_t(v)];
        if (m_residual[size_t(v)][size_t(u)] > std::numeric_limits<std::int64_t>::max() - pathFlow)
            return false;
    }
    if (m_totalFlow > std::numeric_limits<std::int64_t>::max() - pathFlow) return false;
    for (int v = sink; v != source; v = parent[size_t(v)]) {
        const int u = parent[size_t(v)];
        m_residual[size_t(u)][size_t(v)] -= pathFlow;
        m_residual[size_t(v)][size_t(u)] += pathFlow;
    }
    m_totalFlow += pathFlow;
    FlowIterationResult state;
    state.iterationNumber = static_cast<int>(m_iterations.size());
    state.kind = FlowIterationKind::Augmentation;
    state.residualCapacity = m_residual;
    state.augmentingPath = std::move(path);
    state.pathFlow = pathFlow;
    state.totalFlow = m_totalFlow;
    m_iterations.push_back(std::move(state));
    return true;
}

void FlowSolver::finalizeMinCut() {
    if (!synchronizeRevision() || !m_network->hasValidEndpoints() || m_residual.empty()) return;
    const int n = m_network->getNumNodes();
    std::vector<bool> visited(size_t(n), false);
    std::queue<int> queue;
    const int source = m_network->getSource() - 1;
    queue.push(source); visited[size_t(source)] = true;
    while (!queue.empty()) {
        const int u = queue.front(); queue.pop();
        for (int v = 0; v < n; ++v) {
            if (!visited[size_t(v)] && m_residual[size_t(u)][size_t(v)] > 0) {
                visited[size_t(v)] = true; queue.push(v);
            }
        }
    }
    m_minCutEdges.clear();
    for (int i = 1; i <= n; ++i)
        for (int j = 1; j <= n; ++j)
            if (visited[size_t(i - 1)] && !visited[size_t(j - 1)] && m_network->getCapacity(i, j) > 0)
                m_minCutEdges.insert({i, j});
}

void FlowSolver::addFinalState() {
    if (!synchronizeRevision() || m_residual.empty()) return;
    FlowIterationResult state;
    state.iterationNumber = static_cast<int>(m_iterations.size());
    state.kind = FlowIterationKind::FinalCut;
    state.residualCapacity = m_residual;
    state.totalFlow = m_totalFlow;
    m_iterations.push_back(std::move(state));
}

std::int64_t FlowSolver::runFullAlgorithm() {
    initializeAlgorithm();
    if (m_iterations.empty()) return 0;
    while (performStep()) {}
    finalizeMinCut();
    addFinalState();
    return m_totalFlow;
}

std::vector<int> FlowSolver::findNegativeCycle() const {
    if (!m_network || m_residual.empty()) return {};
    const int n = m_network->getNumNodes();
    std::vector<std::int64_t> distance(size_t(n), 0);
    std::vector<int> predecessor(size_t(n), -1);
    int cycleNode = -1;
    for (int iter = 0; iter < n; ++iter) {
        cycleNode = -1;
        for (int u = 0; u < n; ++u) for (int v = 0; v < n; ++v) {
            if (m_residual[size_t(u)][size_t(v)] <= 0) continue;
            const std::int64_t cost = m_network->getEdgeCost(u + 1, v + 1);
            if ((cost > 0 && distance[size_t(u)] > std::numeric_limits<std::int64_t>::max() - cost) ||
                (cost < 0 && distance[size_t(u)] < std::numeric_limits<std::int64_t>::min() - cost)) continue;
            if (distance[size_t(u)] + cost < distance[size_t(v)]) {
                distance[size_t(v)] = distance[size_t(u)] + cost;
                predecessor[size_t(v)] = u;
                if (iter == n - 1) cycleNode = v;
            }
        }
    }
    if (cycleNode < 0) return {};
    int x = cycleNode;
    for (int i = 0; i < n; ++i) {
        if (predecessor[size_t(x)] < 0) return {};
        x = predecessor[size_t(x)];
    }
    std::vector<int> cycle;
    std::vector<bool> visited(size_t(n), false);
    int current = x;
    while (!visited[size_t(current)]) {
        visited[size_t(current)] = true;
        cycle.push_back(current + 1);
        current = predecessor[size_t(current)];
        if (current < 0) return {};
    }
    cycle.push_back(current + 1);
    std::reverse(cycle.begin(), cycle.end());
    return cycle;
}

int FlowSolver::eliminateNegativeCycles() {
    if (!synchronizeRevision() || !m_network || m_network->getNumNodes() == 0) return 0;
    if (m_residual.empty()) {
        const int n = m_network->getNumNodes();
        m_residual.assign(size_t(n), std::vector<std::int64_t>(size_t(n), 0));
        for (int i = 1; i <= n; ++i) for (int j = 1; j <= n; ++j)
            m_residual[size_t(i - 1)][size_t(j - 1)] = m_network->getCapacity(i, j);
    }
    int eliminated = 0;
    for (int guard = 0; guard < 1000; ++guard) {
        const std::vector<int> cycle = findNegativeCycle();
        if (cycle.empty()) break;
        std::int64_t amount = std::numeric_limits<std::int64_t>::max();
        for (size_t i = 1; i < cycle.size(); ++i)
            amount = std::min(amount, m_residual[size_t(cycle[i - 1] - 1)][size_t(cycle[i] - 1)]);
        if (amount <= 0 || amount == std::numeric_limits<std::int64_t>::max()) break;
        bool wouldOverflow = false;
        for (size_t i = 1; i < cycle.size(); ++i) {
            const size_t u = size_t(cycle[i - 1] - 1), v = size_t(cycle[i] - 1);
            if (m_residual[v][u] > std::numeric_limits<std::int64_t>::max() - amount) {
                wouldOverflow = true;
                break;
            }
        }
        if (wouldOverflow) break;
        for (size_t i = 1; i < cycle.size(); ++i) {
            const size_t u = size_t(cycle[i - 1] - 1), v = size_t(cycle[i] - 1);
            m_residual[u][v] -= amount;
            m_residual[v][u] += amount;
        }
        ++eliminated;
    }
    if (eliminated > 0) {
        FlowIterationResult state;
        state.iterationNumber = static_cast<int>(m_iterations.size());
        state.kind = FlowIterationKind::NegativeCycleCancellation;
        state.residualCapacity = m_residual;
        state.totalFlow = m_totalFlow;
        state.eliminatedCycles = eliminated;
        m_iterations.push_back(std::move(state));
    }
    return eliminated;
}

std::int64_t FlowSolver::totalFlow() const {
    synchronizeRevision();
    return m_totalFlow;
}
std::int64_t FlowSolver::getResidual(int from, int to) const {
    synchronizeRevision();
    if (from < 1 || to < 1 || size_t(from) > m_residual.size() || size_t(to) > m_residual.size()) return 0;
    return m_residual[size_t(from - 1)][size_t(to - 1)];
}
std::int64_t FlowSolver::getFlow(int from, int to) const {
    synchronizeRevision();
    if (!m_network || m_network->getCapacity(from, to) <= 0) return 0;
    return m_network->getCapacity(from, to) - getResidual(from, to);
}
const std::vector<FlowIterationResult>& FlowSolver::iterations() const {
    synchronizeRevision();
    return m_iterations;
}
const std::set<std::pair<int, int>>& FlowSolver::minCutEdges() const {
    synchronizeRevision();
    return m_minCutEdges;
}
bool FlowSolver::isMinCutEdge(int from, int to) const {
    synchronizeRevision();
    return m_minCutEdges.count({from, to}) != 0;
}
