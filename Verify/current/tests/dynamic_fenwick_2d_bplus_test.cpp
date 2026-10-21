// SPDX-License-Identifier: CC0-1.0
// 実行例: c++ -std=c++23 -O2 Verify/current/tests/dynamic_fenwick_2d_bplus_test.cpp -o /tmp/fenwick_test && /tmp/fenwick_test
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree-2d.cpp"
#endif
// B+分割の確保失敗・所有権・厳密な内部セル数を確認する。
#include <initializer_list>
#include <utility>
#include <type_traits>
#include <iterator>
#include <cstdlib>
#include <cstdio>
#include <new>
#include <cstdint>
#include <cstddef>
#include <climits>
#include <map>
#include <set>
#include <random>
static int fail_after=-1;
void* operator new(std::size_t n){if(fail_after==0)throw std::bad_alloc();if(fail_after>0)--fail_after;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}
void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}
void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
#include FENWICK_SOURCE
using I=std::int64_t;using U=std::uint32_t;using Row=reinsirk::detail::BPlusFenwickRow<I>;
static std::uint64_t assertions=0;static void check(bool x){++assertions;if(!x){std::fprintf(stderr,"assertion %ju failed\n",std::uintmax_t(assertions));std::abort();}}
static I reference(const std::map<U,I>& vals,U l,U r){I s=0;for(auto it=vals.upper_bound(l);it!=vals.end()&&it->first<=r;++it)s+=it->second;return s;}
static void verify(const Row& row,const std::map<U,I>& vals,const std::set<U>& cells,std::mt19937_64& rng){
 check(row.size()==cells.size());
 for(int j=0;j<12;++j){U a=U(rng()%INT_MAX),b=U(rng()%INT_MAX);if(a>b)std::swap(a,b);const I expected=reference(vals,a,b);fail_after=0;check(row.sum(a,b)==expected);fail_after=-1;}
 for(auto it=vals.begin();it!=vals.end();){const U col=it->first;const I expected=it->second;fail_after=0;check(row.sum(col-1,col)==expected);fail_after=-1;const auto left=std::distance(it,vals.end());std::advance(it,std::min<std::ptrdiff_t>(left,97));}
}
int main(){
 std::mt19937_64 rng(907213);std::uint64_t failures=0;
 for(int mode=0;mode<3;++mode){Row row;std::map<U,I>vals;std::set<U>cells;
  for(U k=0;k<12000;++k){const U col=mode==0?k*131+1:mode==1?INT_MAX-k*131:U(rng()%INT_MAX)+1;const I delta=I(rng()%101)-50;
   fail_after=0;bool failed=false;try{row.add(col,delta,INT_MAX);}catch(const std::bad_alloc&){failed=true;}fail_after=-1;
   if(failed){++failures;verify(row,vals,cells,rng);row.add(col,delta,INT_MAX);}
   vals[col]+=delta;for(std::uint64_t c=col;c<=INT_MAX;c+=c&(0-c))cells.insert(U(c));
   if(k%317==0)verify(row,vals,cells,rng);
  }
  verify(row,vals,cells,rng);Row copy=row;verify(copy,vals,cells,rng);
  fail_after=0;Row moved(std::move(copy));check(copy.empty());Row target;target.swap(moved);check(moved.empty());fail_after=-1;verify(target,vals,cells,rng);
  row.clear();vals.clear();cells.clear();check(row.empty());row.add(1,3,INT_MAX);check(row.sum(0,1)==3);
 }
 check(failures>0);std::printf("{\"assertions\":%ju,\"split_allocation_failures\":%ju,\"status\":\"pass\"}\n",std::uintmax_t(assertions),std::uintmax_t(failures));
}
