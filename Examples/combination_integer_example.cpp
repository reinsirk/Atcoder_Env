// SPDX-License-Identifier: CC0-1.0
#include "../Math/Combination/Binomial_Mod.cpp"
#include <cassert>
#include <iostream>

int main()
{
    BinomialMod prime(998244353, 100); // 必要な範囲だけ前計算。
    assert(prime.COM(10, 3) == 120);
    assert(prime.H(3, 4) == 15);
    BinomialMod small(7, 6);
    assert(small.Lucas(100, 20) == 0);
    BinomialMod composite(12, 11);
    assert(composite.COM(6, 2) == 3);
    BinomialOptions options;
    options.table_limit = 0; // テーブルの代わりに積で計算。
    BinomialMod uncached(998244353, 0, options);
    assert(uncached.COM(10, 3) == 120);
    BinomialMod large_composite(2'000'000'014, 0, options);
    assert(!large_composite.is_prime_modulus());
    assert(large_composite.COM(100, 2) == 4950);
    assert(large_composite.COM(2'000'000'014, 1) == 0);
    std::cout << prime.COM(10, 3) << '\n';
}
