#include "algorithms/topological_sort_solver.h"
#include "algorithms/graph_utils.h"
#include <stack>
#include <functional>

std::vector<TopologicalStep>
TopologicalSortSolver::solve(const IGraphData &graph,
                             std::vector<int> &outOrder) const {
  outOrder.clear();

  if (graph.graphKind() != GraphKind::Directed) return {};

  std::vector<TopologicalStep> steps;

  auto adj = GraphUtils::buildSimpleAdjList(graph, true); // directed

  // Calculate in-degrees
  std::unordered_map<int, int> inDegree;
  for (const auto &node : graph.getNodes()) {
    inDegree[node.getIndex()] = 0;
  }
  for (const auto &edge : graph.getEdges()) {
    inDegree[edge.getSecond().getIndex()]++;
  }

  // Queue for nodes with in-degree 0
  std::queue<int> q;
  for (const auto &node : graph.getNodes()) {
    if (inDegree[node.getIndex()] == 0) {
      q.push(node.getIndex());
      steps.push_back({node.getIndex(), 0, true, false});
    }
  }

  while (!q.empty()) {
    int u = q.front();
    q.pop();
    outOrder.push_back(u);

    // Mark as sorted
    if (!steps.empty() && steps.back().nodeIndex == u &&
        steps.back().isProcessing) {
      steps.back().isProcessing = false;
      steps.back().isSorted = true;
    } else {
      steps.push_back({u, 0, false, true});
    }

    for (int v : adj[u]) {
      inDegree[v]--;
      if (inDegree[v] == 0) {
        q.push(v);
        steps.push_back({v, 0, true, false});
      }
    }
  }

  if (outOrder.size() != graph.getNodes().size()) {
    outOrder.clear();
    return {};
  }

  return steps;
}

bool TopologicalSortSolver::hasCycle(const IGraphData &graph) const {
  if (graph.graphKind() != GraphKind::Directed) return false;
  auto adj = GraphUtils::buildSimpleAdjList(graph, true);

  std::unordered_set<int> visited;
  std::unordered_set<int> recStack;

  std::function<bool(int)> dfs = [&](int u) -> bool {
    visited.insert(u);
    recStack.insert(u);

    for (int v : adj[u]) {
      if (recStack.count(v))
        return true;
      if (!visited.count(v) && dfs(v))
        return true;
    }

    recStack.erase(u);
    return false;
  };

  for (const auto &node : graph.getNodes()) {
    if (!visited.count(node.getIndex())) {
      if (dfs(node.getIndex()))
        return true;
    }
  }

  return false;
}
