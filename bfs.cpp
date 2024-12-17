#include "bfs.h"

bfs_out bfs_seq(const graph &g, size_t start) {
    bfs_out res;
    std::vector<size_t> cur;
    std::vector<char> used(g.size());

    cur.push_back(start);
    used[start] = 1;
    while (!cur.empty()) {
        res.push_back(std::move(cur));
        cur = bfs_seq_layer(res.back(), used, g);
    }

    return res;
}