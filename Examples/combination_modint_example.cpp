// SPDX-License-Identifier: CC0-1.0
#include <atcoder/modint>
#include "../Math/Combination/ModInt_Binomial.cpp"
#include <cassert>
#include <iostream>

int main()
{
    ModIntBinomial<atcoder::modint998244353> prime(100);
    assert(prime.COM(10, 3).val() == 120);
    assert(prime.H(3, 4).val() == 15);
    ModIntBinomial<atcoder::static_modint<7>> small(6);
    assert(small.Lucas(100, 20).val() == 0);
    using Mint = atcoder::dynamic_modint<0>;
    {
        Mint::set_mod(72); // 法の設定は構築前。
        ModIntBinomial<Mint> composite(71);
        assert(composite.COM(10, 5).val() == 36);
    }
    Mint::set_mod(2'000'000'014);
    BinomialOptions options;
    options.table_limit = 0; // 大きい法でも表を持たずに計算できる。
    ModIntBinomial<Mint> large_composite(0, options);
    assert(large_composite.COM(100, 2).val() == 4950);
    std::cout << prime.COM(10, 3).val() << '\n';
}
