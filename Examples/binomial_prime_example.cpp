#include <atcoder/modint>
#include "../Math/Combination_AtCoderMod.cpp"
#include <iostream>

int main()
{
    PrimeBinomial<atcoder::modint998244353> comb;
    std::cout << comb.COM(10, 3).val() << '\n'; // 120
    std::cout << comb.H(3, 4).val() << '\n';    // 15
    PrimeBinomial<atcoder::static_modint<7>> small;
    std::cout << small.Lucas(100, 20).val() << '\n'; // 0
}
