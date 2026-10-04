#include "features/mst/vis_mst.h"
#include "ui/theme.h"
#include "algorithms/kruskal_solver.h"
#include "algorithms/prim_solver.h"
#include "algorithms/boruvka_solver.h"
#include "rendering/graph_primitives.h"
#include <QPainter>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <vector>
#include <string>
#include <random>
#include <ctime>
#include <cmath>

VisMST::VisMST(QWidget* parent)
    : QWidget(parent), m_graph(nullptr), m_timer(new QTimer(this)),
      m_statusLabel(nullptr), m_timerLabel(new QLabel("0.00 s", this)),
      m_speedLabel(new QLabel("Speed", this)), m_pauseBtn(nullptr),
      m_startBtn(nullptr), m_algoGroup(new QButtonGroup(this)),
      m_speedSlider(new QSlider(Qt::Horizontal, this)), m_edgeWeightsCheck(nullptr), m_toolbar(nullptr),
      m_stepIdx(0), m_animDone(false), m_hasCurrent(false), m_currentAlgo(AlgoType::None)
{
    setMinimumSize(650, 350);
    auto* btnK = GraphUI::button("Kruskal", this, true);
    auto* btnP = GraphUI::button("Prim", this);
    auto* btnB = GraphUI::button("Boruvka", this);
    auto* btnR = GraphUI::button("New graph", this);
    btnK->setObjectName("mstKruskal"); btnP->setObjectName("mstPrim"); btnB->setObjectName("mstBoruvka");
    for (auto* b : {btnK, btnP, btnB}) { b->setCheckable(true); b->setProperty("role", "algorithm"); m_algoGroup->addButton(b); }
    m_algoGroup->setExclusive(true);
    m_startBtn = GraphUI::button("Start", this); m_startBtn->setObjectName("mstStart");
    m_statusLabel = GraphUI::status("Ready", this);
    m_pauseBtn = GraphUI::button("Pause", this);
    m_pauseBtn->setEnabled(false);
    m_speedSlider->setRange(1, 100);
    m_speedSlider->setValue(70);
    m_speedSlider->setFixedWidth(180);
    m_paused = false;
    m_elapsedMs = 0;
    m_timerLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    updateTimerLabel();

    auto* lbl = new QLabel("Accepted · Rejected · Final tree", this);
    lbl->setProperty("role", "muted");
    m_edgeWeightsCheck = new QCheckBox("Edge weights", this);
    connect(m_edgeWeightsCheck, &QCheckBox::toggled, this, [this](bool checked) {
        m_showWeights = checked;
        update();
    });
    m_toolbar = GraphUI::toolbar(this);
    delete m_toolbar->layout();
    auto* outer = new QVBoxLayout(m_toolbar);
    outer->setContentsMargins(12, 8, 12, 8); outer->setSpacing(6);
    auto* controls = new QHBoxLayout; controls->setSpacing(8);
    controls->addWidget(btnK); controls->addWidget(btnP); controls->addWidget(btnB);
    controls->addWidget(m_startBtn); controls->addWidget(btnR); controls->addWidget(m_pauseBtn);
    controls->addWidget(m_speedLabel); controls->addWidget(m_speedSlider);
    controls->addStretch(1);
    auto* info = new QHBoxLayout;
    info->addWidget(m_statusLabel, 1); info->addWidget(m_timerLabel); info->addSpacing(12);
    info->addWidget(lbl); info->addWidget(m_edgeWeightsCheck);
    outer->addLayout(controls); outer->addLayout(info);
    m_toolbar->setGeometry(0,0,width(),104);
    connect(btnK, &QPushButton::clicked, this, &VisMST::onKruskalClicked);
    connect(btnP, &QPushButton::clicked, this, &VisMST::onPrimClicked);
    connect(btnB, &QPushButton::clicked, this, &VisMST::onBoruvkaClicked);
    connect(btnR, &QPushButton::clicked, this, &VisMST::onResetClicked);
    btnK->setChecked(true); m_startBtn->setEnabled(true);
    connect(m_startBtn, &QPushButton::clicked, this, [this] { startAnimation(m_selectedAlgo); });
    connect(m_pauseBtn, &QPushButton::clicked, this, &VisMST::onPauseClicked);
    connect(m_speedSlider, &QSlider::valueChanged, this, &VisMST::onSpeedChanged);
    connect(m_timer, &QTimer::timeout, this, &VisMST::onAnimationTick);
    srand(static_cast<unsigned>(time(nullptr)));
    buildGraph();
}

