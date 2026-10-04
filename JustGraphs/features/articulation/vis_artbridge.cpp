#include "features/articulation/vis_artbridge.h"
#include "ui/theme.h"
#include "domain/undirectedgraph.h"
#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QTimerEvent>
#include <QShowEvent>
#include <QTime>
#include <cmath>

VisArtBridge::VisArtBridge(QWidget* parent)
	: QWidget(parent), m_timer(new QTimer(this))
{
	m_timer->setObjectName("articulationAnimationTimer");
	setMinimumSize(700, 500);
	m_graph = std::make_unique<UndirectedGraph>();
	m_renderer = std::make_unique<MutableGraphRenderer>();
	m_renderSettings.showDiscLowLabels = true;
	setupUi();
	connect(m_timer, &QTimer::timeout, this, &VisArtBridge::onAnimTick);
	m_pulseTimer = startTimer(16); // 60fps pulse animation
}

VisArtBridge::~VisArtBridge() = default;

void VisArtBridge::setupUi() {
	auto* toolbar = GraphUI::toolbar(this);
	delete toolbar->layout(); // Two compact rows: actions, then status.
	toolbar->setObjectName("graphToolbar");
	toolbar->setGeometry(0, 0, width(), 104);
	
	m_btnRun = GraphUI::button("Find articulation points and bridges", toolbar, true);
	connect(m_btnRun, &QPushButton::clicked, this, &VisArtBridge::onRun);

	m_btnReset = GraphUI::button("Reset", toolbar);
	connect(m_btnReset, &QPushButton::clicked, this, &VisArtBridge::onReset);
	m_btnDeleteMode = GraphUI::button("Delete nodes/edges", toolbar);
	m_btnDeleteMode->setObjectName("articulationDeleteMode");
	m_btnDeleteMode->setCheckable(true);
    m_btnDeleteMode->setProperty("role", "algorithm");
	connect(m_btnDeleteMode, &QPushButton::toggled, this, [this](bool deleting) {
		m_interaction.setMode(deleting ? GraphEditInteraction::Mode::Delete
		                               : GraphEditInteraction::Mode::Edit);
		if (deleting) resetState();
		m_btnRun->setEnabled(!deleting);
		m_instrLabel->setText(deleting ? "Left-click a node or edge to delete it"
		                              : "Right click: add node  ·  Click two nodes: add edge  ·  Drag: move node");
		update();
	});

	m_instrLabel = GraphUI::status("Right click: add node  ·  Click two nodes: add edge  ·  Drag: move node", toolbar);
	m_instrLabel->setStyleSheet(QString("background: %1; border: 1px solid %2; border-radius: 8px; padding: 6px 10px; color: %3;")
		.arg(GraphUI::Surface.name(), GraphUI::Border.name(), GraphUI::Muted.name()));
	auto* layout = new QVBoxLayout(toolbar);
	layout->setContentsMargins(24, 10, 24, 10);
	layout->setSpacing(8);
	auto* actions = new QHBoxLayout;
	actions->setSpacing(10);
	actions->addWidget(m_btnRun);
	actions->addWidget(m_btnReset);
	actions->addWidget(m_btnDeleteMode);
	actions->addStretch();
	layout->addLayout(actions);
	layout->addWidget(m_instrLabel);

	// ── Step label (bottom-left) ──────────────────────────────
	m_stepLabel = new QLabel("", this);
	m_stepLabel->setStyleSheet(QString("color: %1; background: white; border: 1px solid %2; border-radius: 8px; padding: 8px 12px;")
		.arg(GraphUI::Text.name(), GraphUI::Border.name()));
	m_stepLabel->setWordWrap(true);
	m_stepLabel->hide();

	m_resultLabel = new QLabel("", this);
	m_resultLabel->hide();
}

void VisArtBridge::resetState() {
	m_timer->stop();
	m_playback.cancel();
	m_animating = false;
	m_animStep = 0;
	m_steps.clear();
	m_result = {};
	m_animState = AnimationState(); 
	m_stepLabel->hide();
	m_resultLabel->hide();
	m_instrLabel->setText(m_interaction.mode() == GraphEditInteraction::Mode::Delete
		? "Left-click a node or edge to delete it"
		: "Right click: add node  ·  Click two nodes: add edge  ·  Drag: move node");
	m_btnRun->setEnabled(m_interaction.mode() == GraphEditInteraction::Mode::Edit);
	update();
}

