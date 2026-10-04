#pragma once

#include <QPoint>
#include <QPainterPath>
#include <QPolygonF>
#include <QLineF>
#include <QPixmap>
#include <QRectF>
#include <QTimer>
#include <QWidget>
#include <memory>
#include <vector>

#include "maps/road_router.h"
#include "features/shortest_paths/road_route_controller.h"

class QLabel;
class QPushButton;
class QSlider;

class LuxembourgMapCanvas final : public QWidget {
    Q_OBJECT
public:
    explicit LuxembourgMapCanvas(const Luxembourg::Map* map, RoadRouteController* controller, QWidget* parent = nullptr);

    int sourceIndex() const { return m_controller->sourceIndex(); }
    int targetIndex() const { return m_controller->targetIndex(); }
    qreal zoom() const { return m_fitScale > 0 ? m_scale / m_fitScale : 1.0; }
    bool routing() const { return m_controller->running(); }
    bool paused() const { return m_controller->paused(); }
    Luxembourg::Algorithm selectedAlgorithm() const { return m_controller->selectedAlgorithm(); }
    qint64 routeLength() const { return routeResult().found ? routeResult().cost : -1; }
    const Luxembourg::Route& routeResult() const { return m_controller->route(); }
    const Luxembourg::Map* map() const { return m_map; }
    QPointF screenPosition(int index) const;
    QPointF worldPositionAt(QPointF screen) const { return screenToWorld(screen); }
    bool routeAttempted() const { return m_controller->attempted(); }
    int settledShown() const { return m_controller->visitsShown(); }

    bool setEndpoints(int source, int target) { return m_controller->setEndpoints(source, target); }
    void clearRoute() { m_controller->reset(); }
    void startRoute() { m_controller->start(); }
    void fitMap();
    void zoomBy(qreal factor);
    void stopAnimation() { m_controller->cancel(); }
    void pauseAnimation() { m_controller->pause(); }
    void cancelInteraction();
    void setAlgorithm(Luxembourg::Algorithm algorithm) { m_controller->setAlgorithm(algorithm); }
    void setSpeed(int speed) { m_controller->setSpeed(speed); }
    void togglePause() { m_controller->togglePause(); }

signals:
    void endpointsChanged();
    void routeChanged();
    void routeFinished();

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void showEvent(QShowEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    QPointF screenToWorld(QPointF screen) const;
    QPointF worldToScreen(QPointF world) const;
    void zoomAt(QPointF screen, qreal factor);
    void rebuildRoadGeometry();
    const Luxembourg::Map* m_map = nullptr;
    RoadRouteController* m_controller = nullptr;
    QList<QLineF> m_roadSegments;
    QPixmap m_roadCache;
    bool m_roadCacheDirty = true;
    QPolygonF m_settledPoints;
    QPointF m_worldCenter;
    qreal m_scale = 1.0;
    qreal m_fitScale = 1.0;
    bool m_hasView = false;
    bool m_userMovedView = false;
    bool m_pendingClick = false;
    bool m_dragging = false;
    bool m_dragged = false;
    QPoint m_pressPosition;
    QPoint m_lastPosition;
};

class VisShortestPathsPage final : public QWidget {
    Q_OBJECT
public:
    explicit VisShortestPathsPage(QWidget* parent = nullptr);

    LuxembourgMapCanvas* mapCanvas() const { return m_canvas; }

protected:
    void hideEvent(QHideEvent*) override;

private:
    void updateStatus();

    std::unique_ptr<Luxembourg::Map> m_map;
    Luxembourg::RoadRouter m_router;
    std::unique_ptr<RoadRouteController> m_controller;
    LuxembourgMapCanvas* m_canvas = nullptr;
    QPushButton* m_startButton = nullptr;
    QPushButton* m_pauseButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QPushButton* m_fitButton = nullptr;
    QPushButton* m_zoomInButton = nullptr;
    QPushButton* m_zoomOutButton = nullptr;
    QSlider* m_speedSlider = nullptr;
    QPushButton* m_dijkstraButton = nullptr;
    QPushButton* m_aStarButton = nullptr;
    QPushButton* m_biDijkstraButton = nullptr;
    QPushButton* m_bellmanFordButton = nullptr;
    QPushButton* m_floydWarshallButton = nullptr;
    QLabel* m_statusLabel = nullptr;
};
