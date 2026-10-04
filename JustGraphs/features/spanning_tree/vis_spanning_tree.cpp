#include "features/spanning_tree/vis_spanning_tree.h"
#include "rendering/graph_primitives.h"
#include "ui/theme.h"
#include "algorithms/dfs_spanning_tree_solver.h"
#include "algorithms/bfs_spanning_tree_solver.h"
#include <QPainter>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <ctime>
#include <cmath>
#include <numbers>

VisSpanningTree::VisSpanningTree(QWidget* parent)
    : QWidget(parent), m_graph(nullptr), m_timer(new QTimer(this)),
      m_statusLabel(nullptr), m_timerLabel(new QLabel("0.00 s", this)),
      m_speedLabel(new QLabel("Speed", this)), m_pauseBtn(nullptr),
      m_startBtn(nullptr), m_algoGroup(new QButtonGroup(this)),
      m_speedSlider(new QSlider(Qt::Horizontal, this)), m_toolbar(nullptr),
      m_currentAlgo(AlgoType::None), m_stepIdx(0), m_animDone(false),
      m_currentNode(-1), m_currentEdge({-1,-1}), m_rootNode(0)
{
    setMinimumSize(400, 350);
    
    auto* btnDFS = GraphUI::button("DFS tree", this);
    auto* btnBFS = GraphUI::button("BFS tree", this, true);
    auto* btnR = GraphUI::button("New graph", this);
    btnDFS->setObjectName("spanningDfs"); btnBFS->setObjectName("spanningBfs");
    for (auto* b : {btnDFS, btnBFS}) { b->setCheckable(true); b->setProperty("role", "algorithm"); m_algoGroup->addButton(b); }
    m_algoGroup->setExclusive(true);
    m_startBtn = GraphUI::button("Start", this); m_startBtn->setObjectName("spanningStart");
    m_statusLabel = GraphUI::status("Ready", this);
    m_pauseBtn = GraphUI::button("Pause", this);
    
    m_pauseBtn->setEnabled(false);
    m_speedSlider->setRange(1, 100);
    m_speedSlider->setValue(50);
    m_speedSlider->setFixedWidth(150);
    m_paused = false;
    m_elapsedMs = 0;
    
    m_timerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    updateTimerLabel();

    auto* lbl = new QLabel("Tree edge   Back edge   Cross edge   Unvisited   Root", this);
    lbl->setProperty("role", "muted");
    
    m_toolbar = GraphUI::toolbar(this);
    delete m_toolbar->layout();
    auto* vbox = new QVBoxLayout(m_toolbar);
    vbox->setContentsMargins(12,8,12,8);
    vbox->setSpacing(6);
    
    auto* hbox = new QHBoxLayout();
    hbox->addWidget(btnDFS); 
    hbox->addWidget(btnBFS); 
    hbox->addWidget(m_startBtn); hbox->addWidget(btnR); 
    hbox->addWidget(m_pauseBtn);
    hbox->addWidget(m_speedLabel); 
    hbox->addWidget(m_speedSlider);
    hbox->addStretch(1);
    auto* info = new QHBoxLayout;
    info->addWidget(m_statusLabel, 1);
    info->addWidget(m_timerLabel);
    info->addSpacing(12);
    info->addWidget(lbl);
    
    vbox->addLayout(hbox);
    vbox->addLayout(info);
    
    m_toolbar->setGeometry(0,0,width(), 104);
    
    connect(btnDFS, &QPushButton::clicked, this, &VisSpanningTree::onDFSClicked);
    connect(btnBFS, &QPushButton::clicked, this, &VisSpanningTree::onBFSClicked);
    connect(btnR, &QPushButton::clicked, this, &VisSpanningTree::onResetClicked);
    btnDFS->setChecked(true); m_startBtn->setEnabled(true);
    connect(m_startBtn, &QPushButton::clicked, this, [this] { startAnimation(m_selectedAlgo); });
    connect(m_pauseBtn, &QPushButton::clicked, this, &VisSpanningTree::onPauseClicked);
    connect(m_speedSlider, &QSlider::valueChanged, this, &VisSpanningTree::onSpeedChanged);
    connect(m_timer, &QTimer::timeout, this, &VisSpanningTree::onAnimationTick);
    
    srand(static_cast<unsigned>(time(nullptr)));
    // buildGraph() is now called in showEvent to ensure proper centering
}

VisSpanningTree::~VisSpanningTree() = default;

void VisSpanningTree::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (m_toolbar)
        m_toolbar->setGeometry(0, 0, width(), 104);
}

