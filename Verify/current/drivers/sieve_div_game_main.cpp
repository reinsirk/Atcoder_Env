// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>

int main() {
    std::int64_t n{}; verify_in >> n;
    int bound = 1;
    while (std::int64_t{bound} * bound < n) ++bound;
    IntegerSieve sieve(bound);
    int answer = 0;
    for (const auto& [prime, count] : sieve.prime_factorization(n)) {
        (void)prime;
        int remaining = count;
        for (int exponent = 1; exponent <= remaining; ++exponent) {
            remaining -= exponent;
            ++answer;
        }
    }
    verify_out << answer << '\n';
}
