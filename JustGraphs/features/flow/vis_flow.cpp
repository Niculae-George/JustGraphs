#include "features/flow/vis_flow.h"
#include "ui/theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QShortcut>
#include <QStyle>
#include <QApplication>
#include <QIntValidator>
#include <QLineF>
#include <QPolygonF>
#include <QFontMetrics>
#include <cmath>
#include <numbers>

namespace {
GraphEditInteraction::EdgeSegment flowEdgeSegment(const QPointF& from, const QPointF& to) {
    const QLineF line(from, to);
    const double angle = std::atan2(line.dy(), line.dx());
    const double shift = -11.0;
    const QPointF normal(-std::sin(angle) * shift, std::cos(angle) * shift);
    return {0, 0,
            from + QPointF(20.0 * std::cos(angle), 20.0 * std::sin(angle)) + normal,
            to - QPointF(25.0 * std::cos(angle), 25.0 * std::sin(angle)) + normal};
}
}

VisFlow::VisFlow(QWidget* parent) : QWidget(parent), m_network(std::make_unique<FlowNetwork>()),
    m_solver(std::make_unique<FlowSolver>(m_network.get())), m_networkWidget(new NetworkWidget(this)) {
    m_networkWidget->setNetwork(m_network.get());
    m_networkWidget->setSolver(m_solver.get());
    setupUI();
    connect(m_networkWidget, &NetworkWidget::networkEdited, this, &VisFlow::invalidateAlgorithm);
    connect(m_networkWidget, &NetworkWidget::statusMessage, this, &VisFlow::setStatus);
    connect(m_networkWidget, &NetworkWidget::returnToEditMode, this, [this] { findChild<QPushButton*>("flowEdit")->setChecked(true); });
    connect(m_networkWidget, &NetworkWidget::nodeSelectionChanged, this, [this](int node) {
        m_btnDelete->setEnabled(node>0 && m_networkWidget->interactionMode()==NetworkWidget::InteractionMode::Edit);
        if (node>0 && m_networkWidget->interactionMode()==NetworkWidget::InteractionMode::Edit)
            setStatus(QString("Node %1 selected. Select another node to connect, or drag to move.").arg(node));
    });
    connect(m_networkWidget, &NetworkWidget::interactionModeChanged, this, [this](NetworkWidget::InteractionMode mode) { m_btnDelete->setEnabled(mode==NetworkWidget::InteractionMode::Edit && m_networkWidget->selectedNode()>0); });
    connect(m_networkWidget, &NetworkWidget::interactionModeChanged, this, [this](NetworkWidget::InteractionMode mode) {
        auto* group = findChild<QButtonGroup*>("flowModes");
        const int id = mode==NetworkWidget::InteractionMode::Delete ? 1
            : mode==NetworkWidget::InteractionMode::SetSource ? 2
            : mode==NetworkWidget::InteractionMode::SetSink ? 3
            : mode==NetworkWidget::InteractionMode::Edit ? 0 : -1;
        group->setExclusive(false);
        for (auto* button : group->buttons()) button->setChecked(group->id(button)==id);
        group->setExclusive(true);
    });
}

VisFlow::~VisFlow() = default;

void VisFlow::hideEvent(QHideEvent* event) {
    m_networkWidget->cancelTransientEditing();
    QWidget::hideEvent(event);
}

