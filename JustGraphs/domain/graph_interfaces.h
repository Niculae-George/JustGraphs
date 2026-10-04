#ifndef GRAPH_INTERFACES_H
#define GRAPH_INTERFACES_H

#include <list>
#include <vector>
#include <unordered_map>
#include <QtCore/QPoint>
#include "domain/Node.h"
#include "domain/edge.h"

enum class GraphKind { Directed, Undirected };

class IGraphData {
public:
    virtual ~IGraphData() = default;
    virtual const std::list<Node>& getNodes() const = 0;
    virtual const std::vector<Edge>& getEdges() const = 0;
    virtual GraphKind graphKind() const = 0;
};

class IGraphOperations {
public:
    virtual ~IGraphOperations() = default;
    virtual int addNode(QPoint p) = 0;
    virtual bool addNodeWithId(int id, QPoint p) = 0;
    virtual bool setNodePosition(int id, QPoint p) = 0;
    virtual bool removeNode(int id) = 0;
    virtual bool addEdge(int from, int to, int cost = 1) = 0;
    virtual bool removeEdge(int from, int to) = 0;
    virtual void clear() = 0;
};

class IGraphComplete : public IGraphData, public IGraphOperations {
public:
    virtual ~IGraphComplete() = default;
};

struct ComponentResult {
    int numComponents = 0;
    std::unordered_map<int, int> nodeToComponent;
};

struct CondensedEdge { int from = 0; int to = 0; };
struct CondensedGraph {
    std::list<Node> nodes;
    std::vector<CondensedEdge> edges;
};
struct CondensedResult {
    bool isShowing = false;
    CondensedGraph graph;
};

#endif
