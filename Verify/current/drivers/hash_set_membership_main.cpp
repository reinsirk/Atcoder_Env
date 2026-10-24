// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>
#include <vector>

int main() {
    int n{}; if (!(verify_in >> n)) return 0;
    std::vector<std::int64_t> a(n);
    for (auto& x : a) verify_in >> x;
    int m{}; verify_in >> m;
    std::vector<std::int64_t> b(m);
    for (auto& x : b) verify_in >> x;
    int l{}; verify_in >> l;
    std::vector<std::int64_t> c(l);
    for (auto& x : c) verify_in >> x;
    HashSet<std::int64_t> sums;
    for (auto x : a) for (auto y : b) for (auto z : c) sums.insert(x + y + z);
    int q{}; verify_in >> q;
    while (q--) {
        std::int64_t x{}; verify_in >> x;
        verify_out << (sums.contains(x) ? "Yes\n" : "No\n");
    }
}
