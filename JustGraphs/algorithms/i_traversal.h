#ifndef I_TRAVERSAL_H
#define I_TRAVERSAL_H

#include "algorithms/graph_algorithm_types.h"
#include "domain/graph_interfaces.h"
#include <vector>
#include <functional>
#include <string>

class ITraversalAlgorithm {
public:
    virtual ~ITraversalAlgorithm() = default;
    virtual std::vector<TraversalStep> solve(
        const IGraphData& graph, int startIndex, int stopAt = -1) const = 0;
    virtual std::string name() const = 0;
};

#endif // I_TRAVERSAL_H
