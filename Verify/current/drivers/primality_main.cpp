// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
int main() {
    int q{};verify_in>>q;while(q--) { std::uint64_t n{};verify_in>>n;verify_out<<(is_prime(n)?"Yes":"No")<<'\n'; }
}
