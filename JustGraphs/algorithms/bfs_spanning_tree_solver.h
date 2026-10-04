#ifndef BFS_SPANNING_TREE_SOLVER_H
#define BFS_SPANNING_TREE_SOLVER_H

#include "algorithms/i_spanning_tree.h"

class BFSSpanningTreeSolver : public ISpanningTreeAlgorithm {
public:
    std::vector<SpanningTreeStep> solve(const IGraphData& graph, int startNode) const override;
    std::string name() const override { return "BFS Spanning Tree"; }
};

#endif // BFS_SPANNING_TREE_SOLVER_H
