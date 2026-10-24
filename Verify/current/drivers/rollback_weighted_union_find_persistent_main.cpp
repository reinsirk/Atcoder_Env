// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <array>
#include <cstdint>
#include <vector>

int main() {
    int n, q; verify_in >> n >> q;
    std::vector<std::array<int, 3>> queries(q + 1);
    std::vector<std::vector<int>> children(q + 1);
    for (int i = 1; i <= q; ++i) {
        int k; auto& query = queries[i];
        verify_in >> query[0] >> k >> query[1] >> query[2];
        children[k + 1].push_back(i);
    }
    RollbackWeightedUnionFind<std::int64_t> uf(n);
    std::vector<int> answers(q + 1, -1);
    struct Frame { int vertex, state; bool exit; };
    std::vector<Frame> stack{{0, 0, false}};
    while (!stack.empty()) {
        const auto frame = stack.back(); stack.pop_back();
        if (frame.exit) {
            uf.rollback(frame.state);
            continue;
        }
        const int state = uf.get_state();
        if (frame.vertex != 0) {
            const auto& query = queries[frame.vertex];
            if (query[0] == 0) uf.unite(query[1], query[2], std::int64_t{0});
            else answers[frame.vertex] = uf.same(query[1], query[2]);
        }
        stack.push_back({frame.vertex, state, true});
        for (int child : children[frame.vertex]) stack.push_back({child, 0, false});
    }
    for (int i = 1; i <= q; ++i)
        if (answers[i] != -1) verify_out << answers[i] << '\n';
}
