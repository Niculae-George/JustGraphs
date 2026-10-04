#include "rendering/component_color_provider.h"

// Categorical colors stay distinct on pale node fills and graph canvases.
const std::vector<QColor> ComponentColorProvider::s_palette = {
    QColor(52, 120, 184),  // blue
    QColor(211, 132, 34),  // orange
    QColor(59, 140, 90),   // green
    QColor(200, 77, 84),   // red
    QColor(128, 100, 184), // purple
    QColor(50, 143, 159),  // cyan
    QColor(182, 80, 131),  // magenta
    QColor(135, 145, 58),  // olive
    QColor(80, 107, 192),  // indigo
    QColor(154, 106, 69),  // brown
    QColor(41, 151, 125),  // teal
    QColor(103, 127, 155)  // slate blue
};

ComponentColorProvider::ComponentColorProvider() = default;

QColor ComponentColorProvider::getColorForComponent(int componentId) const {
    if (componentId < 0 || componentId >= m_componentCount || s_palette.empty()) {
        return QColor(128, 148, 174);
    }
    return s_palette[componentId % s_palette.size()];
}

void ComponentColorProvider::setComponentCount(int count) {
    m_componentCount = count;
}
