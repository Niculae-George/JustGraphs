#ifndef UNION_FIND_H
#define UNION_FIND_H

#include <unordered_map>
#include <vector>

struct UnionFind {
    std::unordered_map<int, int> parent, rank_;

    void init(const std::vector<int>& ids) {
        for (int id : ids) { parent[id] = id; rank_[id] = 0; }
    }

    int find(int x) {
        if (parent[x] != x) parent[x] = find(parent[x]);
        return parent[x];
    }

    bool unite(int a, int b) {
        int ra = find(a), rb = find(b);
        if (ra == rb) return false;
        if (rank_[ra] < rank_[rb]) std::swap(ra, rb);
        parent[rb] = ra;
        if (rank_[ra] == rank_[rb]) rank_[ra]++;
        return true;
    }

    bool connected(int a, int b) { return find(a) == find(b); }
};

#endif // UNION_FIND_H