void VisFlow::setupUI() {
    auto* main = new QVBoxLayout(this);
    main->setContentsMargins(12, 8, 12, 12);
    main->setSpacing(8);
    auto* toolbar = GraphUI::toolbar(this);
    auto* root = qobject_cast<QHBoxLayout*>(toolbar->layout());
    auto* rows = new QVBoxLayout;
    rows->setSpacing(7);
    auto* modes = new QHBoxLayout;
    modes->setSpacing(6);
    auto* group = new QButtonGroup(this);
    group->setObjectName("flowModes");
    group->setExclusive(true);
    const QList<QPair<QString, NetworkWidget::InteractionMode>> defs{
        {"Edit network", NetworkWidget::InteractionMode::Edit},
        {"Delete nodes/edges", NetworkWidget::InteractionMode::Delete},
        {"Set source", NetworkWidget::InteractionMode::SetSource},
        {"Set sink", NetworkWidget::InteractionMode::SetSink}
    };
    const QStringList names{"flowEdit", "flowDeleteMode", "flowSource", "flowSink"};
    for (int i=0; i<defs.size(); ++i) {
        auto* b = GraphUI::button(defs[i].first, toolbar);
        b->setCheckable(true);
        b->setProperty("role", "algorithm");
        b->setObjectName(names[i]);
        group->addButton(b, i);
        modes->addWidget(b);
    }
    group->button(0)->setChecked(true);
    connect(group, &QButtonGroup::idClicked, this, [this](int id) {
        const auto mode = id==1 ? NetworkWidget::InteractionMode::Delete : id==2 ? NetworkWidget::InteractionMode::SetSource : id==3 ? NetworkWidget::InteractionMode::SetSink : NetworkWidget::InteractionMode::Edit;
        m_networkWidget->setMode(mode);
        setStatus(mode==NetworkWidget::InteractionMode::Delete ? "Click a node to delete it, or click an edge to delete that direction."
            : mode==NetworkWidget::InteractionMode::SetSource ? "Click a node to set the source."
            : mode==NetworkWidget::InteractionMode::SetSink ? "Click a node to set the sink."
            : "Right-click to add nodes. Select two nodes to connect, or drag a node to move it.");
    });
    modes->addStretch();
    auto* actionsTop = new QHBoxLayout;
    actionsTop->setSpacing(6);
    auto* actionsBottom = new QHBoxLayout;
    actionsBottom->setSpacing(6);
    m_btnStart = GraphUI::button("Start flow", toolbar, true); m_btnStart->setObjectName("flowStart");
    m_btnNextStep = GraphUI::button("Next step", toolbar); m_btnNextStep->setObjectName("flowNext");
    auto* cycles = GraphUI::button("Remove negative cycles", toolbar); cycles->setObjectName("flowCycles");
    auto* reset = GraphUI::button("Reset result", toolbar); reset->setObjectName("flowReset");
    auto* clear = GraphUI::button("Clear network", toolbar); clear->setObjectName("flowClear");
    m_btnDelete = GraphUI::button("Delete node", toolbar); m_btnDelete->setObjectName("flowDelete");
    m_btnDelete->setEnabled(false);
    m_btnNextStep->setEnabled(false);
    for (auto* b : {m_btnStart,m_btnNextStep,cycles}) actionsTop->addWidget(b);
    actionsTop->addStretch();
    for (auto* b : {reset,m_btnDelete,clear}) actionsBottom->addWidget(b);
    actionsBottom->addStretch();
    rows->addLayout(modes);
    rows->addLayout(actionsTop);
    rows->addLayout(actionsBottom);
    root->addLayout(rows, 1);
    main->addWidget(toolbar);
    m_statusLabel = GraphUI::status("Right-click empty space to add nodes. Select two nodes to connect them.", this);
    m_statusLabel->setObjectName("flowStatus");
    m_statusLabel->setWordWrap(true);
    main->addWidget(m_statusLabel);
    main->addWidget(m_networkWidget->edgeEditorWidget());
    main->addWidget(m_networkWidget, 1);
    auto* hint = new QLabel("Right-click: add node · Select two nodes: connect · Drag: move · Blue ring: source · Red ring: sink", this);
    hint->setWordWrap(true); hint->setProperty("role", "hint"); main->addWidget(hint);
    connect(m_networkWidget, &NetworkWidget::interactionModeChanged, hint, [hint](NetworkWidget::InteractionMode mode) {
        hint->setText(mode==NetworkWidget::InteractionMode::Delete
            ? "Delete mode: click a node to remove it and its edges, or click an edge to remove that direction. Right-click and dragging are disabled."
            : mode==NetworkWidget::InteractionMode::SetSource ? "Set source: click a node. Source and sink must be different."
            : mode==NetworkWidget::InteractionMode::SetSink ? "Set sink: click a node. Source and sink must be different."
            : mode==NetworkWidget::InteractionMode::View ? "Algorithm view: click nodes to inspect the current flow network."
            : "Right-click: add nodes · Select two nodes: connect · Drag: move · Blue ring: source · Red ring: sink");
    });
    connect(m_btnStart, &QPushButton::clicked, this, &VisFlow::onStartAlgorithm);
    connect(m_btnNextStep, &QPushButton::clicked, this, &VisFlow::onNextStep);
    connect(cycles, &QPushButton::clicked, this, &VisFlow::onEliminateNegCycles);
    connect(reset, &QPushButton::clicked, this, &VisFlow::onReset);
    connect(clear, &QPushButton::clicked, this, &VisFlow::onClear);
    connect(m_btnDelete, &QPushButton::clicked, m_networkWidget, &NetworkWidget::deleteSelectedNode);
}

