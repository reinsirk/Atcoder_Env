#include "../DataStructure/line-container-2d.cpp"
#include <iostream>
#include <utility>

int main() {
    LineContainer2D points;
    std::cout << points.max_optional(1, 1).has_value() << '\n'; // 0
    points.add(1, 2);
    points.add(3, -1);
    std::cout << points.size() << ' ' << points.empty() << '\n'; // 2 0
    std::cout << points.max_ld(2, 1) << ' ' << points.min_ld(2, 1) << '\n'; // 5 4
    std::cout << points.max_ld(0.5L, -0.25L) << ' '
              << points.min_ld(0.5L, -0.25L) << '\n'; // 1.75 -0
    auto [x, y] = points.argmax(2, 1);
    std::cout << x << ' ' << y << '\n'; // 3 -1
    auto [u, v] = points.argmin(2, 1);
    std::cout << u << ' ' << v << '\n'; // 1 2
    std::cout << *points.max_optional(0, 1) << ' '
              << *points.min_optional(0, 1) << '\n'; // 2 -1
    std::cout << points.argmax_optional(2, 1)->first << ' '
              << points.argmin_optional(2, 1)->first << '\n'; // 3 1
    std::cout << points.max_ll(2, 1) << ' ' << points.min_ll(2, 1) << '\n'; // 5 4

    auto copy = points;
    auto moved = std::move(copy);
    std::cout << copy.empty() << ' ' << moved.max_ld(2, 1) << '\n'; // 1 5
    copy.swap(moved);
    std::cout << copy.size() << ' ' << moved.empty() << '\n'; // 2 1
    points.clear();
    std::cout << points.empty() << '\n'; // 1
    points.add(1.5L, -1.5L);
    std::cout << points.max_ll(1, 0) << ' ' << points.max_ll(0, 1) << '\n'; // 1 -1
    std::cout << points.max_rounded_ll(1, 0) << ' '
              << points.min_rounded_ll(0, 1) << '\n'; // 2 -2
}
