#ifndef GRAPH_ALGORITHM_TYPES_H
#define GRAPH_ALGORITHM_TYPES_H

#include <limits>
#include <string>
#include <vector>

// ─── Shared step/result structs used by algorithm solvers ───

struct PathStep {
    int  nodeIndex;
    int  fromIndex;
    int  cost;
    bool isFinal;
};

struct TraversalStep {
    int nodeIndex;
    int parentIndex;
    int distance;
};

struct MSTStep {
    int  fromIndex;
    int  toIndex;
    int  cost;
    bool accepted;
};

static constexpr int INF_COST = std::numeric_limits<int>::max() / 2;

#endif // GRAPH_ALGORITHM_TYPES_H
