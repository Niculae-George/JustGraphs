#include "ui/graph_edit_interaction.h"
#include <algorithm>
#include <cmath>
#include <limits>

void GraphEditInteraction::setMode(Mode mode) {
    if (m_mode == mode) return;
    cancel();
    m_mode = mode;
}

int GraphEditInteraction::hitEdge(const QVector<EdgeSegment>& edges, const QPointF& position, qreal tolerance) {
    if (!std::isfinite(tolerance) || tolerance < 0) return -1;
    qreal best = tolerance * tolerance, bestTarget = std::numeric_limits<qreal>::infinity();
    int hit = -1;
    for (qsizetype i = 0; i < edges.size(); ++i) {
        const auto& edge = edges[i];
        const QPointF segment = edge.end - edge.start;
        const qreal lengthSquared = QPointF::dotProduct(segment, segment);
        const qreal t = lengthSquared > 0 ? std::clamp(QPointF::dotProduct(position - edge.start, segment) / lengthSquared, qreal(0), qreal(1)) : 0;
        const QPointF delta = position - (edge.start + t * segment);
        const qreal distance = QPointF::dotProduct(delta, delta);
        const QPointF target = position - edge.end;
        const qreal targetDistance = QPointF::dotProduct(target, target);
        // Overlapping opposite arcs can be selected near their target arrow.
        if (distance < best || (distance == best && targetDistance < bestTarget)) {
            best = distance;
            bestTarget = targetDistance;
            hit = int(i);
        }
    }
    return hit;
}

int GraphEditInteraction::hitTest(const QVector<NodePosition>& nodes, const QPoint& position, int radius) {
    const int radiusSquared = radius * radius;
    for (const auto& node : nodes) {
        const QPoint delta = position - node.position;
        if (delta.x() * delta.x() + delta.y() * delta.y() <= radiusSquared)
            return node.id;
    }
    return -1;
}

void GraphEditInteraction::press(int nodeId, const QPoint& cursorPosition, const QPoint& nodePosition) {
    if (m_mode == Mode::Delete) return;
    m_pressPosition = cursorPosition;
    m_draggedNodeId = nodeId;
    m_nodeOffset = nodeId >= 0 ? nodePosition - cursorPosition : QPoint();
    m_moved = false;
}

QPoint GraphEditInteraction::draggedPosition(const QPoint& cursorPosition) const {
    return cursorPosition + m_nodeOffset;
}

bool GraphEditInteraction::move(const QPoint& position, int dragThreshold) {
    if (m_mode == Mode::Delete) return false;
    if (m_draggedNodeId < 0 || m_moved)
        return m_moved;
    const QPoint delta = position - m_pressPosition;
    m_moved = delta.x() * delta.x() + delta.y() * delta.y() >= dragThreshold * dragThreshold;
    return m_moved;
}

GraphEditInteraction::Release GraphEditInteraction::release(int hitNodeId) {
    if (m_mode == Mode::Delete) return Release::None;
    Release result = Release::None;
    if (m_draggedNodeId >= 0)
        result = m_moved ? Release::DraggedNode : Release::ClickedNode;
    else if (hitNodeId >= 0)
        result = Release::ClickedNode;
    else
        result = Release::ClickedEmpty;

    if (result == Release::ClickedNode && hitNodeId >= 0) {
        m_selectedNodeId = m_selectedNodeId == hitNodeId ? -1 : hitNodeId;
    } else if (result == Release::ClickedEmpty) m_selectedNodeId = -1;
    m_draggedNodeId = -1;
    m_moved = false;
    return result;
}

void GraphEditInteraction::cancel() {
    m_selectedNodeId = -1;
    m_draggedNodeId = -1;
    m_moved = false;
}
