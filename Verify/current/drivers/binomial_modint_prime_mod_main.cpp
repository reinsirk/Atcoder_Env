// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>
#include <cstdint>
#include <atcoder/modint>

int main() {

    int q{};
    std::uint32_t modulus{};
    verify_in >> q >> modulus;
    using Mint = atcoder::dynamic_modint<0>;
    Mint::set_mod(static_cast<int>(modulus));
    BinomialOptions options;
    options.table_limit = 9'999'999;
    const auto max_n = std::min<std::uint32_t>(modulus - 1, options.table_limit);
    ModIntBinomial<Mint> comb(max_n, options);
    for (int i = 0; i < q; ++i) {
        std::int64_t n{}, k{};
        verify_in >> n >> k;
        verify_out << comb.COM(n, k).val() << '\n';
    }
}
