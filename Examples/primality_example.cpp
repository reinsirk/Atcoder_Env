// SPDX-License-Identifier: CC0-1.0

#include "../Math/prime_judge(Miller-Rabin).cpp"
#include <array>
#include <cassert>
#include <limits>
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
    // 境界を含めて、前後の素数を探す。
    const auto next = next_prime(100);
    const auto previous = prev_prime(100);
    assert(next == 101 && previous == 97);
    std::cout << *next << ' ' << *previous << '\n';
    assert(next_prime(-7) == 2 && !prev_prime(1));
    assert(!next_prime(std::numeric_limits<std::uint64_t>::max()));
}
