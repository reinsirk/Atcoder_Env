// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_RWUF_REQUIRE_EXTRA
#define SIRKIT_RWUF_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_RWUF_SOURCE
#define SIRKIT_RWUF_SOURCE "../../../DataStructure/rollback-union-findw.cpp"
#endif
#include SIRKIT_RWUF_SOURCE
#include <cstdio>
#include <cstdlib>
#include <new>
#include <type_traits>
static std::ptrdiff_t countdown=-1;
static std::size_t live=0,calls=0,checks=0,allocation_failures=0,value_failures=0;
[[gnu::noinline]]void*operator new(std::size_t size){if(countdown==0)throw std::bad_alloc();if(countdown>0)--countdown;if(void*p=std::malloc(size?size:1)){++live;++calls;return p;}throw std::bad_alloc();}
[[gnu::noinline]]void*operator new[](std::size_t size){return ::operator new(size);}
[[gnu::noinline]]static void release(void*p)noexcept{if(p)--live;std::free(p);}
void operator delete(void*p)noexcept{release(p);}void operator delete[](void*p)noexcept{release(p);}void operator delete(void*p,std::size_t)noexcept{release(p);}void operator delete[](void*p,std::size_t)noexcept{release(p);}
static void require_impl(bool ok,const char*what,int line){++checks;if(!ok){std::fprintf(stderr,"line %d: %s\n",line,what);std::abort();}}
#define require(x) require_impl(bool(x),#x,__LINE__)
using UF=RollbackWeightedUnionFind<std::int64_t>;
struct Snapshot{int n,state,count,default_state;std::vector<int>roots,sizes;std::vector<std::int64_t>weights;std::vector<std::vector<int>>groups,default_groups;};
static Snapshot capture(const UF&uf){Snapshot x{uf.size(),uf.get_state(),uf.component_count(),0,{},{},{},{},{}};for(int i=0;i<uf.size();++i){x.roots.push_back(uf.find(i));x.sizes.push_back(uf.size(i));x.weights.push_back(uf.get_weight(i));}x.groups=uf.groups();auto copy=uf;copy.rollback();x.default_state=copy.get_state();x.default_groups=copy.groups();return x;}
static void unchanged(const UF&uf,const Snapshot&x){const auto y=capture(uf);require(y.n==x.n&&y.state==x.state&&y.count==x.count);require(y.roots==x.roots&&y.sizes==x.sizes&&y.weights==x.weights&&y.groups==x.groups);require(y.default_state==x.default_state&&y.default_groups==x.default_groups);}
static UF prepared(int n=33){UF u(n);for(int i=1;i<n/2;++i)u.unite(i-1,i,3);u.snapshot();for(int i=n/2+1;i<n;++i)u.unite(i-1,i,-2);return u;}
static void constructors(){bool done=false;for(int k=0;k<32;++k){const auto old=live;bool failed=false;countdown=k;try{UF u(129);countdown=-1;require(u.size()==129);}catch(const std::bad_alloc&){countdown=-1;failed=true;++allocation_failures;}require(live==old);if(!failed){done=true;break;}}require(done);auto source=prepared();const auto snap=capture(source);done=false;for(int k=0;k<32;++k){const auto old=live;bool failed=false;countdown=k;try{UF u(source);countdown=-1;unchanged(u,snap);}catch(const std::bad_alloc&){countdown=-1;failed=true;++allocation_failures;}require(live==old);unchanged(source,snap);if(!failed){done=true;break;}}require(done);}
template<class Make,class Operation>static void allocating_operation(Make make,Operation operation){bool done=false;for(int k=0;k<256;++k){auto uf=make();const auto before=capture(uf);const auto old=live;bool failed=false;countdown=k;try{operation(uf);countdown=-1;}catch(const std::bad_alloc&){countdown=-1;failed=true;++allocation_failures;}if(failed){unchanged(uf,before);require(live==old);}else{done=true;break;}}require(done);}
static int growth_state(){UF u(1024);for(int i=0;i<300;++i){auto old=calls;u.unite(0,0,0);if(calls!=old)return i;}return -1;}
static void allocations(){constructors();auto source=prepared(65);const auto snap=capture(source);allocating_operation([]{return prepared();},[&](UF&u){u=source;});unchanged(source,snap);allocating_operation([]{return prepared();},[](UF&u){u.reset(1000);});allocating_operation([]{return prepared();},[](UF&u){u.reserve_history(10000);});allocating_operation([]{return prepared(19);},[](UF&u){auto groups=u.groups();require(!groups.empty());});const int growth=growth_state();require(growth>=0);for(int kind=0;kind<3;++kind)allocating_operation([&]{UF u(1024);for(int i=0;i<growth;++i)u.unite(0,0,0);u.snapshot();return u;},[&](UF&u){if(kind==0)require(u.unite(0,1,5));else if(kind==1)require(!u.unite(0,0,0));else require(u.unite_checked(0,0,1)==UF::MergeResult::Contradiction);});}
static void noalloc(){UF u(64);u.reserve_history(1000);const auto old=calls;countdown=0;for(int i=1;i<64;++i)u.unite(i-1,i,2);for(int i=0;i<64;++i){require(u.same(0,i));require(u.diff(0,i)==2*i);require(u.try_diff(0,i)==2*i);require(u.size(i)==64);require(u.consistent(0,i,2*i));}u.snapshot();u.unite(0,1,5);u.undo();u.rollback(0);UF moved(std::move(u));u=std::move(moved);countdown=-1;require(calls==old);
#ifdef SIRKIT_RWUF_REQUIRE_EXTRA
 for(int i=1;i<64;++i){u.unite(i-1,i,2);}u.snapshot();const int size=u.size(0),count=u.component_count();const auto old_calls=calls;countdown=0;u.clear_history();require(u.get_state()==0&&u.size(0)==size&&u.component_count()==count&&u.diff(0,63)==126);u.rollback();for(int i=0;i<800;++i)u.unite(0,63,126);for(int i=0;i<800;++i)u.undo();countdown=-1;require(calls==old_calls&&u.get_state()==0);static_assert(noexcept(u.clear_history()));
#endif
}
struct ValueError{};
struct Value{
 std::int64_t v=0;static inline int budget=-1;static inline std::ptrdiff_t owners=0;
 static void tick(){if(budget==0)throw ValueError();if(budget>0)--budget;}
 explicit Value(std::int64_t x=0):v(x){tick();++owners;}
 Value(const Value&x):v(x.v){tick();++owners;}Value(Value&&x):v(x.v){tick();++owners;}
 Value&operator=(const Value&x){tick();v=x.v;return *this;}Value&operator=(Value&&x){tick();v=x.v;return *this;}
 ~Value(){--owners;}
 friend Value operator+(const Value&a,const Value&b){tick();return Value(a.v+b.v);}friend Value operator-(const Value&a,const Value&b){tick();return Value(a.v-b.v);}friend bool operator==(const Value&a,const Value&b){tick();return a.v==b.v;}
};
using VUF=RollbackWeightedUnionFind<Value>;
struct VSnapshot{int n,state,count;std::vector<int>roots;std::vector<std::int64_t>weights;};
static VSnapshot vcapture(const VUF&u){VSnapshot s{u.size(),u.get_state(),u.component_count(),{}, {}};for(int i=0;i<u.size();++i){s.roots.push_back(u.find(i));s.weights.push_back(u.get_weight(i).v);}return s;}
static void vunchanged(const VUF&u,const VSnapshot&s){const auto t=vcapture(u);require(t.n==s.n&&t.state==s.state&&t.count==s.count&&t.roots==s.roots&&t.weights==s.weights);}
static VUF vprepared(){VUF u(8);u.unite(0,1,Value(3));u.unite(2,3,Value(-2));u.unite(0,2,Value(1));u.snapshot();return u;}
template<class Op>static void value_operation(Op operation,bool expect_unchanged){bool done=false;for(int k=0;k<512;++k){VUF u=vprepared();const auto snap=vcapture(u);const auto owners=Value::owners;bool failed=false;Value::budget=k;try{operation(u);Value::budget=-1;}catch(const ValueError&){Value::budget=-1;failed=true;++value_failures;}if(failed){if(expect_unchanged)vunchanged(u,snap);require(Value::owners==owners);}else{done=true;break;}}require(done);}
static void value_assignment(){auto source=vprepared();const auto src=vcapture(source);bool done=false;for(int k=0;k<512;++k){VUF target(2);target.unite(0,1,Value(17));const auto before=vcapture(target);const auto owners=Value::owners;bool failed=false;Value::budget=k;try{target=source;Value::budget=-1;}catch(const ValueError&){Value::budget=-1;failed=true;++value_failures;}vunchanged(source,src);if(failed){vunchanged(target,before);require(Value::owners==owners);}else{vunchanged(target,src);done=true;break;}}require(done);}
static void values(){const auto owners=Value::owners;value_operation([](VUF&u){u.unite(1,4,Value(9));},true);value_operation([](VUF&u){u.unite(4,3,Value(9));},true);value_operation([](VUF&u){u.unite_checked(1,3,Value(-4));},true);value_operation([](VUF&u){(void)u.try_diff(1,3);},true);value_operation([](VUF&u){(void)u.diff(1,3);},true);value_operation([](VUF&u){(void)u.root_with_weight(3);},true);value_operation([](VUF&u){(void)u.get_weight(3);},true);value_operation([](VUF&u){(void)u.consistent(1,3,Value(-4));},true);value_operation([](VUF&u){VUF copy(u);},true);value_operation([](VUF&u){u.reset(20);},true);value_operation([](VUF&u){u.undo();},true);value_assignment();
#ifdef SIRKIT_RWUF_REQUIRE_EXTRA
 {auto u=vprepared();const auto state=vcapture(u);const auto old_calls=calls;Value::budget=0;countdown=0;u.clear_history();countdown=-1;Value::budget=-1;require(calls==old_calls&&u.get_state()==0&&u.component_count()==state.count);for(int i=0;i<u.size();++i)require(u.find(i)==state.roots[i]&&u.get_weight(i).v==state.weights[i]);}
#endif
 require(Value::owners==owners);}
int main(){const auto old=live;allocations();noalloc();values();require(live==old);require(Value::owners==0);std::printf("PASS checks=%zu allocation_failures=%zu value_failures=%zu live_delta=%td value_live=%td\n",checks,allocation_failures,value_failures,static_cast<std::ptrdiff_t>(live)-static_cast<std::ptrdiff_t>(old),Value::owners);}
