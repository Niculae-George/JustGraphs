#include "features/shortest_paths/road_route_controller.h"

#include <algorithm>

namespace {
bool supported(Luxembourg::Algorithm algorithm) {
    switch (algorithm) {
    case Luxembourg::Algorithm::Dijkstra:
    case Luxembourg::Algorithm::AStar:
    case Luxembourg::Algorithm::BidirectionalDijkstra:
    case Luxembourg::Algorithm::BellmanFord: return true;
    }
    return false;
}
}

RoadRouteController::RoadRouteController(const Luxembourg::Map& map,
    const Luxembourg::IRoadRouter& router, QObject* parent)
    : QObject(parent), m_map(map), m_router(router) {
    m_timer.setInterval(16);
    connect(&m_timer, &QTimer::timeout, this, &RoadRouteController::advance);
}

void RoadRouteController::clearResult() {
    m_timer.stop();
    m_playback.cancel();
    m_route = {};
    m_attempted = false;
    m_visitsShown = 0;
}

bool RoadRouteController::setEndpoints(int source, int target) {
    const auto valid = [this](int index) {
        return index == -1 || (index >= 0 && size_t(index) < m_map.nodes().size());
    };
    if (!valid(source) || !valid(target) || (source == -1 && target != -1)) return false;
    clearResult();
    m_source = source;
    m_target = target;
    emit endpointsChanged();
    emit changed();
    return true;
}

bool RoadRouteController::setAlgorithm(Luxembourg::Algorithm algorithm) {
    if (!supported(algorithm)) return false;
    if (m_algorithm == algorithm) return true;
    clearResult();
    m_algorithm = algorithm;
    emit changed();
    return true;
}

void RoadRouteController::reset() { setEndpoints(-1, -1); }

void RoadRouteController::start() {
    if (m_source < 0 || m_target < 0) return;
    clearResult();
    m_route = m_router.solve(m_map, m_source, m_target, m_algorithm);
    m_attempted = true;
    m_playback.start();
    if (!m_route.found || m_route.settled.empty()) {
        m_playback.complete();
        emit changed();
        emit finished();
        return;
    }
    m_timer.start();
    emit changed();
}

void RoadRouteController::cancel() {
    m_timer.stop();
    m_playback.cancel();
    emit changed();
}

void RoadRouteController::pause() {
    if (m_playback.pause()) {
        m_timer.stop();
        emit changed();
    }
}

void RoadRouteController::togglePause() {
    if (m_playback.resume()) {
        m_timer.start();
        emit changed();
    } else pause();
}

void RoadRouteController::setSpeed(int value) { m_speed = std::clamp(value, 1, 100); }

void RoadRouteController::advance() {
    if (!running()) return;
    const int duration = 30000 - (m_speed - 1) * 299;
    const size_t batch = std::max<size_t>(1, (m_route.settled.size() * 16 + size_t(duration - 1)) / size_t(duration));
    m_visitsShown = int(std::min(m_route.settled.size(), size_t(m_visitsShown) + batch));
    if (size_t(m_visitsShown) == m_route.settled.size()) {
        m_timer.stop();
        m_playback.complete();
        emit finished();
    }
    emit changed();
}
