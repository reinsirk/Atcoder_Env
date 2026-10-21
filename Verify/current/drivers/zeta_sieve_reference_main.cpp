// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

// 提出先のACL有無に依存しない、このドライバ専用の法998244353整数。
struct VerifyMod998 {
    std::uint32_t value = 0;
    VerifyMod998() = default;
    VerifyMod998(std::int64_t x) {
        x %= 998244353;
        if (x < 0) x += 998244353;
        value = static_cast<std::uint32_t>(x);
    }
    VerifyMod998& operator+=(VerifyMod998 b) { value += b.value; if (value >= 998244353) value -= 998244353; return *this; }
    VerifyMod998& operator-=(VerifyMod998 b) { if (value < b.value) value += 998244353; value -= b.value; return *this; }
    VerifyMod998& operator*=(VerifyMod998 b) { value = static_cast<std::uint32_t>(std::uint64_t(value) * b.value % 998244353); return *this; }
    friend VerifyMod998 operator+(VerifyMod998 a, VerifyMod998 b) { return a += b; }
    friend VerifyMod998 operator-(VerifyMod998 a, VerifyMod998 b) { return a -= b; }
    friend VerifyMod998 operator*(VerifyMod998 a, VerifyMod998 b) { return a *= b; }
    VerifyMod998 pow(std::int64_t exponent) const { VerifyMod998 a = *this, result = 1; for (; exponent; exponent >>= 1, a *= a) if (exponent & 1) result *= a; return result; }
    friend bool operator==(VerifyMod998, VerifyMod998) = default;
};

int main() {
    IntegerSieve sieve(500);
    const auto phi = sieve.totient_table();
    const auto mu = sieve.mobius_table();
    for (int x = 1; x <= 500; ++x) {
        int count = 0, distinct = 0, remaining = x; bool square = false, prime = x >= 2;
        for (int y = 1; y <= x; ++y) count += std::gcd(x, y) == 1;
        assert(phi[x] == count);
        std::vector<int> enumerate_divisors;
        for (int y = 1; y <= x; ++y) if (x % y == 0) enumerate_divisors.push_back(y);
        assert(sieve.sorted_divisors(x) == enumerate_divisors);
        auto unsorted = sieve.enumerate_divisors(x); std::sort(unsorted.begin(), unsorted.end()); assert(unsorted == enumerate_divisors);
        for (int p = 2; p <= x; ++p) {
            if (x % p == 0 && p != x) prime = false;
            if (remaining % p == 0) {
                ++distinct; int exponent = 0;
                do { remaining /= p; ++exponent; } while (remaining % p == 0);
                square |= exponent >= 2;
            }
        }
        assert(sieve.is_prime(x) == prime);
        assert(mu[x] == (square ? 0 : distinct % 2 ? -1 : 1));
        for (auto [d, value] : sieve.divisor_totients(x)) assert(value == phi[d]);
        int product = 1; for (auto [p, exponent] : sieve.prime_factorization(x)) while (exponent--) product *= p;
        assert(product == x);
    }
    assert(!sieve.is_prime(0));
    for (int exponent = 0; exponent <= 7; ++exponent) {
        auto powers = sieve.power_table<VerifyMod998>(exponent, 500);
        for (int x = 0; x <= 500; ++x) assert(powers[x] == VerifyMod998(x).pow(exponent));
    }
    auto copied = sieve; auto moved = std::move(copied);
    assert(copied.max_value() == -1 && moved.prime_list == sieve.prime_list);
    IntegerSieve empty(0); empty.swap(moved); assert(empty.prime_list == sieve.prime_list && moved.max_value() == 0);
    std::cout << "OK\n";
}