void VisArtBridge::onReset() {
	resetState();
	m_graph = std::make_unique<UndirectedGraph>();
	m_interaction.cancel();
	update();
}

void VisArtBridge::onRun() {
	if (!m_graph || m_graph->getNodes().empty() || m_interaction.mode() == GraphEditInteraction::Mode::Delete) return;
	if (m_animating) return;

	resetState();

	m_result = ArticulationBridgeSolver::solve(*m_graph, &m_steps);

	m_animStep = 0;
	m_animating = true;
	m_playback.start();
	m_animState.isAnimating = true;
	m_animState.showResult = false;
	m_btnRun->setEnabled(false);
	m_stepLabel->show();
	m_stepLabel->setText("Pass 1/1: Running DFS...");
	m_timer->start(m_tickInterval);
}

// ─────────────────────────────────────────────────────────────
//  Animation tick
// ─────────────────────────────────────────────────────────────
void VisArtBridge::onAnimTick() {
	if (m_animStep >= m_steps.size()) {
		m_timer->stop();
		m_animating = false;
		m_playback.complete();
		m_animState.isAnimating = false;
		m_animState.showResult = true;
		m_animState.activeNode = -1;
		m_animState.activeNode2 = -1;
		m_animState.activeEdge = {-1, -1};
		m_animState.articulationPoints = m_result.articulationPoints;
		m_animState.bridges = m_result.bridges;
		m_stepLabel->hide();
		updateInfoPanel();
		m_btnRun->setEnabled(true);
		update();
		return;
	}

	const auto& step = m_steps[m_animStep++];

	switch (step.type) {
	case ArtBridgeStepType::NodeEnter:
		m_animState.visitedNodes.insert(step.nodeId);
		m_animState.discValues[step.nodeId] = step.discVal;
		m_animState.lowValues[step.nodeId] = step.lowVal;
		m_animState.activeNode = step.nodeId;
		m_animState.activeNode2 = -1;
		m_animState.activeEdge = {-1, -1};
		m_stepLabel->setText(
			QString("Enter node %1 — disc=%2, low=%3")
			.arg(step.nodeId).arg(step.discVal).arg(step.lowVal));
		break;

	case ArtBridgeStepType::EdgeTree:
		m_animState.activeNode = step.nodeId;
		m_animState.activeNode2 = step.nodeId2;
		m_animState.activeEdge = {step.nodeId, step.nodeId2};
		m_stepLabel->setText(
			QString("Tree edge: %1 → %2").arg(step.nodeId).arg(step.nodeId2));
		break;

	case ArtBridgeStepType::EdgeBack:
		m_animState.lowValues[step.nodeId] = step.lowVal;
		m_animState.activeNode = step.nodeId;
		m_animState.activeNode2 = step.nodeId2;
		m_animState.activeEdge = {step.nodeId, step.nodeId2};
		m_stepLabel->setText(
			QString("Back edge: %1 → %2 — low[%1]=%3")
			.arg(step.nodeId).arg(step.nodeId2).arg(step.lowVal));
		break;

	case ArtBridgeStepType::LowUpdate:
		m_animState.lowValues[step.nodeId] = step.lowVal;
		m_stepLabel->setText(
			QString("Update low[%1]=%2 (from child %3)")
			.arg(step.nodeId).arg(step.lowVal).arg(step.nodeId2));
		break;

	case ArtBridgeStepType::NodeExit:
		m_animState.activeNode = -1;
		m_animState.activeNode2 = -1;
		m_animState.activeEdge = {-1, -1};
		m_stepLabel->setText(
			QString("Exit node %1 — disc=%2, low=%3")
			.arg(step.nodeId).arg(step.discVal).arg(step.lowVal));
		break;

	case ArtBridgeStepType::ResultAP:
		m_animState.articulationPoints.insert(step.nodeId);
		m_stepLabel->setText(
			QString("✦ Articulation point found: node %1").arg(step.nodeId));
		break;

	case ArtBridgeStepType::ResultBridge:
		m_animState.bridges.push_back({
			std::min(step.nodeId, step.nodeId2),
			std::max(step.nodeId, step.nodeId2)
			});
		m_stepLabel->setText(
			QString("⚡ Bridge found: edge (%1 — %2)")
			.arg(step.nodeId).arg(step.nodeId2));
		break;

	case ArtBridgeStepType::Done:
		break;
	}

	update();
}

