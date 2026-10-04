#include "domain/directedgraph.h"
#include "domain/undirectedgraph.h"
#include "rendering/component_graph_renderer.h"
#include "rendering/icomponent_colorable.h"
#include "ui/graph_edit_interaction.h"
#include "ui/playback_state.h"

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QTextStream>

namespace {
int failures = 0;
void check(bool condition, const char* message) {
    if (condition) return;
    QTextStream(stderr) << "FAIL: " << message << '\n';
    ++failures;
}

bool hasInk(const QImage& image, const QRect& area, const QColor& background) {
    const QRect clipped = area.intersected(image.rect());
    for (int y = clipped.top(); y <= clipped.bottom(); ++y)
        for (int x = clipped.left(); x <= clipped.right(); ++x)
            if (image.pixelColor(x, y) != background) return true;
    return false;
}
}

class TestColorProvider final : public IComponentColorProvider {
public:
    QColor getColorForComponent(int component) const override {
        return component == 0 ? QColor(255, 0, 255) : QColor(0, 255, 255);
    }
    void setComponentCount(int count) override { componentCount = count; }
    int componentCount = -1;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    GraphEditInteraction edit;
    QVector<GraphEditInteraction::NodePosition> nodes{{7, QPoint(20, 20)}};
    check(GraphEditInteraction::hitTest(nodes, QPoint(23, 24), 5) == 7, "gesture hit test uses node IDs");
    edit.press(7, QPoint(23, 24), QPoint(20, 20));
    check(!edit.move(QPoint(28, 24), 6), "drag threshold is respected");
    check(edit.move(QPoint(29, 24), 6), "movement beyond threshold starts drag");
    check(edit.draggedPosition(QPoint(26, 20)) == QPoint(23, 16), "drag preserves cursor offset");
    check(edit.release(7) == GraphEditInteraction::Release::DraggedNode, "drag release reports moved node");
    check(edit.selectedNodeId() == -1, "drag does not leave node selected");
    edit.press(7, QPoint(20, 20), QPoint(20, 20));
    check(edit.release(7) == GraphEditInteraction::Release::ClickedNode, "click release reports node selection");
    check(edit.selectedNodeId() == 7, "node click selects node ID");
    edit.press(-1, QPoint(0, 0));
    check(edit.release(-1) == GraphEditInteraction::Release::ClickedEmpty, "empty click clears selection");
    check(edit.selectedNodeId() == -1, "empty click leaves no selected node");

    edit.press(7, QPoint(20, 20)); edit.release(7);
    edit.setMode(GraphEditInteraction::Mode::Delete);
    check(edit.selectedNodeId() == -1 && !edit.dragging(), "changing mode cancels pending gestures");
    edit.press(7, QPoint(20, 20));
    check(!edit.move(QPoint(40, 20)) && edit.release(7) == GraphEditInteraction::Release::None, "delete mode cannot drag or connect");
    edit.cancel();
    check(edit.mode() == GraphEditInteraction::Mode::Delete, "cancel preserves mode");
    edit.setMode(GraphEditInteraction::Mode::Edit);
    edit.press(7, QPoint(20, 20));
    check(edit.release(7) == GraphEditInteraction::Release::ClickedNode, "edit behavior resumes after mode switch");
    using Segment = GraphEditInteraction::EdgeSegment;
    QVector<Segment> edges{{1, 2, {0, 0}, {100, 100}}};
    check(GraphEditInteraction::hitEdge(edges, {50, 54}) == 0, "diagonal edge hit accepts nearby click");
    check(GraphEditInteraction::hitEdge(edges, {120, 120}) == -1, "edge extension beyond endpoint is not hit");
    check(GraphEditInteraction::hitEdge(edges, {50, 70}) == -1, "distant click misses edge");
    edges = {{1, 2, {0, 0}, {100, 0}}, {2, 1, {100, 0}, {0, 0}}};
    check(GraphEditInteraction::hitEdge(edges, {75, 0}) == 0 && GraphEditInteraction::hitEdge(edges, {25, 0}) == 1,
          "overlapping opposite arcs selected near target endpoint");
    edges = {{1, 2, {0, -11}, {100, -11}}, {2, 1, {100, 11}, {0, 11}}};
    check(GraphEditInteraction::hitEdge(edges, {50, -9}) == 0 && GraphEditInteraction::hitEdge(edges, {50, 9}) == 1,
          "parallel offset flow directions independently hit");
    edges = {{1, 2, {10, 10}, {10, 10}}};
    check(GraphEditInteraction::hitEdge(edges, {10, 10}, 0) == 0, "degenerate segment safe");
    check(GraphEditInteraction::hitEdge(edges, {10, 10}, -1) == -1, "negative tolerance rejected");

    PlaybackState playback;
    check(playback.start() && playback.active(), "playback enters running phase");
    check(playback.pause() && playback.phase() == PlaybackState::Phase::Paused, "playback pauses");
    check(playback.resume(), "playback resumes");
    playback.complete();
    check(playback.phase() == PlaybackState::Phase::Completed, "playback completes");
    playback.cancel();
    check(playback.phase() == PlaybackState::Phase::Idle, "playback cancellation returns to idle");

    DirectedGraph graph;
    const int first = graph.addNode(QPoint(30, 50));
    const int second = graph.addNode(QPoint(130, 50));
    check(graph.addEdge(first, second), "directed graph accepts valid edge");

