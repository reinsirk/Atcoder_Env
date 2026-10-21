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
    Mint::set_mod(72); // 法の設定は構築前。
    ModIntBinomial<Mint> composite(71);
    assert(composite.COM(10, 5).val() == 36);
    std::cout << prime.COM(10, 3).val() << '\n';
}
