// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>

int main() {
    int n, q; if (!(verify_in >> n >> q)) return 0;
    DynamicUnionFind uf;
    while (q--) {
        int t, x, y; verify_in >> t >> x >> y;
        if (t == 0) uf.unite(x, y);
        else verify_out << uf.same(x, y) << '\n';
    }
}
