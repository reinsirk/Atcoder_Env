// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <cstdint>

int main() {
    int q{};
    if (!(verify_in >> q)) return 0;
    CompressedBinaryTrie<std::uint32_t, 29> trie;
    while (q--) {
        int type{};
        std::uint32_t x{};
        verify_in >> type >> x;
        if (type == 0) trie.insert_unique(x);
        else if (type == 1) trie.del(x);
        else verify_out << trie.min_xor(x).first << '\n';
    }
}
