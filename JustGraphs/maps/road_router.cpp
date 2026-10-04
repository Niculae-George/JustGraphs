#include "maps/road_router.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <tuple>

namespace Luxembourg {

Route RoadRouter::solve(const Map& map, int sourceIndex, int targetIndex, Algorithm algorithm) const
{
    const auto& nodes = map.nodes();
    const auto& arcs = map.arcs();
    const auto& adjacency_ = map.adjacency();
    const auto& reverseAdjacency_ = map.reverseAdjacency();
    Route route;
    switch (algorithm) {
    case Algorithm::Dijkstra:
    case Algorithm::AStar:
    case Algorithm::BidirectionalDijkstra:
    case Algorithm::BellmanFord: break;
    default: return route;
    }
    const int count = static_cast<int>(nodes.size());
    if (sourceIndex < 0 || targetIndex < 0 || sourceIndex >= count || targetIndex >= count
        || adjacency_.size() != nodes.size()) return route;

    constexpr qint64 inf = std::numeric_limits<qint64>::max();
    const size_t source = static_cast<size_t>(sourceIndex);
    const size_t target = static_cast<size_t>(targetIndex);

    if (algorithm == Algorithm::BellmanFord) {
        std::vector<qint64> distance(nodes.size(), inf);
        std::vector<bool> reached(nodes.size(), false), queued(nodes.size(), false), reported(nodes.size(), false);
        std::vector<int> previous(nodes.size(), -1);
        std::queue<int> work;
        distance[source] = 0;
        reached[source] = true;
        queued[source] = true;
        work.push(sourceIndex);
        while (!work.empty()) {
            const int vertex = work.front();
            work.pop();
            const size_t v = static_cast<size_t>(vertex);
            queued[v] = false;
            if (!reported[v]) {
                reported[v] = true;
                route.settled.push_back(vertex);
            }
            for (const auto& [neighbor, weight] : adjacency_[v]) {
                const size_t n = static_cast<size_t>(neighbor);
                if (distance[v] > inf - weight) continue;
                const qint64 candidate = distance[v] + weight;
                if (!reached[n] || candidate < distance[n]) {
                    reached[n] = true;
                    distance[n] = candidate;
                    previous[n] = vertex;
                    if (!queued[n]) {
                        queued[n] = true;
                        work.push(neighbor);
                    }
                }
            }
        }
        if (!reached[target]) return route;
        route.found = true;
        route.cost = distance[target];
        for (int at = targetIndex; at != -1; at = previous[static_cast<size_t>(at)]) route.nodes.push_back(at);
        std::reverse(route.nodes.begin(), route.nodes.end());
        return route;
    }

    if (algorithm == Algorithm::BidirectionalDijkstra) {
        std::vector<qint64> forwardDistance(nodes.size(), inf), backwardDistance(nodes.size(), inf);
        std::vector<bool> forwardReached(nodes.size(), false), backwardReached(nodes.size(), false);
        std::vector<bool> forwardSettled(nodes.size(), false), backwardSettled(nodes.size(), false), reported(nodes.size(), false);
        std::vector<int> previous(nodes.size(), -1), next(nodes.size(), -1);
        using Entry = std::pair<qint64, int>;
        using Heap = std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>>;
        Heap forwardHeap, backwardHeap;
        forwardDistance[source] = 0; forwardReached[source] = true; forwardHeap.emplace(0, sourceIndex);
        backwardDistance[target] = 0; backwardReached[target] = true; backwardHeap.emplace(0, targetIndex);
        bool bestReached = false;
        qint64 best = 0;
        int meeting = -1;
        auto consider = [&](int vertex) {
            const size_t v = static_cast<size_t>(vertex);
            if (!forwardReached[v] || !backwardReached[v] || forwardDistance[v] > inf - backwardDistance[v]) return;
            const qint64 candidate = forwardDistance[v] + backwardDistance[v];
            if (!bestReached || candidate < best) {
                bestReached = true;
                best = candidate;
                meeting = vertex;
            }
        };
        auto clean = [](Heap& heap, const std::vector<qint64>& distance, const std::vector<bool>& settled) {
            while (!heap.empty()) {
                const auto [cost, vertex] = heap.top();
                const size_t v = static_cast<size_t>(vertex);
                if (!settled[v] && cost == distance[v]) break;
                heap.pop();
            }
        };
        while (true) {
            clean(forwardHeap, forwardDistance, forwardSettled);
            clean(backwardHeap, backwardDistance, backwardSettled);
            if (forwardHeap.empty() || backwardHeap.empty()) break;
            if (bestReached) {
                const qint64 minForward = forwardHeap.top().first;
                const qint64 minBackward = backwardHeap.top().first;
                if (minForward >= best || minBackward >= best - minForward) break;
            }
            const bool expandForward = forwardHeap.top().first <= backwardHeap.top().first;
            Heap& heap = expandForward ? forwardHeap : backwardHeap;
            auto& distances = expandForward ? forwardDistance : backwardDistance;
            auto& reached = expandForward ? forwardReached : backwardReached;
            auto& settled = expandForward ? forwardSettled : backwardSettled;
            auto& predecessors = expandForward ? previous : next;
            const auto& adjacency = expandForward ? adjacency_ : reverseAdjacency_;
            const auto [cost, vertex] = heap.top(); heap.pop();
            const size_t v = static_cast<size_t>(vertex);
            if (settled[v] || !reached[v] || cost != distances[v]) continue;
            settled[v] = true;
            if (!reported[v]) {
                reported[v] = true;
                route.settled.push_back(vertex);
            }
            consider(vertex);
            for (const auto& [neighbor, weight] : adjacency[v]) {
                const size_t n = static_cast<size_t>(neighbor);
                if (settled[n] || cost > inf - weight) continue;
                const qint64 candidate = cost + weight;
                if (!reached[n] || candidate < distances[n]) {
                    reached[n] = true;
                    distances[n] = candidate;
                    predecessors[n] = vertex;
                    heap.emplace(candidate, neighbor);
                    consider(neighbor);
                }
            }
        }
        if (!bestReached || meeting < 0) return route;
        route.found = true;
        route.cost = best;
        for (int at = meeting; at != -1; at = previous[static_cast<size_t>(at)]) route.nodes.push_back(at);
        std::reverse(route.nodes.begin(), route.nodes.end());
        for (int at = next[static_cast<size_t>(meeting)]; at != -1; at = next[static_cast<size_t>(at)]) route.nodes.push_back(at);
        return route;
    }

    if (algorithm == Algorithm::AStar) {
        long double scale = std::numeric_limits<long double>::infinity();
        for (const RoadArc& arc : arcs) {
            const QPointF a = nodes[static_cast<size_t>(arc.from)].position;
            const QPointF b = nodes[static_cast<size_t>(arc.to)].position;
            const long double dx = static_cast<long double>(a.x()) - b.x();
            const long double dy = static_cast<long double>(a.y()) - b.y();
            const long double length = std::hypot(dx, dy);
            if (length == 0.0L) {
                if (arc.length == 0) scale = 0.0L;
                continue;
            }
            scale = std::min(scale, static_cast<long double>(arc.length) / length);
        }
        if (!std::isfinite(scale) || scale < 0.0L) scale = 0.0L;
        scale *= (1.0L - 1.0e-12L);
        auto heuristic = [&](int vertex) {
            const QPointF a = nodes[static_cast<size_t>(vertex)].position;
            const QPointF b = nodes[target].position;
            const long double dx = static_cast<long double>(a.x()) - b.x();
            const long double dy = static_cast<long double>(a.y()) - b.y();
            const long double estimate = scale * std::hypot(dx, dy);
            if (estimate >= static_cast<long double>(inf)) return inf;
            return static_cast<qint64>(std::max(0.0L, std::floor(estimate)));
        };
        std::vector<qint64> distance(nodes.size(), inf);
        std::vector<bool> reached(nodes.size(), false), reported(nodes.size(), false);
        std::vector<int> previous(nodes.size(), -1);
        using Entry = std::tuple<qint64, qint64, int>;
        std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> heap;
        distance[source] = 0; reached[source] = true;
        heap.emplace(heuristic(sourceIndex), 0, sourceIndex);
        while (!heap.empty()) {
            const auto [priority, queuedCost, vertex] = heap.top(); heap.pop();
            Q_UNUSED(priority);
            const size_t v = static_cast<size_t>(vertex);
            if (!reached[v] || queuedCost != distance[v]) continue;
            // A node can be reopened if its distance improves; animation reports each dense index once.
            if (!reported[v]) { reported[v] = true; route.settled.push_back(vertex); }
            if (vertex == targetIndex) {
                route.found = true;
                route.cost = distance[v];
                for (int at = targetIndex; at != -1; at = previous[static_cast<size_t>(at)]) route.nodes.push_back(at);
                std::reverse(route.nodes.begin(), route.nodes.end());
                return route;
            }
            for (const auto& [neighbor, weight] : adjacency_[v]) {
                const size_t n = static_cast<size_t>(neighbor);
                if (distance[v] > inf - weight) continue;
                const qint64 candidate = distance[v] + weight;
                if (!reached[n] || candidate < distance[n]) {
                    reached[n] = true;
                    distance[n] = candidate;
                    previous[n] = vertex;
                    // Keep queue priorities exact even near qint64's limit.
                    const qint64 h = heuristic(neighbor);
                    heap.emplace(candidate > inf - h ? inf : candidate + h, candidate, neighbor);
                }
            }
        }
        return route;
    }

    std::vector<qint64> distance(nodes.size(), inf);
    std::vector<bool> reached(nodes.size(), false);
    std::vector<int> previous(nodes.size(), -1);
    std::vector<bool> settled(nodes.size(), false);
    using Entry = std::pair<qint64, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> heap;
    distance[source] = 0;
    reached[source] = true;
    heap.emplace(0, sourceIndex);
    while (!heap.empty()) {
        const auto [cost, vertex] = heap.top(); heap.pop();
        const size_t v = static_cast<size_t>(vertex);
        if (settled[v] || cost != distance[v]) continue;
        settled[v] = true;
        route.settled.push_back(vertex);
        if (vertex == targetIndex) {
            route.found = true;
            route.cost = cost;
            for (int at = targetIndex; at != -1; at = previous[static_cast<size_t>(at)]) route.nodes.push_back(at);
            std::reverse(route.nodes.begin(), route.nodes.end());
            return route;
        }
        for (const auto& [neighbor, weight] : adjacency_[v]) {
            const size_t n = static_cast<size_t>(neighbor);
            if (settled[n] || cost > inf - weight) continue;
            const qint64 candidate = cost + weight;
            if (!reached[n] || candidate < distance[n]) {
                reached[n] = true;
                distance[n] = candidate;
                previous[n] = vertex;
                heap.emplace(candidate, neighbor);
            }
        }
    }
    return route;
}

} // namespace Luxembourg