void VisFlow::setStatus(const QString& text, bool error) {
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(error ? QString("color: %1;").arg(GraphUI::Error.name()) : QString());
}

void VisFlow::invalidateAlgorithm() {
    m_solver->reset();
    m_networkWidget->setIterationIndex(-1);
    m_btnNextStep->setEnabled(false);
}

void VisFlow::onStartAlgorithm() {
    if (m_network->getNumNodes()<2) { setStatus("Add at least two nodes before starting.", true); return; }
    if (m_network->getSource()==-1 || m_network->getSink()==-1) { setStatus("Choose a source and a different sink.", true); return; }
    if (m_network->getSource()==m_network->getSink()) { setStatus("Source and sink must be different.", true); return; }
    m_networkWidget->cancelTransientEditing();
    m_solver->initializeAlgorithm(); m_networkWidget->setIterationIndex(0);
    setStatus("Algorithm initialized. Press Next step.");
    m_btnNextStep->setEnabled(true);
    m_networkWidget->setMode(NetworkWidget::InteractionMode::View);
    m_btnDelete->setEnabled(false);
    auto* group = findChild<QButtonGroup*>("flowModes");
    group->setExclusive(false);
    for (auto* button : group->buttons()) button->setChecked(false);
    group->setExclusive(true);
}

void VisFlow::onNextStep() {
    if (!m_solver->performStep()) {
        m_solver->finalizeMinCut(); m_solver->addFinalState();
        const auto& it=m_solver->iterations();
        if(!it.empty()) { m_networkWidget->setIterationIndex(int(it.size())-1); setStatus(QString("Algorithm finished. Max flow: %1").arg(it.back().totalFlow)); }
        m_btnNextStep->setEnabled(false); return;
    }
    const auto& it=m_solver->iterations();
    if(!it.empty()) {
        m_networkWidget->setIterationIndex(int(it.size())-1);
        const auto& state = it.back();
        setStatus(QString("Iteration %1: augmenting path found, flow %2. Total flow: %3")
                      .arg(state.iterationNumber).arg(state.pathFlow).arg(state.totalFlow));
    }
}

void VisFlow::onEliminateNegCycles() {
    if (m_network->getNumNodes()<2) { setStatus("Add at least two nodes first.", true); return; }
    int cycles=m_solver->eliminateNegativeCycles();
    if (!cycles) setStatus("No negative cycles found.");
    else {
        const auto& it=m_solver->iterations();
        m_networkWidget->setIterationIndex(it.empty() ? -1 : int(it.size())-1);
        m_networkWidget->setMode(NetworkWidget::InteractionMode::View);
        setStatus(QString("Eliminated %1 negative cycle(s).").arg(cycles));
    }
    m_networkWidget->update();
}

void VisFlow::onReset() {
    m_networkWidget->cancelTransientEditing();
    m_solver->reset(); m_networkWidget->setIterationIndex(-1);
    m_networkWidget->setMode(NetworkWidget::InteractionMode::Edit);
    findChild<QPushButton*>("flowEdit")->setChecked(true);
    m_btnNextStep->setEnabled(false); setStatus("Results reset.");
}