    auto provider = std::make_unique<TestColorProvider>();
    auto* providerView = provider.get();
    ComponentGraphRenderer renderer;
    renderer.settings().nodeRadius = 10;
    renderer.settings().edgeWidth = 2;
    renderer.setColorProvider(std::move(provider));
    ComponentResult components;
    components.numComponents = 2;
    components.nodeToComponent.emplace(first, 0);
    components.nodeToComponent.emplace(second, 1);
    QImage image(160, 100, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    const Graph& readOnlyGraph = graph;
    const ComponentResult copiedComponents = components;
    renderer.renderResults(painter, readOnlyGraph, copiedComponents, CondensedResult{});
    painter.end();
    check(providerView->componentCount == 2, "custom color provider receives component count");
    const QColor customEdge = image.pixelColor(80, 50);
    check(customEdge.red() > 245 && customEdge.blue() > 245 && customEdge.green() < 10,
          "custom color provider controls component edge color");
    check(hasInk(image, QRect(112, 44, 6, 5), renderer.settings().backgroundColor),
          "component-result rendering draws arrow for directed graph");

    renderer.setColorProvider(nullptr);
    QImage fallbackImage(160, 100, QImage::Format_ARGB32_Premultiplied);
    fallbackImage.fill(Qt::white);
    QPainter fallbackPainter(&fallbackImage);
    renderer.renderResults(fallbackPainter, readOnlyGraph, copiedComponents, CondensedResult{});
    fallbackPainter.end();
    const QColor fallbackEdge = fallbackImage.pixelColor(80, 50);
    check(fallbackEdge.red() == 128 && fallbackEdge.green() == 148 && fallbackEdge.blue() == 174,
          "null color provider uses documented neutral fallback");

    QImage plainImage(160, 100, QImage::Format_ARGB32_Premultiplied);
    plainImage.fill(Qt::white);
    QPainter plainPainter(&plainImage);
    ComponentGraphRenderer plainRenderer;
    plainRenderer.settings().nodeRadius = 10;
    plainRenderer.render(plainPainter, readOnlyGraph);
    plainPainter.end();
    check(hasInk(plainImage, QRect(112, 44, 6, 5), plainRenderer.settings().backgroundColor),
          "plain render draws arrow on directed graph edge");

    UndirectedGraph undirected;
    const int u = undirected.addNode(QPoint(30, 50));
    const int v = undirected.addNode(QPoint(130, 50));
    undirected.addEdge(u, v);
    QImage undirectedImage(160, 100, QImage::Format_ARGB32_Premultiplied);
    undirectedImage.fill(Qt::white);
    QPainter undirectedPainter(&undirectedImage);
    plainRenderer.render(undirectedPainter, undirected);
    undirectedPainter.end();
    check(!hasInk(undirectedImage, QRect(112, 44, 6, 5), plainRenderer.settings().backgroundColor),
          "plain render omits arrow on undirected graph edge");

    QImage componentUndirectedImage(160, 100, QImage::Format_ARGB32_Premultiplied);
    componentUndirectedImage.fill(Qt::white);
    QPainter componentUndirectedPainter(&componentUndirectedImage);
    renderer.renderWithComponents(componentUndirectedPainter, undirected, copiedComponents);
    componentUndirectedPainter.end();
    check(!hasInk(componentUndirectedImage, QRect(112, 44, 6, 5), renderer.settings().backgroundColor),
          "component-result rendering omits arrow for undirected graph");

    MutableGraphRenderer stateRenderer;
    MutableRenderSettings contradictorySettings;
    contradictorySettings.nodeRadius = 10;
    contradictorySettings.directed = false;
    QImage stateDirectedImage(160, 100, QImage::Format_ARGB32_Premultiplied);
    stateDirectedImage.fill(Qt::white);
    QPainter stateDirectedPainter(&stateDirectedImage);
    stateRenderer.renderWithState(stateDirectedPainter, readOnlyGraph, AnimationState{}, contradictorySettings);
    stateDirectedPainter.end();
    check(hasInk(stateDirectedImage, QRect(112, 44, 6, 5), contradictorySettings.backgroundColor),
          "renderWithState derives directed arrows from graph kind over contradictory settings");

    contradictorySettings.directed = true;
    QImage stateUndirectedImage(160, 100, QImage::Format_ARGB32_Premultiplied);
    stateUndirectedImage.fill(Qt::white);
    QPainter stateUndirectedPainter(&stateUndirectedImage);
    stateRenderer.renderWithState(stateUndirectedPainter, undirected, AnimationState{}, contradictorySettings);
    stateUndirectedPainter.end();
    check(!hasInk(stateUndirectedImage, QRect(112, 44, 6, 5), contradictorySettings.backgroundColor),
          "renderWithState suppresses arrows for undirected graph over contradictory settings");

    QImage fallbackPlainImage(160, 100, QImage::Format_ARGB32_Premultiplied);
    fallbackPlainImage.fill(Qt::white);
    QPainter fallbackPlainPainter(&fallbackPlainImage);
    ComponentResult noComponents;
    renderer.renderResults(fallbackPlainPainter, readOnlyGraph, noComponents, CondensedResult{});
    fallbackPlainPainter.end();
    check(hasInk(fallbackPlainImage, QRect(112, 44, 6, 5), renderer.settings().backgroundColor),
          "empty component result falls back to graph-only directed rendering");

    return failures == 0 ? 0 : 1;
}
