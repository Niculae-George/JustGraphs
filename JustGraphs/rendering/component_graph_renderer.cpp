#include "rendering/component_graph_renderer.h"
#include "rendering/graph_primitives.h"
#include "rendering/component_color_provider.h"
#include "domain/graph_interfaces.h"
#include "ui/theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <unordered_map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ComponentGraphRenderer::ComponentGraphRenderer()
    : m_colorProvider(std::make_unique<ComponentColorProvider>())
{
}

void ComponentGraphRenderer::setColorProvider(std::unique_ptr<IComponentColorProvider> provider) {
    m_colorProvider = std::move(provider);
}

void ComponentGraphRenderer::render(QPainter& p, const Graph& graph) {
    AnimationState emptyState;
    MutableRenderSettings settings = m_settings;
    settings.directed = graph.graphKind() == GraphKind::Directed;
    renderWithState(p, graph, emptyState, settings, m_selectedNodeId);
}

void ComponentGraphRenderer::renderResults(QPainter& p, const Graph& graph,
                                           const ComponentResult& compResults,
                                           const CondensedResult& condensedResults) {
    if (m_condensedViewActive && condensedResults.isShowing && compResults.numComponents > 0) {
        renderCondensedGraph(p, condensedResults.graph, compResults);
    } else if (compResults.numComponents > 0) {
        renderWithComponents(p, graph, compResults);
    } else {
        render(p, graph);
    }
}

void ComponentGraphRenderer::renderWithComponents(QPainter& p, const Graph& graph,
                                                  const ComponentResult& compResult) {
    m_settings.directed = graph.graphKind() == GraphKind::Directed;
    if (m_colorProvider)
        m_colorProvider->setComponentCount(compResult.numComponents);
    // Background
    p.fillRect(p.viewport(), m_settings.backgroundColor);
    p.setRenderHint(QPainter::Antialiasing);
    
    // Draw edges with component colors
    for (const auto& edge : graph.getEdges()) {
        int uid = edge.getFirst().getIndex();
        auto it = compResult.nodeToComponent.find(uid);
        int comp = (it != compResult.nodeToComponent.end()) ? it->second : -1;
        drawComponentEdge(p, edge, comp, compResult.numComponents);
    }
    
    // Draw nodes with component colors
    for (const auto& node : graph.getNodes()) {
        int id = node.getIndex();
        auto it = compResult.nodeToComponent.find(id);
        int comp = (it != compResult.nodeToComponent.end()) ? it->second : -1;
        bool isSelected = (id == m_selectedNodeId);
        drawComponentNode(p, node, comp, compResult.numComponents);
        
        // Draw selection border if selected
        if (isSelected) {
            QPointF center(node.getX(), node.getY());
            float radius = static_cast<float>(m_settings.nodeRadius) + 3;
            p.setPen(QPen(m_settings.nodeSelectedColor, 3));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(center, radius, radius);
        }
    }
}

void ComponentGraphRenderer::renderCondensedGraph(QPainter& p, const CondensedGraph& condensed,
                                                  const ComponentResult& compResult) {
    if (m_colorProvider)
        m_colorProvider->setComponentCount(compResult.numComponents);
    // Background
    p.fillRect(p.viewport(), m_settings.backgroundColor);
    p.setRenderHint(QPainter::Antialiasing);
    
    constexpr int nodeWidth = 80, nodeHeight = 40;
    constexpr double a = nodeWidth / 2.0, b = nodeHeight / 2.0;
    const double arrowSize = 15.0;
    std::unordered_map<int, QPointF> centers;
    for (const auto& node : condensed.nodes)
        centers.emplace(node.getIndex(), QPointF(node.getX(), node.getY()));
    
    // Draw edges between components
    for (const auto& edge : condensed.edges) {
        auto from = centers.find(edge.from), to = centers.find(edge.to);
        if (from == centers.end() || to == centers.end()) continue;
        QPointF startCenter = from->second;
        QPointF endCenter = to->second;
        
        QPointF direction = endCenter - startCenter;
        double length = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());
        
        if (length > 0) {
            QPointF unitDirection = direction / length;
            double t1 = 1.0 / std::sqrt((unitDirection.x() * unitDirection.x()) / (a * a) +
                                         (unitDirection.y() * unitDirection.y()) / (b * b));
            QPointF adjStart = startCenter + t1 * unitDirection;
            
            QPointF reverseDirection = startCenter - endCenter;
            QPointF unitRv = reverseDirection / length;
            double t2 = 1.0 / std::sqrt((unitRv.x() * unitRv.x()) / (a * a) + 
                                        (unitRv.y() * unitRv.y()) / (b * b));
            QPointF adjEnd = endCenter + t2 * unitRv;
            
            double angle = std::atan2(adjEnd.y() - adjStart.y(), adjEnd.x() - adjStart.x());
            
            int comp = edge.from - 1;
            QColor edgeColor = getComponentColor(comp, compResult.numComponents);
            
            QPen edgePen(edgeColor, 3);
            p.setPen(edgePen);
            p.drawLine(adjStart, adjEnd);
            
            // Arrow head
            QPointF a1(adjEnd.x() - arrowSize * std::cos(angle - M_PI / 6),
                       adjEnd.y() - arrowSize * std::sin(angle - M_PI / 6));
            QPointF a2(adjEnd.x() - arrowSize * std::cos(angle + M_PI / 6),
                       adjEnd.y() - arrowSize * std::sin(angle + M_PI / 6));
            
            p.setBrush(edgeColor);
            p.drawPolygon(QPolygonF() << adjEnd << a1 << a2);
        }
    }
    
    // Draw component nodes (as ellipses)
    for (const auto& node : condensed.nodes) {
        QRect r(node.getX() - nodeWidth / 2, node.getY() - nodeHeight / 2, 
                nodeWidth, nodeHeight);
        
        int comp = node.getIndex() - 1;
        QColor nodeColor = getComponentColor(comp, compResult.numComponents);
        
        p.setPen(QPen(nodeColor, 3));
        p.setBrush(QBrush(nodeColor.lighter(185)));
        p.drawEllipse(r);
        
        // Build label with all nodes in this component
        QString label;
        bool first = true;
        for (const auto& origNode : compResult.nodeToComponent) {
            if (origNode.second == comp) {
                if (!first) label += ",";
                label += QString::number(origNode.first);
                first = false;
            }
        }
        
        p.setPen(GraphUI::Text);
        QFont font = p.font();
        font.setPointSize(10);
        font.setBold(true);
        p.setFont(font);
        p.drawText(r, Qt::AlignCenter, label);
    }
}

