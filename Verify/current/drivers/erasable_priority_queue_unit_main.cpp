// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>
#include <iterator>
#include <random>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

int main() {
    auto require = [](bool value) { if (!value) throw std::runtime_error("ErasablePriorityQueue verification failed"); };
    auto test = [&]<bool Minimize>() {
        ErasablePriorityQueue<int, Minimize> queue(std::vector<int>{2, 2, -3, 9});
        std::multiset<int> reference{-3, 2, 2, 9};
        std::mt19937_64 random(20261019);
        for (int step = 0; step < 10000; ++step) {
            const auto type = random() % 4;
            if (reference.empty() || type == 0) {
                const int value = static_cast<int>(random() % 101) - 50;
                if (step & 1) queue.emplace(value); else queue.push(value);
                reference.insert(value);
            } else if (type == 1) {
                auto it = reference.begin(); std::advance(it, random() % reference.size());
                queue.erase(*it); reference.erase(it);
            } else {
                auto it = Minimize ? reference.begin() : std::prev(reference.end());
                require(queue.top() == *it && queue.try_top() == *it);
                if (type == 2) queue.pop(); else require(queue.extract_top() == *it);
                reference.erase(it);
            }
            require(queue.size() == reference.size() && queue.empty() == reference.empty());
            if (reference.empty()) require(!queue.try_top());
            else require(queue.top() == (Minimize ? *reference.begin() : *reference.rbegin()));
        }
        auto copy = queue;
        queue.clear(); require(queue.empty() && !queue.extract_top());
        auto moved = std::move(copy); require(copy.empty());
        while (!reference.empty()) {
            auto it = Minimize ? reference.begin() : std::prev(reference.end());
            require(moved.extract_top() == *it); reference.erase(it);
        }
        require(moved.empty() && !moved.try_top());
    };
    test.template operator()<true>(); test.template operator()<false>();
    std::int64_t a, b; if (verify_in >> a >> b) verify_out << a + b << '\n';
}
