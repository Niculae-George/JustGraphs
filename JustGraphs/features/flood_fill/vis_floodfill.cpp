#include "features/flood_fill/vis_floodfill.h"
#include "algorithms/flood_fill_solver.h"
#include "algorithms/flood_wall_generator.h"
#include "ui/theme.h"
#include <QPainter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QRandomGenerator>

VisFloodFill::VisFloodFill(QWidget* parent)
    : QWidget(parent), m_timer(new QTimer(this)) {
    m_timer->setObjectName("floodFillTimer");
    m_grid.assign(ROWS, std::vector<Cell>(COLS));
    setupUi();
    resetGrid();

    connect(m_timer, &QTimer::timeout, this, &VisFloodFill::onAnimationTick);
}

VisFloodFill::~VisFloodFill() = default;

void VisFloodFill::setupUi() {
    m_toolbar = GraphUI::toolbar(this);
    delete m_toolbar->layout(); // Replace helper's one-row layout with action + status rows.
    m_toolbar->setGeometry(0, 0, width(), 104);
    

    auto* btnGenerate = GraphUI::button("Generate walls", m_toolbar, true);
    btnGenerate->setObjectName("floodGenerateWalls");
    auto* btnReset = GraphUI::button("Clear grid", m_toolbar);
    btnReset->setObjectName("floodClearGrid");
    m_statusLabel = GraphUI::status("Left click: toggle wall  ·  Right click: flood fill", m_toolbar);
    m_statusLabel->setObjectName("floodStatus");
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setStyleSheet(QString("background: %1; border: 1px solid %2; border-radius: 8px; padding: 6px 10px; color: %3;")
        .arg(GraphUI::Surface.name(), GraphUI::Border.name(), GraphUI::Muted.name()));

    m_speedSlider = new QSlider(Qt::Horizontal, m_toolbar);
    m_speedSlider->setRange(1, 100);
    m_speedSlider->setValue(85);
    m_speedSlider->setFixedWidth(150);
    auto* speedLabel = new QLabel("Speed", m_toolbar);
    speedLabel->setStyleSheet(QString("color: %1;").arg(GraphUI::Muted.name()));

    auto* outer = new QVBoxLayout(m_toolbar);
    outer->setContentsMargins(24, 10, 24, 10);
    outer->setSpacing(8);
    auto* actions = new QHBoxLayout;
    actions->setSpacing(12);
    actions->addWidget(btnGenerate);
    actions->addWidget(btnReset);
    actions->addSpacing(8);
    actions->addWidget(speedLabel);
    actions->addWidget(m_speedSlider);
    actions->addStretch();
    outer->addLayout(actions);
    outer->addWidget(m_statusLabel);

    connect(btnGenerate, &QPushButton::clicked, this, &VisFloodFill::onGenerateWallsClicked);
    connect(btnReset, &QPushButton::clicked, this, &VisFloodFill::onResetClicked);
    connect(m_speedSlider, &QSlider::valueChanged, this, &VisFloodFill::onSpeedChanged);
}

void VisFloodFill::clearVisualization() {
    m_timer->stop();
    m_playback.cancel();
    m_animating = false;
    m_animStep = 0;
    m_fillOrder.clear();
}

void VisFloodFill::resetGrid() {
    clearVisualization();
    for (auto& row : m_grid) {
        for (auto& cell : row) {
            cell.type = 0;
        }
    }
    m_statusLabel->setText("Click to place walls, Right-Click to start Flood Fill");
    update();
}

void VisFloodFill::onGenerateWallsClicked() {
    clearVisualization();
    const auto walls = FloodWallGenerator::generate(ROWS, COLS, QRandomGenerator::global()->generate());
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c)
            m_grid[r][c].type = walls[r][c];
    m_statusLabel->setText("Walls generated · Left click: toggle wall · Right click: flood fill");
    update();
}

void VisFloodFill::onResetClicked() {
    resetGrid();
}

void VisFloodFill::onSpeedChanged(int) {
    int val = m_speedSlider->value();
    m_tickInterval = 1 + (100 - val) * 2;
    if (m_timer->isActive()) {
        m_timer->setInterval(m_tickInterval);
    }
}

void VisFloodFill::mousePressEvent(QMouseEvent* event) {
    if (m_animating) return;

    if (event->pos().x() < offsetX() || event->pos().y() < offsetY() ||
        event->pos().x() >= offsetX() + COLS * cellSize() ||
        event->pos().y() >= offsetY() + ROWS * cellSize()) return;

    int r = (event->pos().y() - offsetY()) / cellSize();
    int c = (event->pos().x() - offsetX()) / cellSize();

    if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
        if (event->button() == Qt::LeftButton) {
            m_grid[r][c].type = (m_grid[r][c].type == 1) ? 0 : 1;
            update();
        } else if (event->button() == Qt::RightButton) {
            if (m_grid[r][c].type != 1) {
                startFloodFill(r, c);
            }
        }
    }
}

void VisFloodFill::startFloodFill(int startR, int startC) {
    m_animating = true;
    m_playback.start();
    m_animStep = 0;

    std::vector<std::vector<int>> gridData(ROWS, std::vector<int>(COLS));
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            gridData[r][c] = m_grid[r][c].type;
            if (m_grid[r][c].type == 2) {
                m_grid[r][c].type = 0;
                gridData[r][c] = 0;
            }
        }
    }

    auto fillPoints = FloodFillSolver::solve(gridData, startR, startC, 0);
    m_fillOrder.clear();
    for (const auto& p : fillPoints) {
        m_fillOrder.push_back({p.r, p.c});
    }

    m_statusLabel->setText("Filling area...");
    onSpeedChanged(0);
    m_timer->start(m_tickInterval);
}

void VisFloodFill::onAnimationTick() {
    if (m_animStep < m_fillOrder.size()) {
        auto [r, c] = m_fillOrder[m_animStep++];
        m_grid[r][c].type = 2;
        update();
    } else {
        m_timer->stop();
        m_animating = false;
        m_playback.complete();
        m_statusLabel->setText("Flood Fill complete!");
    }
}

void VisFloodFill::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), GraphUI::Canvas);

    int ox = offsetX();
    int oy = offsetY();

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            QRect cellRect(ox + c * cellSize(), oy + r * cellSize(), cellSize(), cellSize());
            
            if (m_grid[r][c].type == 1) {
                p.fillRect(cellRect.adjusted(1,1,-1,-1), GraphUI::Text);
            } else if (m_grid[r][c].type == 2) {
                p.fillRect(cellRect.adjusted(1,1,-1,-1), GraphUI::Blue);
            }
            p.setPen(GraphUI::Border);
            p.drawRect(cellRect);
        }
    }
}

int VisFloodFill::cellSize() const {
    return qMax(4, qMin(CELL_SIZE, qMin((width() - 24) / COLS, (height() - 128) / ROWS)));
}
int VisFloodFill::offsetX() const { return (width() - COLS * cellSize()) / 2; }
int VisFloodFill::offsetY() const { return 116; }

void VisFloodFill::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_toolbar->setGeometry(0, 0, width(), 104);
}

void VisFloodFill::hideEvent(QHideEvent* event) {
    if (m_timer->isActive()) {
        m_timer->stop();
        m_playback.pause();
    }
    QWidget::hideEvent(event);
}

void VisFloodFill::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (m_playback.resume()) m_timer->start(m_tickInterval);
}
