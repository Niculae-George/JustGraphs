#include "features/topological/vis_topological.h"
#include "ui/theme.h"
#include "algorithms/topological_sort_solver.h"
#include <QCursor>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>
#include <cmath>

VisTopologicalSort::VisTopologicalSort(QWidget *parent)
    : QWidget(parent), m_graph(nullptr) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 8, 12, 12);
  layout->setSpacing(10);
  m_toolbar = GraphUI::toolbar(this);
  auto *barLayout = qobject_cast<QHBoxLayout *>(m_toolbar->layout());
  auto *rows = new QVBoxLayout;
  auto *actions = new QHBoxLayout;
  m_btnSort = GraphUI::button("Sort graph", m_toolbar, true);
  m_btnClear = GraphUI::button("Clear graph", m_toolbar);
  m_btnDeleteMode = GraphUI::button("Delete nodes/edges", m_toolbar);
  m_btnDeleteMode->setObjectName("topologicalDeleteMode");
  m_btnDeleteMode->setProperty("role", "algorithm");
  m_btnDeleteMode->setCheckable(true);
  actions->addWidget(m_btnSort);
  actions->addWidget(m_btnClear);
  actions->addWidget(m_btnDeleteMode);
  actions->addStretch();
  auto *mode = new QLabel("Directed graph", m_toolbar);
  mode->setProperty("role", "hint");
  rows->addWidget(mode);
  rows->addLayout(actions);
  m_resultLabel = GraphUI::status("Right-click to add nodes. Click two nodes to connect them.", m_toolbar);
  m_resultLabel->setWordWrap(true);
  m_resultLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  rows->addWidget(m_resultLabel);
  barLayout->addLayout(rows);
  layout->addWidget(m_toolbar);
  layout->addStretch(1);
  connect(m_btnDeleteMode, &QPushButton::toggled, this, [this](bool deleting) {
    m_interaction.setMode(deleting ? GraphEditInteraction::Mode::Delete : GraphEditInteraction::Mode::Edit);
    m_resultLabel->setText(deleting ? "Left-click a node or edge to delete it. Click near the target arrow to choose an edge direction."
                                  : "Right-click to add nodes. Click two nodes to connect them.");
    update();
  });
  connect(m_btnSort, &QPushButton::clicked, this, &VisTopologicalSort::onSortClicked);
  connect(m_btnClear, &QPushButton::clicked, this, &VisTopologicalSort::onClearClicked);
  m_graph = std::make_unique<DirectedGraph>();
  m_renderer = std::make_unique<MutableGraphRenderer>();
  m_renderSettings.directed = true;
  setMinimumSize(480, 400);
}

QRect VisTopologicalSort::canvasRect() const {
  return QRect(12, m_toolbar->geometry().bottom() + 12, width() - 24,
               qMax(0, height() - m_toolbar->geometry().bottom() - 42));
}

VisTopologicalSort::~VisTopologicalSort() {}

void VisTopologicalSort::hideEvent(QHideEvent *event) {
  m_interaction.cancel();
  QWidget::hideEvent(event);
}

void VisTopologicalSort::mousePressEvent(QMouseEvent *event) {
  if (!m_graph || !canvasRect().adjusted(30, 30, -30, -30).contains(event->pos()))
    return;
  if (event->button() != Qt::LeftButton || m_interaction.mode() == GraphEditInteraction::Mode::Delete) return;
  QVector<GraphEditInteraction::NodePosition> nodes;
  for (const auto& node : m_graph->getNodes()) nodes.push_back({node.getIndex(), node.getCoord()});
  const int hit = GraphEditInteraction::hitTest(nodes, event->pos(), m_renderSettings.nodeRadius + 4);
  QPoint nodePosition;
  for (const auto& node : m_graph->getNodes()) if (node.getIndex() == hit) { nodePosition = node.getCoord(); break; }
  m_interaction.press(hit, event->pos(), nodePosition);
}

void VisTopologicalSort::mouseMoveEvent(QMouseEvent *event) {
  if (m_interaction.dragging() && m_interaction.move(event->pos())) {
    const auto bounds = canvasRect().adjusted(30, 30, -30, -30);
    const QPoint target = m_interaction.draggedPosition(event->pos());
    QPoint newPos(qBound(bounds.left(), target.x(), bounds.right()),
                  qBound(bounds.top(), target.y(), bounds.bottom()));
    m_graph->setNodePosition(m_interaction.draggedNodeId(), newPos);
    update();
  }
}

