#include "../Math/prime_judge(Miller-Rabin).cpp"
#include <array>
#include <cstdint>
#include <iostream>

int main()
{
    const std::array<std::uint64_t, 6> values{
        0, 2, 998244353, 341550071728321,
        2305843009213693951, 18446744073709551557u};
    for (auto value : values)
        std::cout << is_prime(value) << '\n'; // 順に0, 1, 1, 0, 1, 1。
    std::cout << is_prime(-7) << '\n';        // 0。
    std::cout << MillerRabin(2047, {2}) << '\n'; // 1。指定した底だけでは合成数も通る。
}
