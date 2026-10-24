// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RUF_REQUIRE_EXTRA
#define SIRKIT_RUF_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_RUF_SOURCE
#define SIRKIT_RUF_SOURCE "../../../DataStructure/rollback-union-find.cpp"
#endif
#include SIRKIT_RUF_SOURCE
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>
static std::ptrdiff_t countdown=-1;
static std::size_t live=0,calls=0,checks=0,failures=0;
[[gnu::noinline]] void* operator new(std::size_t size){if(countdown==0)throw std::bad_alloc();if(countdown>0)--countdown;if(void*p=std::malloc(size?size:1)){++live;++calls;return p;}throw std::bad_alloc();}
[[gnu::noinline]] void* operator new[](std::size_t size){return ::operator new(size);}
[[gnu::noinline]] static void release(void*p)noexcept{if(p)--live;std::free(p);}
void operator delete(void*p)noexcept{release(p);}void operator delete[](void*p)noexcept{release(p);}void operator delete(void*p,std::size_t)noexcept{release(p);}void operator delete[](void*p,std::size_t)noexcept{release(p);}
static void require_impl(bool ok,const char*what,int line){++checks;if(!ok){std::fprintf(stderr,"line %d: %s\n",line,what);std::abort();}}
#define require(x) require_impl(bool(x),#x,__LINE__)
struct Snapshot{int n,state,count,default_state;std::vector<int>roots,sizes;std::vector<std::vector<int>>groups,default_groups;};
static Snapshot capture(const RollbackUnionFind&uf){Snapshot x{uf.size(),uf.get_state(),uf.component_count(),0,{},{},{},{}};for(int i=0;i<uf.size();++i){x.roots.push_back(uf.find(i));x.sizes.push_back(uf.size(i));}x.groups=uf.groups();auto copy=uf;copy.rollback();x.default_state=copy.get_state();x.default_groups=copy.groups();return x;}
static void unchanged(const RollbackUnionFind&uf,const Snapshot&x){const auto y=capture(uf);require(y.n==x.n&&y.state==x.state&&y.count==x.count);require(y.roots==x.roots&&y.sizes==x.sizes&&y.groups==x.groups);require(y.default_state==x.default_state&&y.default_groups==x.default_groups);}
static RollbackUnionFind prepared(int n=129){RollbackUnionFind uf(static_cast<std::size_t>(n));for(int i=1;i<n/2;++i)uf.unite(i-1,i);uf.snapshot();for(int i=n/2+1;i<n;++i)uf.unite(i-1,i);return uf;}
static int growth_state(){RollbackUnionFind uf(1024);for(int i=0;i<1023;++i){const auto old=calls;uf.unite(0,i+1);if(calls!=old)return i;}return -1;}
static void constructor_fail(){bool success=false;for(int k=0;k<32;++k){const auto old=live;bool failed=false;countdown=k;try{RollbackUnionFind uf(129);countdown=-1;require(uf.component_count()==129);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}require(live==old);if(!failed){success=true;break;}}require(success);}
static void copy_constructor_fail(){auto source=prepared();const auto before=capture(source);bool success=false;for(int k=0;k<32;++k){const auto old=live;bool failed=false;countdown=k;try{RollbackUnionFind copy(source);countdown=-1;unchanged(copy,before);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}require(live==old);unchanged(source,before);if(!failed){success=true;break;}}require(success);}
static void assignment_fail(){auto source=prepared();const auto src=capture(source);bool success=false;for(int k=0;k<32;++k){auto target=prepared(17);const auto before=capture(target);const auto old=live;bool failed=false;countdown=k;try{target=source;countdown=-1;}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}unchanged(source,src);if(failed){unchanged(target,before);require(live==old);}else{unchanged(target,src);success=true;break;}}require(success);}
static void reserve_fail(){
 bool allocates=false;{auto probe=prepared();const auto old=calls;probe.reserve_history(10000);allocates=calls!=old;}
 auto uf=prepared();const auto before=capture(uf);const auto old_live=live,old_calls=calls;countdown=0;bool threw=false;
 try{uf.reserve_history(10000);}catch(const std::bad_alloc&){threw=true;++failures;}
 countdown=-1;require(threw==allocates);require(live==old_live);if(!allocates)require(calls==old_calls);unchanged(uf,before);uf.reserve_history(10000);unchanged(uf,before);
}
static void unite_fail(){
 const int growth=growth_state();
 if(growth<0){
  RollbackUnionFind uf(1024);uf.unite(0,1);uf.snapshot();const auto before=capture(uf);const auto old=calls;countdown=0;require(uf.merge(1,2));uf.undo();countdown=-1;require(calls==old);unchanged(uf,before);return;
 }
 for(int merge_case=0;merge_case<2;++merge_case){
  auto make_boundary=[&](){RollbackUnionFind value(1024);for(int i=0;i<growth;++i)value.unite(0,i+1);value.snapshot();return value;};
  bool allocates=false;
  {auto probe=make_boundary();const auto old=calls;(void)(merge_case?probe.merge(0,growth+1):probe.unite(0,0));allocates=calls!=old;}
  auto uf=make_boundary();const auto before=capture(uf);const auto old_live=live,old_calls=calls;countdown=0;bool threw=false;bool result=false;
  try{result=merge_case?uf.merge(0,growth+1):uf.unite(0,0);}catch(const std::bad_alloc&){threw=true;++failures;}
  countdown=-1;require(threw==allocates);
  if(threw){require(live==old_live);unchanged(uf,before);require((merge_case?uf.merge(0,growth+1):uf.unite(0,0))==bool(merge_case));}
  else{require(calls==old_calls);require(result==bool(merge_case));}
  require(uf.get_state()==growth+1);uf.undo();unchanged(uf,before);
 }
}
static void groups_fail(){auto uf=prepared(19);const auto before=capture(uf);bool success=false;for(int k=0;k<128;++k){const auto old=live;bool failed=false;countdown=k;try{auto groups=uf.groups();countdown=-1;require(groups==before.groups);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}require(live==old);unchanged(uf,before);if(!failed){success=true;break;}}require(success);}
static void nonallocating(){RollbackUnionFind uf(64);uf.reserve_history(1000);auto copy=uf;copy.reserve_history(1000);const auto old=calls;countdown=0;for(int i=1;i<64;++i)uf.unite(i-1,i);for(int i=0;i<64;++i){require(uf.find(i)==uf.find(0));require(uf.size(i)==64);require(uf.same(0,i));}uf.snapshot();uf.unite(0,1);uf.undo();uf.rollback(0);uf.swap(copy);uf.snapshot();uf.rollback();RollbackUnionFind moved(std::move(uf));uf=std::move(moved);countdown=-1;require(calls==old);
#ifdef SIRKIT_RUF_REQUIRE_EXTRA
 uf.reserve_history(512);for(int i=0;i<400;++i)uf.unite(i%64,(i+1)%64);uf.snapshot();const int components=uf.component_count();const int component_size=uf.size(0);const auto old_calls=calls;countdown=0;uf.clear_history();require(uf.get_state()==0&&uf.component_count()==components&&uf.size(0)==component_size);uf.rollback();for(int i=0;i<400;++i)uf.unite(i%64,(i+1)%64);for(int i=0;i<400;++i)uf.undo();countdown=-1;require(calls==old_calls);require(uf.get_state()==0&&uf.component_count()==components);static_assert(noexcept(std::declval<RollbackUnionFind&>().clear_history()));
#endif
}
int main(){const auto old=live;constructor_fail();copy_constructor_fail();assignment_fail();reserve_fail();unite_fail();groups_fail();nonallocating();require(live==old);std::printf("PASS checks=%zu allocation_failures=%zu live_delta=%td\n",checks,failures,static_cast<std::ptrdiff_t>(live)-static_cast<std::ptrdiff_t>(old));}