void VisTopologicalSort::mouseReleaseEvent(QMouseEvent *event) {
  if (!m_graph)
    return;
  if (!canvasRect().adjusted(30, 30, -30, -30).contains(event->pos())) {
    if (event->button() == Qt::LeftButton) m_interaction.release(-1);
    return;
  }

  if (m_interaction.mode() == GraphEditInteraction::Mode::Delete) {
    if (event->button() != Qt::LeftButton) return;
    QVector<GraphEditInteraction::NodePosition> nodes;
    for (const auto& node : m_graph->getNodes()) nodes.push_back({node.getIndex(), node.getCoord()});
    const int hit = GraphEditInteraction::hitTest(nodes, event->pos(), m_renderSettings.nodeRadius + 4);
    bool changed = false;
    if (hit >= 0) changed = m_graph->removeNode(hit);
    else {
      QVector<GraphEditInteraction::EdgeSegment> edges;
      for (const auto& edge : m_graph->getEdges())
        edges.push_back({edge.getFirst().getIndex(), edge.getSecond().getIndex(), edge.getFirst().getCoord(), edge.getSecond().getCoord()});
      const int index = GraphEditInteraction::hitEdge(edges, event->pos());
      if (index >= 0) changed = m_graph->removeEdge(edges[index].from, edges[index].to);
    }
    if (changed) {
      m_sortedOrder.clear();
      m_resultLabel->setText("Graph changed. Run sort again to refresh result.");
      update();
    }
    return;
  }

  if (event->button() == Qt::RightButton) {
    m_interaction.cancel();
    m_graph->addNode(event->pos());
    m_sortedOrder.clear();
    m_resultLabel->setText("Graph changed. Run sort again to refresh result.");
    update();
    return;
  }

  if (event->button() == Qt::LeftButton) {
    QVector<GraphEditInteraction::NodePosition> nodes;
    for (const auto& node : m_graph->getNodes()) nodes.push_back({node.getIndex(), node.getCoord()});
    const int selected = GraphEditInteraction::hitTest(nodes, event->pos(), m_renderSettings.nodeRadius + 4);
    const int first = m_interaction.selectedNodeId();
    const auto release = m_interaction.release(selected);
    if (release == GraphEditInteraction::Release::ClickedNode && selected >= 0 && first >= 0 && first != selected) {
      if (m_graph->addEdge(first, selected, 1)) {
        m_sortedOrder.clear();
        m_resultLabel->setText("Graph changed. Run sort again to refresh result.");
      }
      m_interaction.cancel();
      update();
    }
  }
}

void VisTopologicalSort::onSortClicked() {
  if (!m_graph || m_graph->getNodes().empty()) {
    m_resultLabel->setText("Add at least one node before sorting.");
    return;
  }

  TopologicalSortSolver solver;

  // A failed run must not leave a previous result visible as current.
  m_sortedOrder.clear();
  // Check for cycle
  if (solver.hasCycle(*m_graph)) {
    m_resultLabel->setText("Cycle detected. Remove the cycle to sort this graph.");
    return;
  }

  // Get sorted order
  solver.solve(*m_graph, m_sortedOrder);

  // Build order text
  QString orderText = "Order: ";
  for (size_t i = 0; i < m_sortedOrder.size(); ++i) {
    if (i > 0)
      orderText += " → ";
    orderText += QString::number(m_sortedOrder[i]);
  }

  m_resultLabel->setText(orderText);

}

void VisTopologicalSort::onClearClicked() {
  m_graph->clear();
  m_interaction.cancel();
  m_sortedOrder.clear();
  m_resultLabel->setText(m_interaction.mode() == GraphEditInteraction::Mode::Delete
    ? "Left-click a node or edge to delete it."
    : "Right-click to add nodes. Click two nodes to connect them.");
  update();
}

void VisTopologicalSort::paintEvent(QPaintEvent *) {
  QPainter painter(this);
  painter.fillRect(rect(), GraphUI::Canvas);
  if (!m_graph || !m_renderer)
    return;
  painter.save();
  painter.setClipRect(canvasRect());
  const int selectedNode = m_interaction.selectedNodeId();
  m_renderer->renderWithState(painter, *m_graph, m_animState, m_renderSettings, selectedNode);
  if (const Node* first = m_graph->findNode(m_interaction.selectedNodeId()))
    m_renderer->drawPendingEdge(painter, first->getCoord(), mapFromGlobal(QCursor::pos()));
  painter.restore();
  painter.setPen(GraphUI::Muted);
  painter.drawText(QRect(12, height() - 26, width() - 24, 20), Qt::AlignCenter,
                   m_interaction.mode() == GraphEditInteraction::Mode::Delete
                     ? "Delete mode · Click a node or edge · Click Delete nodes/edges again to return to editing"
                     : "Drag a node to move it · Edges point from the first node to the second");
}
