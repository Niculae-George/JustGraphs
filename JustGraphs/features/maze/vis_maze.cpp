#include "features/maze/vis_maze.h"
#include "ui/theme.h"
#include "algorithms/bfs_traversal.h"
#include "algorithms/dfs_traversal.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <ctime>
#include <random>
#include <stack>
#include <algorithm>
#include "algorithms/i_traversal.h"

static constexpr int DR[4]  = {-1,  0,  1,  0};
static constexpr int DC[4]  = { 0,  1,  0, -1};
static constexpr int OPP[4] = { 2,  3,  0,  1};
static inline int cellToIndex(int r, int c, int cols) { return r * cols + c + 1; }
static inline int indexToRow(int idx, int cols) { return (idx - 1) / cols; }
static inline int indexToCol(int idx, int cols) { return (idx - 1) % cols; }

VisMaze::VisMaze(QWidget* parent)
    : QWidget(parent), m_graph(nullptr), m_timer(new QTimer(this)), m_animStep(0), m_animDone(false),
      m_showingPath(false), m_pathStep(0), m_currentAlgo(AlgoType::None),
      m_selectedAlgo(AlgoType::BFS), m_toolbar(nullptr)
{
    setMinimumSize(400, 350);

    m_bfsBtn = GraphUI::button("BFS", this);
    m_dfsBtn = GraphUI::button("DFS", this);
    m_bfsBtn->setObjectName("mazeBfs");
    m_dfsBtn->setObjectName("mazeDfs");
    m_bfsBtn->setProperty("role", "algorithm");
    m_dfsBtn->setProperty("role", "algorithm");
    m_bfsBtn->setCheckable(true);
    m_dfsBtn->setCheckable(true);
    m_bfsBtn->setChecked(true);
    m_algorithmGroup = new QButtonGroup(this);
    m_algorithmGroup->setExclusive(true);
    m_algorithmGroup->addButton(m_bfsBtn);
    m_algorithmGroup->addButton(m_dfsBtn);
    m_startBtn = GraphUI::button("Start", this);
    m_startBtn->setObjectName("mazeStart");
    auto* btnReset = GraphUI::button("New maze", this);
    m_pauseBtn = GraphUI::button("Pause", this);
    m_statusLabel = GraphUI::status("Ready. Choose BFS or DFS", this);
    m_hintLabel = new QLabel("Start: top-left  |  Exit: bottom-right", this);
    m_speedLabel = new QLabel("Speed", this);
    m_speedSlider = new QSlider(Qt::Horizontal, this);
    m_pauseBtn->setEnabled(false);
    m_paused = false;
    m_tickInterval = 18;
    m_speedSlider->setRange(1, 100);
    m_speedSlider->setValue(55);
    m_speedSlider->setFixedWidth(150);
    m_hintLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_speedLabel->setAlignment(Qt::AlignVCenter);
    m_toolbar = GraphUI::toolbar(this);
    delete m_toolbar->layout();
    auto* outer = new QVBoxLayout(m_toolbar);
    outer->setContentsMargins(12, 8, 12, 8);
    outer->setSpacing(6);
    auto* controls = new QHBoxLayout;
    controls->setSpacing(8);
    controls->addWidget(m_bfsBtn); controls->addWidget(m_dfsBtn); controls->addWidget(m_startBtn); controls->addWidget(btnReset);
    controls->addWidget(m_pauseBtn); controls->addSpacing(8);
    controls->addWidget(m_speedLabel); controls->addWidget(m_speedSlider);
    controls->addStretch(1);
    auto* info = new QHBoxLayout;
    info->addWidget(m_statusLabel, 1);
    info->addWidget(m_hintLabel);
    outer->addLayout(controls); outer->addLayout(info);
    m_toolbar->setGeometry(0, 0, width(), 104);

    connect(m_bfsBtn, &QPushButton::clicked, this, &VisMaze::onBfsClicked);
    connect(m_dfsBtn, &QPushButton::clicked, this, &VisMaze::onDfsClicked);
    connect(m_startBtn, &QPushButton::clicked, this, [this] { startAnimation(m_selectedAlgo); });
    connect(btnReset, &QPushButton::clicked, this, &VisMaze::onResetClicked);
    connect(m_pauseBtn, &QPushButton::clicked, this, &VisMaze::onPauseClicked);
    connect(m_speedSlider, &QSlider::valueChanged, this, &VisMaze::onSpeedChanged);
    connect(m_timer, &QTimer::timeout, this, &VisMaze::onAnimationTick);

    updateStatusLabel();
    srand(static_cast<unsigned>(time(nullptr)));
    generateMaze();
}

