#include "domain/flownetwork.h"
#include "algorithms/flow_solver.h"
#include <QCoreApplication>
#include <QTextStream>
#include <limits>

namespace {
int failures = 0;
void check(bool value, const char* message) {
    if (!value) { QTextStream(stderr) << "FAIL: " << message << '\n'; ++failures; }
}
void addNodes(FlowNetwork& n, int count) {
    for (int i = 0; i < count; ++i) n.addNode(QPointF(50 + i * 60, 50));
}
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    FlowNetwork network;
    addNodes(network, 5);
    network.addEdge(1, 2, 5); network.addEdge(1, 3, 3);
    network.addEdge(2, 4, 5); network.addEdge(3, 4, 3);
    network.setEdgeCost(2, 4, -2);
    network.setSource(1); network.setSink(4);
    FlowSolver solver(&network);
    check(solver.runFullAlgorithm() == 8, "known network maximum flow is eight");
    check(solver.isMinCutEdge(1, 2) && solver.isMinCutEdge(1, 3), "minimum cut separates source from both paths");
    const auto iterations = solver.iterations().size();
    const auto cut = solver.minCutEdges();
    check(network.setNodePosition(2, QPointF(260, 160)), "geometry edit accepted");
    check(solver.iterations().size() == iterations && solver.minCutEdges() == cut, "moving node preserves solver results");
    check(network.setCapacity(1, 2, 2), "capacity edit accepted");
    const FlowSolver& constSolver = solver;
    check(constSolver.iterations().empty() && constSolver.minCutEdges().empty() && constSolver.totalFlow() == 0, "const reads synchronize and invalidate results after model revision");
    check(solver.runFullAlgorithm() == 5, "edited network recomputes maximum flow five");
    check(solver.minCutEdges().size() == 2, "edited minimum cut contains both source arcs");
    check(network.removeNode(3), "node removal accepted");
    check(solver.iterations().empty() && solver.minCutEdges().empty(), "node removal invalidates computed results");
    check(network.getSource() == 1 && network.getSink() == 3, "node removal remaps endpoints");
    check(solver.runFullAlgorithm() == 2, "remapped graph computes its new flow");

    check(network.removeEdge(1, 2), "flow edge removal accepted");
    check(solver.iterations().empty() && solver.minCutEdges().empty() && solver.totalFlow() == 0,
          "edge removal invalidates algorithm history and cut");
    check(solver.runFullAlgorithm() == 0, "removed capacity no longer contributes flow");

    FlowNetwork cycle;
    addNodes(cycle, 3);
    cycle.addEdge(1, 2, 4); cycle.addEdge(2, 3, 4); cycle.addEdge(3, 1, 4);
    cycle.setEdgeCost(1, 2, -3); cycle.setEdgeCost(2, 3, -3); cycle.setEdgeCost(3, 1, -3);
    FlowSolver cycleSolver(&cycle);
    check(cycleSolver.eliminateNegativeCycles() == 1, "negative residual cycle is cancelled");
    const auto& states = cycleSolver.iterations();
    check(!states.empty() && states.back().kind == FlowIterationKind::NegativeCycleCancellation && states.back().eliminatedCycles == 1,
          "negative-cycle iteration contains numeric status data");

    FlowNetwork invalid;
    addNodes(invalid, 2);
    invalid.addEdge(1, 2, 7);
    FlowSolver invalidSolver(&invalid);
    check(invalidSolver.runFullAlgorithm() == 0 && invalidSolver.iterations().empty(), "solver refuses network without valid distinct endpoints");
    invalid.setSource(1); invalid.setSink(2);
    check(invalidSolver.runFullAlgorithm() == 7, "solver works after endpoints become valid");

    FlowNetwork large;
    addNodes(large, 4);
    const int high = std::numeric_limits<int>::max();
    large.addEdge(1, 2, high); large.addEdge(1, 3, high);
    large.addEdge(2, 4, high); large.addEdge(3, 4, high);
    large.addEdge(2, 1, high); // antiparallel original capacity shares residual pair
    large.setSource(1); large.setSink(4);
    FlowSolver largeSolver(&large);
    check(largeSolver.runFullAlgorithm() == std::int64_t(high) * 2, "total flow safely exceeds int range with antiparallel capacities");
    check(largeSolver.isMinCutEdge(1, 2) && largeSolver.isMinCutEdge(1, 3), "large antiparallel network reports expected minimum cut");

    FlowNetwork invalidZero;
    addNodes(invalidZero, 2);
    invalidZero.addEdge(1, 2, 7);
    check(!invalidZero.setSource(0) && invalidZero.setSink(2), "zero source is rejected before algorithm entry");
    FlowSolver invalidZeroSolver(&invalidZero);
    check(invalidZeroSolver.runFullAlgorithm() == 0 && invalidZeroSolver.iterations().empty(), "solver safely refuses rejected zero source");
    return failures ? 1 : 0;
}
