// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <string>

int main() {
    int n{}; verify_in >> n;
    Trie dictionary;
    int answer = 0;
    while (n--) {
        std::string word; verify_in >> word;
        if (!dictionary.contains(word)) {
            dictionary.insert(word);
            ++answer;
        }
    }
    verify_out << answer << '\n';
}
