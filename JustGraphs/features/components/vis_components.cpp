#include "features/components/vis_components.h"
#include "ui/theme.h"
#include "domain/undirectedgraph.h"
#include "domain/directedgraph.h"
#include "algorithms/connected_components_solver.h"
#include "algorithms/kosaraju_solver.h"
#include "algorithms/condensed_graph_solver.h"
#include "rendering/component_graph_renderer.h"
#include <QPainter>
#include <QMouseEvent>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QString>
#include <QFontMetrics>
#include <cmath>

VisComponents::VisComponents(QWidget* parent)
    : QWidget(parent), m_graph(nullptr)
{
    setMinimumSize(500, 350);
    m_graph = std::make_unique<UndirectedGraph>();
    m_renderer = std::make_unique<ComponentGraphRenderer>();

    m_checkBox = new QCheckBox("Directed graph", this);
    m_checkBox->setObjectName("componentsDirected");
    m_checkBox->setAttribute(Qt::WA_LayoutUsesWidgetRect);
    m_checkBox->ensurePolished();
    m_checkBox->setFixedWidth(qMax(m_checkBox->sizeHint().width(),
        m_checkBox->fontMetrics().horizontalAdvance(m_checkBox->text()) + 40));
    m_checkBox->setMinimumHeight(36);
    m_checkBox->setStyleSheet("QCheckBox::indicator:unchecked { background: white; "
        "border: 1px solid #9DBCE9; border-radius: 4px; }");
    connect(m_checkBox, &QCheckBox::checkStateChanged, this, &VisComponents::onCheckBoxStateChanged);

    m_btnComponents = GraphUI::button("Find connected components", this, true);
    m_btnComponents->setObjectName("componentsFind");
    connect(m_btnComponents, &QPushButton::clicked, this, &VisComponents::onFindComponents);

    m_btnToggleCondensed = GraphUI::button("Show condensed graph", this);
    m_btnToggleCondensed->setObjectName("componentsCondensed");
    m_btnToggleCondensed->setEnabled(false);
    connect(m_btnToggleCondensed, &QPushButton::clicked, this, &VisComponents::onToggleCondensedView);

    m_btnDeleteMode = GraphUI::button("Delete nodes/edges", this);
    m_btnDeleteMode->setObjectName("componentsDeleteMode");
    m_btnDeleteMode->setCheckable(true);
    m_btnDeleteMode->setProperty("role", "algorithm");
    connect(m_btnDeleteMode, &QPushButton::toggled, this, [this](bool deleting) {
        m_interaction.setMode(deleting ? GraphEditInteraction::Mode::Delete
                                       : GraphEditInteraction::Mode::Edit);
        m_btnToggleCondensed->setEnabled(!deleting && m_graph->graphKind() == GraphKind::Directed
                                         && m_compResult.numComponents > 0);
        if (deleting && m_condensedResult.isShowing) {
            m_condensedResult.isShowing = false;
            m_renderer->setCondensedViewActive(false);
            m_btnToggleCondensed->setText("Show condensed graph");
        }
        m_editHint->setText(deleting ? "Left-click a node or edge to delete it"
                                     : "Right-click to add nodes · Select two nodes to connect · Drag to move");
        update();
    });

    // Reserve the widest labels so switching graph mode keeps the row stable.
    auto reserveButtonWidth = [](QPushButton* button, const QString& longestText) {
        const QString original = button->text();
        button->setText(longestText);
        button->ensurePolished();
        button->setFixedWidth(qMax(button->sizeHint().width(),
            button->fontMetrics().horizontalAdvance(longestText) + 36));
        button->setText(original);
    };
    reserveButtonWidth(m_btnComponents, "Find strongly connected components");
    reserveButtonWidth(m_btnToggleCondensed, "Show condensed graph");

    m_toolbar = GraphUI::toolbar(this);
    delete m_toolbar->layout();
    auto* outer = new QVBoxLayout(m_toolbar);
    outer->setContentsMargins(12, 8, 12, 8);
    outer->setSpacing(4);
    auto* controls = new QHBoxLayout;
    controls->setSpacing(10);
    controls->addWidget(m_checkBox, 0, Qt::AlignVCenter);
    controls->addWidget(m_btnComponents, 0, Qt::AlignVCenter);
    controls->addWidget(m_btnToggleCondensed, 0, Qt::AlignVCenter);
    controls->addStretch(1);
    m_editHint = new QLabel("Right-click to add nodes · Select two nodes to connect · Drag to move", m_toolbar);
    m_editHint->setProperty("role", "muted");
    m_editHint->setWordWrap(true);
    auto* editHint = new QHBoxLayout;
    editHint->setSpacing(12);
    editHint->addWidget(m_btnDeleteMode, 0, Qt::AlignVCenter);
    editHint->addWidget(m_editHint, 1);
    outer->addLayout(controls);
    outer->addLayout(editHint);
    m_toolbar->setGeometry(0, 0, width(), 104);
    updateButtonText();
}

