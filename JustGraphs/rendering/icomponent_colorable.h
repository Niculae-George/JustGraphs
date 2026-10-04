#ifndef ICOMPONENT_COLORABLE_H
#define ICOMPONENT_COLORABLE_H

#include <QPainter>
#include "domain/graph.h"

// Forward declarations
struct ComponentResult;
struct CondensedGraph;

// Pure interface for component color palette
class IComponentColorProvider {
public:
    virtual ~IComponentColorProvider() = default;
    
    virtual QColor getColorForComponent(int componentId) const = 0;
    virtual void setComponentCount(int count) = 0;
};

// Interface for renderers that support component coloring
// ISP: Separates component coloring concern from basic graph rendering
class IComponentColorable {
public:
    virtual ~IComponentColorable() = default;
    
    // Render graph with component coloring
    virtual void renderWithComponents(QPainter& p, const Graph& graph,
                                      const ComponentResult& compResult) = 0;
    
    // Render condensed graph (for SCC)
    virtual void renderCondensedGraph(QPainter& p, const CondensedGraph& condensed,
                                        const ComponentResult& compResult) = 0;
    
    // Check if condensed view is currently active
    virtual bool isCondensedViewActive() const = 0;
    
    // Toggle condensed view
    virtual void setCondensedViewActive(bool active) = 0;
};

#endif // ICOMPONENT_COLORABLE_H
