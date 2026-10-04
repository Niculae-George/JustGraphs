#include "algorithms/dfs_traversal.h"
#include "algorithms/graph_utils.h"
#include <stack>
#include <unordered_set>

std::vector<TraversalStep> DFSTraversal::solve(const IGraphData& graph, int startIndex, int stopAt) const
{
    if (!graph.getNodes().size() || std::ranges::none_of(graph.getNodes(), [startIndex](const Node& n){ return n.getIndex() == startIndex; })) return {};
    bool isDirected = graph.graphKind() == GraphKind::Directed;
    auto adj = GraphUtils::buildSimpleAdjList(graph, isDirected);
    
    std::unordered_map<int, int> visitedDepth; // nodeIndex -> depth
    std::unordered_map<int, int> parents;      // nodeIndex -> parentIndex
    std::vector<TraversalStep> steps;
    std::stack<std::pair<int, int>> stk; // {nodeIndex, parentIndex}

    stk.push({startIndex, -1});
    visitedDepth[startIndex] = 0;

    while (!stk.empty()) {
        auto [cur, par] = stk.top(); stk.pop();
        int depth = (par == -1) ? 0 : visitedDepth[par] + 1;
        visitedDepth[cur] = depth;
        
        steps.push_back({cur, par, depth});
        if (cur == stopAt) break;

        const auto& neighbors = adj[cur];
        for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
            int nb = *it;
            if (visitedDepth.find(nb) == visitedDepth.end()) {
                visitedDepth[nb] = depth + 1;
                stk.push({nb, cur});
            }
        }
    }
    return steps;
}
