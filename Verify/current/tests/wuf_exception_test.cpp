// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_WUF_REQUIRE_EXTRA
#define SIRKIT_WUF_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_WUF_SOURCE
#define SIRKIT_WUF_SOURCE "../../../DataStructure/union_find_w.cpp"
#endif
#include SIRKIT_WUF_SOURCE
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <type_traits>

static volatile std::ptrdiff_t countdown=-1;
static volatile std::size_t live=0,calls=0;
static std::size_t checks=0,allocation_failures=0,value_failures=0;
[[gnu::noinline]] void* operator new(std::size_t n) {
    if(countdown==0) throw std::bad_alloc();
    if(countdown>0) countdown=countdown-1;
    if(void* p=std::malloc(n?n:1)) { live=live+1; calls=calls+1; return p; }
    throw std::bad_alloc();
}
[[gnu::noinline]] void* operator new[](std::size_t n) { return ::operator new(n); }
[[gnu::noinline]] static void release(void* p) noexcept { if(p)live=live-1; std::free(p); }
void operator delete(void* p) noexcept { release(p); } void operator delete[](void* p) noexcept { release(p); }
void operator delete(void* p,std::size_t) noexcept { release(p); } void operator delete[](void* p,std::size_t) noexcept { release(p); }
static void require_impl(bool condition,const char* text,int line) { ++checks; if(!condition) { std::fprintf(stderr,"line %d: %s\n",line,text); std::abort(); } }
#define require(x) require_impl(bool(x),#x,__LINE__)
struct ValueFailure {};
struct Value {
    static inline int alive=0,until_failure=-1;
    std::int64_t value=0;
    static void event() { if(until_failure==0) throw ValueFailure{}; if(until_failure>0)--until_failure; }
    Value() { event(); ++alive; }
    explicit Value(std::int64_t x) : value(x) { event(); ++alive; }
    Value(const Value& other) : value(other.value) { event(); ++alive; }
    Value(Value&& other) : value(other.value) { event(); ++alive; }
    Value& operator=(const Value& other) { event(); value=other.value; return *this; }
    Value& operator=(Value&& other) { event(); value=other.value; return *this; }
    ~Value() { --alive; }
    Value& operator+=(const Value& rhs) { event(); value+=rhs.value; return *this; }
    friend Value operator+(const Value& a,const Value& b) { return Value(a.value+b.value); }
    friend Value operator-(const Value& a,const Value& b) { return Value(a.value-b.value); }
    friend Value operator-(const Value& a) { return Value(-a.value); }
    friend bool operator==(const Value& a,const Value& b) { return a.value==b.value; }
};
template<class T> std::int64_t raw(const T& value) { if constexpr(std::is_same_v<T,Value>) return value.value; else return value; }
template<class T> T number(std::int64_t x) { return T(x); }
struct Snapshot { int n,components; std::vector<int> roots,sizes; std::vector<std::int64_t> weights; };
template<class T> Snapshot capture(UnionFindW<T>& uf) {
    Snapshot out{uf.size(),uf.component_count(),{},{},{}};
    for(int i=0;i<uf.size();++i) { out.roots.push_back(uf.root(i)); out.sizes.push_back(uf.size(i)); out.weights.push_back(raw(uf.weight(i))); }
    return out;
}
template<class T> void unchanged(UnionFindW<T>& uf,const Snapshot& old) {
    require(uf.size()==old.n); require(uf.component_count()==old.components);
    for(int i=0;i<old.n;++i) {
        require(uf.root(i)==old.roots[i]); require(uf.size(i)==old.sizes[i]); require(raw(uf.weight(i))==old.weights[i]);
        for(int j=0;j<old.n;++j) { require(uf.same(i,j)==(old.roots[i]==old.roots[j])); if(old.roots[i]==old.roots[j]) require(raw(uf.diff(i,j))==old.weights[j]-old.weights[i]); }
    }
}
template<class T> void prepare(UnionFindW<T>& uf) {
    for(int i=0;i+3<uf.size();i+=4) { uf.merge(i+3,i,number<T>(5)); uf.merge(i+2,i+1,number<T>(-7)); uf.merge(i+2,i+3,number<T>(11)); }
}
static void allocation_constructor() {
    bool success=false;
    for(int k=0;k<16;++k) { const auto before=live; bool failed=false; countdown=k;
        try { UnionFindW<std::int64_t> uf(65); countdown=-1; require(uf.size()==65&&uf.component_count()==65); }
        catch(const std::bad_alloc&) { countdown=-1; failed=true; ++allocation_failures; }
        require(live==before); if(!failed) { success=true; break; }
    } require(success);
}
static void allocation_copy() {
    UnionFindW<std::int64_t> source(65); prepare(source); const auto src=capture(source); bool success=false;
    for(int k=0;k<16;++k) { const auto before=live; bool failed=false; countdown=k;
        try { auto copy=source; countdown=-1; unchanged(copy,src); }
        catch(const std::bad_alloc&) { countdown=-1; failed=true; ++allocation_failures; }
        unchanged(source,src); require(live==before); if(!failed) { success=true; break; }
    } require(success);
}
static void allocation_assignment() {
    UnionFindW<std::int64_t> source(65); prepare(source); const auto src=capture(source); bool success=false;
    for(int k=0;k<16;++k) { UnionFindW<std::int64_t> target(3); target.unite(0,2,3); const auto old=capture(target); const auto before=live; bool failed=false; countdown=k;
        try { target=source; countdown=-1; }
        catch(const std::bad_alloc&) { countdown=-1; failed=true; ++allocation_failures; }
        unchanged(source,src); require(live==before); if(failed)unchanged(target,old);else { unchanged(target,src); success=true; break; }
    } require(success);
}
static void allocation_groups() {
    UnionFindW<std::int64_t> source(65); prepare(source); const auto src=capture(source); const auto expected=source.groups(); bool success=false;
    for(int k=0;k<128;++k) { const auto before=live; bool failed=false; countdown=k;
        try { auto groups=source.groups(); countdown=-1; require(groups==expected); }
        catch(const std::bad_alloc&) { countdown=-1; failed=true; ++allocation_failures; }
        unchanged(source,src); require(live==before); if(!failed) { success=true; break; }
    } require(success);
}
static void nonallocating() {
    UnionFindW<std::int64_t> a(128),b(128); const auto before=calls; countdown=0;
    for(int step=1;step<128;step*=2)for(int i=0;i<128;i+=2*step) { require(a.merge(i,i+step,step)); b.unite(i+step,i,-step); }
    for(int i=0;i<128;++i) { require(a.root(i)==0); require(a.leader(i)==0); require(a.same(i,127-i)); require(a.issame(i,127-i)); require(a.size(i)==128); require(a.weight(i)==i); require(a.diff(0,i)==i); require(a.try_diff(i,0)==-i); require(a.constrain(0,i,i)); require(!a.merge(i,0,999)); }
    auto* alias=&a; a=*alias; a=std::move(*alias); UnionFindW<std::int64_t> moved(std::move(a)); a=std::move(moved); a.swap(b); a.swap(b);
#ifdef SIRKIT_WUF_REQUIRE_EXTRA
    static_assert(noexcept(std::declval<UnionFindW<std::int64_t>&>().clear())); static_assert(noexcept(std::declval<UnionFindW<std::int64_t>&>().empty()));
    swap(a,b); swap(a,b); const auto live_before=live; a.clear(); a.clear(); require(a.empty()&&a.size()==0&&a.component_count()==0&&a.groups().empty()); require(live==live_before);
#endif
    countdown=-1; require(calls==before); require(moved.size()==0&&moved.component_count()==0);
}
static void value_construction_copy() {
    const auto before=live; const auto values_before=Value::alive;
    for(int mode=0;mode<3;++mode) {
        bool success=false;
        for(int k=0;k<128;++k) {
            const auto old_live=live; const auto old_values=Value::alive; bool failed=false;
            {
                UnionFindW<Value> source(17); prepare(source); const auto src=capture(source); UnionFindW<Value> target(3); target.unite(0,2,Value(3)); const auto dst=capture(target);
                Value::until_failure=k;
                try {
                    if(mode==0) { UnionFindW<Value> constructed(17); Value::until_failure=-1; require(constructed.size()==17); }
                    else if(mode==1) { auto copied=source; Value::until_failure=-1; unchanged(copied,src); }
                    else { target=source; Value::until_failure=-1; unchanged(target,src); }
                } catch(const ValueFailure&) { Value::until_failure=-1; failed=true; ++value_failures; if(mode==2)unchanged(target,dst); }
                unchanged(source,src);
            }
            require(live==old_live); require(Value::alive==old_values);
            if(!failed) { success=true; break; }
        } require(success);
    }
    require(live==before); require(Value::alive==values_before);
}
#ifdef SIRKIT_WUF_REQUIRE_EXTRA
static void reset_failures() {
    bool success=false;
    for(int k=0;k<16;++k) { UnionFindW<std::int64_t> uf(65); prepare(uf); const auto old=capture(uf); const auto before=live; bool failed=false; countdown=k;
        try { uf.reset(128); countdown=-1; }
        catch(const std::bad_alloc&) { countdown=-1; failed=true; ++allocation_failures; }
        require(live==before); if(failed)unchanged(uf,old);else { require(uf.size()==128&&uf.component_count()==128); for(int i=0;i<128;++i)require(uf.root(i)==i&&uf.weight(i)==0); success=true; break; }
    } require(success);
    success=false;
    for(int k=0;k<128;++k) { const auto before=live; const auto values=Value::alive; bool failed=false;
        { UnionFindW<Value> uf(17); prepare(uf); const auto old=capture(uf); Value::until_failure=k;
          try { uf.reset(23); Value::until_failure=-1; require(uf.size()==23&&uf.component_count()==23); }
          catch(const ValueFailure&) { Value::until_failure=-1; failed=true; ++value_failures; unchanged(uf,old); }
        }
        require(live==before); require(Value::alive==values); if(!failed) { success=true; break; }
    } require(success);
}
#endif
int main() {
    const auto before=live; allocation_constructor(); allocation_copy(); allocation_assignment(); allocation_groups(); nonallocating(); value_construction_copy();
#ifdef SIRKIT_WUF_REQUIRE_EXTRA
    reset_failures();
#endif
    require(live==before); require(Value::alive==0);
    std::printf("PASS checks=%zu allocation_failures=%zu value_failures=%zu live_delta=%td value_live=%d\n",checks,allocation_failures,value_failures,static_cast<std::ptrdiff_t>(live)-static_cast<std::ptrdiff_t>(before),Value::alive);
}
