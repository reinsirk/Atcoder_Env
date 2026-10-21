// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>
#include <cstdint>

int main() {

    int q{};
    std::uint32_t modulus{};
    verify_in >> q >> modulus;
    // 問題のn,kはmin(modulus,10^7)未満。
    BinomialOptions options;
    options.table_limit = 9'999'999;
    const auto max_n = std::min<std::uint32_t>(modulus - 1, options.table_limit);
    BinomialMod comb(modulus, max_n, options);
    for (int i = 0; i < q; ++i) {
        std::int64_t n{}, k{};
        verify_in >> n >> k;
        verify_out << comb.COM(n, k) << '\n';
    }
}