void VisFlow::onClear() {
    m_networkWidget->cancelTransientEditing(); m_network->clear(); m_solver->reset();
    m_networkWidget->setIterationIndex(-1); m_networkWidget->clearSelection();
    m_networkWidget->setMode(NetworkWidget::InteractionMode::Edit);
    findChild<QPushButton*>("flowEdit")->setChecked(true);
    m_btnNextStep->setEnabled(false); setStatus("Network cleared."); m_networkWidget->update();
    emit m_networkWidget->networkEdited();
}

NetworkWidget::NetworkWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(480,300); setMouseTracking(true); setFocusPolicy(Qt::StrongFocus); setObjectName("flowCanvas");
    m_edgeEditor=new QWidget(this); m_edgeEditor->setObjectName("flowEdgeEditor");
    auto* layout=new QHBoxLayout(m_edgeEditor); layout->setContentsMargins(8,6,8,6); layout->setSpacing(6);
    auto* capLabel=new QLabel("Capacity",m_edgeEditor);
    m_capacity=new QLineEdit(m_edgeEditor); m_capacity->setObjectName("flowCapacity"); m_capacity->setPlaceholderText("1–10000"); m_capacity->setValidator(new QIntValidator(1,10000,m_capacity)); m_capacity->setMaximumWidth(100);
    auto* costLabel=new QLabel("Cost",m_edgeEditor);
    m_cost=new QLineEdit(m_edgeEditor); m_cost->setObjectName("flowCost"); m_cost->setPlaceholderText("−10000–10000"); m_cost->setValidator(new QIntValidator(-10000,10000,m_cost)); m_cost->setMaximumWidth(105);
    m_edgeError=new QLabel(m_edgeEditor); m_edgeError->setProperty("role","error");
    m_edgeError->setWordWrap(true);
    m_edgeError->setStyleSheet(QString("color: %1;").arg(GraphUI::Error.name()));
    m_applyEdge=GraphUI::button("Apply",m_edgeEditor,true); m_applyEdge->setObjectName("flowApplyEdge");
    auto* cancel=GraphUI::button("Cancel",m_edgeEditor); cancel->setObjectName("flowCancelEdge");
    layout->addWidget(capLabel); layout->addWidget(m_capacity); layout->addWidget(costLabel); layout->addWidget(m_cost); layout->addWidget(m_edgeError,1); layout->addWidget(m_applyEdge); layout->addWidget(cancel);
    m_edgeEditor->setStyleSheet(QString("QWidget#flowEdgeEditor { background: %1; border: 1px solid %2; border-radius: 8px; }")
        .arg(GraphUI::Surface.name(),GraphUI::Border.name()));
    auto editorPolicy=m_edgeEditor->sizePolicy(); editorPolicy.setRetainSizeWhenHidden(true); m_edgeEditor->setSizePolicy(editorPolicy);
    m_edgeEditor->setMinimumHeight(48);
    m_edgeEditor->hide();
    connect(m_applyEdge,&QPushButton::clicked,this,&NetworkWidget::applyPendingEdge);
    connect(cancel,&QPushButton::clicked,this,&NetworkWidget::cancelPendingEdge);
    connect(m_capacity,&QLineEdit::returnPressed,this,&NetworkWidget::applyPendingEdge);
    connect(m_cost,&QLineEdit::returnPressed,this,&NetworkWidget::applyPendingEdge);
    auto* escape=new QShortcut(QKeySequence(Qt::Key_Escape),m_edgeEditor);
    escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape,&QShortcut::activated,this,&NetworkWidget::cancelPendingEdge);
    auto* enter=new QShortcut(QKeySequence(Qt::Key_Return),m_edgeEditor);
    enter->setContext(Qt::WidgetWithChildrenShortcut);
    connect(enter,&QShortcut::activated,this,&NetworkWidget::applyPendingEdge);
}

