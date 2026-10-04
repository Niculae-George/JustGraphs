#include "algorithms/boruvka_solver.h"
#include "algorithms/union_find.h"
#include "domain/edge.h"
#include <unordered_map>
#include <unordered_set>

std::vector<MSTStep> BoruvkaSolver::solve(const IGraphData& graph) const
{
    std::vector<MSTStep> steps;
    if (graph.getNodes().empty()) return steps;

    std::vector<int> ids;
    for (const auto& n : graph.getNodes()) ids.push_back(n.getIndex());

    UnionFind uf;
    uf.init(ids);

    int n = static_cast<int>(graph.getNodes().size()), mstEdges = 0;
    while (mstEdges < n - 1) {
        std::unordered_map<int, const Edge*> cheapest;
        for (const auto& ed : graph.getEdges()) {
            int u = ed.getFirst().getIndex(), v = ed.getSecond().getIndex();
            int ru = uf.find(u), rv = uf.find(v);
            if (ru == rv) continue;
            if (!cheapest.count(ru) || ed.getCost() < cheapest[ru]->getCost()) cheapest[ru] = &ed;
            if (!cheapest.count(rv) || ed.getCost() < cheapest[rv]->getCost()) cheapest[rv] = &ed;
        }
        if (cheapest.empty()) break;

        std::unordered_set<const Edge*> added;
        for (auto& [comp, ed] : cheapest) {
            if (added.count(ed)) continue;
            int u = ed->getFirst().getIndex(), v = ed->getSecond().getIndex();
            if (uf.unite(u, v)) {
                steps.push_back({u, v, ed->getCost(), true});
                added.insert(ed);
                mstEdges++;
            }
        }
        for (const auto& ed : graph.getEdges()) {
            int u = ed.getFirst().getIndex(), v = ed.getSecond().getIndex();
            if (uf.connected(u, v) && !added.count(&ed)) {
                bool inMST = false;
                for (auto& s : steps)
                    if (s.accepted && ((s.fromIndex == u && s.toIndex == v) || (s.fromIndex == v && s.toIndex == u)))
                    { inMST = true; break; }
                if (!inMST) steps.push_back({u, v, ed.getCost(), false});
            }
        }
    }
    return steps;
}
