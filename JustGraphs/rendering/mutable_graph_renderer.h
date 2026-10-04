#ifndef MUTABLE_GRAPH_RENDERER_H
#define MUTABLE_GRAPH_RENDERER_H

#include "rendering/graph_renderer.h"
#include "ui/theme.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <utility>

// Animation state for mutable graph rendering
struct AnimationState {
    std::unordered_set<int> visitedNodes;
    std::unordered_set<int> activeNodes;
    std::unordered_set<int> articulationPoints;
    std::vector<std::pair<int, int>> bridges;
    std::pair<int, int> activeEdge = {-1, -1};
    int activeNode = -1;
    int activeNode2 = -1;
    float pulseValue = 0.0f;
    bool showResult = false;
    bool isAnimating = false;
    
    // Articulation bridge specific
    std::unordered_map<int, int> discValues;
    std::unordered_map<int, int> lowValues;
};

// Render settings for mutable graphs on the shared light canvas.
struct MutableRenderSettings {
    QColor backgroundColor = GraphUI::Canvas;
    
    QColor nodeColor = GraphUI::BlueSoft;
    QColor nodeVisitedColor = QColor(83, 139, 211);
    QColor nodeActiveColor = GraphUI::Blue;
    QColor nodeSelectedColor = GraphUI::Success;
    QColor nodeAPColor = GraphUI::Error;
    
    QColor edgeColor = QColor(128, 148, 174);
    QColor edgeTreeColor = QColor(83, 139, 211);
    QColor edgeActiveColor = GraphUI::Blue;
    QColor edgeBridgeColor = GraphUI::Warning;
    
    QColor textColor = GraphUI::Text;
    QColor labelColor = GraphUI::Muted;
    
    int nodeRadius = 18;
    int edgeWidth = 2;
    int activeEdgeWidth = 3;
    int bridgeEdgeWidth = 4;
    bool directed = false;
    bool showDiscLowLabels = false;  
};

class MutableGraphRenderer : public IGraphRenderer {
public:
    void render(QPainter& p, const Graph& graph) override;
    
    // Extended render with animation state and settings
    void renderWithState(QPainter& p, const Graph& graph, 
                         const AnimationState& animState,
                         const MutableRenderSettings& settings = MutableRenderSettings(),
                         int selectedNodeId = -1);
    
    // Draw individual elements (for custom compositions)
    void drawNode(QPainter& p, const Node& node, const MutableRenderSettings& settings,
                  const AnimationState& animState, bool isSelected = false);
    
    void drawEdge(QPainter& p, const Edge& edge, const MutableRenderSettings& settings,
                  const AnimationState& animState);
    
    // Draw pending edge (for edge creation UI)
    void drawPendingEdge(QPainter& p, const QPoint& from, const QPoint& to);
    
protected:  // Allow derived classes to access these
    void drawBackground(QPainter& p, const QRect& rect, const QColor& color);
    void drawArrowHead(QPainter& p, const QPointF& tip, double angle, double arrowSize, const QColor& color);
    bool isNodeActive(int nodeId, const AnimationState& state) const;
    bool isEdgeBridge(int u, int v, const AnimationState& state) const;
    bool isEdgeActive(int u, int v, const AnimationState& state) const;
    QColor getNodeColor(const Node& node, const MutableRenderSettings& settings, 
                        const AnimationState& state, bool isSelected) const;
    QColor getEdgeColor(const Edge& edge, const MutableRenderSettings& settings,
                        const AnimationState& state) const;
};

#endif // MUTABLE_GRAPH_RENDERER_H
