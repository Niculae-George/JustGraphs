#ifndef ARTICULATION_BRIDGE_SOLVER_H
#define ARTICULATION_BRIDGE_SOLVER_H

#include "domain/graph_interfaces.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class ArtBridgeStepType {
	NodeEnter,    // DFS enters a node  — sets disc[v]
	EdgeTree,     // DFS tree edge traversed
	EdgeBack,     // DFS back edge found
	LowUpdate,    // low[v] updated after returning from child
	NodeExit,     // DFS exits a node — AP/bridge check happens here
	ResultAP,     // Articulation point confirmed
	ResultBridge, // Bridge confirmed
	Done          // Algorithm finished
};

struct ArtBridgeAnimStep {
	ArtBridgeStepType type;
	int nodeId = -1;  // primary node involved
	int nodeId2 = -1; // secondary node (for edge steps)
	int discVal = -1; // disc[nodeId] at this step
	int lowVal = -1;  // low[nodeId] at this step
};

struct ArtBridgeResult {
	std::unordered_set<int> articulationPoints; // set of node indices
	std::vector<std::pair<int, int>> bridges;   // pairs of node indices
	std::unordered_map<int, int> disc;          // disc[nodeIndex]
	std::unordered_map<int, int> low;           // low[nodeIndex]
	bool isBiconnected = false;                 // true if no APs and no bridges
};

class ArticulationBridgeSolver {
public:
	// Solve and optionally collect animation steps.
	// graph must be an undirected graph.
	static ArtBridgeResult solve(const IGraphData& graph,
		std::vector<ArtBridgeAnimStep>* steps = nullptr);

private:
	static void dfs(int u, int parent, int& timer, std::unordered_map<int, int>& disc,
			std::unordered_map<int, int>& low, std::unordered_set<int>& visited,
			const std::unordered_map<int, std::vector<int>>& adj,
			ArtBridgeResult& result, std::vector<ArtBridgeAnimStep>* steps);
};

#endif // ARTICULATION_BRIDGE_SOLVER_H
