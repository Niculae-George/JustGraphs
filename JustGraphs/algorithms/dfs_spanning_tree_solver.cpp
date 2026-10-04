#include "algorithms/dfs_spanning_tree_solver.h"
#include "algorithms/graph_utils.h"
#include <stack>
#include <unordered_set>

std::vector<SpanningTreeStep>
DFSSpanningTreeSolver::solve(const IGraphData &graph, int startNode) const {
  std::vector<SpanningTreeStep> steps;
  if (graph.getNodes().empty())
    return steps;

  auto adj = GraphUtils::buildSimpleAdjList(graph, false);

  std::unordered_set<int> visited;
  std::unordered_set<int> on_stack;

  auto dfs = [&](auto &self, int u, int p, int depth) -> void {
    visited.insert(u);
    on_stack.insert(u);

    steps.push_back({SpanningTreeStep::Action::VisitNode, u, p, -1, -1, depth});

    for (int v : adj[u]) {
      if (v == p)
        continue;

      if (visited.find(v) == visited.end()) {
        steps.push_back(
            {SpanningTreeStep::Action::TreeEdge, v, u, u, v, depth + 1});
        self(self, v, u, depth + 1);
      } else if (on_stack.find(v) != on_stack.end()) {
        steps.push_back(
            {SpanningTreeStep::Action::BackEdge, v, -1, u, v, depth});
      }
    }

    on_stack.erase(u);
    steps.push_back(
        {SpanningTreeStep::Action::CompleteNode, u, p, -1, -1, depth});
  };

  dfs(dfs, startNode, -1, 0);

  return steps;
}
