// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cstdint>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

int main() {
    auto require = [](bool value) { if (!value) throw std::runtime_error("HashSet verification failed"); };
    std::mt19937_64 random(20261019);
    HashSet<int> values;
    std::set<int> reference;
    values.reserve(256);
    for (int step = 0; step < 5000; ++step) {
        int key = static_cast<int>(random() % 257) - 128;
        if (random() & 1) {
            auto [it, added] = values.insert_checked(key);
            auto [expected_it, expected_added] = reference.insert(key);
            require(added == expected_added && *it == *expected_it);
        } else {
            values.erase(key, false); reference.erase(key);
        }
        require(values.size() == reference.size() && values.empty() == reference.empty());
        require(values.contains(key) == reference.contains(key));
        require(values.count(key) == static_cast<int>(reference.count(key)));
        std::set<int> enumerated(values.begin(), values.end()); require(enumerated == reference);
        const auto& const_values = values;
        require((const_values.find(key) != const_values.end()) == reference.contains(key));
    }
    auto copy = values;
    values.clear(); require(values.empty());
    std::set<int> enumerated(copy.begin(), copy.end()); require(enumerated == reference);
    auto moved = std::move(copy); require(copy.empty());
    require(std::set<int>(moved.begin(), moved.end()) == reference);
    auto removed = moved.erase_if([](int x) { return x < 0; });
    auto expected_removed = std::erase_if(reference, [](int x) { return x < 0; });
    require(removed == expected_removed && std::set<int>(moved.begin(), moved.end()) == reference);
    HashSet<std::pair<int, int>> pairs;
    pairs.insert({-1, 2}); pairs.insert({-1, 2}); pairs.insert({2, -1});
    require(pairs.size() == 2 && pairs.contains({-1, 2}));
    HashSet<std::string> strings;
    strings.insert(""); strings.insert("abc"); strings.insert(std::string("a\0b", 3));
    require(strings.size() == 3 && strings.contains(std::string("a\0b", 3)));
    HashSet<std::vector<int>> vectors;
    vectors.insert({1, 2}); vectors.insert({2, 1}); vectors.insert({});
    require(vectors.size() == 3 && vectors.contains({1, 2}) && vectors.contains({}));
}
