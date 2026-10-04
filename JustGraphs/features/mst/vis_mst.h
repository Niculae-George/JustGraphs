#ifndef VIS_MST_H
#define VIS_MST_H
#include <QWidget>
#include <QTimer>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QElapsedTimer>
#include <QHideEvent>
#include <QCheckBox>
#include <QButtonGroup>
#include "algorithms/graph_algorithm_types.h"
#include "ui/playback_state.h"
#include "domain/undirectedgraph.h"
#include <memory>
#include <algorithm>

class VisMST : public QWidget {
    Q_OBJECT
public:
    explicit VisMST(QWidget* parent = nullptr);
    ~VisMST();
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void hideEvent(QHideEvent* event) override;
private slots:
    void onKruskalClicked();
    void onPrimClicked();
    void onBoruvkaClicked();
    void onResetClicked();
    void onAnimationTick();
    void onPauseClicked();
    void onSpeedChanged(int value);
private:
    static constexpr int COLS=20, ROWS=9, SPACING=65, NODE_R=10;
    constexpr int offsetX() const { return 60; }
    constexpr int offsetY() const { return 116; }
    int spacing() const { const int byWidth = (width() - 2 * offsetX() - 20) / (COLS - 1); const int byHeight = (height() - offsetY() - 20) / (ROWS - 1); const int value = std::min(byWidth, byHeight); return value < 28 ? 28 : value; }
    enum class AlgoType { None, Kruskal, Prim, Boruvka };
    std::unique_ptr<UndirectedGraph> m_graph;
    QTimer* m_timer;
    PlaybackState m_playback;
    QLabel* m_statusLabel;
    QLabel* m_timerLabel;
    QLabel* m_speedLabel;
    QPushButton* m_pauseBtn;
    QPushButton* m_startBtn;
    QButtonGroup* m_algoGroup;
    QSlider* m_speedSlider;
    QCheckBox* m_edgeWeightsCheck;
    QWidget* m_toolbar;
    std::vector<MSTStep> m_steps, m_accepted, m_rejected;
    MSTStep m_current;
    int m_stepIdx; bool m_animDone; bool m_hasCurrent; AlgoType m_currentAlgo;
    AlgoType m_selectedAlgo = AlgoType::Kruskal;
    bool m_paused;
    bool m_showWeights = false;
    qint64 m_elapsedMs;
    QElapsedTimer m_elapsedClock;
    void buildGraph();
    void updateLabel();
    void updateTimerLabel();
    int currentInterval() const;
    void startAnimation(AlgoType algo);
    void selectAlgorithm(AlgoType algo);
};
#endif
