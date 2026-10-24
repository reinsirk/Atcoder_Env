#include "../DataStructure/line-container.cpp"
#include <cstdint>
#include <iostream>

int main() {
    MaxLineContainer<std::int64_t> maximum;
    std::cout << maximum.query_optional(0).has_value() << '\n'; // 0
    maximum.add(2, 1);
    maximum.add(-1, 4);
    std::cout << maximum.query(3) << '\n'; // 7
    auto [k, m] = maximum.query_line(3);
    std::cout << k << ' ' << m << '\n'; // 2 1
    const auto result = maximum.query_with_line(3);
    std::cout << result.value << ' ' << result.k << ' ' << result.m << '\n'; // 7 2 1
    std::cout << maximum.query_line_optional(3).has_value() << '\n'; // 1
    std::cout << maximum.size() << ' ' << maximum.empty() << '\n'; // 2 0

    MinLineContainer<std::int64_t> minimum;
    minimum.add(2, 1);
    minimum.add(-1, 4);
    std::cout << minimum.query(3) << '\n'; // 1
    auto copy = minimum; // 係数は最小化でも元の符号のまま。
    minimum.clear();
    std::cout << minimum.empty() << ' ' << copy.query(3) << '\n'; // 1 1
    std::cout << minimum.query_with_line_optional(3).has_value() << '\n'; // 0
    std::cout << copy.query_with_line_optional(3)->value << '\n'; // 1
    for (const auto& line : copy)
        std::cout << line.k << ' ' << line.m << '\n'; // 傾きの降順。
    std::cout << lc_div<std::int64_t>(-5, 2) << '\n'; // -3
    std::cout << (lc_inf<std::int64_t>() > 0) << '\n'; // 1
}
