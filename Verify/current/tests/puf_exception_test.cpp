// SPDX-License-Identifier: CC0-1.0
#ifndef SIRKIT_PUF_REQUIRE_EXTRA
#define SIRKIT_PUF_REQUIRE_EXTRA
#endif
#ifndef SIRKIT_PUF_SOURCE
#define SIRKIT_PUF_SOURCE "../../../DataStructure/persistent-union-find.cpp"
#endif
#include SIRKIT_PUF_SOURCE
#include <cstdio>
#include <cstdlib>
#include <new>
#include <exception>
static std::ptrdiff_t allocation_budget=-1;
static std::size_t blocks=0,checks=0,allocation_failures=0,value_failures=0;
[[gnu::noinline]]void*operator new(std::size_t n){if(allocation_budget==0)throw std::bad_alloc();if(allocation_budget>0)--allocation_budget;if(void*p=std::malloc(n?n:1)){++blocks;return p;}throw std::bad_alloc();}
[[gnu::noinline]]void*operator new[](std::size_t n){return::operator new(n);}
[[gnu::noinline]]static void free_block(void*p)noexcept{if(p)--blocks;std::free(p);}
void operator delete(void*p)noexcept{free_block(p);}void operator delete[](void*p)noexcept{free_block(p);}void operator delete(void*p,std::size_t)noexcept{free_block(p);}void operator delete[](void*p,std::size_t)noexcept{free_block(p);}
static void demand(bool value,const char* expression,int line){++checks;if(!value){std::fprintf(stderr,"line %d: %s\n",line,expression);std::abort();}}
#define require(x) demand(bool(x),#x,__LINE__)
struct ValueFailure:std::exception{};
struct Throwing {
    static inline int live=0,copy_budget=-1,assignment_budget=-1;
    int value;
    Throwing()=delete;
    explicit Throwing(int x):value(x){++live;}
    Throwing(const Throwing&other):value(other.value){if(copy_budget==0)throw ValueFailure{};if(copy_budget>0)--copy_budget;++live;}
    Throwing(Throwing&&other)noexcept:value(other.value){++live;}
    Throwing&operator=(const Throwing&other){if(assignment_budget==0)throw ValueFailure{};if(assignment_budget>0)--assignment_budget;value=other.value;return *this;}
    ~Throwing(){--live;}
};
static void constructors(){
    for(int failure=0;failure<32;++failure){const auto before=blocks;bool caught=false;allocation_budget=failure;try{PersistentUnionFind uf(100000);}catch(const std::bad_alloc&){caught=true;}allocation_budget=-1;require(blocks==before);if(caught)++allocation_failures;else break;}
    for(int failure=0;failure<32;++failure){const auto before=blocks;bool caught=false;allocation_budget=failure;try{PersistentArray<int,1>array(std::vector<int>{1,2,3,4,5,6,7,8,9});}catch(const std::bad_alloc&){caught=true;}allocation_budget=-1;require(blocks==before);if(caught)++allocation_failures;else break;}
}
static void uf_failures(){
    int caught_count=0;
    for(int failure=0;failure<128;++failure){const auto before=blocks;bool caught=false;{
        PersistentUnionFind uf(1<<20);const auto initial=uf.root;uf.unite(1,2);const auto old=uf.root;auto sibling=uf;const auto old_arr=uf.arr.root;allocation_budget=failure;
        try{require(uf.unite(3,4,initial));}catch(const std::bad_alloc&){caught=true;}
        allocation_budget=-1;
        require(uf.same(1,2,old)&&!uf.same(3,4,old));require(!uf.same(1,2,initial));require(sibling.root==old&&sibling.arr.root==old_arr);
        if(caught){++caught_count;++allocation_failures;require(uf.root==old&&uf.arr.root==old_arr);require(uf.same(1,2)&&!uf.same(3,4));}
        else{require(uf.same(3,4)&&!uf.same(1,2));require(uf.root==uf.arr.root);}
        uf.restore(old);require(uf.unite(5,6));require(uf.same(1,2)&&uf.same(5,6));require(!uf.same(5,6,old));
    }require(blocks==before);if(!caught)break;}
    if(caught_count==0){
        for(int failure=0;failure<16;++failure){const auto before=blocks;bool caught=false;{
            PersistentUnionFind uf(1<<20);const auto initial=uf.root;uf.unite(1,2);const auto historical=uf.root;
            allocation_budget=failure;
            for(int step=0;step<4096;++step){const auto old=uf.root,old_arr=uf.arr.root;try{uf.unite(3,4,initial);}catch(const std::bad_alloc&){caught=true;allocation_budget=-1;require(uf.root==old&&uf.arr.root==old_arr);break;}}
            allocation_budget=-1;require(uf.same(1,2,historical)&&!uf.same(3,4,historical));require(!uf.same(1,2,initial));uf.restore(historical);require(uf.unite(5,6));require(uf.same(1,2)&&uf.same(5,6));
        }require(blocks==before);if(caught){++caught_count;++allocation_failures;}else break;}
    }
    require(caught_count>0);
}
static void array_failures(){
    int caught_count=0;
    for(int failure=0;failure<128;++failure){const auto before=blocks;bool caught=false;{
        PersistentArray<int,1>array(std::numeric_limits<std::int64_t>::max(),-7);const auto initial=array.root;array.update(7,5);auto old=array.root;auto sibling=array;allocation_budget=failure;
        try{array.update(initial,std::numeric_limits<std::int64_t>::max()-1,14);}catch(const std::bad_alloc&){caught=true;}
        allocation_budget=-1;require(array.get(old,7)==5&&array.get(initial,7)==-7);require(sibling.root==old&&sibling.get(7)==5);
        if(caught){++caught_count;++allocation_failures;require(array.root==old);require(array.get(7)==5);}
        else{require(array.get(7)==-7&&array.get(std::numeric_limits<std::int64_t>::max()-1)==14);}
        array.update(old,15,8);require(array.get(7)==5&&array.get(15)==8);require(array.get(old,15)==-7);
    }require(blocks==before);if(!caught)break;}
    if(caught_count==0){
        for(int failure=0;failure<16;++failure){const auto before=blocks;bool caught=false;{
            PersistentArray<int,1>array(std::numeric_limits<std::int64_t>::max(),-7);const auto initial=array.root;array.update(7,5);const auto historical=array.root;
            allocation_budget=failure;
            for(int step=0;step<4096;++step){const auto old=array.root;try{array.update(initial,std::numeric_limits<std::int64_t>::max()-1,14);}catch(const std::bad_alloc&){caught=true;allocation_budget=-1;require(array.root==old);break;}}
            allocation_budget=-1;require(array.get(historical,7)==5&&array.get(initial,7)==-7);array.update(historical,15,8);require(array.get(7)==5&&array.get(15)==8);require(array.get(historical,15)==-7);
        }require(blocks==before);if(caught){++caught_count;++allocation_failures;}else break;}
    }
    require(caught_count>0);
}
static void value_failures_update(bool assignments){
    int caught_count=0;
    for(int failure=0;failure<200;++failure){const auto before=blocks;bool caught=false;{
        PersistentArray<Throwing,2>array(1050,Throwing(-9));array.update(3,Throwing(4));auto old=array.root;auto sibling=array;
        if(assignments)Throwing::assignment_budget=failure;else Throwing::copy_budget=failure;
        try{array.update(old,1049,Throwing(13));}catch(const ValueFailure&){caught=true;}
        Throwing::assignment_budget=Throwing::copy_budget=-1;
        require(array.get(old,3).value==4&&array.get(old,1049).value==-9);require(sibling.root==old&&sibling.get(1049).value==-9);
        if(caught){++caught_count;++value_failures;require(array.root==old);require(array.get(3).value==4&&array.get(1049).value==-9);}
        else require(array.get(1049).value==13&&array.get(3).value==4);
        array.update(old,1048,Throwing(12));require(array.get(1048).value==12&&array.get(old,1048).value==-9);
    }require(Throwing::live==0&&blocks==before);if(!caught)break;}
    require(caught_count>0);
}
#ifdef SIRKIT_PUF_REQUIRE_EXTRA
static void pair_value_failures(bool assignments,bool same_index){
    int caught_count=0;
    for(int failure=0;failure<300;++failure){const auto before=blocks;bool caught=false;{
        PersistentArray<Throwing,2>array(1050,Throwing(-9));array.update(3,Throwing(4));auto old=array.root;auto sibling=array;
        if(assignments)Throwing::assignment_budget=failure;else Throwing::copy_budget=failure;
        try{array.update_two(old,1049,Throwing(13),same_index?1049:1048,Throwing(14));}catch(const ValueFailure&){caught=true;}
        Throwing::assignment_budget=Throwing::copy_budget=-1;
        require(array.get(old,3).value==4&&array.get(old,1049).value==-9&&array.get(old,1048).value==-9);require(sibling.root==old&&sibling.get(1049).value==-9);
        if(caught){++caught_count;++value_failures;require(array.root==old);require(array.get(3).value==4&&array.get(1049).value==-9&&array.get(1048).value==-9);}
        else{require(array.get(1049).value==(same_index?14:13));require(array.get(1048).value==(same_index?-9:14));}
        array.update(old,1048,Throwing(12));require(array.get(1048).value==12&&array.get(old,1048).value==-9);
    }require(Throwing::live==0&&blocks==before);if(!caught)break;}
    require(caught_count>0);
}
#endif
static void value_failures_constructor(){
    int caught_count=0;
    for(int failure=0;failure<200;++failure){const auto before=blocks;bool caught=false;Throwing::copy_budget=failure;try{PersistentArray<Throwing,3>array(0,Throwing(-9));}catch(const ValueFailure&){caught=true;}Throwing::copy_budget=-1;require(Throwing::live==0&&blocks==before);if(caught){++caught_count;++value_failures;}else break;}
    require(caught_count>0);
    for(int failure=0;failure<50;++failure){const auto before=blocks;bool caught=false;{std::vector<Throwing>values;for(int i=0;i<18;++i)values.emplace_back(i);Throwing::assignment_budget=failure;try{PersistentArray<Throwing,2>array(values,Throwing(-9));}catch(const ValueFailure&){caught=true;}Throwing::assignment_budget=-1;for(int i=0;i<18;++i)require(values[i].value==i);}require(Throwing::live==0&&blocks==before);if(caught)++value_failures;else break;}
}
int main(){const auto initial_blocks=blocks;constructors();uf_failures();array_failures();value_failures_update(false);value_failures_update(true);value_failures_constructor();
#ifdef SIRKIT_PUF_REQUIRE_EXTRA
pair_value_failures(false,false);pair_value_failures(true,false);pair_value_failures(false,true);pair_value_failures(true,true);
#endif
require(Throwing::live==0&&blocks==initial_blocks);std::printf("PASS checks=%zu allocation_failures=%zu value_failures=%zu live_delta=0\n",checks,allocation_failures,value_failures);}
