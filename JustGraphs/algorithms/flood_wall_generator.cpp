#include "algorithms/flood_wall_generator.h"

#include <random>

namespace {
void divide(std::vector<std::vector<int>>& grid, int top, int bottom,
            int left, int right, std::mt19937& random) {
    const int height = bottom - top + 1;
    const int width = right - left + 1;
    const bool canSplitHorizontally = height >= 5;
    const bool canSplitVertically = width >= 5;
    if (!canSplitHorizontally && !canSplitVertically) return;

    bool horizontal;
    if (!canSplitVertically) horizontal = true;
    else if (!canSplitHorizontally) horizontal = false;
    else horizontal = std::uniform_int_distribution<int>(0, 1)(random) == 0;

    if (horizontal) {
        const int wallRow = std::uniform_int_distribution<int>(top + 2, bottom - 2)(random);
        for (int col = left; col <= right; ++col) grid[wallRow][col] = 1;
        const int doorway = std::uniform_int_distribution<int>(left, right)(random);
        // Some dividers retain a doorway; others create enclosed regions.
        if (std::uniform_int_distribution<int>(0, 3)(random) != 0) grid[wallRow][doorway] = 0;
        divide(grid, top, wallRow - 1, left, right, random);
        divide(grid, wallRow + 1, bottom, left, right, random);
    } else {
        const int wallCol = std::uniform_int_distribution<int>(left + 2, right - 2)(random);
        for (int row = top; row <= bottom; ++row) grid[row][wallCol] = 1;
        const int doorway = std::uniform_int_distribution<int>(top, bottom)(random);
        if (std::uniform_int_distribution<int>(0, 3)(random) != 0) grid[doorway][wallCol] = 0;
        divide(grid, top, bottom, left, wallCol - 1, random);
        divide(grid, top, bottom, wallCol + 1, right, random);
    }
}
}

std::vector<std::vector<int>> FloodWallGenerator::generate(int rows, int cols, std::uint32_t seed) {
    if (rows <= 0 || cols <= 0) return {};

    std::vector<std::vector<int>> grid(static_cast<std::size_t>(rows),
                                       std::vector<int>(static_cast<std::size_t>(cols), 0));
    if (rows < 5 || cols < 5) return grid;

    for (int row = 0; row < rows; ++row) {
        grid[row][0] = 1;
        grid[row][cols - 1] = 1;
    }
    for (int col = 0; col < cols; ++col) {
        grid[0][col] = 1;
        grid[rows - 1][col] = 1;
    }

    std::mt19937 random(seed);
    divide(grid, 1, rows - 2, 1, cols - 2, random);
    return grid;
}
