#include "../String/RollingHash.cpp"
#include <cstdint>
#include <iostream>
#include <string_view>
#include <utility>

int main() {
    // 実運用では十分大きな底を実行時に乱数で選び、比較対象間で共有する。
    constexpr std::uint64_t base = 911382323;
    const RollingHash first("abracadabra", base), second("abra", base);
    std::cout << first.size() << ' ' << first.empty() << '\n'; // 11 0
    std::cout << first.equal(0, 4, second, 0, 4) << '\n'; // 1（衝突の可能性あり）
    std::cout << first.lcp(0, first, 7) << '\n'; // 4
    std::cout << first.compare(0, 4, first, 7, 11) << '\n'; // 0
    auto joined = first.concat(first.get(0, 4), first.get(4, 11), 7);
    std::cout << (joined == first.whole()) << '\n'; // 1
    RollingHash copy = {"abracadabra", base};
    RollingHash moved = std::move(copy);
    std::cout << copy.empty() << ' ' << moved.equal(0, moved.size(), first, 0, first.size()) << '\n'; // 1 1
    const RollingHash empty(std::string_view{}, base);
    std::cout << empty.empty() << ' ' << empty.whole() << '\n'; // 1 0
}
