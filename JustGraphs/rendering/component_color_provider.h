#ifndef COMPONENT_COLOR_PROVIDER_H
#define COMPONENT_COLOR_PROVIDER_H

#include "rendering/icomponent_colorable.h"
#include <QColor>
#include <vector>

// Concrete implementation of IComponentColorProvider
// Uses a restrained categorical palette with twelve distinct hues.
class ComponentColorProvider : public IComponentColorProvider {
public:
    ComponentColorProvider();
    
    // IComponentColorProvider implementation
    QColor getColorForComponent(int componentId) const override;
    void setComponentCount(int count) override;

private:
    int m_componentCount = 0;
    
    // Twelve distinct colors, cycled for larger component counts.
    static const std::vector<QColor> s_palette;
};

#endif // COMPONENT_COLOR_PROVIDER_H