VisMaze::~VisMaze() = default;

void VisMaze::resizeEvent(QResizeEvent* e) {
    QWidget::resizeEvent(e);
    if (m_toolbar)
        m_toolbar->setGeometry(0, 0, width(), 104);
}

void VisMaze::hideEvent(QHideEvent* event) {
    if (m_timer->isActive()) {
        m_timer->stop();
        m_playback.pause();
        m_paused = true;
        m_pauseBtn->setText("Resume");
        updateStatusLabel();
    }
    QWidget::hideEvent(event);
}

void VisMaze::generateMaze()
{
    m_timer->stop();
    m_playback.cancel();
    m_maze.assign(ROWS, std::vector<Cell>(COLS));
    m_visited.assign(ROWS, std::vector<bool>(COLS, false));
    m_visitOrder.clear(); m_finalPath.clear(); m_pathHighlight.clear();
    m_animStep = 0; m_animDone = false; m_showingPath = false; m_pathStep = 0;
    m_currentAlgo = AlgoType::None;
    m_paused = false;
    m_tickInterval = 18;
    m_pauseBtn->setText("Pause");
    m_pauseBtn->setEnabled(false);
    updateStatusLabel();

    std::mt19937 rng(static_cast<unsigned>(time(nullptr)));
    std::vector<std::vector<bool>> visited(ROWS, std::vector<bool>(COLS, false));
    std::stack<std::pair<int,int>> stk;
    stk.emplace(0, 0); visited[0][0] = true;
    while(!stk.empty()) {
        auto [r, c] = stk.top();
        std::vector<int> neighbors;
        for(int d = 0; d < 4; d++) {
            int nr = r+DR[d], nc = c+DC[d];
            if(nr>=0 && nr<ROWS && nc>=0 && nc<COLS && !visited[nr][nc]) neighbors.push_back(d);
        }
        if(neighbors.empty()) { stk.pop(); }
        else {
            std::uniform_int_distribution<int> dist(0, static_cast<int>(neighbors.size())-1);
            int d = neighbors[dist(rng)]; int nr = r+DR[d], nc = c+DC[d];
            m_maze[r][c].walls[d] = false; m_maze[nr][nc].walls[OPP[d]] = false;
            visited[nr][nc] = true; stk.emplace(nr, nc);
        }
    }

    m_graph = std::make_unique<DirectedGraph>();
    const int cell = cellSize();
    const int mazeX = offsetX(), mazeY = offsetY();
    for(int r = 0; r < ROWS; r++)
        for(int c = 0; c < COLS; c++)
            m_graph->addNode(QPoint(mazeX + c*cell + cell/2, mazeY + r*cell + cell/2));
    
    for(int r = 0; r < ROWS; r++)
        for(int c = 0; c < COLS; c++)
            for(int d = 0; d < 4; d++) {
                if(m_maze[r][c].walls[d]) continue;
                int nr = r+DR[d], nc = c+DC[d];
                m_graph->addEdge(cellToIndex(r, c, COLS), cellToIndex(nr, nc, COLS));
            }
    update();
}

static void reconstructPath(int endIdx, int cols, const std::unordered_map<int,int>& parent,
                            std::vector<std::pair<int,int>>& outPath) {
    outPath.clear();
    for(int cur = endIdx; cur != -1; cur = parent.count(cur) ? parent.at(cur) : -1)
        outPath.emplace_back(indexToRow(cur, cols), indexToCol(cur, cols));
    std::reverse(outPath.begin(), outPath.end());
}

