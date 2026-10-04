#include "algorithms/kosaraju_solver.h"

std::unordered_map<int, std::vector<int>> KosarajuSolver::buildDirectedAdjList(const IGraphData& graph) const

{
    std::unordered_map<int, std::vector<int>> adj;
    for (const auto& node : graph.getNodes()) adj[node.getIndex()] = {};
    for (const auto& edge : graph.getEdges()) {
        int from = edge.getFirst().getIndex();
        int to = edge.getSecond().getIndex();
        adj[from].push_back(to);
    }
    return adj;
}

std::unordered_map<int, std::vector<int>> KosarajuSolver::getTranspose(const IGraphData& graph) const

{
    std::unordered_map<int, std::vector<int>> transposeAdj;
    for (const auto& node : graph.getNodes()) transposeAdj[node.getIndex()] = {};
    for (const auto& edge : graph.getEdges()) {
        int from = edge.getFirst().getIndex();
        int to = edge.getSecond().getIndex();
        transposeAdj[to].push_back(from);
    }
    return transposeAdj;
}

void KosarajuSolver::fillOrder(int nodeIndex, std::unordered_set<int>& visited,
                                std::stack<int>& Stack,
                                const std::unordered_map<int, std::vector<int>>& adjList) const
{
    std::stack<std::pair<int, size_t>> dfsStack;
    dfsStack.emplace(nodeIndex, 0);
    visited.insert(nodeIndex);
    while (!dfsStack.empty()) {
        int currentNode = dfsStack.top().first;
        size_t& currentIndex = dfsStack.top().second;
        auto it = adjList.find(currentNode);
        if (it == adjList.end()) { Stack.push(currentNode); dfsStack.pop(); continue; }
        const auto& neighbors = it->second;
        if (currentIndex < neighbors.size()) {
            int neighbor = neighbors[currentIndex]; currentIndex++;
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                dfsStack.emplace(neighbor, 0);
            }
        } else {
            Stack.push(currentNode); dfsStack.pop();
        }
    }
}

void KosarajuSolver::DFSUtil(int nodeIndex, std::unordered_set<int>& visited,
                              std::unordered_map<int, std::vector<int>>& transposeAdj,
                              int component,
                              std::unordered_map<int, int>& outComponentColors) const
{
    std::stack<std::pair<int, size_t>> stack;
    stack.emplace(nodeIndex, 0);
    visited.insert(nodeIndex);
    outComponentColors[nodeIndex] = component;
    while (!stack.empty()) {
        int currentNode = stack.top().first;
        size_t& currentIndex = stack.top().second;
        auto it = transposeAdj.find(currentNode);
        if (it == transposeAdj.end()) { stack.pop(); continue; }
        const auto& neighbors = it->second;
        if (currentIndex < neighbors.size()) {
            int neighbor = neighbors[currentIndex]; currentIndex++;
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                outComponentColors[neighbor] = component;
                stack.emplace(neighbor, 0);
            }
        } else { stack.pop(); }
    }
}

int KosarajuSolver::solve(const IGraphData& graph,
                           std::unordered_map<int, int>& outComponentColors) const {
    outComponentColors.clear();
    if (graph.getNodes().empty()) return 0;

    auto adjList = buildDirectedAdjList(graph);

    std::stack<int> finishTimeStack;
    std::unordered_set<int> visited;
    for (const auto& node : graph.getNodes()) {
        int nodeIndex = node.getIndex();
        if (visited.find(nodeIndex) == visited.end())
            fillOrder(nodeIndex, visited, finishTimeStack, adjList);
    }

    auto transposeAdj = getTranspose(graph);
    visited.clear();
    int numComponents = 0;
    while (!finishTimeStack.empty()) {
        int nodeIndex = finishTimeStack.top(); finishTimeStack.pop();
        if (visited.find(nodeIndex) == visited.end()) {
            DFSUtil(nodeIndex, visited, transposeAdj, numComponents, outComponentColors);
            numComponents++;
        }
    }
    return numComponents;
}
