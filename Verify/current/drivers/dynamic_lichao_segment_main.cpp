// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>
#include <limits>

int main() {
    int n{}, q{};
    if (!(verify_in >> n >> q)) return 0;
    constexpr std::int64_t infinity = std::numeric_limits<std::int64_t>::max();
    // 問題の直線値は約±2e18まで届くので、共通INF=2^60は空の印に使わない。
    DynamicLiChaoTree<std::int64_t, infinity> tree(-1'000'000'000, 1'000'000'000);
    auto read_line = [&] {
        std::int64_t l{}, r{}, a{}, b{}; verify_in >> l >> r >> a >> b;
        tree.add_segment(a, b, l, r - 1);
    };
    while (n--) read_line();
    while (q--) {
        int type{}; verify_in >> type;
        if (type == 0) read_line();
        else {
            std::int64_t x{}; verify_in >> x;
            auto answer = tree.query(x);
            if (answer == infinity) verify_out << "INFINITY\n";
            else verify_out << answer << '\n';
        }
    }
}
