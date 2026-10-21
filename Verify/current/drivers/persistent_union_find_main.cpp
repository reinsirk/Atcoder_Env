// SPDX-License-Identifier: CC0-1.0
#include "Verify/current/common/fast_io.hpp"
#include <vector>
int main() {
    int n,q;verify_in>>n>>q;PersistentUnionFind uf(n);
    std::vector<PersistentUnionFind::Node*> versions(q+1);versions[0]=uf.get_root();
    for(int i=0;i<q;++i){int t,k,u,v;verify_in>>t>>k>>u>>v;
        if(t==0){uf.unite(u,v,versions[k+1]);versions[i+1]=uf.get_root();}
        else verify_out<<uf.same(u,v,versions[k+1])<<'\n';
    }
}