void VisArtBridge::timerEvent(QTimerEvent*) {
	m_animState.pulseValue = 0.5f + 0.5f * std::sin(static_cast<float>(QTime::currentTime().msec()) * 0.006f);
	if (m_animState.showResult) update();
}

void VisArtBridge::paintEvent(QPaintEvent*) {
	if (!m_graph || !m_renderer) return;
	
	QPainter p(this);
	p.fillRect(rect(), GraphUI::Canvas);
	m_renderSettings.backgroundColor = GraphUI::Canvas;
	m_renderSettings.nodeColor = GraphUI::BlueSoft;
	m_renderSettings.nodeVisitedColor = GraphUI::Blue;
	m_renderSettings.nodeActiveColor = GraphUI::Blue;
	m_renderSettings.nodeSelectedColor = GraphUI::Blue;
	m_renderSettings.nodeAPColor = GraphUI::Error;
	m_renderSettings.edgeColor = GraphUI::Edge;
	m_renderSettings.edgeTreeColor = GraphUI::Blue;
	m_renderSettings.edgeActiveColor = GraphUI::Blue;
	m_renderSettings.edgeBridgeColor = GraphUI::Error;
	m_renderSettings.textColor = GraphUI::Text;
	m_renderSettings.labelColor = GraphUI::Muted;
	int selectedNodeId = m_interaction.selectedNodeId();
	m_renderer->renderWithState(p, *m_graph, m_animState, m_renderSettings, selectedNodeId);
	
	if (m_animState.showResult || m_animating) {
		p.setFont(QFont());
		int ly = height() - 78;
		auto drawLegendItem = [&](QColor c, const QString& label, int y) {
			p.setBrush(c); p.setPen(Qt::NoPen);
			p.drawEllipse(14, y, 12, 12);
			p.setPen(QColor(148, 163, 184));
			p.setFont(QFont());
			p.drawText(32, y + 10, label);
			};
			drawLegendItem(GraphUI::Error, "Articulation point", ly);
			drawLegendItem(GraphUI::Blue, "Visited node", ly + 18);

		p.setPen(QPen(GraphUI::Error, 3));
		p.drawLine(14, ly + 36 + 6, 26, ly + 36 + 6);
		p.setPen(GraphUI::Muted);
		p.setFont(QFont());
		p.drawText(32, ly + 36 + 10, "Bridge");
	}

	if (m_stepLabel->isVisible()) {
		m_stepLabel->adjustSize();
		m_stepLabel->move(24, height() - m_stepLabel->height() - 16);
	}
}

void VisArtBridge::updateInfoPanel() {
	const auto apCount = m_result.articulationPoints.size();
	const auto bridgeCount = m_result.bridges.size();
	if (m_result.isBiconnected) {
		m_instrLabel->setText("Graph is biconnected  ·  no articulation points or bridges");
	} else {
		m_instrLabel->setText(QString("Complete  ·  %1 articulation point(s)  ·  %2 bridge(s)")
			.arg(apCount).arg(bridgeCount));
	}
}

void VisArtBridge::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	if (auto* toolbar = findChild<QWidget*>("graphToolbar", Qt::FindDirectChildrenOnly))
		toolbar->setGeometry(0, 0, width(), 104);
}

// ─────────────────────────────────────────────────────────────
//  Mouse interaction (identical pattern to VisComponents)
// ─────────────────────────────────────────────────────────────
void VisArtBridge::mousePressEvent(QMouseEvent* m) {
	if (!m_graph || m_animating || m_interaction.mode() == GraphEditInteraction::Mode::Delete) return;
	if (m->button() != Qt::LeftButton) return;
	QVector<GraphEditInteraction::NodePosition> nodes;
	for (const auto& n : m_graph->getNodes()) nodes.push_back({n.getIndex(), n.getCoord()});
	const int hit = GraphEditInteraction::hitTest(nodes, m->pos(), m_renderSettings.nodeRadius + 4);
	QPoint nodePosition;
	for (const auto& node : m_graph->getNodes()) if (node.getIndex() == hit) { nodePosition = node.getCoord(); break; }
	m_interaction.press(hit, m->pos(), nodePosition);
}

