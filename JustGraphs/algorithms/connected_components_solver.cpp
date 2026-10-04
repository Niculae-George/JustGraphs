#include "algorithms/connected_components_solver.h"
#include "algorithms/graph_utils.h"
#include <stack>
#include <unordered_set>

ComponentResult ConnectedComponentsSolver::solve(const IGraphData& graph) const {
    ComponentResult result;
    if (graph.getNodes().empty()) return result;

    auto adj = GraphUtils::buildSimpleAdjList(graph, false); // false for undirected (bi-directional)
    std::unordered_set<int> visited;

    for (const auto& node : graph.getNodes()) {
        int startNode = node.getIndex();
        if (visited.find(startNode) == visited.end()) {
            // Iterative DFS
            std::stack<int> s;
            s.push(startNode);
            visited.insert(startNode);
            result.nodeToComponent[startNode] = result.numComponents;

            while (!s.empty()) {
                int u = s.top(); s.pop();
                for (int v : adj[u]) {
                    if (visited.find(v) == visited.end()) {
                        visited.insert(v);
                        result.nodeToComponent[v] = result.numComponents;
                        s.push(v);
                    }
                }
            }
            result.numComponents++;
        }
    }
    return result;
}
