// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PUF_SOURCE
#define SIRKIT_PUF_SOURCE "../../../DataStructure/persistent-union-find.cpp"
#endif
#include SIRKIT_PUF_SOURCE
#include <cstdio>
#include <cstdlib>
#include <string>
#include <type_traits>

static std::size_t checks=0;
static void require(bool b){++checks;if(!b)std::abort();}
struct TrivialDefault {
    static inline int default_calls=0;
    int value;
    TrivialDefault():value(-999){++default_calls;}
    explicit TrivialDefault(int x):value(x){}
    friend bool operator==(const TrivialDefault&,const TrivialDefault&)=default;
};
struct CopyOnly {
    int value;
    CopyOnly()=delete;
    explicit CopyOnly(int x):value(x){}
    CopyOnly(const CopyOnly&)=default;
    CopyOnly(CopyOnly&&)=delete;
    CopyOnly&operator=(const CopyOnly&)=default;
    CopyOnly&operator=(CopyOnly&&)=delete;
    friend bool operator==(const CopyOnly&,const CopyOnly&)=default;
};
struct alignas(128) Aligned {
    int value;
    Aligned()=delete;
    explicit Aligned(int x):value(x){}
    friend bool operator==(const Aligned&,const Aligned&)=default;
};
static_assert(std::is_trivially_copyable_v<TrivialDefault>);
static_assert(!std::is_trivially_default_constructible_v<TrivialDefault>);
static_assert(std::is_trivially_copyable_v<CopyOnly>);
static_assert(std::is_copy_constructible_v<CopyOnly> && !std::is_move_constructible_v<CopyOnly>);
static_assert(std::is_trivially_copyable_v<Aligned>);

template<class T,int Shift,class Factory>
void exercise(Factory make){
    using Array=PersistentArray<T,Shift>;
    const int n=(1<<Shift)+3;
    const T id=make(-7),x=make(123),y=make(456);
    std::vector<T> input;
    for(int i=0;i<n;++i){const T value=make(i);input.push_back(value);}
    Array a(input,id);
    const auto initial=a.root;
    for(int i=0;i<n;++i)require(a.get(i)==make(i));
    a.update(n-1,x);
    const auto single=a.root;
    a.update_two(initial,0,x,n-1,y);
    const auto pair=a.root;
    for(int i=0;i<n;++i){
        require(a.get(initial,i)==make(i));
        require(a.get(single,i)==(i==n-1?x:make(i)));
        require(a.get(pair,i)==(i==0?x:(i==n-1?y:make(i))));
    }
    auto copy=a;
    a.update_two(0,x,0,y);
    require(a.get(0)==y&&copy.get(0)==x);
    a.update_two(nullptr,0,x,n-1,y);
    for(int i=0;i<n;++i)require(a.get(i)==(i==0?x:(i==n-1?y:id)));
    a.restore(initial);
    require(a.get(0)==make(0)&&a.get(n-1)==make(n-1));
    Array survivor(0,id);
    {Array owner(n,id);owner.update_two(0,x,n-1,y);survivor=owner;}
    require(survivor.get(0)==x&&survivor.get(n-1)==y);
    auto moved=std::move(survivor);
    require(survivor.size()==0&&survivor.root==nullptr);
    require(moved.get(0)==x&&moved.get(n-1)==y);
    // Nodeの公開構築/コピーも、Tに余分な既定構築やムーブを要求しない。
    typename Array::Node leaf(true,id,nullptr),branch(false,id,nullptr);
    typename Array::Node leaf_copy(leaf),branch_copy(branch);
    (void)leaf_copy;(void)branch_copy;
}
int main(){
    exercise<TrivialDefault,4>([](int x){return TrivialDefault(x);});
    exercise<CopyOnly,4>([](int x){return CopyOnly(x);});
    exercise<Aligned,1>([](int x){return Aligned(x);});
    exercise<Aligned,8>([](int x){return Aligned(x);});
    exercise<std::string,4>([](int x){return std::to_string(x)+std::string(32,'x');});
    exercise<double,4>([](int x){return static_cast<double>(x)+0.25;});
    require(TrivialDefault::default_calls==0);
    std::printf("PASS generic layout checks=%zu default_calls=%d\n",checks,TrivialDefault::default_calls);
}
