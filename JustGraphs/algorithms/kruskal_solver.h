#ifndef KRUSKAL_SOLVER_H
#define KRUSKAL_SOLVER_H

#include "algorithms/i_mst_algorithm.h"

class KruskalSolver : public IMSTAlgorithm {
public:
    std::vector<MSTStep> solve(const IGraphData& graph) const override;
    std::string name() const override { return "Kruskal"; }
};

#endif // KRUSKAL_SOLVER_H
