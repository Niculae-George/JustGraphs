#ifndef GRAPH_EDIT_INTERACTION_H
#define GRAPH_EDIT_INTERACTION_H

#include <QPoint>
#include <QPointF>
#include <QVector>

// Widget-independent state machine for click-to-connect and drag-to-move.
// Widgets retain ownership of graph mutations and decide their hit radius.
class GraphEditInteraction {
public:
    struct NodePosition { int id; QPoint position; };
    struct EdgeSegment { int from, to; QPointF start, end; };
    enum class Mode { Edit, Delete };
    void setMode(Mode mode);
    Mode mode() const { return m_mode; }
    static int hitEdge(const QVector<EdgeSegment>& edges, const QPointF& position, qreal tolerance = 8);
    enum class Release { None, ClickedNode, DraggedNode, ClickedEmpty };

    static int hitTest(const QVector<NodePosition>& nodes, const QPoint& position, int radius);
    void press(int nodeId, const QPoint& cursorPosition, const QPoint& nodePosition = QPoint());
    bool move(const QPoint& position, int dragThreshold = 6);
    QPoint draggedPosition(const QPoint& cursorPosition) const;
    Release release(int hitNodeId);
    void cancel();

    int selectedNodeId() const { return m_selectedNodeId; }
    int draggedNodeId() const { return m_draggedNodeId; }
    bool dragging() const { return m_draggedNodeId >= 0; }
    bool moved() const { return m_moved; }

private:
    Mode m_mode = Mode::Edit;
    int m_selectedNodeId = -1;
    int m_draggedNodeId = -1;
    QPoint m_pressPosition;
    QPoint m_nodeOffset;
    bool m_moved = false;
};

#endif
