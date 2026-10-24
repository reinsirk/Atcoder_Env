// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_WUF_REQUIRE_EXTRA
#define SIRKIT_WUF_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_WUF_SOURCE
#define SIRKIT_WUF_SOURCE "../../../DataStructure/union_find_w.cpp"
#endif
#include SIRKIT_WUF_SOURCE
#include <array>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <type_traits>

namespace {
using Big = boost::multiprecision::cpp_int;
std::uint64_t checks=0,states=0,operations=0;
void require(bool condition) { ++checks; if(!condition) throw std::runtime_error("weighted union find reference mismatch"); }
struct Mod {
    static constexpr std::uint64_t modulus=998244353;
    std::uint64_t x=0;
    Mod()=default;
    explicit Mod(std::int64_t value) : x(static_cast<std::uint64_t>((value%static_cast<std::int64_t>(modulus)+static_cast<std::int64_t>(modulus))%static_cast<std::int64_t>(modulus))) {}
    Mod& operator+=(const Mod& rhs) { x=(x+rhs.x)%modulus; return *this; }
    friend Mod operator+(Mod lhs,const Mod& rhs) { lhs+=rhs; return lhs; }
    friend Mod operator-(const Mod& lhs,const Mod& rhs) { return Mod(static_cast<std::int64_t>(lhs.x)-static_cast<std::int64_t>(rhs.x)); }
    friend Mod operator-(const Mod& value) { return Mod(-static_cast<std::int64_t>(value.x)); }
    friend bool operator==(const Mod&,const Mod&)=default;
};
template<class T> Big to_big(const T& value) {
    if constexpr(std::is_same_v<T,Mod>) return value.x;
    else return value;
}
template<class T> Big norm(Big value) {
    if constexpr(std::is_same_v<T,Mod>) { value%=Mod::modulus; if(value<0)value+=Mod::modulus; }
    else if constexpr(std::is_unsigned_v<T>) { const Big modulus=Big{1}<<std::numeric_limits<T>::digits; value%=modulus; if(value<0)value+=modulus; }
    return value;
}
template<class T> T from_big(const Big& value) {
    const Big v=norm<T>(value);
    if constexpr(std::is_same_v<T,Mod>) return Mod(v.convert_to<std::int64_t>());
    else return v.convert_to<T>();
}
template<class T> struct Model {
    std::vector<int> label,count;
    std::vector<Big> potential;
    int components;
    explicit Model(int n) : label(n),count(n,1),potential(n),components(n) { std::iota(label.begin(),label.end(),0); }
    Big difference(int a,int b) const { return norm<T>(potential[b]-potential[a]); }
    bool connected(int a,int b) const { return label[a]==label[b]; }
    bool add(int a,int b,const T& w,bool consistency) {
        int ra=label[a],rb=label[b];
        if(ra==rb) return consistency && difference(a,b)==to_big(w);
        Big shift=norm<T>(to_big(w)+potential[a]-potential[b]);
        if(count[ra]<count[rb]) { std::swap(ra,rb); shift=norm<T>(-shift); }
        for(std::size_t i=0;i<label.size();++i)if(label[i]==rb) { label[i]=ra; potential[i]=norm<T>(potential[i]+shift); }
        count[ra]+=count[rb]; count[rb]=0; --components; return true;
    }
    std::vector<std::vector<int>> groups() const {
        std::vector<std::vector<int>> result;
        for(std::size_t root=0;root<label.size();++root) if(count[root]) {
            result.emplace_back();
            for(std::size_t v=0;v<label.size();++v)if(label[v]==static_cast<int>(root))result.back().push_back(static_cast<int>(v));
        }
        return result;
    }
};
template<class T> void observe(UnionFindW<T>& uf,const Model<T>& model,bool all_pairs=true) {
    ++states; require(uf.size()==static_cast<int>(model.label.size())); require(uf.component_count()==model.components);
    for(int i=0;i<uf.size();++i) {
        require(uf.root(i)==model.label[i]); require(uf.leader(i)==model.label[i]); require(uf.size(i)==model.count[model.label[i]]);
        require(to_big(uf.weight(i))==model.potential[i]);
        if(all_pairs)for(int j=0;j<uf.size();++j) {
            const bool same=model.connected(i,j); require(uf.same(i,j)==same); require(uf.issame(i,j)==same);
            const auto diff=uf.try_diff(i,j); require(diff.has_value()==same);
            if(same) { require(to_big(*diff)==model.difference(i,j)); require(to_big(uf.diff(i,j))==model.difference(i,j)); }
        }
    }
    require(uf.groups()==model.groups());
#ifdef SIRKIT_WUF_REQUIRE_EXTRA
    require(uf.empty()==model.label.empty());
#endif
}
template<class T> void random_sequences(std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    for(int trial=0;trial<20;++trial) {
        const int n=1+static_cast<int>(rng()%31); UnionFindW<T> uf(n); Model<T> model(n);
        for(int op=0;op<650;++op) {
            ++operations; const int a=static_cast<int>(rng()%n),b=static_cast<int>(rng()%n),kind=static_cast<int>(rng()%10);
            T w=from_big<T>(Big(static_cast<int>(rng()%201)-100));
            if(model.connected(a,b) && rng()%3==0) w=from_big<T>(model.difference(a,b));
            if(kind<=2) { const bool expected=model.add(a,b,w,false); require(uf.merge(a,b,w)==expected); }
            else if(kind==3) { uf.unite(a,b,w); (void)model.add(a,b,w,false); }
            else if(kind<=5) require(uf.constrain(a,b,w)==model.add(a,b,w,true));
            else if(kind==6) { require(uf.same(a,b)==model.connected(a,b)); const auto diff=uf.try_diff(a,b); require(diff.has_value()==model.connected(a,b)); if(diff)require(to_big(*diff)==model.difference(a,b)); }
            else if(kind==7) { auto copy=uf; observe(copy,model); if(n>1) { const T zero{}; copy.unite(0,n-1,zero); require(uf.component_count()==model.components); } }
            else if(kind==8) { UnionFindW<T> copy; copy=uf; auto moved=std::move(copy); require(copy.size()==0&&copy.component_count()==0); copy=std::move(moved); require(moved.size()==0); copy.swap(uf); observe(copy,model); }
            else { auto* alias=&uf; uf=*alias; uf=std::move(*alias); }
            if(op%47==0) observe(uf,model);
        }
        observe(uf,model);
#ifdef SIRKIT_WUF_REQUIRE_EXTRA
        auto copy=uf; copy.clear(); require(copy.empty()&&copy.size()==0&&copy.component_count()==0); require(copy.groups().empty());
        copy.reset(static_cast<std::size_t>(n)); Model<T> fresh(n); observe(copy,fresh); swap(copy,uf); observe(copy,model); observe(uf,fresh);
        copy.reset(0); require(copy.empty()); copy.reset(3); observe(copy,Model<T>(3));
#endif
    }
}
template<class T> void balanced() {
    constexpr int n=32768; UnionFindW<T> uf(n); std::vector<Big> abs(n);
    for(int i=0;i<n;++i)abs[i]=norm<T>(Big((i*37)%1000-500));
    for(int step=1;step<n;step*=2)for(int i=0;i<n;i+=2*step) { const T w=from_big<T>(abs[i+step]-abs[i]); require(uf.merge(i+step,i,from_big<T>(-to_big(w)))); ++operations; }
    require(uf.component_count()==1); const int root=n-1;
    for(int i=n-1;i>=0;--i) { require(uf.root(i)==root); require(to_big(uf.weight(i))==norm<T>(abs[i]-abs[root])); require(to_big(uf.diff(root,i))==norm<T>(abs[i]-abs[root])); }
    const auto groups=uf.groups(); require(groups.size()==1&&groups[0].size()==n);
    for(int i=0;i<n;++i)require(groups[0][i]==i);
}
template<class E,class F> void expect(F&& f) { bool caught=false; try { f(); } catch(const E&) { caught=true; } require(caught); }
void bounds() {
    UnionFindW<std::int64_t> empty; require(empty.size()==0&&empty.component_count()==0&&empty.groups().empty());
    expect<std::length_error>([]{UnionFindW<std::int64_t> uf(static_cast<std::size_t>(std::numeric_limits<int>::max())+1);});
    expect<std::length_error>([]{UnionFindW<std::int64_t> uf(std::numeric_limits<std::size_t>::max());});
#ifndef SKIP_RUNTIME_CHECKS
    UnionFindW<std::int64_t> uf(3);
    for(int bad:{-1,3,std::numeric_limits<int>::min(),std::numeric_limits<int>::max()}) {
        expect<std::out_of_range>([&]{(void)uf.root(bad);}); expect<std::out_of_range>([&]{(void)uf.leader(bad);});
        expect<std::out_of_range>([&]{(void)uf.size(bad);}); expect<std::out_of_range>([&]{(void)uf.weight(bad);});
        expect<std::out_of_range>([&]{(void)uf.same(bad,bad);}); expect<std::out_of_range>([&]{(void)uf.issame(0,bad);});
        expect<std::out_of_range>([&]{(void)uf.merge(bad,bad,0);}); expect<std::out_of_range>([&]{uf.unite(0,bad,0);});
        expect<std::out_of_range>([&]{(void)uf.constrain(bad,bad,0);}); expect<std::out_of_range>([&]{(void)uf.try_diff(bad,0);}); expect<std::out_of_range>([&]{(void)uf.diff(0,bad);});
    }
    expect<std::invalid_argument>([&]{(void)uf.diff(0,1);}); expect<std::out_of_range>([&]{(void)empty.same(0,0);});
#endif
#ifdef SIRKIT_WUF_REQUIRE_EXTRA
    UnionFindW<std::int64_t> reset_uf(2); reset_uf.merge(0,1,7);
    expect<std::length_error>([&]{reset_uf.reset(static_cast<std::size_t>(std::numeric_limits<int>::max())+1);}); require(reset_uf.diff(0,1)==7);
    expect<std::length_error>([&]{reset_uf.reset(std::numeric_limits<std::size_t>::max());}); require(reset_uf.diff(0,1)==7);
#endif
}
void endpoints() {
    using T=std::int64_t; constexpr T lo=std::numeric_limits<T>::min(),hi=std::numeric_limits<T>::max();
    for(T w:{lo,lo+1,T{-1},T{0},T{1},hi-1,hi}) { UnionFindW<T> uf(2); require(uf.merge(0,1,w)); require(uf.weight(0)==0&&uf.weight(1)==w); require(uf.diff(0,1)==w); require(uf.try_diff(0,1)==w); require(uf.constrain(0,1,w)); if(w!=lo)require(uf.diff(1,0)==-w); }
    UnionFindW<T> uf(4); const T q=hi/4; require(uf.merge(0,1,q)); require(uf.merge(2,3,-q)); require(uf.merge(0,2,q)); require(uf.diff(0,3)==0); require(uf.diff(1,3)==-q); require(uf.weight(3)==0);
    UnionFindW<T> reversed(4); reversed.unite(2,3,5); reversed.unite(0,2,7); require(reversed.root(0)==2); require(reversed.diff(0,3)==12); require(reversed.constrain(0,3,12));
}
#ifdef SIRKIT_WUF_REQUIRE_EXTRA
template<class T> void small_unsigned_regression() {
    UnionFindW<T> uf(3); require(uf.merge(0,1,static_cast<T>(std::numeric_limits<T>::max()-5))); require(uf.merge(0,2,static_cast<T>(5)));
    require(uf.constrain(1,2,static_cast<T>(11))); require(uf.diff(1,2)==static_cast<T>(11)); require(uf.try_diff(1,2)==static_cast<T>(11)); require(!uf.constrain(1,2,static_cast<T>(12)));
}
#endif
}
int main() {
    bounds(); endpoints(); random_sequences<std::int64_t>(61022); random_sequences<std::uint64_t>(61023); random_sequences<Mod>(61024); balanced<std::int64_t>(); balanced<Mod>();
#ifdef SIRKIT_WUF_REQUIRE_EXTRA
    small_unsigned_regression<std::uint8_t>(); small_unsigned_regression<std::uint16_t>(); random_sequences<std::uint8_t>(61025); random_sequences<std::uint16_t>(61026); balanced<std::uint8_t>();
#endif
    std::cout<<"PASS checks="<<checks<<" states="<<states<<" operations="<<operations<<'\n';
}
