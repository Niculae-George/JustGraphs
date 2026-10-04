#include "algorithms/articulation_bridge_solver.h"
#include <algorithm>

// ─────────────────────────────────────────────────────────────
//  solve() — entry point
// ─────────────────────────────────────────────────────────────

ArtBridgeResult ArticulationBridgeSolver::solve(
	const IGraphData& graph,
	std::vector<ArtBridgeAnimStep>* steps)
{
	ArtBridgeResult result;

	const auto& nodes = graph.getNodes();
	const auto& edges = graph.getEdges();

	if (nodes.empty()) return result;

	// Build adjacency list  (undirected: both directions)
	std::unordered_map<int, std::vector<int>> adj;
	for (const auto& n : nodes)
		adj[n.getIndex()] = {};

	for (const auto& e : edges) {
		int u = e.getFirst().getIndex();
		int v = e.getSecond().getIndex();
		adj[u].push_back(v);
		adj[v].push_back(u);
	}

	std::unordered_set<int> visited;
	int timer = 0;

	// Run DFS from each unvisited node (handles disconnected graphs)
	for (const auto& n : nodes) {
		int id = n.getIndex();
		if (visited.find(id) == visited.end()) {
			dfs(id, -1, timer, result.disc, result.low,
				visited, adj, result, steps);
		}
	}

	result.isBiconnected =
		result.articulationPoints.empty() && result.bridges.empty();

	if (steps) {
		steps->push_back({ ArtBridgeStepType::Done, -1, -1, -1, -1 });
	}

	return result;
}

// ─────────────────────────────────────────────────────────────
//  dfs() — recursive Tarjan DFS
// ─────────────────────────────────────────────────────────────

void ArticulationBridgeSolver::dfs(
	int u, int parent,
	int& timer,
	std::unordered_map<int, int>& disc,
	std::unordered_map<int, int>& low,
	std::unordered_set<int>& visited,
	const std::unordered_map<int, std::vector<int>>& adj,
	ArtBridgeResult& result,
	std::vector<ArtBridgeAnimStep>* steps)
{
	visited.insert(u);
	disc[u] = low[u] = ++timer;
	int children = 0;

	if (steps) {
		steps->push_back({ ArtBridgeStepType::NodeEnter, u, parent,
						  disc[u], low[u] });
	}

	for (int v : adj.at(u)) {
		if (visited.find(v) == visited.end()) {
			// Tree edge
			++children;
			if (steps) {
				steps->push_back({ ArtBridgeStepType::EdgeTree, u, v,
								  disc[u], low[u] });
			}

			dfs(v, u, timer, disc, low, visited, adj, result, steps);

			// Update low[u] after returning
			low[u] = std::min(low[u], low[v]);

			if (steps) {
				steps->push_back({ ArtBridgeStepType::LowUpdate, u, v,
								  disc[u], low[u] });
			}

			// ── Articulation Point check ────────────────────
			bool isAP = false;
			if (parent == -1 && children > 1)  isAP = true;  // root case
			if (parent != -1 && low[v] >= disc[u]) isAP = true; // non-root

			if (isAP && result.articulationPoints.find(u) ==
				result.articulationPoints.end()) {
				result.articulationPoints.insert(u);
				if (steps) {
					steps->push_back({ ArtBridgeStepType::ResultAP, u, -1,
									  disc[u], low[u] });
				}
			}

			// ── Bridge check ────────────────────────────────
			if (low[v] > disc[u]) {
				int a = std::min(u, v), b = std::max(u, v);
				result.bridges.push_back({ a, b });
				if (steps) {
					steps->push_back({ ArtBridgeStepType::ResultBridge, u, v,
									  disc[u], low[v] });
				}
			}

		}
		else if (v != parent) {
			// Back edge — update low[u]
			low[u] = std::min(low[u], disc[v]);
			if (steps) {
				steps->push_back({ ArtBridgeStepType::EdgeBack, u, v,
								  disc[u], low[u] });
			}
		}
	}

	if (steps) {
		steps->push_back({ ArtBridgeStepType::NodeExit, u, parent,
						  disc[u], low[u] });
	}
}
