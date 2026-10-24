// SPDX-License-Identifier: CC0-1.0
#ifndef SOURCE
#define SOURCE "../../../Math/Zeta/Sieve.cpp"
#endif
#include SOURCE
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
namespace {
std::uint64_t checks = 0;
void check(bool value) { ++checks; if (!value) std::abort(); }
void verify(int n) {
    const auto count = static_cast<std::size_t>(n) + 1;
    std::vector<int> reference(count), primes;
    for (int i = 2; i <= n; ++i) {
        if (reference[static_cast<std::size_t>(i)] != 0) continue;
        reference[static_cast<std::size_t>(i)] = i;
        primes.push_back(i);
        if (i > n / i) continue;
        for (std::int64_t j = std::int64_t{i} * i; j <= n; j += i)
            if (reference[static_cast<std::size_t>(j)] == 0)
                reference[static_cast<std::size_t>(j)] = i;
    }
    IntegerSieve actual(n);
    check(actual.max_value() == n);
    check(actual.least_prime_factor == reference);
    check(actual.prime_list == primes);
    for (int i = 0; i <= n; ++i)
        check(actual.is_prime(i) == (i >= 2 && reference[static_cast<std::size_t>(i)] == i));
    auto copied = actual;
    check(copied.least_prime_factor == reference);
    copied.least_prime_factor[0] = 123;
    check(actual.least_prime_factor[0] == 0);
    auto moved = std::move(actual);
    check(actual.max_value() == -1 && actual.least_prime_factor.empty() && actual.prime_list.empty());
    check(moved.least_prime_factor == reference);
}
}
int main() {
    for (int n = 0; n <= 257; ++n) verify(n);
    for (int n : {4095, 4096, 4097, 8191, 8192, 8193, 262143, 262144, 1000000, 10000000}) verify(n);
#ifndef SKIP_RUNTIME_CHECKS
    bool caught = false;
    try { IntegerSieve invalid(-1); } catch (const std::invalid_argument&) { caught = true; }
    check(caught);
#endif
    std::cout << "PASS checks=" << checks << '\n';
}
