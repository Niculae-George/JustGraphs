#ifndef BORUVKA_SOLVER_H
#define BORUVKA_SOLVER_H

#include "algorithms/i_mst_algorithm.h"

class BoruvkaSolver : public IMSTAlgorithm {
public:
    std::vector<MSTStep> solve(const IGraphData& graph) const override;
    std::string name() const override { return "Boruvka"; }
};

#endif // BORUVKA_SOLVER_H
