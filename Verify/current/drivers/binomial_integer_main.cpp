// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>

int main() {

    int q{};
    std::uint32_t modulus{};
    verify_in >> q >> modulus;
    BinomialMod comb(modulus, modulus - 1);
    for (int i = 0; i < q; ++i) {
        std::int64_t n{}, k{};
        verify_in >> n >> k;
        verify_out << comb.COM(n, k) << '\n';
    }
}