void VisSpanningTree::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (!m_graph || m_graph->getNodes().empty()) {
        buildGraph();
    }
}

void VisSpanningTree::hideEvent(QHideEvent* event) {
    if (m_timer->isActive()) {
        m_timer->stop();
        m_elapsedMs += m_elapsedClock.elapsed();
        m_playback.pause();
        m_paused = true;
        m_pauseBtn->setText("Resume");
        updateTimerLabel();
    }
    QWidget::hideEvent(event);
}

void VisSpanningTree::buildGraph() {
    m_timer->stop(); m_playback.cancel();
    m_graph = std::make_unique<UndirectedGraph>();
    
    m_steps.clear(); 
    m_stepIdx = 0; 
    m_animDone = false; 
    m_currentAlgo = AlgoType::None;
    m_paused = false;
    m_pauseBtn->setText("Pause");
    m_pauseBtn->setEnabled(false);
    m_elapsedMs = 0;
    
    m_visitedNodes.clear();
    m_completedNodes.clear();
    m_treeEdges.clear();
    m_backEdges.clear();
    m_crossEdges.clear();
    m_currentNode = -1;
    m_currentEdge = {-1, -1};
    
    updateTimerLabel();
    m_statusLabel->clear();
    updateLabel();
    
    std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
    std::uniform_int_distribution<int> costDist(1, 10);
    
    // Organic layout with ~10 nodes
    const int w = width();
    const int h = height();
    int cx = w / 2;
    constexpr int toolbarHeight = 104;
    const int availableHeight = std::max(1, h - toolbarHeight);
    int cy = toolbarHeight + availableHeight / 2;
    int radius = std::min(w, availableHeight) / 3;
    
    int numNodes = 10;
    for (int i = 0; i < numNodes; ++i) {
        double angle = 2.0 * std::numbers::pi * i / numNodes;
        int x = cx + static_cast<int>(radius * std::cos(angle));
        int y = cy + static_cast<int>(radius * std::sin(angle));
        
        // Add some jitter
        x += (rng() % 40) - 20;
        y += (rng() % 40) - 20;
        
        m_graph->addNode(QPoint(x, y));
    }
    
    // Outer ring
    for (int i = 0; i < numNodes; ++i) {
        m_graph->addEdge(i + 1, (i + 1) % numNodes + 1, costDist(rng));
    }
    
    // Some cross connections to create cycles and cross edges
    m_graph->addEdge(1, 5, costDist(rng));
    m_graph->addEdge(2, 7, costDist(rng));
    m_graph->addEdge(4, 8, costDist(rng));
    m_graph->addEdge(9, 3, costDist(rng));
    m_graph->addEdge(6, 10, costDist(rng));
    
    m_rootNode = 1;
    update();
}

void VisSpanningTree::startAnimation(AlgoType algo) {
    m_timer->stop(); 
    m_playback.start();
    m_currentAlgo = algo;
    
    m_visitedNodes.clear();
    m_completedNodes.clear();
    m_treeEdges.clear();
    m_backEdges.clear();
    m_crossEdges.clear();
    m_currentNode = -1;
    m_currentEdge = {-1, -1};
    
    m_stepIdx = 0; 
    m_animDone = false;
    m_paused = false;
    m_pauseBtn->setText("Pause");
    m_pauseBtn->setEnabled(true);
    m_elapsedMs = 0;
    m_elapsedClock.restart();
    
    updateTimerLabel();
    m_statusLabel->setText("Initializing...");
    
    if(algo == AlgoType::DFS) {
        m_steps = DFSSpanningTreeSolver{}.solve(*m_graph, m_rootNode);
    } else if(algo == AlgoType::BFS) {
        m_steps = BFSSpanningTreeSolver{}.solve(*m_graph, m_rootNode);
    }
    updateLabel();
    m_timer->start(currentInterval());
}

void VisSpanningTree::selectAlgorithm(AlgoType algo) {
    if (m_selectedAlgo == algo) return;
    m_selectedAlgo = algo; m_timer->stop(); m_playback.cancel(); m_currentAlgo = AlgoType::None;
    m_steps.clear(); m_stepIdx = 0; m_animDone = false; m_paused = false;
    m_pauseBtn->setText("Pause"); m_pauseBtn->setEnabled(false); m_elapsedMs = 0;
    m_visitedNodes.clear(); m_completedNodes.clear(); m_treeEdges.clear(); m_backEdges.clear(); m_crossEdges.clear();
    m_currentNode = -1; m_currentEdge = {-1, -1};
    updateTimerLabel(); updateLabel(); update();
}