VisMST::~VisMST() = default;

void VisMST::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (m_toolbar)
        m_toolbar->setGeometry(0, 0, width(), 104);
    if (m_graph) {
        const int gap = spacing();
        for (const auto& node : m_graph->getNodes()) {
            const int cell = node.getIndex() - 1;
            m_graph->setNodePosition(node.getIndex(), QPoint(offsetX() + (cell % COLS) * gap,
                                                            offsetY() + (cell / COLS) * gap));
        }
        update();
    }
}

void VisMST::hideEvent(QHideEvent* event) {
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

void VisMST::buildGraph() {
    m_timer->stop(); m_playback.cancel(); m_graph = std::make_unique<UndirectedGraph>();
    m_steps.clear(); m_accepted.clear(); m_rejected.clear();
    m_hasCurrent = false; m_stepIdx = 0; m_animDone = false; m_currentAlgo = AlgoType::None;
    m_paused = false;
    m_pauseBtn->setText("Pause");
    m_pauseBtn->setEnabled(false);
    m_elapsedMs = 0;
    updateTimerLabel();
    m_statusLabel->clear();
    updateLabel();
    std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
    std::uniform_int_distribution<int> costDist(1, 99);
    const int ox = offsetX(), oy = offsetY(), gap = spacing();
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++) m_graph->addNode(QPoint(ox+c*gap, oy+r*gap));
    
    auto idx = [&](int r, int c){ return r*COLS+c; };
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS-1;c++) m_graph->addEdge(idx(r,c)+1, idx(r,c+1)+1, costDist(rng));
    for(int r=0;r<ROWS-1;r++) for(int c=0;c<COLS;c++) m_graph->addEdge(idx(r,c)+1, idx(r+1,c)+1, costDist(rng));
    update();
}

void VisMST::startAnimation(AlgoType algo) {
    m_timer->stop(); m_currentAlgo = algo;
    m_playback.start();
    m_accepted.clear(); m_rejected.clear(); m_hasCurrent = false; m_stepIdx = 0; m_animDone = false;
    m_paused = false;
    m_pauseBtn->setText("Pause");
    m_pauseBtn->setEnabled(true);
    m_elapsedMs = 0;
    m_elapsedClock.restart();
    updateTimerLabel();
    m_statusLabel->setText("Initializing...");
    if(algo==AlgoType::Kruskal) m_steps=KruskalSolver{}.solve(*m_graph);
    else if(algo==AlgoType::Prim) m_steps=PrimSolver{}.solve(*m_graph);
    else m_steps=BoruvkaSolver{}.solve(*m_graph);
    updateLabel();
    m_timer->start(currentInterval());
}

void VisMST::selectAlgorithm(AlgoType algo) {
    if (m_selectedAlgo == algo) return;
    m_selectedAlgo = algo;
    m_startBtn->setEnabled(true);
    m_timer->stop(); m_playback.cancel(); m_currentAlgo = AlgoType::None; m_steps.clear();
    m_accepted.clear(); m_rejected.clear(); m_hasCurrent = false; m_stepIdx = 0; m_animDone = false;
    m_paused = false; m_pauseBtn->setText("Pause"); m_pauseBtn->setEnabled(false);
    m_elapsedMs = 0; updateTimerLabel(); updateLabel(); update();
}

void VisMST::onAnimationTick() {
    if(m_stepIdx >= static_cast<int>(m_steps.size())) { 
        m_elapsedMs += m_elapsedClock.elapsed();
        m_timer->stop(); m_hasCurrent=false; m_animDone=true; m_playback.complete();
        m_pauseBtn->setEnabled(false);
        updateTimerLabel();
        updateLabel();
        update(); return; 
    }
    m_current = m_steps[m_stepIdx++]; m_hasCurrent = true;
    if(m_current.accepted) m_accepted.push_back(m_current); else m_rejected.push_back(m_current);
    updateLabel();
    updateTimerLabel();
    update();
}

