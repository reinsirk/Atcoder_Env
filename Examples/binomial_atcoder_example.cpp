#include <atcoder/modint>
#include "../Math/Combination_Mod.cpp"
#include <iostream>

int main()
{
    ModIntBinomial<atcoder::modint998244353> comb;
    std::cout << comb.COM(10, 3).val() << '\n'; // 120
    std::cout << comb.Lucas(std::int64_t{1'000'000'000'000}, 3).val() << '\n';
    ModIntBinomial<atcoder::static_modint<12>> composite(11);
    std::cout << composite.COM(6, 2).val() << '\n'; // 3
    using Mint = atcoder::dynamic_modint<0>;
    Mint::set_mod(72); // 法の設定は計算器の構築より前。
    ModIntBinomial<Mint> dynamic_comb(71);
    std::cout << dynamic_comb.COM(10, 5).val() << '\n'; // 36
}
