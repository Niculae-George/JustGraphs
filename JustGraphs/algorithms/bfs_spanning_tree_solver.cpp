#include "algorithms/bfs_spanning_tree_solver.h"
#include "algorithms/graph_utils.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>

std::vector<SpanningTreeStep>
BFSSpanningTreeSolver::solve(const IGraphData &graph, int startNode) const {
  std::vector<SpanningTreeStep> steps;
  if (graph.getNodes().empty())
    return steps;

  auto adj = GraphUtils::buildSimpleAdjList(graph, false);

  std::unordered_set<int> visited;
  std::unordered_map<int, int> depths;
  std::unordered_map<int, int> parents;
  std::queue<int> q;
  std::unordered_set<std::string> reported_edges;
  auto get_edge_key = [](int u, int v) {
    if (u > v)
      std::swap(u, v);
    return std::to_string(u) + "-" + std::to_string(v);
  };

  visited.insert(startNode);
  depths[startNode] = 0;
  parents[startNode] = -1;
  q.push(startNode);

  steps.push_back(
      {SpanningTreeStep::Action::VisitNode, startNode, -1, -1, -1, 0});

  while (!q.empty()) {
    int u = q.front();
    q.pop();

    int d = depths[u];

    for (int v : adj[u]) {
      if (v == parents[u])
        continue;

      std::string key = get_edge_key(u, v);
      if (reported_edges.count(key))
        continue;
      reported_edges.insert(key);

      if (visited.find(v) == visited.end()) {
        visited.insert(v);
        depths[v] = d + 1;
        parents[v] = u;

        steps.push_back(
            {SpanningTreeStep::Action::TreeEdge, v, u, u, v, d + 1});
        steps.push_back(
            {SpanningTreeStep::Action::VisitNode, v, u, -1, -1, d + 1});

        q.push(v);
      } else {
        steps.push_back({SpanningTreeStep::Action::CrossEdge, v, -1, u, v, d});
      }
    }

    steps.push_back(
        {SpanningTreeStep::Action::CompleteNode, u, parents[u], -1, -1, d});
  }

  return steps;
}
