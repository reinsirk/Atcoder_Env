#include <cassert>
#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include "../DataStructure/Hash_set.cpp"

int main() {
    HashSet<std::int64_t> values{3, 1, 3};
    assert(values.size() == 2 && values.contains(1) && values.contain(3));
    assert(values.count(2) == 0 && *values.insert(2) == 2);
    assert(!values.insert_checked(2).second && values.insert_checked(4).second);
    assert(values.emplace(5).second);
    const std::vector<std::int64_t> more{6, 7, 7};
    values.insert_range(more.begin(), more.end());
    values.reserve(100);
    values.rehash(32);
    assert(values.capacity() >= 32 && values.load_factor() <= 0.5);
    const auto old_capacity = values.capacity();
    assert(values.erase_if([](std::int64_t x) { return x % 2 == 0; }) == 3);
    for (auto it = values.begin(); it != values.end();) it = values.erase(it);
    assert(values.empty() && values.capacity() == old_capacity);
    values.insert(8);
    values.erase(values.find(8), false);
    values.insert(10);
    values.erase(10, false);
    assert(!values.contains(10));
    assert(values.erase(99) == values.end() && values.erase(values.end()) == values.end());

    HashSet<std::string> words{"alpha", "beta"};
    auto copy = words;
    copy.erase("alpha");
    assert(words.contains("alpha") && copy != words);
    auto moved = std::move(copy);
    assert(moved.contains("beta") && copy.empty());
    copy.insert("reused");
    swap(copy, moved);
    assert(copy.contains("beta") && moved.contains("reused"));
    const auto& constant_words = words;
    assert(*constant_words.find("beta") == "beta");
    auto last = words.cend();
    --last;
    assert(words.contains(*last));
    words.clear();
    assert(words.empty());

    HashSet<std::pair<int, int>> coordinates;
    coordinates.emplace(2, 3);
    assert(coordinates.contains({2, 3}));
    HashSet<std::tuple<int, std::string>> states;
    states.emplace(1, "ready");
    assert(states.contains({1, "ready"}));
    HashSet<std::vector<int>> sequences;
    sequences.insert({1, 2, 3});
    assert(sequences.contains({1, 2, 3}));
    HashSet<int> from_range(more.begin(), more.end());
    assert(from_range.size() == 2);
}
