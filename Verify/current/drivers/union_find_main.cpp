// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
int main() {
    int n,q; verify_in>>n>>q; UnionFind uf(n);
    while(q--){int t,u,v;verify_in>>t>>u>>v;if(t==0)uf.unite(u,v);else verify_out<<uf.same(u,v)<<'\n';}
}
