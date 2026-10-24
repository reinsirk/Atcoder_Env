// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <string>

int main() {
    int n{}; if (!(verify_in >> n)) return 0;
    HashSet<std::string> values;
    while (n--) {
        std::string key; verify_in >> key;
        values.insert(key);
    }
    verify_out << values.size() << '\n';
}
