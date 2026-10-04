#include "app/page_registry.h"
#include "features/articulation/vis_artbridge.h"
#include "features/components/vis_components.h"
#include "features/flow/vis_flow.h"
#include "features/flood_fill/vis_floodfill.h"
#include "features/maze/vis_maze.h"
#include "features/mst/vis_mst.h"
#include "features/shortest_paths/vis_shortest_paths_page.h"
#include "features/spanning_tree/vis_spanning_tree.h"
#include "features/topological/vis_topological.h"

namespace {
template<class Page> QWidget* createPage(QWidget* parent) { return new Page(parent); }
}

const std::vector<PageDescriptor>& pageDescriptors() {
    static const std::vector<PageDescriptor> pages = {
        {"maze", "Maze explorer", "Maze Explorer", "Explore breadth-first and depth-first search", "BFS DFS", createPage<VisMaze>},
        {"components", "Components", "Connected Components", "Find connected and strongly connected groups", "SCC Kosaraju", createPage<VisComponents>},
        {"mst", "Minimum spanning tree", "Minimum Spanning Tree", "Compare Kruskal, Prim, and Borůvka", "Kruskal Prim Boruvka", createPage<VisMST>},
        {"spanning", "Spanning tree", "Spanning Tree", "Build trees with breadth-first and depth-first search", "BFS DFS", createPage<VisSpanningTree>},
        {"shortest-paths", "Shortest paths", "Shortest Paths & Luxembourg Map", "Compare Dijkstra, A*, bidirectional Dijkstra, and Bellman–Ford on Luxembourg roads · Pan and zoom", "Luxembourg map roads routes Dijkstra A* bidirectional Bellman-Ford", createPage<VisShortestPathsPage>},
        {"flow", "Flow networks", "Flow Networks", "Visualize maximum flow and residual networks", "max flow source sink residual", createPage<VisFlow>},
        {"topological", "Topological sort", "Topological Sort", "Order tasks in a directed acyclic graph", "DAG dependencies", createPage<VisTopologicalSort>},
        {"flood-fill", "Flood fill", "Flood Fill", "Explore graph traversal on a grid", "grid cells BFS", createPage<VisFloodFill>},
        {"articulation", "Points & bridges", "Articulation Points & Bridges", "Find critical vertices and edges", "Tarjan cut vertices", createPage<VisArtBridge>}
    };
    return pages;
}
