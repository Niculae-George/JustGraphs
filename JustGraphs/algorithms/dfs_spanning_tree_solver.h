#ifndef DFS_SPANNING_TREE_SOLVER_H
#define DFS_SPANNING_TREE_SOLVER_H

#include "algorithms/i_spanning_tree.h"

class DFSSpanningTreeSolver : public ISpanningTreeAlgorithm {
public:
    std::vector<SpanningTreeStep> solve(const IGraphData& graph, int startNode) const override;
    std::string name() const override { return "DFS Spanning Tree"; }
};

#endif // DFS_SPANNING_TREE_SOLVER_H
