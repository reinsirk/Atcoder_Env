// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"

int main() {
    int n, k; verify_in >> n >> k;
    Heap heap;
    for (int i = 0; i < n; ++i) {
        int value; verify_in >> value;
        heap.push(value);
        if (heap.size() > static_cast<std::size_t>(k)) (void)heap.pop_value();
        if (i + 1 >= k) verify_out << heap.top() << '\n';
    }
}
