// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>

int main() {
    int n{}; if (!(verify_in >> n)) return 0;
    HashSet<std::int64_t> values;
    while (n--) {
        std::int64_t key{}; verify_in >> key;
        auto it = values.find(key);
        if (it == values.end()) values.insert(key);
        else values.erase(it, false);
    }
    verify_out << values.size() << '\n';
}
