#ifndef DIRECTEDGRAPH_H
#define DIRECTEDGRAPH_H
#include "domain/graph.h"

class DirectedGraph : public Graph
{
public:
    DirectedGraph();
    using Graph::addEdge;
    bool addEdge(int from, int to, int cost = 1) override;
    GraphKind graphKind() const override { return GraphKind::Directed; }
};

#endif // DIRECTEDGRAPH_H
