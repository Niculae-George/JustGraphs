#ifndef TOPOLOGICAL_SORT_SOLVER_H
#define TOPOLOGICAL_SORT_SOLVER_H

#include "domain/graph_interfaces.h"
#include <vector>
#include <queue>
#include <unordered_map>

struct TopologicalStep {
    int nodeIndex = -1;
    int inDegree = 0;
    bool isProcessing = false;  // Currently removing from queue
    bool isSorted = false;      // Added to final order
};

class TopologicalSortSolver {
public:
    // Returns sorted order; empty if cycle detected
    std::vector<TopologicalStep> solve(const IGraphData& graph, std::vector<int>& outOrder) const;
    
    // Check if graph has a cycle (using DFS for detection)
    bool hasCycle(const IGraphData& graph) const;
    
    std::string name() const { return "Topological Sort (Kahn's Algorithm)"; }
};

#endif // TOPOLOGICAL_SORT_SOLVER_H
