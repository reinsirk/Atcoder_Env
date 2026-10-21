// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>

int main() {
    int n{}, q{}; if (!(verify_in >> n >> q)) return 0;
    ErasablePriorityQueue<std::int64_t> low;
    ErasablePriorityQueue<std::int64_t, false> high;
    auto add = [&](std::int64_t x) { low.push(x); high.push(x); };
    while (n--) { std::int64_t x{}; verify_in >> x; add(x); }
    while (q--) {
        int t{}; verify_in >> t;
        if (t == 0) { std::int64_t x{}; verify_in >> x; add(x); }
        else if (t == 1) {
            auto x = low.top(); low.pop(); high.erase(x); verify_out << x << '\n';
        } else {
            auto x = high.top(); high.pop(); low.erase(x); verify_out << x << '\n';
        }
    }
}