void VisMST::updateLabel() {
    if(m_currentAlgo == AlgoType::None) {
        const QString selected = m_selectedAlgo == AlgoType::Kruskal ? "Kruskal" : m_selectedAlgo == AlgoType::Prim ? "Prim" : "Boruvka";
        m_statusLabel->setText(QString("%1 selected · Ready to start").arg(selected));
        return;
    }
    QString algoName;
    if(m_currentAlgo==AlgoType::Kruskal) algoName="Kruskal";
    else if(m_currentAlgo==AlgoType::Prim) algoName="Prim";
    else algoName="Boruvka";
    
    int totalCost=0; for(const auto& s:m_accepted) totalCost+=s.cost;
    QString status = m_animDone ? QString("%1: Done! Total cost: %2").arg(algoName).arg(totalCost)
                                : QString("%1: Building... (%2 edges)").arg(algoName).arg(static_cast<int>(m_accepted.size()));
    m_statusLabel->setText(status);
    m_statusLabel->setProperty("role", m_animDone ? "success" : "status");
}

void VisMST::updateTimerLabel() {
    qint64 totalMs = m_elapsedMs;
    if (!m_paused && m_timer->isActive())
        totalMs += m_elapsedClock.elapsed();
    double seconds = static_cast<double>(totalMs) / 1000.0;
    m_timerLabel->setText(QString("Time: %1s").arg(seconds, 0, 'f', 2));
}

int VisMST::currentInterval() const {
    int slider = m_speedSlider ? m_speedSlider->value() : 70;
    int minMs = 2;
    int maxMs = 120;
    int slow = 101 - slider;
    int range = maxMs - minMs;
    return minMs + (slow * slow * range) / 10000;
}

void VisMST::onKruskalClicked() { selectAlgorithm(AlgoType::Kruskal); }
void VisMST::onPrimClicked() { selectAlgorithm(AlgoType::Prim); }
void VisMST::onBoruvkaClicked() { selectAlgorithm(AlgoType::Boruvka); }
void VisMST::onResetClicked() { buildGraph(); }
void VisMST::onSpeedChanged(int) {
    if (m_timer->isActive())
        m_timer->setInterval(currentInterval());
}
void VisMST::onPauseClicked() {
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

void VisMST::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing); p.fillRect(rect(), GraphUI::Canvas);
    const int R = NODE_R;
    auto nodePos = [&](int idx) -> QPointF {
        for(const auto& n : m_graph->getNodes()) if(n.getIndex()==idx) return QPointF(n.getX(), n.getY());
        return QPointF(0,0);
    };
    auto drawEd = [&](const MSTStep& s, QColor col, int width, bool showCost) {
        QPointF a=nodePos(s.fromIndex), b=nodePos(s.toIndex);
        GraphPrimitives::drawEdgeSegment(p, a, b, col, width);
        if(showCost && m_showWeights) {
            QPointF mid=(a+b)/2.0; QLineF ln(a,b); double angle=std::atan2(ln.dy(),ln.dx());
            QPointF off(-10*std::sin(angle), 10*std::cos(angle));
            p.save(); QFont f=p.font(); f.setPointSize(8); f.setBold(true); p.setFont(f);
            p.setPen(col); p.drawText(mid+off, QString::number(s.cost)); p.restore();
        }
    };
    for(const auto& ed : m_graph->getEdges()) {
        MSTStep s{ed.getFirst().getIndex(), ed.getSecond().getIndex(), ed.getCost(), false};
        drawEd(s, GraphUI::Edge, 1, m_showWeights);
    }
    for(const auto& s : m_rejected) drawEd(s, GraphUI::Error, 1, m_showWeights);
    QColor mstCol = m_animDone ? GraphUI::Blue : GraphUI::Success;
    for(const auto& s : m_accepted) drawEd(s, mstCol, 2, m_showWeights);
    if(m_hasCurrent && !m_animDone) {
        QColor cur = m_current.accepted ? GraphUI::Success : GraphUI::Error;
        drawEd(m_current, cur, 3, m_showWeights);
    }
    for(const auto& n : m_graph->getNodes()) {
        QRect r(n.getX()-R, n.getY()-R, R*2, R*2);
        bool inMST = false;
        for(const auto& s : m_accepted) if(s.fromIndex==n.getIndex()||s.toIndex==n.getIndex()) { inMST=true; break; }
        QColor fill = inMST ? (m_animDone ? GraphUI::BlueSoft : GraphUI::Success) : GraphUI::Surface;
        QColor border = inMST ? (m_animDone ? GraphUI::Blue : GraphUI::Success) : GraphUI::Muted;
        GraphPrimitives::drawLabeledNode(p, r, fill, border, QString::number(n.getIndex()), GraphUI::Text, 9);
    }
}
