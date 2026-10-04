#include "features/shortest_paths/road_route_controller.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QTextStream>
#include <type_traits>

namespace {
int failures = 0;
void check(bool condition, const char* message) {
    if (!condition) { QTextStream(stderr) << "FAIL: " << message << '\n'; ++failures; }
}

template<class T>
concept MutableRoadNodes = requires(T& map) { map.nodes().push_back(Luxembourg::RoadNode{}); };
template<class T>
concept MutableRoadArcs = requires(T& map) { map.arcs().push_back(Luxembourg::RoadArc{}); };
static_assert(!MutableRoadNodes<Luxembourg::Map>);
static_assert(!MutableRoadArcs<Luxembourg::Map>);
static_assert(std::is_same_v<decltype(std::declval<const Luxembourg::Map&>().nodes()), const std::vector<Luxembourg::RoadNode>&>);

Luxembourg::Map makeMap() {
    QByteArray xml = "<map><nodes>"
        "<node id='10' longitude='6000000' latitude='4900000'/>"
        "<node id='20' longitude='6000100' latitude='4900100'/>"
        "<node id='30' longitude='6000200' latitude='4900200'/>"
        "</nodes><arcs><arc from='10' to='20' length='7'/><arc from='20' to='30' length='8'/>"
        "</arcs></map>";
    QBuffer buffer(&xml);
    QString error;
    if (!buffer.open(QIODevice::ReadOnly)) return {};
    Luxembourg::Map map;
    if (!map.load(buffer, error)) {
        QTextStream(stderr) << "Fixture load failed: " << error << '\n';
        ++failures;
    }
    return map;
}

class SpyRouter final : public Luxembourg::IRoadRouter {
public:
    mutable int calls = 0;
    mutable int source = -9;
    mutable int target = -9;
    mutable Luxembourg::Algorithm algorithm = Luxembourg::Algorithm::Dijkstra;
    Luxembourg::Route solve(const Luxembourg::Map&, int from, int to,
                            Luxembourg::Algorithm selected) const override {
        ++calls; source = from; target = to; algorithm = selected;
        return {true, 15, {from, to}, {0, 1, 2}};
    }
};

void waitMs(int ms) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

bool awaitFinished(RoadRouteController& controller, int timeoutMs = 1200) {
    if (!controller.running()) return !controller.paused();
    QEventLoop loop;
    QObject::connect(&controller, &RoadRouteController::finished, &loop, &QEventLoop::quit);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
    return !controller.running() && !controller.paused();
}

bool awaitFirstVisit(RoadRouteController& controller, int timeoutMs = 500) {
    if (controller.visitsShown() > 0) return true;
    QEventLoop loop;
    QObject::connect(&controller, &RoadRouteController::changed, &loop, [&] {
        if (controller.visitsShown() > 0) loop.quit();
    });
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
    return controller.visitsShown() > 0;
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const Luxembourg::Map map = makeMap();
    SpyRouter router;
    RoadRouteController controller(map, router);

    check(map.nodes().size() == 3, "loaded fixture has three nodes");
    check(!controller.setEndpoints(-1, 1), "target cannot be selected before source");
    check(!controller.setEndpoints(0, 3), "endpoint at dense node count is rejected");
    check(!controller.setEndpoints(10, 20), "sparse external IDs are rejected as endpoint indices");
    controller.start();
    check(router.calls == 0 && !controller.attempted(), "start with unset endpoints does nothing");

    check(controller.setEndpoints(0, 2), "valid dense endpoints accepted");
    controller.setSpeed(0);
    controller.start();
    check(controller.running() && controller.attempted(), "route computation starts playback");
    check(router.calls == 1 && router.source == 0 && router.target == 2
          && router.algorithm == Luxembourg::Algorithm::Dijkstra,
          "controller injects router and forwards selected algorithm and endpoints");
    check(controller.route().found && controller.route().cost == 15, "router result retained");
    check(controller.visitsShown() == 0, "playback begins before first visit batch");
    check(controller.setAlgorithm(Luxembourg::Algorithm::Dijkstra), "selecting same algorithm succeeds");
    check(controller.running() && controller.route().found && router.calls == 1,
          "same algorithm selection leaves active playback and result untouched");
    check(!controller.setAlgorithm(static_cast<Luxembourg::Algorithm>(99)), "unknown algorithm is rejected");

    check(awaitFirstVisit(controller), "playback reveals visit after timer tick");
    controller.pause();
    const int pausedVisits = controller.visitsShown();
    check(controller.paused() && pausedVisits > 0, "pause retains progressed visits");
    waitMs(55);
    check(controller.visitsShown() == pausedVisits, "paused playback does not advance visits");
    check(controller.sourceIndex() == 0 && controller.targetIndex() == 2
          && controller.route().found && controller.attempted(),
          "pause preserves endpoints and route result");
    controller.setSpeed(100);
    controller.togglePause();
    check(controller.running(), "resume restarts playback");
    check(awaitFinished(controller), "speed 100 completes playback within bounded time");
    check(controller.visitsShown() == 3, "completed playback reveals every settled visit");

    check(controller.setEndpoints(1, 2), "new valid endpoints accepted");
    controller.start();
    check(controller.running(), "second playback starts");
    controller.pause(); // equivalent lifecycle action used when page is hidden
    check(controller.paused() && controller.sourceIndex() == 1 && controller.targetIndex() == 2
          && controller.route().found && controller.attempted(),
          "hide-equivalent pause preserves endpoints and computed result");
    check(controller.setAlgorithm(Luxembourg::Algorithm::AStar), "algorithm can change while paused");
    check(controller.sourceIndex() == 1 && controller.targetIndex() == 2,
          "algorithm change retains endpoints");
    check(!controller.attempted() && !controller.route().found && controller.visitsShown() == 0
          && !controller.running() && !controller.paused(),
          "algorithm change invalidates old playback result");
    check(router.calls == 2, "changing algorithm does not compute until started");
    controller.reset();
    check(controller.sourceIndex() == -1 && controller.targetIndex() == -1
          && !controller.attempted() && !controller.route().found && controller.visitsShown() == 0,
          "reset clears endpoints, result and playback state");
    return failures ? 1 : 0;
}
