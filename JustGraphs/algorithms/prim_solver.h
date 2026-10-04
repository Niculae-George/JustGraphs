#ifndef PRIM_SOLVER_H
#define PRIM_SOLVER_H

#include "algorithms/i_mst_algorithm.h"

class PrimSolver : public IMSTAlgorithm {
public:
    std::vector<MSTStep> solve(const IGraphData& graph) const override;
    std::string name() const override { return "Prim"; }
};

#endif // PRIM_SOLVER_H
