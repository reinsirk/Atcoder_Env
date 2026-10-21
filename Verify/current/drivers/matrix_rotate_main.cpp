// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <vector>
int main(){
    int n{};verify_in>>n;std::vector<std::vector<int>>a(n,std::vector<int>(n)),b=a;
    for(auto&r:a)for(auto&x:r)verify_in>>x;
    for(auto&r:b)for(auto&x:r)verify_in>>x;
    for(int turns=0;turns<4;++turns){bool ok=true;for(int i=0;i<n;++i)for(int j=0;j<n;++j)if(a[i][j]&&!b[i][j])ok=false;
        if(ok){verify_out<<"Yes\n";return 0;}a=rotate_90r(a);
    }verify_out<<"No\n";
}