void NetworkWidget::setNetwork(FlowNetwork* n) { m_network=n; update(); }
void NetworkWidget::setSolver(const FlowSolver* solver) { m_solver=solver; update(); }
bool NetworkWidget::edgeEditorVisible() const { return m_edgeEditor && m_edgeEditor->isVisible(); }
void NetworkWidget::setMode(InteractionMode m) {
    if (m_mode==m) return;
    m_deletePressed=false;
    cancelTransientEditing();
    if(m==InteractionMode::SetSource || m==InteractionMode::SetSink) setSelectedNode(-1);
    m_mode=m; emit interactionModeChanged(m); update();
}
void NetworkWidget::setIterationIndex(int i) { m_iterationIndex=i; update(); }
void NetworkWidget::setSelectedNode(int node) { if(m_selectedNode==node)return; m_selectedNode=node; emit nodeSelectionChanged(node); update(); }
void NetworkWidget::cancelTransientEditing() {
    m_deletePressed=false;
    const bool hadPair=m_editInteraction.selectedNodeId()>0 || m_pendingFrom>0;
    m_pendingFrom=m_pendingTo=-1;
    m_editInteraction.cancel();
    if(m_edgeEditor) m_edgeEditor->hide();
    if(hadPair) setSelectedNode(-1);
    if(hadPair) emit statusMessage("Connection selection cleared. Select two nodes to connect.",false);
    update();
}
void NetworkWidget::cancelPendingEdge() { cancelTransientEditing(); setFocus(); }
void NetworkWidget::setInlineError(const QString& s) { m_edgeError->setText(s); }

int NetworkWidget::getNodeAt(const QPoint& p) const {
    if(!m_network)return -1;
    const auto& pos=m_network->getNodePositions();
    QVector<GraphEditInteraction::NodePosition> nodes;
    nodes.reserve(static_cast<qsizetype>(pos.size()));
    for(size_t i=0;i<pos.size();++i)
        nodes.push_back({int(i)+1, QPoint(qRound(pos[i].x()), qRound(pos[i].y()))});
    return GraphEditInteraction::hitTest(nodes, p, int(NODE_RADIUS+2));
}

void NetworkWidget::mousePressEvent(QMouseEvent* e) {
    setFocus();
    const QPoint p=e->position().toPoint();
    if(!m_network)return;
    if(m_mode==InteractionMode::SetSource || m_mode==InteractionMode::SetSink) {
        if(e->button()!=Qt::LeftButton)return;
        int node=getNodeAt(p); if(node<1)return;
        if((m_mode==InteractionMode::SetSource && node==m_network->getSink()) || (m_mode==InteractionMode::SetSink && node==m_network->getSource())) {
            emit statusMessage("Source and sink must be different.",true); return;
        }
        const bool accepted = m_mode==InteractionMode::SetSource ? m_network->setSource(node) : m_network->setSink(node);
        if (!accepted) { emit statusMessage("Source and sink must be different.",true); return; }
        emit networkEdited(); update(); emit statusMessage(QString("%1 set to node %2.").arg(m_mode==InteractionMode::SetSource?"Source":"Sink").arg(node),false);
        m_mode=InteractionMode::Edit; emit interactionModeChanged(m_mode); emit returnToEditMode(); return;
    }
    if(m_mode==InteractionMode::View) {
        if(e->button()==Qt::LeftButton) {
            int node=getNodeAt(p);
            m_editInteraction.press(node, p, node > 0 ? m_network->getNodePositions()[size_t(node - 1)].toPoint() : QPoint());
            if(node<1){setSelectedNode(-1);return;}
            setSelectedNode(node);
        }
        return;
    }
    if(m_mode==InteractionMode::Delete) {
        if(e->button()==Qt::LeftButton) { m_deletePressPos=p; m_deletePressed=true; }
        return;
    }
    if(e->button()==Qt::RightButton) {
        const QPointF position=clampedPosition(p);
        bool overlaps=false;
        for(const QPointF& existing : m_network->getNodePositions())
            if(QLineF(position,existing).length()<NODE_RADIUS*2+4) { overlaps=true; break; }
        if(!overlaps) { cancelTransientEditing(); m_network->addNode(position); emit networkEdited(); setSelectedNode(int(m_network->getNodePositions().size())); update(); emit statusMessage("Node added.",false); }
        return;
    }
    if(e->button()!=Qt::LeftButton)return;
    const int node=getNodeAt(p);
    m_editInteraction.press(node, p, node > 0 ? m_network->getNodePositions()[size_t(node - 1)].toPoint() : QPoint());
    if(node<1) { setSelectedNode(-1); return; }
    setSelectedNode(node);
}

