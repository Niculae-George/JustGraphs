#pragma once

#include <QIODevice>
#include <QPointF>
#include <QRectF>
#include <QString>

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Luxembourg {

struct RoadNode {
    int id = 0;
    QPointF position;
};

struct RoadArc {
    int from = 0;
    int to = 0;
    qint64 length = 0;
};

class Map {
public:
    using Adjacency = std::vector<std::vector<std::pair<int, qint64>>>;
    const std::vector<RoadNode>& nodes() const { return nodes_; }
    const std::vector<RoadArc>& arcs() const { return arcs_; }
    const QRectF& bounds() const { return bounds_; }
    const Adjacency& adjacency() const { return adjacency_; }
    const Adjacency& reverseAdjacency() const { return reverseAdjacency_; }

    bool load(QIODevice& input, QString& error);
    int nearestNode(QPointF worldPosition) const;

private:
    std::vector<RoadNode> nodes_;
    std::vector<RoadArc> arcs_;
    QRectF bounds_;
    Adjacency adjacency_;
    Adjacency reverseAdjacency_;
};

} // namespace Luxembourg
