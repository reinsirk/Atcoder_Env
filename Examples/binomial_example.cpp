#include "../Math/Combination_Mod.cpp"
#include <iostream>

int main()
{
    BinomialMod prime(998244353); // 既定で500万まで前計算。
    std::cout << prime.COM(10, 3) << '\n'; // 120
    BinomialMod small_prime(7, 6);
    std::cout << small_prime.Lucas(100, 20) << '\n'; // 0
    BinomialMod prime_power(8, 7), composite(12, 11);
    std::cout << prime_power.COM(6, 2) << '\n'; // 7
    std::cout << composite.COM(6, 2) << '\n';   // 3
    std::cout << prime.COM(std::int64_t{1'000'000'000'000}, 3) << '\n';
    std::cout << composite.H(3, 4) << '\n'; // 3（15 mod 12）
    BinomialMod trivial(1);
    std::cout << trivial.COM(0, 0) << '\n'; // 0
    std::cout << prime.COM(3, 5) << '\n';   // 0
}