VisComponents::~VisComponents() = default;

void VisComponents::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (m_toolbar) m_toolbar->setGeometry(0, 0, width(), 104);
}

void VisComponents::onCheckBoxStateChanged(int state) {
    recreateGraph(state == Qt::Checked);
    updateButtonText(); update();
}

void VisComponents::invalidateResults() {
    m_compResult = {};
    m_condensedResult = {};
    m_renderer->setCondensedViewActive(false);
    m_btnToggleCondensed->setEnabled(false);
    m_btnToggleCondensed->setText("Show condensed graph");
}

void VisComponents::hideEvent(QHideEvent* event) {
    m_interaction.cancel();
    QWidget::hideEvent(event);
}

void VisComponents::onFindComponents() {
    if(!m_graph) return;
    int count = 0;
    if(m_graph->graphKind() == GraphKind::Undirected) {
        m_compResult = ConnectedComponentsSolver{}.solve(*m_graph);
        count = m_compResult.numComponents;
        QMessageBox::information(this, "Connected Components",
            QString("The graph has %1 connected components!").arg(count));
        m_btnToggleCondensed->setEnabled(false);
    } else if(m_graph->graphKind() == GraphKind::Directed) {
        count = KosarajuSolver{}.solve(*m_graph, m_compResult.nodeToComponent);
        m_compResult.numComponents = count;
        QMessageBox::information(this, "Strongly Connected Components",
            QString("The graph has %1 strongly connected components!").arg(count));

        m_condensedResult.graph = CondensedGraphSolver{}.solve(*m_graph, count, m_compResult.nodeToComponent);
        m_btnToggleCondensed->setEnabled(m_interaction.mode() == GraphEditInteraction::Mode::Edit);
        m_btnToggleCondensed->setText("Show condensed graph");
    }
    update();
}

void VisComponents::onToggleCondensedView() {
    if(m_compResult.numComponents > 0 && m_interaction.mode() == GraphEditInteraction::Mode::Edit) {
        m_condensedResult.isShowing = !m_condensedResult.isShowing;
        m_renderer->setCondensedViewActive(m_condensedResult.isShowing);
        m_btnToggleCondensed->setText(m_condensedResult.isShowing ? "Show normal graph" : "Show condensed graph");
        update();
    }
}

void VisComponents::updateButtonText() {
    if(m_graph->graphKind() == GraphKind::Undirected) {
        m_btnComponents->setText("Find connected components");
        m_btnToggleCondensed->setEnabled(false);
    } else if(m_graph->graphKind() == GraphKind::Directed) {
        m_btnComponents->setText("Find strongly connected components");
        m_btnToggleCondensed->setEnabled(m_compResult.numComponents > 0 &&
                                         m_interaction.mode() == GraphEditInteraction::Mode::Edit);
    }
}

void VisComponents::recreateGraph(bool directed) {
    if (!m_graph) return;
    
    struct EdgeInfo { int from; int to; int cost; };
    std::vector<std::pair<QPoint, int>> nodeInfos;
    for (const auto& n : m_graph->getNodes()) {
        nodeInfos.push_back({QPoint(n.getX(), n.getY()), n.getIndex()});
    }
    
    std::vector<EdgeInfo> edgeInfos;
    for (const auto& ed : m_graph->getEdges()) {
        edgeInfos.push_back({ed.getFirst().getIndex(), ed.getSecond().getIndex(), ed.getCost()});
    }

    if (directed) {
        m_graph = std::make_unique<DirectedGraph>();
    } else {
        m_graph = std::make_unique<UndirectedGraph>();
    }
    
    // Recreate renderer for new graph type
    m_renderer = std::make_unique<ComponentGraphRenderer>();
    m_renderer->settings().directed = directed;
    
    for (const auto& info : nodeInfos) {
        m_graph->addNodeWithId(info.second, info.first);
    }
    for (const auto& info : edgeInfos) {
        m_graph->addEdge(info.from, info.to, info.cost);
    }
    m_interaction.cancel();
    invalidateResults();
    updateButtonText();
}

