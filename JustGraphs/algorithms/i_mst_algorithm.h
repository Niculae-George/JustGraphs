#ifndef I_MST_ALGORITHM_H
#define I_MST_ALGORITHM_H

#include "algorithms/graph_algorithm_types.h"
#include "domain/graph_interfaces.h"  // ISP-compliant interfaces
#include <vector>
#include <string>

// MST algorithm interface using ISP-compliant IGraphData
// Algorithms only need read access to graph nodes and edges
class IMSTAlgorithm {
public:
    virtual ~IMSTAlgorithm() = default;
    virtual std::vector<MSTStep> solve(const IGraphData& graph) const = 0;
    virtual std::string name() const = 0;
};

#endif // I_MST_ALGORITHM_H
