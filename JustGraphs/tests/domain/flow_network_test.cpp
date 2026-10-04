#include "domain/flownetwork.h"
#include <QCoreApplication>
#include <QTextStream>
#include <limits>

namespace {
int failures = 0;
void check(bool value, const char* message) {
    if (!value) { QTextStream(stderr) << "FAIL: " << message << '\n'; ++failures; }
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    FlowNetwork network;
    for (int i = 0; i < 5; ++i) check(network.addNode(QPointF(100 + i * 60, 100)) == i + 1, "node gets stable one-based ID");
    check(network.setSource(1) && network.setSink(4), "valid endpoints accepted");
    check(!network.setSource(0) && !network.setSource(6) && !network.setSink(0) && !network.setSink(6), "invalid endpoints rejected");
    check(!network.setSink(1) && !network.setSource(4), "equal chosen endpoints rejected");
    check(network.setSource(-1) && network.setSink(-1), "unset endpoint sentinel accepted");
    check(!network.setSource(-2) && !network.setSink(-2), "other negative IDs rejected");
    check(network.setSource(1) && network.setSink(4), "valid endpoints can be selected again");

    const auto revision = network.topologyRevision();
    check(network.setNodePosition(2, QPointF(250, 180)), "existing node can move");
    check(network.topologyRevision() == revision, "position edits do not change topology revision");
    check(!network.setNodePosition(0, QPointF()) && !network.setNodePosition(6, QPointF()), "invalid node move rejected");
    check(!network.setNodePosition(2, QPointF(std::numeric_limits<double>::infinity(), 0)), "nonfinite position rejected");
    check(network.getNodePositions()[1] == QPointF(250, 180), "move updates correct position");
    check(network.addNode(QPointF(std::numeric_limits<double>::quiet_NaN(), 0)) == -1, "nonfinite node creation rejected");

    check(network.addEdge(1, 2, 5) && network.addEdge(1, 3, 3) && network.addEdge(2, 4, 5) && network.addEdge(3, 4, 3), "valid capacities accepted");
    check(!network.addEdge(0, 2, 1) && !network.addEdge(1, 6, 1) && !network.addEdge(1, 2, 0) && !network.addEdge(1, 2, -2), "invalid capacities/endpoints rejected");
    check(network.setEdgeCost(2, 4, -2), "valid edge cost accepted");
    check(!network.setEdgeCost(0, 2, 1) && !network.setEdgeCost(1, 6, 1), "invalid cost endpoints rejected");
    check(!network.setEdgeCost(1, 2, std::numeric_limits<int>::min()), "INT_MIN cost rejected to prevent reverse negation overflow");
    check(!network.addEdge(2, 2, 1) && !network.setCapacity(2, 2, 2), "self-loop capacities rejected");
    check(!network.setEdgeCost(2, 2, 3), "self-loop costs rejected");

    check(network.removeNode(3) && network.getNumNodes() == 4, "interior node can be deleted");
    check(network.getSource() == 1 && network.getSink() == 3, "endpoint indices remap after interior deletion");
    check(network.getCapacity(1, 2) == 5 && network.getCapacity(2, 3) == 5 && network.getEdgeCost(2, 3) == -2, "remaining edges/costs follow remapped nodes");
    check(network.removeNode(1) && network.getSource() == -1 && network.getSink() == 2, "deleting source clears it and remaps sink");
    check(network.removeNode(2) && network.getSink() == -1, "deleting sink clears it");
    check(!network.removeNode(0) && !network.removeNode(6), "invalid deletion rejected");

    network.clear();
    check(network.getNumNodes() == 0 && network.getSource() == -1 && network.getSink() == -1, "clear removes network data");
    check(network.addNode(QPointF(10, 10)) == 1 && network.addNode(QPointF(30, 10)) == 2, "network can be rebuilt after clear");
    network.addEdge(1, 2, 8); network.addEdge(2, 1, 3); network.setEdgeCost(1, 2, 7);
    const auto beforeRemoval = network.topologyRevision();
    check(network.removeEdge(1, 2) && network.topologyRevision() == beforeRemoval + 1, "edge removal invalidates topology once");
    check(network.getCapacity(1, 2) == 0 && network.getCapacity(2, 1) == 3, "only selected capacity direction removed");
    check(network.getEdgeCost(1, 2) == 7 && network.getEdgeCost(2, 1) == -7, "paired residual costs survive with reverse edge");
    check(!network.removeEdge(1, 2) && !network.removeEdge(0, 2) && network.topologyRevision() == beforeRemoval + 1, "missing/invalid removal leaves revision unchanged");
    check(network.removeEdge(2, 1) && network.getEdgeCost(1, 2) == 0 && network.getEdgeCost(2, 1) == 0, "removing last direction clears paired costs");
    return failures ? 1 : 0;
}
