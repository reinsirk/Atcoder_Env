// SPDX-License-Identifier: CC0-1.0
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
int main(){
    using UF=RollbackWeightedUnionFind<std::int64_t>;using R=UF::MergeResult;
    auto check=[](bool ok){if(!ok)throw std::runtime_error("weighted rollback mismatch");};
    UF uf(6);check(uf.unite_checked(0,1,7)==R::Merged);check(uf.unite_checked(1,2,-3)==R::Merged);
    check(uf.diff(0,2)==4);uf.snapshot();int state=uf.get_state();
    check(uf.unite_checked(0,2,4)==R::AlreadyConsistent);check(uf.unite_checked(0,2,5)==R::Contradiction);
    check(uf.get_state()==state+2);uf.undo();check(uf.get_state()==state+1);uf.rollback();check(uf.get_state()==state);
    check(uf.unite(3,4,11));check(uf.unite(2,3,9));check(uf.diff(0,4)==24);check(!uf.try_diff(0,5));
    auto copy=uf;uf.rollback(state);check(!uf.same(0,4));check(copy.diff(0,4)==24);copy.rollback(0);check(copy.component_count()==6);
    uf.reset(3);const auto mx=std::numeric_limits<std::int64_t>::max();
    check(uf.unite(0,1,mx));check(uf.unite(0,2,mx-1));check(uf.diff(1,2)==-1);
    check(uf.consistent(1,2,-1));check(!uf.consistent(1,2,1));
    uf.clear();check(uf.empty());std::cout<<"OK\n";
}