void VisComponents::mouseReleaseEvent(QMouseEvent* m) {
    if(!m_graph) return;
    if (m_interaction.mode() == GraphEditInteraction::Mode::Delete) {
        if (m->button() != Qt::LeftButton || m->pos().y() < 104) { m_interaction.cancel(); return; }
        QVector<GraphEditInteraction::NodePosition> nodes;
        for (const auto& n : m_graph->getNodes()) nodes.push_back({n.getIndex(), n.getCoord()});
        const int nodeId = GraphEditInteraction::hitTest(nodes, m->pos(), m_renderer->settings().nodeRadius + 4);
        bool changed = false;
        if (nodeId >= 0) {
            changed = m_graph->removeNode(nodeId);
        } else {
            QVector<GraphEditInteraction::EdgeSegment> edges;
            for (const auto& edge : m_graph->getEdges()) {
                const auto* from = m_graph->findNode(edge.getFirst().getIndex());
                const auto* to = m_graph->findNode(edge.getSecond().getIndex());
                if (from && to) edges.push_back({edge.getFirst().getIndex(), edge.getSecond().getIndex(), from->getCoord(), to->getCoord()});
            }
            const int edgeIndex = GraphEditInteraction::hitEdge(edges, m->pos());
            if (edgeIndex >= 0) {
                const auto edge = edges[edgeIndex];
                changed = m_graph->removeEdge(edge.from, edge.to);
            }
        }
        m_interaction.cancel();
        if (changed) { invalidateResults(); updateButtonText(); update(); }
        return;
    }
    if(m->button() == Qt::RightButton) {
        QVector<GraphEditInteraction::NodePosition> nodes;
        for (const auto& n : m_graph->getNodes()) nodes.push_back({n.getIndex(), n.getCoord()});
        if (GraphEditInteraction::hitTest(nodes, m->pos(), 25) < 0) { m_graph->addNode(m->pos()); invalidateResults(); update(); }
        return;
    } else if(m->button() == Qt::LeftButton) {
        QVector<GraphEditInteraction::NodePosition> nodes;
        for (const auto& n : m_graph->getNodes()) nodes.push_back({n.getIndex(), n.getCoord()});
        const int selected = GraphEditInteraction::hitTest(nodes, m->pos(), m_renderer->settings().nodeRadius + 4);
        const int first = m_interaction.selectedNodeId();
        const auto release = m_interaction.release(selected);
        if (release == GraphEditInteraction::Release::DraggedNode) return;
        if (selected >= 0 && first >= 0 && selected != first) {
            bool exists = false;
            for (const auto& ed : m_graph->getEdges()) {
                if ((ed.getFirst().getIndex() == first && ed.getSecond().getIndex() == selected) ||
                    (m_graph->graphKind() == GraphKind::Undirected && ed.getFirst().getIndex() == selected && ed.getSecond().getIndex() == first))
                    { exists = true; break; }
            }
            if (!exists) { m_graph->addEdge(first, selected); invalidateResults(); }
            else QMessageBox::information(this, "Edge exists", "An edge already exists between these nodes!");
            m_interaction.cancel();
            update();
        }
    }
}

void VisComponents::mousePressEvent(QMouseEvent* m) {
    if(!m_graph || m->button() != Qt::LeftButton || m_interaction.mode() == GraphEditInteraction::Mode::Delete) return;
    QVector<GraphEditInteraction::NodePosition> nodes;
    for (const auto& n : m_graph->getNodes()) nodes.push_back({n.getIndex(), n.getCoord()});
    const int hit = GraphEditInteraction::hitTest(nodes, m->pos(), m_renderer->settings().nodeRadius + 4);
    QPoint nodePosition;
    for (const auto& node : m_graph->getNodes()) if (node.getIndex() == hit) { nodePosition = node.getCoord(); break; }
    m_interaction.press(hit, m->pos(), nodePosition);
}

void VisComponents::mouseMoveEvent(QMouseEvent* m) {
    if(!m_graph || m_interaction.mode() == GraphEditInteraction::Mode::Delete || !m_interaction.dragging() || !m_interaction.move(m->pos())) return;
    const int radius = m_renderer->settings().nodeRadius;
    const QPoint target = m_interaction.draggedPosition(m->pos());
    m_graph->setNodePosition(m_interaction.draggedNodeId(),
        QPoint(qBound(radius, target.x(), width() - radius),
               qBound(104 + radius, target.y(), height() - radius)));
    if (m_compResult.numComponents > 0) {
        if (m_graph->graphKind() == GraphKind::Directed)
            m_condensedResult.graph = CondensedGraphSolver{}.solve(*m_graph, m_compResult.numComponents, m_compResult.nodeToComponent);
    }
    update();
}

void VisComponents::paintEvent(QPaintEvent*) {
    if(!m_graph || !m_renderer) return;
    
    QPainter p(this);
    
    int selectedNodeId = m_interaction.selectedNodeId();
    m_renderer->setSelectedNodeId(selectedNodeId);
    
    m_renderer->renderResults(p, *m_graph, m_compResult, m_condensedResult);
}