void NetworkWidget::mouseMoveEvent(QMouseEvent* e) {
    const QPoint p=e->position().toPoint();
    if(!m_network || !m_editInteraction.dragging() || m_editInteraction.draggedNodeId()<1 ||
       !m_editInteraction.move(p, QApplication::startDragDistance())) {
        if(m_editInteraction.selectedNodeId()>0) { m_tempMousePos=p; update(); }
        return;
    }
    const int dragged = m_editInteraction.draggedNodeId();
    m_network->setNodePosition(dragged,clampedPosition(m_editInteraction.draggedPosition(p)));
    update();
}

void NetworkWidget::mouseReleaseEvent(QMouseEvent* e) {
    const QPoint p=e->position().toPoint();
    if (m_mode==InteractionMode::Delete) {
        if (e->button()==Qt::LeftButton && m_deletePressed) {
            const QPoint delta=p-m_deletePressPos;
            const int threshold=QApplication::startDragDistance();
            const bool click=delta.x()*delta.x()+delta.y()*delta.y()<threshold*threshold;
            m_deletePressed=false;
            if (click) deleteAt(p);
        }
        return;
    }
    if(!m_network || e->button()!=Qt::LeftButton)return;
    const int target=getNodeAt(p);
    const int first=m_editInteraction.selectedNodeId();
    const auto gesture=m_editInteraction.release(target);
    if(gesture==GraphEditInteraction::Release::DraggedNode) {
        m_editInteraction.cancel();
        setSelectedNode(-1);
        update();
        return;
    }
    if(gesture==GraphEditInteraction::Release::ClickedNode && m_mode==InteractionMode::Edit &&
       target>0 && first>0 && target!=first) {
        beginPendingEdge(first,target);
        m_editInteraction.cancel();
        setSelectedNode(-1);
    } else if(gesture==GraphEditInteraction::Release::ClickedEmpty) {
        m_editInteraction.cancel();
        setSelectedNode(-1);
    } else {
        setSelectedNode(m_editInteraction.selectedNodeId());
        m_tempMousePos=p;
    }
    update();
}

void NetworkWidget::beginPendingEdge(int from,int to) {
    if(from<1 || to<1 || from==to)return;
    m_pendingFrom=from; m_pendingTo=to; m_edgeError->clear();
    const int existing=m_network->getCapacity(from,to);
    m_capacity->setText(QString::number(existing>0?existing:10));
    m_cost->setText(QString::number(existing>0?m_network->getEdgeCost(from,to):0));
    m_edgeEditor->show(); positionEdgeEditor(); m_capacity->setFocus(); m_capacity->selectAll(); emit edgeRequested(from,to);
    emit statusMessage(QString("Connection %1 → %2: enter capacity, then Apply or press Enter.").arg(from).arg(to),false);
}

void NetworkWidget::applyPendingEdge() {
    if(m_pendingFrom<1 || m_pendingTo<1)return;
    bool capOk=false,costOk=false;
    const int cap=m_capacity->text().toInt(&capOk), cost=m_cost->text().toInt(&costOk);
    if(!capOk || cap<1 || cap>10000) { setInlineError("Capacity must be 1–10000."); m_capacity->setFocus(); return; }
    if(!costOk || cost < -10000 || cost > 10000) { setInlineError("Cost must be −10000–10000."); m_cost->setFocus(); return; }
    const int from=m_pendingFrom,to=m_pendingTo;
    const bool edgeAccepted = m_network->getCapacity(from,to)>0
        ? m_network->setCapacity(from,to,cap) : m_network->addEdge(from,to,cap);
    if (!edgeAccepted || !m_network->setEdgeCost(from,to,cost)) {
        setInlineError("Edge values are invalid."); return;
    }
    m_pendingFrom=m_pendingTo=-1; m_edgeEditor->hide(); setSelectedNode(-1); emit networkEdited(); update(); emit statusMessage(QString("Edge %1 → %2 saved.").arg(from).arg(to),false);
}