void VisArtBridge::mouseReleaseEvent(QMouseEvent* m) {
	if (!m_graph || m_animating) return;
	if (m_interaction.mode() == GraphEditInteraction::Mode::Delete) {
		if (m->button() != Qt::LeftButton || m->pos().y() < 112) { m_interaction.cancel(); return; }
		QVector<GraphEditInteraction::NodePosition> nodes;
		for (const auto& n : m_graph->getNodes()) nodes.push_back({n.getIndex(), n.getCoord()});
		const int nodeId = GraphEditInteraction::hitTest(nodes, m->pos(), m_renderSettings.nodeRadius + 4);
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
		if (changed) { resetState(); update(); }
		return;
	}

	if (m->pos().y() < 112) {
		if (m->button() == Qt::LeftButton) m_interaction.release(-1);
		return;
	}

	if (m->button() == Qt::RightButton) {
		// Add node (only if not overlapping existing)
		QVector<GraphEditInteraction::NodePosition> nodes;
		for (const auto& n : m_graph->getNodes()) nodes.push_back({n.getIndex(), n.getCoord()});
		if (GraphEditInteraction::hitTest(nodes, m->pos(), 28) < 0) {
			m_graph->addNode(m->pos());
			m_interaction.cancel();
			resetState();
			update();
		}
		return;
	}

	if (m->button() == Qt::LeftButton) {
		QVector<GraphEditInteraction::NodePosition> nodes;
		for (const auto& n : m_graph->getNodes()) nodes.push_back({n.getIndex(), n.getCoord()});
		const int selected = GraphEditInteraction::hitTest(nodes, m->pos(), m_renderSettings.nodeRadius + 4);
		const int first = m_interaction.selectedNodeId();
		const auto release = m_interaction.release(selected);
		if (release == GraphEditInteraction::Release::DraggedNode) return;
		if (selected >= 0) {
			if (first >= 0 && first != selected) {
				// Check duplicate edge
				bool exists = false;
				for (const auto& e : m_graph->getEdges()) {
					if ((e.getFirst().getIndex() == first && e.getSecond().getIndex() == selected) ||
						(e.getFirst().getIndex() == selected && e.getSecond().getIndex() == first))
					{
						exists = true; break;
					}
				}
				if (!exists) {
					m_graph->addEdge(first, selected);
					m_interaction.cancel();
					resetState();
				}
				m_interaction.cancel();
				update();
		} else if (release == GraphEditInteraction::Release::ClickedEmpty) m_interaction.cancel();
		update();
		}
	}
}

void VisArtBridge::mouseMoveEvent(QMouseEvent* m) {
	if (!m_graph || m_interaction.mode() == GraphEditInteraction::Mode::Delete || !m_interaction.dragging() || !m_interaction.move(m->pos()) || m_animating) return;
	const QPoint target = m_interaction.draggedPosition(m->pos());
	const int radius = m_renderSettings.nodeRadius;
	QPoint position(qBound(radius, target.x(), width() - radius),
	                qBound(112 + radius, target.y(), height() - radius));
	m_graph->setNodePosition(m_interaction.draggedNodeId(), position);
	update();
}

// ─────────────────────────────────────────────────────────────
//  Hide — full reset
// ─────────────────────────────────────────────────────────────
void VisArtBridge::hideEvent(QHideEvent* event) {
	m_timer->stop();
	if (m_animating) m_playback.pause();
	if (m_pulseTimer) { killTimer(m_pulseTimer); m_pulseTimer = 0; }
	m_interaction.cancel();
	QWidget::hideEvent(event);
}

void VisArtBridge::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);
	if (!m_pulseTimer) m_pulseTimer = startTimer(16);
	if (m_animating && m_playback.resume()) m_timer->start(m_tickInterval);
}