void VisSpanningTree::onAnimationTick() {
    if(m_stepIdx >= static_cast<int>(m_steps.size())) { 
        m_elapsedMs += m_elapsedClock.elapsed();
        m_timer->stop(); 
        m_animDone = true; m_playback.complete();
        m_currentNode = -1;
        m_currentEdge = {-1, -1};
        m_pauseBtn->setEnabled(false);
        updateTimerLabel();
        updateLabel();
        update(); 
        return; 
    }
    
    SpanningTreeStep step = m_steps[m_stepIdx++];
    m_currentAction = step.action;
    
    auto getOrderedEdge = [](int u, int v) {
        return (u < v) ? std::make_pair(u, v) : std::make_pair(v, u);
    };

    switch(step.action) {
        case SpanningTreeStep::Action::VisitNode:
            m_visitedNodes.insert(step.nodeIndex);
            m_currentNode = step.nodeIndex;
            m_currentEdge = {-1, -1};
            break;
        case SpanningTreeStep::Action::TreeEdge:
            m_treeEdges.insert(getOrderedEdge(step.edgeFrom, step.edgeTo));
            m_currentEdge = getOrderedEdge(step.edgeFrom, step.edgeTo);
            break;
        case SpanningTreeStep::Action::BackEdge:
            m_backEdges.insert(getOrderedEdge(step.edgeFrom, step.edgeTo));
            m_currentEdge = getOrderedEdge(step.edgeFrom, step.edgeTo);
            break;
        case SpanningTreeStep::Action::CrossEdge:
            m_crossEdges.insert(getOrderedEdge(step.edgeFrom, step.edgeTo));
            m_currentEdge = getOrderedEdge(step.edgeFrom, step.edgeTo);
            break;
        case SpanningTreeStep::Action::CompleteNode:
            m_completedNodes.insert(step.nodeIndex);
            m_currentNode = step.nodeIndex;
            m_currentEdge = {-1, -1};
            break;
    }
    
    updateLabel();
    updateTimerLabel();
    update();
}

void VisSpanningTree::updateLabel() {
    if(m_currentAlgo == AlgoType::None) {
        m_statusLabel->setText(m_selectedAlgo == AlgoType::DFS ? "DFS selected · Ready to start" : "BFS selected · Ready to start");
        return;
    }
    
    QString algoName = (m_currentAlgo == AlgoType::DFS) ? "DFS Spanning Tree" : "BFS Spanning Tree";
    
    if (m_animDone) {
        m_statusLabel->setText(QString("%1: Done! (%2 tree edges, %3 back edges, %4 cross edges)")
            .arg(algoName).arg(m_treeEdges.size()).arg(m_backEdges.size()).arg(m_crossEdges.size()));
    } else {
        m_statusLabel->setText(QString("%1: Building... (%2/%3 nodes, %4 tree edges)")
            .arg(algoName).arg(m_visitedNodes.size()).arg(m_graph->getNodes().size()).arg(m_treeEdges.size()));
    }
    
    m_statusLabel->setProperty("role", m_animDone ? "success" : "status");
}

void VisSpanningTree::updateTimerLabel() {
    qint64 totalMs = m_elapsedMs;
    if (!m_paused && m_timer->isActive())
        totalMs += m_elapsedClock.elapsed();
    double seconds = static_cast<double>(totalMs) / 1000.0;
    m_timerLabel->setText(QString("Time: %1s").arg(seconds, 0, 'f', 2));
}

int VisSpanningTree::currentInterval() const {
    int slider = m_speedSlider ? m_speedSlider->value() : 50;
    int minMs = 50;
    int maxMs = 1500;
    int slow = 101 - slider;
    int range = maxMs - minMs;
    return minMs + (slow * slow * range) / 10000;
}

void VisSpanningTree::onDFSClicked() { selectAlgorithm(AlgoType::DFS); }
void VisSpanningTree::onBFSClicked() { selectAlgorithm(AlgoType::BFS); }
void VisSpanningTree::onResetClicked() { buildGraph(); }
void VisSpanningTree::onSpeedChanged(int) {
    if (m_timer->isActive())
        m_timer->setInterval(currentInterval());
}

void VisSpanningTree::onPauseClicked() {
    if (m_currentAlgo == AlgoType::None || m_animDone)
        return;
    if (m_paused) {
        if (!m_playback.resume()) return;
        m_paused = false;
        m_pauseBtn->setText("Pause");
        m_elapsedClock.restart();
        m_timer->start(currentInterval());
        return;
    }
    if (!m_playback.pause()) return;
    m_paused = true;
    m_pauseBtn->setText("Resume");
    m_elapsedMs += m_elapsedClock.elapsed();
    m_timer->stop();
    updateTimerLabel();
}

