#ifndef FLOWNETWORK_H
#define FLOWNETWORK_H

#include <vector>
#include <QPointF>

// Editable flow-network data. IDs are one-based; -1 means endpoint unset.
// This class owns no algorithm state or presentation text.
class FlowNetwork {
public:
    FlowNetwork() = default;

    int addNode(const QPointF& pos);
    bool setNodePosition(int node, const QPointF& pos);
    bool removeNode(int node);
    bool addEdge(int from, int to, int capacity);
    bool removeEdge(int from, int to);
    bool setCapacity(int from, int to, int capacity);
    bool setEdgeCost(int from, int to, int cost);
    void clear();

    int getNumNodes() const;
    const std::vector<QPointF>& getNodePositions() const;
    int getCapacity(int from, int to) const;
    int getEdgeCost(int from, int to) const;
    bool setSource(int source);
    bool setSink(int sink);
    int getSource() const;
    int getSink() const;
    bool hasValidEndpoints() const;
    unsigned long long topologyRevision() const;

private:
    int m_source = -1;
    int m_sink = -1;
    std::vector<std::vector<int>> m_capacity;
    std::vector<std::vector<int>> m_cost;
    std::vector<QPointF> m_nodePositions;
    unsigned long long m_topologyRevision = 0;

    bool validNode(int node) const;
    bool validEndpoints(int from, int to) const;
    void topologyChanged();
};

#endif // FLOWNETWORK_H