std::vector<std::pair<int,int>> VisMaze::runTraversal(const ITraversalAlgorithm& algorithm,
                                                       std::vector<std::pair<int,int>>& outPath) {
    const int startIdx = cellToIndex(0, 0, COLS), endIdx = cellToIndex(ROWS-1, COLS-1, COLS);
    std::unordered_map<int,int> parent;
    std::vector<TraversalStep> steps = algorithm.solve(*m_graph, startIdx, endIdx);
    
    std::vector<std::pair<int,int>> result;
    for(const auto& s : steps) {
        result.emplace_back(indexToRow(s.nodeIndex, COLS), indexToCol(s.nodeIndex, COLS));
        if (s.parentIndex != -1) parent[s.nodeIndex] = s.parentIndex;
    }
    if (parent.find(startIdx) == parent.end()) parent[startIdx] = -1;

    reconstructPath(endIdx, COLS, parent, outPath);
    return result;
}

void VisMaze::startAnimation(AlgoType algo) {
    m_timer->stop(); m_currentAlgo = algo;
    m_playback.start();
    m_paused = false;
    m_pauseBtn->setText("Pause");
    m_pauseBtn->setEnabled(true);
    m_visited.assign(ROWS, std::vector<bool>(COLS, false));
    m_pathHighlight.clear(); m_animStep = 0; m_animDone = false; m_showingPath = false; m_pathStep = 0;
    if(algo == AlgoType::BFS) m_visitOrder = runTraversal(BFSTraversal{}, m_finalPath);
    else m_visitOrder = runTraversal(DFSTraversal{}, m_finalPath);
    m_tickInterval = currentInterval();
    updateStatusLabel();
    m_timer->start(m_tickInterval);
}

void VisMaze::onAnimationTick() {
    if(!m_showingPath) {
        if(m_animStep < static_cast<int>(m_visitOrder.size())) {
            auto [r, c] = m_visitOrder[m_animStep++]; m_visited[r][c] = true; update();
            updateStatusLabel();
        } else { m_showingPath = true; m_pathStep = 0; m_tickInterval = currentInterval(); m_timer->setInterval(m_tickInterval); updateStatusLabel(); }
    } else {
        if(m_pathStep < static_cast<int>(m_finalPath.size())) { m_pathHighlight.push_back(m_finalPath[m_pathStep++]); update(); updateStatusLabel(); }
        else { m_timer->stop(); m_animDone = true; m_playback.complete(); m_pauseBtn->setEnabled(false); updateStatusLabel(); update(); }
    }
}

void VisMaze::clearVisualization() {
    m_timer->stop();
    m_playback.cancel();
    m_currentAlgo = AlgoType::None;
    m_visited.assign(ROWS, std::vector<bool>(COLS, false));
    m_visitOrder.clear(); m_finalPath.clear(); m_pathHighlight.clear();
    m_animStep = 0; m_animDone = false; m_showingPath = false; m_pathStep = 0;
    m_paused = false;
    m_pauseBtn->setText("Pause");
    m_pauseBtn->setEnabled(false);
    updateStatusLabel();
    update();
}

void VisMaze::onBfsClicked() {
    if (m_selectedAlgo == AlgoType::BFS)
        return;
    m_selectedAlgo = AlgoType::BFS;
    clearVisualization();
}
void VisMaze::onDfsClicked() {
    if (m_selectedAlgo == AlgoType::DFS)
        return;
    m_selectedAlgo = AlgoType::DFS;
    clearVisualization();
}
void VisMaze::onResetClicked() { m_timer->stop(); generateMaze(); }
void VisMaze::onPauseClicked() {
    if (m_currentAlgo == AlgoType::None || m_animDone)
        return;
    if (m_paused) {
        if (!m_playback.resume()) return;
        m_paused = false;
        m_pauseBtn->setText("Pause");
        updateStatusLabel();
        m_timer->start(m_tickInterval);
        return;
    }
    if (!m_playback.pause()) return;
    m_paused = true;
    m_pauseBtn->setText("Resume");
    updateStatusLabel();
    m_timer->stop();
}

