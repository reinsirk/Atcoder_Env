// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_UF_REQUIRE_EXTRA
#define SIRKIT_UF_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_UF_SOURCE
#define SIRKIT_UF_SOURCE "../../../DataStructure/union_find.cpp"
#endif
#include SIRKIT_UF_SOURCE
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

static volatile std::ptrdiff_t countdown=-1;
static volatile std::size_t live=0,calls=0;
static std::size_t checks=0,failures=0;
[[gnu::noinline]] void* operator new(std::size_t n){if(countdown==0)throw std::bad_alloc();if(countdown>0)countdown=countdown-1;if(void*p=std::malloc(n?n:1)){live=live+1;calls=calls+1;return p;}throw std::bad_alloc();}
[[gnu::noinline]] void* operator new[](std::size_t n){return ::operator new(n);}
[[gnu::noinline]] static void release(void*p)noexcept{if(p)live=live-1;std::free(p);}
void operator delete(void*p)noexcept{release(p);}void operator delete[](void*p)noexcept{release(p);}void operator delete(void*p,std::size_t)noexcept{release(p);}void operator delete[](void*p,std::size_t)noexcept{release(p);}
static void require_impl(bool ok,const char* what,int line){++checks;if(!ok){std::fprintf(stderr,"line %d: %s\n",line,what);std::abort();}}
#define require(x) require_impl(bool(x),#x,__LINE__)
struct Snapshot{int n,components;std::vector<int>roots,sizes;};
static Snapshot capture(UnionFind&uf){Snapshot out{uf.size(),uf.component_count(),{}, {}};for(int i=0;i<uf.size();++i){out.roots.push_back(uf.root(i));out.sizes.push_back(uf.size(i));}return out;}
static void unchanged(UnionFind&uf,const Snapshot&old){require(uf.size()==old.n);require(uf.component_count()==old.components);for(int i=0;i<old.n;++i){require(uf.root(i)==old.roots[i]);require(uf.size(i)==old.sizes[i]);for(int j=0;j<old.n;++j)require(uf.same(i,j)==(old.roots[i]==old.roots[j]));}}
static void prepare(UnionFind&uf){for(int i=0;i+3<uf.size();i+=4){uf.merge(i+3,i);uf.merge(i+2,i+1);uf.merge(i+2,i+3);}}
static void constructor_fail(){bool success=false;for(int k=0;k<16;++k){const auto before=live;bool failed=false;countdown=k;try{UnionFind uf(65);countdown=-1;require(uf.size()==65&&uf.component_count()==65);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}require(live==before);if(!failed){success=true;break;}}require(success);}
static void copy_constructor_fail(){UnionFind source(65);prepare(source);const auto src=capture(source);bool success=false;for(int k=0;k<16;++k){const auto before=live;bool failed=false;countdown=k;try{UnionFind copy(source);countdown=-1;unchanged(copy,src);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}unchanged(source,src);require(live==before);if(!failed){success=true;break;}}require(success);}
static void assignment_fail(){UnionFind source(65);prepare(source);const auto src=capture(source);bool success=false;for(int k=0;k<16;++k){UnionFind target(3);target.merge(0,2);const auto old=capture(target);const auto before=live;bool failed=false;countdown=k;try{target=source;countdown=-1;}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}unchanged(source,src);require(live==before);if(failed)unchanged(target,old);else{unchanged(target,src);success=true;break;}}require(success);}
static void groups_fail(){UnionFind source(65);prepare(source);const auto src=capture(source);const auto expected=source.groups();bool success=false;for(int k=0;k<128;++k){const auto before=live;bool failed=false;countdown=k;try{auto groups=source.groups();countdown=-1;require(groups==expected);}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}unchanged(source,src);require(live==before);if(!failed){success=true;break;}}require(success);}
static void nonallocating(){UnionFind a(128),b(128);const auto before=calls;countdown=0;for(int step=1;step<128;step*=2)for(int i=0;i<128;i+=2*step){require(a.merge(i,i+step));b.unite(i+step,i);}for(int i=0;i<128;++i){require(a.root(i)==0);require(a.leader(i)==0);require(a.same(i,127-i));require(a.issame(i,127-i));require(a.size(i)==128);require(!a.merge(i,0));}auto* alias=&a;a=*alias;a=std::move(*alias);UnionFind moved(std::move(a));a=std::move(moved);countdown=-1;require(calls==before);require(moved.size()==0&&moved.component_count()==0);require(a.size()==128&&a.component_count()==1);
#ifdef SIRKIT_UF_REQUIRE_EXTRA
 const auto old_calls=calls,old_live=live;static_assert(noexcept(std::declval<UnionFind&>().clear()));static_assert(noexcept(std::declval<UnionFind&>().swap(std::declval<UnionFind&>())));
 countdown=0;a.swap(b);swap(a,b);a.clear();require(a.empty());require(a.size()==0&&a.component_count()==0);a.clear();
 for(std::size_t n:{std::size_t{0},std::size_t{1},std::size_t{17},std::size_t{128},std::size_t{5}}){a.reset(n);require(a.size()==static_cast<int>(n));require(a.component_count()==static_cast<int>(n));for(int i=0;i<a.size();++i)require(a.root(i)==i&&a.size(i)==1);}
 countdown=-1;require(calls==old_calls&&live==old_live);
#endif
}
#ifdef SIRKIT_UF_REQUIRE_EXTRA
static void reset_fail(){bool success=false;for(int k=0;k<16;++k){UnionFind target(65);prepare(target);const auto old=capture(target);const auto before=live;bool failed=false;countdown=k;try{target.reset(1024);countdown=-1;}catch(const std::bad_alloc&){countdown=-1;failed=true;++failures;}require(live==before);if(failed)unchanged(target,old);else{require(target.size()==1024&&target.component_count()==1024);for(int i=0;i<target.size();++i)require(target.root(i)==i&&target.size(i)==1);success=true;break;}}require(success);}
#endif
int main(){const auto old_live=live;constructor_fail();copy_constructor_fail();assignment_fail();groups_fail();nonallocating();
#ifdef SIRKIT_UF_REQUIRE_EXTRA
 reset_fail();
#endif
 require(live==old_live);std::printf("PASS checks=%zu allocation_failures=%zu live_delta=%td\n",checks,failures,static_cast<std::ptrdiff_t>(live)-static_cast<std::ptrdiff_t>(old_live));
}
