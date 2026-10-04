#ifndef FLOW_SOLVER_H
#define FLOW_SOLVER_H

#include <cstdint>
#include <set>
#include <vector>

class FlowNetwork;

enum class FlowIterationKind { Initial, Augmentation, FinalCut, NegativeCycleCancellation };

struct FlowIterationResult {
    int iterationNumber = 0;
    FlowIterationKind kind = FlowIterationKind::Initial;
    std::vector<std::vector<std::int64_t>> residualCapacity;
    std::vector<int> augmentingPath;
    std::int64_t pathFlow = 0;
    std::int64_t totalFlow = 0;
    int eliminatedCycles = 0;
};

class FlowSolver {
public:
    explicit FlowSolver(const FlowNetwork* network = nullptr);
    void setNetwork(const FlowNetwork* network);
    void initializeAlgorithm();
    bool performStep();
    void finalizeMinCut();
    void addFinalState();
    std::int64_t runFullAlgorithm();
    int eliminateNegativeCycles();

    std::int64_t totalFlow() const;
    std::int64_t getFlow(int from, int to) const;
    std::int64_t getResidual(int from, int to) const;
    const std::vector<FlowIterationResult>& iterations() const;
    const std::set<std::pair<int, int>>& minCutEdges() const;
    bool isMinCutEdge(int from, int to) const;
    void reset();

private:
    const FlowNetwork* m_network = nullptr;
    mutable unsigned long long m_revision = 0;
    mutable std::vector<std::vector<std::int64_t>> m_residual;
    mutable std::vector<FlowIterationResult> m_iterations;
    mutable std::set<std::pair<int, int>> m_minCutEdges;
    mutable std::int64_t m_totalFlow = 0;

    bool synchronizeRevision() const;
    void resetCache() const;
    bool bfs(const std::vector<std::vector<std::int64_t>>& residual, int source, int sink,
             std::vector<int>& parent) const;
    std::vector<int> findNegativeCycle() const;
};

#endif
