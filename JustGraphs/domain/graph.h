#ifndef GRAPH_H
#define GRAPH_H
#include "domain/edge.h"
#include "domain/graph_interfaces.h"
#include <list>
#include <vector>
#include <cstdint>

class Graph : public IGraphComplete {
protected:
    std::list<Node> m_nodes;
    std::vector<Edge> m_edges;
    std::int64_t m_nextNodeId = 1;
    const Node* findNodePtr(int id) const;
    Node* findNodePtr(int id);
public:
    Graph() = default;
    virtual ~Graph() = default;
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;
    Graph(Graph&&) = delete;
    Graph& operator=(Graph&&) = delete;

    int addNode(QPoint p) override;
    bool addNodeWithId(int id, QPoint p) override;
    bool setNodePosition(int id, QPoint p) override;
    bool removeNode(int id) override;
    bool removeEdge(int from, int to) override;
    bool addEdge(int from, int to, int cost = 1) override = 0;
    bool addEdge(const Node& from, const Node& to, int cost = 1);
    void clear() override;
    const Node* findNode(int id) const;
    const std::list<Node>& getNodes() const override;
    const std::vector<Edge>& getEdges() const override;
};

#endif
