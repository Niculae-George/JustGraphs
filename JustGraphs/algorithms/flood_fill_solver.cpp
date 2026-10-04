#include "algorithms/flood_fill_solver.h"
#include <queue>

std::vector<FloodFillSolver::GridPoint> FloodFillSolver::solve(
    const std::vector<std::vector<int>>& grid, 
    int startR, int startC, 
    int targetType)
{
    std::vector<GridPoint> fillOrder;
    int rows = grid.size();
    if (rows == 0) return fillOrder;
    const size_t cols = grid[0].size();
    for (const auto& row : grid) {
        if (row.size() != cols) return fillOrder;
    }

    if (startR < 0 || startR >= rows || startC < 0 || size_t(startC) >= cols) return fillOrder;
    if (grid[startR][startC] != targetType) return fillOrder;

    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::queue<GridPoint> q;

    q.push({startR, startC});
    visited[startR][startC] = true;

    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};

    while (!q.empty()) {
        GridPoint curr = q.front();
        q.pop();
        fillOrder.push_back(curr);

        for (int i = 0; i < 4; ++i) {
            int nr = curr.r + dr[i];
            int nc = curr.c + dc[i];
            if (nr >= 0 && nr < rows && nc >= 0 && static_cast<size_t>(nc) < cols &&
                grid[nr][nc] == targetType && !visited[nr][nc]) {
                visited[nr][nc] = true;
                q.push({nr, nc});
            }
        }
    }

    return fillOrder;
}
