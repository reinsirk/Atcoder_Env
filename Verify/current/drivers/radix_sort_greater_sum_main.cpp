// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

int main() {
    int n{}; verify_in >> n;
    std::vector<std::int64_t> values(n);
    for (auto& value : values) verify_in >> value;
    auto sorted = values;
    RadixSort(sorted);
    std::vector<std::int64_t> prefix(n + 1);
    for (int i = 0; i < n; ++i) prefix[i + 1] = prefix[i] + sorted[i];
    for (int i = 0; i < n; ++i) {
        const auto position = std::upper_bound(sorted.begin(), sorted.end(), values[i]) - sorted.begin();
        if (i) verify_out << ' ';
        verify_out << prefix[n] - prefix[static_cast<std::size_t>(position)];
    }
    verify_out << '\n';
}
