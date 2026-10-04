#ifndef VIS_MAZE_H
#define VIS_MAZE_H
#include <QWidget>
#include <QTimer>
#include "ui/playback_state.h"
#include <QEvent>
#include <QHideEvent>
#include <QPushButton>
#include <QButtonGroup>
#include <QLabel>
#include <QSlider>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "domain/directedgraph.h"
#include "algorithms/i_traversal.h"
#include <memory>

struct Cell { bool walls[4] = {true,true,true,true}; };

class VisMaze : public QWidget
{
    Q_OBJECT
public:
    explicit VisMaze(QWidget* parent = nullptr);
    ~VisMaze();
protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void hideEvent(QHideEvent* event) override;
private slots:
    void onBfsClicked();
    void onDfsClicked();
    void onResetClicked();
    void onAnimationTick();
    void onPauseClicked();
    void onSpeedChanged(int value);
private:
    static constexpr int COLS = 35, ROWS = 25;
    std::vector<std::vector<Cell>> m_maze;
    std::unique_ptr<DirectedGraph> m_graph;
    QTimer* m_timer;
    PlaybackState m_playback;
    std::vector<std::pair<int,int>> m_visitOrder;
    std::vector<std::pair<int,int>> m_finalPath;
    int m_animStep; bool m_animDone; bool m_showingPath; int m_pathStep;
    enum class AlgoType { None, BFS, DFS };
    AlgoType m_currentAlgo;
    AlgoType m_selectedAlgo;
    std::vector<std::vector<bool>> m_visited;
    std::vector<std::pair<int,int>> m_pathHighlight;
    QPushButton* m_pauseBtn;
    QPushButton* m_bfsBtn;
    QPushButton* m_dfsBtn;
    QPushButton* m_startBtn;
    QButtonGroup* m_algorithmGroup;
    QLabel* m_statusLabel;
    QLabel* m_hintLabel;
    QLabel* m_speedLabel;
    QSlider* m_speedSlider;
    QWidget* m_toolbar;
    bool m_paused;
    int m_tickInterval;
    int currentInterval() const;
    void updateStatusLabel();
    void generateMaze();
    std::vector<std::pair<int,int>> runTraversal(const ITraversalAlgorithm& algorithm,
                                                  std::vector<std::pair<int,int>>& path);
    void startAnimation(AlgoType algo);
    static constexpr int canvasTop() { return 116; }
    static constexpr int canvasBottomMargin() { return 16; }
    int cellSize() const { return std::max(1, std::min((width() - 24) / COLS, (height() - canvasTop() - canvasBottomMargin()) / ROWS)); }
    int offsetX() const { return (width() - COLS * cellSize()) / 2; }
    int offsetY() const { return canvasTop() + std::max(0, (height() - canvasTop() - canvasBottomMargin() - ROWS * cellSize()) / 2); }
    void clearVisualization();
};
#endif
