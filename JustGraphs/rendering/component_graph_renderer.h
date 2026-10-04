#ifndef COMPONENT_GRAPH_RENDERER_H
#define COMPONENT_GRAPH_RENDERER_H

#include "rendering/mutable_graph_renderer.h"
#include "rendering/icomponent_colorable.h"
#include <memory>

// Forward declaration
class IComponentColorProvider;

// Renderer for graphs with component coloring support
// Extends MutableGraphRenderer and implements IComponentColorable
// ISP: Separates component coloring from basic graph rendering
class ComponentGraphRenderer : public MutableGraphRenderer, public IComponentColorable {
public:
    ComponentGraphRenderer();
    
    // IGraphRenderer implementation
    void render(QPainter& p, const Graph& graph) override;
    void renderResults(QPainter& p, const Graph& graph,
                       const ComponentResult& compResults,
                       const CondensedResult& condensedResults);
    
    // IComponentColorable implementation
    void renderWithComponents(QPainter& p, const Graph& graph,
                              const ComponentResult& compResult) override;
    
    void renderCondensedGraph(QPainter& p, const CondensedGraph& condensed,
                              const ComponentResult& compResult) override;
    
    bool isCondensedViewActive() const override { return m_condensedViewActive; }
    void setCondensedViewActive(bool active) override { m_condensedViewActive = active; }
    
    // Set render settings
    void setSettings(const MutableRenderSettings& settings) { m_settings = settings; }
    MutableRenderSettings& settings() { return m_settings; }
    
    // Set selected node
    void setSelectedNodeId(int nodeId) { m_selectedNodeId = nodeId; }
    void setColorProvider(std::unique_ptr<IComponentColorProvider> provider);

private:
    bool m_condensedViewActive = false;
    int m_selectedNodeId = -1;
    MutableRenderSettings m_settings;
    std::unique_ptr<IComponentColorProvider> m_colorProvider;
    
    // Component rendering helpers
    QColor getComponentColor(int component, int totalComponents) const;
    void drawComponentNode(QPainter& p, const Node& node, int component, int totalComponents);
    void drawComponentEdge(QPainter& p, const Edge& edge, int component, int totalComponents);
};

#endif // COMPONENT_GRAPH_RENDERER_H
