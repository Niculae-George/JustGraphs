#include "domain/directedgraph.h"
#include "domain/undirectedgraph.h"
#include "algorithms/bfs_traversal.h"
#include "algorithms/dfs_traversal.h"
#include "algorithms/topological_sort_solver.h"
#include "algorithms/connected_components_solver.h"
#include "algorithms/prim_solver.h"
#include "algorithms/kruskal_solver.h"
#include "algorithms/flood_fill_solver.h"
#include <QCoreApplication>
#include <QTextStream>
#include <algorithm>

namespace { int failures = 0; void check(bool ok, const char* what) { if (!ok) { QTextStream(stderr) << "FAIL: " << what << '\n'; ++failures; } } }
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    DirectedGraph d;
    int a=d.addNode({}), b=d.addNode({}), c=d.addNode({});
    d.addEdge(a,b);
    auto bfs = BFSTraversal{}.solve(d,b);
    auto dfs = DFSTraversal{}.solve(d,b);
    check(bfs.size()==1 && bfs[0].nodeIndex==b, "BFS follows directed reachability");
    check(dfs.size()==1 && dfs[0].nodeIndex==b, "DFS follows directed reachability");
    check(BFSTraversal{}.solve(d,999).empty(), "BFS rejects unknown root");
    check(DFSTraversal{}.solve(d,999).empty(), "DFS rejects unknown root");
    std::vector<int> order;
    auto topo=TopologicalSortSolver{}.solve(d,order);
    check(order.size()==3 && !TopologicalSortSolver{}.hasCycle(d), "directed DAG topologically sorted");
    d.addEdge(c,a);
    d.addEdge(b,c);
    check(TopologicalSortSolver{}.solve(d,order).empty() && TopologicalSortSolver{}.hasCycle(d), "directed cycle detected");
    UndirectedGraph u;
    int n1=u.addNode({}), n2=u.addNode({}), n3=u.addNode({});
    u.addNode({}); // Isolated vertex exercises disconnected-component handling.
    u.addEdge(n1,n2,4); u.addEdge(n2,n3,2); u.addEdge(n1,n3,9);
    auto comps=ConnectedComponentsSolver{}.solve(u);
    check(comps.numComponents==2, "independent components fixture includes isolated vertex");
    UndirectedGraph weighted;
    int w1=weighted.addNode({}), w2=weighted.addNode({}), w3=weighted.addNode({}), w4=weighted.addNode({});
    weighted.addEdge(w1,w2,7); weighted.addEdge(w1,w3,2); weighted.addEdge(w2,w3,3);
    weighted.addEdge(w2,w4,6); weighted.addEdge(w3,w4,4);
    auto prim=PrimSolver{}.solve(weighted);
    int primCost=0, primAccepted=0;
    for (const auto& s: prim) if (s.accepted) { primCost+=s.cost; ++primAccepted; }
    auto kruskal=KruskalSolver{}.solve(weighted);
    int kruskalCost=0, kruskalAccepted=0;
    for (const auto& s: kruskal) if (s.accepted) { kruskalCost+=s.cost; ++kruskalAccepted; }
    check(primCost==9 && primAccepted==3, "Prim fixture has known minimum spanning tree");
    check(kruskalCost==9 && kruskalAccepted==3, "Kruskal fixture has known minimum spanning tree");
    check(TopologicalSortSolver{}.solve(u,order).empty(), "topological sort rejects undirected graph by type");
    check(!TopologicalSortSolver{}.hasCycle(u), "cycle query defines undirected input as not a directed cycle");

    const std::vector<std::vector<int>> fillGrid{
        {0, 0, 1, 0},
        {1, 0, 1, 0},
        {1, 1, 1, 0}
    };
    const auto fill = FloodFillSolver::solve(fillGrid, 0, 0, 0);
    check(fill.size() == 3 && fill[0].r == 0 && fill[0].c == 0 &&
          fill[1].r == 0 && fill[1].c == 1 && fill[2].r == 1 && fill[2].c == 1,
          "flood fill returns reachable target cells in breadth-first order around obstacles");
    check(FloodFillSolver::solve(fillGrid, 0, 2, 0).empty(),
          "flood fill does nothing when start cell does not match target type");
    check(FloodFillSolver::solve(fillGrid, -1, 0, 0).empty() &&
          FloodFillSolver::solve(fillGrid, 3, 0, 0).empty() &&
          FloodFillSolver::solve(fillGrid, 0, 4, 0).empty(),
          "flood fill rejects unknown start coordinates");
    check(FloodFillSolver::solve({}, 0, 0, 0).empty(),
          "flood fill handles an empty grid as a no-op");
    check(FloodFillSolver::solve({{0, 0}, {}}, 0, 0, 0).empty(),
          "flood fill rejects ragged grids without indexing past short rows");
    return failures ? 1 : 0;
}