QPointF NetworkWidget::clampedPosition(const QPointF& p) const {
    const qreal pad=NODE_RADIUS+2;
    return QPointF(qBound(pad,p.x(),qreal(width())-pad),qBound(pad,p.y(),qreal(height())-pad));
}
void NetworkWidget::positionEdgeEditor() { }

void NetworkWidget::deleteSelectedNode() {
    if(!m_network || m_mode!=InteractionMode::Edit || m_selectedNode<1)return;
    const int removed=m_selectedNode;
    if(!m_network->removeNode(removed))return;
    cancelTransientEditing(); setSelectedNode(-1); emit networkEdited();
    emit statusMessage(QString("Node %1 deleted.").arg(removed),false); update();
}
void NetworkWidget::deleteAt(const QPoint& pos) {
    if (!m_network || m_mode != InteractionMode::Delete) return;
    const int node = getNodeAt(pos);
    if (node > 0) {
        if (!m_network->removeNode(node)) return;
        cancelTransientEditing();
        setSelectedNode(-1);
        emit networkEdited();
        emit statusMessage(QString("Node %1 deleted.").arg(node), false);
        update();
        return;
    }

    const auto& positions = m_network->getNodePositions();
    const int n = int(positions.size());
    QVector<GraphEditInteraction::EdgeSegment> segments;
    QVector<QPair<int,int>> endpoints;
    for (int from=1; from<=n; ++from) for (int to=1; to<=n; ++to) {
        if (m_network->getCapacity(from,to) <= 0) continue;
        auto segment = flowEdgeSegment(positions[size_t(from-1)], positions[size_t(to-1)]);
        segment.from = from;
        segment.to = to;
        segments.push_back(segment);
        endpoints.push_back({from,to});
    }
    const int hit = GraphEditInteraction::hitEdge(segments, pos, 8.0);
    if (hit < 0 || hit >= endpoints.size()) return;
    const auto [from,to] = endpoints[hit];
    if (!m_network->removeEdge(from,to)) return;
    cancelTransientEditing();
    emit networkEdited();
    emit statusMessage(QString("Edge %1 → %2 deleted.").arg(from).arg(to), false);
    update();
}
void NetworkWidget::keyPressEvent(QKeyEvent* e) {
    if(e->key()==Qt::Key_Escape && edgeEditorVisible()) { cancelPendingEdge(); return; }
    if((e->key()==Qt::Key_Delete || e->key()==Qt::Key_Backspace) && m_mode==InteractionMode::Edit) { deleteSelectedNode(); return; }
    QWidget::keyPressEvent(e);
}

void NetworkWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing); painter.fillRect(rect(),GraphUI::Surface);
    if(!m_network)return;
    const auto& iterations=m_solver ? m_solver->iterations() : std::vector<FlowIterationResult>{};
    const bool show=m_iterationIndex>=0 && m_iterationIndex<int(iterations.size());
    const std::vector<std::vector<std::int64_t>>* residual=nullptr;
    std::set<std::pair<int,int>> pathEdges;
    if(show) { const auto& state=iterations[m_iterationIndex]; residual=&state.residualCapacity; for(size_t i=1;i<state.augmentingPath.size();++i)pathEdges.insert({state.augmentingPath[i-1],state.augmentingPath[i]}); }
    const auto& positions=m_network->getNodePositions(); const int n=int(positions.size());
    for(int i=1;i<=n;++i) for(int j=1;j<=n;++j) {
        const bool onPath=pathEdges.count({i,j})>0;
        const int cap=m_network->getCapacity(i,j); const bool real=cap>0;
        if(!real && !onPath)continue;
        const bool reverse=real && m_network->getCapacity(j,i)>0 && i>j;
        std::int64_t rem=-1; if(show && residual && i-1<int(residual->size()) && j-1<int((*residual)[i-1].size())) rem=(*residual)[i-1][j-1];
        const bool minCut=show && m_iterationIndex==int(iterations.size())-1 && m_solver->isMinCutEdge(i,j);
        drawEdge(painter,positions[i-1],positions[j-1],cap,rem,m_network->getEdgeCost(i,j),minCut,onPath,reverse);
    }
    const int firstNode=m_editInteraction.selectedNodeId();
    if(firstNode>0 && firstNode<=n) { painter.setPen(QPen(GraphUI::Blue,2,Qt::DashLine)); painter.drawLine(positions[firstNode-1],m_tempMousePos); }
    if(m_pendingFrom>0 && m_pendingFrom<=n && m_pendingTo>0 && m_pendingTo<=n) {
        painter.setPen(QPen(GraphUI::Blue,2,Qt::DashLine));
        painter.drawLine(positions[m_pendingFrom-1],positions[m_pendingTo-1]);
    }
    for(int i=0;i<n;++i)drawNode(painter,i+1,positions[i],i+1==m_network->getSource(),i+1==m_network->getSink());
}

