// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <atcoder/modint>
int main() {
    int t{}; std::uint32_t mod{};verify_in>>t>>mod;
    using Mint=atcoder::dynamic_modint<0>; Mint::set_mod(static_cast<int>(mod)); PrimeBinomial<Mint> comb(9'999'999);
    while(t--) { std::int64_t n{},k{};verify_in>>n>>k;verify_out<<comb.COM(n,k).val()<<'\n'; }
}
