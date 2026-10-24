#include "../Internal/Nyaan_hash_map_min.cpp"
#include <cstdint>
#include <iostream>
#include <string>

int main() {
    reinsirk::detail::InternalHashMap<std::int64_t, std::string> table;
    table.set_default("missing");
    table.reserve(10);
    std::cout << (table.capacity() >= 10) << ' ' << table.load_factor() << '\n'; // 1 0
    std::cout << table[-3] << '\n'; // missing
    int calls = 0;
    auto factory = [&] { ++calls; return std::string("ready"); };
    table.find_or_emplace(7, factory);
    table.find_or_emplace(7, factory);
    const auto& view = table;
    std::cout << *view.find(7) << ' ' << calls << ' ' << view.count(-3) << '\n'; // ready 1 1
    std::cout << view.contains(8) << ' ' << view.contain(7) << '\n'; // 0 1
    std::size_t visited = 0;
    view.for_each([&](const auto&, const auto&) { ++visited; });
    std::cout << view.enumerate().size() << ' ' << visited << '\n'; // 2 2
    // 確保済み容量があるときだけfind_nonemptyを使える。
    std::cout << *view.find_nonempty(7) << '\n'; // ready
    const auto capacity = table.capacity();
    table.clear(); // 値が保持する資源も残る。
    std::cout << table.empty() << ' ' << (table.capacity() == capacity) << '\n'; // 1 1
    table.release(); // スロットの値と配列を破棄する。
    std::cout << table.size() << ' ' << table.capacity() << '\n'; // 0 0
    std::cout << table[9] << '\n'; // missing: 設定済み初期値は保持する。
    auto copy = table;
    auto moved = std::move(copy);
    table.swap(moved);
    table.reallocate();
    std::cout << copy.empty() << ' ' << table.size() << '\n'; // 1 1
}
