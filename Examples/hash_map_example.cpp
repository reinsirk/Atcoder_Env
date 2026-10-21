#include "../DataStructure/Hash_map.cpp"
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

struct Point {
    int x, y;
    bool operator==(const Point&) const = default;
};
struct PointHash {
    GenericHash hash;
    std::size_t operator()(const Point& p) const { return hash(std::pair{p.x, p.y}); }
};

int main() {
    HashMap<std::string, int> scores;
    scores.reserve(10);
    auto [it, added] = scores.try_emplace("alice", 3);
    std::cout << added << ' ' << it->second << '\n'; // 1 3
    scores.insert_or_assign("alice", 8);
    scores["bob"] = 5;
    const auto& view = scores;
    std::cout << view.at("alice") << ' ' << view.contains("bob") << '\n'; // 8 1
    for (auto p = scores.begin(); p != scores.end();) p = scores.erase(p);
    std::cout << scores.empty() << '\n'; // 1

    using Key = std::tuple<std::string, std::vector<int>, std::array<int, 2>>;
    HashMap<Key, int> nested;
    Key key{"path", {1, 2, 3}, {4, 5}};
    nested[key] = 7;
    std::cout << nested.at(key) << '\n'; // 7
    HashMap<Point, int, PointHash> points;
    points[{2, 3}] = 9;
    std::cout << points.at({2, 3}) << '\n'; // 9
    // ハッシュ関数だけを標準コンテナでも使える。
    std::unordered_map<std::vector<int>, int, GenericHash> standard;
    standard[{1, 2}] = 6;
    std::cout << standard.at({1, 2}) << '\n'; // 6
}
