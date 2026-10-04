#ifndef I_SPANNING_TREE_H
#define I_SPANNING_TREE_H

#include "algorithms/graph_algorithm_types.h"
#include "domain/graph_interfaces.h"
#include <vector>
#include <string>

struct SpanningTreeStep {
    enum class Action {
        VisitNode,
        TreeEdge,
        BackEdge,
        CrossEdge,
        CompleteNode
    };
    Action action;
    int nodeIndex;
    int parentIndex;
    int edgeFrom;
    int edgeTo;
    int depth;
};

class ISpanningTreeAlgorithm {
public:
    virtual ~ISpanningTreeAlgorithm() = default;
    virtual std::vector<SpanningTreeStep> solve(const IGraphData& graph, int startNode) const = 0;
    virtual std::string name() const = 0;
};

#endif // I_SPANNING_TREE_H
