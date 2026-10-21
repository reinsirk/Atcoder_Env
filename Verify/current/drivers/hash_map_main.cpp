// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>

int main() {
    int q{}; if (!(verify_in >> q)) return 0;
    HashMap<std::uint64_t, std::uint64_t> table;
    while (q--) {
        int t{}; std::uint64_t key{}; verify_in >> t >> key;
        if (t == 0) { std::uint64_t value{}; verify_in >> value; table[key] = value; }
        else verify_out << table[key] << '\n';
    }
}
