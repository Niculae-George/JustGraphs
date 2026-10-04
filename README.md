# JustGraphs

**From seven university homework assignments to a complete graph algorithm studio.** JustGraphs brings classroom algorithms to life in an interactive desktop application built around SOLID principles.

A C++20 + Qt desktop app for seeing graph algorithms work.
Nine visualizers, editable graphs, animation controls, and a built-in Luxembourg road map.

![JustGraphs overview](screenshots/overview.png)

## Build and run

Install these first:

- Python 3.9+
- CMake 3.21+
- Qt 6.9+ desktop kit, including Core, Gui, Widgets, deployment tools, and Test
- A C++20 compiler: Xcode Command Line Tools on macOS, MSVC or MinGW on Windows, GCC or Clang on Linux (use a Qt kit matching your compiler)

Run from the project folder:

```sh
python3 build.py
```

On Windows, use `py -3 build.py`.
The script asks for build mode and install location, then builds and installs the app.

For a Release build with tests:

```sh
python3 build.py --yes --test
```

If Qt is not found, add `--qt-prefix "/path/to/Qt/desktop-kit"`.
Open the installed `JustGraphs.app` on macOS, `Start-JustGraphs.cmd` on Windows,
or `start-justgraphs` on Linux. Default output: `dist/<OS>/release/`.
The script uses the current platform; it does not install prerequisites or cross-compile.

## Controls

- Choose a visualizer in the sidebar. Search filters the list.
- For animated visualizers, choose an algorithm, then press **Start**. Use **Speed** and **Pause / Resume** where available.
- In editable graphs: **right-click** to add a node, **click two nodes** to connect them, and **drag** to move a node.
- **Delete nodes/edges** switches to deletion. Click a node or edge to remove it; deleting a node removes its edges too. Switch the button off to edit again. In Flow Networks, choose **Edit network**.
- **New graph**, **New maze**, **Clear**, or **Reset** lets you start over.

<details>
<summary>Sidebar search and delete mode</summary>

![Sidebar search](screenshots/search.png)
![Deleting graph nodes and edges](screenshots/graph-delete.png)
![Delete mode in Points & Bridges](screenshots/graph-delete-articulation.png)
![Paused maze animation](screenshots/maze-paused.png)

</details>

## Visualizers

### Maze Explorer

Find a path from top-left to bottom-right with **BFS** or **DFS**. Generate a new maze and compare the searches.

![Maze solved with BFS](screenshots/maze-bfs.png)

<details>
<summary>DFS example</summary>

![Maze solved with DFS](screenshots/maze-dfs.png)

</details>

### Connected Components

Find separate groups in an undirected graph. Turn on **Directed graph** for strongly connected components using Kosaraju, then **Show condensed graph** to collapse each group into one node.

![Connected components](screenshots/components-undirected.png)

<details>
<summary>Directed components and condensed graph</summary>

![Strongly connected components](screenshots/components-directed.png)
![Condensed graph](screenshots/components-condensed.png)

</details>

### Minimum Spanning Tree

Compare **Kruskal**, **Prim**, and **Boruvka** on a weighted graph. See accepted edges, rejected edges, and the final tree. Turn on **Edge weights** to show costs.

![Kruskal minimum spanning tree](screenshots/mst-kruskal.png)

<details>
<summary>Prim, Boruvka, and edge weights</summary>

![Prim minimum spanning tree](screenshots/mst-prim.png)
![Boruvka minimum spanning tree](screenshots/mst-boruvka.png)
![Minimum spanning tree with edge weights visible](screenshots/mst-weights.png)

</details>

### Spanning Tree

Build a **BFS** or **DFS** tree. Edge colors show tree, back, and cross edges.

![BFS spanning tree](screenshots/spanning-bfs.png)

<details>
<summary>DFS example</summary>

![DFS spanning tree](screenshots/spanning-dfs.png)

</details>

### Shortest Paths & Luxembourg Map

Click two road nodes to choose start and destination. Compare **Dijkstra**, **A\***, **bidirectional Dijkstra**, and **Bellman–Ford**. Drag to pan, scroll or press **+ / −** to zoom, and use **Fit map** to return to the full map.

The map is bundled with the app; no download is needed. Routes use the XML's directed roads and length costs. Floyd–Warshall is disabled because this map is too large for its all-pairs table.

![Dijkstra route across Luxembourg](screenshots/map-dijkstra.png)

<details>
<summary>Other algorithms and pan / zoom</summary>

![A-star route](screenshots/map-astar.png)
![Bidirectional Dijkstra route](screenshots/map-bidirectional.png)
![Bellman-Ford route](screenshots/map-bellman-ford.png)
![Zoomed Luxembourg map](screenshots/map-pan-zoom.png)

</details>

### Flow Networks

Create a directed network. When connecting two nodes, enter **capacity** and optional **cost** in the top editor. Choose **Set source** and **Set sink**, then **Start flow** and **Next step** to follow maximum flow and the final minimum cut.

![Editing an edge capacity and cost](screenshots/flow-capacity.png)

<details>
<summary>Maximum flow and negative-cycle removal</summary>

![Maximum flow and minimum cut](screenshots/flow-max-flow.png)

**Remove negative cycles** cancels negative-cost cycles in the residual network.

![Negative-cycle removal](screenshots/flow-negative-cycle.png)

</details>

### Topological Sort

Build a directed graph and press **Sort graph** to get an order. Cycles are reported; use delete mode to remove a mistaken edge, then sort again.

![Topological order](screenshots/topological-order.png)

<details>
<summary>Cycle detection and repair</summary>

![A cycle prevents topological sorting](screenshots/topological-cycle.png)
![Sorting again after deleting the cycle edge](screenshots/topological-repaired.png)

</details>

### Flood Fill

Press **Generate walls**, or left-click cells to add/remove walls. Right-click an open cell to fill its connected region. **Clear grid** removes walls and fill.

![Automatically generated flood-fill walls](screenshots/flood-walls.png)

<details>
<summary>Filled region</summary>

![Flood fill inside generated walls](screenshots/flood-fill.png)

</details>

### Articulation Points & Bridges

Build an undirected graph and run the search. Highlighted points and bridges show where removing one node or edge would disconnect the graph.

![Articulation points and bridges](screenshots/points-bridges.png)

## Architecture

The app is split into small CMake libraries:

| Folder inside `JustGraphs/` | Job |
| --- | --- |
| `app/` | Window, sidebar, and page registry |
| `features/` | Each visualizer's controls and interaction |
| `algorithms/` | Graph algorithms and their results |
| `domain/` | Nodes, edges, editable graphs, and flow-network data |
| `maps/` | Luxembourg data loading and road routing |
| `rendering/` + `ui/` | Drawing, theme, shared gestures, and playback state |
| `tests/` | Model, algorithm, rendering, UI, and architecture checks |

Pages call algorithms and pass results to renderers. Models own data; algorithms do not depend on widgets.
Read-only graph access and editing use separate interfaces. Flow networks and road maps have their own models.
Wall generation is separate from the Flood Fill page, so the UI only applies the generated grid.

CMake discovers source files inside each module automatically. `cmake/` contains build and deployment helpers; `build.py` provides the guided installer.

Keep app source, tests, map resources, build configuration/scripts, this README, and screenshots in the repository.
Build folders, installed binaries, and IDE caches are generated locally and ignored.