QColor ComponentGraphRenderer::getComponentColor(int component, int totalComponents) const {
    Q_UNUSED(totalComponents);
    return m_colorProvider ? m_colorProvider->getColorForComponent(component)
                           : QColor(128, 148, 174);
}

void ComponentGraphRenderer::drawComponentNode(QPainter& p, const Node& node, 
                                               int component, int totalComponents) {
    QPointF center(node.getX(), node.getY());
    float radius = static_cast<float>(m_settings.nodeRadius);
    
    QColor baseColor = getComponentColor(component, totalComponents);
    
    // Node fill with gradient
    QRadialGradient grad(center, radius);
    grad.setColorAt(0, baseColor.lighter(185));
    grad.setColorAt(1, baseColor.lighter(150));
    
    QPainterPath nodePath;
    nodePath.addEllipse(center, radius, radius);
    
    p.setPen(Qt::NoPen);
    p.setBrush(grad);
    p.drawPath(nodePath);
    
    // Border
    p.setPen(QPen(baseColor.darker(130), 1.5f));
    p.setBrush(Qt::NoBrush);
    p.drawPath(nodePath);
    
    // Node index label
    p.setPen(m_settings.textColor);
    QFont nodeFont = p.font();
    nodeFont.setPointSize(9);
    nodeFont.setBold(true);
    p.setFont(nodeFont);
    p.drawText(QRectF(center.x() - radius, center.y() - radius,
                      radius * 2, radius * 2),
               Qt::AlignCenter, QString::number(node.getIndex()));
}

void ComponentGraphRenderer::drawComponentEdge(QPainter& p, const Edge& edge,
                                               int component, int totalComponents) {
    QPointF A(edge.getFirst().getX(), edge.getFirst().getY());
    QPointF B(edge.getSecond().getX(), edge.getSecond().getY());
    
    double angle = std::atan2(B.y() - A.y(), B.x() - A.x());
    
    // Adjust endpoints to node boundary
    double nodeRadius = m_settings.nodeRadius;
    QPointF startPoint(A.x() + nodeRadius * std::cos(angle),
                       A.y() + nodeRadius * std::sin(angle));
    QPointF endPoint(B.x() - nodeRadius * std::cos(angle),
                     B.y() - nodeRadius * std::sin(angle));
    
    QColor edgeColor = getComponentColor(component, totalComponents);
    
    QPen edgePen(edgeColor, m_settings.edgeWidth, Qt::SolidLine, Qt::RoundCap);
    GraphPrimitives::drawEdgeSegment(p, startPoint, endPoint, edgePen.color(), edgePen.widthF(), edgePen.style());
    
    // Directional arrow for directed graphs
    if (m_settings.directed) {
        double arrowSize = 10.0;
        QPointF a1(endPoint.x() - arrowSize * std::cos(angle - M_PI / 6),
                   endPoint.y() - arrowSize * std::sin(angle - M_PI / 6));
        QPointF a2(endPoint.x() - arrowSize * std::cos(angle + M_PI / 6),
                   endPoint.y() - arrowSize * std::sin(angle + M_PI / 6));
        
        p.setBrush(edgeColor);
        p.drawPolygon(QPolygonF() << endPoint << a1 << a2);
    }
}
