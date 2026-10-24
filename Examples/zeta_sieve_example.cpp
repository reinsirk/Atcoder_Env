// SPDX-License-Identifier: CC0-1.0
#include "../Math/Zeta/Sieve.cpp"
#include <cassert>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

// べき乗表の例用。実際には利用側のMintを渡す。
struct ExamplePower {
    std::int64_t value;
    ExamplePower(std::int64_t x = 0) : value(x) {}
    ExamplePower operator*(ExamplePower b) const { return value * b.value; }
    ExamplePower pow(std::int64_t k) const {
        ExamplePower base = *this, result = 1;
        for (; k; k /= 2, base = base * base) if (k % 2) result = result * base;
        return result;
    }
};

int main() {
    IntegerSieve sieve(100);
    assert(sieve.max_value() == 100 && sieve.is_prime(97) && !sieve.is_prime(1));
    assert((sieve.prime_factorization(72) == std::map<int, int>{{2, 3}, {3, 2}}));
    assert((sieve.sorted_divisors(12) == std::vector<int>{1, 2, 3, 4, 6, 12}));
    assert(sieve.enumerate_divisors(12).size() == 6);
    assert(sieve.divisor_totients(12).at(12) == 4);
    assert(sieve.mobius_table()[6] == 1 && sieve.mobius_table()[4] == 0);
    assert(sieve.totient_table()[12] == 4);
    const auto squares = sieve.power_table<ExamplePower>(2, 10);
    assert(squares[0].value == 0 && squares[10].value == 100);
    auto copy = sieve;
    IntegerSieve moved(std::move(copy));
    assert(copy.max_value() == -1 && moved.is_prime(97));
    IntegerSieve small(2);
    small.swap(moved);
    assert(small.is_prime(97) && moved.max_value() == 2);
}
