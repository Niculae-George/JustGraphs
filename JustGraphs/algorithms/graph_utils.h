#ifndef GRAPH_UTILS_H
#define GRAPH_UTILS_H

#include "algorithms/graph_algorithm_types.h"
#include "domain/graph_interfaces.h"  // ISP-compliant interfaces

#include <unordered_map>
#include <vector>

namespace GraphUtils {

// Build unweighted adjacency list (ISP-compliant - uses IGraphData)
inline std::unordered_map<int, std::vector<int>> buildSimpleAdjList(const IGraphData& graph, bool directed = true) {

    std::unordered_map<int, std::vector<int>> adj;
    for (const auto& n : graph.getNodes()) adj[n.getIndex()] = {};
    for (const auto& ed : graph.getEdges()) {
        int u = ed.getFirst().getIndex();
        int v = ed.getSecond().getIndex();
        adj[u].push_back(v);
        if (!directed) adj[v].push_back(u);
    }
    return adj;
}

} // namespace GraphUtils

#endif // GRAPH_UTILS_H
