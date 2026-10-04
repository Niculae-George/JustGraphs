#include "algorithms/bfs_traversal.h"
#include "algorithms/graph_utils.h"
#include <queue>
#include <unordered_set>

std::vector<TraversalStep> BFSTraversal::solve(const IGraphData& graph, int startIndex, int stopAt) const
{
    if (!graph.getNodes().size() || std::ranges::none_of(graph.getNodes(), [startIndex](const Node& n){ return n.getIndex() == startIndex; })) return {};
    bool isDirected = graph.graphKind() == GraphKind::Directed;
    auto adj = GraphUtils::buildSimpleAdjList(graph, isDirected);
    
    std::unordered_map<int, int> visitedDist; // nodeIndex -> distance
    std::unordered_map<int, int> parents;    // nodeIndex -> parentIndex
    std::vector<TraversalStep> steps;
    std::queue<int> q;

    visitedDist[startIndex] = 0;
    parents[startIndex] = -1;
    q.push(startIndex);

    while (!q.empty()) {
        int cur = q.front(); q.pop();
        int dist = visitedDist[cur];
        
        steps.push_back({cur, parents[cur], dist});
        if (cur == stopAt) break;

        for (int nb : adj[cur]) {
            if (visitedDist.find(nb) == visitedDist.end()) {
                visitedDist[nb] = dist + 1;
                parents[nb] = cur;
                q.push(nb);
            }
        }
    }
    return steps;
}
