#include "maps/luxembourg_map.h"
#include "maps/road_router.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QFile>
#include <QElapsedTimer>
#include <random>
#include <QTextStream>

#include <limits>
#include <set>

namespace {

int failures = 0;
Luxembourg::Algorithm selectedAlgorithm = Luxembourg::Algorithm::Dijkstra;

void check(bool condition, const char* description)
{
    if (condition) return;
    QTextStream(stderr) << "FAIL: " << description << '\n';
    ++failures;
}

Luxembourg::Map loadText(const QByteArray& xml)
{
    Luxembourg::Map map;
    QBuffer input;
    input.setData(xml);
    QString localError;
    check(input.open(QIODevice::ReadOnly), "open in-memory XML fixture");
    check(map.load(input, localError), "load valid in-memory XML fixture");
    return map;
}

QByteArray makeMap(const QByteArray& nodes, const QByteArray& arcs)
{
    return "<map><nodes>" + nodes + "</nodes><arcs>" + arcs + "</arcs></map>";
}

void testSmallDirectedWeightedMap()
{
    const QByteArray xml = makeMap(
        "<node id='10' longitude='6000000' latitude='4900000'/>"
        "<node id='20' longitude='6000100' latitude='4900100'/>"
        "<node id='30' longitude='6000200' latitude='4900200'/>"
        "<node id='40' longitude='6000300' latitude='4900300'/>"
        "<node id='90' longitude='6000400' latitude='4900400'/>",
        "<arc from='10' to='40' length='100'/>"
        "<arc from='10' to='20' length='0'/>"
        "<arc from='20' to='30' length='4'/>"
        "<arc from='30' to='40' length='5'/>"
        "<arc from='20' to='40' length='50'/>");
    const Luxembourg::Map map = loadText(xml);

    check(map.nodes().size() == 5, "small map keeps all nodes");
    check(map.arcs().size() == 5, "small map keeps directed arcs");
    check(map.nodes().size() > 0 && map.nodes()[0].id == 10 && map.nodes()[1].id == 20,
          "sparse external IDs map to dense input-order indices");
    check(map.bounds().contains(map.nodes().front().position) && map.bounds().contains(map.nodes().back().position),
          "bounds contain projected nodes");
    check(map.bounds().top() <= map.bounds().bottom() && map.bounds().left() <= map.bounds().right(),
          "bounds are normalized for north-up rendering");

    const Luxembourg::Route route = Luxembourg::RoadRouter{}.solve(map, 0, 3, selectedAlgorithm);
    check(route.found && route.cost == 9, "algorithm chooses cheaper multi-edge route over direct edge");
    check(route.nodes == std::vector<int>({0, 1, 2, 3}), "route contains dense node indices in source-to-target order");
    check(!Luxembourg::RoadRouter{}.solve(map, 3, 0, selectedAlgorithm).found, "directed arcs do not imply reverse reachability");
    const Luxembourg::Route isolated = Luxembourg::RoadRouter{}.solve(map, 4, 4, selectedAlgorithm);
    check(isolated.found && isolated.cost == 0 && isolated.nodes == std::vector<int>({4}),
          "isolated-node self route has zero cost");
    check(map.nearestNode(map.nodes()[2].position) == 2, "nearest node returns dense index");
    check(map.nearestNode(QPointF(1.0e9, 1.0e9)) >= 0, "nearest node works outside map bounds");
    check(!Luxembourg::RoadRouter{}.solve(map, -1, 0, selectedAlgorithm).found && !Luxembourg::RoadRouter{}.solve(map, 0, 5, selectedAlgorithm).found,
          "out-of-range route endpoints are rejected");
    check(!Luxembourg::RoadRouter{}.solve(map, 0, 3, static_cast<Luxembourg::Algorithm>(99)).found,
          "unsupported algorithm cannot silently select another router");
}

void testLargeCosts()
{
    const Luxembourg::Map map = loadText(makeMap(
        "<node id='1' longitude='1' latitude='1'/><node id='2' longitude='2' latitude='2'/>"
        "<node id='3' longitude='3' latitude='3'/>",
        "<arc from='1' to='2' length='1500000000'/><arc from='2' to='3' length='1500000000'/>"));
    const Luxembourg::Route route = Luxembourg::RoadRouter{}.solve(map, 0, 2, selectedAlgorithm);
    check(route.found && route.cost == 3000000000LL,
          "route costs above 32-bit range remain exact");

    const Luxembourg::Map maximum = loadText(makeMap(
        "<node id='1' longitude='1' latitude='1'/><node id='2' longitude='2' latitude='2'/>",
        "<arc from='1' to='2' length='9223372036854775807'/>"));
    const auto maximumRoute = Luxembourg::RoadRouter{}.solve(maximum, 0, 1, selectedAlgorithm);
    check(maximumRoute.found && maximumRoute.cost == std::numeric_limits<qint64>::max(),
          "maximum representable cost is distinct from unreachable");

    const Luxembourg::Map overflow = loadText(makeMap(
        "<node id='1' longitude='1' latitude='1'/><node id='2' longitude='2' latitude='2'/>"
        "<node id='3' longitude='3' latitude='3'/>",
        "<arc from='1' to='2' length='9223372036854775707'/>"
        "<arc from='2' to='3' length='200'/>"
        "<arc from='1' to='3' length='9223372036854775757'/>"));
    const auto safeRoute = Luxembourg::RoadRouter{}.solve(overflow, 0, 2, selectedAlgorithm);
    check(safeRoute.found && safeRoute.cost == 9223372036854775757LL &&
          safeRoute.nodes == std::vector<int>({0, 2}),
          "overflowing alternative cannot replace a representable shortest path");
}

void testFailedLoadsClearState()
{
    const QByteArray valid = makeMap("<node id='1' longitude='1' latitude='2'/>", "");
    const std::vector<QByteArray> invalidMaps = {
        makeMap("<node id='1' longitude='1' latitude='2'/><node id='1' longitude='2' latitude='3'/>", ""),
        makeMap("<node id='1' longitude='1' latitude='2'/>", "<arc from='1' to='77' length='1'/>") ,
        makeMap("<node id='1' longitude='1'/>", ""),
        makeMap("<node id='1' longitude='1' latitude='2'/>", "<arc from='1' to='1' length='-1'/>") ,
        makeMap("<node id='1' longitude='1' latitude='2'/>", "<arc from='1' to='1'/>") ,
        QByteArray("<map><nodes><node id='1' longitude='1' latitude='2'/></nodes><arcs><arc from='1' to='1' length='1'></arcs></map>")
    };
    const char* labels[] = {"duplicate IDs are rejected", "missing arc endpoint rejected", "missing node attribute rejected",
                            "negative arc cost rejected", "missing arc attribute rejected", "malformed XML rejected"};

    for (size_t i = 0; i < invalidMaps.size(); ++i) {
        Luxembourg::Map map = loadText(valid);
        QBuffer input;
        input.setData(invalidMaps[i]);
        input.open(QIODevice::ReadOnly);
        QString error;
        const bool loaded = map.load(input, error);
        check(!loaded, labels[i]);
        check(!error.isEmpty(), "failed load reports an error");
        check(map.nodes().empty() && map.arcs().empty() && map.bounds().isNull(),
              "failed load clears previously loaded graph state");
        check(!Luxembourg::RoadRouter{}.solve(map, 0, 0).found && map.nearestNode(QPointF()) == -1,
              "failed load cannot expose stale routing or nearest-node state");
    }
}

bool hasArc(const Luxembourg::Map& map, int fromIndex, int toIndex, qint64* length)
{
    if (fromIndex < 0 || toIndex < 0 || static_cast<size_t>(fromIndex) >= map.nodes().size()
        || static_cast<size_t>(toIndex) >= map.nodes().size()) return false;
    for (const auto& arc : map.arcs()) {
        if (arc.from == fromIndex && arc.to == toIndex) {
            if (length) *length = arc.length;
            return true;
        }
    }
    return false;
}

void testRandomDirectedMaps()
{
    constexpr int count = 8;
    constexpr qint64 infinity = 1000000000;
    std::mt19937 random(20261004);
    for (int trial = 0; trial < 12; ++trial) {
        QByteArray nodes, arcs;
        std::vector<std::vector<qint64>> reference(count, std::vector<qint64>(count, infinity));
        for (int i = 0; i < count; ++i) {
            nodes += QString("<node id='%1' longitude='%2' latitude='%3'/>")
                .arg(1000 + 7*i).arg(4960000 + i*100).arg(610000 + i*i*37).toUtf8();
            reference[i][i] = 0;
        }
        for (int i = 0; i < count; ++i) for (int j = 0; j < count; ++j) {
            if (i == j || random() % 100 >= 35) continue;
            const int weight = int(random() % 81);
            reference[i][j] = weight;
            arcs += QString("<arc from='%1' to='%2' length='%3'/>")
                .arg(1000+7*i).arg(1000+7*j).arg(weight).toUtf8();
        }
        // Independent all-pairs oracle is practical for this eight-node fixture.
        for (int k = 0; k < count; ++k) for (int i = 0; i < count; ++i) for (int j = 0; j < count; ++j)
            reference[i][j] = std::min(reference[i][j], reference[i][k] + reference[k][j]);
        const auto map = loadText(makeMap(nodes, arcs));
        for (int source = 0; source < count; ++source) for (int target = 0; target < count; ++target) {
            const auto route = Luxembourg::RoadRouter{}.solve(map, source, target, selectedAlgorithm);
            check(route.found == (reference[source][target] < infinity), "directed randomized reachability agrees with independent oracle");
            if (!route.found) continue;
            check(route.cost == reference[source][target], "all algorithms return independently verified shortest cost");
            bool valid = !route.nodes.empty() && route.nodes.front() == source && route.nodes.back() == target;
            qint64 sum = 0;
            for (size_t i = 1; i < route.nodes.size(); ++i) {
                qint64 length = 0;
                if (!hasArc(map, route.nodes[i-1], route.nodes[i], &length)) valid = false;
                sum += length;
            }
            check(valid && sum == route.cost, "randomized route follows original directed arcs with exact cost");
            std::set<int> seen(route.settled.begin(), route.settled.end());
            check(seen.size() == route.settled.size() && seen.size() <= map.nodes().size(), "animation visit order contains valid unique nodes");
        }
    }
}

void testFullDataset(const QString& path)
{
    QFile file(path);
    check(file.open(QIODevice::ReadOnly), "open full Luxembourg XML dataset");
    if (!file.isOpen()) return;
    Luxembourg::Map map;
    QString error;
    check(map.load(file, error), "load full Luxembourg XML dataset");
    if (map.nodes().empty()) {
        QTextStream(stderr) << "Dataset load error: " << error << '\n';
        return;
    }
    check(map.nodes().size() == 42314, "full dataset node count matches fixture contract");
    check(map.arcs().size() == 100358, "full dataset arc count matches fixture contract");
    const Luxembourg::Route route = Luxembourg::RoadRouter{}.solve(map, 0, 42313, selectedAlgorithm);
    check(route.found, "full dataset has route from first to last node");
    if (!route.found) return;

    qint64 sum = 0;
    bool pathValid = !route.nodes.empty() && route.nodes.front() == 0 && route.nodes.back() == 42313;
    for (size_t i = 1; i < route.nodes.size(); ++i) {
        qint64 length = 0;
        if (!hasArc(map, route.nodes[i - 1], route.nodes[i], &length)
            || sum > std::numeric_limits<qint64>::max() - length) {
            pathValid = false;
            break;
        }
        sum += length;
    }
    check(pathValid, "full dataset route uses existing directed arcs");
    check(pathValid && sum == route.cost, "full dataset route cost equals exact arc sum");
    check(route.settled.size() <= map.nodes().size(), "settled list does not exceed node count");
    std::set<int> settled(route.settled.begin(), route.settled.end());
    check(settled.size() == route.settled.size(), "each full-map node settles at most once");
    check(route.cost == 927, "full dataset 0-to-42313 route matches independent reference cost");
    const auto middleRoute = Luxembourg::RoadRouter{}.solve(map, 0, 10000, selectedAlgorithm);
    check(middleRoute.found && middleRoute.cost == 14733, "full dataset 0-to-10000 route matches independent reference cost");
    const auto longRoute = Luxembourg::RoadRouter{}.solve(map, 1, 30000, selectedAlgorithm);
    check(longRoute.found && longRoute.cost == 28814, "full dataset 1-to-30000 route matches independent reference cost");
    const Luxembourg::Route reverse = Luxembourg::RoadRouter{}.solve(map, 42313, 0, selectedAlgorithm);
    check(reverse.found && reverse.cost == 927, "full dataset reverse route matches independent reference cost");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    testFailedLoadsClearState();
    for (auto algorithm : {Luxembourg::Algorithm::Dijkstra, Luxembourg::Algorithm::AStar,
                           Luxembourg::Algorithm::BidirectionalDijkstra, Luxembourg::Algorithm::BellmanFord}) {
        selectedAlgorithm = algorithm;
        testSmallDirectedWeightedMap();
        testLargeCosts();
        testRandomDirectedMaps();
        QElapsedTimer elapsed; elapsed.start();
        if (argc > 1) testFullDataset(QString::fromLocal8Bit(argv[1]));
        else check(false, "dataset path argument is required");
        QTextStream(stdout) << "Algorithm " << int(algorithm) << " full-map checks: " << elapsed.elapsed() << " ms\n";
    }
    if (failures != 0) QTextStream(stderr) << failures << " regression check(s) failed\n";
    return failures == 0 ? 0 : 1;
}
