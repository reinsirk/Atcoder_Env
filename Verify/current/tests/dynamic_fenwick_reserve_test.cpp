// SPDX-License-Identifier: CC0-1.0
// 点を登録した後のreserve、確保失敗、再試行を独立map/setと照合する。
#ifndef FENWICK_SOURCE
#define FENWICK_SOURCE "../../../DataStructure/dynamic-binary-indexed-tree.cpp"
#endif
#include FENWICK_SOURCE
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <new>
#include <random>
#include <set>
#include <source_location>
static int remaining=-1;
static std::size_t allocation_calls=0,checks=0,failures=0,scenarios=0;
[[gnu::noinline]]void* operator new(std::size_t n){++allocation_calls;if(remaining==0)throw std::bad_alloc();if(remaining>0)--remaining;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
[[gnu::noinline]]void* operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]]static void release(void*p)noexcept{std::free(p);}
void operator delete(void*p)noexcept{release(p);}void operator delete[](void*p)noexcept{release(p);}
void operator delete(void*p,std::size_t)noexcept{release(p);}void operator delete[](void*p,std::size_t)noexcept{release(p);}
using I=std::int64_t;using U=std::uint64_t;
static std::mt19937_64 rng(UINT64_C(0x350f1e2a));
static void require(bool b,std::source_location loc=std::source_location::current()){++checks;if(!b){std::fprintf(stderr,"failed line %u\n",loc.line());std::abort();}}
template<class S>struct Reference {
 U n;DynamicFenwickTree<S,I> bit;std::map<U,I> values;std::set<U> cells;
 explicit Reference(U width):n(width),bit(static_cast<S>(width)){}
 void register_cells(U k){
  for(U c=k+1;c<=n;){cells.insert(c);const U step=c&(U{0}-c);if(step>n-c)break;c+=step;}
 }
 void add(U k,I value){bit.add(static_cast<S>(k),value);values[k]+=value;register_cells(k);require(bit.stored_nodes()==cells.size());}
 void set(U k,I value){bit.set(static_cast<S>(k),value);values[k]=value;register_cells(k);require(bit.stored_nodes()==cells.size());}
 I sum(U l,U r)const{I total=0;for(auto[k,v]:values)if(l<=k&&k<r)total+=v;return total;}
 U bound(I w,bool upper)const{
  if(upper?w<0:w<=0)return 0;
  I total=0;for(auto[k,v]:values){total+=v;if(upper?total>w:total>=w)return k;}return n;
 }
 void verify()const{
  const I total=sum(0,n);remaining=0;
  require(static_cast<U>(bit.size())==n&&bit.stored_nodes()==cells.size());
  require(bit.sum_all()==total&&bit.sum(static_cast<S>(n))==total);
  require(bit.sum(S{0},static_cast<S>(n))==total&&bit.sum(static_cast<S>(n),static_cast<S>(n))==0);
  for(auto[k,v]:values)require(bit.get(static_cast<S>(k))==v&&bit[static_cast<S>(k)]==v);
  for(int rep=0;rep<7;++rep){
   const auto endpoint=[&]{return n==std::numeric_limits<U>::max()?rng():rng()%(n+1);};
   U l=endpoint(),r=endpoint();if(l>r)std::swap(l,r);
   require(bit.sum(static_cast<S>(l),static_cast<S>(r))==sum(l,r));
   require(bit.sum(static_cast<S>(r))==sum(0,r));
  }
  for(I w:std::array<I,6>{-1,0,1,total/2,total,total+1}){
   require(static_cast<U>(bit.lower_bound(w))==bound(w,false));
   require(static_cast<U>(bit.upper_bound(w))==bound(w,true));
  }
  require(bit.stored_nodes()==cells.size());remaining=-1;
 }
 void clear(){bit.clear();values.clear();cells.clear();}
 void reset(U width){bit.reset(static_cast<S>(width));n=width;values.clear();cells.clear();}
};
template<class S>static Reference<S> populated(U n,unsigned count){
 Reference<S> state(n);
 for(unsigned t=0,inserted=0;inserted<count;++t){
  const U k=(U{1009}*t+7)%n;if(state.values.contains(k))continue;
  const I value=inserted%3?I(inserted%9)+1:0;
  state.add(k,value);
  if(inserted%4==0)state.add(k,-value); // 相殺後もセル登録は残る。
  ++inserted;
 }
 state.verify();return state;
}
template<class S>static void exercise(U n,unsigned count,std::size_t hint){
 unsigned local_failures=0;bool succeeded=false;
 for(int allowed=0;allowed<16;++allowed){
  auto state=populated<S>(n,count);const auto nodes=state.bit.stored_nodes();
  bool threw=false;remaining=allowed;
  try{state.bit.reserve(hint);}catch(const std::bad_alloc&){threw=true;}
  remaining=-1;
  require(state.bit.stored_nodes()==nodes);state.verify();
  if(threw){++failures;++local_failures;state.bit.reserve(hint);state.verify();}
  else succeeded=true;
  // 移行先の索引を、既存列・新列・setの双方から使い直す。
  if(!state.values.empty())state.add(state.values.begin()->first,3);
  U fresh=0;while(state.values.contains(fresh))++fresh;
  state.add(fresh,5);state.set(fresh,2);state.verify();
  state.clear();state.verify();state.bit.reserve(hint);state.verify();
  state.add(n-1,7);state.verify();
  state.reset(n==8191?8192:8191);state.verify();state.add(state.n-1,11);state.verify();
  state.reset(n);state.verify();state.add(n-1,13);state.verify();
  if(succeeded)break;
 }
 require(succeeded);++scenarios;
 std::printf("reserve n=%zu points=%u hint=%zu failures=%u\n",static_cast<std::size_t>(n),count,hint,local_failures);
}
int main(){
 // 列幅と点数でSmall/Flat/Tree/Hashの移行境界を通す。内部の確保回数は固定しない。
 for(unsigned count:{0u,1u,4u,5u,31u,32u}){
  exercise<int>(2000003,count,16);exercise<int>(2000003,count,64);exercise<int>(2000003,count,1000);
 }
 for(unsigned count:{33u,64u,128u}){
  exercise<int>(2000003,count,128);exercise<int>(2000003,count,1000);
 }
 for(unsigned count:{129u,257u,1000u})exercise<int>(2000003,count,10000);
 for(unsigned count:{1u,5u,32u,33u,65u})exercise<int>(8191,count,4096);
 exercise<std::uint64_t>(std::numeric_limits<std::uint64_t>::max(),4,1024);
 require(failures>0);
 std::printf("{\"scenarios\":%zu,\"allocation_failures\":%zu,\"checks\":%zu,\"status\":\"pass\"}\n",scenarios,failures,checks);
}
