#include "algorithms/prim_solver.h"
#include "domain/edge.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <tuple>

std::vector<MSTStep> PrimSolver::solve(const IGraphData& graph) const
{
    std::vector<MSTStep> steps;
    if (graph.getNodes().empty()) return steps;

    std::unordered_map<int, std::vector<std::pair<int, int>>> adj;
    for (const auto& n : graph.getNodes()) adj[n.getIndex()] = {};
    for (const auto& ed : graph.getEdges()) {
        int u = ed.getFirst().getIndex(), v = ed.getSecond().getIndex(), c = ed.getCost();
        adj[u].emplace_back(v, c);
        adj[v].emplace_back(u, c);
    }

    std::unordered_set<int> inMST;
    using T = std::tuple<int, int, int>;
    std::priority_queue<T, std::vector<T>, std::greater<T>> pq;

    int start = graph.getNodes().front().getIndex();
    inMST.insert(start);
    for (auto& [nb, c] : adj[start]) pq.emplace(c, start, nb);

    while (!pq.empty()) {
        auto [cost, u, v] = pq.top(); pq.pop();
        if (inMST.count(v)) {
            steps.push_back({u, v, cost, false});
            continue;
        }
        inMST.insert(v);
        steps.push_back({u, v, cost, true});
        for (auto& [nb, c] : adj[v]) {
            if (!inMST.count(nb)) pq.emplace(c, v, nb);
        }
    }
    return steps;
}
