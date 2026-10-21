// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

int main() {
    std::string text; verify_in >> text;
    RollingHash hash(text, std::uint64_t{1000000007});
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i) verify_out << ' ';
        verify_out << hash.lcp(0, hash, i);
    }
    verify_out << '\n';
}
