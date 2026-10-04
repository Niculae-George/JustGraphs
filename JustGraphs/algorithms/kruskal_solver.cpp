#include "algorithms/kruskal_solver.h"
#include "algorithms/union_find.h"
#include "domain/edge.h"
#include <algorithm>

std::vector<MSTStep> KruskalSolver::solve(const IGraphData& graph) const
{
    std::vector<MSTStep> steps;
    if (graph.getNodes().empty()) return steps;

    std::vector<const Edge*> sorted;
    for (const auto& ed : graph.getEdges()) sorted.push_back(&ed);
    std::sort(sorted.begin(), sorted.end(),
              [](const Edge* a, const Edge* b) { return a->getCost() < b->getCost(); });

    UnionFind uf;
    std::vector<int> ids;
    for (const auto& n : graph.getNodes()) ids.push_back(n.getIndex());
    uf.init(ids);

    for (const Edge* ed : sorted) {
        int u = ed->getFirst().getIndex(), v = ed->getSecond().getIndex();
        bool ok = uf.unite(u, v);
        steps.push_back({u, v, ed->getCost(), ok});
    }
    return steps;
}
