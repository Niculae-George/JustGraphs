#ifndef FLOOD_FILL_SOLVER_H
#define FLOOD_FILL_SOLVER_H

#include <vector>

class FloodFillSolver {
public:
    struct GridPoint {
        int r, c;
    };

    // Rectangular grids only; invalid shapes and out-of-bounds starts return no cells.
    static std::vector<GridPoint> solve(const std::vector<std::vector<int>>& grid, 
                                       int startR, int startC, 
                                       int targetType);
};

#endif // FLOOD_FILL_SOLVER_H