void VisSpanningTree::paintEvent(QPaintEvent*) {
    QPainter p(this); 
    p.setRenderHint(QPainter::Antialiasing); 
    p.fillRect(rect(), GraphUI::Canvas);
    
    auto nodePos = [&](int idx) -> QPointF {
        for(const auto& n : m_graph->getNodes()) 
            if(n.getIndex() == idx) return QPointF(n.getX(), n.getY());
        return QPointF(0,0);
    };
    
    auto getOrderedEdge = [](int u, int v) {
        return (u < v) ? std::make_pair(u, v) : std::make_pair(v, u);
    };
    
    auto drawEd = [&](int u, int v, QColor col, int width, bool isDashed, int cost) {
        QPointF a = nodePos(u), b = nodePos(v);
        GraphPrimitives::drawEdgeSegment(p, a, b, col, width, isDashed ? Qt::DashLine : Qt::SolidLine);
        
        // Draw cost
        QPointF mid = (a + b) / 2.0; 
        QLineF ln(a, b); 
        double angle = std::atan2(ln.dy(), ln.dx());
        QPointF off(-10 * std::sin(angle), 10 * std::cos(angle));
        
        p.save(); 
        QFont f = p.font(); 
        f.setPointSize(8); 
        f.setBold(true); 
        p.setFont(f);
        p.setPen(GraphUI::Muted);
        p.drawText(mid + off, QString::number(cost)); 
        p.restore();
    };

    // Draw all edges
    for(const auto& ed : m_graph->getEdges()) {
        int u = ed.getFirst().getIndex();
        int v = ed.getSecond().getIndex();
        int cost = ed.getCost();
        
        auto edgeKey = getOrderedEdge(u, v);
        
        if (m_treeEdges.count(edgeKey)) {
            // Tree Edge
            drawEd(u, v, GraphUI::Success, 3, false, cost);
        } else if (m_backEdges.count(edgeKey)) {
            // Back Edge
            drawEd(u, v, GraphUI::Error, 2, true, cost);
        } else if (m_crossEdges.count(edgeKey)) {
            // Cross Edge
            drawEd(u, v, GraphUI::Warning, 2, true, cost);
        } else {
            // Unexplored
            drawEd(u, v, GraphUI::Edge, 1, false, cost);
        }
        
        // Highlight active edge
        if (!m_animDone && m_currentEdge == edgeKey) {
            p.save();
            GraphPrimitives::drawEdgeSegment(p, nodePos(u), nodePos(v), GraphUI::Blue, 3);
            p.restore();
        }
    }

    // Draw all nodes
    const int R = 12;
    for(const auto& n : m_graph->getNodes()) {
        int idx = n.getIndex();
        QRectF r(n.getX() - R, n.getY() - R, R * 2, R * 2);
        
        QColor fill = GraphUI::Surface;
        QColor border = GraphUI::Border;
        
        if (m_completedNodes.count(idx)) {
            fill = GraphUI::BlueSoft;
            border = GraphUI::Blue;
        } else if (m_visitedNodes.count(idx)) {
            fill = GraphUI::BlueSoft;
            border = GraphUI::Blue;
        }
        
        // Highlight active node
        if (!m_animDone && m_currentNode == idx) {
            fill = GraphUI::Blue;
            border = GraphUI::Blue;
            
            // Glow effect
            p.save();
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(GraphUI::Blue, 2));
            p.drawEllipse(n.getX() - R - 2, n.getY() - R - 2, (R + 2) * 2, (R + 2) * 2);
            p.restore();
        }
        
        // Shared node primitive is drawn after the root decorator below.
        
        // Root Node Decorator
        if (idx == m_rootNode) {
            p.save();
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(GraphUI::Text, 2));
            p.drawEllipse(n.getX() - R - 4, n.getY() - R - 4, (R + 4) * 2, (R + 4) * 2);
            QFont f = p.font();
            f.setPointSize(8);
            p.setFont(f);
            p.setPen(GraphUI::Text);
            p.drawText(n.getX() - 12, n.getY() - R - 6, "root");
            p.restore();
        }
        
        // Text
        GraphPrimitives::drawLabeledNode(p, r, fill, border, QString::number(idx), GraphUI::Text, 9, 2);
    }
}