void VisMaze::onSpeedChanged(int) {
    m_tickInterval = currentInterval();
    if (m_timer->isActive())
        m_timer->setInterval(m_tickInterval);
}

int VisMaze::currentInterval() const {
    int slider = m_speedSlider ? m_speedSlider->value() : 55;
    int maxMs = m_showingPath ? 180 : 120;
    int minMs = m_showingPath ? 6 : 3;
    int slow = 101 - slider;
    int range = maxMs - minMs;
    return minMs + (slow * slow * range) / 10000;
}

void VisMaze::updateStatusLabel() {
    if (m_currentAlgo == AlgoType::None) {
        m_statusLabel->setText(QString("Ready. %1 selected; press Start").arg(m_selectedAlgo == AlgoType::BFS ? "BFS" : "DFS"));
        m_statusLabel->setProperty("role", "muted");
        return;
    }
    QString algoName = (m_currentAlgo == AlgoType::BFS) ? "BFS" : "DFS";
    QString text;
    if (m_animDone) {
        text = QString("%1 - done! Path length: %2 steps").arg(algoName).arg(m_finalPath.size());
    } else if (m_paused) {
        text = QString("%1 - paused").arg(algoName);
    } else if (!m_showingPath) {
        text = QString("%1 - exploring... %2/%3").arg(algoName).arg(m_animStep).arg(m_visitOrder.size());
    } else {
        text = QString("%1 - building path... %2/%3").arg(algoName).arg(m_pathStep).arg(m_finalPath.size());
    }
    m_statusLabel->setText(text);
    m_statusLabel->setProperty("role", m_animDone ? "success" : "status");
}

void VisMaze::paintEvent(QPaintEvent*) {
    QPainter p(this); p.setRenderHint(QPainter::Antialiasing, false);
    const int C = cellSize();
    const int ox = offsetX(), oy = offsetY();
    p.fillRect(rect(), GraphUI::Canvas);
    QColor visitColor(38, 112, 232, 105);
    const int visitInset = std::min(1, (C - 1) / 2);
    const int visitSize = std::max(1, C - visitInset * 2);
    for(int r = 0; r < ROWS; r++) for(int c = 0; c < COLS; c++)
        if(m_visited[r][c]) p.fillRect(ox+c*C+visitInset, oy+r*C+visitInset, visitSize, visitSize, visitColor);
    const int pathInset = std::min(3, std::max(0, (C - 1) / 3));
    const int pathSize = std::max(1, C - pathInset * 2);
    for(auto& [r, c] : m_pathHighlight)
        p.fillRect(ox+c*C+pathInset, oy+r*C+pathInset, pathSize, pathSize, QColor(27, 139, 103, 120));
    // Draw wall strokes last so fills never wash them out.
    QPen wallPen(QColor(35, 54, 78));
    wallPen.setWidth(2);
    // Logical pixels keep walls equally legible on standard and Retina displays.
    p.setPen(wallPen);
    for(int r = 0; r < ROWS; r++) for(int c = 0; c < COLS; c++) {
        int x = ox+c*C, y = oy+r*C;
        if(m_maze[r][c].walls[0]) p.drawLine(x, y, x+C, y);
        if(m_maze[r][c].walls[1]) p.drawLine(x+C, y, x+C, y+C);
        if(m_maze[r][c].walls[2]) p.drawLine(x, y+C, x+C, y+C);
        if(m_maze[r][c].walls[3]) p.drawLine(x, y, x, y+C);
    }
    const int markerInset = std::min(4, std::max(0, (C - 1) / 3));
    const int markerSize = std::max(1, C - markerInset * 2);
    p.fillRect(ox+markerInset, oy+markerInset, markerSize, markerSize, GraphUI::Blue);
    p.fillRect(ox+(COLS-1)*C+markerInset, oy+(ROWS-1)*C+markerInset, markerSize, markerSize, GraphUI::BlueSoft);
}
