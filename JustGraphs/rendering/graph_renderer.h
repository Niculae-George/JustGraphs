#ifndef I_GRAPH_RENDERER_H
#define I_GRAPH_RENDERER_H

#include <QPainter>
#include "domain/graph.h"

class IGraphRenderer {
public:
    virtual ~IGraphRenderer() = default;
    virtual void render(QPainter& p, const Graph& graph) = 0;
};

#endif // I_GRAPH_RENDERER_H
