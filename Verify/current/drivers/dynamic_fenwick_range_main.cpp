// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>

int main() {
    int n, q;
    if (!(verify_in >> n >> q)) return 0;
    DynamicFenwickTree<int, std::int64_t> bit(n);
    for (int i = 0; i < n; ++i) { std::int64_t a; verify_in >> a; bit.add(i, a); }
    while (q--) {
        int t, p; std::int64_t x;
        verify_in >> t >> p >> x;
        if (t == 0) bit.add(p, x);
        else verify_out << bit.sum(p, static_cast<int>(x)) << '\n';
    }
}
