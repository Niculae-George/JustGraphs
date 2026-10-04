#ifndef KOSARAJU_SOLVER_H
#define KOSARAJU_SOLVER_H

#include "domain/graph_interfaces.h"  // ISP-compliant interfaces

#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <vector>

class KosarajuSolver {
public:
    // Returns number of components; fills componentsColors with node→component mapping
    int solve(const IGraphData& graph,
              std::unordered_map<int, int>& outComponentColors) const;


private:
    void fillOrder(int nodeIndex,
                   std::unordered_set<int>& visited,
                   std::stack<int>& Stack,
                   const std::unordered_map<int, std::vector<int>>& adjList) const;

    void DFSUtil(int nodeIndex,
                 std::unordered_set<int>& visited,
                 std::unordered_map<int, std::vector<int>>& transposeAdj,
                 int component,
                 std::unordered_map<int, int>& outComponentColors) const;

    std::unordered_map<int, std::vector<int>> getTranspose(const IGraphData& graph) const;
    std::unordered_map<int, std::vector<int>> buildDirectedAdjList(const IGraphData& graph) const;

};

#endif // KOSARAJU_SOLVER_H
