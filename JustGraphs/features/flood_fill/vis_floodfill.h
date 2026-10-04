#ifndef VIS_FLOODFILL_H
#define VIS_FLOODFILL_H

#include <QWidget>
#include <QTimer>
#include <QEvent>
#include <QHideEvent>
#include <QShowEvent>
#include "ui/playback_state.h"
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <vector>
#include <memory>

class VisFloodFill : public QWidget {
    Q_OBJECT
public:
    explicit VisFloodFill(QWidget* parent = nullptr);
    ~VisFloodFill();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onGenerateWallsClicked();
    void onResetClicked();
    void onAnimationTick();
    void onSpeedChanged(int value);

private:
    static constexpr int ROWS = 25;
    static constexpr int COLS = 40;
    static constexpr int CELL_SIZE = 22;

    void setupUi();
    void clearVisualization();
    void resetGrid();
    void startFloodFill(int r, int c);

    struct Cell {
        int type = 0; // 0: empty, 1: wall, 2: filled
    };

    std::vector<std::vector<Cell>> m_grid;
    std::vector<std::pair<int, int>> m_fillOrder;
    size_t m_animStep = 0;
    bool m_animating = false;

    QTimer* m_timer;
    PlaybackState m_playback;
    int m_tickInterval = 10;

    QWidget* m_toolbar;
    QLabel* m_statusLabel;
    QSlider* m_speedSlider;

    int cellSize() const;
    int offsetX() const;
    int offsetY() const;
};

#endif // VIS_FLOODFILL_H
