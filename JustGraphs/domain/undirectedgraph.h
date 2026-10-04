#ifndef UNDIRECTEDGRAPH_H
#define UNDIRECTEDGRAPH_H
#include "domain/graph.h"

class UndirectedGraph : public Graph
{
public:
    UndirectedGraph();
    using Graph::addEdge;
    bool addEdge(int from, int to, int cost = 1) override;
    GraphKind graphKind() const override { return GraphKind::Undirected; }
};

#endif // UNDIRECTEDGRAPH_H
