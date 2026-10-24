// SPDX-License-Identifier: CC0-1.0
#include <atcoder/modint>
#include "../Math/Combination/Prime_Binomial.cpp"
#include <cassert>
#include <iostream>
#include <utility>

int main()
{
    PrimeBinomial<atcoder::modint998244353> prime(100);
    assert(prime.COM(10, 3).val() == 120);
    assert(prime.H(3, 4).val() == 15);
    PrimeBinomial<atcoder::static_modint<7>> small(6);
    assert(small.Lucas(100, 20).val() == 0);
    PrimeBinomial<atcoder::static_modint<3>> ternary(2);
    assert(ternary.Lucas(80, 17).val() == 2);
    auto moved = std::move(ternary);
    ternary.prepare(2); // move元はprepareで表を再準備できる。
    assert(ternary.Lucas(80, 17).val() == moved.Lucas(80, 17).val());
    using Mint = atcoder::dynamic_modint<0>;
    Mint::set_mod(97); // 素数の法を構築前に設定。
    PrimeBinomial<Mint> dynamic_prime(96);
    assert(dynamic_prime.COM(10, 3).val() == 23);
    std::cout << prime.COM(10, 3).val() << '\n';
}
