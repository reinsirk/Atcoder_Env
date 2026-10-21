// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>

int main() {
    int n{}, q{}; if (!(verify_in >> n >> q)) return 0;
    MinLineContainer<std::int64_t> lines;
    while (n--) { std::int64_t a{}, b{}; verify_in >> a >> b; lines.add(a, b); }
    while (q--) {
        int t{}; verify_in >> t;
        if (t == 0) { std::int64_t a{}, b{}; verify_in >> a >> b; lines.add(a, b); }
        else { std::int64_t x{}; verify_in >> x; verify_out << lines.query(x) << '\n'; }
    }
}