void NetworkWidget::drawNode(QPainter& p,int index,const QPointF& pos,bool source,bool sink) {
    QColor fill=GraphUI::BlueSoft,border=GraphUI::Blue; int w=2;
    if(source)w=4; if(sink){fill=QColor(255,237,239);border=GraphUI::Error;w=4;}
    if(index==m_selectedNode) {
        p.setPen(QPen(GraphUI::Text,2)); p.setBrush(Qt::NoBrush);
        p.drawEllipse(pos,NODE_RADIUS+5,NODE_RADIUS+5);
    }
    p.setPen(QPen(border,w));p.setBrush(fill);p.drawEllipse(pos,NODE_RADIUS,NODE_RADIUS);
    p.setPen(GraphUI::Text);QFont f=p.font();f.setPointSize(10);f.setBold(true);p.setFont(f);
    p.drawText(QRectF(pos.x()-NODE_RADIUS,pos.y()-NODE_RADIUS,2*NODE_RADIUS,2*NODE_RADIUS),Qt::AlignCenter,QString::number(index));
}

void NetworkWidget::drawEdge(QPainter& p,const QPointF& from,const QPointF& to,int capacity,std::int64_t residual,int cost,bool minCut,bool onPath,bool reverse) {
    QLineF line(from,to); double angle=std::atan2(line.dy(),line.dx());
    Q_UNUSED(reverse);
    const auto segment=flowEdgeSegment(from,to);
    QPointF start=segment.start;
    QPointF end=segment.end;
    QColor color=GraphUI::Muted;int width=2;if(minCut){color=GraphUI::Error;width=4;}else if(onPath){color=GraphUI::Blue;width=3;}
    p.setPen(QPen(color,width));p.drawLine(start,end);
    QPointF a=end-QPointF(ARROW_SIZE*std::cos(angle-std::numbers::pi/6),ARROW_SIZE*std::sin(angle-std::numbers::pi/6));
    QPointF b=end-QPointF(ARROW_SIZE*std::cos(angle+std::numbers::pi/6),ARROW_SIZE*std::sin(angle+std::numbers::pi/6));
    QPolygonF arrow;arrow<<end<<a<<b;p.setBrush(color);p.drawPolygon(arrow);
    if(residual>=0 || capacity>0) {
        QString label=residual<0?QString::number(capacity):QString::number(residual);
        if(cost)label+=QString(" [c:%1]").arg(cost);
        QPointF mid=(start+end)/2.0+QPointF(-std::sin(angle)*14,std::cos(angle)*14);
        QFont f=p.font();f.setPointSize(10);QFontMetrics fm(f);int w=fm.horizontalAdvance(label)+10,h=fm.height()+4;
        QRectF r(mid.x()-w/2,mid.y()-h/2,w,h);p.setPen(QPen(GraphUI::Border));p.setBrush(GraphUI::Surface);p.drawRoundedRect(r,4,4);p.setPen(GraphUI::Text);p.drawText(r,Qt::AlignCenter,label);
    }
}
