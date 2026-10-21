// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <string>
#include <atcoder/modint>
int main() {
    int n{};std::string s;verify_in>>n>>s;
    using Mint=atcoder::static_modint<3>;PrimeBinomial<Mint> comb(2);Mint answer=0;
    const std::string colors="BWR";
    for(int i=0;i<n;++i) answer+=comb.Lucas(n-1,i)*static_cast<int>(colors.find(s[i]));
    if((n-1)%2) answer=-answer;
    verify_out<<colors[answer.val()]<<'\n';
}
