#include "algorithms/condensed_graph_solver.h"
#include <unordered_set>
#include <string>

CondensedGraph CondensedGraphSolver::solve(const IGraphData& graph, int numComponents, const std::unordered_map<int, int>& nodeToComponent) const {
    CondensedGraph condensed;
    if (numComponents <= 0) return condensed;

    std::unordered_map<int, QPoint> componentPositions;
    std::unordered_map<int, int> nodeCount;

    for (const auto& n : graph.getNodes()) {
        int comp = nodeToComponent.at(n.getIndex());
        componentPositions[comp] += QPoint(n.getX(), n.getY());
        nodeCount[comp]++;
    }

    for (int i = 0; i < numComponents; i++) {
        if (nodeCount[i] > 0) {
            componentPositions[i] /= nodeCount[i];
            condensed.nodes.emplace_back(i + 1, componentPositions[i]);
        }
    }

    std::unordered_set<std::string> addedEdges;
    for (const auto& ed : graph.getEdges()) {
        int uComp = nodeToComponent.at(ed.getFirst().getIndex());
        int vComp = nodeToComponent.at(ed.getSecond().getIndex());
        if (uComp != vComp) {
            std::string key = std::to_string(uComp) + "->" + std::to_string(vComp);
            if (addedEdges.find(key) == addedEdges.end()) {
                if (nodeCount[uComp] && nodeCount[vComp]) {
                    condensed.edges.push_back({uComp + 1, vComp + 1});
                    addedEdges.insert(key);
                }
            }
        }
    }

    return condensed;
}
