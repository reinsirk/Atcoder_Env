// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <atcoder/modint>

int main() {
    using Mint = atcoder::modint998244353;
    int n, q; verify_in >> n >> q;
    UnionFindW<Mint> uf(n);
    while (q--) {
        int type, u, v; verify_in >> type >> u >> v;
        if (type == 0) {
            int x; verify_in >> x;
            verify_out << uf.constrain(v, u, Mint(x)) << '\n';
        } else {
            const auto difference = uf.try_diff(v, u);
            verify_out << (difference ? difference->val() : -1) << '\n';
        }
    }
}
