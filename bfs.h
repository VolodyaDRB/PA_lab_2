#pragma once

#include <vector>
#include <atomic>
#include <parlay/primitives.h>

using graph = std::vector<std::vector<size_t>>;
using bfs_out = std::vector<std::vector<size_t>>;

enum class BFSTypes {
    SEQ,
    PAR,
};

template<typename B>
std::vector<size_t> bfs_seq_layer(const std::vector<size_t> &src, std::vector<B> &used, const graph &g) {
    std::vector<size_t> res;

    for (auto v: src) {
        for (auto u: g[v]) {
            if (!used[u]) {
                used[u] = 1;
                res.push_back(u);
            }
        }
    }

    return res;
}

bfs_out bfs_seq(const graph &g, size_t start);

template<size_t BLOCK_SIZE = 1000>
bfs_out bfs_par(const graph &g, size_t start) {
    bfs_out res;
    std::vector<std::atomic<bool>> used(g.size());
    std::vector<size_t> degs(g.size());
    std::vector<size_t> cur, vs;

    cur.push_back(start);
    used[start] = true;
    while (!cur.empty()) {
        res.push_back(std::move(cur));

        if (res.back().size() < BLOCK_SIZE) {
            cur = bfs_seq_layer(res.back(), used, g);
            continue;
        }

        size_t n = res.back().size();
        parlay::parallel_for(0, n, [&](size_t i) {
            degs[i] = g[res.back()[i]].size();
        });
        size_t sz = parlay::scan_inplace(std::ranges::subrange{degs.begin(), degs.begin() + n});
        vs.resize(sz);
        parlay::parallel_for(0, n, [&](size_t i) {
            size_t v = res.back()[i];
            for (int j = 0; j < g[v].size(); ++j) {
                vs[degs[i] + j] = g[res.back()[i]][j];
            }
        });

        cur = parlay::filter(std::ranges::subrange{vs.begin(), vs.begin() + sz}, [&](size_t v) {
            bool exp = false;
            return used[v].compare_exchange_strong(exp, true);
        }).to_vector();
    }

    return res;
}

template<BFSTypes BFSType>
bfs_out bfs(const graph &g, size_t start) {
    if constexpr (BFSType == BFSTypes::SEQ) {
        return bfs_seq(g, start);
    } else {
        return bfs_par(g, start);
    }
}