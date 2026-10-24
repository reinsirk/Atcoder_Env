// SPDX-License-Identifier: CC0-1.0
// 検査用リンク引数: -Wl,--wrap=malloc,--wrap=realloc,--wrap=free
#ifndef LI_CHAO_SOURCE
#define LI_CHAO_SOURCE "../../../DataStructure/dynamic-li-chao-tree.cpp"
#endif
#include LI_CHAO_SOURCE
#include <array>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <source_location>

namespace {
std::int64_t budget=-1;
std::size_t checks=0,failures=0;
struct Allocation{void* pointer{};std::size_t bytes{};};
std::array<Allocation,1024> live{};
bool denied(){if(budget==0)return true;if(budget>0)--budget;return false;}
std::size_t find(void* p){for(std::size_t i=0;i<live.size();++i)if(live[i].pointer==p)return i;return live.size();}
void store(void* p,std::size_t n){if(!p)return;for(auto& r:live)if(!r.pointer){r={p,n};return;}std::abort();}
std::size_t active(){std::size_t n=0;for(auto& a:live)n+=a.pointer!=nullptr;return n;}
void require(bool b,std::source_location loc=std::source_location::current()){++checks;if(!b){std::fprintf(stderr,"failed:%u\n",loc.line());std::abort();}}
template<class F>void fail(F f){bool caught=false;budget=0;try{f();}catch(const std::bad_alloc&){caught=true;}budget=-1;require(caught);++failures;}
}
extern "C" void* __real_malloc(std::size_t);
extern "C" void* __real_realloc(void*,std::size_t);
extern "C" void __real_free(void*);
extern "C" void* __wrap_malloc(std::size_t n){if(denied())return nullptr;void* p=__real_malloc(n);store(p,n);return p;}
extern "C" void* __wrap_realloc(void* old,std::size_t n){require(n!=0);if(denied())return nullptr;auto i=old?find(old):live.size();void* p=__real_realloc(old,n);if(p){if(i!=live.size())live[i]={p,n};else store(p,n);}return p;}
extern "C" void __wrap_free(void* p){if(p){auto i=find(p);if(i!=live.size())live[i]={};}__real_free(p);}
[[gnu::noinline]] void* operator new(std::size_t n){if(denied())throw std::bad_alloc();if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
[[gnu::noinline]] void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
using I=std::int64_t;
using Tree=DynamicLiChaoTree<I,std::numeric_limits<I>::max()>;
[[gnu::noinline]] static void observe(const Tree& tree) {
  volatile I value = tree.query(0);
  (void)value;
}
int main(){
 const auto before=active();
 {
  Tree full(-9,11);full.reserve(1);full.add_line(0,7);fail([&]{full.reserve(2);});require(full.stored_nodes()==1&&full.query(11)==7);full.reserve(2);require(full.query(-9)==7);
  Tree source(-1'000'000'000,1'000'000'000);source.reserve(32768);
  std::array<I,256> positions{};for(std::size_t i=0;i<positions.size();++i)positions[i]=I(i)*100003-19000000;
  budget=0;for(std::size_t i=0;i<positions.size();++i)source.add_segment(0,I(i)+1,positions[i],positions[i]);budget=-1;
  const auto nodes=source.stored_nodes();require(nodes>1&&nodes<32768);fail([&]{source.reserve(65536);});require(source.stored_nodes()==nodes);
  budget=0;for(std::size_t i=0;i<positions.size();++i){require(source.query(positions[i])==I(i)+1);const auto l=source.query_line(positions[i]);require(l&&l->get(positions[i])==I(i)+1);}budget=-1;
  Tree destination(-3,7);destination.add_line(0,13);const auto live_before=active();fail([&]{destination=source;});require(active()==live_before);require(destination.domain_min()==-3&&destination.domain_max()==7);require(destination.stored_nodes()==1&&destination.query(0)==13);
  fail([&]{Tree copy(source);observe(copy);});require(active()==live_before);
  destination.reserve(32768);budget=0;destination=source;budget=-1;require(destination.domain_min()==source.domain_min()&&destination.domain_max()==source.domain_max());require(destination.stored_nodes()==nodes);
  budget=0;destination=destination;budget=-1;
  for(std::size_t i=0;i<positions.size();++i)require(destination.query(positions[i])==I(i)+1);
  budget=0;Tree moved(std::move(destination));budget=-1;require(destination.empty()&&destination.stored_nodes()==0);require(moved.stored_nodes()==nodes);fail([&]{destination.add_line(0,-7);});require(destination.empty());destination.add_line(0,-7);require(destination.query(positions[0])==-7&&moved.query(positions[0])==1);
  Tree assigned(0,0);assigned.add_line(0,9);budget=0;assigned=std::move(moved);budget=-1;require(moved.empty()&&moved.stored_nodes()==0);require(assigned.stored_nodes()==nodes);require(assigned.query(positions[255])==256);
  budget=0;source.clear();require(source.empty()&&source.query(positions[0])==std::numeric_limits<I>::max());source.reserve(32768);for(std::size_t i=0;i<positions.size();++i)source.add_segment(0,-I(i)-1,positions[i],positions[i]);budget=-1;require(source.stored_nodes()==nodes);
  for(std::size_t i=0;i<positions.size();++i)require(source.query(positions[i])==-I(i)-1);
  Tree empty(-4,8);empty.reserve(32768);budget=0;Tree emptycopy(empty);require(emptycopy.empty()&&emptycopy.stored_nodes()==0&&emptycopy.domain_min()==-4);emptycopy=empty;budget=-1;
  bool length=false;try{source.reserve(std::numeric_limits<std::size_t>::max());}catch(const std::length_error&){length=true;}require(length);require(source.stored_nodes()==nodes);
 }
 require(active()==before);
 std::printf("{\"checks\":%zu,\"failure_points\":%zu,\"live_delta\":%zd,\"status\":\"PASS\"}\n",checks,failures,static_cast<std::ptrdiff_t>(active())-static_cast<std::ptrdiff_t>(before));
}
