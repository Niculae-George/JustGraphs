#pragma once

#include "maps/luxembourg_map.h"

namespace Luxembourg {

enum class Algorithm { Dijkstra, AStar, BidirectionalDijkstra, BellmanFord };

struct Route {
    bool found = false;
    qint64 cost = 0;
    std::vector<int> nodes;
    // Unique processing order used for visualization, not a settled-distance guarantee.
    std::vector<int> settled;
};

class IRoadRouter {
public:
    virtual ~IRoadRouter() = default;
    virtual Route solve(const Map& map, int source, int target, Algorithm algorithm) const = 0;
};

class RoadRouter final : public IRoadRouter {
public:
    Route solve(const Map& map, int source, int target,
                Algorithm algorithm = Algorithm::Dijkstra) const override;
};

} // namespace Luxembourg
