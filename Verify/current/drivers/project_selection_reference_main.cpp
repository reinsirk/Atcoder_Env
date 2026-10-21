// SPDX-License-Identifier: CC0-1.0
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <limits>
#include <random>
#include <string>
#include <utility>
#include <vector>

int main() {
    std::mt19937 generator(1171234);
    for (int trial = 0; trial < 120; ++trial) {
        const int n = 1 + generator() % 7;
        TwoVariableSubmodularOpt<std::int64_t> model(n);
        std::vector<std::int64_t> energy(1 << n);
        auto verify = [&] {
            const auto [value, assignment] = model.solve_with_assignment();
            assert(value == *std::min_element(energy.begin(), energy.end()));
            int mask = 0; for (int i = 0; i < n; ++i) if (assignment[i]) mask |= 1 << i;
            assert(energy[mask] == value && model.solve() == value);
            auto copied = model; assert(copied.solve() == value);
        };
        for (int step = 0; step < 25; ++step) {
            int i = generator() % n, j = generator() % n;
            const std::int64_t a = static_cast<int>(generator() % 11) - 5;
            const std::int64_t b = static_cast<int>(generator() % 11) - 5;
            const std::int64_t d = static_cast<int>(generator() % 11) - 5;
            const std::int64_t c = a + d - b + generator() % 6;
            model.add_submodular_function(i, j, a, b, c, d);
            for (int mask = 0; mask < (1 << n); ++mask) {
                const bool x = (mask >> i) & 1, y = (mask >> j) & 1;
                energy[mask] += x ? (y ? d : c) : (y ? b : a);
            }
            std::vector<int> subset;
            for (int k = 0; k < n; ++k) if (generator() % 3 == 0) subset.push_back(k);
            const bool target = generator() & 1;
            const std::int64_t profit = generator() % 8;
            if (target) model.add_all_true_profit(subset, profit); else model.add_all_false_profit(subset, profit);
            for (int mask = 0; mask < (1 << n); ++mask) {
                bool match = true; for (int k : subset) match &= bool((mask >> k) & 1) == target;
                if (match) energy[mask] -= profit;
            }
            verify();
        }
        model.clear(); assert(model.solve() == 0);
        model.force(0, true); assert(model.reconstruct()[0]);
    }
    std::cout << "OK\n";
}
