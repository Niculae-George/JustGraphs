#include "algorithms/flood_wall_generator.h"

#include <iostream>
#include <utility>

namespace {
int failures = 0;
void check(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
}

int main() {
    check(FloodWallGenerator::generate(0, 4, 1).empty(), "zero rows return empty grid");
    check(FloodWallGenerator::generate(4, -1, 1).empty(), "negative columns return empty grid");
    for (const auto dimensions : {std::pair{1, 1}, std::pair{2, 7}, std::pair{4, 4}}) {
        const auto grid = FloodWallGenerator::generate(dimensions.first, dimensions.second, 42);
        check(static_cast<int>(grid.size()) == dimensions.first, "tiny grid preserves row count");
        bool allOpen = true;
        for (const auto& row : grid) {
            check(static_cast<int>(row.size()) == dimensions.second, "tiny grid preserves column count");
            for (int cell : row) allOpen = allOpen && cell == 0;
        }
        check(allOpen, "valid tiny grids are entirely open");
    }

    const auto first = FloodWallGenerator::generate(24, 32, 12345);
    const auto repeated = FloodWallGenerator::generate(24, 32, 12345);
    const auto otherSeed = FloodWallGenerator::generate(24, 32, 54321);
    check(first == repeated, "same seed reproduces identical grid");
    check(first != otherSeed, "different seeds produce different layouts");

    int walls = 0;
    int open = 0;
    bool shapeValid = static_cast<int>(first.size()) == 24;
    bool borderValid = true;
    for (int row = 0; row < static_cast<int>(first.size()); ++row) {
        shapeValid = shapeValid && static_cast<int>(first[row].size()) == 32;
        for (int col = 0; col < static_cast<int>(first[row].size()); ++col) {
            const int cell = first[row][col];
            check(cell == 0 || cell == 1, "grid contains only open and wall values");
            walls += cell == 1;
            open += cell == 0;
            if (row == 0 || col == 0 || row == 23 || col == 31) borderValid = borderValid && cell == 1;
        }
    }
    check(shapeValid, "generated grid has requested rectangular shape");
    check(borderValid, "large grid has continuous outer wall");
    check(walls > 0 && open > 0, "generated grid preserves both walls and open cells");
    return failures ? 1 : 0;
}
