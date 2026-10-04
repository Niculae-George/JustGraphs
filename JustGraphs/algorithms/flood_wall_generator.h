#ifndef FLOOD_WALL_GENERATOR_H
#define FLOOD_WALL_GENERATOR_H

#include <cstdint>
#include <vector>

class FloodWallGenerator {
public:
    // Returns a rectangular 0=open, 1=wall grid. Invalid dimensions return empty.
    static std::vector<std::vector<int>> generate(int rows, int cols, std::uint32_t seed);
};

#endif // FLOOD_WALL_GENERATOR_H
