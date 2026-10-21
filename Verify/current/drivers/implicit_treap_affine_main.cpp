// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>
#include <vector>

// 問題用の法998244353での区間アフィン写像。ACLには依存しない。
struct VerifyAffineSum {
    static constexpr std::int64_t mod = 998244353;
    struct S { std::int64_t sum, count; };
    struct F { std::int64_t a, b; };
    static S op(S x, S y) { return {(x.sum + y.sum) % mod, x.count + y.count}; }
    static S e() { return {0, 0}; }
    static S mapping(F f, S x) { return {(f.a * x.sum + f.b * x.count) % mod, x.count}; }
    static F composition(F f, F g) { return {f.a * g.a % mod, (f.a * g.b + f.b) % mod}; }
    static F id() { return {1, 0}; }
};
int main() {
    int n, q; if (!(verify_in >> n >> q)) return 0;
    using A = VerifyAffineSum;
    std::vector<A::S> initial(n);
    for (auto& value : initial) { verify_in >> value.sum; value.count = 1; }
    lazy_implicit_treap<A::S, A::op, A::e, A::F, A::mapping, A::composition, A::id, true> sequence(initial);
    while (q--) {
        int type, l; verify_in >> type >> l;
        if (type == 0) { std::int64_t x; verify_in >> x; sequence.insert_at(l, {x, 1}); }
        else if (type == 1) sequence.erase_at(l);
        else {
            int r; verify_in >> r;
            if (type == 2) sequence.reverse(l, r);
            else if (type == 3) { std::int64_t b, c; verify_in >> b >> c; sequence.apply(l, r, {b, c}); }
            else verify_out << sequence.prod(l, r).sum << '\n';
        }
    }
}
