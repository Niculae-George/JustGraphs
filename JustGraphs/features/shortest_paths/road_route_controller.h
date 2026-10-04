#pragma once

#include "maps/road_router.h"
#include "ui/playback_state.h"
#include <QObject>
#include <QTimer>

// Map and router outlive the controller. Endpoints are dense zero-based road
// indices, with -1 representing an unselected endpoint.
class RoadRouteController final : public QObject {
    Q_OBJECT
public:
    RoadRouteController(const Luxembourg::Map& map, const Luxembourg::IRoadRouter& router,
                        QObject* parent = nullptr);
    int sourceIndex() const { return m_source; }
    int targetIndex() const { return m_target; }
    Luxembourg::Algorithm selectedAlgorithm() const { return m_algorithm; }
    const Luxembourg::Route& route() const { return m_route; }
    bool attempted() const { return m_attempted; }
    bool running() const { return m_playback.phase() == PlaybackState::Phase::Running; }
    bool paused() const { return m_playback.phase() == PlaybackState::Phase::Paused; }
    int visitsShown() const { return m_visitsShown; }

    bool setEndpoints(int source, int target);
    bool setAlgorithm(Luxembourg::Algorithm algorithm);
    void reset();
    void start();
    void cancel();
    void pause();
    void togglePause();
    void setSpeed(int value);

signals:
    void endpointsChanged();
    void changed();
    void finished();

private:
    void clearResult();
    void advance();
    const Luxembourg::Map& m_map;
    const Luxembourg::IRoadRouter& m_router;
    QTimer m_timer;
    PlaybackState m_playback;
    Luxembourg::Route m_route;
    Luxembourg::Algorithm m_algorithm = Luxembourg::Algorithm::Dijkstra;
    int m_source = -1;
    int m_target = -1;
    int m_visitsShown = 0;
    int m_speed = 50;
    bool m_attempted = false;
};
