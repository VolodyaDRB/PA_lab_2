#include <iostream>
#include <chrono>
#include <cassert>
#include "bfs.h"

graph g;
bfs_out seq_ans;

typedef std::chrono::high_resolution_clock chrono_time;
using float_sec = std::chrono::duration<double>;
using float_time_point = std::chrono::time_point<chrono_time, float_sec>;

float_time_point get_cur_time() {
    return chrono_time::now();
}

size_t cord_to_vertex(size_t x, size_t y, size_t z, size_t len) {
    return x + y * len + z * len * len;
}

void add_edge(size_t v, size_t u) {
    g[v].push_back(u);
    g[u].push_back(v);
}

void gen_perf_test(size_t len) {
    g.assign(len * len * len, {});
    for (size_t x = 0; x < len; ++x) {
        for (size_t y = 0; y < len; ++y) {
            for (size_t z = 0; z < len; ++z) {
                size_t v = cord_to_vertex(x, y, z, len);
                if (x + 1 < len) add_edge(v, cord_to_vertex(x + 1, y, z, len));
                if (y + 1 < len) add_edge(v, cord_to_vertex(x, y + 1, z, len));
                if (z + 1 < len) add_edge(v, cord_to_vertex(x, y, z + 1, len));
            }
        }
    }
}

std::mt19937_64 rnd(std::random_device{}());
void gen_rand_test(size_t max_sz, size_t max_deg) {
    g.assign(rnd() % max_sz, {});
    for (size_t v = 0; v < g.size(); ++v) {
        for (int i = 0; i < rnd() % max_deg; ++i) {
            g[v].push_back(rnd() % g.size());
        }
    }
}

bool check_with_seq(const bfs_out &ans) {
    bool res = (ans.size() == seq_ans.size());
    for (size_t v = 0; v < ans.size(); ++v) {
        res &= (ans[v].size() == seq_ans[v].size());
        res &= (std::set(ans[v].begin(), ans[v].end()) == std::set(seq_ans[v].cbegin(), seq_ans[v].cend()));
    }
    return res;
}

template<BFSTypes BFSType>
double single_test() {
    auto startTime = get_cur_time();

    bfs_out ans = bfs<BFSType>(g, 0);
    double res = (get_cur_time() - startTime).count();
    if constexpr (BFSType == BFSTypes::SEQ) {
        seq_ans = ans;
    } else {
        assert(check_with_seq(ans));
    }

    return res;
}

int main() {
    printf("Number of workers = %s\n", getenv("PARLAY_NUM_THREADS"));

    const std::tuple<size_t, size_t, unsigned int> rand_test_groups[] = {
            {1000, 1, 10},
            {1000, 10, 10},
            {1000, 100, 10},
            {1000, 500, 10},
    };

    for (const auto [sz, deg, q]: rand_test_groups) {
        for (int i = 0; i < q; ++i) {
            gen_rand_test(sz, deg);
            single_test<BFSTypes::SEQ>();
            single_test<BFSTypes::PAR>();
        }
    }

    const std::tuple<size_t, unsigned int> perf_test_groups[] = {
            {400, 5},
    };

    for (const auto [len, q]: perf_test_groups) {
        double res[2];
        for (int i = 0; i < q; ++i) {
            gen_perf_test(len);
            res[static_cast<int>(BFSTypes::SEQ)] += single_test<BFSTypes::SEQ>();
            res[static_cast<int>(BFSTypes::PAR)] += single_test<BFSTypes::PAR>();
        }
        printf("Len = %zu\n", len);
        printf("\tSeq: %f s\n", res[static_cast<int>(BFSTypes::SEQ)] / q);
        printf("\tPar: %f s\n", res[static_cast<int>(BFSTypes::PAR)] / q);
        printf("\tSpeed-up: x%f\n", res[static_cast<int>(BFSTypes::SEQ)] / res[static_cast<int>(BFSTypes::PAR)]);
    }

    return 0;
}
