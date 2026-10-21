#include "../Math/Zeta_Mebius.cpp"
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

int main() {
    std::vector<std::int64_t> f{1, 2, 3, 4};
    subset_zeta_transform(f);
    for (auto x : f) std::cout << x << ' ';
    std::cout << '\n'; // 1 3 4 10
    subset_mobius_transform(f);
    std::cout << f[3] << '\n'; // 4
    const auto bitwise = bitwise_or_convolution(std::vector<int>{1, 2}, std::vector<int>{3, 4});
    std::cout << bitwise[0] << ' ' << bitwise[1] << '\n'; // 3 18
    IntegerSieve sieve(100);
    std::vector<int> values{0, 1, 2, 3, 4, 5, 6};
    multiple_zeta_transform(values, sieve);
    std::cout << values[1] << ' ' << values[2] << '\n'; // 21 12
    multiple_mobius_transform(values, sieve);
    std::cout << values[2] << '\n'; // 2
    for (auto [p, exponent] : sieve.prime_factorization(72)) std::cout << p << '^' << exponent << ' ';
    std::cout << '\n'; // 2^3 3^2
    std::cout << sieve.totient_table()[12] << ' ' << sieve.mobius_table()[6] << '\n'; // 4 1
    auto gcd = gcd_convolution(std::vector<int>{2, 3, 5}, std::vector<int>{7, 11, 13});
    for (auto x : gcd) std::cout << x << ' ';
    std::cout << '\n'; // 14 170 126（0との混合項も含む）
    auto lcm = lcm_convolution(std::vector<int>{2, 3, 5}, std::vector<int>{7, 11, 13}, sieve);
    for (auto x : lcm) std::cout << x << ' ';
    std::cout << '\n'; // 118 33 159（添字が3以上になる項は含めない）
    std::cout << (gcd == gcd_convolution(std::vector<int>{2, 3, 5}, std::vector<int>{7, 11, 13}, sieve)) << '\n'; // 1
    std::vector<std::pair<int, int>> sparse{{1, 1}, {2, 2}, {3, 3}, {6, 6}};
    sparse_multiple_zeta_transform(sparse, sieve);
    std::cout << sparse[0].second << '\n'; // 12
    sparse_multiple_mobius_transform(sparse, sieve);
    std::cout << sparse[0].second << '\n'; // 1
}
