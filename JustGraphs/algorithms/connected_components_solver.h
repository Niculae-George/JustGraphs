#ifndef CONNECTED_COMPONENTS_SOLVER_H
#define CONNECTED_COMPONENTS_SOLVER_H

#include "domain/graph_interfaces.h"  // ISP-compliant interfaces

#include <unordered_map>
#include <vector>

class ConnectedComponentsSolver {
public:
    // Returns number of components and fills a map of nodeId -> componentId
    ComponentResult solve(const IGraphData& graph) const;

};

#endif // CONNECTED_COMPONENTS_SOLVER_H
